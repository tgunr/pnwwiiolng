#include <syscfg.h> // Path is project specific.
/*------------------------------------------------------------------
Module: postRender.cpp
Description: Handles post render filtering
-------------------------------------------------------------------*/
#include "include/vars.h"

#include "osgImport/TextureRectangle.h"
#include "osgImport/Texture2D.h"
#include "osgImport/FrameBufferObject.h"

#include "shader/glslProgram.h"
#include "fx/postRender.h"
#include "fio/fio.h"
#include "shader/glsl.h"
#include "math/Vec2f.h"
#include "trn/LensFlare.h"
#include "ogl/ogl.h"
#include "pi/piinfo.h"
#include "ogl/States.h"
#include "cfg/cfgCodes.h"

#if USE_STATESET
#include "ob/obdraw.h"
#include "pers/persgunsight.h"
#include "pers/persmain.h"
#endif

static bool canUseTexture2D = false;

static ref_ptr<PostRenderFilter> postrender;

/*------------------------------------------------------------------
Function: PostRenderKill
Description:
-------------------------------------------------------------------*/
void PostRenderKill()
{
    postrender->kill();
    postrender = nullptr;
}

/*------------------------------------------------------------------
Function: PostRenderDraw
Description:
-------------------------------------------------------------------*/
void PostRenderFilter::apply()
{
    if(!IsValid())
    {
        return;
    }

    if(!g_postRenderFilter)
    {
        return;
    }

    if(!g_lensFlare.GetSunTexture())
    {
        return;
    }

    if(ogl::UseDeprecatedPath())
    {
        copyscene();
        drawDeprecated();
    }
    else
        draw();
}

/*------------------------------------------------------------------
Function: init
Description:
-------------------------------------------------------------------*/
void PostRenderFilter::init()
{
    // Texture2D Ok?
    // Note: When testing GL_TEXTURE_RECTANGLE_ARB, set oglCanUseTexture2D
    // to return false, since we need the sun glare FBO to be a GL_TEXTURE_RECTANGLE_ARB
    // too. Just setting canUseTexture2D to false here will produce anomalies.
    canUseTexture2D = oglCanUseTexture2D(WantedWidth, WantedHeight);

#if USE_STATESET
    downsize = new RenderStateSet;
    blur = new RenderStateSet;
    filter = new RenderStateSet;
    final = new RenderStateSet;
    depthblur = new RenderStateSet;

    downsize->SetMode(GL_BLEND, 0);
    downsize->SetMode(GL_LIGHTING, 0);
    downsize->SetMode(GL_DEPTH_TEST, 0);

    blur->SetMode(GL_BLEND, 0);
    blur->SetMode(GL_LIGHTING, 0);
    blur->SetMode(GL_DEPTH_TEST, 0);

    filter->SetMode(GL_BLEND, 0);
    filter->SetMode(GL_LIGHTING, 0);
    filter->SetMode(GL_DEPTH_TEST, 0);

    final->SetMode(GL_BLEND, 0);
    final->SetMode(GL_LIGHTING, 0);
    final->SetMode(GL_DEPTH_TEST, 0);

    depthblur->SetMode(GL_BLEND, 0);
    depthblur->SetMode(GL_LIGHTING, 0);
    depthblur->SetMode(GL_DEPTH_TEST, 0);
#endif

    if(ogl::UseDeprecatedPath())
        setupDeprecated();
    else
        setup();
}

/*------------------------------------------------------------------
Function: GetSampleOffsets_DownScale4x4
Description:
-------------------------------------------------------------------*/
void GetSampleOffsets_DownScale4x4(int width, int height, Vec2f offsets[])
{
    float u = 1.0f / float(width);
    float v = 1.0f / float(height);

    int i = 0;

    for(int y = 0; y < 4; y++)
    {
        for(int x = 0; x < 4; x++)
        {
            offsets[i].x() = (x - 1.5f) * u;
            offsets[i].y() = (y - 1.5f) * v;

            i++;
        }
    }
}

/*------------------------------------------------------------------
Function: PostRenderFilter::GetInstance
Description:
-------------------------------------------------------------------*/
PostRenderFilter& PostRenderFilter::GetInstance()
{
    if(!postrender.valid())
        postrender = new PostRenderFilter;

    return *postrender.get();
}

/*------------------------------------------------------------------
Function: PostRenderFilter::PostRenderFilter
Description:
-------------------------------------------------------------------*/
PostRenderFilter::PostRenderFilter()
{
    currentluminance = 0.0f;
    lastluminance = 0.0f;
    isvalid = false;
}

#if USE_STATESET
/*------------------------------------------------------------------
Function: PostRenderFilter::setupDeprecated
Description:
-------------------------------------------------------------------*/
void PostRenderFilter::setupDeprecated()
{
    extern Texture* obdrawGetSceneTexture();
    _scenetexture = obdrawGetSceneTexture();

    // Failed!
    if(!_scenetexture)
    {
        return;
    }

    if(canUseTexture2D)
    {
        _postvertex = GLSL.loadVertexShader("fx/shaders/GLSL/postrender/2D/postvertex.vert");
    }
    else
    {
        _postvertex = GLSL.loadVertexShader("fx/shaders/GLSL/postrender/RECT/postvertex.vert");
    }

    _createDepthBlurDeprecated();
    _createBlurDeprecated();
    _createFinalDeprecated();

    isvalid = true;
}
#else
/*------------------------------------------------------------------
Function: PostRenderFilter::setupDeprecated
Description:
-------------------------------------------------------------------*/
void PostRenderFilter::setupDeprecated()
{
    bool fbosupported = piinfoIsSupportedPref(cfgPOST_RENDER_FILTER);

    if(!fbosupported)
    {
        return;
    }

    // FIXME: what to do with non-fbo support(?)

    if(canUseTexture2D)
    {
        Texture2D* tex = new Texture2D;

        tex->setSourceFormat(GL_RGBA);
        tex->setSourceType(GL_UNSIGNED_BYTE);
        tex->setInternalFormat(GL_RGBA8);
        tex->setFilter(Texture::MAG_FILTER, Texture::LINEAR);
        tex->setFilter(Texture::MIN_FILTER, Texture::LINEAR);
        tex->setTextureHeight(WantedHeight);
        tex->setTextureWidth(WantedWidth);

        _scenetexture = tex;
    }
    // Default to texture rectangle (non-normalized)
    else if(GLEW_ARB_texture_rectangle)
    {
        TextureRectangle* tex = new TextureRectangle;

        tex->setSourceFormat(GL_RGBA);
        tex->setSourceType(GL_UNSIGNED_BYTE);
        tex->setInternalFormat(GL_RGBA8);
        tex->setFilter(Texture::MAG_FILTER, Texture::LINEAR);
        tex->setFilter(Texture::MIN_FILTER, Texture::LINEAR);
        tex->setTextureHeight(WantedHeight);
        tex->setTextureWidth(WantedWidth);

        _scenetexture = tex;
    }

    // Failed!
    if(!_scenetexture)
    {
        return;
    }

    if(canUseTexture2D)
    {
        _postvertex = GLSL.loadVertexShader("fx/shaders/GLSL/postrender/2D/postvertex.vert");
    }
    else
    {
        _postvertex = GLSL.loadVertexShader("fx/shaders/GLSL/postrender/RECT/postvertex.vert");
    }

    _createDownSizeDeprecated();
    _createFilterXDeprecated();
    _createFilterYDeprecated();
    _createBlurDeprecated();
    _createFinalDeprecated();

    isvalid = true;
}
#endif

/*------------------------------------------------------------------
Function: PostRenderFilter::setup
Description:
-------------------------------------------------------------------*/
void PostRenderFilter::setup()
{
#if(0)
    extern Texture* obdrawGetSceneTexture();

    bool fbosupported = piinfoIsSupportedPref(cfgPOST_RENDER_FILTER);

    if(!fbosupported)
    {
        return;
    }

    _scenetexture = obdrawGetSceneTexture();

    if(!_scenetexture)
    {
        if(canUseTexture2D)
        {
            Texture2D* tex = new Texture2D;

            tex->setSourceFormat(GL_RGBA);
            tex->setSourceType(GL_UNSIGNED_BYTE);
            tex->setInternalFormat(GL_RGBA8);
            tex->setFilter(Texture::MAG_FILTER, Texture::LINEAR);
            tex->setFilter(Texture::MIN_FILTER, Texture::LINEAR);
            tex->setTextureHeight(WantedHeight);
            tex->setTextureWidth(WantedWidth);

            _scenetexture = tex;
        }
        // Default to texture rectangle (non-normalized)
        else if(GLEW_ARB_texture_rectangle)
        {
            TextureRectangle* tex = new TextureRectangle;

            tex->setSourceFormat(GL_RGBA);
            tex->setSourceType(GL_UNSIGNED_BYTE);
            tex->setInternalFormat(GL_RGBA8);
            tex->setFilter(Texture::MAG_FILTER, Texture::LINEAR);
            tex->setFilter(Texture::MIN_FILTER, Texture::LINEAR);
            tex->setTextureHeight(WantedHeight);
            tex->setTextureWidth(WantedWidth);

            _scenetexture = tex;
        }
    }

    _createDownSize();
    _createFilterX();
    _createFilterY();
    _createBlur();
    _createFinal();

    ProgramState::Bind(0);

    isvalid = true;
#endif
}

