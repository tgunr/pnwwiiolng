#pragma once

#define USE_NORMAL_MAP
//#undef USE_NORMAL_MAP

extern	LOGICAL ldmdlGetBounds(struct OBJECT* lpobj);
extern	LOGICAL ldmdlGetObjectBounds(struct OBJECT* lpobj, struct piVECTOR* loc, struct piVECTOR* obox, float* minrad, float* maxrad);
extern	LOGICAL ldmdlUpdateLocation(struct OBJECT* lpobj);
extern	LOGICAL ldmdlCalcSurfaceNormals(struct OBJECT* lpobj);
extern	LOGICAL ldmdlCalcCollisionNormals(struct OBJECT* lpobj);
extern	LOGICAL ldmdlCalcVertexNormals(struct OBJECT* lpobj);
extern	LOGICAL ldmdlCalcNormals(struct OBJECT* lpobj);
extern	LOGICAL ldmdlReverseNormals(struct OBJECT* lpobj);
extern	LOGICAL ldmdlReverseObjectNormals(struct OBJECT* lpobj);
extern	LOGICAL ldmdlSortObjectTriangles(struct OBJECT* lpobj);
extern	LOGICAL ldmdlProcessNormalsSerialized();
extern	void	ldmdlReduceAttachments(struct OBJECT* node);
extern	void	ldmdlReduceModel(struct OBJECT* lpobj);
extern	void	ldmdlProcessBounds(struct OBJECT* o);
extern	void	ldmdlProcessBranchCollisionData(struct OBJECT* o);

	// difference between this and ldmdlCalcNormals is that this uses
	// the pdlverts instead and the vertex normals are left untouched
//	LOGICAL ldmdlCalcNormals1(struct OBJECT* lpobj);
extern	LOGICAL ldmdlCalcNormals2(struct OBJECT* lpobj);

extern	LOGICAL ldmdlUpdateGroupFilteringAndBlendStatus(struct OBJECT * ObjGrp);
extern  LOGICAL ldmdlReduceSpeedTreeObjects(struct OBJECT* root);

struct piVECTOR;
class piQUAT;
struct OBJECT;

extern void ldmdlTangentArray(const piVECTOR* vertex,      // IN
                           const piVECTOR* normal,      // IN
                           const piUV* texcoord,        // IN
                           const piINDICES* triangle,   // IN
                           SINT32 vertexCount,          // IN
                           SINT32 triangleCount,        // IN
                           piQUAT** tangent);           // OUT

extern    void ldmdlTangentArray2(const struct piUNIFIED_VERTEX* vertex,   // IN
                            SINT32 vertexCount,         // IN
                            SINT32 triangleCount,       // IN
                            piQUAT** tangent);          // OUT

extern    piQUAT* ldmdlTangentArray3(OBJECT* o);
extern	void ldmdlIndexVertices(OBJECT* o);

