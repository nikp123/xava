#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <sys/time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <assert.h>

#include <unistd.h>
#include <dirent.h>

#include "shared/log.h"
#include "shared.h"

#ifdef __WIN32__
#include <windows.h>
#endif

#ifdef __unix__
#include "shared/io/unix.h"
#endif

EXP_FUNC int xavaMkdir(const char *dir) {
    /* Stolen from: https://gist.github.com/JonathonReinhart/8c0d90191c38af2dcadb102c4e202950 */
    /* Adapted from http://stackoverflow.com/a/2336245/119527 */
    const size_t len = strlen(dir);
    char _path[MAX_PATH];
    char *p;

    errno = 0;

    /* Copy string so its mutable */
    if (len > sizeof(_path)-1) {
        errno = ENAMETOOLONG;
        return -1;
    }
    strcpy(_path, dir);

    /* Iterate the string */
    int offset = 1;
    #ifdef __WIN32__
        offset = 3;
    #endif
    for (p = _path + offset; *p; p++) {
        if (*p == DIRBRK) {
            /* Temporarily truncate */
            *p = '\0';

            if (mkdir(_path) != 0) {
                if (errno != EEXIST)
                    return -1;
            }

            *p = DIRBRK;
        }
    }

    if (mkdir(_path) != 0) {
        if (errno != EEXIST)
            return -1;
    }
    return 0;
}

// returned in UNIX time except milliseconds
EXP_FUNC unsigned long xavaGetTime(void) {
    #ifdef WIN
        return timeGetTime();
    #else
        struct timeval tv;
        gettimeofday(&tv, NULL);
        return tv.tv_sec*1000+tv.tv_usec/1000;
    #endif
}

#ifdef __WIN32__
// sleep in 100ns intervals
BOOLEAN __internal_xava_shared_io_sleep_windows(LONGLONG ns) {
    /* Declarations */
    HANDLE timer;   /* Timer handle */
    LARGE_INTEGER li;   /* Time defintion */
    /* Create timer */
    if(!(timer = CreateWaitableTimer(NULL, TRUE, NULL)))
        return FALSE;
    /* Set timer properties */
    li.QuadPart = -ns;
    if(!SetWaitableTimer(timer, &li, 0, NULL, NULL, FALSE)){
        CloseHandle(timer);
        return FALSE;
    }
    /* Start & wait for timer */
    WaitForSingleObject(timer, INFINITE);
    /* Clean resources */
    CloseHandle(timer);
    /* Slept without problems */
    return TRUE;
}
#endif

EXP_FUNC unsigned long xavaSleep(unsigned long oldTime, int framerate) {
    unsigned long newTime = 0;
    if(framerate) {
    #ifdef WIN
        newTime = xavaGetTime();
        if(newTime-oldTime<1000/framerate&&newTime>oldTime)
            __internal_xava_shared_io_sleep_windows(
                (1000/framerate-(newTime-oldTime)) * 10000);
        return xavaGetTime();
    #else
        newTime = xavaGetTime();
        if(oldTime+1000/framerate>newTime)
            usleep((1000/framerate+oldTime-newTime)*1000);
        return xavaGetTime();
    #endif
    }
    #ifdef WIN
    Sleep(oldTime);
    #else
    usleep(oldTime*1000);
    #endif
    return 0;
}

// XAVA event stack
EXP_FUNC void pushXAVAEventStack(XG_EVENT *stack, XG_EVENT event) {
  arr_add(stack, event);
}

EXP_FUNC XG_EVENT popXAVAEventStack(XG_EVENT *stack) { return arr_pop(stack); }

EXP_FUNC XG_EVENT *newXAVAEventStack() {
  XG_EVENT *stack;

  arr_init(stack);

  return stack;
}

EXP_FUNC void destroyXAVAEventStack(XG_EVENT *stack) { arr_free(stack); }

EXP_FUNC bool pendingXAVAEventStack(XG_EVENT *stack) {
  return (arr_count(stack) > 0) ? true : false;
}

// used for blocking in case an processing an event at the wrong
// time can cause disaster
EXP_FUNC bool isEventPendingXAVA(XG_EVENT *stack, XG_EVENT event) {
  int found;
  arr_find(stack, event, &found);

  return (found != -1);
}