/*------------------------------------------------------------------
Function: PostRenderFilter::copyscene
Description:
-------------------------------------------------------------------*/
void PostRenderFilter::copyscene()
{
#if USE_STATESET
#else
    oglSetActiveTextureStage(0);
    _scenetexture->apply();

    glCopyTexSubImage2D(_scenetexture->getTextureTarget(),
                        0,
                        0,
                        0,
                        0,
                        0,
                        WantedWidth,
                        WantedHeight);
#endif
}

/*------------------------------------------------------------------
Function: setblackout
Description:
-------------------------------------------------------------------*/
void PostRenderFilter::setblackout(float r, float g, float b, float a)
{
    color.set(r, g, b, a);
}

/*------------------------------------------------------------------
Function: setconcussion
Description:
-------------------------------------------------------------------*/
void PostRenderFilter::setconcussion(float v)
{
    concussion = v < 0.0f ? 0.0f : v > 1.0f ? 1.0f : v;
}

#if USE_STATESET
/*------------------------------------------------------------------
Function: SetupFilterKernel
Description:
-------------------------------------------------------------------*/
static void SetupFilterKernel(glslProgram* pgm)
{
    float dx = 1.0f;
    float dy = 1.0f;

    if(canUseTexture2D)
    {
        dx = 1.0f / (float)WantedWidth;
        dy = 1.0f / (float)WantedHeight;
    }

#define num_taps 12

    glslUniform* filter = pgm->GetOrCreateUniform("filter", glslUniform::FLOAT_VEC2, num_taps);

    Vec2f v[num_taps];

    v[0]  = Vec2f(-0.326212f * dx, -0.405805f * dy);
    v[1]  = Vec2f(-0.840144f * dx, -0.07358f * dy);
    v[2]  = Vec2f(-0.695914f * dx, 0.457137f * dy);
    v[3]  = Vec2f(-0.203345f * dx, 0.620716f * dy);
    v[4]  = Vec2f(0.96234f * dx, -0.194983f * dy);
    v[5]  = Vec2f(0.473434f * dx, -0.480026f * dy);
    v[6]  = Vec2f(0.519456f * dx, 0.767022f * dy);
    v[7]  = Vec2f(0.185461f * dx, -0.893124f * dy);
    v[8]  = Vec2f(0.507431f * dx, 0.064425f * dy);
    v[9]  = Vec2f(0.89642f * dx, 0.412458f * dy);
    v[10] = Vec2f(-0.32194f * dx, -0.932615f * dy);
    v[11] = Vec2f(-0.791559f * dx, -0.597705f * dy);

    for(int i = 0; i < num_taps; i++)
    {
        filter->SetElement(i, v[i]);
    }

    filter->SetOneTimeApply(true);
}

/*------------------------------------------------------------------
Function: _createFinalDeprecated
Description:
-------------------------------------------------------------------*/
void PostRenderFilter::_createFinalDeprecated()
{
    extern Texture* obdrawGetSceneDepthTexture();

    // Create shaders for blur in x direction
    ref_ptr<glslShader> frag;
    ref_ptr<glslShader> vert;

    if(canUseTexture2D)
    {
        frag = GLSL.loadFragmentShader("fx/shaders/GLSL/postrender/2D/finaldof.frag");
    }
    else
    {
        vert = GLSL.loadVertexShader("fx/shaders/GLSL/postrender/RECT/final.vert");
        frag = GLSL.loadFragmentShader("fx/shaders/GLSL/postrender/RECT/finaldof.frag");
    }

    // Create blur program
    _finalPgm = new glslProgram;

    if(canUseTexture2D)
        _finalPgm->AddShader(_postvertex.get());
    else
        _finalPgm->AddShader(vert.get());

    _finalPgm->AddShader(frag.get());

    // Now the uniform variables
    _finalPgm->GetOrCreateUniform("src1", glslUniform::SAMPLER_2D);
    _finalPgm->GetOrCreateUniform("src2", glslUniform::SAMPLER_2D);

    _finalPgm->GetUniform("src1")->SetOneTimeApply(true);
    _finalPgm->GetUniform("src2")->SetOneTimeApply(true);

    _finalPgm->GetOrCreateUniform("color", glslUniform::FLOAT_VEC4);
    _finalPgm->GetOrCreateUniform("lum", glslUniform::FLOAT);

    _finalPgm->GetUniform("color")->Set(Vec4f(0, 0, 0, 0));
    _finalPgm->GetUniform("lum")->Set(0.0f);

    _finalPgm->GetUniform("src1")->Set(0);
    _finalPgm->GetUniform("src2")->Set(1);

    final->SetRenderState(new Program(_finalPgm.get()));
    final->SetRenderState(new TextureUnit(0, _depthBlurTex.get()));
    final->SetRenderState(new TextureUnit(1, _blurTex.get()));
    final->SetRenderState(new Viewport(0, 0, WantedWidth, WantedHeight));

    SetupFilterKernel(_finalPgm.get());
}
#else
/*------------------------------------------------------------------
Function: _createFinalDeprecated
Description:
-------------------------------------------------------------------*/
void PostRenderFilter::_createFinalDeprecated()
{
    // Create shaders for blur in x direction
    ref_ptr<glslShader> frag;
    ref_ptr<glslShader> vert;

    if(canUseTexture2D)
    {
        frag = GLSL.loadFragmentShader("fx/shaders/GLSL/postrender/2D/final.frag");
    }
    else
    {
        vert = GLSL.loadVertexShader("fx/shaders/GLSL/postrender/RECT/final.vert");
        frag = GLSL.loadFragmentShader("fx/shaders/GLSL/postrender/RECT/final.frag");
    }

    // Create blur program
    _finalPgm = new glslProgram;

    if(canUseTexture2D)
        _finalPgm->AddShader(_postvertex.get());
    else
        _finalPgm->AddShader(vert.get());

    _finalPgm->AddShader(frag.get());

    // Now the uniform variables
    _finalPgm->GetOrCreateUniform("src1", glslUniform::SAMPLER_2D);
    _finalPgm->GetOrCreateUniform("src2", glslUniform::SAMPLER_2D);
    _finalPgm->GetOrCreateUniform("src3", glslUniform::SAMPLER_2D);
    _finalPgm->GetOrCreateUniform("camera", glslUniform::FLOAT_VEC4);

    _finalPgm->GetUniform("src1")->SetOneTimeApply(true);
    _finalPgm->GetUniform("src2")->SetOneTimeApply(true);
    _finalPgm->GetUniform("src3")->SetOneTimeApply(true);

    _finalPgm->GetOrCreateUniform("color", glslUniform::FLOAT_VEC4);
    _finalPgm->GetOrCreateUniform("lum", glslUniform::FLOAT);

    _finalPgm->GetUniform("color")->Set(Vec4f(0, 0, 0, 0));
    _finalPgm->GetUniform("lum")->Set(0.0f);

    _finalPgm->GetUniform("src1")->Set(0);
    _finalPgm->GetUniform("src2")->Set(1);
    _finalPgm->GetUniform("src3")->Set(2);
}
#endif

