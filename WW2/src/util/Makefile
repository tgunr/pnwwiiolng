CFLAGS = -I ./  -I../osg -I../sdm -DIS_BIG_ENDIAN=1  -Wall # ggdb -Wuninitialized -O
CXXFLAGS = ${CFLAGS}

commentlist: commentlist.o
	g++ -o commentlist commentlist.o -losg -losgDB -lOpenThreads

fltlist: fltlist.o ../sdm/sdm.o 
	g++ -o fltlist fltlist.o ../sdm/sdm.o

texlist: texlist.o
	g++ -o texlist texlist.o -losg -losgDB -lOpenThreads

makeatlas: makeatlas.o ../osg/osgTextureAtlas.o
	g++ -o makeatlas makeatlas.o ../osg/osgTextureAtlas.o -losg -losgDB -lOpenThreads -framework OpenGL -framework GLUT