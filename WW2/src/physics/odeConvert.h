#pragma once

#include "math/Quat.h"

#include <ode/ode.h>

extern class Matrixf& OdeToMatrix(dReal const* pos, dReal const* rot, class Matrixf&);
extern class Matrixd& OdeToMatrix(dReal const* pos, dReal const* rot, class Matrixd&);

extern class Matrixf& dMatrixToMatrix(dReal const*, class Matrixf&);
extern class Matrixd& dMatrixToMatrix(dReal const*, class Matrixd&);
extern void MatrixTodMatrix(class Matrixf const&, dReal*);
extern void MatrixTodMatrix(class Matrixd const&, dReal*);

extern class Vec3d Vec3Ode(dReal const*);
extern dReal* OdeVec3f(dVector3, class Vec3f const&);
extern dReal* OdeVec3d(dVector3, class Vec3d const&);

extern Quat QuatOde(dReal const*);
extern dReal* OdeQuat(dQuaternion, class Quat const&);

extern bool osgGeomRayGet(dGeomID, class Vec3d& start, class Vec3d& end);
extern dGeomID osgGeomCreateRay(dSpaceID, class Vec3d const& start, class Vec3d const& end);

extern void geomGetRotationMatrix(dGeomID geom, class Matrixf& mat);
extern void geomGetRotationMatrix(dGeomID geom, class Matrixd& mat);
extern void geomGetRotationTransMatrix(dGeomID geom, class Matrixf& mat);
extern void geomGetRotationTransMatrix(dGeomID geom, class Matrixd& mat);