/*------------------------------------------------------------------
Function: _createBlurDeprecated
Description:
-------------------------------------------------------------------*/
void PostRenderFilter::_createBlurDeprecated()
{
    if(canUseTexture2D)
        _blurTex = new Texture2D;
    else
        _blurTex = new TextureRectangle;

    // Create render texture FBO
    if(canUseTexture2D)
    {
        Texture2D* tex = dynamic_cast<Texture2D*>(_blurTex.get());

        tex->setTextureSize(WantedWidth / 4, WantedHeight / 4);
        tex->setInternalFormat(GL_RGBA);
        tex->setSourceFormat(GL_RGBA);
        tex->setSourceType(GL_UNSIGNED_SHORT);
        tex->setFilter(Texture::MAG_FILTER, Texture::LINEAR);
        tex->setFilter(Texture::MIN_FILTER, Texture::NEAREST);

        blurfbo = new FrameBufferObject;
        blurfbo->setAttachment(FrameBufferObject::COLOR_BUFFER0, FrameBufferAttachment(tex));
    }
    else
    {
        TextureRectangle* tex = dynamic_cast<TextureRectangle*>(_blurTex.get());

        tex->setTextureSize(WantedWidth / 4, WantedHeight / 4);
        tex->setInternalFormat(GL_RGBA);
        tex->setSourceFormat(GL_RGBA);
        tex->setSourceType(GL_UNSIGNED_SHORT);
        tex->setFilter(Texture::MAG_FILTER, Texture::LINEAR);
        tex->setFilter(Texture::MIN_FILTER, Texture::NEAREST);

        blurfbo = new FrameBufferObject;

        blurfbo->setAttachment(FrameBufferObject::COLOR_BUFFER0, FrameBufferAttachment(tex));
    }

    // Create shaders for blur in x direction
    char* frag;

    if(canUseTexture2D)
        frag = "fx/shaders/GLSL/postrender/2D/blur.frag";
    else
        frag = "fx/shaders/GLSL/postrender/RECT/blur.frag";

    // Create blur program
    _blurPgm = GLSL.loadProgram("blur", nullptr, frag);
    _blurPgm->AddShader(_postvertex.get());

    _blurPgm->GetOrCreateUniform("offsets", glslUniform::FLOAT_VEC2, 16);
    _blurPgm->GetUniform("offsets")->SetOneTimeApply(true);

    Vec2f offsets[16];
    GetSampleOffsets_DownScale4x4(WantedWidth / 4, WantedHeight / 4, offsets);

    for(int i = 0; i < 16; i++)
    {
        _blurPgm->GetUniform("offsets")->SetElement(i, offsets[i]);
    }

    _blurPgm->GetOrCreateUniform("src1", glslUniform::SAMPLER_2D);
    _blurPgm->GetUniform("src1")->SetOneTimeApply(true);
    _blurPgm->GetUniform("src1")->Set(0);

#if USE_STATESET
    blur->SetRenderState(blurfbo.get());
    blur->SetRenderState(new Program(_blurPgm.get()));
    blur->SetRenderState(new TextureUnit(0, g_lensFlare.GetSunTexture()));
    blur->SetRenderState(new Viewport(0, 0, WantedWidth / 4, WantedHeight / 4));
#endif
}

/*------------------------------------------------------------------
Function: _createDownSizeDeprecated
Description:
-------------------------------------------------------------------*/
void PostRenderFilter::_createDownSizeDeprecated()
{
    if(canUseTexture2D)
        _downsizeTex = new Texture2D;
    else
        _downsizeTex = new TextureRectangle;

    if(canUseTexture2D)
    {
        Texture2D* tex = dynamic_cast<Texture2D*>(_downsizeTex.get());
        downsizefbo = new FrameBufferObject;
        downsizefbo->setAttachment(FrameBufferObject::COLOR_BUFFER0, FrameBufferAttachment(tex));
        tex->setTextureSize(WantedWidth / 2, WantedHeight / 2);
    }
    else
    {
        TextureRectangle* tex = dynamic_cast<TextureRectangle*>(_downsizeTex.get());
        tex->setTextureSize(WantedWidth / 2, WantedHeight / 2);
        downsizefbo = new FrameBufferObject;
        downsizefbo->setAttachment(FrameBufferObject::COLOR_BUFFER0, FrameBufferAttachment(tex));
    }

    _downsizeTex->setInternalFormat(_scenetexture->getInternalFormat());
    _downsizeTex->setSourceFormat(_scenetexture->getSourceFormat());
    _downsizeTex->setSourceType(_scenetexture->getSourceType());
    _downsizeTex->setFilter(Texture::MAG_FILTER, Texture::LINEAR);
    _downsizeTex->setFilter(Texture::MIN_FILTER, Texture::LINEAR);

    // Create shaders for downsizing
    char* frag;

    if(canUseTexture2D)
        frag = "fx/shaders/GLSL/postrender/2D/downsize.frag";
    else
        frag = "fx/shaders/GLSL/postrender/RECT/downsize.frag";

    // Create downsizing program
    _downsizePgm = GLSL.loadProgram("downsize", nullptr, frag);
    _downsizePgm->AddShader(_postvertex.get());

    // Now the uniform variables
    _downsizePgm->GetOrCreateUniform("src1", glslUniform::SAMPLER_2D);
    _downsizePgm->GetUniform("src1")->SetOneTimeApply(true);
    _downsizePgm->GetUniform("src1")->Set(0);

#if USE_STATESET
    downsize->SetRenderState(downsizefbo.get());
    downsize->SetRenderState(new Program(_downsizePgm.get()));
    downsize->SetRenderState(new TextureUnit(0, _scenetexture));
    downsize->SetRenderState(new Viewport(0, 0, WantedWidth / 2, WantedHeight / 2));
#endif
}

#if USE_STATESET
/*------------------------------------------------------------------
Function: _createDepthBlurDeprecated
Description:
-------------------------------------------------------------------*/
void PostRenderFilter::_createDepthBlurDeprecated()
{
    extern Texture* obdrawGetSceneDepthTexture();

    if(canUseTexture2D)
        _depthBlurTex = new Texture2D;
    else
        _depthBlurTex = new TextureRectangle;

    if(canUseTexture2D)
    {
        Texture2D* tex = dynamic_cast<Texture2D*>(_depthBlurTex.get());
        depthblurfbo = new FrameBufferObject;
        depthblurfbo->setAttachment(FrameBufferObject::COLOR_BUFFER0, FrameBufferAttachment(tex));
        tex->setTextureSize(WantedWidth, WantedHeight);
    }
    else
    {
        TextureRectangle* tex = dynamic_cast<TextureRectangle*>(_depthBlurTex.get());
        tex->setTextureSize(WantedWidth, WantedHeight);
        depthblurfbo = new FrameBufferObject;
        depthblurfbo->setAttachment(FrameBufferObject::COLOR_BUFFER0, FrameBufferAttachment(tex));
    }

    _depthBlurTex->setInternalFormat(_scenetexture->getInternalFormat());
    _depthBlurTex->setSourceFormat(_scenetexture->getSourceFormat());
    _depthBlurTex->setSourceType(_scenetexture->getSourceType());
    _depthBlurTex->setFilter(Texture::MAG_FILTER, Texture::LINEAR);
    _depthBlurTex->setFilter(Texture::MIN_FILTER, Texture::LINEAR);

    // Create shaders for downsizing
    char* frag;

    if(canUseTexture2D)
        frag = "fx/shaders/GLSL/postrender/2D/depthblur.frag";
    else
        frag = "fx/shaders/GLSL/postrender/RECT/depthblur.frag";

    // Create downsizing program
    _depthBlurPgm = GLSL.loadProgram("depthblur", nullptr, frag);
    _depthBlurPgm->AddShader(_postvertex.get());

    // Now the uniform variables
    _depthBlurPgm->GetOrCreateUniform("src1", glslUniform::SAMPLER_2D);
    _depthBlurPgm->GetUniform("src1")->SetOneTimeApply(true);
    _depthBlurPgm->GetUniform("src1")->Set(0);

    _depthBlurPgm->GetOrCreateUniform("src2", glslUniform::SAMPLER_2D);
    _depthBlurPgm->GetUniform("src2")->SetOneTimeApply(true);
    _depthBlurPgm->GetUniform("src2")->Set(1);

    _depthBlurPgm->GetOrCreateUniform("camera", glslUniform::FLOAT_VEC4);

    depthblur->SetRenderState(depthblurfbo.get());
    depthblur->SetRenderState(new Program(_depthBlurPgm.get()));
    depthblur->SetRenderState(new TextureUnit(0, _scenetexture));
    depthblur->SetRenderState(new TextureUnit(1, obdrawGetSceneDepthTexture()));
    depthblur->SetRenderState(new Viewport(0, 0, WantedWidth, WantedHeight));
}

