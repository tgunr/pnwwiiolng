#include <osg/Matrix>
#include <osg/Vec3d>
#include <osgDB/readFile>
#include <osg/Node>
#include <osg/TriangleFunctor>
#include <osg/Geode>
#include <osg/Geometry>

typedef struct
{
	double P1, Pa, Pb, Paa, Pab, Pbb, Paaa, Paab, Pabb, Pbbb;
} projectionIntegrals;

typedef struct
{
	double Fa, Fb, Fc, Faa, Fbb, Fcc, Faaa, Fbbb, Fccc, Faab, Fbbc, Fcca;
} faceIntegrals;

typedef struct
{
	double T0, T1[3], T2[3], TP[3];
} volumeIntegrals;

#define MAX_VERTS 1000     /* maximum number of polyhedral vertices */
#define MAX_FACES 1000     /* maximum number of polyhedral faces */
#define MAX_POLYGON_SZ 3 /* maximum number of verts per polygonal face */
#define X 0
#define Y 1
#define Z 2
#define SQR(x) ((x)*(x))
#define CUBE(x) ((x)*(x)*(x))

typedef struct 
{
	int numVerts;
	double norm[3];
	double w;
	int verts[MAX_POLYGON_SZ];
	struct polyhedron *poly;
} FACE;

typedef struct polyhedron 
{
	int numVerts, numFaces;
	double verts[MAX_VERTS][3];
	FACE faces[MAX_FACES];
} POLYHEDRON;

static void compVolumeIntegrals(volumeIntegrals* v, const POLYHEDRON *p);
static int CalcRotationalInertiaIndexedBody( double mass, int numverts, int numtris, const float* vertices, const int* indices, 
										osg::Matrix& I, osg::Vec3d& cm, double& vol);


struct odeMesh
{
	odeMesh()
	{
		verts = new osg::Vec3Array;
		indices = new osg::IntArray;
	}
	
	inline void operator () (const osg::Vec3& v1,const osg::Vec3& v2,const osg::Vec3& v3, bool treatVertexDataAsTemporary)
	{
		verts->push_back(v1);
		verts->push_back(v2);
		verts->push_back(v3);
		indices->push_back(indices->size());
		indices->push_back(indices->size());
		indices->push_back(indices->size());
	}
	
	osg::Vec3Array* verts;
	osg::IntArray* indices;
};

class dTriMeshDataVisitor : public osg::NodeVisitor
{
public:
	dTriMeshDataVisitor() : osg::NodeVisitor(osg::NodeVisitor::TRAVERSE_ALL_CHILDREN), mesh(0){}

	virtual void apply(osg::Node& node) { if(mesh == 0) traverse(node); }
	virtual void apply(osg::Geode& node)
	{ 
		if(node.getNumDrawables() < 1) return;
		
		osg::Geometry* geom = dynamic_cast<osg::Geometry*>(node.getDrawable(0));
		if(!geom) return;

		
		geom->accept(tf);
		
		mesh = 1;
	}
	
	int mesh;
	osg::TriangleFunctor<odeMesh> tf;
};
										
int main(int argc, char** argv)
{
	osg::Node* node = osgDB::readNodeFile(argv[1]);
	dTriMeshDataVisitor v;

	v.apply(*node);

	osg::Matrix I;
	osg::Vec3d cm;
	double vol;
	if(CalcRotationalInertiaIndexedBody(1,v.tf.verts->size(),v.tf.verts->size()/3,
		(const float*)v.tf.verts->getDataPointer(),(const int*)v.tf.indices->getDataPointer(), I, cm, vol) != 0)
	{
		printf("error!\n");
	}

	printf("CM: %lf %lf %lf\n", cm.x(), cm.y(), cm.z());
	printf("I: %lf %lf %lf %lf %lf %lf %lf %lf %lf\n", I(0,0),I(0,1),I(0,2),I(1,0),I(1,1),I(1,2),I(2,0),I(2,1),I(2,2));
	printf("vol: %lf\n", vol);

	return 0;
}

