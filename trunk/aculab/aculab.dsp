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
# ADD CPP /nologo /MD /W3 /GX /O2 /I ".." /I "..\include" /D "NDEBUG" /D "_LIB" /D "_WINSTATIC" /D "WIN32" /D "_MBCS" /FD /c
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
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I ".." /I "..\include" /D "_DEBUG" /D "_LIB" /D "_WINSTATIC" /D "WIN32" /D "_MBCS" /FR /FD /GZ /c
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

SOURCE=.\cllib.c
# End Source File
# Begin Source File

SOURCE=.\clnt.c
# End Source File
# Begin Source File

SOURCE=.\common.c
# End Source File
# Begin Source File

SOURCE=.\smbesp.c
# End Source File
# Begin Source File

SOURCE=.\smclib.c
# End Source File
# Begin Source File

SOURCE=.\smfwcaps.c
# End Source File
# Begin Source File

SOURCE=.\smlib.c
# End Source File
# Begin Source File

SOURCE=.\smnt.c
# End Source File
# Begin Source File

SOURCE=.\smwavlib.c
# End Source File
# Begin Source File

SOURCE=.\swlib.c
# End Source File
# Begin Source File

SOURCE=.\swnt.c
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

SOURCE=..\include\buffers.h
# End Source File
# Begin Source File

SOURCE=.\mvcldrvr.h
# End Source File
# Begin Source File

SOURCE=.\mvswdrvr.h
# End Source File
# Begin Source File

SOURCE=..\include\phone.h
# End Source File
# Begin Source File

SOURCE=..\include\phoneclient.h
# End Source File
# Begin Source File

SOURCE=.\smbesp.h
# End Source File
# Begin Source File

SOURCE=.\smclib.h
# End Source File
# Begin Source File

SOURCE=.\smdrvr.h

!IF  "$(CFG)" == "aculab - Win32 Release"

# Begin Custom Build
InputPath=.\smdrvr.h

"prosody_error.i" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	perl ..\scripts\extract-error.pl ERR_SM prosody_error < $(InputPath) > prosody_error.i

# End Custom Build

!ELSEIF  "$(CFG)" == "aculab - Win32 Debug"

# Begin Custom Build
InputPath=.\smdrvr.h

"prosody_error.i" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	perl ..\scripts\extract-error.pl ERR_SM prosody_error < $(InputPath) > prosody_error.i

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\smfwcaps.h
# End Source File
# Begin Source File

SOURCE=.\smosintf.h
# End Source File
# Begin Source File

SOURCE=.\smwavlib.h
# End Source File
# Begin Source File

SOURCE=..\include\switch.h
# End Source File
# End Group
# End Target
# End Project
