#pragma once

#include "math/Vec4f.h"
#include "osgImport/FrameBufferObject.h"
#include "osgImport/Referenced.h"

class glslShader;
class glslProgram;

class PostRenderFilter : public Referenced
{
public:
    PostRenderFilter();

    void apply();
    void kill();
    void drawDeprecated();
    void draw();
    void setupDeprecated();
    void setup();
    void setSceneTexture(Texture* texture) {_scenetexture = texture;}
    void copyscene();
    void setblackout(float r, float g, float b, float a);
    void setconcussion(float v);
    void init();

    bool IsValid() {return isvalid;}

    Texture* getscenetexture() {return _scenetexture.get();}

    static PostRenderFilter& GetInstance();

private:
    void _createDownSizeDeprecated();
    void _createFinalDeprecated();
    void _createFakeHDRDeprecated();
    void _createBlurDeprecated();

#if USE_STATESET
    void _createDepthBlurDeprecated();
    void _createFilterDeprecated();
    void _applyFilter();
    void _applyDepthBlur();
#else
    void _createFilterXDeprecated();
    void _createFilterYDeprecated();
    void _applyFilterX();
    void _applyFilterY();
#endif
    void _createDownSize();
    void _createFinal();
    void _createBlur();
    void _createFilterX();
    void _createFilterY();
    void _applyDownsize();
    void _applyFinal();
    void _applyBlur();
    void _applyFakeHDR();
    void _applyAdaption();

    void _drawQuad(float x, float y) const;

private:
    ref_ptr<Texture> _scenetexture;

#if USE_STATESET
    ref_ptr<RenderStateSet> downsize;
    ref_ptr<RenderStateSet> fakehdr;
    ref_ptr<RenderStateSet> blur;
    ref_ptr<RenderStateSet> filter;
    ref_ptr<RenderStateSet> final;
    ref_ptr<RenderStateSet> depthblur;
#endif

    ref_ptr<FrameBufferObject> downsizefbo;
    ref_ptr<FrameBufferObject> fakehdrfbo;
    ref_ptr<FrameBufferObject> blurfbo;

#if USE_STATESET
    ref_ptr<FrameBufferObject> filterfbo;
    ref_ptr<FrameBufferObject> depthblurfbo;
#else
    ref_ptr<FrameBufferObject> filterxfbo;
    ref_ptr<FrameBufferObject> filteryfbo;
#endif

    ref_ptr<glslShader> _postvertex;

    ref_ptr<glslProgram> _downsizePgm;
    ref_ptr<glslProgram> _finalPgm;
    ref_ptr<glslProgram> _fakehdrPgm;
    ref_ptr<glslProgram> _blurPgm;

#if USE_STATESET
    ref_ptr<glslProgram> _filterPgm;
    ref_ptr<glslProgram> _depthBlurPgm;
#else
    ref_ptr<glslProgram> _filterXPgm;
    ref_ptr<glslProgram> _filterYPgm;
#endif

    ref_ptr<Texture> _downsizeTex;
    ref_ptr<Texture> _blurTex;
    ref_ptr<Texture> _fakehdrTex;

#if USE_STATESET
    ref_ptr<Texture> _filterTex;
    ref_ptr<Texture> _depthBlurTex;
#else
    ref_ptr<Texture> _filterXTex;
    ref_ptr<Texture> _filterYTex;
#endif

    Vec4f color;

    bool isvalid;

    float concussion;
    float currentluminance;
    float lastluminance;
};

void PostRenderKill();