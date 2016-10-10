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

#include "pi/pi.h"
#include "math/Vec4f.h"

/** TexEnv encapsulates the OpenGL glTexEnv (texture environment) state.
*/
class TexEnv
{
    public :

        enum Mode {
            DECAL     = GL_DECAL,
            MODULATE  = GL_MODULATE,
            BLEND     = GL_BLEND,
            REPLACE   = GL_REPLACE,
            ADD       = GL_ADD
        };

        TexEnv(Mode mode=MODULATE);

        virtual bool isTextureAttribute() const { return true; }

        void setMode( Mode mode ) { _mode = mode; }

        Mode getMode() const { return _mode; }

        void setColor( const Vec4f& color ) { _color = color; }

        Vec4f& getColor() { return _color; }

        const Vec4f& getColor() const { return _color; }

        virtual void apply() const;

    protected :

        virtual ~TexEnv();

        Mode        _mode;
        Vec4f   _color;
};

