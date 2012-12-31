# Microsoft Developer Studio Generated NMAKE File, Format Version 4.20
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

!IF "$(CFG)" == ""
CFG=fsim - Win32 Debug
!MESSAGE No configuration specified.  Defaulting to fsim - Win32 Debug.
!ENDIF 

!IF "$(CFG)" != "fsim - Win32 Release" && "$(CFG)" != "fsim - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE on this makefile
!MESSAGE by defining the macro CFG on the command line.  For example:
!MESSAGE 
!MESSAGE NMAKE /f "fsim.mak" CFG="fsim - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "fsim - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "fsim - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 
!ERROR An invalid configuration is specified.
!ENDIF 

!IF "$(OS)" == "Windows_NT"
NULL=
!ELSE 
NULL=nul
!ENDIF 
################################################################################
# Begin Project
# PROP Target_Last_Scanned "fsim - Win32 Debug"
CPP=cl.exe

!IF  "$(CFG)" == "fsim - Win32 Release"

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
OUTDIR=.\Release
INTDIR=.\Release

ALL : "..\Release\fsim.lib"

CLEAN : 
	-@erase "$(INTDIR)\aim9m.obj"
	-@erase "$(INTDIR)\air.obj"
	-@erase "$(INTDIR)\alarm.obj"
	-@erase "$(INTDIR)\audio.obj"
	-@erase "$(INTDIR)\box.obj"
	-@erase "$(INTDIR)\damage.obj"
	-@erase "$(INTDIR)\deadreckon.obj"
	-@erase "$(INTDIR)\dis_if.obj"
	-@erase "$(INTDIR)\drone.obj"
	-@erase "$(INTDIR)\effects.obj"
	-@erase "$(INTDIR)\flaps.obj"
	-@erase "$(INTDIR)\gear.obj"
	-@erase "$(INTDIR)\hsi.obj"
	-@erase "$(INTDIR)\hud.obj"
	-@erase "$(INTDIR)\imath.obj"
	-@erase "$(INTDIR)\init.obj"
	-@erase "$(INTDIR)\interpolate.obj"
	-@erase "$(INTDIR)\inventory.obj"
	-@erase "$(INTDIR)\m61a1.obj"
	-@erase "$(INTDIR)\missile.obj"
	-@erase "$(INTDIR)\missilec.obj"
	-@erase "$(INTDIR)\navaid.obj"
	-@erase "$(INTDIR)\panel.obj"
	-@erase "$(INTDIR)\place.obj"
	-@erase "$(INTDIR)\planes.obj"
	-@erase "$(INTDIR)\pm.obj"
	-@erase "$(INTDIR)\radar.obj"
	-@erase "$(INTDIR)\render.obj"
	-@erase "$(INTDIR)\resupply.obj"
	-@erase "$(INTDIR)\scale.obj"
	-@erase "$(INTDIR)\terrain.obj"
	-@erase "$(INTDIR)\transpose.obj"
	-@erase "$(INTDIR)\weapon.obj"
	-@erase "$(INTDIR)\windows.obj"
	-@erase "..\Release\fsim.lib"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /c
# ADD CPP /nologo /W3 /GX /O2 /I "..\V\lib" /I "." /I "..\dis" /I "c:\msdev\include" /I "c:\source\include" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "HAVE_RAND" /D HAVE_STDLIB=1 /D HAVE_DIS=1 /D "_CTYPE_DISABLE_MACROS" /D "FLIGHTDEBUG" /YX"pm.h" /c
# SUBTRACT CPP /X
CPP_PROJ=/nologo /ML /W3 /GX /O2 /I "..\V\lib" /I "." /I "..\dis" /I\
 "c:\msdev\include" /I "c:\source\include" /D "NDEBUG" /D "WIN32" /D "_WINDOWS"\
 /D "HAVE_RAND" /D HAVE_STDLIB=1 /D HAVE_DIS=1 /D "_CTYPE_DISABLE_MACROS" /D\
 "FLIGHTDEBUG" /Fp"$(INTDIR)/fsim.pch" /YX"pm.h" /Fo"$(INTDIR)/" /c 
CPP_OBJS=.\Release/
CPP_SBRS=.\.
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o"$(OUTDIR)/fsim.bsc" 
BSC32_SBRS= \
	
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo /out:"..\Release\fsim.lib"
LIB32_FLAGS=/nologo /out:"..\Release\fsim.lib" 
LIB32_OBJS= \
	"$(INTDIR)\aim9m.obj" \
	"$(INTDIR)\air.obj" \
	"$(INTDIR)\alarm.obj" \
	"$(INTDIR)\audio.obj" \
	"$(INTDIR)\box.obj" \
	"$(INTDIR)\damage.obj" \
	"$(INTDIR)\deadreckon.obj" \
	"$(INTDIR)\dis_if.obj" \
	"$(INTDIR)\drone.obj" \
	"$(INTDIR)\effects.obj" \
	"$(INTDIR)\flaps.obj" \
	"$(INTDIR)\gear.obj" \
	"$(INTDIR)\hsi.obj" \
	"$(INTDIR)\hud.obj" \
	"$(INTDIR)\imath.obj" \
	"$(INTDIR)\init.obj" \
	"$(INTDIR)\interpolate.obj" \
	"$(INTDIR)\inventory.obj" \
	"$(INTDIR)\m61a1.obj" \
	"$(INTDIR)\missile.obj" \
	"$(INTDIR)\missilec.obj" \
	"$(INTDIR)\navaid.obj" \
	"$(INTDIR)\panel.obj" \
	"$(INTDIR)\place.obj" \
	"$(INTDIR)\planes.obj" \
	"$(INTDIR)\pm.obj" \
	"$(INTDIR)\radar.obj" \
	"$(INTDIR)\render.obj" \
	"$(INTDIR)\resupply.obj" \
	"$(INTDIR)\scale.obj" \
	"$(INTDIR)\terrain.obj" \
	"$(INTDIR)\transpose.obj" \
	"$(INTDIR)\weapon.obj" \
	"$(INTDIR)\windows.obj"

"..\Release\fsim.lib" : "$(OUTDIR)" $(DEF_FILE) $(LIB32_OBJS)
    $(LIB32) @<<
  $(LIB32_FLAGS) $(DEF_FLAGS) $(LIB32_OBJS)
<<

!ELSEIF  "$(CFG)" == "fsim - Win32 Debug"

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
OUTDIR=.\Debug
INTDIR=.\Debug

ALL : "..\Debug\fsim.lib"

CLEAN : 
	-@erase "$(INTDIR)\aim9m.obj"
	-@erase "$(INTDIR)\air.obj"
	-@erase "$(INTDIR)\alarm.obj"
	-@erase "$(INTDIR)\audio.obj"
	-@erase "$(INTDIR)\box.obj"
	-@erase "$(INTDIR)\damage.obj"
	-@erase "$(INTDIR)\deadreckon.obj"
	-@erase "$(INTDIR)\dis_if.obj"
	-@erase "$(INTDIR)\drone.obj"
	-@erase "$(INTDIR)\effects.obj"
	-@erase "$(INTDIR)\flaps.obj"
	-@erase "$(INTDIR)\gear.obj"
	-@erase "$(INTDIR)\hsi.obj"
	-@erase "$(INTDIR)\hud.obj"
	-@erase "$(INTDIR)\imath.obj"
	-@erase "$(INTDIR)\init.obj"
	-@erase "$(INTDIR)\interpolate.obj"
	-@erase "$(INTDIR)\inventory.obj"
	-@erase "$(INTDIR)\m61a1.obj"
	-@erase "$(INTDIR)\missile.obj"
	-@erase "$(INTDIR)\missilec.obj"
	-@erase "$(INTDIR)\navaid.obj"
	-@erase "$(INTDIR)\panel.obj"
	-@erase "$(INTDIR)\place.obj"
	-@erase "$(INTDIR)\planes.obj"
	-@erase "$(INTDIR)\pm.obj"
	-@erase "$(INTDIR)\radar.obj"
	-@erase "$(INTDIR)\render.obj"
	-@erase "$(INTDIR)\resupply.obj"
	-@erase "$(INTDIR)\scale.obj"
	-@erase "$(INTDIR)\terrain.obj"
	-@erase "$(INTDIR)\transpose.obj"
	-@erase "$(INTDIR)\weapon.obj"
	-@erase "$(INTDIR)\windows.obj"
	-@erase "..\Debug\fsim.lib"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

