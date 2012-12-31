#include <algorithm>

#include <osg/Texture2D>
#include <osg/Geometry>
#include <osg/State>
#include <osg/ClusterCullingCallback>
#include <osg/CoordinateSystemNode>
#include <osg/MatrixTransform>

#include <osgDB/ReadFile>
#include <osgDB/WriteFile>
#include <osgDB/ImageOptions>
#include <osgDB/FileNameUtils>

#include <osgUtil/Optimizer>
#include <osgUtil/TriStripVisitor>

#include <osgProducer/Viewer>

#include "osgGDAL/Dataset.h"
#include "osgGDAL/Image.h"

static int numLevels=0;
static bool useCompressedTextures = true;
static bool use565 = true;
static int minlevel=3;
static int maxlevel=3;
static osg::EllipsoidModel* ellipsoid;

class GraphicsContext {
    public:
        GraphicsContext()
        {
            rs = new Producer::RenderSurface;
            rs->setWindowRectangle(0,0,1,1);
            rs->useBorder(false);
            rs->useConfigEventThread(false);
            rs->realize();
            std::cout<<"Realized window"<<std::endl;
        }

        virtual ~GraphicsContext()
        {
        }
        
    private:
        Producer::ref_ptr<Producer::RenderSurface> rs;
};

static bool createWorld(osgGDAL::Image* image, const std::string& baseName);
static osg::Node* createTileAndRecurse(osgGDAL::Image* image, const std::string& basename, const std::string& extension, 
										int reducelevel, int srcwin[4]);
static osg::Node* createTile(osgGDAL::Image* image, const std::string& basename, const std::string& extension, 
										int reducelevel, int srcwin[4]);

int main( int argc, char **argv )
{
   	GDALAllRegister();
 	osg::ArgumentParser arguments(&argc,argv);
	
	ellipsoid = new osg::EllipsoidModel(1,1);
	
    std::string marblefilename("bluemarble.unshaded.tif");
    while (arguments.read("-m", marblefilename)) {}

    std::string basename("bluemarble.ive");
    while (arguments.read("-o",basename)) {}

    while (arguments.read("-l",numLevels)) {}

    while (arguments.read("--compressed")) { useCompressedTextures = true; use565 = false; }
    while (arguments.read("--565")) { useCompressedTextures = false; use565 = true;}
    while (arguments.read("--RGB")) { useCompressedTextures = false; use565 = false; }

	minlevel = maxlevel-numLevels;
	
	GDALDataset* gdaldataset = (GDALDataset*)GDALOpen(marblefilename.c_str(), GA_ReadOnly );
	osgGDAL::Dataset* dataset = new osgGDAL::Dataset(gdaldataset);
	osgGDAL::Image* image = new osgGDAL::Image;
	image->SetDataset(dataset);
	image->SetBands(1,2,3);
	
    GraphicsContext gfx;
    createWorld(image, basename);

    return 0;
}

static bool createWorld(osgGDAL::Image* image, const std::string& baseName)
{

    std::string path = osgDB::getFilePath(baseName);
    std::string base = path.empty()?osgDB::getStrippedName(baseName):
                                    path +'/'+ osgDB::getStrippedName(baseName);
    std::string extension = '.'+osgDB::getLowerCaseFileExtension(baseName);
    	
    osg::ref_ptr<osg::Group> group = new osg::Group;
    
    int xsize = image->Dataset()->GetDataset()->GetRasterXSize();
    int ysize = image->Dataset()->GetDataset()->GetRasterYSize();
    int reducelevel = maxlevel;
    
    {
	    int srcwin[4] = {0,0,xsize/2,ysize};
		group->addChild(createTileAndRecurse(image, base+"_west", extension, reducelevel, srcwin));
	}
	{
		int srcwin[4] = {xsize/2,0,xsize,ysize};
		group->addChild(createTileAndRecurse(image, base+"_east", extension, reducelevel, srcwin));
   	}
    osg::StateSet* stateset = group->getOrCreateStateSet();
    stateset->setMode(GL_CULL_FACE,osg::StateAttribute::ON);
	
    osgDB::writeNodeFile(*group, baseName);
}

static osg::Node* createTileAndRecurse(osgGDAL::Image* image, const std::string& basename, const std::string& extension, 
										int reducelevel, int srcwin[4])
{
    osg::Group* group = new osg::Group;
	int width  = (srcwin[2] - srcwin[0])/2;
	int height = (srcwin[3] - srcwin[1])/2;
	int subwin[4];
         
    if (reducelevel>minlevel)
    {   
		float cut_off_distance = 2*osg::PI/(1<<(maxlevel-reducelevel+1));
        float max_visible_distance = 1e7*2;

        // create current layer, and write to disk.
        unsigned int numTiles = 0;

        for(unsigned i=0;i<2;++i)
        {
            for(unsigned j=0;j<2;++j)
            {
                // create name for tile.
                char char_num = 'A'+numTiles;
                std::string lbasename = basename+"_"+char_num;

            	subwin[0] = srcwin[0] + j*width;
            	subwin[1] = srcwin[1] + i*height;
            	subwin[2] = subwin[0] + width;
            	subwin[3] = subwin[1] + height; 

                // create the subtiles and write out to disk.
                {

                    osg::ref_ptr<osg::Node> node = createTileAndRecurse(image,lbasename,extension,reducelevel-1,subwin);
                    osgDB::writeNodeFile(*node, lbasename+extension);
                }
                
                // create Pagedg for tile.
                osg::PagedLOD* pagedlod = new osg::PagedLOD;

                osg::Node* tile = createTile(image,basename,extension,reducelevel,srcwin);
                pagedlod->addChild(tile, cut_off_distance,max_visible_distance);
                pagedlod->setRange(1,0.0f,cut_off_distance);
                pagedlod->setFileName(1,lbasename+extension);
           
				double lon,lat;
				int srcx = (subwin[0]+subwin[2])/2;
				int srcy = (subwin[1]+subwin[3])/2;
				image->Transform().PixelToLonLat(srcx,srcy, &lon,&lat);

				osg::Vec3d pos;
				ellipsoid->convertLatLongHeightToXYZ(osg::DegreesToRadians(lat), osg::DegreesToRadians(lon), 0, 
													pos.x(), pos.y(), pos.z());
				pagedlod->setCenter(pos);

                group->addChild(pagedlod);
                
                // increment number of tiles.
                ++numTiles;
            }
        }
    }
    else
    {
        for(unsigned i=0;i<2;++i)
        {
            for(unsigned j=0;j<2;++j)
            {
            	subwin[0] = srcwin[0] + j*width;
            	subwin[1] = srcwin[1] + i*height;
            	subwin[2] = subwin[0] + width;
            	subwin[3] = subwin[1] + height;              
            	group->addChild(createTile(image, basename, extension, reducelevel, subwin));
            }
        }
    }
    
    return group;   
}

