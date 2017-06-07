#include "syscfg.h" // Path is project specific.
////////////////////////////////////////////////////
// Module: obmesh.cpp
// Description: Mesh creation routine
////////////////////////////////////////////////////

#include "pi/pi.h"

#include "ob/obmesh.h"
#include "mem/mem.h"

////////////////////////////////////////////////////
// Function: obmeshCreate
// Description: Returns number of vertices and the
// mesh as a piLVERTEX* structure. User must free
// this structure.
////////////////////////////////////////////////////
SINT32 obmeshCreate(SINT32 iNumVertsAlongX,
                    SINT32 iNumVertsAlongY,
                    FLOAT32 fLengthAlongX,
                    FLOAT32 fLengthAlongY,
                    FLOAT32 u1,
                    FLOAT32 v1,
                    FLOAT32 u2,
                    FLOAT32 v2,
                    piLVERTEX** lpVertices)
{
    SINT32 iMeshVertCount = (iNumVertsAlongX - 1) * (iNumVertsAlongY - 1) * 6;

    *lpVertices = (piLVERTEX*)memAlloc(sizeof(piLVERTEX) * iMeshVertCount);

    if(lpVertices == nullptr)
    {
        return -1;
    }

    piLVERTEX* lpVert = *lpVertices;

    FLOAT32 fWidthFactor = u2 - u1;
    FLOAT32 fHeightFactor = v2 - v1;

    FLOAT32 dx = 1.0f / (iNumVertsAlongX - 1);
    FLOAT32 dy = 1.0f / (iNumVertsAlongY - 1);

    FLOAT32 du = fWidthFactor / (iNumVertsAlongX - 1);
    FLOAT32 dv = fHeightFactor / (iNumVertsAlongY - 1);

 	SINT32 i = 0;
	SINT32 x = 0;
	SINT32 y = 0;

    for(i = 0; i < iMeshVertCount; i++)
	{
        lpVert[i].z = 0.0f;

        lpVert[i].color = colorWHITEALPHA;
        lpVert[i].specular = colorALPHA100;
	}

    for(y = 0, i = 0; y < (iNumVertsAlongY - 1); y++)
	{
        for(x = 0; x < (iNumVertsAlongX - 1); x++)
        {
            ////////////////////////////////////////
			// First triangle of the current quad
			// 1 ___
			//  |\  |
			//  |  \|
            //  -----
			// 0     2
            ////////////////////////////////////////

			// 0
			lpVert[i].x  = fLengthAlongX * x * dx;
			lpVert[i].y  = fLengthAlongY * y * dy;
			lpVert[i].tu = x * du + u1;
			lpVert[i].tv = y * dv + v1;
			i++;

			// 1
			lpVert[i].x  = fLengthAlongX * x * dx;
			lpVert[i].y  = fLengthAlongY * (y + 1.0f) * dy;
			lpVert[i].tu = x * du + u1;
			lpVert[i].tv = (y + 1.0f) * dv + v1;
			i++;

			// 2
			lpVert[i].x  = fLengthAlongX * (x + 1.0f) * dx;
			lpVert[i].y  = fLengthAlongY * y * dy;
			lpVert[i].tu = (x + 1.0f) * du + u1;
			lpVert[i].tv = y * dv + v1;
			i++;

            //////////////////////////////////////////
			// Second triangle of the current quad
			// 0 ___ 1
			//  |\  |
			//  |  \|
            //  -----
			//       2
            //////////////////////////////////////////

			// 0
			lpVert[i].x  = fLengthAlongX * x * dx;
			lpVert[i].y  = fLengthAlongY * (y + 1.0f) * dy;
			lpVert[i].tu = x * du + u1;
			lpVert[i].tv = (y + 1.0f) * dv + v1;
			i++;

			// 1
			lpVert[i].x  = fLengthAlongX * (x + 1.0f) * dx;
			lpVert[i].y  = fLengthAlongY * (y + 1.0f) * dy;
			lpVert[i].tu = (x + 1.0f) * du + u1;
			lpVert[i].tv = (y + 1.0f) * dv + v1;
			i++;

			// 2
			lpVert[i].x = fLengthAlongX * (x + 1.0f) * dx;
			lpVert[i].y = fLengthAlongY * y * dy;
			lpVert[i].tu = (x + 1.0f) * du + u1;
			lpVert[i].tv = y * dv + v1;
			i++;
		}
	}

    return iMeshVertCount;
}