# ADD BASE CPP /nologo /W3 /GX /Z7 /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /c
# ADD CPP /nologo /W3 /GX /Z7 /Od /I "..\V\lib" /I "." /I "..\dis" /I "c:\msdev\include" /I "c:\source\include" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "HAVE_RAND" /D HAVE_STDLIB=1 /D HAVE_DIS=1 /D "_CTYPE_DISABLE_MACROS" /D "FLIGHTDEBUG" /YX"pm.h" /c
# SUBTRACT CPP /X
CPP_PROJ=/nologo /MLd /W3 /GX /Z7 /Od /I "..\V\lib" /I "." /I "..\dis" /I\
 "c:\msdev\include" /I "c:\source\include" /D "_DEBUG" /D "WIN32" /D "_WINDOWS"\
 /D "HAVE_RAND" /D HAVE_STDLIB=1 /D HAVE_DIS=1 /D "_CTYPE_DISABLE_MACROS" /D\
 "FLIGHTDEBUG" /Fp"$(INTDIR)/fsim.pch" /YX"pm.h" /Fo"$(INTDIR)/" /c 
CPP_OBJS=.\Debug/
CPP_SBRS=.\.
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o"$(OUTDIR)/fsim.bsc" 
BSC32_SBRS= \
	
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo /out:"..\Debug\fsim.lib"
LIB32_FLAGS=/nologo /out:"..\Debug\fsim.lib" 
LIB32_OBJS= \
	"$(INTDIR)\aim9m.obj" \
	"$(INTDIR)\air.obj" \
	"$(INTDIR)\alarm.obj" \
	"$(INTDIR)\audio.obj" \
	"$(INTDIR)\box.obj" \
	"$(INTDIR)\damage.obj" \
	"$(INTDIR)\deadreckon.obj" \
	"$(INTDIR)\dis_if.obj" \
	"$(INTDIR)\drone.obj" \
	"$(INTDIR)\effects.obj" \
	"$(INTDIR)\flaps.obj" \
	"$(INTDIR)\gear.obj" \
	"$(INTDIR)\hsi.obj" \
	"$(INTDIR)\hud.obj" \
	"$(INTDIR)\imath.obj" \
	"$(INTDIR)\init.obj" \
	"$(INTDIR)\interpolate.obj" \
	"$(INTDIR)\inventory.obj" \
	"$(INTDIR)\m61a1.obj" \
	"$(INTDIR)\missile.obj" \
	"$(INTDIR)\missilec.obj" \
	"$(INTDIR)\navaid.obj" \
	"$(INTDIR)\panel.obj" \
	"$(INTDIR)\place.obj" \
	"$(INTDIR)\planes.obj" \
	"$(INTDIR)\pm.obj" \
	"$(INTDIR)\radar.obj" \
	"$(INTDIR)\render.obj" \
	"$(INTDIR)\resupply.obj" \
	"$(INTDIR)\scale.obj" \
	"$(INTDIR)\terrain.obj" \
	"$(INTDIR)\transpose.obj" \
	"$(INTDIR)\weapon.obj" \
	"$(INTDIR)\windows.obj"

"..\Debug\fsim.lib" : "$(OUTDIR)" $(DEF_FILE) $(LIB32_OBJS)
    $(LIB32) @<<
  $(LIB32_FLAGS) $(DEF_FLAGS) $(LIB32_OBJS)
<<

!ENDIF 

.c{$(CPP_OBJS)}.obj:
   $(CPP) $(CPP_PROJ) $<  

.cpp{$(CPP_OBJS)}.obj:
   $(CPP) $(CPP_PROJ) $<  

.cxx{$(CPP_OBJS)}.obj:
   $(CPP) $(CPP_PROJ) $<  

.c{$(CPP_SBRS)}.sbr:
   $(CPP) $(CPP_PROJ) $<  

.cpp{$(CPP_SBRS)}.sbr:
   $(CPP) $(CPP_PROJ) $<  

.cxx{$(CPP_SBRS)}.sbr:
   $(CPP) $(CPP_PROJ) $<  

################################################################################
# Begin Target

# Name "fsim - Win32 Release"
# Name "fsim - Win32 Debug"

!IF  "$(CFG)" == "fsim - Win32 Release"

!ELSEIF  "$(CFG)" == "fsim - Win32 Debug"

!ENDIF 

################################################################################
# Begin Source File

SOURCE=.\weapon.c

!IF  "$(CFG)" == "fsim - Win32 Release"

DEP_CPP_WEAPO=\
	"..\dis\dis/dis.h"\
	"..\dis\dis/disenum.h"\
	"..\V\lib\Alib.h"\
	"..\V\lib\Vlib.h"\
	".\damage.h"\
	".\imath.h"\
	".\interpolate.h"\
	".\manifest.h"\
	".\pm.h"\
	".\scale.h"\
	

"$(INTDIR)\weapon.obj" : $(SOURCE) $(DEP_CPP_WEAPO) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "fsim - Win32 Debug"

DEP_CPP_WEAPO=\
	"..\dis\dis/datum.h"\
	"..\dis\dis/dis.h"\
	"..\dis\dis/disenum.h"\
	"..\dis\dis/disenum2.h"\
	"..\dis\dis/disenum3.h"\
	"..\dis\dis/disp.h"\
	"..\dis\dis/simx.h"\
	"..\V\lib\Alib.h"\
	"..\V\lib\Vlib.h"\
	".\damage.h"\
	".\imath.h"\
	".\interpolate.h"\
	".\manifest.h"\
	".\navaid.h"\
	".\pm.h"\
	".\scale.h"\
	"\msdev\include\sys\types.h"\
	"\source\include\byte.h"\
	"\source\include\netinet\in.h"\
	"\source\include\rpc\types.h"\
	"\source\include\rpc\xdr.h"\
	"\source\include\sys\cdefs.h"\
	"\source\include\sys\time.h"\
	

"$(INTDIR)\weapon.obj" : $(SOURCE) $(DEP_CPP_WEAPO) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\air.c

"$(INTDIR)\air.obj" : $(SOURCE) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\alarm.c
DEP_CPP_ALARM=\
	".\alarm.h"\
	"\msdev\include\sys\types.h"\
	"\source\include\machine\endian.h"\
	"\source\include\machine\limits.h"\
	"\source\include\machine\param.h"\
	"\source\include\machine\signal.h"\
	"\source\include\machine\trap.h"\
	"\source\include\sys\cdefs.h"\
	"\source\include\sys\errno.h"\
	"\source\include\sys\param.h"\
	"\source\include\sys\resource.h"\
	"\source\include\sys\signal.h"\
	"\source\include\sys\syslimits.h"\
	"\source\include\sys\time.h"\
	"\source\include\sys\ucred.h"\
	"\source\include\sys\uio.h"\
	

"$(INTDIR)\alarm.obj" : $(SOURCE) $(DEP_CPP_ALARM) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\box.c

!IF  "$(CFG)" == "fsim - Win32 Release"

DEP_CPP_BOX_C=\
	"..\dis\dis/dis.h"\
	"..\dis\dis/disenum.h"\
	"..\V\lib\Alib.h"\
	"..\V\lib\Vlib.h"\
	".\box.h"\
	".\damage.h"\
	".\imath.h"\
	".\interpolate.h"\
	".\manifest.h"\
	".\pm.h"\
	".\scale.h"\
	

"$(INTDIR)\box.obj" : $(SOURCE) $(DEP_CPP_BOX_C) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "fsim - Win32 Debug"

DEP_CPP_BOX_C=\
	"..\dis\dis/datum.h"\
	"..\dis\dis/dis.h"\
	"..\dis\dis/disenum.h"\
	"..\dis\dis/disenum2.h"\
	"..\dis\dis/disenum3.h"\
	"..\dis\dis/disp.h"\
	"..\dis\dis/simx.h"\
	"..\V\lib\Alib.h"\
	"..\V\lib\Vlib.h"\
	".\box.h"\
	".\damage.h"\
	".\imath.h"\
	".\interpolate.h"\
	".\manifest.h"\
	".\navaid.h"\
	".\pm.h"\
	".\scale.h"\
	"\msdev\include\sys\types.h"\
	"\source\include\byte.h"\
	"\source\include\netinet\in.h"\
	"\source\include\rpc\types.h"\
	"\source\include\rpc\xdr.h"\
	"\source\include\sys\cdefs.h"\
	"\source\include\sys\time.h"\
	

"$(INTDIR)\box.obj" : $(SOURCE) $(DEP_CPP_BOX_C) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\damage.c

!IF  "$(CFG)" == "fsim - Win32 Release"

DEP_CPP_DAMAG=\
	"..\dis\dis/dis.h"\
	"..\dis\dis/disenum.h"\
	"..\V\lib\Alib.h"\
	"..\V\lib\Vlib.h"\
	".\damage.h"\
	".\imath.h"\
	".\interpolate.h"\
	".\manifest.h"\
	".\pm.h"\
	".\scale.h"\
	