/** 
 * @name Get the directory path of whatever file path string you specify.
 * @details Does NOT check if the string that was passed through is a path or not.
 *          The dir and file pointers are heap allocated so they must be freed.
 * @param source - The UNIX formatted path string containing our directory and file
 * @param dir - Pointer to where our output directory string will be written (can be NULL'd if you want no result)
 * @param file - Pointer to where our file name string will be written (can be NULL'd if you want no result)
**/
void splitDirAndFileFromUnixPathString(const char *source, char **dir, char **file) {
    // this special little routine copies the path from the filename (if there is any)
    const size_t path_size = strlen(source);

    for (size_t i=path_size; i; --i) {
        // Break path if it's found
        if (source[i] == DIRBRK) {
            if(dir != NULL)  *dir  = strndup(source, i);
            if(file != NULL) *file = strdup(source+i+1); 
            break;
        }
    }
}

/** 
 * @details Appends a UNIX style path to any OS string, does path break conversions for you.
 *          Does not do any allocations.
 * @param root Dynamically or statically allocated string, which is larger than root + relative
 * @param param Static string we want to append to root
**/
void appendUnixRelativePathToNative(char *root, const char *relative) {
    size_t og_root_offset = strlen(root);

    strcat(root, relative);

    // break if the native platform uses UNIX paths
    #if DIRBRK == '/'
        return; 
    #endif
    
    // Can be just a Windows thing, but just in case an OS uses some weird chars for dirs
    // this code can handle it
    for(size_t i = og_root_offset; i < strlen(root); i++) {
        if(root[i] == '/') root[i] = DIRBRK;
    }
}

// Returns true when a path exists, must be natively formatted
bool checkPath(char *path) {
    DIR *dir = opendir(path);
    // close the directory
    if(dir != NULL) { closedir(dir); return true; }

    // check errno if you want to handle errors
    return false;
}

/**
 * This function is all in one gigantic function which manages all the platform differences when
 * it comes to what kind of resource you're trying to find. For example: "I want a cache file on Windows"
 * and it returning the proper path with which you can access that resource, taking into consideration
 * all the specific platform differences that exist in the world.
 * 
 * @param type Defines what type of file resource you're trying to fetch
 * @param virtualFilePath Relative file path inside of the general resource folder for that system
 *  For example, when you have a config file on Linux, this appends that path to ~/.config/.
 * @param actualPath Pointer to the path 
 **/
