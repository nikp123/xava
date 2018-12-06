!include "MUI.nsh"

# define name of installer
OutFile "xava-win-installer.exe"

# define installation directory
InstallDir $PROGRAMFILES\xava

# For removing Start Menu shortcut in Windows 7
RequestExecutionLevel admin

# start default section
Section
    # set the installation directory as the destination for the following actions
    SetOutPath $INSTDIR

    # include these files
    File xava.exe
    File portaudio-list.exe
    File README.txt
    File LICENSE.txt
    File *.dll

    SetOutPath $APPDATA\xava\

    File config
 
    # create the uninstaller
    WriteUninstaller "$INSTDIR\uninstall.exe"
 
    # create a shortcut named "new shortcut" in the start menu programs directory
    # point the new shortcut at the program uninstaller
    CreateShortCut "$SMPROGRAMS\XAVA-G.lnk" "$INSTDIR\xava.exe"

    Exec "$INSTDIR\portaudio-list.exe"
    Exec "$WINDIR\notepad.exe $INSTDIR\README.txt"
SectionEnd
 
# uninstaller section start
Section "uninstall"
 
    # first, delete the uninstaller
    Delete "$INSTDIR\uninstall.exe"
 
    # second, remove the link from the start menu
    Delete "$SMPROGRAMS\XAVA-G.lnk"
 
# uninstaller section end
SectionEnd