static osg::Node* createTile(osgGDAL::Image* gdalimage, const std::string& basename, const std::string& extension, 
										int reducelevel, int srcwin[4])
{
printf("createTile: %d %d %d %d %d\n", srcwin[0],srcwin[1],srcwin[2],srcwin[3], reducelevel);

    osg::Geode* geode = new osg::Geode;
	osg::Image* image = gdalimage->CreateReducedImage(srcwin, reducelevel);
	osg::Geometry* geom = new osg::Geometry;


	osg::Texture2D* texture = new osg::Texture2D;
	texture->setImage(image);
    texture->setWrap(osg::Texture::WRAP_S,osg::Texture::CLAMP_TO_EDGE);
    texture->setWrap(osg::Texture::WRAP_T,osg::Texture::CLAMP_TO_EDGE);
    texture->setFilter(osg::Texture::MIN_FILTER,osg::Texture::LINEAR);
    texture->setFilter(osg::Texture::MAG_FILTER,osg::Texture::LINEAR);
    texture->setMaxAnisotropy(8);

 	osg::StateSet* stateset = new osg::StateSet;
	stateset->setTextureAttributeAndModes(0,texture,osg::StateAttribute::ON);
	image->scaleImage(1024,1024,1);
	if (useCompressedTextures)
	{
	    texture->setInternalFormatMode(osg::Texture::USE_S3TC_DXT3_COMPRESSION);
	    osg::ref_ptr<osg::State> state = new osg::State;
	    texture->apply(*state);
	    image->readImageFromCurrentTexture(0,true);
	    texture->setInternalFormatMode(osg::Texture::USE_IMAGE_DATA_FORMAT);
	} else if (use565)
	{
	    image->scaleImage(image->s(),image->t(),image->r(),GL_UNSIGNED_SHORT_5_6_5);
	}
  	
	int xsize=11, ysize=11;
	int width  = srcwin[2] - srcwin[0];
	int height = srcwin[3] - srcwin[1];
	
	int xinc = width  / (xsize-1);
	int yinc = height / (ysize-1);
	
	float uinc = 1.0 / (float)(xsize-1);
	float vinc = 1.0 / (float)(ysize-1);
	
	osg::Vec3Array* verts = new osg::Vec3Array(xsize*ysize);
	osg::Vec3Array* normals = new osg::Vec3Array(xsize*ysize);
	osg::Vec2Array* texcoords = new osg::Vec2Array(xsize*ysize);
	
	int i=0;
	for(int y = 0; y < ysize; y++)
	{
		int srcy = srcwin[1] + y*yinc;
		for(int x = 0; x < xsize; x++,i++)
		{
			double lon,lat;
			int srcx = srcwin[0] + x*xinc;
			gdalimage->Transform().PixelToLonLat(srcx,srcy, &lon,&lat);

			osg::Vec3d pos;
			ellipsoid->convertLatLongHeightToXYZ(osg::DegreesToRadians(lat), osg::DegreesToRadians(lon), 0, 
													pos.x(), pos.y(), pos.z());
			(*verts)[i] = pos;
			(*normals)[i] = pos;
			(*texcoords)[i] = osg::Vec2(uinc*x,vinc*y);
		}
	}
	
	for(int y = 0; y < ysize-1; y++)
	{
        osg::DrawElementsUShort& drawElements = *(new osg::DrawElementsUShort(GL_QUAD_STRIP,2*xsize));
        geom->addPrimitiveSet(&drawElements);
		int ei=0;
		for(int x = 0; x < xsize; x++)
		{
	    	drawElements[ei++] = (y)*xsize+x;
		    drawElements[ei++] = (y+1)*xsize+x;
		}
	}
	
	stateset->setMode(GL_CULL_FACE, osg::StateAttribute::ON);
	geode->setStateSet( stateset );

	geom->setNormalBinding(osg::Geometry::BIND_PER_VERTEX);	
	geom->setVertexArray(verts);
	geom->setNormalArray(normals);
	geom->setTexCoordArray(0,texcoords);
	geom->setCullCallback(new osg::ClusterCullingCallback(geom));
	geode->addDrawable(geom);
	  
	osgUtil::TriStripVisitor tsv;
    tsv.stripify(*geom);
 
	return geode;
}