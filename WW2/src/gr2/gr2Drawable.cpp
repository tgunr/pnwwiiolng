#include <syscfg.h> // Path is project specific.
#include "gr2/gr2Drawable.h"
#include "gr2/gr2Layer.h"

#include <osg/Material>

/************************************************************************************************
	gr2Drawable
*************************************************************************************************/

#define RAD_TO_DEG(x) ((x) * 57.29578f)

gr2Drawable::gr2Drawable(gr2Model* m) : _model(m)
{
	setSupportsDisplayList(false);
}

gr2Drawable::~gr2Drawable()
{
}

void gr2Drawable::drawImplementation(osg::RenderInfo& ri) const
{
	if(!_model) return;

	ri.getState()->disableAllVertexArrays();
	ri.getState()->setActiveTextureUnit(0);
	ri.getState()->setClientActiveTextureUnit(0);
	/*
    ri.getState()->disableTexCoordPointersAboveAndIncluding(0);
    ri.getState()->disableColorPointer();
    ri.getState()->disableFogCoordPointer();
    ri.getState()->setActiveTextureUnit(0);
    ri.getState()->setClientActiveTextureUnit(0);
	*/

	bool rotate = (bool)_rotation.length2();

	if(rotate)
		glRotatef((int)_rotation.w(), (int)_rotation.x(), (int)_rotation.y(), (int)_rotation.z());

	//glRotatef(180, 0, 0, 1);
	gr2RenderModelOSG(_model, *ri.getState());
	//glRotatef(180, 0, 0, 1);

	if(rotate)
		glRotatef((int)_rotation.w(), (int)_rotation.x(), (int)_rotation.y(), (int)_rotation.z());

    ri.getState()->disableTexCoordPointersAboveAndIncluding(0);
    ri.getState()->setActiveTextureUnit(0);
    ri.getState()->setClientActiveTextureUnit(0);
}

osg::BoundingBox gr2Drawable::computeBound() const
{
	return osg::BoundingBox(osg::Vec3(-1,-1,0),osg::Vec3(1,1,1.7));
}
