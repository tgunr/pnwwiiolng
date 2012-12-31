#include <syscfg.h> // Path is project specific.
#include <iostream>
#include <GLUT/glut.h>

#include <osgDB/readFile>
#include <osg/Image>

#include "osgTextureAtlas.h"

static bool ispow2(unsigned int l)
{
    return (l == 0) ? false: (!(l & (l - 1)));
}

int main(int argc, char** argv)
{
	// just to initialize OpenGL so we don't crash
	glutInit(&argc, argv);
	glutCreateWindow("init");

	osgTextureAtlasCreator c(2048);

	for(int i = 2; i < argc; i++)
	{
		std::cout << "\n";
		osg::ref_ptr<osg::Image> image = osgDB::readImageFile(argv[i]);
		if(image.valid())
		{
			std::cout << image->getFileName() << "(" << image->s() << "x" << image->t() << ")";
			if(image->s() != image->t())
			{
				std::cout << " not square";
				continue;
			}
			if(!ispow2(image->s()))
			{
				std::cout << " not pow2";
				continue;
			}
			if(image->s() > 512)
			{
				std::cout << " big";
				//continue;
			}
			c.AddImage(image.get());
		}
	}
	std::cout << "\n";
	for(int i = 0;;i++)
	{
		osg::ref_ptr<osgTextureAtlas> atlas = c.CreateAtlas();
		if(!atlas.valid()) break;

		char buf[256];
		snprintf(buf, 255, "%s%d", argv[1], i);
		atlas->WriteAtlasFile(buf);
	}
	return 0;
}