/*------------------------------------------------------------------
Function: _createFilterDeprecated
Description:
-------------------------------------------------------------------*/
void PostRenderFilter::_createFilterDeprecated()
{
    if(canUseTexture2D)
        _filterTex = new Texture2D;
    else
        _filterTex = new TextureRectangle;

    int w = _downsizeTex->getTextureWidth();
    int h = _downsizeTex->getTextureHeight();

    if(canUseTexture2D)
    {
        Texture2D* tex = dynamic_cast<Texture2D*>(_filterTex.get());
        filterfbo = new FrameBufferObject;
        filterfbo->setAttachment(FrameBufferObject::COLOR_BUFFER0, FrameBufferAttachment(tex));
        tex->setTextureSize(w, h);
    }
    else
    {
        TextureRectangle* tex = dynamic_cast<TextureRectangle*>(_filterTex.get());
        tex->setTextureSize(w, h);
        filterfbo = new FrameBufferObject;
        filterfbo->setAttachment(FrameBufferObject::COLOR_BUFFER0, FrameBufferAttachment(tex));
    }

    _filterTex->setInternalFormat(_scenetexture->getInternalFormat());
    _filterTex->setSourceFormat(_scenetexture->getSourceFormat());
    _filterTex->setSourceType(_scenetexture->getSourceType());
    _filterTex->setFilter(Texture::MAG_FILTER, Texture::LINEAR);
    _filterTex->setFilter(Texture::MIN_FILTER, Texture::LINEAR);

    // Create shaders for downsizing
    char* frag;

    if(canUseTexture2D)
        frag = "fx/shaders/GLSL/postrender/2D/filter.frag";
    else
        frag = "fx/shaders/GLSL/postrender/RECT/filter.frag";

    // Create downsizing program
    _filterPgm = GLSL.loadProgram("filter", nullptr, frag);
    _filterPgm->AddShader(_postvertex.get());

    // Now the uniform variables
    _filterPgm->GetOrCreateUniform("src", glslUniform::SAMPLER_2D);
    _filterPgm->GetUniform("src")->SetOneTimeApply(true);
    _filterPgm->GetUniform("src")->Set(0);

    filter->SetRenderState(filterfbo.get());
    filter->SetRenderState(new Program(_filterPgm.get()));
    filter->SetRenderState(new TextureUnit(0, _downsizeTex.get()));
    filter->SetRenderState(new Viewport(0, 0, w, h));
}
#else
/*------------------------------------------------------------------
Function: _createFilterXDeprecated
Description:
-------------------------------------------------------------------*/
void PostRenderFilter::_createFilterXDeprecated()
{
    if(canUseTexture2D)
        _filterXTex = new Texture2D;
    else
        _filterXTex = new TextureRectangle;

    int w = _downsizeTex->getTextureWidth() / 2;
    int h = _downsizeTex->getTextureHeight() / 2;

    if(canUseTexture2D)
    {
        Texture2D* tex = dynamic_cast<Texture2D*>(_filterXTex.get());
        filterxfbo = new FrameBufferObject;
        filterxfbo->setAttachment(FrameBufferObject::COLOR_BUFFER0, FrameBufferAttachment(tex));
        tex->setTextureSize(w, h);
    }
    else
    {
        TextureRectangle* tex = dynamic_cast<TextureRectangle*>(_filterXTex.get());
        tex->setTextureSize(w, h);
        filterxfbo = new FrameBufferObject;
        filterxfbo->setAttachment(FrameBufferObject::COLOR_BUFFER0, FrameBufferAttachment(tex));
    }

    _filterXTex->setInternalFormat(_scenetexture->getInternalFormat());
    _filterXTex->setSourceFormat(_scenetexture->getSourceFormat());
    _filterXTex->setSourceType(_scenetexture->getSourceType());
    _filterXTex->setFilter(Texture::MAG_FILTER, Texture::LINEAR);
    _filterXTex->setFilter(Texture::MIN_FILTER, Texture::LINEAR);

    // Create shaders for downsizing
    char* frag;

    if(canUseTexture2D)
        frag = "fx/shaders/GLSL/postrender/2D/filterx.frag";
    else
        frag = "fx/shaders/GLSL/postrender/RECT/filterx.frag";

    // Create downsizing program
    _filterXPgm = GLSL.loadProgram("filterx", nullptr, frag);
    _filterXPgm->AddShader(_postvertex.get());

    // Now the uniform variables
    _filterXPgm->GetOrCreateUniform("src", glslUniform::SAMPLER_2D);
    _filterXPgm->GetUniform("src")->SetOneTimeApply(true);
    _filterXPgm->GetUniform("src")->Set(0);
}

/*------------------------------------------------------------------
Function: _createFilterYDeprecated
Description:
-------------------------------------------------------------------*/
void PostRenderFilter::_createFilterYDeprecated()
{
    if(canUseTexture2D)
        _filterYTex = new Texture2D;
    else
        _filterYTex = new TextureRectangle;

    int w = _filterXTex->getTextureWidth() / 2;
    int h = _filterXTex->getTextureHeight() / 2;

    if(canUseTexture2D)
    {
        Texture2D* tex = dynamic_cast<Texture2D*>(_filterYTex.get());
        filteryfbo = new FrameBufferObject;
        filteryfbo->setAttachment(FrameBufferObject::COLOR_BUFFER0, FrameBufferAttachment(tex));
        tex->setTextureSize(w, h);
    }
    else
    {
        TextureRectangle* tex = dynamic_cast<TextureRectangle*>(_filterYTex.get());
        tex->setTextureSize(w, h);
        filteryfbo = new FrameBufferObject;
        filteryfbo->setAttachment(FrameBufferObject::COLOR_BUFFER0, FrameBufferAttachment(tex));
    }

    _filterYTex->setInternalFormat(_scenetexture->getInternalFormat());
    _filterYTex->setSourceFormat(_scenetexture->getSourceFormat());
    _filterYTex->setSourceType(_scenetexture->getSourceType());
    _filterYTex->setFilter(Texture::MAG_FILTER, Texture::LINEAR);
    _filterYTex->setFilter(Texture::MIN_FILTER, Texture::LINEAR);

    // Create shaders for downsizing
    char* frag;

    if(canUseTexture2D)
        frag = "fx/shaders/GLSL/postrender/2D/filtery.frag";
    else
        frag = "fx/shaders/GLSL/postrender/RECT/filtery.frag";

    // Create downsizing program
    _filterYPgm = GLSL.loadProgram("filtery", nullptr, frag);
    _filterYPgm->AddShader(_postvertex.get());

    // Now the uniform variables
    _filterYPgm->GetOrCreateUniform("src", glslUniform::SAMPLER_2D);
    _filterYPgm->GetUniform("src")->SetOneTimeApply(true);
    _filterYPgm->GetUniform("src")->Set(0);
}
#endif

/*------------------------------------------------------------------
Function: _createFakeHDRDeprecated
Description:
-------------------------------------------------------------------*/
void PostRenderFilter::_createFakeHDRDeprecated()
{
    if(canUseTexture2D)
        _fakehdrTex = new Texture2D;
    else
        _fakehdrTex = new TextureRectangle;

    if(canUseTexture2D)
    {
        Texture2D* tex = dynamic_cast<Texture2D*>(_fakehdrTex.get());

        tex->setTextureSize(WantedWidth / 4, WantedHeight / 4);
        tex->setInternalFormat(GL_RGBA);
        tex->setSourceFormat(GL_RGBA);
        tex->setSourceType(GL_UNSIGNED_SHORT);
        tex->setFilter(Texture::MAG_FILTER, Texture::LINEAR);
        tex->setFilter(Texture::MIN_FILTER, Texture::LINEAR);

        fakehdrfbo = new FrameBufferObject;

        fakehdrfbo->setAttachment(FrameBufferObject::COLOR_BUFFER0, FrameBufferAttachment(tex));
    }
    else
    {
        TextureRectangle* tex = dynamic_cast<TextureRectangle*>(_fakehdrTex.get());

        tex->setTextureSize(WantedWidth / 4, WantedHeight / 4);
        tex->setInternalFormat(_scenetexture->getInternalFormat());
        tex->setSourceFormat(_scenetexture->getSourceFormat());
        tex->setSourceType(_scenetexture->getSourceType());
        tex->setFilter(Texture::MAG_FILTER, Texture::LINEAR);
        tex->setFilter(Texture::MIN_FILTER, Texture::LINEAR);

        fakehdrfbo = new FrameBufferObject;

        fakehdrfbo->setAttachment(FrameBufferObject::COLOR_BUFFER0, FrameBufferAttachment(tex));
    }

    // Create shaders for downsizing
    char* frag;

    if(canUseTexture2D)
        frag = "fx/shaders/GLSL/postrender/2D/fakehdr.frag";
    else
        frag = "fx/shaders/GLSL/postrender/RECT/fakehdr.frag";

    // Create downsizing program
    _fakehdrPgm = GLSL.loadProgram("fakehdr", nullptr, frag);
    _fakehdrPgm->AddShader(_postvertex.get());

    // Now the uniform variables
    _fakehdrPgm->GetOrCreateUniform("src1", glslUniform::SAMPLER_2D);
    _fakehdrPgm->GetUniform("src1")->SetOneTimeApply(true);
    _fakehdrPgm->GetUniform("src1")->Set(0);
}

