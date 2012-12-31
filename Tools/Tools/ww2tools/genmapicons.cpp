#include <string>
#include <vector>
#include <iostream>

#include <osg/Node>
#include <osg/Group>
#include <osg/LOD>
#include <osg/Projection>
#include <osgDB/readFile>
#include <osgDB/writeFile>
#include <osg/Texture>
#include <osg/Geode>
#include <osg/MatrixTransform>

#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/filesystem/convenience.hpp>

class IconScreenCaptureCallback : public osg::NodeCallback
{
public: 
	IconScreenCaptureCallback() : SetModelViewAndProjectionMatrix(0), mpp(1), wantedmpp(1), dirit(0), screenwidth(0), screenheight(0), done(true) {}
	
	void SetDimensions(int width, int height) { screenwidth = width; screenheight = height; }
	void SetDirectory(std::string const& path) { terraindir = path; }
	void SetMetersPerPixel(double x) { wantedmpp = mpp = x; }
	void StartCapture() { done = false; }
	void (*SetModelViewAndProjectionMatrix)(osg::Matrixd const& modelview, osg::Matrix const& proj);
	
	virtual void operator()(osg::Node* node, osg::NodeVisitor* nv)
	{
		Capture(node->asGroup());
		traverse(node,nv);
	}
	
	bool Done() const { return done; }
	
protected:
	void Capture(osg::Group*);
	void GetModelDimensions(osg::Node* node, float* minx, float* miny, float* maxx, float* maxy);
	void SaveImage( unsigned char* data, int width, int height, const char* filename);
	bool ProcessModel(osg::Node* node);
	void GetTextureDimensions(float width, float height, float mpp, int& texwidth, int &texheight, float& x, float& y);


	boost::filesystem::path terraindir, savepath;
	double mpp, wantedmpp;
	boost::filesystem::directory_iterator* dirit;
	osg::ref_ptr<osg::Node> curmodel;
	int screenwidth, screenheight;
	bool done;
};

void IconScreenCaptureCallback::GetTextureDimensions(float width, float height, float mpp, int& texwidth, int &texheight, float& x, float& y)
{
	texwidth = (int)(width/mpp);
	texheight = (int)(height/mpp);
	
// nearest power of 2
	for(int i = 0;; i++)
	{
		if(texwidth > (1 << i) && texwidth <= (1 << (i+1)))
		{
			x -= (((1 << (i+1)) - texwidth)/2) * mpp;
			texwidth = (1 << (i+1));
			break;
		}
	}

	for(int i = 0;; i++)
	{
		if(texheight > (1 << i) && texheight <= (1 << (i+1)))
		{
			y -= (((1 << (i+1)) - texheight)/2) * mpp;
			texheight = (1 << (i+1));
			break;
		}
	}
}

