!ifndef VERSION
  !define VERSION '0.3'
!endif

!ifdef OUTFILE
  OutFile "${OUTFILE}"
!else
  OutFile qtheorafrontend-${VERSION}-setup.exe
!endif

SetCompressor /SOLID lzma

InstType "Stable"
InstType "Experimental"

InstallDir $PROGRAMFILES\QTheoraFrontend

!include "MUI2.nsh"
!include "Sections.nsh"

Name "QTheoraFrontend"
Caption "QTheoraFrontend ${VERSION} Setup"

RequestExecutionLevel user

Var StartMenuFolder

!define MUI_ABORTWARNING
!define MUI_COMPONENTSPAGE_NODESC
!define MUI_STARTMENUPAGE_REGISTRY_ROOT "HKCU"
!define MUI_STARTMENUPAGE_REGISTRY_KEY "Software\QTheoraFrontend"
!define MUI_STARTMENUPAGE_REGISTRY_VALUENAME "Start Menu Folder"

!insertmacro MUI_PAGE_LICENSE "LICENSE.txt"
!insertmacro MUI_PAGE_COMPONENTS
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_STARTMENU "Application" $StartMenuFolder
!insertmacro MUI_PAGE_INSTFILES

!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES

Section "Base" Base
  SetOutPath $INSTDIR
  SectionIn 1 2 RO
  File "qtheorafrontend.exe"
  File "README.txt"
  File "LICENSE.txt"
  WriteUninstaller $INSTDIR\Uninstall.exe

  WriteRegStr HKCU "Software\QTheoraFrontend" "" $INSTDIR
  !insertmacro MUI_STARTMENU_WRITE_BEGIN Application
    CreateDirectory "$SMPROGRAMS\$StartMenuFolder"
    CreateShortCut "$SMPROGRAMS\$StartMenuFolder\QTheoraFrontend.lnk" "$INSTDIR\qtheorafrontend.exe"
    CreateShortCut "$SMPROGRAMS\$StartMenuFolder\Uninstall.lnk" "$INSTDIR\Uninstall.exe"
  !insertmacro MUI_STARTMENU_WRITE_END
SectionEnd

Section "ffmpeg2theora-0.24" libtheora10
  SectionIn 1
  SetOutPath $INSTDIR
  File /oname=ffmpeg2theora.exe "ffmpeg2theora-0.24.exe"
SectionEnd

Section "ffmpeg2theora-0.24-thusnelda" libtheora11
  SectionIn 2
  SetOutPath $INSTDIR
  File /oname=ffmpeg2theora.exe "ffmpeg2theora-0.24-thusnelda.exe"
SectionEnd

Section "Uninstall"
  !insertmacro MUI_STARTMENU_GETFOLDER Application $StartMenuFolder
  Delete "$SMPROGRAMS\$StartMenuFolder\QTheoraFrontend.lnk"
  Delete "$SMPROGRAMS\$StartMenuFolder\Uninstall.lnk"
  RMDir "$SMPROGRAMS\$StartMenuFolder"
  DeleteRegKey /ifempty HKCU "Software\QTheoraFrontend"

  Delete $INSTDIR\Uninstall.exe
  Delete $INSTDIR\qtheorafrontend.exe
  Delete $INSTDIR\ffmpeg2theora.exe
  Delete $INSTDIR\LICENSE.txt
  Delete $INSTDIR\README.txt
  RMDir $INSTDIR
SectionEnd

!insertmacro MUI_LANGUAGE "English"

LangString DESC_Base ${LANG_ENGLISH} "Base components."
LangString DESC_libtheora10 ${LANG_ENGLISH} "Base section."
LangString DESC_libtheora11 ${LANG_ENGLISH} "Base section."

;Assign language strings to sections
!insertmacro MUI_FUNCTION_DESCRIPTION_BEGIN
!insertmacro MUI_DESCRIPTION_TEXT ${Base} $(DESC_Base)
!insertmacro MUI_DESCRIPTION_TEXT ${libtheora10} $(DESC_libtheora10)
!insertmacro MUI_DESCRIPTION_TEXT ${libtheora11} $(DESC_libtheora11)
!insertmacro MUI_FUNCTION_DESCRIPTION_END

Function .onInit
  StrCpy $1 ${libtheora10}
FunctionEnd

Function .onSelChange
  !insertmacro StartRadioButtons $1
  !insertmacro RadioButton ${libtheora10}
  !insertmacro RadioButton ${libtheora11}
  !insertmacro EndRadioButtons
FunctionEnd