/*------------------------------------------------------------------
Function: _createFinal
Description:
-------------------------------------------------------------------*/
void PostRenderFilter::_createFinal()
{
    _finalPgm = GLSL.loadProgram("final", nullptr, nullptr);

    if(canUseTexture2D)
    {
        _finalPgm->AddShader(GLSL.loadVertexShader("fx/shaders/GLSL+/postrender/2D/final.vert").get());
        _finalPgm->AddShader(GLSL.loadFragmentShader("fx/shaders/GLSL+/postrender/2D/final.frag").get());
    }
    else
    {
        _finalPgm->AddShader(GLSL.loadVertexShader("fx/shaders/GLSL+/postrender/RECT/final.vert").get());
        _finalPgm->AddShader(GLSL.loadFragmentShader("fx/shaders/GLSL+/postrender/RECT/final.frag").get());
    }

    _finalPgm->AddBindAttribLocation("in_pos", oglVertexBuffer::ATTRIBUTE_POSITION);
    _finalPgm->AddBindAttribLocation("in_tex0", oglVertexBuffer::ATTRIBUTE_TEXCOORD0);

    // Now the uniform variables
    _finalPgm->GetOrCreateUniform("src1", glslUniform::SAMPLER_2D);
    _finalPgm->GetOrCreateUniform("src2", glslUniform::SAMPLER_2D);
    _finalPgm->GetOrCreateUniform("src3", glslUniform::SAMPLER_2D);

    _finalPgm->GetUniform("src1")->SetOneTimeApply(true);
    _finalPgm->GetUniform("src2")->SetOneTimeApply(true);
    _finalPgm->GetUniform("src3")->SetOneTimeApply(true);;

    _finalPgm->GetOrCreateUniform("color", glslUniform::FLOAT_VEC4);
    _finalPgm->GetOrCreateUniform("lum", glslUniform::FLOAT);

    _finalPgm->GetUniform("color")->Set(Vec4f(0, 0, 0, 0));
    _finalPgm->GetUniform("lum")->Set(0.0f);

    _finalPgm->GetUniform("src1")->Set(0);
    _finalPgm->GetUniform("src2")->Set(1);
    _finalPgm->GetUniform("src3")->Set(2);

    _finalPgm->Apply();

#if USE_STATESET
    final->SetRenderState(new Program(_finalPgm.get()));
    final->SetRenderState(new TextureUnit(0, _scenetexture));
    final->SetRenderState(new TextureUnit(1, _blurTex.get()));
    final->SetRenderState(new TextureUnit(2, _filterTex.get()));
#endif
}

/*------------------------------------------------------------------
Function: _createBlur
Description:
-------------------------------------------------------------------*/
void PostRenderFilter::_createBlur()
{
    if(canUseTexture2D)
        _blurTex = new Texture2D;
    else
        _blurTex = new TextureRectangle;

    // Create render texture FBO
    if(canUseTexture2D)
    {
        Texture2D* tex = dynamic_cast<Texture2D*>(_blurTex.get());

        tex->setTextureSize(WantedWidth / 4, WantedHeight / 4);
        tex->setInternalFormat(GL_RGBA);
        tex->setSourceFormat(GL_RGBA);
        tex->setSourceType(GL_UNSIGNED_SHORT);
        tex->setFilter(Texture::MAG_FILTER, Texture::LINEAR);
        tex->setFilter(Texture::MIN_FILTER, Texture::NEAREST);

        blurfbo = new FrameBufferObject;
        blurfbo->setAttachment(FrameBufferObject::COLOR_BUFFER0, FrameBufferAttachment(tex));
    }
    else
    {
        TextureRectangle* tex = dynamic_cast<TextureRectangle*>(_blurTex.get());

        tex->setTextureSize(WantedWidth / 4, WantedHeight / 4);
        tex->setInternalFormat(GL_RGBA);
        tex->setSourceFormat(GL_RGBA);
        tex->setSourceType(GL_UNSIGNED_SHORT);
        tex->setFilter(Texture::MAG_FILTER, Texture::LINEAR);
        tex->setFilter(Texture::MIN_FILTER, Texture::NEAREST);

        blurfbo = new FrameBufferObject;

        blurfbo->setAttachment(FrameBufferObject::COLOR_BUFFER0, FrameBufferAttachment(tex));
    }

    _blurPgm = GLSL.loadProgram("blur", nullptr, nullptr);

    if(canUseTexture2D)
    {
        _blurPgm->AddShader(GLSL.loadVertexShader("fx/shaders/GLSL+/postrender/2D/blur.vert").get());
        _blurPgm->AddShader(GLSL.loadFragmentShader("fx/shaders/GLSL+/postrender/2D/blur.frag").get());
    }
    else
    {
        _blurPgm->AddShader(GLSL.loadVertexShader("fx/shaders/GLSL+/postrender/RECT/blur.vert").get());
        _blurPgm->AddShader(GLSL.loadFragmentShader("fx/shaders/GLSL+/postrender/RECT/blur.frag").get());
    }

    _blurPgm->AddBindAttribLocation("in_pos", oglVertexBuffer::ATTRIBUTE_POSITION);
    _blurPgm->AddBindAttribLocation("in_tex0", oglVertexBuffer::ATTRIBUTE_TEXCOORD0);

    // Now the uniform variables
    _blurPgm->GetOrCreateUniform("src", glslUniform::SAMPLER_2D);

    _blurPgm->GetUniform("src")->Set(0);
    _blurPgm->GetUniform("src")->SetOneTimeApply(true);

    _blurPgm->GetOrCreateUniform("offsets", glslUniform::FLOAT_VEC2, 16);
    _blurPgm->GetUniform("offsets")->SetOneTimeApply(true);

    Vec2f offsets[16];
    GetSampleOffsets_DownScale4x4(WantedWidth / 4, WantedHeight / 4, offsets);

    for(int i = 0; i < 16; i++)
    {
        _blurPgm->GetUniform("offsets")->SetElement(i, offsets[i]);
    }

    _blurPgm->Apply();

#if USE_STATESET
    blur->SetRenderState(blurfbo.get());
    blur->SetRenderState(new Program(_blurPgm.get()));
    blur->SetRenderState(new TextureUnit(0, g_lensFlare.GetSunTexture()));
#endif
}

/*------------------------------------------------------------------
Function: _createDownSize
Description:
-------------------------------------------------------------------*/
void PostRenderFilter::_createDownSize()
{
    if(canUseTexture2D)
        _downsizeTex = new Texture2D;
    else
        _downsizeTex = new TextureRectangle;

    if(canUseTexture2D)
    {
        Texture2D* tex = dynamic_cast<Texture2D*>(_downsizeTex.get());
        downsizefbo = new FrameBufferObject;
        downsizefbo->setAttachment(FrameBufferObject::COLOR_BUFFER0, FrameBufferAttachment(tex));
        tex->setTextureSize(WantedWidth / 2, WantedHeight / 2);
    }
    else
    {
        TextureRectangle* tex = dynamic_cast<TextureRectangle*>(_downsizeTex.get());
        tex->setTextureSize(WantedWidth / 2, WantedHeight / 2);
        downsizefbo = new FrameBufferObject;
        downsizefbo->setAttachment(FrameBufferObject::COLOR_BUFFER0, FrameBufferAttachment(tex));
    }

    _downsizeTex->setInternalFormat(_scenetexture->getInternalFormat());
    _downsizeTex->setSourceFormat(_scenetexture->getSourceFormat());
    _downsizeTex->setSourceType(_scenetexture->getSourceType());
    _downsizeTex->setFilter(Texture::MAG_FILTER, Texture::LINEAR);
    _downsizeTex->setFilter(Texture::MIN_FILTER, Texture::LINEAR);

    // Create downsizing program
    _downsizePgm = GLSL.loadProgram("downsize", nullptr, nullptr);

    if(canUseTexture2D)
    {
        _downsizePgm->AddShader(GLSL.loadVertexShader("fx/shaders/GLSL+/postrender/2D/downsize.vert").get());
        _downsizePgm->AddShader(GLSL.loadFragmentShader("fx/shaders/GLSL+/postrender/2D/downsize.frag").get());
    }
    else
    {
        _downsizePgm->AddShader(GLSL.loadVertexShader("fx/shaders/GLSL+/postrender/RECT/downsize.vert").get());
        _downsizePgm->AddShader(GLSL.loadFragmentShader("fx/shaders/GLSL+/postrender/RECT/downsize.frag").get());
    }

    _downsizePgm->AddBindAttribLocation("in_pos", oglVertexBuffer::ATTRIBUTE_POSITION);
    _downsizePgm->AddBindAttribLocation("in_tex0", oglVertexBuffer::ATTRIBUTE_TEXCOORD0);

    // Now the uniform variables
    _downsizePgm->GetOrCreateUniform("src1", glslUniform::SAMPLER_2D);
    _downsizePgm->GetUniform("src1")->Set(0);
    _downsizePgm->GetUniform("src1")->SetOneTimeApply(true);

    _downsizePgm->Apply();
#if USE_STATESET
    downsize->SetRenderState(downsizefbo.get());
    downsize->SetRenderState(new Program(_downsizePgm.get()));
    downsize->SetRenderState(new TextureUnit(0, _scenetexture));
#endif
}