int CalcRotationalInertiaIndexedBody( double mass, int numverts, int numtris, const float* vertices, const int* indices, 
										osg::Matrix& I, osg::Vec3d& cm, double& vol)
{
	POLYHEDRON p;
	volumeIntegrals v;
	int i,k;
	const float* verts = vertices;
	double density;
	FACE *f;
	double dx1, dy1, dz1, dx2, dy2, dz2, nx, ny, nz, len;
	double r[3];
	
	if(numverts >= MAX_VERTS || numtris >= MAX_FACES) return -1;

// copy data into polyhedron structure 
	p.numVerts = numverts;
	p.numFaces = numtris;

	for(i = 0; i < numverts; i++)
	{
		p.verts[i][0] = *verts++;
		p.verts[i][1] = *verts++;
		p.verts[i][2] = *verts++;
	}
	
	for(k = 0, i = 0; i < numtris; i++)
	{
		f = &p.faces[i];

		f->numVerts = 3;
		f->verts[0] = indices[k++];
		f->verts[1] = indices[k++];
		f->verts[2] = indices[k++];
		f->poly = &p;
	
		/* compute face normal and offset w from first 3 vertices */
		dx1 = p.verts[f->verts[1]][X] - p.verts[f->verts[0]][X];
		dy1 = p.verts[f->verts[1]][Y] - p.verts[f->verts[0]][Y];
		dz1 = p.verts[f->verts[1]][Z] - p.verts[f->verts[0]][Z];
		dx2 = p.verts[f->verts[2]][X] - p.verts[f->verts[1]][X];
		dy2 = p.verts[f->verts[2]][Y] - p.verts[f->verts[1]][Y];
		dz2 = p.verts[f->verts[2]][Z] - p.verts[f->verts[1]][Z];
		nx = dy1 * dz2 - dy2 * dz1;
		ny = dz1 * dx2 - dz2 * dx1;
		nz = dx1 * dy2 - dx2 * dy1;
		len = sqrt(nx * nx + ny * ny + nz * nz);
		f->norm[X] = nx / len;
		f->norm[Y] = ny / len;
		f->norm[Z] = nz / len;
		f->w = - f->norm[X] * p.verts[f->verts[0]][X]
		       - f->norm[Y] * p.verts[f->verts[0]][Y]
		       - f->norm[Z] * p.verts[f->verts[0]][Z];
	}
	
	compVolumeIntegrals(&v, &p);

	r[X] = v.T1[X] / v.T0;
	r[Y] = v.T1[Y] / v.T0;
	r[Z] = v.T1[Z] / v.T0;
	
	cm.x() = r[X];
	cm.y() = r[Y];
	cm.z() = r[Z];
	
	vol = v.T0;	

	density = mass / v.T0;

	I(X,X) = density * (v.T2[Y] + v.T2[Z]);
	I(Y,Y) = density * (v.T2[Z] + v.T2[X]);
	I(Z,Z) = density * (v.T2[X] + v.T2[Y]);
	I(X,Y) = I(Y,X) = - density * v.TP[X];
	I(Y,Z) = I(Z,Y) = - density * v.TP[Y];
	I(Z,X) = I(X,Z) = - density * v.TP[Z];

	I(X,X) -= mass * (r[Y]*r[Y] + r[Z]*r[Z]);
	I(Y,Y) -= mass * (r[Z]*r[Z] + r[X]*r[X]);
	I(Z,Z) -= mass * (r[X]*r[X] + r[Y]*r[Y]);
	I(X,Y) = I(Y,X) += mass * r[X] * r[Y]; 
	I(Y,Z) = I(Z,Y) += mass * r[Y] * r[Z]; 
	I(Z,X) = I(X,Z) += mass * r[Z] * r[X]; 

	return 0;
}

static void compProjectionIntegrals(projectionIntegrals* p, const FACE *f, const int A, const int B, const int C)
{
	double a0, a1, da;
	double b0, b1, db;
	double a0_2, a0_3, a0_4, b0_2, b0_3, b0_4;
	double a1_2, a1_3, b1_2, b1_3;
	double C1, Ca, Caa, Caaa, Cb, Cbb, Cbbb;
	double Cab, Kab, Caab, Kaab, Cabb, Kabb;
	int i;

	p->P1 = p->Pa = p->Pb = p->Paa = p->Pab = p->Pbb = p->Paaa = p->Paab = p->Pabb = p->Pbbb = 0.0;

	for (i = 0; i < f->numVerts; i++) 
	{
		a0 = f->poly->verts[f->verts[i]][A];
		b0 = f->poly->verts[f->verts[i]][B];
		a1 = f->poly->verts[f->verts[(i+1) % f->numVerts]][A];
		b1 = f->poly->verts[f->verts[(i+1) % f->numVerts]][B];
		da = a1 - a0;
		db = b1 - b0;
		a0_2 = a0 * a0; a0_3 = a0_2 * a0; a0_4 = a0_3 * a0;
		b0_2 = b0 * b0; b0_3 = b0_2 * b0; b0_4 = b0_3 * b0;
		a1_2 = a1 * a1; a1_3 = a1_2 * a1; 
		b1_2 = b1 * b1; b1_3 = b1_2 * b1;

		C1 = a1 + a0;
		Ca = a1*C1 + a0_2; Caa = a1*Ca + a0_3; Caaa = a1*Caa + a0_4;
		Cb = b1*(b1 + b0) + b0_2; Cbb = b1*Cb + b0_3; Cbbb = b1*Cbb + b0_4;
		Cab = 3*a1_2 + 2*a1*a0 + a0_2; Kab = a1_2 + 2*a1*a0 + 3*a0_2;
		Caab = a0*Cab + 4*a1_3; Kaab = a1*Kab + 4*a0_3;
		Cabb = 4*b1_3 + 3*b1_2*b0 + 2*b1*b0_2 + b0_3;
		Kabb = b1_3 + 2*b1_2*b0 + 3*b1*b0_2 + 4*b0_3;

		p->P1 += db*C1;
		p->Pa += db*Ca;
		p->Paa += db*Caa;
		p->Paaa += db*Caaa;
		p->Pb += da*Cb;
		p->Pbb += da*Cbb;
		p->Pbbb += da*Cbbb;
		p->Pab += db*(b1*Cab + b0*Kab);
		p->Paab += db*(b1*Caab + b0*Kaab);
		p->Pabb += da*(a1*Cabb + a0*Kabb);
	}

	p->P1 /= 2.0;
	p->Pa /= 6.0;
	p->Paa /= 12.0;
	p->Paaa /= 20.0;
	p->Pb /= -6.0;
	p->Pbb /= -12.0;
	p->Pbbb /= -20.0;
	p->Pab /= 24.0;
	p->Paab /= 60.0;
	p->Pabb /= -60.0;
}