"$(INTDIR)\damage.obj" : $(SOURCE) $(DEP_CPP_DAMAG) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "fsim - Win32 Debug"

DEP_CPP_DAMAG=\
	"..\dis\dis/datum.h"\
	"..\dis\dis/dis.h"\
	"..\dis\dis/disenum.h"\
	"..\dis\dis/disenum2.h"\
	"..\dis\dis/disenum3.h"\
	"..\dis\dis/disp.h"\
	"..\dis\dis/simx.h"\
	"..\V\lib\Alib.h"\
	"..\V\lib\Vlib.h"\
	".\damage.h"\
	".\imath.h"\
	".\interpolate.h"\
	".\manifest.h"\
	".\navaid.h"\
	".\pm.h"\
	".\scale.h"\
	"\msdev\include\sys\types.h"\
	"\source\include\byte.h"\
	"\source\include\netinet\in.h"\
	"\source\include\rpc\types.h"\
	"\source\include\rpc\xdr.h"\
	"\source\include\sys\cdefs.h"\
	"\source\include\sys\time.h"\
	

"$(INTDIR)\damage.obj" : $(SOURCE) $(DEP_CPP_DAMAG) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\deadreckon.c
DEP_CPP_DEADR=\
	".\deadreckon.h"\
	

"$(INTDIR)\deadreckon.obj" : $(SOURCE) $(DEP_CPP_DEADR) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\dis_if.c

!IF  "$(CFG)" == "fsim - Win32 Release"

DEP_CPP_DIS_I=\
	"..\dis\dis/datum.h"\
	"..\dis\dis/dis.h"\
	"..\dis\dis/disenum.h"\
	"..\dis\dis/disenum2.h"\
	"..\dis\dis/disenum3.h"\
	"..\dis\dis/disp.h"\
	"..\dis\dis/simx.h"\
	".\deadreckon.h"\
	".\dis.h"\
	".\pm.h"\
	"\msdev\include\sys\types.h"\
	"\source\include\byte.h"\
	"\source\include\netinet\in.h"\
	"\source\include\rpc\types.h"\
	"\source\include\rpc\xdr.h"\
	"\source\include\sys\cdefs.h"\
	"\source\include\sys\time.h"\
	

"$(INTDIR)\dis_if.obj" : $(SOURCE) $(DEP_CPP_DIS_I) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "fsim - Win32 Debug"

DEP_CPP_DIS_I=\
	"..\dis\dis/datum.h"\
	"..\dis\dis/dis.h"\
	"..\dis\dis/disenum.h"\
	"..\dis\dis/disenum2.h"\
	"..\dis\dis/disenum3.h"\
	"..\dis\dis/disp.h"\
	"..\dis\dis/simx.h"\
	"..\V\lib\Alib.h"\
	"..\V\lib\Vlib.h"\
	".\damage.h"\
	".\deadreckon.h"\
	".\dis.h"\
	".\imath.h"\
	".\interpolate.h"\
	".\manifest.h"\
	".\navaid.h"\
	".\pm.h"\
	".\scale.h"\
	"\msdev\include\sys\types.h"\
	"\source\include\byte.h"\
	"\source\include\netinet\in.h"\
	"\source\include\rpc\types.h"\
	"\source\include\rpc\xdr.h"\
	"\source\include\sys\cdefs.h"\
	"\source\include\sys\time.h"\
	

"$(INTDIR)\dis_if.obj" : $(SOURCE) $(DEP_CPP_DIS_I) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\drone.c

!IF  "$(CFG)" == "fsim - Win32 Release"

DEP_CPP_DRONE=\
	"..\dis\dis/datum.h"\
	"..\dis\dis/dis.h"\
	"..\dis\dis/disenum.h"\
	"..\dis\dis/disenum2.h"\
	"..\dis\dis/disenum3.h"\
	"..\dis\dis/disp.h"\
	"..\dis\dis/simx.h"\
	"..\V\lib\Alib.h"\
	"..\V\lib\Vlib.h"\
	".\alarm.h"\
	".\damage.h"\
	".\imath.h"\
	".\interpolate.h"\
	".\manifest.h"\
	".\pm.h"\
	".\scale.h"\
	"\msdev\include\sys\types.h"\
	"\source\include\byte.h"\
	"\source\include\netinet\in.h"\
	"\source\include\rpc\types.h"\
	"\source\include\rpc\xdr.h"\
	"\source\include\sys\cdefs.h"\
	"\source\include\sys\time.h"\
	

