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
#include "math/Vec3f.h"
#include "math/Vec3d.h"
#include <float.h>
#include <algorithm>
#include <limits>

#define USE_FLOAT_BOUNDINGBOX

class BoundingSphere;

/** General purpose axis-aligned bounding box class for enclosing objects/vertices.
  * Bounds leaf objects in a scene such as osg::Drawable objects. Used for frustum
  * culling etc.
*/
class BoundingBox
{
    public:

#ifdef USE_FLOAT_BOUNDINGBOX
        using vec_type = Vec3f;
        using value_type = float;
#else
        using vec_type = Vec3d;
        using value_type = double;
#endif


        /** Minimum extent. (Smallest X, Y, and Z values of all coordinates.) */
        vec_type _min;
        /** Maximum extent. (Greatest X, Y, and Z values of all coordinates.) */
        vec_type _max;

        /** Creates an uninitialized bounding box. */
        inline BoundingBox() : _min(FLT_MAX,FLT_MAX,FLT_MAX),
                        _max(-FLT_MAX,-FLT_MAX,-FLT_MAX) {}

        /** Creates a bounding box initialized to the given extents. */
        inline BoundingBox(value_type xmin, value_type ymin, value_type zmin,
                           value_type xmax, value_type ymax, value_type zmax) :
                           _min(xmin,ymin,zmin),
                           _max(xmax,ymax,zmax) {}

        /** Creates a bounding box initialized to the given extents. */
    inline BoundingBox(const vec_type& min,const vec_type& max) :
    _min(min),
    _max(max) {}

    // bounding box copy
    inline BoundingBox(const BoundingBox& bb) :
    _min(bb._min),
    _max(bb._max) {}

    // assignement operator
    inline BoundingBox & operator=(const BoundingBox& bb)
    { _min = bb._min; _max = bb._max; return *this; }
    
    // offset
    inline void offset( value_type diffx, value_type diffy, value_type diffz = 0.0 )
    {
        _min.set(_min.x()+diffx,_min.y()+diffy,_min.z()+diffz);
        _max.set(_max.x()+diffx,_max.y()+diffy,_max.z()+diffz);
    }

    /** Clear the bounding box. Erases existing minimum and maximum extents. */
        inline void init()
        {
            _min.set(FLT_MAX,FLT_MAX,FLT_MAX);
            _max.set(-FLT_MAX,-FLT_MAX,-FLT_MAX);
        }

        /** Returns true if the bounding box extents are valid, false otherwise. */
        inline bool valid() const
        {
            return _max.x()>=_min.x() &&  _max.y()>=_min.y() &&  _max.z()>=_min.z();
        }

        /** Sets the bounding box extents. */
        inline void set (value_type xmin, value_type ymin, value_type zmin,
                         value_type xmax, value_type ymax, value_type zmax)
        {
            _min.set(xmin,ymin,zmin);
            _max.set(xmax,ymax,zmax);
        }

        /** Sets the bounding box extents. */
        inline void set(const vec_type& min,const vec_type& max)
        {
            _min = min;
            _max = max;
        }


        inline value_type& xMin() { return _min.x(); }
        inline value_type xMin() const { return _min.x(); }

        inline value_type& yMin() { return _min.y(); }
        inline value_type yMin() const { return _min.y(); }

        inline value_type& zMin() { return _min.z(); }
        inline value_type zMin() const { return _min.z(); }

        inline value_type& xMax() { return _max.x(); }
        inline value_type xMax() const { return _max.x(); }

        inline value_type& yMax() { return _max.y(); }
        inline value_type yMax() const { return _max.y(); }

        inline value_type& zMax() { return _max.z(); }
        inline value_type zMax() const { return _max.z(); }

        /** Calculates and returns the bounding box center. */
        inline const vec_type center() const
        {
            return (_min+_max)*0.5;
        }

        /** Calculates and returns the bounding box radius. */
        inline value_type radius() const
        {
            return sqrt(radius2());
        }

        /** Calculates and returns the squared length of the bounding box radius.
          * Note, radius2() is faster to calculate than radius(). */
        inline value_type radius2() const
        {
            return 0.25f*((_max-_min).length2());
        }

        /** Returns a specific corner of the bounding box.
          * pos specifies the corner as a number between 0 and 7.
          * Each bit selects an axis, X, Y, or Z from least- to
          * most-significant. Unset bits select the minimum value
          * for that axis, and set bits select the maximum. */
        inline const vec_type corner(unsigned int pos) const
        {
            return vec_type(pos&1?_max.x():_min.x(),pos&2?_max.y():_min.y(),pos&4?_max.z():_min.z());
        }

        /** Expands the bounding box to include the given coordinate.
          * If the box is uninitialized, set its min and max extents to v. */
        inline void expandBy(const vec_type& v)
        {
            if(v.x()<_min.x()) _min.x() = v.x();
            if(v.x()>_max.x()) _max.x() = v.x();

            if(v.y()<_min.y()) _min.y() = v.y();
            if(v.y()>_max.y()) _max.y() = v.y();

            if(v.z()<_min.z()) _min.z() = v.z();
            if(v.z()>_max.z()) _max.z() = v.z();
        }

        /** Expands the bounding box to include the given coordinate.
          * If the box is uninitialized, set its min and max extents to
          * Vec3(x,y,z). */
        inline void expandBy(value_type x,value_type y,value_type z)
        {
            if(x<_min.x()) _min.x() = x;
            if(x>_max.x()) _max.x() = x;

            if(y<_min.y()) _min.y() = y;
            if(y>_max.y()) _max.y() = y;

            if(z<_min.z()) _min.z() = z;
            if(z>_max.z()) _max.z() = z;
        }

        /** Expands this bounding box to include the given bounding box.
          * If this box is uninitialized, set it equal to bb. */
        void expandBy(const BoundingBox& bb);

        /** Expands this bounding box to include the given sphere.
          * If this box is uninitialized, set it to include sh. */
        void expandBy(const BoundingSphere& sh);


        /** Returns the intersection of this bounding box and the specified bounding box. */
        BoundingBox intersect(const BoundingBox& bb) const
		{    return BoundingBox(std::max(xMin(),bb.xMin()),std::max(yMin(),bb.yMin()),std::max(zMin(),bb.zMin()),
								std::min(xMax(),bb.xMax()),std::min(yMax(),bb.yMax()),std::min(zMax(),bb.zMax()));

        }

        /** Return true if this bounding box intersects the specified bounding box. */
        bool intersects(const BoundingBox& bb) const
        {    return std::max(xMin(),bb.xMin()) <= std::min(xMax(),bb.xMax()) &&
                    std::max(yMin(),bb.yMin()) <= std::min(yMax(),bb.yMax()) &&
                    std::max(zMin(),bb.zMin()) <= std::min(zMax(),bb.zMax());

        }

        /** Returns true if this bounding box contains the specified coordinate. */
        inline bool contains(const vec_type& v) const
        {
            return valid() &&
                   (v.x()>=_min.x() && v.x()<=_max.x()) &&
                   (v.y()>=_min.y() && v.y()<=_max.y()) &&
                   (v.z()>=_min.z() && v.z()<=_max.z());
        }

        /** Returns true if this bounding box contains the specified coordinate. */
        inline bool contains2D(const vec_type& v) const
        {
            return valid() &&
                   (v.x()>=_min.x() && v.x()<=_max.x()) &&
                   (v.y()>=_min.y() && v.y()<=_max.y());
        }
};