#if USE_STATESET
#else
/*------------------------------------------------------------------
Function: _createFilterX
Description:
-------------------------------------------------------------------*/
void PostRenderFilter::_createFilterX()
{
    if(canUseTexture2D)
        _filterXTex = new Texture2D;
    else
        _filterXTex = new TextureRectangle;

    int w = _downsizeTex->getTextureWidth() / 2;
    int h = _downsizeTex->getTextureHeight() / 2;

    if(canUseTexture2D)
    {
        Texture2D* tex = dynamic_cast<Texture2D*>(_filterXTex.get());
        filterxfbo = new FrameBufferObject;
        filterxfbo->setAttachment(FrameBufferObject::COLOR_BUFFER0, FrameBufferAttachment(tex));
        tex->setTextureSize(w, h);
    }
    else
    {
        TextureRectangle* tex = dynamic_cast<TextureRectangle*>(_filterXTex.get());
        tex->setTextureSize(w, h);
        filterxfbo = new FrameBufferObject;
        filterxfbo->setAttachment(FrameBufferObject::COLOR_BUFFER0, FrameBufferAttachment(tex));
    }

    _filterXTex->setInternalFormat(_scenetexture->getInternalFormat());
    _filterXTex->setSourceFormat(_scenetexture->getSourceFormat());
    _filterXTex->setSourceType(_scenetexture->getSourceType());
    _filterXTex->setFilter(Texture::MAG_FILTER, Texture::LINEAR);
    _filterXTex->setFilter(Texture::MIN_FILTER, Texture::LINEAR);

    _filterXPgm = GLSL.loadProgram("filterx", nullptr, nullptr);

    if(canUseTexture2D)
    {
        _filterXPgm->AddShader(GLSL.loadVertexShader("fx/shaders/GLSL+/postrender/2D/filterx.vert").get());
        _filterXPgm->AddShader(GLSL.loadFragmentShader("fx/shaders/GLSL+/postrender/2D/filterx.frag").get());
    }
    else
    {
        _filterXPgm->AddShader(GLSL.loadVertexShader("fx/shaders/GLSL+/postrender/RECT/filterx.vert").get());
        _filterXPgm->AddShader(GLSL.loadFragmentShader("fx/shaders/GLSL+/postrender/RECT/filterx.frag").get());
    }

    _filterXPgm->AddBindAttribLocation("in_pos", oglVertexBuffer::ATTRIBUTE_POSITION);
    _filterXPgm->AddBindAttribLocation("in_tex0", oglVertexBuffer::ATTRIBUTE_TEXCOORD0);

    // Now the uniform variables
    _filterXPgm->GetOrCreateUniform("src", glslUniform::SAMPLER_2D);
    _filterXPgm->GetUniform("src")->Set(0);
    _filterXPgm->GetUniform("src")->SetOneTimeApply(true);

    _filterXPgm->GetOrCreateUniform("offsets", glslUniform::FLOAT_VEC2, 16);
    _filterXPgm->GetUniform("offsets")->SetOneTimeApply(true);

    Vec2f offsets[16];
    GetSampleOffsets_DownScale4x4(w, h, offsets);

    for(int i = 0; i < 16; i++)
    {
        _filterXPgm->GetUniform("offsets")->SetElement(i, offsets[i]);
    }

    _filterXPgm->Apply();
}

/*------------------------------------------------------------------
Function: _createFilterY
Description:
-------------------------------------------------------------------*/
void PostRenderFilter::_createFilterY()
{
    if(canUseTexture2D)
        _filterYTex = new Texture2D;
    else
        _filterYTex = new TextureRectangle;

    int w = _filterXTex->getTextureWidth() / 2;
    int h = _filterXTex->getTextureHeight() / 2;

    if(canUseTexture2D)
    {
        Texture2D* tex = dynamic_cast<Texture2D*>(_filterYTex.get());
        filteryfbo = new FrameBufferObject;
        filteryfbo->setAttachment(FrameBufferObject::COLOR_BUFFER0, FrameBufferAttachment(tex));
        tex->setTextureSize(w, h);
    }
    else
    {
        TextureRectangle* tex = dynamic_cast<TextureRectangle*>(_filterYTex.get());
        tex->setTextureSize(w, h);
        filteryfbo = new FrameBufferObject;
        filteryfbo->setAttachment(FrameBufferObject::COLOR_BUFFER0, FrameBufferAttachment(tex));
    }

    _filterYTex->setInternalFormat(_scenetexture->getInternalFormat());
    _filterYTex->setSourceFormat(_scenetexture->getSourceFormat());
    _filterYTex->setSourceType(_scenetexture->getSourceType());
    _filterYTex->setFilter(Texture::MAG_FILTER, Texture::LINEAR);
    _filterYTex->setFilter(Texture::MIN_FILTER, Texture::LINEAR);

    _filterYPgm = GLSL.loadProgram("filtery", nullptr, nullptr);

    if(canUseTexture2D)
    {
        _filterYPgm->AddShader(GLSL.loadVertexShader("fx/shaders/GLSL+/postrender/2D/filtery.vert").get());
        _filterYPgm->AddShader(GLSL.loadFragmentShader("fx/shaders/GLSL+/postrender/2D/filtery.frag").get());
    }
    else
    {
        _filterYPgm->AddShader(GLSL.loadVertexShader("fx/shaders/GLSL+/postrender/RECT/filtery.vert").get());
        _filterYPgm->AddShader(GLSL.loadFragmentShader("fx/shaders/GLSL+/postrender/RECT/filtery.frag").get());
    }

    _filterYPgm->AddBindAttribLocation("in_pos", oglVertexBuffer::ATTRIBUTE_POSITION);
    _filterYPgm->AddBindAttribLocation("in_tex0", oglVertexBuffer::ATTRIBUTE_TEXCOORD0);

    // Now the uniform variables
    _filterYPgm->GetOrCreateUniform("src", glslUniform::SAMPLER_2D);
    _filterYPgm->GetUniform("src")->Set(0);
    _filterYPgm->GetUniform("src")->SetOneTimeApply(true);

    _filterYPgm->GetOrCreateUniform("offsets", glslUniform::FLOAT_VEC2, 16);
    _filterYPgm->GetUniform("offsets")->SetOneTimeApply(true);

    Vec2f offsets[16];
    GetSampleOffsets_DownScale4x4(w, h, offsets);

    for(int i = 0; i < 16; i++)
    {
        _filterYPgm->GetUniform("offsets")->SetElement(i, offsets[i]);
    }

    _filterYPgm->Apply();
}
#endif

/*------------------------------------------------------------------
Function: _drawQuad
Description:
-------------------------------------------------------------------*/
void PostRenderFilter::_drawQuad(float x, float y) const
{
    piLVERTEX v[4];
    float u1, v1, u2, v2;

    u1 = 0.0f;
    v1 = 0.0f;
    u2 = x;
    v2 = y;

    float xx = 1.0f;
    float yy = 1.0f;

	v[0].x = -xx;
	v[0].y =  yy;
	v[0].z = zdepthLAYER0;
	v[0].color = colorWHITEALPHA;
	v[0].specular = colorALPHA100;
	v[0].tu = u1;
	v[0].tv = v2;

	v[1].x = xx;
	v[1].y = yy;
	v[1].z = zdepthLAYER0;
	v[1].color = colorWHITEALPHA;
	v[1].specular = colorALPHA100;
	v[1].tu = u2;
	v[1].tv = v2;

	v[2].x = xx;
	v[2].y = -yy;
	v[2].z = zdepthLAYER0;
	v[2].color = colorWHITEALPHA;
	v[2].specular = colorALPHA100;
	v[2].tu = u2;
	v[2].tv = v1;

	v[3].x = -xx;
	v[3].y = -yy;
	v[3].z = zdepthLAYER0;
	v[3].color = colorWHITEALPHA;
	v[3].specular = colorALPHA100;
	v[3].tu = u1;
	v[3].tv = v1;

    piINDICES indices[2];

    indices[0].v1 = 0;
    indices[0].v2 = 1;
    indices[0].v3 = 3;
    indices[1].v1 = 1;
    indices[1].v2 = 2;
    indices[1].v3 = 3;

    oglRenderIndexScene(oglPT_TRIANGLELIST,
                        oglVT_LVERTEX,
                        v,
                        4,
                        (unsigned short*)indices,
                        6);
}

