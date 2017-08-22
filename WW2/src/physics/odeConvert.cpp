#include "syscfg.h" // Path is project specific.
#include "physics/odeConvert.h"
#include "math/Matrixd.h"
#include "math/Matrixf.h"

bool osgGeomRayGet(dGeomID geom, Vec3d& start, Vec3d& end)
{
	dGeomID ray;

	int geomClass = dGeomGetClass(geom);

	if(geomClass == dGeomTransformClass)
	{
		ray = dGeomTransformGetGeom(geom);
	}
	else if(geomClass == dRayClass)
	{
		ray = geom;
	}
	else
	{
		return false;
	}

	double len = dGeomRayGetLength(ray);
	dVector3 dStart, dDir;
	dGeomRayGet (ray, dStart, dDir);

	start.x() = dStart[0];
	start.y() = dStart[1];
	start.z() = dStart[2];

	end.x() = dStart[0] + len * dDir[0];
	end.y() = dStart[1] + len * dDir[1];
	end.z() = dStart[2] + len * dDir[2];

	if(geomClass == dGeomTransformClass)
	{
		const dReal* pos = dGeomGetPosition(geom);
		const dReal* rot = dGeomGetRotation(geom);

		Matrixd mat;
		dMatrixToMatrix(rot, mat);
		mat(3,0) = pos[0]; mat(3,1) = pos[1]; mat(3,2) = pos[2];

		start = start * mat;
		end = end * mat;
	}

	return true;
}

void MatrixTodMatrix(Matrixf const& mat, dReal* dMat)
{
	dMat[0] = mat(0,0);
	dMat[1] = mat(1,0);
	dMat[2] = mat(2,0);
	dMat[4] = mat(0,1);
	dMat[5] = mat(1,1);
	dMat[6] = mat(2,1);
	dMat[8] = mat(0,2);
	dMat[9] = mat(1,2);
	dMat[10]= mat(2,2);
}

void MatrixTodMatrix(Matrixd const& mat, dReal* dMat)
{
	dMat[0] = mat(0,0);
	dMat[1] = mat(1,0);
	dMat[2] = mat(2,0);
	dMat[4] = mat(0,1);
	dMat[5] = mat(1,1);
	dMat[6] = mat(2,1);
	dMat[8] = mat(0,2);
	dMat[9] = mat(1,2);
	dMat[10]= mat(2,2);
}

Matrixf& OdeToMatrix(dReal const* pos, dReal const* rot, Matrixf& mat)
{
	dMatrixToMatrix(rot, mat);
	mat(3,0) = (float)pos[0]; mat(3,1) = (float)pos[1]; mat(3,2) = (float)pos[2];
	return mat;
}

Matrixd& OdeToMatrix(dReal const* pos, dReal const* rot, Matrixd& mat)
{
	dMatrixToMatrix(rot, mat);
	mat(3,0) = pos[0]; mat(3,1) = pos[1]; mat(3,2) = pos[2];
	return mat;
}

Matrixf& dMatrixToMatrix(dReal const* dMat, Matrixf& mat)
{
	mat(0,0) = (float)dMat[0];
	mat(1,0) = (float)dMat[1];
	mat(2,0) = (float)dMat[2];
	mat(0,1) = (float)dMat[4];
	mat(1,1) = (float)dMat[5];
	mat(2,1) = (float)dMat[6];
	mat(0,2) = (float)dMat[8];
	mat(1,2) = (float)dMat[9];
	mat(2,2) = (float)dMat[10];

	return mat;
}

Matrixd& dMatrixToMatrix(dReal const* dMat, Matrixd& mat)
{
	mat(0,0) = dMat[0];
	mat(1,0) = dMat[1];
	mat(2,0) = dMat[2];
	mat(0,1) = dMat[4];
	mat(1,1) = dMat[5];
	mat(2,1) = dMat[6];
	mat(0,2) = dMat[8];
	mat(1,2) = dMat[9];
	mat(2,2) = dMat[10];

	return mat;
}


Vec3d Vec3Ode(dReal const* v) { return Vec3d(v[0],v[1],v[2]); }
dReal* OdeVec3(dVector3 dv, Vec3f const& v) { dv[0] = v.x(); dv[1] = v.y(); dv[2] = v.z(); return dv;}
dReal* OdeVec3d(dVector3 dv, Vec3d const& v) { dv[0] = v.x(); dv[1] = v.y(); dv[2] = v.z(); return dv;}

Quat QuatOde(dReal const* q) { return Quat(q[1],q[2],q[3],q[0]); }
dReal* OdeQuat(dQuaternion dq, Quat const& q) { dq[1] = q.x(); dq[2] = q.y(); dq[3] = q.z(); dq[0] = q.w(); return dq; }

dGeomID osgGeomCreateRay(dSpaceID space, Vec3d const& start, Vec3d const& end)
{
	Vec3d dir = end-start;
	dGeomID ray = dCreateRay(0,dir.length());

	dGeomID geom = dCreateGeomTransform(space);
	dGeomTransformSetGeom(geom, ray);
	dGeomTransformSetCleanup(geom, 1);

	dGeomRaySet(ray, start.x(),start.y(),start.z(), dir.x(),dir.y(),dir.z());

	return geom;
}

void geomGetRotationMatrix(dGeomID geom, Matrixf& mat)
{
    const dReal* rot = dGeomGetRotation(geom);

    dMatrixToMatrix(rot, mat);

    mat(3, 0) = 0;
    mat(3, 1) = 0;
    mat(3, 2) = 0;
    mat(3, 3) = 1;
}

void geomGetRotationMatrix(dGeomID geom, Matrixd& mat)
{
    const dReal* rot = dGeomGetRotation(geom);

    dMatrixToMatrix(rot, mat);

    mat(3, 0) = 0;
    mat(3, 1) = 0;
    mat(3, 2) = 0;
    mat(3, 3) = 1;
}

void geomGetRotationTransMatrix(dGeomID geom, Matrixf& mat)
{
    const dReal* rot = dGeomGetRotation(geom);
    const dReal* pos = dGeomGetPosition(geom);

    OdeToMatrix(pos, rot, mat);
}

void geomGetRotationTransMatrix(dGeomID geom, Matrixd& mat)
{
    const dReal* rot = dGeomGetRotation(geom);
    const dReal* pos = dGeomGetPosition(geom);

    OdeToMatrix(pos, rot, mat);
}
