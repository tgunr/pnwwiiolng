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

#define SHADOW_TERRAIN	        0x00000001
#define SHADOW_OBJECTS	        0x00000002
#define SHADOW_VEHICLES         0x00000004
#define SHADOW_GRASS            0x00000008
#define SHADOW_COCKPITS	        0x00000010
#define SHADOW_CORPSES	        0x00000020
#define SHADOW_PLANE_DISABLE    0x00000040
#define SHADOWED_CORPSE         0x00000080

#include "math/Vec2f.h"
#include "shader/glsl.h"
#include "shader/glslShader.h"
#include "osgImport/Texture2D.h"
#include "osgImport/FrameBufferObject.h"
#include "osgImport/Polytope.h"
#include "ogl/oglMaterial.h"

#define DEBUG_SHADOW_MAP 0

struct SingleShadowMap
{
    piVECTOR center;

    bool valid;

    ref_ptr<Texture2D> map;

#if USE_STATESET
    ref_ptr<RenderStateSet> stateset;
#else
    ref_ptr<FrameBufferObject> fbo;
#endif
};

struct ShadowMapOctet
{
    int r;
    int c;
    int sx;
    int sy;

    piMATRIX mvp;
    piMATRIX mv;
    piMATRIX proj;
    piMATRIX texmat;

    piVECTOR center;

    bool valid;

    ref_ptr<Texture2D> map;
#if USE_STATESET
    ref_ptr<RenderStateSet> stateset;
#else
    ref_ptr<FrameBufferObject> fbo;
#endif
};

struct DynamicShadowMap
{
    piMATRIX mvp;
    piMATRIX mv;
    piMATRIX proj;
    piMATRIX texmat;

    piVECTOR center;

    bool valid;

    ref_ptr<Texture2D> map;

#if USE_STATESET
    ref_ptr<RenderStateSet> stateset;
#else
    ref_ptr<FrameBufferObject> fbo;
#endif
};

/** ShadowedTexture provides an implementation of shadow textures.*/
class ShadowMap : public Referenced
{
public:
    ShadowMap();
    virtual ~ShadowMap() {};

    static ShadowMap& GetInstance();

    /** Set the texture unit that the shadow texture will be applied on.*/
    void setTextureUnit(unsigned int unit);
    void Kill();

    /** Get the texture unit that the shadow texture will be applied on.*/
    unsigned int getTextureUnit() const { return _shadowTextureUnit; }

    /** set the polygon offset used initially */
    void setPolygonOffset(const Vec2f& polyOffset);

    /** get the used polygon offset */
    const Vec2f& getPolygonOffset() const { return _polyOffset; }

    /** Set the values for the ambient bias the shader will use.*/
    void setAmbientBias(const Vec2f& ambientBias );

    /** Get the values that are used for the ambient bias in the shader.*/
    const Vec2f& getAmbientBias() const { return _ambientBias; }

    /** set the size in pixels x / y for the shadow texture.*/
    void setTextureSize(int w, int h);
    void Reset() {_reset = true;}

    bool BeginStaticShadowMap(int corner);
    bool BeginShadowMap();
    bool BeginDynamicShadowMap();
    bool SetShadowStuff(struct OBJECT* o);
    bool SetShadowStuff(oglMaterial& material, int unit = 0);
    bool SetShadowStuff(oglMaterial& material, const Vec3d& point, int unit = 0);
#if USE_STATESET
    bool SetShadowStuff(RenderStateSet& material, const Vec3d& point, int unit);
#endif
    bool SetShadowStuff(const piVECTOR& point, unsigned int flags = 0);
    bool Enabled() {return _enabled;}
    bool Valid();
    bool cull(const BoundingSphere& bs);

    float GetScale() {return 1.0f / _texwidth;}

    void ApplyShadowMaps();
    void EndStaticShadowMap(int corner);
    void EndShadowMap();
    void EndDynamicShadowMap();
    void ApplyShadowMapNoShader();
    void UnApplyShadowMapNoShader();
    void RenderShadowCasters();
    void RenderStaticShadowCasters();
    void RenderDynamicShadowCasters();
    void UnApplyShadowMaps(int unit = 0);
    void PostRenderUpdate();
    void Enable(bool enable) {_enabled = enable;}
    void setupCull();
    void GetShadowAmbient(Vec3f& ambient);

    float DynamicShadowDistance() {return _DynamicShadowDistance;}
    float StaticShadowDistance() {return _StaticShadowDistance;}

    const Matrixf& GetShadowMapCamera() {return camera;}
    const Matrixf& GetInverseMVP();
    Texture2D* GetShadowMapTexture();

#if DEBUG_SHADOW_MAP
    void Debug();
#endif

    /** Get the values that are used for the ambient bias in the shader.*/
    int getTextureWidth() {return _texwidth;}
    int getTextureHeight() {return _texheight;}

    /** Set the Light that will cast shadows */
    void setLight(LIGHT* light);

    /** initialize the ShadowedScene and local cached data structures.*/
    virtual void init();

protected:

    /** Create the managed Uniforms */
    bool FindOctetCenter(int corner);
    bool FindPlayerPosition();

    bool _reset;
    bool _enabled;

    float _DynamicShadowDistance;
    float _StaticShadowDistance;

    ref_ptr<FrameBufferObject> _shadowfbo;

    SingleShadowMap smoctet;
    ShadowMapOctet _smOctet[4];

    Matrixf camera;
    Matrixf projection;
    Matrixf modelviewproj;
    Matrixf invmvp;
    Matrixf oldview;

    DynamicShadowMap DynShadowMap;

    ref_ptr<glslProgram> _program;
    ref_ptr<Texture2D> _white;

    LIGHT* _light;

    piMATRIX _invModelView;
    Polytope polytope;

    unsigned int _baseTextureUnit;
    unsigned int _shadowTextureUnit;

    Vec2f _polyOffset;
    Vec2f _ambientBias;

    piVECTOR _offset;

    int DynamicTexWidth;
    int DynamicTexHeight;
    int _texwidth;
    int _texheight;
    int _lastcorner;
    int lastshadowcreate;
};

