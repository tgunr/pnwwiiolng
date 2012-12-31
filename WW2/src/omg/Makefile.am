# Generate header files from Object Model Generator (OMG)
# XML definitions.

srcdir = @srcdir@
VPATH = @srcdir@

OMG_TO_H = omg-to-h.py
OMG_DTD  = omg.dtd

noinst_PROGRAMS = omg #OMGApp.exe

# How to build a .h file from an omg file.
.xml.h:	$(OMG_TO_H) $(OMG_DTD)
	@python $(OMG_TO_H) --dtd $(OMG_DTD) $< $@

omg: *.py $(OMG_DTD)	\
	omgMissionStatus.h	\
	omgMissionSortieData.h

#OMG_FILES = OMGApp.cs OMGElemeng.cs OMGFile.cs #OMGClass.cs
#OMGApp.exe:	$(OMG_FILES)
#	gmcs $(OMG_FILES) -out:OMGApp.exe

#	omgVehicleIntro.h
#	omgVehicleUpdate.h

omg_SOURCES = \
	omgMissionStatus.xml \
	omgMissionSortieData.xml

#	omgVehicleIntro.xml \
#	omgVehicleUpdate.xml

omgMissionStatus.h:: *.py $(OMG_DTD)

omgMissionSortieData.h:: *.py $(OMG_DTD)

#omgVehicleIntro.h:: *.py $(OMG_DTD)

#omgVehicleUpdate.h:: *.py $(OMG_DTD)
