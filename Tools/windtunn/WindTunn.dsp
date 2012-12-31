# Microsoft Developer Studio Project File - Name="WindTunn" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Console Application" 0x0103

CFG=WindTunn - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "WindTunn.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "WindTunn.mak" CFG="WindTunn - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "WindTunn - Win32 Release" (based on "Win32 (x86) Console Application")
!MESSAGE "WindTunn - Win32 Debug" (based on "Win32 (x86) Console Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""$/ww2/windtunn", FOEAAAAA"
# PROP Scc_LocalPath "."
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "WindTunn - Win32 Release"

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
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /W3 /GX /O2 /I "." /I ".\include" /I ".\\" /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /c
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386

!ELSEIF  "$(CFG)" == "WindTunn - Win32 Debug"

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
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /GZ /c
# ADD CPP /nologo /W3 /Gm /GX /ZI /Od /I ".\\" /I ".\include" /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /GZ /c
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept

!ENDIF 

# Begin Target

# Name "WindTunn - Win32 Release"
# Name "WindTunn - Win32 Debug"
# Begin Group "fio"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\fio\fio.c
# End Source File
# Begin Source File

SOURCE=.\fio\fio.h
# End Source File
# Begin Source File

SOURCE=.\fio\fiolzw.c
# End Source File
# Begin Source File

SOURCE=.\fio\fiolzw.h
# End Source File
# End Group
# Begin Group "math"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\math\euler.c
# End Source File
# Begin Source File

SOURCE=.\math\euler.h
# End Source File
# Begin Source File

SOURCE=.\math\math3d.c
# End Source File
# Begin Source File

SOURCE=.\math\math3d.h
# End Source File
# Begin Source File

SOURCE=.\math\matheul.c
# End Source File
# Begin Source File

SOURCE=.\math\matheul.h
# End Source File
# Begin Source File

SOURCE=.\math\mathvect.c
# End Source File
# Begin Source File

SOURCE=.\math\mathvect.h
# End Source File
# End Group
# Begin Group "mem"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\mem\mem.c
# End Source File
# Begin Source File

SOURCE=.\mem\mem.h
# End Source File
# End Group
# Begin Group "ml"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\ml\mlAtmo.c
# End Source File
# Begin Source File

SOURCE=.\ml\mlAtmo.h
# End Source File
# Begin Source File

SOURCE=.\ml\mlCompon.c
# End Source File
# Begin Source File

SOURCE=.\ml\mlCompon.h
# End Source File
# Begin Source File

SOURCE=.\ml\mlEngine.c
# End Source File
# Begin Source File

SOURCE=.\ml\mlEngine.h
# End Source File
# Begin Source File

SOURCE=.\ml\mlfoils.c
# End Source File
# Begin Source File

SOURCE=.\ml\mlfoils.h
# End Source File
# Begin Source File

SOURCE=.\ml\mlgauges.c
# End Source File
# Begin Source File

SOURCE=.\ml\mlgauges.h
# End Source File
# Begin Source File

SOURCE=.\ml\mlMain.c
# End Source File
# Begin Source File

SOURCE=.\ml\mltypes.h
# End Source File
# Begin Source File

SOURCE=.\ml\mlutility.c
# End Source File
# Begin Source File

SOURCE=.\ml\mlutility.h
# End Source File
# Begin Source File

SOURCE=.\ml\mlwndtnn.c
# End Source File
# Begin Source File

SOURCE=.\ml\mlwndtnn.h
# End Source File
# End Group
# Begin Group "pi"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\pi\pi.h
# End Source File
# End Group
# Begin Group "sys"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\sys\syscfg.h
# End Source File
# End Group
# Begin Source File

SOURCE=.\vars.h
# End Source File
# End Target
# End Project