/************************************************************************************************
	compFaceIntegrals
*************************************************************************************************/

static void compFaceIntegrals(faceIntegrals* fi, const FACE *f, const int A, const int B, const int C)
{
	const double *n;
	double w;
	double k1, k2, k3, k4;
	projectionIntegrals p;
	
	compProjectionIntegrals(&p, f, A, B, C);

	w = f->w;
	n = f->norm;
	k1 = 1 / n[C]; k2 = k1 * k1; k3 = k2 * k1; k4 = k3 * k1;

	fi->Fa = k1 * p.Pa;
	fi->Fb = k1 * p.Pb;
	fi->Fc = -k2 * (n[A]*p.Pa + n[B]*p.Pb + w*p.P1);

	fi->Faa = k1 * p.Paa;
	fi->Fbb = k1 * p.Pbb;
	fi->Fcc = k3 * (SQR(n[A])*p.Paa + 2*n[A]*n[B]*p.Pab + SQR(n[B])*p.Pbb
	 + w*(2*(n[A]*p.Pa + n[B]*p.Pb) + w*p.P1));

	fi->Faaa = k1 * p.Paaa;
	fi->Fbbb = k1 * p.Pbbb;
	fi->Fccc = -k4 * (CUBE(n[A])*p.Paaa + 3*SQR(n[A])*n[B]*p.Paab 
	   + 3*n[A]*SQR(n[B])*p.Pabb + CUBE(n[B])*p.Pbbb
	   + 3*w*(SQR(n[A])*p.Paa + 2*n[A]*n[B]*p.Pab + SQR(n[B])*p.Pbb)
	   + w*w*(3*(n[A]*p.Pa + n[B]*p.Pb) + w*p.P1));

	fi->Faab = k1 * p.Paab;
	fi->Fbbc = -k2 * (n[A]*p.Pabb + n[B]*p.Pbbb + w*p.Pbb);
	fi->Fcca = k3 * (SQR(n[A])*p.Paaa + 2*n[A]*n[B]*p.Paab + SQR(n[B])*p.Pabb
	 + w*(2*(n[A]*p.Paa + n[B]*p.Pab) + w*p.Pa));
}

/************************************************************************************************
	compVolumeIntegrals
*************************************************************************************************/

static void compVolumeIntegrals(volumeIntegrals* v, const POLYHEDRON *p)
{
	const FACE *f;
	double nx, ny, nz;
	int i;
	int A, B, C;
	faceIntegrals fi;
	
	v->T0 = v->T1[X] = v->T1[Y] = v->T1[Z] =
	v->T2[X] = v->T2[Y] = v->T2[Z] =
	v->TP[X] = v->TP[Y] = v->TP[Z] = 0;

	for (i = 0; i < p->numFaces; i++) 
	{
		f = &p->faces[i];

		nx = fabs(f->norm[X]);
		ny = fabs(f->norm[Y]);
		nz = fabs(f->norm[Z]);
		if (nx > ny && nx > nz) C = X;
		else C = (ny > nz) ? Y : Z;
		A = (C + 1) % 3;
		B = (A + 1) % 3;

		compFaceIntegrals(&fi, f, A, B, C);

		v->T0 += f->norm[X] * ((A == X) ? fi.Fa : ((B == X) ? fi.Fb : fi.Fc));

		v->T1[A] += f->norm[A] * fi.Faa;
		v->T1[B] += f->norm[B] * fi.Fbb;
		v->T1[C] += f->norm[C] * fi.Fcc;
		v->T2[A] += f->norm[A] * fi.Faaa;
		v->T2[B] += f->norm[B] * fi.Fbbb;
		v->T2[C] += f->norm[C] * fi.Fccc;
		v->TP[A] += f->norm[A] * fi.Faab;
		v->TP[B] += f->norm[B] * fi.Fbbc;
		v->TP[C] += f->norm[C] * fi.Fcca;
	}

	v->T1[X] /= 2; v->T1[Y] /= 2; v->T1[Z] /= 2;
	v->T2[X] /= 3; v->T2[Y] /= 3; v->T2[Z] /= 3;
	v->TP[X] /= 2; v->TP[Y] /= 2; v->TP[Z] /= 2;
}