#if USE_STATESET
/*------------------------------------------------------------------
Function: _applyDownsize
Description:
-------------------------------------------------------------------*/
void PostRenderFilter::_applyDownsize()
{
    logTimerBeginMacro();

    downsize->Apply();
    // Viewport has to be same size as FBO texture
    oglSetViewport(0, 0, _downsizeTex->getTextureWidth(), _downsizeTex->getTextureHeight());

    if(canUseTexture2D)
        _drawQuad(1.0f, 1.0f);
    else
        // TEXTURE_RECTANGLE is not normalized. W and H size of source texture
        _drawQuad(float(_scenetexture->getTextureWidth()), float(_scenetexture->getTextureHeight()));

    logTimerEndMacro();
}
#else
/*------------------------------------------------------------------
Function: _applyDownsize
Description:
-------------------------------------------------------------------*/
void PostRenderFilter::_applyDownsize()
{
    logTimerBeginMacro();

    downsizefbo->apply();

    oglSetActiveTextureStage(0);
    _scenetexture->apply();

    // Viewport has to be same size as FBO texture
    oglSetViewport(0, 0, _downsizeTex->getTextureWidth(), _downsizeTex->getTextureHeight());

    _downsizePgm->Apply();
    _downsizePgm->ApplyUniformMap();

    if(canUseTexture2D)
        _drawQuad(1.0f, 1.0f);
    else
        // TEXTURE_RECTANGLE is not normalized. W and H size of source texture
        _drawQuad(float(_scenetexture->getTextureWidth()), float(_scenetexture->getTextureHeight()));

    logTimerEndMacro();
}
#endif

#if USE_STATESET
/*------------------------------------------------------------------
Function: _applyFilter
Description:
-------------------------------------------------------------------*/
void PostRenderFilter::_applyFilter()
{
    logTimerBeginMacro();

    filter->Apply();
    // Viewport has to be same size as FBO texture
    oglSetViewport(0, 0, _filterTex->getTextureWidth(), _filterTex->getTextureHeight());

    if(canUseTexture2D)
        _drawQuad(1.0f, 1.0f);
    else
        // TEXTURE_RECTANGLE is not normalized. W and H size of source texture
        _drawQuad(float(_downsizeTex->getTextureWidth()), float(_downsizeTex->getTextureHeight()));

    logTimerEndMacro();
}
#else
/*------------------------------------------------------------------
Function: _applyFilterX
Description:
-------------------------------------------------------------------*/
void PostRenderFilter::_applyFilterX()
{
    logTimerBeginMacro();

    filterxfbo->apply();

    oglSetActiveTextureStage(0);
    _downsizeTex->apply(); //<-- source

    // Viewport has to be same size as FBO texture
    oglSetViewport(0, 0, _filterXTex->getTextureWidth(), _filterXTex->getTextureHeight());

    _filterXPgm->Apply();
    _filterXPgm->ApplyUniformMap();

    if(canUseTexture2D)
        _drawQuad(1.0f, 1.0f);
    else
        // TEXTURE_RECTANGLE is not normalized. W and H size of source texture
        _drawQuad(float(_downsizeTex->getTextureWidth()), float(_downsizeTex->getTextureHeight()));

    logTimerEndMacro();
}

/*------------------------------------------------------------------
Function: _applyFilterY
Description:
-------------------------------------------------------------------*/
void PostRenderFilter::_applyFilterY()
{
    logTimerBeginMacro();

    filteryfbo->apply();

    oglSetActiveTextureStage(0);
    _filterXTex->apply(); //<-- source

    // Viewport has to be same size as FBO texture
    oglSetViewport(0, 0, _filterYTex->getTextureWidth(), _filterYTex->getTextureHeight());

    _filterYPgm->Apply();
    _filterYPgm->ApplyUniformMap();

    if(canUseTexture2D)
        _drawQuad(1.0f, 1.0f);
    else
        // TEXTURE_RECTANGLE is not normalized. W and H size of source texture
        _drawQuad(float(_filterXTex->getTextureWidth()), float(_filterXTex->getTextureHeight()));

    logTimerEndMacro();
}
#endif

/*------------------------------------------------------------------
Function: _applyFakeHDR
Description:
-------------------------------------------------------------------*/
void PostRenderFilter::_applyFakeHDR()
{
    logTimerBeginMacro();

    fakehdrfbo->apply();

    oglSetActiveTextureStage(0);
    _downsizeTex->apply(); //<-- source texture

    // Viewport has to be same size as FBO texture
    oglSetViewport(0, 0, _fakehdrTex->getTextureWidth(), _fakehdrTex->getTextureHeight());

    _fakehdrPgm->Apply();
    _fakehdrPgm->ApplyUniformMap();

    if(canUseTexture2D)
        _drawQuad(1.0f, 1.0f);
    else
        // TEXTURE_RECTANGLE is not normalized. W and H size of source texture
        _drawQuad(float(_downsizeTex->getTextureWidth()), float(_downsizeTex->getTextureHeight()));

    logTimerEndMacro();
}

#if USE_STATESET
/*------------------------------------------------------------------
Function: _applyBlur
Description:
-------------------------------------------------------------------*/
void PostRenderFilter::_applyBlur()
{
    logTimerBeginMacro();

    blur->Apply();

    if(canUseTexture2D)
        _drawQuad(1.0f, 1.0f);
    else
    {
        // TEXTURE_RECTANGLE is not normalized. W and H size of source texture
        _drawQuad(float(g_lensFlare.GetSunTexture()->getTextureWidth()), float(g_lensFlare.GetSunTexture()->getTextureHeight()));
    }

    logTimerEndMacro();
}
#else
/*------------------------------------------------------------------
Function: _applyBlur
Description:
-------------------------------------------------------------------*/
void PostRenderFilter::_applyBlur()
{
    logTimerBeginMacro();

    Texture* src = g_lensFlare.GetSunTexture();

    if(!src)
        src = _fakehdrTex.get();

    // Target FBO
    blurfbo->apply();
    // Viewport must match target texture size
    oglSetViewport(0, 0, _blurTex->getTextureWidth(), _blurTex->getTextureHeight()); //<-- fbo size must match viewport

    oglSetActiveTextureStage(0);
    src->apply(); //<-- source texture

    // Shader that reduces size and applies a blur
    _blurPgm->Apply();
    _blurPgm->ApplyUniformMap();

    if(canUseTexture2D)
        _drawQuad(1.0f, 1.0f);
    else
        // TEXTURE_RECTANGLE is not normalized. W and H size of source texture
        _drawQuad(float(src->getTextureWidth()), float(src->getTextureHeight()));

    logTimerEndMacro();
}
#endif

/*------------------------------------------------------------------
Function: _applyAdaption
Description:
-------------------------------------------------------------------*/
void PostRenderFilter::_applyAdaption()
{
    logTimerBeginMacro();

    static int lasttime = 0;

    if(lasttime == 0)
        lasttime = frametime;

    float swap = lastluminance;
    lastluminance = currentluminance;
    currentluminance = swap;

    float time = float(frametime - lasttime) * 0.001f;

    static float value = 0.75f;
    static float power = 30.0f;

    float t = (1.0f - powf(value, power * time));

    // Gradually adjust brightness to simulate looking into/away from the sun
    currentluminance = lastluminance + (g_lensFlare.getAfterBright() - lastluminance) * t;

    lasttime = frametime;

    logTimerEndMacro();
}

#if USE_STATESET
#include "physics/colliderQuery.h"
#include "physics/colliderMain.h"
#include "physics/physicsMain.h"
#include "trn/trnvars.h"
#include "trn/trncd.h"
#include "pers/persmain.h"
#include "ml/mlutility.h"

