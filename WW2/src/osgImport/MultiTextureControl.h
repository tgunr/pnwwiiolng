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

/**
  This node provides control over the which texture units are active and the
  blending weighting between them.
 */
class MultiTextureControl
{
public:

    MultiTextureControl();

    void setTextureWeight(unsigned int unit, float weight);

    float getTextureWeight(unsigned int unit) const
    {
        return (unit<_textureWeightList.size()) ?  _textureWeightList[unit] : 0.0f;
    }

    unsigned int getNumTextureWeights() const
    {
        return _textureWeightList.size();
    }

protected:
    virtual ~MultiTextureControl() {}

    MultiTextureControl& operator = (const MultiTextureControl&)
    {
        return *this;
    }

    void updateStateSet();

    using TextureWeightList = std::vector<float>;
    TextureWeightList _textureWeightList;
};

