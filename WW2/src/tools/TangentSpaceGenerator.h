#pragma once
/* -*-c++-*- OpenSceneGraph - Copyright (C) 1998-2006 Robert Osfield
 *
 * This library is open source and may be redistributed and/or modified under
 * the terms of the OpenSceneGraph Public License (OSGPL) version 0.0 or
 * (at your option) any later version.  The full license is in LICENSE file
 * included with this distribution, and on the openscenegraph.org website.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * OpenSceneGraph Public License for more details.
*/

#include "ob/obmain.h"
#include "math/Vec4f.h"
#include <vector>

/**
 The TangentSpaceGenerator class generates three arrays containing tangent-space basis vectors.
 It takes a texture-mapped Geometry object as input, traverses its primitive sets and computes
 Tangent, Normal and Binormal vectors for each vertex, storing them into arrays.
 The resulting arrays can be used as vertex program varying (per-vertex) parameters,
 enabling advanced effects like bump-mapping.
 To use this class, simply call the generate() method specifying the Geometry object
 you want to process and the texture unit that contains UV mapping for the normal map;
 then you can retrieve the TBN arrays by calling getTangentArray(), getNormalArray()
 and getBinormalArray() methods.
 */
class TangentSpaceGenerator
{
public:
    TangentSpaceGenerator();
    virtual ~TangentSpaceGenerator() {}

    void generate(OBJECT* geo);
    void generate(piUNIFIED_VERTEX* vertices,
                  piINDICES* ptris,
                  unsigned int numverts,
                  unsigned int numtris,
                  unsigned int flags);
    void generate(piVectorConstPtr vx, // positions
                  piVectorConstPtr nx, // normals
                  piTexCoordConstPtr tx, // texture coords
                  piINDICES* ptris,
                  unsigned int numverts,
                  unsigned int numtris,
                  unsigned int flags);

    inline std::vector<Vec4f>& getTangentArray() {return T_;}
    inline const std::vector<Vec4f>& getTangentArray() const {return T_;}

    inline std::vector<Vec4f>& getNormalArray() {return N_;}
    inline const std::vector<Vec4f>& getNormalArray() const {return N_;}

    inline std::vector<Vec4f>& getBinormalArray() {return B_;}
    inline const std::vector<Vec4f>& getBinormalArray() const {return B_;}

    inline std::vector<int>* getIndices() {return &indices_;}

protected:
    TangentSpaceGenerator &operator=(const TangentSpaceGenerator &) {return *this;}

    void compute(piVectorConstPtr vx,
                 piVectorConstPtr nx,
                 piTexCoordConstPtr tx,
                 piINDICES* tris,
                 int a,
                 int b,
                 int c);

    std::vector<Vec4f> T_;
    std::vector<Vec4f> B_;
    std::vector<Vec4f> N_;
    std::vector<int> indices_;
};
