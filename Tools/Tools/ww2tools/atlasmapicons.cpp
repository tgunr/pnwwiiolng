#include <sstream>
#include <map>
#include <vector>
#include <set>
#include <stdio.h>

#include <osg/Image>
#include <osgDB/readFile>
#include <osgDB/writeFile>
#include <osgUtil/Optimizer>
#include <osgDB/FileNameUtils>

static osg::Image* ApplyAlphaBMP(osg::Image* image);

typedef std::set<osg::Image*> ImageSet;
ImageSet _images;

struct AtlasEntry
{
	AtlasEntry() : x(0), y(0), mpp(1) { uv[0]=uv[1]=uv[2]=uv[3]=0; }
	std::string path;
	float uv[4];
	int x,y;
	float mpp;
};

typedef std::map<osg::Image*,std::vector<AtlasEntry> > AtlasMap;
AtlasMap _atlas;

int main(int argc, char** argv)
{
	osgDB::ReaderWriter::Options* options = new osgDB::ReaderWriter::Options;
	options->setOptionString("dds_flip");
	osgDB::Registry::instance()->setOptions(options);

	int maxwidth = 0;
	int maxheight = 0;
	int margin = 0;
	osg::Group* group = new osg::Group;
	for(int i = 1; i < argc; i++)
	{
		if(argv[i][0] == '-')
		{
			if(maxwidth == 0)
				maxwidth = atoi(argv[i]+1);
			else if(maxheight == 0)
				maxheight = atoi(argv[i]+1);
			else
				margin = atoi(argv[i]+1);
			continue;
		}
		
		std::string name = argv[i];
		
		osg::ref_ptr<osg::Image> img = osgDB::readImageFile(name);
		osg::Image* image = ApplyAlphaBMP(img.get());
		image->setFileName(name);
		_images.insert(image);
		
		group->addChild(osg::createGeodeForImage(image));
	}
	
	if(maxwidth == 0) maxwidth = 2048;
	if(maxheight == 0) maxheight = maxwidth;
	if(margin == 0) margin = 8;
	
	osgUtil::Optimizer::TextureAtlasVisitor av;
	osgUtil::Optimizer::TextureAtlasBuilder& atlas = av.getTextureAtlasBuilder();
	atlas.setMaximumAtlasSize(maxwidth,maxheight);
	atlas.setMargin(margin);
	av.apply(*group);
	av.optimize();
	
	bool hasxy = false;
	
	{
		ImageSet::iterator it = _images.begin();
		for(; it != _images.end(); ++it)
		{
			osg::Image* srcImage = *it;
			osg::Image* image = atlas.getImageAtlas(srcImage);
			if(!image) break;
			
			AtlasEntry entry;
			entry.path = osgDB::getStrippedName(srcImage->getFileName());
			
			osg::Matrix mat = atlas.getTextureMatrix(srcImage);
			entry.uv[0] = mat(3,0); 
			entry.uv[1] = mat(3,1);
			entry.uv[2] = mat(3,0) + mat(0,0);
			entry.uv[3] = mat(3,1) + mat(1,1);
			
			std::string xypath = osgDB::getNameLessExtension(srcImage->getFileName());
			xypath += ".xy";

			FILE* f = fopen(xypath.c_str(), "rb");
			if(f)
			{
				hasxy = true;
				fscanf(f, "%d %d %f", &entry.x, &entry.y, &entry.mpp);
				fclose(f);
			}

			_atlas[image].push_back(entry);
		}
	}
	{
		AtlasMap::iterator it = _atlas.begin();
		int i=0;
		for(; it != _atlas.end(); ++it,i++)
		{
			osg::Image* image = it->first;
			image->flipVertical(); // will eventually output to dds so flip now
			{
				std::stringstream ss;
				ss << "atlas" << i << ".png"; // png because dds writer doesn't seem to work, we use another app to create the dds
				osgDB::writeImageFile(*image, ss.str());
			}
			FILE* f;
			{
				std::stringstream ss;
				ss << "atlas" << i << ".atlas";
				f = fopen(ss.str().c_str(), "wb");
			}
			std::vector<AtlasEntry>& entries = it->second;
			std::vector<AtlasEntry>::iterator eit = entries.begin();

			for(; eit != entries.end(); ++eit)
			{
				AtlasEntry& entry = *eit;
				fprintf(f, "%s %f %f %f %f\n", entry.path.c_str()
					, entry.uv[0], entry.uv[1], entry.uv[2], entry.uv[3]
				);
			}
			fclose(f);

			if(hasxy)
			{
				{
					std::stringstream ss;
					ss << "atlas" << i << ".xy";
					f = fopen(ss.str().c_str(), "wb");
				}
				
				eit = entries.begin();
				for(; eit != entries.end(); ++eit)
				{
					AtlasEntry& entry = *eit;
					fprintf(f, "%s %d %d %f\n", entry.path.c_str()
						, entry.x, entry.y, entry.mpp
					);
				}
				fclose(f);
			}
		}
	}
	return 0;
}

static osg::Image* ApplyAlphaBMP(osg::Image* image)
{	
	if(image->getPixelFormat() != GL_RGB) return image;

	std::string const& filename = image->getFileName();
	if(osgDB::getFileExtension(filename) != "bmp") return image;
		
	std::string alphapath = osgDB::getNameLessExtension(filename);
	alphapath += "_alpha.bmp";
	
	osg::ref_ptr<osg::Image> alphaimage = osgDB::readImageFile(alphapath);
	
	osg::Image* outimage = new osg::Image;
	outimage->setFileName(image->getFileName());
	outimage->allocateImage(image->s(), image->t(), 1, GL_RGBA, GL_UNSIGNED_BYTE);

	for(int row = 0; row < image->t(); row++)
	{	
		unsigned char* in = image->data(0,row);
		unsigned char* alpha = alphaimage.valid() ? alphaimage->data(0,row) : 0;
		unsigned char* out = outimage->data(0,row);

		for(int col = 0; col < image->s(); col++)
		{
			unsigned char r = *in++;
			unsigned char g = *in++;
			unsigned char b = *in++;
			
			*out++ = r;
			*out++ = g;
			*out++ = b;
			
			if(alpha)
			{
				*out++ = *alpha;
				alpha += 3;
			}else
			{
				*out++ = (r == 0xff && g == 0x00 && b == 0xff) ? 0x0 : 0xff; // magneta serves as transparent color
			}
		}
	}

	return outimage;
}
