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

/*
 * DISCLAIMER:
 *
 * The following logic assumes we are working with UTF-8 file names with UNIX directory breaks by default.
 * Please try to maintain that. xavaFindAndCheckFile should be the only function that turns those strings
 * into platform specific path types
*/

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
void splitDirAndFileFromNativePathString(const char *source, char **dir, char **file) {
    // this special little routine copies the path from the filename (if there is any)
    const size_t path_size = strlen(source);

    for (size_t i=path_size; i; --i) {
        // Break path if it's found
        if (source[i] == DIRBRK) {
            if(dir != NULL) {
                // strndup is not standard on Windows yet
                *dir = malloc(path_size+1);
                assert(*dir != NULL);
                strncpy(*dir, source, path_size+1);
                (*dir)[i] = '\0';
            }
            if(file != NULL) {
                *file = malloc(path_size-i);
                assert(*file != NULL);
                strncpy(*file, source+i+1, path_size-i);
            }
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

// Returns true if a file exists, must be natively formatted
bool checkFile(char *path) {
    FILE *fp = fopen(path, "rb");
    // close the file
    if(fp != NULL) { fclose(fp); return true; }

    // check errno if you want to handle errors
    return false;
}


char *xavaFileType_DiscoverPath(XF_TYPE type, const char *name) {
    char *path = malloc(MAX_PATH);
    assert(path != NULL);

    switch(type) {
        case XAVA_FILE_TYPE_CACHE: {
            // TODO: When you use it, FINISH IT!
            xavaError("XAVA_FILE_TYPE_CACHE is not implemented yet!");
            goto xavaFileType_DiscoverPath_defer;
            break;
        }
        case XAVA_FILE_TYPE_CONFIG:
        case XAVA_FILE_TYPE_OPTIONAL_CONFIG: {
            #if defined(__unix__) // FOSS-y/Linux-y implementation
                char *configHome = getenv("XDG_CONFIG_HOME");

                if(configHome == NULL) {
                    xavaLog("XDG_CONFIG_HOME is not set. Assuming it's in the default path per XDG-spec.");

                    char *homeDir = getenv("HOME");
                    if(homeDir == NULL) {
                        xavaError("This system is $HOME-less. Execution cannot continue!");
                        goto xavaFileType_DiscoverPath_defer;
                    }

                    snprintf(path, MAX_PATH, "%s/.config/%s/%s", homeDir, PACKAGE, name);
                } else {
                    snprintf(path, MAX_PATH, "%s/%s/%s", configHome, PACKAGE, name);
                }
            #elif defined(__APPLE__)
                char *homeDir = getenv("HOME");

                // you must have a really broken system for this to be true
                if(homeDir == NULL) {
                    xavaError("macOS is $HOME-less. Bailing out!!!");
                    goto xavaFileType_DiscoverPath_defer;
                }

                // do we need to correct for file paths here?
                snprintf(path, MAX_PATH, "%s//Library//Application Support//%s", homeDir, name);
            #elif defined(__WIN32__) // Windows-y implementation
                char *homeDir = getenv("APPDATA");

                // stop using windows 98 ffs
                if(homeDir == NULL) {
                    xavaError("XAVA could not find \%AppData\%! Something's REALLY wrong or you are using an ANCIENT OS.");
                    goto xavaFileType_DiscoverPath_defer;
                }

                // Windows' paths use backslashes, so we need to correct for that here
                char *new_name = malloc(strlen(name)+1);
                strcpy(new_name, name);
                for(size_t i = 0; i < strlen(name); i++) {
                    if(new_name[i] == '/') new_name[i] = '\\';
                }

                snprintf(path, MAX_PATH, "%s\\%s\\%s", homeDir, PACKAGE, new_name);
                free(new_name);
            #else
                #error "Platform not supported"
            #endif
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
                bool found = false;
                char *prefix = find_exe();  // Get full path of the current executable
                assert(prefix != NULL);     // Check if it's valid
                do {
                    char *new_prefix = br_dirname(prefix); // Drop top branch of the file path
                    assert(new_prefix != NULL);            // Check validity
                    free(prefix);                          // Clean last entry
                    prefix = new_prefix;                   // Update pointer

                    snprintf(path, MAX_PATH, "%s/%s", prefix, name);
                    if(checkFile(path)) {
                        xavaSpam("Guessing local file at '%s'", path);
                        found = true;
                        break;
                    }
                } while(strcmp(prefix, "/")); // Keep trying to find XAVAs' files until we run out of parent directories

                // Found a valid path, no need to discover further.
                if(found) break; 

                xavaSpam("None the binary paths' parents contain the file '%s'! Desparately trying to find it in PREFIX instead.", name);
                snprintf(path, MAX_PATH, PREFIX"/share/"PACKAGE"/%s", name);
            #elif defined(__WIN32__)
                // Windows uses widechars internally
                WCHAR wpath[MAX_PATH];

                // Get path of where the executable is installed
                HMODULE hModule = GetModuleHandleW(NULL);
                GetModuleFileNameW(hModule, wpath, MAX_PATH);
                wcstombs(path, wpath, MAX_PATH);

                // Get path from filename
                size_t i = 0;
                for(i = strlen(path)-1; i > 0; i--) { // go from end of the string
                    if(path[i] == DIRBRK) { // if we've hit a path, change the characters in front
                        path[i+1] = '\0'; // end the string after the path
                        break;
                    }
                }
                // append our desired file within that
                strncat(path, name, MAX_PATH-1);
                // convert UNIXy path to native
                for(; i<strlen(path); i++) { if(path[i] == '/') path[i] = '\\'; }
            #else
                #error "Platform not supported"
            #endif
            break;
        }
        default:
        case XAVA_FILE_TYPE_NONE:
            xavaBail("Your code broke. The file type is invalid");
            goto xavaFileType_DiscoverPath_defer;

    }

    return path;
xavaFileType_DiscoverPath_defer:
    free(path);
    return NULL;
}

/**
 * This function is all in one gigantic function which manages all the platform differences when
 * it comes to what kind of resource you're trying to find. For example: "I want a cache file on Windows"
 * and it returning the proper path with which you can access that resource, taking into consideration
 * all the specific platform differences that exist in the world.
 *
 * @param type Defines what type of file resource you're trying to fetch
 * @param name Name of the resource that is trying to be fetched. This name acts like a path
 * on top of the container's own path. For example, when you have a config file on Linux, this
 * appends that path to ~/.config/.
 * @return Heap-allocated value of the path that got discovered. NULL if the function failed.
 **/
EXP_FUNC char *xavaFindAndCheckFile(XF_TYPE type, const char *name) {
    char *path = malloc(MAX_PATH);
    assert(path != NULL);

    // check whether or not the file should be checked for the ability to write to it
    bool writeCheck = false;

    // correct the write check per type
    switch (type) {
        case XAVA_FILE_TYPE_CACHE:
        case XAVA_FILE_TYPE_CONFIG:
        case XAVA_FILE_TYPE_OPTIONAL_CONFIG:
            writeCheck = true; break;
        case XAVA_FILE_TYPE_PACKAGE:
        case XAVA_FILE_TYPE_NONE:
            writeCheck = false; break;
    }

    // we implement all the discovery stuff there and let this function deal
    // with whatever quirkyness the OS has (Linux is by FAR the worse here)
    char *discovered = xavaFileType_DiscoverPath(type, name);
    if(discovered == NULL) goto xavaFindAndCheckFile_defer;
    strncpy(path, discovered, MAX_PATH-1);
    free(discovered);

    // create directory if it doesn't exist at the moment
    if(writeCheck) {
        char *directory;
        splitDirAndFileFromNativePathString(path, &directory, NULL);

        if(checkPath(directory) == 0) {
            if(xavaMkdir(directory) != 0) {
                xavaError("Couldn't create directory '%s'!", directory);
                free(directory);
                goto xavaFindAndCheckFile_defer;
            }
        }

        free(directory);
    }

    /**
     * This logic implements what do we do in case we find or don't find that
     * specific file we just dicsovered the path to.
     **/
    FILE *fp = fopen(path, writeCheck ? "r+" : "r");
    if(fp == NULL) {
        if(type == XAVA_FILE_TYPE_CONFIG) {
            // yay, recursion
            char *install_path = xavaFindAndCheckFile(XAVA_FILE_TYPE_PACKAGE, name);
            if (install_path == NULL) goto xavaFindAndCheckFile_defer;

            if(!xavaCopyFile(install_path, path)) {
                xavaError("Couldn't copy '%s' to '%s'!", install_path, path);
                goto xavaFindAndCheckFile_defer;
            }
        } else {
            xavaError("Couldn't fetch resource '%s' at '%s'", name, path);
            goto xavaFindAndCheckFile_defer;
        }
    } else fclose(fp);

    // Then we FINALLY return the path that's (hopefully valid)
    return path;
xavaFindAndCheckFile_defer:
    free(path);
    return NULL;
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
    // dying or the program bugging out and copying empty files :(
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

