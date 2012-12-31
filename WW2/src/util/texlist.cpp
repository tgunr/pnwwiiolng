#include <syscfg.h> // Path is project specific.
#include <string>
#include <set>
#include <iostream>

#include <osg/Node>
#include <osg/Texture>
#include <osg/Geometry>
#include <osgDB/readFile>

class ImageFinder : public osg::NodeVisitor
{
public:
	ImageFinder() : osg::NodeVisitor(osg::NodeVisitor::TRAVERSE_ALL_CHILDREN){}

	virtual void apply(osg::Node& node) { traverse(node); }
	virtual void apply(osg::Geode& node);

	std::set<std::string> names;
};

int main(int argc, char** argv)
{
	ImageFinder imf;
	osgDB::FilePathList& datapathlist = osgDB::Registry::instance()->getDataFilePathList();

	datapathlist.push_back("terrain");
	datapathlist.push_back("terrain/to");
	datapathlist.push_back("terrain/bld");
	datapathlist.push_back("terrain/brdg");
	datapathlist.push_back("terrain/ag");
	datapathlist.push_back("terrain/cap");
	datapathlist.push_back("terrain/fac");
	datapathlist.push_back("vehicles");
	datapathlist.push_back("vehicles/vo");

	for(int i = 1; i < argc; i++)
	{
		std::string name = argv[i];
		osg::ref_ptr<osg::Node> node = osgDB::readNodeFile(name);
		if(!node.valid()) continue;
		imf.apply(*node);
	}

	std::set<std::string>::iterator it = imf.names.begin();
	for(; it != imf.names.end(); ++it)
	{
		std::string str = *it;
		for(unsigned int i = 0; i < str.size(); i++)
		{
			char& c= str[i];
			if(c == '\\') c = '/';
			c = tolower(c);
		}
		std::cout << str << "\n";
	}
	return 0;
}

void ImageFinder::apply(osg::Geode& geode)
{
	unsigned int n = geode.getNumDrawables();
	for(unsigned int i = 0; i < n; i++)
	{
		osg::Geometry* geom = dynamic_cast<osg::Geometry*>(geode.getDrawable(i));
		if(geom)
		{
			osg::StateSet* stateset = geom->getStateSet();
			if(stateset)
			{
				osg::Texture* texture = dynamic_cast<osg::Texture*>(stateset->getTextureAttribute(0, osg::StateAttribute::TEXTURE));
				if(texture)
				{
					osg::Image* image = texture->getImage(0);
					if(image)
					{
						names.insert(image->getFileName());
					}
				}
			}
		}
	}
	traverse(geode);
}