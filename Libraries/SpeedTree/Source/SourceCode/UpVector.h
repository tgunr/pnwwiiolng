///////////////////////////////////////////////////////////////////////  
//  UpVector.h
//
//  *** INTERACTIVE DATA VISUALIZATION (IDV) CONFIDENTIAL AND PROPRIETARY INFORMATION ***
//
//  This software is supplied under the terms of a license agreement or
//  nondisclosure agreement with Interactive Data Visualization, Inc. and
//  may not be copied, disclosed, or exploited except in accordance with 
//  the terms of that agreement.
//
//      Copyright (c) 2003-2007 IDV, Inc.
//      All rights reserved in all media.
//
//      IDV, Inc.
//      http://www.idvinc.com

#pragma once

// if no up vector has been set, default to SPEEDTREE_UPVECTOR_Z
#if !defined SPEEDTREE_UPVECTOR_Z && !defined SPEEDTREE_UPVECTOR_NEG_Z && !defined SPEEDTREE_UPVECTOR_Y && !defined SPEEDTREE_UPVECTOR_NEG_Y && !defined SPEEDTREE_UPVECTOR_X && !defined SPEEDTREE_UPVECTOR_NEG_X && !defined SPEEDTREE_UPVECTOR_DIRECTX_RIGHT_HANDED_COORDINATE_SYSTEM
#define SPEEDTREE_UPVECTOR_Z
#endif


//  SpeedTree defaults to using a positive Z up vector and all of the branch
//  and leaf computations are done in this orientation.  When the geometry
//  is transferred to the classes CLeafGeometry and CIndexedGeometry, the
//  computed coordinates and normals are passed through the stAssign3d() 
//  function to be potentially reoriented.
//
//  One and only one of the following seven symbols should be defined:
//
//      SPEEDTREE_UPVECTOR_Z*
//      SPEEDTREE_UPVECTOR_NEG_Z
//      SPEEDTREE_UPVECTOR_Y*
//      SPEEDTREE_UPVECTOR_NEG_Y
//      SPEEDTREE_UPVECTOR_X
//      SPEEDTREE_UPVECTOR_NEG_X
//      SPEEDTREE_UPVECTOR_DIRECTX_RIGHT_HANDED_COORDINATE_SYSTEM
//
//  * Note that only SPEEDTREE_UPVECTOR_Z and SPEEDTREE_UPVECTOR_Y
//    are supported in the SpeedTree reference application.


///////////////////////////////////////////////////////////////////////  
//  function Assign3d definition (+Z up vector)

#ifdef SPEEDTREE_UPVECTOR_Z
inline void stAssign3d(float afDest[3], const float afSource[3])
{
    afDest[0] = afSource[0];
    afDest[1] = afSource[1];
    afDest[2] = afSource[2];
}
#endif


///////////////////////////////////////////////////////////////////////  
//  function Assign3d definition (-Z up vector)

#ifdef SPEEDTREE_UPVECTOR_NEG_Z
inline void stAssign3d(float afDest[3], const float afSource[3])
{
    afDest[0] = -afSource[0];
    afDest[1] = afSource[1];
    afDest[2] = -afSource[2];
}
#endif


///////////////////////////////////////////////////////////////////////  
//  function Assign3d definition (+Y up vector)

#ifdef SPEEDTREE_UPVECTOR_Y
inline void stAssign3d(float afDest[3], const float afSource[3])
{
    afDest[0] = -afSource[0];
    afDest[1] = afSource[2];
    afDest[2] = afSource[1];
}
#endif


///////////////////////////////////////////////////////////////////////  
//  function Assign3d definition (-Y up vector)

#ifdef SPEEDTREE_UPVECTOR_NEG_Y
inline void stAssign3d(float afDest[3], const float afSource[3])
{
    afDest[0] = afSource[0];
    afDest[1] = -afSource[2];
    afDest[2] = afSource[1];
}
#endif


///////////////////////////////////////////////////////////////////////  
//  function Assign3d definition (+X up vector)

#ifdef SPEEDTREE_UPVECTOR_X
inline void stAssign3d(float afDest[3], const float afSource[3])
{
    afDest[0] = afSource[2];
    afDest[1] = afSource[1];
    afDest[2] = -afSource[0];
}
#endif


///////////////////////////////////////////////////////////////////////  
//  function Assign3d definition (-X up vector)

#ifdef SPEEDTREE_UPVECTOR_NEG_X
inline void stAssign3d(float afDest[3], const float afSource[3])
{
    afDest[0] = -afSource[2];
    afDest[1] = afSource[1];
    afDest[2] = afSource[0];
}
#endif


///////////////////////////////////////////////////////////////////////  
//  function Assign3d definition (X and Y swapped)

#ifdef SPEEDTREE_UPVECTOR_DIRECTX_RIGHT_HANDED_COORDINATE_SYSTEM
inline void stAssign3d(float afDest[3], const float afSource[3])
{
    afDest[0] = afSource[1];
    afDest[1] = afSource[0];
    afDest[2] = afSource[2];
}
#endif


///////////////////////////////////////////////////////////////////////  
//  function ComputeAzimuth definition

inline float ComputeAzimuth(const float afCameraDir[3])
{
    // find the angle that point is on
    stVec3 cAdjustedDirection;
    stAssign3d(cAdjustedDirection, afCameraDir);

    return VecRad2Deg(atan2f(cAdjustedDirection[1], cAdjustedDirection[0]));
}
