CFLAGS = -I ./  -DIS_BIG_ENDIAN=__BIG_ENDIAN__  -Wall -D__USE_OSX_AGL_IMPLEMENTATION__
# ggdb -Wuninitialized -O 
CXXFLAGS = ${CFLAGS}

sdmfix: sdmfix.o sdm.o DTLOctetHeightField.o DTLSupercellHeightField.o
	c++ -o sdmfix sdmfix.o sdm.o  DTLOctetHeightField.o DTLSupercellHeightField.o -losg -losgDB -losgUtil -framework OpenGL

atlasmapicons: atlasmapicons.o
	c++ -o atlasmapicons atlasmapicons.o -losg -losgDB -losgUtil -losgProducer -lProducer -lOpenThreads -losgText -losgGA -framework OpenGL


flttex: flttex.o
	c++ -o flttex flttex.o -losg -losgDB -losgUtil -losgProducer -lProducer -lOpenThreads -losgText -losgGA

inertia: inertia.o
	c++ -o inertia inertia.o -losg -losgDB -losgUtil -losgProducer -lProducer -lOpenThreads -losgText -losgGA -framework OpenGL

sdminfo: sdminfo.o sdm.o
	c++ -o sdminfo sdminfo.o sdm.o

genmapicons: genmapicons.o
	c++ -o genmapicons genmapicons.o -losg -losgDB -losgUtil -losgProducer -lProducer -lOpenThreads -losgText -losgGA -framework OpenGL -lboost_filesystem

sdmtobil: sdmtobil.o sdm.o
	c++ -o sdmtobil sdmtobil.o sdm.o

ww2genmap: ww2genmap.o sdm.o 
	c++ -o ww2genmap ww2genmap.o sdm.o -lgd 

#-ggdb
