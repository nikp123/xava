!include "MUI2.nsh"

# Set project name
Name "X.A.V.A."

!define PRODUCT_VERSION "0.6.2.0"
!define VERSION "0.6.2.0"

VIProductVersion "${PRODUCT_VERSION}"
VIFileVersion "${VERSION}"
VIAddVersionKey "FileVersion" "${VERSION}"
VIAddVersionKey "LegalCopyright" "(C) Nikola Pavlica."
VIAddVersionKey "FileDescription" "Graphical audio visualizer"

# installer filename
OutFile "xava-win-installer.exe"

# installation directory
InstallDir $PROGRAMFILES64\xava

# Get admin priviledges
RequestExecutionLevel admin

# start default section
Section
    # set the installation directory as the destination for the following actions
    SetOutPath $INSTDIR

    # include these files
    File xava.exe
    File README.md
    File LICENSE.txt
    File config.cfg
    File *.dll

    SetOutPath $APPDATA\xava\
 
    # create the uninstaller
    WriteUninstaller "$INSTDIR\uninstall.exe"
 
    # create a shortcut named "new shortcut" in the start menu programs directory
    # point the new shortcut at the program uninstaller
    CreateShortCut "$SMPROGRAMS\XAVA.lnk" "$INSTDIR\xava.exe"
    CreateShortCut "$SMPROGRAMS\Uninstall XAVA.lnk" "$INSTDIR\uninstall.exe"
SectionEnd

# uninstaller section start
Section "uninstall"
    # first, remove the link from the start menu
    Delete "$SMPROGRAMS\XAVA.lnk"
    Delete "$SMPROGRAMS\Uninstall XAVA.lnk"
 
    # second, delete the program
    RMDir /r "$INSTDIR"
# uninstaller section end
SectionEnd


!ifndef EM_SETCUEBANNER
!define EM_SETCUEBANNER 0x1501 ; NT5 w/Themes & Vista+
!endif

!define MUI_FINISHPAGE_NOAUTOCLOSE
!define MUI_INSTFILESPAGE_NOAUTOCLOSE

!InsertMacro MUI_PAGE_LICENSE LICENSE.txt

!insertmacro MUI_PAGE_DIRECTORY

!insertmacro MUI_PAGE_INSTFILES

# Markdown files dont work for some reason, so we have to do it like this
Function OpenREADME
    Exec 'notepad.exe "$INSTDIR\README.md"'
FunctionEnd

; Finish page
!define MUI_FINISHPAGE_RUN "$INSTDIR\xava.exe"
  !define MUI_FINISHPAGE_RUN_NOTCHECKED
!define MUI_FINISHPAGE_SHOWREADME
  !define MUI_FINISHPAGE_SHOWREADME_NOTCHECKED 
  !define MUI_FINISHPAGE_SHOWREADME_FUNCTION 'OpenREADME'
!define MUI_FINISHPAGE_LINK 'Check out the project page'
  !define MUI_FINISHPAGE_LINK_LOCATION https://www.github.com/nikp123/xava
!insertmacro MUI_PAGE_FINISH
!insertmacro MUI_LANGUAGE English

