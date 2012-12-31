To build Universal binary on the mac:

	env	\
		CFLAGS=-mmacosx-version-min=10.4 -isysroot /Developer/SDKs/MacOSX10.4u.sdk -arch i386 -arch ppc \
		CXXFLAGS=-mmacosx-version-min=10.4 -isysroot /Developer/SDKs/MacOSX10.4u.sdk -arch i386 -arch ppc \
		LDFLAGS=-mmacosx-version-min=10.4 -arch i386 -arch ppc	\
			./configure	\
				--with-ft-prefix=/usr/local \
				--disable-dependency-tracking
	make

Don't forget the --disable-dependency-tracking or else.