/*------------------------------------------------------------------
Function: _applyFinal
Description:
-------------------------------------------------------------------*/
void PostRenderFilter::_applyFinal()
{
    logTimerBeginMacro();

    glslProgram* pgm = dynamic_cast<Program*>(final->GetRenderState(RS_GLSLPROGRAM))->GetProgram();
    glslUniform* focal = pgm->GetUniform("focalplanedepth");

#if EVERYTHING_USING_ODE
    static colliderQuery collider;
    static bool firsttime = true;

    if(firsttime)
    {
        firsttime = false;
        collider.addGeomToCollideWith((dGeomID)colliderWorldSpace());
        dGeomID ray = dCreateRay(collider.space(), 10000.0f);
        dGeomRaySet(ray, 0, 0, 0, 0, 0, -1); // Updated when we collide
        collider.addGeom(ray, 0, 0);
        colliderSetBits(ray, 0, COLLIDER_ALL);
    }

    Vec3d pos = Vec3d(playpos) + WorldOriginOffset;

    pos = physicsGameToPhysical(pos);

    pos.z() += 0.1f;

    colliderQuery::dGeomList& geoms = collider.geoms();
    dGeomRaySet(geoms[0], pos.x(), pos.y(), pos.z(), playdir.x, playdir.y, -playdir.z);

    collider.setDirty(true);

    if(!collider.findCollisions(true))
    {
        focal->Set(oglGetFieldOfViewZMax());
    }
    else
    {
        colliderQuery::CollisionList& c = collider.getCollisions();

        dGeomID current = nullptr;

        for(unsigned int i = 0; i < c.size(); i++)
        {
            if(current == c[i].g2)
                continue;

            current = c[i].g2;

			// don't use blocks
			if(colliderIsType(c[i].g2, COLLIDER_BLOCK))
				continue;

            if(colliderIsType(c[i].g2, COLLIDER_ALL))
            {
                focal->Set((float)c[i].depth);
                break;
            }
        }
    }
#elif(0)
	mlVehicleData* data = persGetPersonaModelDataPtr();
    FASTCOLLIDE collidelist[32];
    FASTCOLLIDE* list = collidelist;
    unsigned int CollisionSlotsLeft = 32;

    piVECTOR OldPos, NewPos;

    Vec3f oldpos(playpos);
    oldpos = -oldpos;

    Vec3f newpos = oldpos + Vec3f(playdir) * 10000.0f;

    oldpos.get(OldPos);
    newpos.get(NewPos);

    trncdCollideWithListOfTerrainObjects(&list,
                                         &CollisionSlotsLeft,
                                         data->MedTrnList,
                                         ML_NUM_MED_LIST_ELEMENTS,
                                         &OldPos,
                                         &NewPos,
                                         0);

    if(32 - CollisionSlotsLeft > 0)
    {
        mlUtilitySortCollideList(collidelist, 32 - CollisionSlotsLeft);

        float dist = (oldpos - newpos).length() * collidelist[0].CollideTime;
        focal->Set(dist);
    }
    else
    {
        focal->Set(oglGetFieldOfViewZMax());
    }
#endif

    glslUniform* u;

    u = pgm->GetUniform("color");
    u->Set(color);

    static float damper = 0.75f;

    u = pgm->GetUniform("lum");
    u->Set(currentluminance * damper);

    final->Apply();

    if(canUseTexture2D)
        _drawQuad(1.0f, 1.0f);
    else
        // TEXTURE_RECTANGLE not normalized
        _drawQuad(float(_scenetexture->getTextureWidth()), float(_scenetexture->getTextureHeight()));

    logTimerEndMacro();
}

/*------------------------------------------------------------------
Function: _applyDepthBlur
Description:
-------------------------------------------------------------------*/
void PostRenderFilter::_applyDepthBlur()
{
    glslUniform* u = _depthBlurPgm->GetUniform("camera");

    // Set shader options
    Vec4f cam;

    cam.x() = FrontClip;
    cam.y() = oglGetFieldOfViewZMax();
    cam.z() = cam.y() * cam.x();
    cam.w() = cam.y() - cam.x();

    u->Set(cam);

    depthblur->Apply();

    if(canUseTexture2D)
        _drawQuad(1.0f, 1.0f);
    else
        // TEXTURE_RECTANGLE not normalized
        _drawQuad(float(_depthBlurTex->getTextureWidth()), float(_depthBlurTex->getTextureHeight()));
}

#else
/*------------------------------------------------------------------
Function: _applyFinal
Description:
-------------------------------------------------------------------*/
void PostRenderFilter::_applyFinal()
{
    extern Texture* obdrawGetSceneDepthTexture();

    logTimerBeginMacro();

    oglSetActiveTextureStage(0);
    _scenetexture->apply();

    oglSetActiveTextureStage(1);
    _blurTex->apply();

    oglSetActiveTextureStage(2);
    _filterYTex->apply();

    _finalPgm->Apply();

    _finalPgm->GetUniform("color")->Set(color);

    static float damper = 0.75f;
    _finalPgm->GetUniform("lum")->Set(currentluminance * damper);

    _finalPgm->ApplyUniformMap();

    if(canUseTexture2D)
        _drawQuad(1.0f, 1.0f);
    else
        // TEXTURE_RECTANGLE not normalized
        _drawQuad(float(_scenetexture->getTextureWidth()), float(_scenetexture->getTextureHeight()));

    logTimerEndMacro();
}
#endif

#if USE_STATESET
/*------------------------------------------------------------------
Function: drawDeprecated
Description:
-------------------------------------------------------------------*/
void PostRenderFilter::drawDeprecated()
{
    logTimerBeginMacro();

    oglSetWorldMatrix(&IdentityMatrix);
    oglSetViewMatrix(&IdentityMatrix);
    oglSetProjectionMatrix(&IdentityMatrix);

    _applyBlur();
    _applyAdaption();
    _applyDepthBlur();
    _applyFinal();

    color.set(0, 0, 0, 0);

    logTimerEndMacro();
}
#else
/*------------------------------------------------------------------
Function: drawDeprecated
Description:
-------------------------------------------------------------------*/
void PostRenderFilter::drawDeprecated()
{
    logTimerBeginMacro();

    oglDisableTexture();
    oglPushRenderState(oglRENDERSTATE_ALPHABLENDENABLE);
    oglPushRenderState(oglRENDERSTATE_ALPHATESTENABLE);
    oglPushRenderState(oglRENDERSTATE_LIGHTING);
    oglPushRenderState(oglRENDERSTATE_ZENABLE);

    // Apply globals
    oglSetRenderState(oglRENDERSTATE_ALPHABLENDENABLE, 0);
    oglSetRenderState(oglRENDERSTATE_ALPHATESTENABLE, 0);
    oglSetRenderState(oglRENDERSTATE_LIGHTING, 0);
    oglSetRenderState(oglRENDERSTATE_ZENABLE, 0);

    oglSetMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    oglSetMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();

    _applyDownsize();
    _applyFilterX();
    _applyFilterY();

#if(0)
    _applyFakeHDR();
#endif

    _applyBlur();
    _applyAdaption();

    glBindFramebufferEXT(FrameBufferObject::READ_DRAW_FRAMEBUFFER, 0);
    oglSetViewport(0, 0, WantedWidth, WantedHeight);

    _applyFinal();

    ProgramState::Bind(0);

    glPopMatrix(); // Projection
    oglSetMatrixMode(GL_MODELVIEW);
    glPopMatrix();

    oglPopRenderState(oglRENDERSTATE_ALPHABLENDENABLE);
    oglPopRenderState(oglRENDERSTATE_ALPHATESTENABLE);
    oglPopRenderState(oglRENDERSTATE_LIGHTING);
    oglPopRenderState(oglRENDERSTATE_ZENABLE);

    color.set(0, 0, 0, 0);

    logTimerEndMacro();
}
#endif

#if USE_STATESET
/*------------------------------------------------------------------
Function: draw
Description:
-------------------------------------------------------------------*/
void PostRenderFilter::draw()
{
    logTimerBeginMacro();

    oglSetWorldMatrix(&IdentityMatrix);
    oglSetViewMatrix(&IdentityMatrix);
    oglSetProjectionMatrix(&IdentityMatrix);

    _applyDownsize();
    _applyFilter();
    _applyBlur();
    _applyAdaption();
    _applyFinal();

    color.set(0, 0, 0, 0);

    logTimerEndMacro();
}
#else
/*------------------------------------------------------------------
Function: draw
Description:
-------------------------------------------------------------------*/
void PostRenderFilter::draw()
{
    logTimerBeginMacro();

    oglDisableTexture();

    // Apply globals
    oglSetRenderState(oglRENDERSTATE_ALPHABLENDENABLE, 0);
    oglSetRenderState(oglRENDERSTATE_ALPHATESTENABLE, 0);
    oglSetRenderState(oglRENDERSTATE_ZENABLE, 0);

    piMATRIX oldview = *oglGetViewMatrix();
    piMATRIX oldproj = *oglGetProjectionMatrix();

    oglSetWorldMatrix(&IdentityMatrix);
    oglSetViewMatrix(&IdentityMatrix);
    oglSetProjectionMatrix(&IdentityMatrix);

    _applyDownsize();
    _applyFilterX();
    _applyFilterY();

#if(0)
    _applyFakeHDR();
#endif

    _applyBlur();
    _applyAdaption();

    glBindFramebufferEXT(FrameBufferObject::READ_DRAW_FRAMEBUFFER, 0);
    oglSetViewport(0, 0, WantedWidth, WantedHeight);

    _applyFinal();

    ProgramState::Bind(0);

    color.set(0, 0, 0, 0);

    oglSetProjectionMatrix(&oldproj);
    oglSetViewMatrix(&oldview);

    logTimerEndMacro();
}
#endif

/*------------------------------------------------------------------
Function: kill
Description:
-------------------------------------------------------------------*/
void PostRenderFilter::kill()
{
    if(_scenetexture.valid())
    {
        while(_scenetexture->referenceCount() > 1)
            _scenetexture->unref();

        _scenetexture = nullptr;
    }
}
