#pragma once

extern LOGICAL obsphCreate(const char* name, int MeshSize, piVECTOR* Location, float Scale, UINT32 Color, OBJECT* Object);
extern LOGICAL obsphCreateCylinder(const char* name, int MeshSize, piVECTOR* Location, float Scale, float Ellipse, UINT32 Color, OBJECT* lpobj);
extern LOGICAL obsphCreateHemi(const char* name, int MeshSize, piVECTOR* Location, float Scale, float Ellipse, UINT32 Color, OBJECT* lpobj);
extern LOGICAL obsphCreateQuarter(const char* name, int MeshSize, piVECTOR* Location, float Scale, UINT32 Color, OBJECT* lpobj);
extern LOGICAL obsphCreateVoxel(const char* name, const char* texname, int MeshSize, piVECTOR* Location, float Scale, float Depth, UINT32 Color, OBJECT* lpobj);
