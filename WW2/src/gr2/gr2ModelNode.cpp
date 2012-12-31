#include <syscfg.h> // Path is project specific.
#include "gr2/gr2ModelNode.h"
#include "gr2/gr2Layer.h"
#include "gr2/gr2Drawable.h"
#include "osg/NodeOps.h"
#include "osg/osgMain.h"

#include <osg/Geode>
#include <osg/Shape>
#include <osg/ShapeDrawable>
#include <osg/Projection>


/************************************************************************************************
	gr2ModelNode
*************************************************************************************************/

gr2ModelNode::gr2ModelNode(gr2Model* m)
{
	osg::Geode* geode = new osg::Geode;
	_drawable = new gr2Drawable(m);
	geode->addDrawable(_drawable);

	// save it
	_geode = geode;
	addChild(_geode.get());

	// we want to cull based on whether we're active or not
	//NodeOpAddCullCallback(this);
}

gr2ModelNode::~gr2ModelNode()
{
}

void gr2ModelNode::Update(osg::NodeVisitor*, osg::NodeCallback*)
{
	// update the node's position based on the model's root bone position
	// setPosition(_drawable->getModel()->getPosition());
}

void gr2ModelNode::SetModel(gr2Model* m)
{
	_drawable->setModel(m);
}

void gr2ModelNode::SetRotation(osg::Vec4d& r)
{
	_drawable->setRotation(r);
}