EXP_FUNC bool xavaFindAndCheckFile(XF_TYPE type, const char *virtualFilePath, char **actualPath) {
    bool writeCheck = false;
    switch(type) {
        case XAVA_FILE_TYPE_CACHE:
            writeCheck = true;

            // TODO: When you use it, FINISH IT!
            {
                xavaError("XAVA_FILE_TYPE_CACHE is not implemented yet!");
                return false;
            }
            break;
        case XAVA_FILE_TYPE_CONFIG:
        case XAVA_FILE_TYPE_OPTIONAL_CONFIG:
        {
            #if defined(__unix__) // FOSS-y/Linux-y implementation
                char *configHome = getenv("XDG_CONFIG_HOME");

                if(configHome == NULL) {
                    xavaLog("XDG_CONFIG_HOME is not set. Assuming it's in the default path per XDG-spec.");

                    char *homeDir;
                    xavaErrorCondition((homeDir = getenv("HOME")) == NULL,
                            "This system is $HOME-less. Aborting execution...");

                    configHome = malloc(MAX_PATH);
                    assert(configHome != NULL);

                    // filename is added in post
                    sprintf(configHome, "%s/.config/%s/", homeDir, PACKAGE);
                    (*actualPath) = configHome;
                } else {
                    (*actualPath) = malloc((strlen(configHome)+strlen(PACKAGE)+strlen(virtualFilePath)+3)*sizeof(char));
                    assert((*actualPath) != NULL);

                    sprintf((*actualPath), "%s/%s/", configHome, PACKAGE);
                }
            #elif defined(__APPLE__) // App-lol implementation
                char *configHome = malloc(MAX_PATH);
                char *homeDir;

                // you must have a really broken system for this to be false
                if((homeDir = getenv("HOME")) == NULL) {
                    xavaError("macOS is $HOME-less. Bailing out!!!");
                    free(configHome);
                    return false;
                }

                sprintf(configHome, "%s//Library//Application Support//", homeDir);
                (*actualPath) = configHome;
            #elif defined(__WIN32__) // Windows-y implementation
                char *configHome = malloc(MAX_PATH);
                char *homeDir;

                // stop using windows 98 ffs
                if((homeDir = getenv("APPDATA")) == NULL) {
                    xavaError("XAVA could not find \%AppData\%! Something's REALLY wrong.");
                    free(configHome);
                    return false;
                }

                sprintf(configHome, "%s\\%s\\", homeDir, PACKAGE);
                (*actualPath) = configHome;
            #else
                #error "Platform not supported"
            #endif

            writeCheck = true;
            break;
        }
        case XAVA_FILE_TYPE_PACKAGE:
        {
            /**
              Lets settle on user-centric behaviour here,
              DO NOT expect to launch from the properties directory 
              but rather the build directory inside its
              Anything else won't work and WILL break.
            **/
            #if defined(__APPLE__)||defined(__unix__)
                char *path = malloc(MAX_PATH);
                char *ptr = find_prefix();
                strcpy(path, ptr);
                if(ptr != NULL) free(ptr);

                if(!checkPath(path)) {
                    strcpy(path, PREFIX"/share/"PACKAGE"/");
                }
                (*actualPath) = path;
            #elif defined(__WIN32__)
                // Windows uses widechars internally
                WCHAR wpath[MAX_PATH];
                char *path = malloc(MAX_PATH);

                // Get path of where the executable is installed
                HMODULE hModule = GetModuleHandleW(NULL);
                GetModuleFileNameW(hModule, wpath, MAX_PATH);
                wcstombs(path, wpath, MAX_PATH);

                // bullshit string hacks with nikp123 (ep. 1)
                for(int i = strlen(path)-1; i > 0; i--) { // go from end of the string
                    if(path[i] == DIRBRK) { // if we've hit a path, change the characters in front
                        path[i+1] = '\0'; // end the string after the path
                        break;
                    }
                }

                (*actualPath) = path;
            #else
                #error "Platform not supported"
            #endif
            writeCheck = false;
            break;
        }
        case XAVA_FILE_TYPE_CUSTOM_WRITE:
            writeCheck = true;
            goto yeet;
        case XAVA_FILE_TYPE_CUSTOM_READ:
        yeet:
            CALLOC_SELF((*actualPath), MAX_PATH);
            break;
        default:
        case XAVA_FILE_TYPE_NONE:
            xavaBail("Your code broke. The file type is invalid");
            break;
    }

    char *actualFileName, *actualFileDir;

    // this special little routine copies the path from the filename (if there is any)
    appendUnixRelativePathToNative(*actualPath, virtualFilePath);
    splitDirAndFileFromUnixPathString(*actualPath, &actualFileDir, &actualFileName);

    // config: create directory
    if(writeCheck)
        xavaMkdir(actualFileDir);

    // add filename
    switch(type) {
        case XAVA_FILE_TYPE_PACKAGE:
        case XAVA_FILE_TYPE_CONFIG:
        case XAVA_FILE_TYPE_OPTIONAL_CONFIG:
        case XAVA_FILE_TYPE_CACHE:
            // literal noop, this is intentional
            break;
        case XAVA_FILE_TYPE_CUSTOM_READ:
        case XAVA_FILE_TYPE_CUSTOM_WRITE:
            (*actualPath) = (char*)virtualFilePath;
            break;
        default:
            (*actualPath) = (char*)actualFileName;
            break;
    }

    switch(type) {
        case XAVA_FILE_TYPE_OPTIONAL_CONFIG:
        case XAVA_FILE_TYPE_CONFIG:
        {
            // don't be surprised if you find a lot of bugs here, beware!
            FILE *fp = fopen((*actualPath), "rb");
            if (fp != NULL) { // if the file already exists, just exit the routine
                fclose(fp);
                return true;
            }

            // If the file is *NOT* (no such file or directory) but some other error
            // ChatGPT says this is portable to Windows too, so idk
            xavaBailCondition(errno != ENOENT, 
                "File '%s' could not be opened due to reason: %s. Please check it!",
                (*actualPath), strerror(errno));

            xavaLog("File '%s' does not exist! Trying to make a new one...", (*actualPath), errno);

            /**
             * In case the user configuration file doesn't exist, try loading the one from the
             * XAVA installtaiton folder instead.
             **/
            char *packageFilePath;
            if(!xavaFindAndCheckFile(XAVA_FILE_TYPE_PACKAGE, virtualFilePath, &packageFilePath)) {
                // throw user error if the file is not optional
                xavaBailCondition(type == XAVA_FILE_TYPE_CONFIG,
                    "Could not find the file '%s' within the XAVA installation! Bailing out...", virtualFilePath);
  
                free(actualFileDir);
                free(actualFileName);
                free((*actualPath));
                return false;
            }

            // try to save the default file
            if(!xavaCopyFile(packageFilePath, (*actualPath))) {
                xavaBailCondition(type != XAVA_FILE_TYPE_OPTIONAL_CONFIG,
                    "Bailing because an required config file (%s) couldn't be generated.",
                    (*actualPath));

                xavaError("Could generate optional config file '%s'!", (*actualPath));
                free((*actualPath));
                free(actualFileDir);
                free(actualFileName);
                free(packageFilePath);
                return false;
            }

            free(actualFileDir); free(actualFileName); free(packageFilePath);

            xavaLog("Successfully created default config file on '%s'!", (*actualPath));            
            break; // this break doesn't do shit apart from shutting up the static analyser
        }
        default:
        {
            FILE *fp;
            if(writeCheck) {
                fp = fopen((*actualPath), "ab");
                if(fp == NULL) {
                    xavaLog("Could not open '%s' for writing! (error: %d)",
                        (*actualPath), strerror(errno));
                    switch(type) {
                        case XAVA_FILE_TYPE_PACKAGE:
                        case XAVA_FILE_TYPE_CACHE:
                            free((*actualPath));
                            break;
                        default:
                            break;
                    }
                    return false;
                }
            } else {
                fp = fopen((*actualPath), "rb");
                if(fp == NULL) {
                    xavaLog("Could not open '%s' for reading! (error: %s)",
                        (*actualPath), strerror(errno));
                    switch(type) {
                        case XAVA_FILE_TYPE_PACKAGE:
                        case XAVA_FILE_TYPE_CACHE:
                            free((*actualPath));
                            break;
                        default:
                            break;
                    }
                    return false;
                }
            }
            fclose(fp);
            return true;
            break; // this also never gets executed sadly
        }
    }

    return false; // if you somehow ended up here, it's probably something breaking so "failure" it is
}

