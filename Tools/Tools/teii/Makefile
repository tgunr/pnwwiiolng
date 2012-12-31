CPPFLAGS = -I/usr/X11R6/include -I./
LIBS = -L./ -ltiff -losg -losgDB -losgUtil -losgText -losgGA -losgProducer -lProducer -lOpenThreads -framework Carbon -lgdal -logdi

specularmarble: specularmarble.o Dataset.o Image.o
	c++ -o specularmarble specularmarble.o Dataset.o Image.o $(LIBS)

osgbluemarble: osgbluemarble.o Dataset.o Image.o
	c++ -o osgbluemarble osgbluemarble.o Dataset.o Image.o $(LIBS)
