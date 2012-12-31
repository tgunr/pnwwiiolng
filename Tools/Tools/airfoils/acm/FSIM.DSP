# Microsoft Developer Studio Project File - Name="fsim" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 5.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=fsim - Win32 Release
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "fsim.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "fsim.mak" CFG="fsim - Win32 Release"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "fsim - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "fsim - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe

!IF  "$(CFG)" == "fsim - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir ".\Release"
# PROP BASE Intermediate_Dir ".\Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir ".\Release"
# PROP Intermediate_Dir ".\Release"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /c
# ADD CPP /nologo /W3 /GX /Ot /Oa /Ow /Oi /Op /Ob1 /I "..\V\lib" /I "." /I "..\dis" /I "c:\source\include" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "HAVE_RAND" /D HAVE_STDLIB=1 /D HAVE_DIS=1 /D "_CTYPE_DISABLE_MACROS" /YX"pm.h" /FD /c
# SUBTRACT CPP /Ox /X
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo /out:"..\Release\fsim.lib"

!ELSEIF  "$(CFG)" == "fsim - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir ".\Debug"
# PROP BASE Intermediate_Dir ".\Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir ".\Debug"
# PROP Intermediate_Dir ".\Debug"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /Z7 /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /c
# ADD CPP /nologo /W3 /GX /Z7 /Od /I "..\V\lib" /I "." /I "..\dis" /I "c:\source\include" /D "_WINDOWS" /D "HAVE_RAND" /D HAVE_STDLIB=1 /D HAVE_DIS=1 /D "_CTYPE_DISABLE_MACROS" /D "FLIGHTDEBUG" /D "_DEBUG" /D "WIN32" /YX"pm.h" /FD /c
# SUBTRACT CPP /X
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo /out:"..\Debug\fsim.lib"

!ENDIF 

# Begin Target

# Name "fsim - Win32 Release"
# Name "fsim - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;hpj;bat;for;f90"
# Begin Source File

SOURCE=.\afds.c
# End Source File
# Begin Source File

SOURCE=.\aim9m.c
# End Source File
# Begin Source File

SOURCE=.\air.c
# End Source File
# Begin Source File

SOURCE=.\alarm.c
# End Source File
# Begin Source File

SOURCE=.\box.c
# End Source File
# Begin Source File

SOURCE=.\damage.c
# End Source File
# Begin Source File

SOURCE=.\dis_if.c
# End Source File
# Begin Source File

SOURCE=.\drone.c
# End Source File
# Begin Source File

SOURCE=.\effects.c
# End Source File
# Begin Source File

SOURCE=.\flaps.c
# End Source File
# Begin Source File

SOURCE=.\fpage.c
# End Source File
# Begin Source File

SOURCE=.\fplan.c
# End Source File
# Begin Source File

SOURCE=.\gear.c
# End Source File
# Begin Source File

SOURCE=.\hsi.c
# End Source File
# Begin Source File

SOURCE=.\hud.c
# End Source File
# Begin Source File

SOURCE=.\imath.c
# End Source File
# Begin Source File

SOURCE=.\init.c
# End Source File
# Begin Source File

SOURCE=.\interpolate.c
# End Source File
# Begin Source File

SOURCE=.\inventory.c
# End Source File
# Begin Source File

SOURCE=.\m61a1.c
# End Source File
# Begin Source File

SOURCE=.\missile.c
# End Source File
# Begin Source File

SOURCE=.\missilec.c
# End Source File
# Begin Source File

SOURCE=.\navaid.c
# End Source File
# Begin Source File

SOURCE=.\panel.c
# End Source File
# Begin Source File

SOURCE=.\place.c
# End Source File
# Begin Source File

SOURCE=.\planes.c
# End Source File
# Begin Source File

SOURCE=.\pm.c

!IF  "$(CFG)" == "fsim - Win32 Release"

# ADD CPP /W4

!ELSEIF  "$(CFG)" == "fsim - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\radar.c
# End Source File
# Begin Source File

SOURCE=.\redraw.c
# End Source File
# Begin Source File

SOURCE=.\render.c
# End Source File
# Begin Source File

SOURCE=.\resupply.c
# End Source File
# Begin Source File

SOURCE=.\runway.c
# End Source File
# Begin Source File

SOURCE=.\scale.c
# End Source File
# Begin Source File

SOURCE=.\terrain.c
# End Source File
# Begin Source File

SOURCE=.\transpose.c
# End Source File
# Begin Source File

SOURCE=.\weapon.c
# End Source File
# Begin Source File

SOURCE=.\windows.c
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl;fi;fd"
# Begin Source File

SOURCE=.\afds.h
# End Source File
# Begin Source File

SOURCE=.\alarm.h
# End Source File
# Begin Source File

SOURCE=.\box.h
# End Source File
# Begin Source File

SOURCE=.\damage.h
# End Source File
# Begin Source File

SOURCE=.\deadreckon.h
# End Source File
# Begin Source File

SOURCE=.\dis.h
# End Source File
# Begin Source File

SOURCE=.\fplan.h
# End Source File
# Begin Source File

SOURCE=.\horizon.h
# End Source File
# Begin Source File

SOURCE=.\hsi.h
# End Source File
# Begin Source File

SOURCE=.\imath.h
# End Source File
# Begin Source File

SOURCE=.\interpolate.h
# End Source File
# Begin Source File

SOURCE=.\manifest.h
# End Source File
# Begin Source File

SOURCE=.\navaid.h
# End Source File
# Begin Source File

SOURCE=.\pm.h
# End Source File
# Begin Source File

SOURCE=.\scale.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;cnt;rtf;gif;jpg;jpeg;jpe"
# End Group
# End Target
# End Project
