#pragma once

#include <osg/Drawable>
#include <osg/Quat>

/************************************************************************************************
	gr2Drawable
*************************************************************************************************/
// wrapper around gr2 stuff, quick and dirty way to slap in granny

struct gr2Model;

class gr2Drawable : public osg::Drawable
{
public:
	gr2Drawable(gr2Model* m = NULL);
	gr2Drawable(const gr2Drawable& drawable,const osg::CopyOp& copyop=osg::CopyOp::SHALLOW_COPY) : _model(0) {}
	~gr2Drawable();

	virtual osg::Object* cloneType() const { return new gr2Drawable(0,false); }
	virtual osg::Object* clone( const osg::CopyOp& copyop ) const { return new gr2Drawable(*this, copyop); }
	virtual osg::BoundingBox computeBound() const;
	virtual void drawImplementation(osg::RenderInfo& ri) const;

	// set which model to draw
	void setRotation(osg::Vec4d& r) { _rotation.set(r.x(), r.y(), r.z(), r.w()); }
	void setModel(gr2Model* m) { _model = m; }
	gr2Model* getModel() { return _model; }

protected:

	gr2Model* _model; // model to draw
	osg::Vec4d _rotation; // arbitrary rotation to apply
};