EXP_FUNC RawData *xavaReadFile(const char *file) {
    RawData *data = malloc(sizeof(RawData));

    FILE *fp = fopen(file, "rb");
    if(fp == NULL) {
        return NULL;
    }

    fseek(fp, 0, SEEK_END);
    data->size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    data->data = malloc(data->size+1);
    if(data->data == NULL) {
        fclose(fp);
        return NULL;
    }

    // Assert just checks if the whole file was been able to be read in.
    // Never expected to fail unless we are literally dealing with the FS
    // dying.
    assert(1 == fread(data->data, data->size, 1, fp));

    ((char*)data->data)[data->size] = 0x00;

    #ifdef DEBUG
        xavaSpam("File %s start of size %d", file, data->size);
    #endif

    // makes sure to include the NULL byte
    data->size++;

    fclose(fp);

    return data;
}

EXP_FUNC bool xavaCopyFile(char *source, char *dest) {
    FILE *fs, *fd;

    fs = fopen(source, "rb"); // We already know that this path is correct
    if(fs == NULL) {
        xavaWarn("Unable to copy the source '%s' file!", source);
        return false;
    }

    fd = fopen(dest, "wb");
    if(fd == NULL) {
        xavaWarn("Unable to write to the destination '%s' file!", dest);
        fclose(fs);
        return false;
    }

    size_t filesize;

    // jump to end of file
    fseek(fs, 0, SEEK_END);
    // report offset == filesize
    filesize = ftell(fs);
    // jump back to beginning
    fseek(fs, 0, SEEK_SET);

    // allocate buffer
    void *fileBuffer = malloc(filesize);
    assert(fileBuffer);

    // read into buffer
    assert(1 == fread(fileBuffer, filesize, 1, fs));
    // write out that buffer
    assert(1 == fwrite(fileBuffer, filesize, 1, fd));
    // free the buffer
    free(fileBuffer);
    // close handles
    fclose(fs); fclose(fd);

    xavaSpam("Copied %d bytes from %s to %s", 
        filesize, source, dest);

    return true;
}

EXP_FUNC void xavaCloseFile(RawData *file) {
    free(file->data);
    free(file);
}



EXP_FUNC void *xavaDuplicateMemory(void *memory, size_t size) {
    void *duplicate = malloc(size+1);
    memcpy(duplicate, memory, size);
    return duplicate;
}