void IconScreenCaptureCallback::Capture(osg::Group* parent)
{
	if(done) return;
	
	using namespace boost::filesystem;
	
	float x, y, width, height;
	float minx, miny, maxx, maxy;
	int texwidth, texheight;

	glColor3f(1,0,1);
	
	if(dirit == 0)
		dirit = new directory_iterator(terraindir);

	if(curmodel.valid())
	{   
		minx = miny = 10000; maxx = maxy = -10000;
		GetModelDimensions(curmodel.get(), &minx, &miny, &maxx, &maxy);
		x = minx; y = miny;
		width = maxx - minx; height = maxy - miny;
			
		GetTextureDimensions(width, height, mpp, texwidth, texheight, x, y);
		
		if(texwidth > 4 && texheight > 4)
		{
			unsigned char* data = new unsigned char[texwidth*texheight*3];
			float sx = screenwidth/2 - texwidth/2;
			float sy = screenheight/2 - texheight/2;
			glReadBuffer(GL_FRONT);
			glReadPixels((GLint)sx,(GLint)sy,(GLsizei)texwidth,(GLsizei)texheight, GL_RGB, GL_UNSIGNED_BYTE, data);
			
			std::string path = savepath.string();
			path += ".bmp";
			SaveImage(data,(int)texwidth,(int)texheight, path.c_str());
			delete [] data;

			path = savepath.string();
			path += ".xy";
			FILE* f = fopen(path.c_str(), "wb");
			fprintf(f, "%d %d %f", (int)x, (int)y, mpp);
			fclose(f);
		}
		parent->removeChild(curmodel.get());
	}
	
	if(!curmodel.valid() || (curmodel.valid() && dirit))
	{
		curmodel = 0;		
		
		directory_iterator endit;
		for(;dirit && *dirit != endit; ++(*dirit))
		{
			std::string ext = extension(**dirit);
			if(ext != ".flt") continue;
			
			osg::Node* node = osgDB::readNodeFile((*dirit)->string());
			
			if(node)
			{							
				ProcessModel(node);
				
				minx = miny = 10000; maxx = maxy = -10000;
				GetModelDimensions(node, &minx, &miny, &maxx, &maxy);
				x = minx; y = miny;
				width = maxx - minx; height = maxy - miny;
				
				mpp = wantedmpp;
				
				GetTextureDimensions(width, height, mpp, texwidth, texheight, x, y);
				if(texheight > 128 || texwidth > 128)
				{
					if(height > width)
						mpp = wantedmpp * (texheight / 128);
					else
						mpp = wantedmpp * (texwidth / 128);
				}
		
				osg::MatrixTransform* transform = new osg::MatrixTransform(osg::Matrix::translate(-(minx+maxx)/2,-(miny+maxy)/2,0));
				transform->addChild(node);
				
				curmodel = transform;
				curmodel->getOrCreateStateSet()->setMode(GL_LIGHTING,osg::StateAttribute::OVERRIDE|osg::StateAttribute::OFF);

				parent->addChild(curmodel.get());
				savepath = terraindir.string() + "/icons/" + basename(**dirit) + "_icon";
			
				SetModelViewAndProjectionMatrix(osg::Matrix::translate(0,0,-100)
					, osg::Matrix::ortho(-(int)screenwidth/2*mpp,screenwidth/2*mpp,-(int)screenheight/2*mpp,screenheight/2*mpp,-10000,10000));
			}
			++(*dirit);
			break;
		}
	}else
	{
		curmodel = 0;
	}

	if(!curmodel.valid())
	{
		delete dirit;
		dirit = 0;
		done = true;
	}
}

void IconScreenCaptureCallback::GetModelDimensions(osg::Node* node, float* minx, float* miny, float* maxx, float* maxy)
{
	osg::Geode* geode = dynamic_cast<osg::Geode*>(node);
	if(geode)
	{

		int count = geode->getNumDrawables();
		for(int i = 0; i < count;  i++)
		{
			osg::Drawable* drawable = geode->getDrawable(i);
			osg::BoundingBox bbox = drawable->getBound();
			
			if(bbox._min.x() < *minx) *minx = bbox._min.x();
			if(bbox._min.y() < *miny) *miny = bbox._min.y();
			if(bbox._max.x() < *minx) *minx = bbox._max.x();
			if(bbox._max.y() < *miny) *miny = bbox._max.y();

			if(bbox._min.x() > *maxx) *maxx = bbox._min.x();
			if(bbox._min.y() > *maxy) *maxy = bbox._min.y();
			if(bbox._max.x() > *maxx) *maxx = bbox._max.x();
			if(bbox._max.y() > *maxy) *maxy = bbox._max.y();
		}
	}
	
	osg::Group* group = node->asGroup();
	if(group)
	{
		int count = group->getNumChildren();
		
		for(int i = 0; i < count; i++)
		{
			GetModelDimensions(group->getChild(i), minx, miny, maxx, maxy);
		}
	}
}

