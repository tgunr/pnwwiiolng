#ifndef BoundingBoxVisitor_H
#define BoundingBoxVisitor_H

#include <osg/NodeVisitor>
#include <osg/Geode>

/************************************************************************************************
	BoundingBoxVisitor
*************************************************************************************************/

class BoundingBoxVisitor : public osg::NodeVisitor
{
public:
	BoundingBoxVisitor() : osg::NodeVisitor(osg::NodeVisitor::TRAVERSE_ALL_CHILDREN) {}
	
	osg::BoundingBox const& getBoundingBox(osg::Node& node){ _bbox.init(); apply(node); return _bbox; }
	
	virtual void apply(osg::Node& node) { traverse(node); }
	virtual void apply(osg::Geode& geode) { _bbox.expandBy(geode.getBoundingBox()); }

protected:
	osg::BoundingBox _bbox;
};

#endif // BoundingBoxVisitor_H