"$(INTDIR)\drone.obj" : $(SOURCE) $(DEP_CPP_DRONE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "fsim - Win32 Debug"

DEP_CPP_DRONE=\
	"..\dis\dis/datum.h"\
	"..\dis\dis/dis.h"\
	"..\dis\dis/disenum.h"\
	"..\dis\dis/disenum2.h"\
	"..\dis\dis/disenum3.h"\
	"..\dis\dis/disp.h"\
	"..\dis\dis/simx.h"\
	"..\V\lib\Alib.h"\
	"..\V\lib\Vlib.h"\
	".\alarm.h"\
	".\damage.h"\
	".\imath.h"\
	".\interpolate.h"\
	".\manifest.h"\
	".\navaid.h"\
	".\pm.h"\
	".\scale.h"\
	"\msdev\include\sys\types.h"\
	"\source\include\byte.h"\
	"\source\include\netinet\in.h"\
	"\source\include\rpc\types.h"\
	"\source\include\rpc\xdr.h"\
	"\source\include\sys\cdefs.h"\
	"\source\include\sys\time.h"\
	

"$(INTDIR)\drone.obj" : $(SOURCE) $(DEP_CPP_DRONE) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\effects.c
DEP_CPP_EFFEC=\
	"..\dis\dis/datum.h"\
	"..\dis\dis/dis.h"\
	"..\dis\dis/disenum.h"\
	"..\dis\dis/disenum2.h"\
	"..\dis\dis/disenum3.h"\
	"..\dis\dis/disp.h"\
	"..\dis\dis/simx.h"\
	"..\V\lib\Alib.h"\
	"..\V\lib\Vlib.h"\
	".\damage.h"\
	".\imath.h"\
	".\interpolate.h"\
	".\manifest.h"\
	".\navaid.h"\
	".\pm.h"\
	".\scale.h"\
	"\msdev\include\sys\types.h"\
	"\source\include\byte.h"\
	"\source\include\netinet\in.h"\
	"\source\include\rpc\types.h"\
	"\source\include\rpc\xdr.h"\
	"\source\include\sys\cdefs.h"\
	"\source\include\sys\time.h"\
	

"$(INTDIR)\effects.obj" : $(SOURCE) $(DEP_CPP_EFFEC) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\flaps.c

!IF  "$(CFG)" == "fsim - Win32 Release"

DEP_CPP_FLAPS=\
	"..\dis\dis/datum.h"\
	"..\dis\dis/dis.h"\
	"..\dis\dis/disenum.h"\
	"..\dis\dis/disenum2.h"\
	"..\dis\dis/disenum3.h"\
	"..\dis\dis/disp.h"\
	"..\dis\dis/simx.h"\
	"..\V\lib\Alib.h"\
	"..\V\lib\Vlib.h"\
	".\damage.h"\
	".\imath.h"\
	".\interpolate.h"\
	".\manifest.h"\
	".\pm.h"\
	".\scale.h"\
	"\msdev\include\sys\types.h"\
	"\source\include\byte.h"\
	"\source\include\netinet\in.h"\
	"\source\include\rpc\types.h"\
	"\source\include\rpc\xdr.h"\
	"\source\include\sys\cdefs.h"\
	"\source\include\sys\time.h"\
	

"$(INTDIR)\flaps.obj" : $(SOURCE) $(DEP_CPP_FLAPS) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "fsim - Win32 Debug"

DEP_CPP_FLAPS=\
	"..\dis\dis/datum.h"\
	"..\dis\dis/dis.h"\
	"..\dis\dis/disenum.h"\
	"..\dis\dis/disenum2.h"\
	"..\dis\dis/disenum3.h"\
	"..\dis\dis/disp.h"\
	"..\dis\dis/simx.h"\
	"..\V\lib\Alib.h"\
	"..\V\lib\Vlib.h"\
	".\damage.h"\
	".\imath.h"\
	".\interpolate.h"\
	".\manifest.h"\
	".\navaid.h"\
	".\pm.h"\
	".\scale.h"\
	"\msdev\include\sys\types.h"\
	"\source\include\byte.h"\
	"\source\include\netinet\in.h"\
	"\source\include\rpc\types.h"\
	"\source\include\rpc\xdr.h"\
	"\source\include\sys\cdefs.h"\
	"\source\include\sys\time.h"\
	

"$(INTDIR)\flaps.obj" : $(SOURCE) $(DEP_CPP_FLAPS) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\gear.c

!IF  "$(CFG)" == "fsim - Win32 Release"

DEP_CPP_GEAR_=\
	"..\dis\dis/datum.h"\
	"..\dis\dis/dis.h"\
	"..\dis\dis/disenum.h"\
	"..\dis\dis/disenum2.h"\
	"..\dis\dis/disenum3.h"\
	"..\dis\dis/disp.h"\
	"..\dis\dis/simx.h"\
	"..\V\lib\Alib.h"\
	"..\V\lib\Vlib.h"\
	".\damage.h"\
	".\imath.h"\
	".\interpolate.h"\
	".\manifest.h"\
	".\pm.h"\
	".\scale.h"\
	"\msdev\include\sys\types.h"\
	"\source\include\byte.h"\
	"\source\include\rpc\types.h"\
	"\source\include\rpc\xdr.h"\
	"\source\include\sys\cdefs.h"\
	

"$(INTDIR)\gear.obj" : $(SOURCE) $(DEP_CPP_GEAR_) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "fsim - Win32 Debug"

DEP_CPP_GEAR_=\
	"..\dis\dis/datum.h"\
	"..\dis\dis/dis.h"\
	"..\dis\dis/disenum.h"\
	"..\dis\dis/disenum2.h"\
	"..\dis\dis/disenum3.h"\
	"..\dis\dis/disp.h"\
	"..\dis\dis/simx.h"\
	"..\V\lib\Alib.h"\
	"..\V\lib\Vlib.h"\
	".\damage.h"\
	".\imath.h"\
	".\interpolate.h"\
	".\manifest.h"\
	".\navaid.h"\
	".\pm.h"\
	".\scale.h"\
	"\msdev\include\sys\types.h"\
	"\source\include\byte.h"\
	"\source\include\netinet\in.h"\
	"\source\include\rpc\types.h"\
	"\source\include\rpc\xdr.h"\
	"\source\include\sys\cdefs.h"\
	"\source\include\sys\time.h"\
	

"$(INTDIR)\gear.obj" : $(SOURCE) $(DEP_CPP_GEAR_) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\hsi.c

!IF  "$(CFG)" == "fsim - Win32 Release"

DEP_CPP_HSI_C=\
	"..\dis\dis/datum.h"\
	"..\dis\dis/dis.h"\
	"..\dis\dis/disenum.h"\
	"..\dis\dis/disenum2.h"\
	"..\dis\dis/disenum3.h"\
	"..\dis\dis/disp.h"\
	"..\dis\dis/simx.h"\
	"..\V\lib\Alib.h"\
	"..\V\lib\VFont.h"\
	"..\V\lib\Vlib.h"\
	".\damage.h"\
	".\hsi.h"\
	".\imath.h"\
	".\interpolate.h"\
	".\manifest.h"\
	".\pm.h"\
	".\scale.h"\
	"\msdev\include\sys\types.h"\
	"\source\include\byte.h"\
	"\source\include\netinet\in.h"\
	"\source\include\rpc\types.h"\
	"\source\include\rpc\xdr.h"\
	"\source\include\sys\cdefs.h"\
	"\source\include\sys\time.h"\
	

"$(INTDIR)\hsi.obj" : $(SOURCE) $(DEP_CPP_HSI_C) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "fsim - Win32 Debug"

DEP_CPP_HSI_C=\
	"..\dis\dis/datum.h"\
	"..\dis\dis/dis.h"\
	"..\dis\dis/disenum.h"\
	"..\dis\dis/disenum2.h"\
	"..\dis\dis/disenum3.h"\
	"..\dis\dis/disp.h"\
	"..\dis\dis/simx.h"\
	"..\V\lib\Alib.h"\
	"..\V\lib\VFont.h"\
	"..\V\lib\Vlib.h"\
	".\damage.h"\
	".\hsi.h"\
	".\imath.h"\
	".\interpolate.h"\
	".\manifest.h"\
	".\navaid.h"\
	".\pm.h"\
	".\scale.h"\
	"\msdev\include\sys\types.h"\
	"\source\include\byte.h"\
	"\source\include\netinet\in.h"\
	"\source\include\rpc\types.h"\
	"\source\include\rpc\xdr.h"\
	"\source\include\sys\cdefs.h"\
	"\source\include\sys\time.h"\
	

"$(INTDIR)\hsi.obj" : $(SOURCE) $(DEP_CPP_HSI_C) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\hud.c

!IF  "$(CFG)" == "fsim - Win32 Release"

DEP_CPP_HUD_C=\
	"..\V\lib\VFont.h"\
	".\horizon.h"\
	".\pm.h"\
	

"$(INTDIR)\hud.obj" : $(SOURCE) $(DEP_CPP_HUD_C) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "fsim - Win32 Debug"

DEP_CPP_HUD_C=\
	"..\dis\dis/datum.h"\
	"..\dis\dis/dis.h"\
	"..\dis\dis/disenum.h"\
	"..\dis\dis/disenum2.h"\
	"..\dis\dis/disenum3.h"\
	"..\dis\dis/disp.h"\
	"..\dis\dis/simx.h"\
	"..\V\lib\Alib.h"\
	"..\V\lib\VFont.h"\
	"..\V\lib\Vlib.h"\
	".\damage.h"\
	".\horizon.h"\
	".\imath.h"\
	".\interpolate.h"\
	".\manifest.h"\
	".\navaid.h"\
	".\pm.h"\
	".\scale.h"\
	"\msdev\include\sys\types.h"\
	"\source\include\byte.h"\
	"\source\include\netinet\in.h"\
	"\source\include\rpc\types.h"\
	"\source\include\rpc\xdr.h"\
	"\source\include\sys\cdefs.h"\
	"\source\include\sys\time.h"\
	

"$(INTDIR)\hud.obj" : $(SOURCE) $(DEP_CPP_HUD_C) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\imath.c
DEP_CPP_IMATH=\
	"..\V\lib\Alib.h"\
	"..\V\lib\Vlib.h"\
	".\imath.h"\
	

"$(INTDIR)\imath.obj" : $(SOURCE) $(DEP_CPP_IMATH) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\interpolate.c
DEP_CPP_INTER=\
	".\interpolate.h"\
	

"$(INTDIR)\interpolate.obj" : $(SOURCE) $(DEP_CPP_INTER) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\inventory.c

!IF  "$(CFG)" == "fsim - Win32 Release"

DEP_CPP_INVEN=\
	"..\dis\dis/datum.h"\
	"..\dis\dis/dis.h"\
	"..\dis\dis/disenum.h"\
	"..\dis\dis/disenum2.h"\
	"..\dis\dis/disenum3.h"\
	"..\dis\dis/disp.h"\
	"..\dis\dis/simx.h"\
	"..\V\lib\Alib.h"\
	"..\V\lib\Vlib.h"\
	".\damage.h"\
	".\imath.h"\
	".\interpolate.h"\
	".\manifest.h"\
	".\pm.h"\
	".\scale.h"\
	"\msdev\include\sys\types.h"\
	"\source\include\byte.h"\
	"\source\include\netinet\in.h"\
	"\source\include\rpc\types.h"\
	"\source\include\rpc\xdr.h"\
	"\source\include\sys\cdefs.h"\
	"\source\include\sys\time.h"\
	

"$(INTDIR)\inventory.obj" : $(SOURCE) $(DEP_CPP_INVEN) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "fsim - Win32 Debug"

DEP_CPP_INVEN=\
	"..\dis\dis/datum.h"\
	"..\dis\dis/dis.h"\
	"..\dis\dis/disenum.h"\
	"..\dis\dis/disenum2.h"\
	"..\dis\dis/disenum3.h"\
	"..\dis\dis/disp.h"\
	"..\dis\dis/simx.h"\
	"..\V\lib\Alib.h"\
	"..\V\lib\Vlib.h"\
	".\damage.h"\
	".\imath.h"\
	".\interpolate.h"\
	".\manifest.h"\
	".\navaid.h"\
	".\pm.h"\
	".\scale.h"\
	"\msdev\include\sys\types.h"\
	"\source\include\byte.h"\
	"\source\include\netinet\in.h"\
	"\source\include\rpc\types.h"\
	"\source\include\rpc\xdr.h"\
	"\source\include\sys\cdefs.h"\
	"\source\include\sys\time.h"\
	

"$(INTDIR)\inventory.obj" : $(SOURCE) $(DEP_CPP_INVEN) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\m61a1.c

!IF  "$(CFG)" == "fsim - Win32 Release"

DEP_CPP_M61A1=\
	"..\dis\dis/datum.h"\
	"..\dis\dis/dis.h"\
	"..\dis\dis/disenum.h"\
	"..\dis\dis/disenum2.h"\
	"..\dis\dis/disenum3.h"\
	"..\dis\dis/disp.h"\
	"..\dis\dis/simx.h"\
	"..\V\lib\Alib.h"\
	"..\V\lib\Vlib.h"\
	".\damage.h"\
	".\dis.h"\
	".\imath.h"\
	".\interpolate.h"\
	".\manifest.h"\
	".\pm.h"\
	".\scale.h"\
	"\msdev\include\sys\types.h"\
	"\source\include\byte.h"\
	"\source\include\netinet\in.h"\
	"\source\include\rpc\types.h"\
	"\source\include\rpc\xdr.h"\
	"\source\include\sys\cdefs.h"\
	"\source\include\sys\time.h"\
	

"$(INTDIR)\m61a1.obj" : $(SOURCE) $(DEP_CPP_M61A1) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "fsim - Win32 Debug"

DEP_CPP_M61A1=\
	"..\dis\dis/datum.h"\
	"..\dis\dis/dis.h"\
	"..\dis\dis/disenum.h"\
	"..\dis\dis/disenum2.h"\
	"..\dis\dis/disenum3.h"\
	"..\dis\dis/disp.h"\
	"..\dis\dis/simx.h"\
	"..\V\lib\Alib.h"\
	"..\V\lib\Vlib.h"\
	".\damage.h"\
	".\dis.h"\
	".\imath.h"\
	".\interpolate.h"\
	".\manifest.h"\
	".\navaid.h"\
	".\pm.h"\
	".\scale.h"\
	"\msdev\include\sys\types.h"\
	"\source\include\byte.h"\
	"\source\include\netinet\in.h"\
	"\source\include\rpc\types.h"\
	"\source\include\rpc\xdr.h"\
	"\source\include\sys\cdefs.h"\
	"\source\include\sys\time.h"\
	

"$(INTDIR)\m61a1.obj" : $(SOURCE) $(DEP_CPP_M61A1) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\missile.c
DEP_CPP_MISSI=\
	"..\dis\dis/datum.h"\
	"..\dis\dis/dis.h"\
	"..\dis\dis/disenum.h"\
	"..\dis\dis/disenum2.h"\
	"..\dis\dis/disenum3.h"\
	"..\dis\dis/disp.h"\
	"..\dis\dis/simx.h"\
	"..\V\lib\Alib.h"\
	"..\V\lib\Vlib.h"\
	".\damage.h"\
	".\dis.h"\
	".\imath.h"\
	".\interpolate.h"\
	".\manifest.h"\
	".\navaid.h"\
	".\pm.h"\
	".\scale.h"\
	"\msdev\include\sys\types.h"\
	"\source\include\byte.h"\
	"\source\include\netinet\in.h"\
	"\source\include\rpc\types.h"\
	"\source\include\rpc\xdr.h"\
	"\source\include\sys\cdefs.h"\
	"\source\include\sys\time.h"\
	

"$(INTDIR)\missile.obj" : $(SOURCE) $(DEP_CPP_MISSI) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\missilec.c

!IF  "$(CFG)" == "fsim - Win32 Release"

DEP_CPP_MISSIL=\
	"..\dis\dis/datum.h"\
	"..\dis\dis/dis.h"\
	"..\dis\dis/disenum.h"\
	"..\dis\dis/disenum2.h"\
	"..\dis\dis/disenum3.h"\
	"..\dis\dis/disp.h"\
	"..\dis\dis/simx.h"\
	"..\V\lib\Alib.h"\
	"..\V\lib\Vlib.h"\
	".\damage.h"\
	".\imath.h"\
	".\interpolate.h"\
	".\manifest.h"\
	".\pm.h"\
	".\scale.h"\
	"\msdev\include\sys\types.h"\
	"\source\include\byte.h"\
	"\source\include\netinet\in.h"\
	"\source\include\rpc\types.h"\
	"\source\include\rpc\xdr.h"\
	"\source\include\sys\cdefs.h"\
	"\source\include\sys\time.h"\
	

"$(INTDIR)\missilec.obj" : $(SOURCE) $(DEP_CPP_MISSIL) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "fsim - Win32 Debug"

DEP_CPP_MISSIL=\
	"..\dis\dis/datum.h"\
	"..\dis\dis/dis.h"\
	"..\dis\dis/disenum.h"\
	"..\dis\dis/disenum2.h"\
	"..\dis\dis/disenum3.h"\
	"..\dis\dis/disp.h"\
	"..\dis\dis/simx.h"\
	"..\V\lib\Alib.h"\
	"..\V\lib\Vlib.h"\
	".\damage.h"\
	".\imath.h"\
	".\interpolate.h"\
	".\manifest.h"\
	".\navaid.h"\
	".\pm.h"\
	".\scale.h"\
	"\msdev\include\sys\types.h"\
	"\source\include\byte.h"\
	"\source\include\netinet\in.h"\
	"\source\include\rpc\types.h"\
	"\source\include\rpc\xdr.h"\
	"\source\include\sys\cdefs.h"\
	"\source\include\sys\time.h"\
	

"$(INTDIR)\missilec.obj" : $(SOURCE) $(DEP_CPP_MISSIL) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\navaid.c

!IF  "$(CFG)" == "fsim - Win32 Release"

DEP_CPP_NAVAI=\
	"..\dis\dis/datum.h"\
	"..\dis\dis/dis.h"\
	"..\dis\dis/disenum.h"\
	"..\dis\dis/disenum2.h"\
	"..\dis\dis/disenum3.h"\
	"..\dis\dis/disp.h"\
	"..\dis\dis/simx.h"\
	"..\V\lib\Alib.h"\
	"..\V\lib\Vlib.h"\
	".\alarm.h"\
	".\damage.h"\
	".\imath.h"\
	".\interpolate.h"\
	".\manifest.h"\
	".\pm.h"\
	".\scale.h"\
	"\msdev\include\sys\types.h"\
	"\source\include\byte.h"\
	"\source\include\netinet\in.h"\
	"\source\include\rpc\types.h"\
	"\source\include\rpc\xdr.h"\
	"\source\include\sys\cdefs.h"\
	"\source\include\sys\time.h"\
	

"$(INTDIR)\navaid.obj" : $(SOURCE) $(DEP_CPP_NAVAI) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "fsim - Win32 Debug"

DEP_CPP_NAVAI=\
	"..\dis\dis/datum.h"\
	"..\dis\dis/dis.h"\
	"..\dis\dis/disenum.h"\
	"..\dis\dis/disenum2.h"\
	"..\dis\dis/disenum3.h"\
	"..\dis\dis/disp.h"\
	"..\dis\dis/simx.h"\
	"..\V\lib\Alib.h"\
	"..\V\lib\Vlib.h"\
	".\alarm.h"\
	".\damage.h"\
	".\imath.h"\
	".\interpolate.h"\
	".\manifest.h"\
	".\navaid.h"\
	".\pm.h"\
	".\scale.h"\
	"\msdev\include\sys\types.h"\
	"\source\include\byte.h"\
	"\source\include\netinet\in.h"\
	"\source\include\rpc\types.h"\
	"\source\include\rpc\xdr.h"\
	"\source\include\sys\cdefs.h"\
	"\source\include\sys\time.h"\
	

"$(INTDIR)\navaid.obj" : $(SOURCE) $(DEP_CPP_NAVAI) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\panel.c

!IF  "$(CFG)" == "fsim - Win32 Release"

DEP_CPP_PANEL=\
	"..\dis\dis/datum.h"\
	"..\dis\dis/dis.h"\
	"..\dis\dis/disenum.h"\
	"..\dis\dis/disenum2.h"\
	"..\dis\dis/disenum3.h"\
	"..\dis\dis/disp.h"\
	"..\dis\dis/simx.h"\
	"..\V\lib\Alib.h"\
	"..\V\lib\Vlib.h"\
	".\damage.h"\
	".\imath.h"\
	".\interpolate.h"\
	".\manifest.h"\
	".\pm.h"\
	".\scale.h"\
	"\msdev\include\sys\types.h"\
	"\source\include\byte.h"\
	"\source\include\netinet\in.h"\
	"\source\include\rpc\types.h"\
	"\source\include\rpc\xdr.h"\
	"\source\include\sys\cdefs.h"\
	"\source\include\sys\time.h"\
	{$(INCLUDE)}"\ddraw.h"\
	
NODEP_CPP_PANEL=\
	".\bitmap.h"\
	

"$(INTDIR)\panel.obj" : $(SOURCE) $(DEP_CPP_PANEL) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "fsim - Win32 Debug"

DEP_CPP_PANEL=\
	"..\dis\dis/datum.h"\
	"..\dis\dis/dis.h"\
	"..\dis\dis/disenum.h"\
	"..\dis\dis/disenum2.h"\
	"..\dis\dis/disenum3.h"\
	"..\dis\dis/disp.h"\
	"..\dis\dis/simx.h"\
	"..\V\lib\Alib.h"\
	"..\V\lib\Vlib.h"\
	".\damage.h"\
	".\imath.h"\
	".\interpolate.h"\
	".\manifest.h"\
	".\navaid.h"\
	".\pm.h"\
	".\scale.h"\
	"\msdev\include\sys\types.h"\
	"\source\include\byte.h"\
	"\source\include\netinet\in.h"\
	"\source\include\rpc\types.h"\
	"\source\include\rpc\xdr.h"\
	"\source\include\sys\cdefs.h"\
	"\source\include\sys\time.h"\
	{$(INCLUDE)}"\ddraw.h"\
	
NODEP_CPP_PANEL=\
	".\bitmap.h"\
	

"$(INTDIR)\panel.obj" : $(SOURCE) $(DEP_CPP_PANEL) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\place.c

!IF  "$(CFG)" == "fsim - Win32 Release"

DEP_CPP_PLACE=\
	"..\dis\dis/datum.h"\
	"..\dis\dis/dis.h"\
	"..\dis\dis/disenum.h"\
	"..\dis\dis/disenum2.h"\
	"..\dis\dis/disenum3.h"\
	"..\dis\dis/disp.h"\
	"..\dis\dis/simx.h"\
	"..\V\lib\Alib.h"\
	"..\V\lib\Vlib.h"\
	".\damage.h"\
	".\imath.h"\
	".\interpolate.h"\
	".\manifest.h"\
	".\pm.h"\
	".\scale.h"\
	"\msdev\include\sys\types.h"\
	"\source\include\byte.h"\
	"\source\include\netinet\in.h"\
	"\source\include\rpc\types.h"\
	"\source\include\rpc\xdr.h"\
	"\source\include\sys\cdefs.h"\
	"\source\include\sys\time.h"\
	

"$(INTDIR)\place.obj" : $(SOURCE) $(DEP_CPP_PLACE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "fsim - Win32 Debug"

DEP_CPP_PLACE=\
	"..\dis\dis/datum.h"\
	"..\dis\dis/dis.h"\
	"..\dis\dis/disenum.h"\
	"..\dis\dis/disenum2.h"\
	"..\dis\dis/disenum3.h"\
	"..\dis\dis/disp.h"\
	"..\dis\dis/simx.h"\
	"..\V\lib\Alib.h"\
	"..\V\lib\Vlib.h"\
	".\damage.h"\
	".\imath.h"\
	".\interpolate.h"\
	".\manifest.h"\
	".\navaid.h"\
	".\pm.h"\
	".\scale.h"\
	"\msdev\include\sys\types.h"\
	"\source\include\byte.h"\
	"\source\include\netinet\in.h"\
	"\source\include\rpc\types.h"\
	"\source\include\rpc\xdr.h"\
	"\source\include\sys\cdefs.h"\
	"\source\include\sys\time.h"\
	

"$(INTDIR)\place.obj" : $(SOURCE) $(DEP_CPP_PLACE) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\pm.c

!IF  "$(CFG)" == "fsim - Win32 Release"

DEP_CPP_PM_C2a=\
	"..\dis\dis/datum.h"\
	"..\dis\dis/dis.h"\
	"..\dis\dis/disenum.h"\
	"..\dis\dis/disenum2.h"\
	"..\dis\dis/disenum3.h"\
	"..\dis\dis/disp.h"\
	"..\dis\dis/simx.h"\
	"..\V\lib\Alib.h"\
	"..\V\lib\Vlib.h"\
	".\damage.h"\
	".\imath.h"\
	".\interpolate.h"\
	".\manifest.h"\
	".\pm.h"\
	".\scale.h"\
	"\msdev\include\sys\types.h"\
	"\source\include\byte.h"\
	"\source\include\netinet\in.h"\
	"\source\include\rpc\types.h"\
	"\source\include\rpc\xdr.h"\
	"\source\include\sys\cdefs.h"\
	"\source\include\sys\time.h"\
	

"$(INTDIR)\pm.obj" : $(SOURCE) $(DEP_CPP_PM_C2a) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "fsim - Win32 Debug"

DEP_CPP_PM_C2a=\
	"..\dis\dis/datum.h"\
	"..\dis\dis/dis.h"\
	"..\dis\dis/disenum.h"\
	"..\dis\dis/disenum2.h"\
	"..\dis\dis/disenum3.h"\
	"..\dis\dis/disp.h"\
	"..\dis\dis/simx.h"\
	"..\V\lib\Alib.h"\
	"..\V\lib\Vlib.h"\
	".\damage.h"\
	".\imath.h"\
	".\interpolate.h"\
	".\manifest.h"\
	".\navaid.h"\
	".\pm.h"\
	".\scale.h"\
	"\msdev\include\sys\types.h"\
	"\source\include\byte.h"\
	"\source\include\netinet\in.h"\
	"\source\include\rpc\types.h"\
	"\source\include\rpc\xdr.h"\
	"\source\include\sys\cdefs.h"\
	"\source\include\sys\time.h"\
	

"$(INTDIR)\pm.obj" : $(SOURCE) $(DEP_CPP_PM_C2a) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\radar.c

!IF  "$(CFG)" == "fsim - Win32 Release"

DEP_CPP_RADAR=\
	"..\dis\dis/datum.h"\
	"..\dis\dis/dis.h"\
	"..\dis\dis/disenum.h"\
	"..\dis\dis/disenum2.h"\
	"..\dis\dis/disenum3.h"\
	"..\dis\dis/disp.h"\
	"..\dis\dis/simx.h"\
	"..\V\lib\Alib.h"\
	"..\V\lib\Vlib.h"\
	".\damage.h"\
	".\imath.h"\
	".\interpolate.h"\
	".\manifest.h"\
	".\pm.h"\
	".\scale.h"\
	"\msdev\include\sys\types.h"\
	"\source\include\byte.h"\
	"\source\include\netinet\in.h"\
	"\source\include\rpc\types.h"\
	"\source\include\rpc\xdr.h"\
	"\source\include\sys\cdefs.h"\
	"\source\include\sys\time.h"\
	{$(INCLUDE)}"\ddraw.h"\
	

"$(INTDIR)\radar.obj" : $(SOURCE) $(DEP_CPP_RADAR) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "fsim - Win32 Debug"

DEP_CPP_RADAR=\
	"..\dis\dis/datum.h"\
	"..\dis\dis/dis.h"\
	"..\dis\dis/disenum.h"\
	"..\dis\dis/disenum2.h"\
	"..\dis\dis/disenum3.h"\
	"..\dis\dis/disp.h"\
	"..\dis\dis/simx.h"\
	"..\V\lib\Alib.h"\
	"..\V\lib\Vlib.h"\
	".\damage.h"\
	".\imath.h"\
	".\interpolate.h"\
	".\manifest.h"\
	".\navaid.h"\
	".\pm.h"\
	".\scale.h"\
	"\msdev\include\sys\types.h"\
	"\source\include\byte.h"\
	"\source\include\netinet\in.h"\
	"\source\include\rpc\types.h"\
	"\source\include\rpc\xdr.h"\
	"\source\include\sys\cdefs.h"\
	"\source\include\sys\time.h"\
	{$(INCLUDE)}"\ddraw.h"\
	

"$(INTDIR)\radar.obj" : $(SOURCE) $(DEP_CPP_RADAR) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\render.c

!IF  "$(CFG)" == "fsim - Win32 Release"

DEP_CPP_RENDE=\
	"..\dis\dis/datum.h"\
	"..\dis\dis/dis.h"\
	"..\dis\dis/disenum.h"\
	"..\dis\dis/disenum2.h"\
	"..\dis\dis/disenum3.h"\
	"..\dis\dis/disp.h"\
	"..\dis\dis/simx.h"\
	"..\V\lib\Alib.h"\
	"..\V\lib\Vlib.h"\
	".\damage.h"\
	".\imath.h"\
	".\interpolate.h"\
	".\manifest.h"\
	".\pm.h"\
	".\scale.h"\
	"\msdev\include\sys\types.h"\
	"\source\include\byte.h"\
	"\source\include\netinet\in.h"\
	"\source\include\rpc\types.h"\
	"\source\include\rpc\xdr.h"\
	"\source\include\sys\cdefs.h"\
	"\source\include\sys\time.h"\
	{$(INCLUDE)}"\ddraw.h"\
	
NODEP_CPP_RENDE=\
	".\bitmap.h"\
	

"$(INTDIR)\render.obj" : $(SOURCE) $(DEP_CPP_RENDE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "fsim - Win32 Debug"

DEP_CPP_RENDE=\
	"..\dis\dis/datum.h"\
	"..\dis\dis/dis.h"\
	"..\dis\dis/disenum.h"\
	"..\dis\dis/disenum2.h"\
	"..\dis\dis/disenum3.h"\
	"..\dis\dis/disp.h"\
	"..\dis\dis/simx.h"\
	"..\V\lib\Alib.h"\
	"..\V\lib\Vlib.h"\
	".\damage.h"\
	".\imath.h"\
	".\interpolate.h"\
	".\manifest.h"\
	".\navaid.h"\
	".\pm.h"\
	".\scale.h"\
	"\msdev\include\sys\types.h"\
	"\source\include\byte.h"\
	"\source\include\netinet\in.h"\
	"\source\include\rpc\types.h"\
	"\source\include\rpc\xdr.h"\
	"\source\include\sys\cdefs.h"\
	"\source\include\sys\time.h"\
	{$(INCLUDE)}"\ddraw.h"\
	
NODEP_CPP_RENDE=\
	".\bitmap.h"\
	

"$(INTDIR)\render.obj" : $(SOURCE) $(DEP_CPP_RENDE) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\resupply.c

!IF  "$(CFG)" == "fsim - Win32 Release"

DEP_CPP_RESUP=\
	"..\dis\dis/datum.h"\
	"..\dis\dis/dis.h"\
	"..\dis\dis/disenum.h"\
	"..\dis\dis/disenum2.h"\
	"..\dis\dis/disenum3.h"\
	"..\dis\dis/disp.h"\
	"..\dis\dis/simx.h"\
	"..\V\lib\Alib.h"\
	"..\V\lib\Vlib.h"\
	".\alarm.h"\
	".\damage.h"\
	".\imath.h"\
	".\interpolate.h"\
	".\manifest.h"\
	".\pm.h"\
	".\scale.h"\
	"\msdev\include\sys\types.h"\
	"\source\include\byte.h"\
	"\source\include\netinet\in.h"\
	"\source\include\rpc\types.h"\
	"\source\include\rpc\xdr.h"\
	"\source\include\sys\cdefs.h"\
	"\source\include\sys\time.h"\
	

"$(INTDIR)\resupply.obj" : $(SOURCE) $(DEP_CPP_RESUP) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "fsim - Win32 Debug"

DEP_CPP_RESUP=\
	"..\dis\dis/datum.h"\
	"..\dis\dis/dis.h"\
	"..\dis\dis/disenum.h"\
	"..\dis\dis/disenum2.h"\
	"..\dis\dis/disenum3.h"\
	"..\dis\dis/disp.h"\
	"..\dis\dis/simx.h"\
	"..\V\lib\Alib.h"\
	"..\V\lib\Vlib.h"\
	".\alarm.h"\
	".\damage.h"\
	".\imath.h"\
	".\interpolate.h"\
	".\manifest.h"\
	".\navaid.h"\
	".\pm.h"\
	".\scale.h"\
	"\msdev\include\sys\types.h"\
	"\source\include\byte.h"\
	"\source\include\netinet\in.h"\
	"\source\include\rpc\types.h"\
	"\source\include\rpc\xdr.h"\
	"\source\include\sys\cdefs.h"\
	"\source\include\sys\time.h"\
	

"$(INTDIR)\resupply.obj" : $(SOURCE) $(DEP_CPP_RESUP) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\scale.c
DEP_CPP_SCALE=\
	"..\V\lib\Alib.h"\
	"..\V\lib\Vlib.h"\
	".\scale.h"\
	

"$(INTDIR)\scale.obj" : $(SOURCE) $(DEP_CPP_SCALE) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\terrain.c

!IF  "$(CFG)" == "fsim - Win32 Release"

DEP_CPP_TERRA=\
	"..\dis\dis/datum.h"\
	"..\dis\dis/dis.h"\
	"..\dis\dis/disenum.h"\
	"..\dis\dis/disenum2.h"\
	"..\dis\dis/disenum3.h"\
	"..\dis\dis/disp.h"\
	"..\dis\dis/simx.h"\
	"..\V\lib\Alib.h"\
	"..\V\lib\Vlib.h"\
	".\damage.h"\
	".\imath.h"\
	".\interpolate.h"\
	".\manifest.h"\
	".\pm.h"\
	".\scale.h"\
	"\msdev\include\sys\types.h"\
	"\source\include\byte.h"\
	"\source\include\netinet\in.h"\
	"\source\include\rpc\types.h"\
	"\source\include\rpc\xdr.h"\
	"\source\include\sys\cdefs.h"\
	"\source\include\sys\time.h"\
	

"$(INTDIR)\terrain.obj" : $(SOURCE) $(DEP_CPP_TERRA) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "fsim - Win32 Debug"

DEP_CPP_TERRA=\
	"..\dis\dis/datum.h"\
	"..\dis\dis/dis.h"\
	"..\dis\dis/disenum.h"\
	"..\dis\dis/disenum2.h"\
	"..\dis\dis/disenum3.h"\
	"..\dis\dis/disp.h"\
	"..\dis\dis/simx.h"\
	"..\V\lib\Alib.h"\
	"..\V\lib\Vlib.h"\
	".\damage.h"\
	".\imath.h"\
	".\interpolate.h"\
	".\manifest.h"\
	".\navaid.h"\
	".\pm.h"\
	".\scale.h"\
	"\msdev\include\sys\types.h"\
	"\source\include\byte.h"\
	"\source\include\netinet\in.h"\
	"\source\include\rpc\types.h"\
	"\source\include\rpc\xdr.h"\
	"\source\include\sys\cdefs.h"\
	"\source\include\sys\time.h"\
	

"$(INTDIR)\terrain.obj" : $(SOURCE) $(DEP_CPP_TERRA) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\transpose.c
DEP_CPP_TRANS=\
	"..\V\lib\Alib.h"\
	"..\V\lib\Vlib.h"\
	

"$(INTDIR)\transpose.obj" : $(SOURCE) $(DEP_CPP_TRANS) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\aim9m.c

!IF  "$(CFG)" == "fsim - Win32 Release"

DEP_CPP_AIM9M=\
	"..\dis\dis/datum.h"\
	"..\dis\dis/dis.h"\
	"..\dis\dis/disenum.h"\
	"..\dis\dis/disenum2.h"\
	"..\dis\dis/disenum3.h"\
	"..\dis\dis/disp.h"\
	"..\dis\dis/simx.h"\
	"..\V\lib\Alib.h"\
	"..\V\lib\Vlib.h"\
	".\damage.h"\
	".\imath.h"\
	".\interpolate.h"\
	".\manifest.h"\
	".\pm.h"\
	".\scale.h"\
	"\msdev\include\sys\types.h"\
	"\source\include\byte.h"\
	"\source\include\netinet\in.h"\
	"\source\include\rpc\types.h"\
	"\source\include\rpc\xdr.h"\
	"\source\include\sys\cdefs.h"\
	"\source\include\sys\time.h"\
	

"$(INTDIR)\aim9m.obj" : $(SOURCE) $(DEP_CPP_AIM9M) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "fsim - Win32 Debug"

DEP_CPP_AIM9M=\
	"..\dis\dis/datum.h"\
	"..\dis\dis/dis.h"\
	"..\dis\dis/disenum.h"\
	"..\dis\dis/disenum2.h"\
	"..\dis\dis/disenum3.h"\
	"..\dis\dis/disp.h"\
	"..\dis\dis/simx.h"\
	"..\V\lib\Alib.h"\
	"..\V\lib\Vlib.h"\
	".\damage.h"\
	".\imath.h"\
	".\interpolate.h"\
	".\manifest.h"\
	".\navaid.h"\
	".\pm.h"\
	".\scale.h"\
	"\msdev\include\sys\types.h"\
	"\source\include\byte.h"\
	"\source\include\netinet\in.h"\
	"\source\include\rpc\types.h"\
	"\source\include\rpc\xdr.h"\
	"\source\include\sys\cdefs.h"\
	"\source\include\sys\time.h"\
	

"$(INTDIR)\aim9m.obj" : $(SOURCE) $(DEP_CPP_AIM9M) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\planes.c

!IF  "$(CFG)" == "fsim - Win32 Release"

DEP_CPP_PLANE=\
	"..\dis\dis/datum.h"\
	"..\dis\dis/dis.h"\
	"..\dis\dis/disenum.h"\
	"..\dis\dis/disenum2.h"\
	"..\dis\dis/disenum3.h"\
	"..\dis\dis/disp.h"\
	"..\dis\dis/simx.h"\
	"..\V\lib\Alib.h"\
	"..\V\lib\Vlib.h"\
	".\damage.h"\
	".\dis.h"\
	".\imath.h"\
	".\interpolate.h"\
	".\manifest.h"\
	".\pm.h"\
	".\scale.h"\
	"\msdev\include\sys\types.h"\
	"\source\include\byte.h"\
	"\source\include\netinet\in.h"\
	"\source\include\rpc\types.h"\
	"\source\include\rpc\xdr.h"\
	"\source\include\sys\cdefs.h"\
	"\source\include\sys\time.h"\
	

"$(INTDIR)\planes.obj" : $(SOURCE) $(DEP_CPP_PLANE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "fsim - Win32 Debug"

DEP_CPP_PLANE=\
	"..\dis\dis/datum.h"\
	"..\dis\dis/dis.h"\
	"..\dis\dis/disenum.h"\
	"..\dis\dis/disenum2.h"\
	"..\dis\dis/disenum3.h"\
	"..\dis\dis/disp.h"\
	"..\dis\dis/simx.h"\
	"..\V\lib\Alib.h"\
	"..\V\lib\Vlib.h"\
	".\damage.h"\
	".\dis.h"\
	".\imath.h"\
	".\interpolate.h"\
	".\manifest.h"\
	".\navaid.h"\
	".\pm.h"\
	".\scale.h"\
	"\msdev\include\sys\types.h"\
	"\source\include\byte.h"\
	"\source\include\netinet\in.h"\
	"\source\include\rpc\types.h"\
	"\source\include\rpc\xdr.h"\
	"\source\include\sys\cdefs.h"\
	"\source\include\sys\time.h"\
	

"$(INTDIR)\planes.obj" : $(SOURCE) $(DEP_CPP_PLANE) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\audio.c

!IF  "$(CFG)" == "fsim - Win32 Release"

DEP_CPP_AUDIO=\
	"..\dis\dis/datum.h"\
	"..\dis\dis/dis.h"\
	"..\dis\dis/disenum.h"\
	"..\dis\dis/disenum2.h"\
	"..\dis\dis/disenum3.h"\
	"..\dis\dis/disp.h"\
	"..\dis\dis/simx.h"\
	"..\V\lib\Alib.h"\
	"..\V\lib\Vlib.h"\
	".\damage.h"\
	".\imath.h"\
	".\interpolate.h"\
	".\manifest.h"\
	".\pm.h"\
	".\scale.h"\
	"\msdev\include\sys\types.h"\
	"\source\include\byte.h"\
	"\source\include\netinet\in.h"\
	"\source\include\rpc\types.h"\
	"\source\include\rpc\xdr.h"\
	"\source\include\sys\cdefs.h"\
	"\source\include\sys\time.h"\
	

"$(INTDIR)\audio.obj" : $(SOURCE) $(DEP_CPP_AUDIO) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "fsim - Win32 Debug"

DEP_CPP_AUDIO=\
	"..\dis\dis/datum.h"\
	"..\dis\dis/dis.h"\
	"..\dis\dis/disenum.h"\
	"..\dis\dis/disenum2.h"\
	"..\dis\dis/disenum3.h"\
	"..\dis\dis/disp.h"\
	"..\dis\dis/simx.h"\
	"..\V\lib\Alib.h"\
	"..\V\lib\Vlib.h"\
	".\damage.h"\
	".\imath.h"\
	".\interpolate.h"\
	".\manifest.h"\
	".\navaid.h"\
	".\pm.h"\
	".\scale.h"\
	"\msdev\include\sys\types.h"\
	"\source\include\byte.h"\
	"\source\include\netinet\in.h"\
	"\source\include\rpc\types.h"\
	"\source\include\rpc\xdr.h"\
	"\source\include\sys\cdefs.h"\
	"\source\include\sys\time.h"\
	

"$(INTDIR)\audio.obj" : $(SOURCE) $(DEP_CPP_AUDIO) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\init.c

!IF  "$(CFG)" == "fsim - Win32 Release"

DEP_CPP_INIT_=\
	"..\dis\dis/datum.h"\
	"..\dis\dis/dis.h"\
	"..\dis\dis/disenum.h"\
	"..\dis\dis/disenum2.h"\
	"..\dis\dis/disenum3.h"\
	"..\dis\dis/disp.h"\
	"..\dis\dis/simx.h"\
	"..\V\lib\Alib.h"\
	"..\V\lib\Vlib.h"\
	".\alarm.h"\
	".\damage.h"\
	".\imath.h"\
	".\interpolate.h"\
	".\manifest.h"\
	".\pm.h"\
	".\scale.h"\
	"\msdev\include\sys\stat.h"\
	"\msdev\include\sys\types.h"\
	"\source\include\byte.h"\
	"\source\include\netinet\in.h"\
	"\source\include\rpc\types.h"\
	"\source\include\rpc\xdr.h"\
	"\source\include\sys\cdefs.h"\
	"\source\include\sys\time.h"\
	

"$(INTDIR)\init.obj" : $(SOURCE) $(DEP_CPP_INIT_) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "fsim - Win32 Debug"

DEP_CPP_INIT_=\
	"..\dis\dis/datum.h"\
	"..\dis\dis/dis.h"\
	"..\dis\dis/disenum.h"\
	"..\dis\dis/disenum2.h"\
	"..\dis\dis/disenum3.h"\
	"..\dis\dis/disp.h"\
	"..\dis\dis/simx.h"\
	"..\V\lib\Alib.h"\
	"..\V\lib\Vlib.h"\
	".\alarm.h"\
	".\damage.h"\
	".\imath.h"\
	".\interpolate.h"\
	".\manifest.h"\
	".\navaid.h"\
	".\pm.h"\
	".\scale.h"\
	"\msdev\include\sys\stat.h"\
	"\msdev\include\sys\types.h"\
	"\source\include\byte.h"\
	"\source\include\netinet\in.h"\
	"\source\include\rpc\types.h"\
	"\source\include\rpc\xdr.h"\
	"\source\include\sys\cdefs.h"\
	"\source\include\sys\time.h"\
	

"$(INTDIR)\init.obj" : $(SOURCE) $(DEP_CPP_INIT_) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\windows.c

!IF  "$(CFG)" == "fsim - Win32 Release"

DEP_CPP_WINDO=\
	"..\dis\dis/datum.h"\
	"..\dis\dis/dis.h"\
	"..\dis\dis/disenum.h"\
	"..\dis\dis/disenum2.h"\
	"..\dis\dis/disenum3.h"\
	"..\dis\dis/disp.h"\
	"..\dis\dis/simx.h"\
	"..\V\lib\Alib.h"\
	"..\V\lib\Vlib.h"\
	".\damage.h"\
	".\imath.h"\
	".\interpolate.h"\
	".\manifest.h"\
	".\pm.h"\
	".\scale.h"\
	"\msdev\include\sys\types.h"\
	"\source\include\byte.h"\
	"\source\include\netinet\in.h"\
	"\source\include\rpc\types.h"\
	"\source\include\rpc\xdr.h"\
	"\source\include\sys\cdefs.h"\
	"\source\include\sys\time.h"\
	

"$(INTDIR)\windows.obj" : $(SOURCE) $(DEP_CPP_WINDO) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "fsim - Win32 Debug"

DEP_CPP_WINDO=\
	"..\dis\dis/datum.h"\
	"..\dis\dis/dis.h"\
	"..\dis\dis/disenum.h"\
	"..\dis\dis/disenum2.h"\
	"..\dis\dis/disenum3.h"\
	"..\dis\dis/disp.h"\
	"..\dis\dis/simx.h"\
	"..\V\lib\Alib.h"\
	"..\V\lib\Vlib.h"\
	".\damage.h"\
	".\imath.h"\
	".\interpolate.h"\
	".\manifest.h"\
	".\navaid.h"\
	".\pm.h"\
	".\scale.h"\
	"\msdev\include\sys\types.h"\
	"\source\include\byte.h"\
	"\source\include\netinet\in.h"\
	"\source\include\rpc\types.h"\
	"\source\include\rpc\xdr.h"\
	"\source\include\sys\cdefs.h"\
	"\source\include\sys\time.h"\
	

"$(INTDIR)\windows.obj" : $(SOURCE) $(DEP_CPP_WINDO) "$(INTDIR)"


!ENDIF 

# End Source File
# End Target
# End Project
################################################################################