bool IconScreenCaptureCallback::ProcessModel(osg::Node* node)
{
//	printf("process_model: %s\n", node->getName().c_str());
	
	const osg::Node::DescriptionList& comments = node->getDescriptions();
	int count = comments.size();
	for(int i = 0; i < count; i++)
	{
//		printf("comment: %s\n", comments[i].c_str());
		if(comments[i].find("damagelevel") != std::string::npos)
		{
//			printf("removed\n");
			return true;
		}
	}
	osg::Group* group = node->asGroup();
	if(group)
	{
		for(int i = 0; i < (int)group->getNumChildren();)
		{
			if(ProcessModel(group->getChild(i)))
				group->removeChild(group->getChild(i));
			else
				i++;
		}
	}
	
	return false;
}

void IconScreenCaptureCallback::SaveImage( unsigned char* data, int width, int height, const char* filename)
{
	osg::Image* image = new osg::Image;
	image->setImage(width,height,1,0,GL_RGB, GL_UNSIGNED_BYTE, data, osg::Image::NO_DELETE);
	osgDB::writeImageFile(*image, filename);
}

#include <osgProducer/Viewer>
#include <osgUtil/SceneView>
#include <osg/Projection>

osgProducer::Viewer* viewer;
osg::Projection* projnode;

void SetModelViewAndProjectionMatrix(osg::Matrixd const& modelview, osg::Matrix const& proj)
{
	viewer->setView(modelview);
	projnode->setMatrix(proj);
}

int main( int argc, char **argv )
{
	osg::ArgumentParser arguments(&argc,argv);

 	osgDB::ReaderWriter::Options* options = new osgDB::ReaderWriter::Options;
	options->setOptionString("dds_flip");
	osgDB::Registry::instance()->setOptions(options);

	if(arguments.argc() < 2 || arguments.find("-h"))
	{
		std::cout << "Usage: genmapicons [-d basedir] [-mpp meters per pixel] dir1 dir2 ...\n";
		return 0;
	}
	
	viewer = new osgProducer::Viewer(arguments);
    viewer->setUpViewer(osgProducer::Viewer::STANDARD_SETTINGS);
	viewer->realize();
	viewer->setClearColor(osg::Vec4(1,0,1,1));

	projnode = new osg::Projection;
	viewer->setSceneData(projnode);
	
	Producer::RenderSurface* surface = viewer->getCameraConfig()->getRenderSurface(0);		
	int x = 0,y = 0;
	unsigned int width=600,height=600;
	surface->setWindowRectangle(x,y,width,height);
	surface->fullScreen(false);

	IconScreenCaptureCallback* screencapture = new IconScreenCaptureCallback;
	screencapture->SetDimensions(width, height);
	screencapture->SetModelViewAndProjectionMatrix = SetModelViewAndProjectionMatrix;	

	osg::MatrixTransform* modelview = new osg::MatrixTransform;
	projnode->addChild(modelview);
	
	osg::Matrix flip;
	flip(0,0) = flip(1,1) = flip(2,2) = -1;
	modelview->setMatrix(flip*osg::Matrix::rotate(osg::PI,0,0,1));
	modelview->setUpdateCallback(screencapture);
	
	std::string basedir = "data/terrain/";
    while(!viewer->done())
    {
        viewer->sync();
        viewer->update();
        viewer->frame();        
		
		if(screencapture->Done())
		{
			if(arguments.argc() < 2) break;
			
			if (arguments.isOption(1))
			{
				std::string option = arguments[1];
				std::string val;
				arguments.read(option, val);
				if(option == "-d")
					basedir = val;
				else if(option == "-mpp")
					screencapture->SetMetersPerPixel(atof(val.c_str()));	
			}else
			{
				std::string path = basedir + arguments[1];
				screencapture->SetDirectory(path);
				screencapture->StartCapture();
				arguments.remove(1,1);
			}
		}
    }

    viewer->sync();
    viewer->cleanup_frame();
    viewer->sync();

    return 0;
}