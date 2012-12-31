#pragma once

#include <osg/PositionAttitudeTransform>

struct gr2Model;
class gr2Drawable;

/************************************************************************************************
	gr2ModelNode
*************************************************************************************************/

class gr2ModelNode : public osg::PositionAttitudeTransform
{
public:
	gr2ModelNode(gr2Model* m = 0);
	~gr2ModelNode();

	osg::Node* GetModel() { return _geode.get(); }
	void SetModel(gr2Model*);
	void SetRotation(osg::Vec4d&);

	virtual void Update(osg::NodeVisitor*, osg::NodeCallback*);

protected:
	gr2Drawable* _drawable;
	osg::ref_ptr<osg::Node> _geode;
};
