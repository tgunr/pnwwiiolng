#include <map>
#include <set>
#include <iostream>

#include <osg/Texture>
#include <osgDB/readFile>

class CollectTextureStats : public osg::NodeVisitor
{
public:
	CollectTextureStats() : osg::NodeVisitor(osg::NodeVisitor::TRAVERSE_ALL_CHILDREN) {}
	
	void collect(osg::Node* node)
	{
		nodeName="";
		apply(*node);
	}
	
	void apply(osg::Node& node) 
	{ 	
	
		if(nodeName.empty() && node.getName()!="db") 
			nodeName = node.getName();
		if(node.getStateSet())  
			apply(*node.getStateSet());
		
		traverse(node); 
	}
	void apply(osg::Geode& geode)
	{
		unsigned int n = geode.getNumDrawables();
		for(unsigned int i = 0; i < n; i++)
			apply(*geode.getDrawable(i));
			
		apply((osg::Node&)geode);
	}
	
	void apply(osg::Drawable& drawable)
	{
		if(drawable.getStateSet())  
			apply(*drawable.getStateSet());

	}
	
	void apply(osg::StateSet& ss)
	{
		osg::Texture* texture = dynamic_cast<osg::Texture*>(ss.getTextureAttribute(0, osg::StateAttribute::TEXTURE));
		if(texture)
		{
			osg::Image* image = texture->getImage(0);

			if(image)
			{
				texToFLT[image->getFileName()].insert(nodeName);
				FLTToTex[nodeName].insert(image->getFileName());
			}
		}
	}
	
	void print()
	{
		print(texToFLT);
		print(FLTToTex);
	}
	
	typedef std::map<std::string, std::set<std::string> > TextureFLTMap;

	void print(TextureFLTMap& fltmap)
	{
		TextureFLTMap::iterator it = fltmap.begin();
		for(;it != fltmap.end(); ++it)
		{
			std::set<std::string>& flttex = it->second;
			std::set<std::string>::iterator it2 = flttex.begin();
			
			std::cout << it->first << ": ";
			for(; it2 != flttex.end(); ++it2)
			{
				std::cout << *it2 << " ";
			}
			
			std::cout << std::endl;
		}
	}
	
protected:
	std::string nodeName;
	TextureFLTMap texToFLT;
	TextureFLTMap FLTToTex;

};


int main( int argc, char **argv )
{
	CollectTextureStats stats;
	
	for(int i = 1; i < argc; i++)
	{
		osg::ref_ptr<osg::Node> node = osgDB::readNodeFile(argv[i]);
		stats.collect(node.get());
	}
	
	stats.print();
	
	return 0;
}