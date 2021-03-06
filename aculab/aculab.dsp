# Microsoft Developer Studio Project File - Name="aculab" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=aculab - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "aculab.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "aculab.mak" CFG="aculab - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "aculab - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "aculab - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "aculab - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /YX /FD /c
# ADD CPP /nologo /MD /W3 /GR /GX /O2 /I ".." /I "..\include" /I "v5" /D "NDEBUG" /D "_LIB" /D "_WINSTATIC" /D "WIN32" /D "_MBCS" /FD /c
# SUBTRACT CPP /YX
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ELSEIF  "$(CFG)" == "aculab - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /YX /FD /GZ /c
# ADD CPP /nologo /MDd /W3 /Gm /GR /GX /ZI /Od /I ".." /I "..\include" /I "v5" /D "_DEBUG" /D "_LIB" /D "_WINSTATIC" /D "WIN32" /D "_MBCS" /FR /FD /GZ /c
# SUBTRACT CPP /YX
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ENDIF 

# Begin Target

# Name "aculab - Win32 Release"
# Name "aculab - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\acuphone.cpp
# End Source File
# Begin Source File

SOURCE=.\acutrunk.cpp
# End Source File
# Begin Source File

SOURCE=.\v5\cllib.c
# End Source File
# Begin Source File

SOURCE=.\v5\clnt.c
# End Source File
# Begin Source File

SOURCE=.\v5\common.c
# End Source File
# Begin Source File

SOURCE=..\common\names.cpp
# End Source File
# Begin Source File

SOURCE=.\v5\smbesp.c
# End Source File
# Begin Source File

SOURCE=.\v5\smclib.c
# End Source File
# Begin Source File

SOURCE=.\v5\smfwcaps.c
# End Source File
# Begin Source File

SOURCE=.\v5\smlib.c
# End Source File
# Begin Source File

SOURCE=.\v5\smnt.c
# End Source File
# Begin Source File

SOURCE=.\v5\smwavlib.c
# End Source File
# Begin Source File

SOURCE=.\v5\swlib.c
# End Source File
# Begin Source File

SOURCE=.\v5\swnt.c
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\acuphone.h
# End Source File
# Begin Source File

SOURCE=.\acutrunk.h
# End Source File
# Begin Source File

SOURCE=.\v5\mvcldrvr.h
# End Source File
# Begin Source File

SOURCE=.\v5\mvswdrvr.h
# End Source File
# Begin Source File

SOURCE=..\include\phone.h
# End Source File
# Begin Source File

SOURCE=..\include\phoneclient.h
# End Source File
# Begin Source File

SOURCE=.\v5\smclib.h
# End Source File
# Begin Source File

SOURCE=.\v5\smdrvr.h

!IF  "$(CFG)" == "aculab - Win32 Release"

# Begin Custom Build
InputPath=.\v5\smdrvr.h

"prosody_error.i" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	python ..\scripts\symname.py -f prosody_error -o prosody_error.i ERR_SM $(InputPath)

# End Custom Build

!ELSEIF  "$(CFG)" == "aculab - Win32 Debug"

# Begin Custom Build
InputPath=.\v5\smdrvr.h

"prosody_error.i" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	python ..\scripts\symname.py -f prosody_error -o prosody_error.i ERR_SM $(InputPath)

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\v5\smfwcaps.h
# End Source File
# Begin Source File

SOURCE=.\v5\smosintf.h
# End Source File
# Begin Source File

SOURCE=.\v5\smwavlib.h
# End Source File
# Begin Source File

SOURCE=..\include\switch.h
# End Source File
# End Group
# Begin Source File

SOURCE=.\beep.al

!IF  "$(CFG)" == "aculab - Win32 Release"

# Begin Custom Build - Performing Custom Build Step on $(InputPath)
InputPath=.\beep.al

"beep.i" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	python ..\scripts\raw-to-c.py $(InputPath) beep.i beep

# End Custom Build

!ELSEIF  "$(CFG)" == "aculab - Win32 Debug"

# Begin Custom Build - Performing Custom Build Step on $(InputPath)
InputPath=.\beep.al

"beep.i" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	python ..\scripts\raw-to-c.py $(InputPath) beep.i beep

# End Custom Build

!ENDIF 

# End Source File
# End Target
# End Project
