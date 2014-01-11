#pragma once

#include "ogl/ogl.h"
#include "osgImport/Referenced.h"
#include "osgImport/ref_ptr.h"
#include "osgImport/Texture.h"
#include "shader/glsl.h"

#if USE_STATESET

#define IsTextureMode(a) \
    a == GL_TEXTURE_1D || \
    a == GL_TEXTURE_2D || \
    a == GL_TEXTURE_3D || \
    a == GL_TEXTURE_CUBE_MAP || \
    a == GL_TEXTURE_RECTANGLE_ARB || \
    a == GL_TEXTURE_2D_ARRAY_EXT || \
    a == GL_TEXTURE_GEN_Q || \
    a == GL_TEXTURE_GEN_R || \
    a == GL_TEXTURE_GEN_S || \
    a == GL_TEXTURE_GEN_T

class Matrixf;
class RenderStateSet;
class RenderState;

enum
{
    RS_NONE,
    RS_GLSLPROGRAM,
    RS_MATERIAL,
    RS_LIGHTMODEL,
    RS_FOG,
    RS_POLYGONOFFSET,
    RS_ALPHAFUNC,
    RS_BLENDFUNC,
    RS_COLORMASK,
    RS_CULLFACE,
    RS_FRONTFACE,
    RS_DEPTHFUNC,
    RS_CLEARDEPTH,
    RS_DEPTHMASK,
    RS_POLYGONMODE,
    RS_SHADEMODEL,
    RS_BLENDEQUATION,
    RS_SAMPLECOVERAGE,
    RS_PIXELTRANSFER,
    RS_HINT,
    RS_LOGICOP,
    RS_DEPTHRANGE,
    RS_LINEWIDTH,
    RS_POINTSIZE,
    RS_POLYGONSTIPPLE,
    RS_LINESTIPPLE,
    RS_POINTPARAMETER,
    RS_STENCILFUNC,
    RS_STENCILOP,
    RS_STENCILMASK,
    RS_BLENDCOLOR,
    RS_VIEWPORT,
    RS_FRAMEBUFFEROBJECT,
    RS_COLOR4F,
    RS_DRAWBUFFER,
    RS_READBUFFER,
    RS_TEXENV0,
    RS_TEXENV1,
    RS_TEXENV2,
    RS_TEXENV3,
    RS_TEXENV4,
    RS_TEXENV5,
    RS_TEXENV6,
    RS_TEXGEN0,
    RS_TEXGEN1,
    RS_TEXGEN2,
    RS_TEXGEN3,
    RS_TEXGEN4,
    RS_TEXGEN5,
    RS_TEXGEN6,
    RS_TEXTURE_UNIT0,
    RS_TEXTURE_UNIT1,
    RS_TEXTURE_UNIT2,
    RS_TEXTURE_UNIT3,
    RS_TEXTURE_UNIT4,
    RS_TEXTURE_UNIT5,
    RS_TEXTURE_UNIT6,
    RS_TEXTURE_MATRIX0,
    RS_TEXTURE_MATRIX1,
    RS_TEXTURE_MATRIX2,
    RS_TEXTURE_MATRIX3,
    RS_TEXTURE_MATRIX4,
    RS_TEXTURE_MATRIX5,
    RS_TEXTURE_MATRIX6,
    RS_COUNT
};

struct ModeStack
{
    typedef std::vector<unsigned int> ValueVec;

    ModeStack() :
        valid(true),
        changed(false),
        last_applied_value(false),
        global_default_value(false),
        locked(false)
    {
    }

    bool valid;
    bool changed;
    bool last_applied_value;
    bool global_default_value;
    bool locked;

    ValueVec valueVec;
};

typedef std::map<GLenum, ModeStack> ModeMap;

namespace States
{
    void SetGlobalDefaultModeValue(GLenum mode, bool enabled);
    void InitDefaultAttributes();
    void ApplyRenderMode(GLenum mode, unsigned int value);
    void HaveAppliedMode(GLenum mode, int value);
    void ApplyRenderStates(const RenderStateSet* cur);
    void ApplyRenderModes(RenderStateSet* cur);
    void HaveAppliedAttribute(int state);
    void LockState(int state, bool lock);
    void LockMode(GLenum mode, bool lock);

    bool IsModeEnabled(GLenum mode);

    RenderState* GetDefaultAttribute(int attrib);
}

class RenderState : public Referenced
{
public:
    RenderState() :
        enabled(true),
        dirty(false)
    {
    }

    virtual ~RenderState() {}

    virtual void apply() const = 0;
    virtual int type() const = 0;

    void SetEnabled(bool enable) {enabled = enable;}
    void SetDirty(bool f) {dirty = f;}

    const bool IsEnabled() const {return enabled;}
    const bool IsDirty() const {return dirty;}

protected:
    bool enabled;
    bool dirty;
};

class RenderStateSet : public Referenced
{
public:
    RenderStateSet() :
      InPool(false)
    {
    }

    virtual ~RenderStateSet()
    {
        RenderStates.clear();
    }

    const RenderState* GetRenderState(int i) const
    {
        if(i >= (int)RenderStates.size())
            return nullptr;

        return RenderStates[i].get();
    }

    RenderState* GetRenderState(int type)
    {
        for(unsigned int i = 0; i < RenderStates.size(); i++)
        {
            if(RenderStates[i]->type() == type)
            {
                return RenderStates[i].get();
            }
        }

        return nullptr;
    }

    unsigned int GetRenderStateSize() const
    {
        return RenderStates.size();
    }

    void RemoveRenderState(int type)
    {
        for(unsigned int i = 0; i < RenderStates.size(); i++)
        {
            if(RenderStates[i]->type() == type)
            {
                RenderStates.erase(RenderStates.begin() + i);
                return;
            }
        }
    }

    void SetRenderState(RenderState* rs)
    {
        if(rs)
        {
            for(unsigned int i = 0; i < RenderStates.size(); i++)
            {
                if(RenderStates[i]->type() == rs->type())
                {
                    RenderStates[i] = rs;
                    return;
                }
            }

            RenderStates.push_back(rs);
        }
    }

    void Apply()
    {
        States::ApplyRenderModes(this);
        States::ApplyRenderStates(this);
    }

    void SetIsPool(bool f)
    {
        InPool = f;
    }

    typedef std::map<GLenum, int> ModeList;

    ModeList& GetModes() {return modes;}

    void SetMode(GLenum mode, int on)
    {
        modes[mode] = on;
    }

    bool IsPool() {return InPool;}

protected:
    bool InPool;
    std::vector<ref_ptr<RenderState> > RenderStates;
    ModeList modes;
};

class TextureStates: public RenderState
{
public:
    TextureStates(int unit = 0) :
      texunit(unit)
    {
    }

    virtual void apply() const
    {
    }

    virtual int type() const
    {
        return RS_NONE;
    }

    int GetTextureUnit() const {return texunit;}
    void SetUnitIndex(int unit_index) {texunit = unit_index;}

protected:
    int texunit;
};

class TextureUnit : public TextureStates
{
public:
    TextureUnit(int unit = 0) :
        TextureStates(unit),
        id(0),
        target(GL_TEXTURE_2D)
    {
    }

    TextureUnit(int unit, int texid) :
        TextureStates(unit)
    {
        SetTexture(texid);
    }

    TextureUnit(int unit, GLenum tgt, unsigned int GLid) :
        TextureStates(unit)
    {
        id = GLid;
        target = tgt;
    }

    TextureUnit(int unit, Texture* t) :
        TextureStates(unit)
    {
        SetTexture(t);
    }

    virtual int type() const {return RS_TEXTURE_UNIT0 + texunit;}
    virtual void apply() const;

    void SetTexture(Texture* t)
    {
        if(t)
        {
            if(!t->getTextureObject(0))
                t->apply();

            id = t->getTextureObject(0)->_id;
            target = t->getTextureTarget();
        }
    }

    void SetTexture(int ww2texid)
    {
        extern TEXTURE* textures;

        if(ww2texid > -1)
        {
            if(!textures[ww2texid].hdevtex)
            {
                oglGetTextureHandle(ww2texid, 0);
            }

            id = textures[ww2texid].genlist;
            target = textures[ww2texid].target;
        }
        else
        {
            id = 0;
            target = 0;
        }

        dirty = true;
    }

    void SetTexture(int GLtexID, GLenum tgt)
    {
        if(GLtexID > 0)
        {
            id = GLtexID;
            target = tgt;
        }
        else
        {
            id = 0;
            target = 0;
        }
    }

    unsigned int GetTexture()
    {
        return id;
    }

    GLenum GetTextureTarget()
    {
        return target;
    }

    bool HasTexture() const
    {
        return id > 0;
    }

protected:
    unsigned int id;
    GLenum target;
};

class TextureMatrix : public TextureStates
{
public:
    TextureMatrix(int texunit)
    {
        TextureUnit = texunit;
        UseCameraRotationInverse = false;
    }

    TextureMatrix(int texunit, const Matrixf& m)
    {
        TextureUnit = texunit;
        UseCameraRotationInverse = false;
        matrix = m;
    }

    virtual int type() const
    {
        return RS_TEXTURE_MATRIX0 + TextureUnit;
    }

    virtual void apply() const;
    const Matrixf& Matrix() const {return matrix;}
    const TextureMatrix& SetMatrix(const Matrixf& m)
    {
        matrix = m;
        return *this;
    }

    void SetUseCameraRotationInverse(bool use) {UseCameraRotationInverse = use;}
    bool CameraRotationInverse() const {return UseCameraRotationInverse;}

protected:
    int TextureUnit;
    Matrixf matrix;
    bool UseCameraRotationInverse;
};

class Program : public RenderState
{
public:
    Program() :
        program(nullptr)
    {
    }

    Program(const std::string& pgm)
    {
        program = GLSL.getProgram(pgm.c_str());
    }

    Program(glslProgram* pgm)
    {
        program = pgm;
    }

    void SetShader(const std::string& pgm)
    {
        program = GLSL.getProgram(pgm.c_str());
    }

    virtual int type() const
    {
        return RS_GLSLPROGRAM;
    }

    glslProgram* GetProgram() {return program.get();}

    virtual void apply() const;

protected:
    ref_ptr<glslProgram> program;
};

class Material : public RenderState
{
public:
    Material();
    Material(int id);

    virtual ~Material() {}

    virtual int type() const {return RS_MATERIAL;}
    virtual void apply() const;

    void SetMaterialID(int id);
    void SetTransparency(float alpha);
    void SetFrontTransparency(float alpha);
    void SetBackTransparency(float alpha);
    void SetFrontFlatColor(const Vec4f& color);
    void SetBackFlatColor(const Vec4f& color);
    void SetFlatColor(const Vec4f& color);

    void SetAmbient(Vec4f color)
    {
        FrontAmbient = color;
        BackAmbient = color;
    }

    void SetDiffuse(Vec4f color)
    {
        FrontDiffuse = color;
        BackDiffuse = color;
    }

    void SetSpecular(Vec4f color)
    {
        FrontSpecular = color;
        BackSpecular = color;
    }

    void SetEmission(Vec4f color)
    {
        FrontEmission = color;
        BackEmission = color;
    }

    void SetShininess(float shininess)
    {
        FrontShininess = shininess;
        BackShininess = shininess;
    }

    void SetFrontAmbient(Vec4f color) {FrontAmbient = color;}
    void SetFrontDiffuse(Vec4f color) {FrontDiffuse = color;}
    void SetFrontSpecular(Vec4f color) {FrontSpecular = color;}
    void SetFrontEmission(Vec4f color) {FrontEmission = color;}
    void SetFrontShininess(float shininess) {FrontShininess=shininess;}

    Vec4f GetFrontAmbient() const {return FrontAmbient;}
    Vec4f GetFrontDiffuse() const {return FrontDiffuse;}
    Vec4f GetFrontSpecular() const {return FrontSpecular;}
    Vec4f GetFrontEmission() const {return FrontEmission;}
    float GetFrontShininess() const {return FrontShininess;}

    void SetBackAmbient(Vec4f color) {BackAmbient = color;}
    void SetBackDiffuse(Vec4f color) {BackDiffuse = color;}
    void SetBackSpecular(Vec4f color) {BackSpecular = color;}
    void SetBackEmission(Vec4f color) {BackEmission = color;}
    void SetBackShininess(float shininess) {BackShininess = shininess;}

    Vec4f GetBackAmbient() const {return BackAmbient;}
    Vec4f GetBackDiffuse() const {return BackDiffuse;}
    Vec4f GetBackSpecular() const {return BackSpecular;}
    Vec4f GetBackEmission() const {return BackEmission;}
    float GetBackShininess() const {return BackShininess;}

    void SetColorMaterial(GLenum face, GLenum color)
    {
        ColorMaterialFace = face;
        ColorMaterial = color;
    }

    GLenum GetColorMaterialFace() const {return ColorMaterialFace;}
    GLenum GetColorMaterial() const {return ColorMaterial;}
    void SetColorMaterialEnabled(bool enabled) {ColorMaterialEnabled = enabled;}
    bool GetColorMaterialEnabled() const {return ColorMaterialEnabled;}

protected:
    Vec4f FrontAmbient;
    Vec4f FrontDiffuse;
    Vec4f FrontSpecular;
    Vec4f FrontEmission;
    Vec4f BackAmbient;
    Vec4f BackDiffuse;
    Vec4f BackSpecular;
    Vec4f BackEmission;
    float BackShininess;
    float FrontShininess;

    int matid;

    // color material
    GLenum ColorMaterialFace;
    GLenum ColorMaterial;
    bool ColorMaterialEnabled;
};

class LightModel : public RenderState
{
public:
    LightModel() :
        AmbientColor(0.2f, 0.2f, 0.2f, 1.0f),
        ColorControl(GL_SINGLE_COLOR),
        LocalViewer(false),
        TwoSide(false)
    {
    }

    virtual int type() const {return RS_LIGHTMODEL;}

    virtual void apply() const;

    void SetLocalViewer(bool localviewer) {LocalViewer = localviewer;}
    void SetTwoSide(bool twoside) {TwoSide = twoside;}
    void SetColorControl(GLenum colorcontrol) {ColorControl = colorcontrol;}
    void SetAmbientColor(Vec4f ambientcolor) {AmbientColor = ambientcolor;}
    bool GetLocalViewer() const {return LocalViewer;}
    bool GetTwoSide() const {return TwoSide;}
    GLenum GetColorControl() const {return ColorControl;}
    Vec4f GetAmbientColor() const {return AmbientColor;}

protected:
    Vec4f AmbientColor;
    GLenum ColorControl;
    bool LocalViewer;
    bool TwoSide;
};

class Fog : public RenderState
{
public:
    Fog(GLenum mode = GL_LINEAR, Vec4f color = Vec4f(0, 0, 0, 0), float density = 1, float start = 0, float end = 1) :
        Color(color),
        Mode(mode),
        Density(density),
        Start(start),
        End(end)
    {
    }

    virtual int type() const {return RS_FOG;}

    virtual void apply() const;

    void Set(GLenum mode, Vec4f color, float density, float start, float end)
    {
        Color = color;
        Mode = mode;
        Density = density;
        Start = start;
        End = end;
    }

    void SetColor(Vec4f color) {Color = color;}
    void SetMode(GLenum mode) {Mode = mode;}
    void SetDensity(float density) {Density = density;}
    void SetStart(float start) {Start = start;}
    void SetEnd(float end) {End = end;}
    void ApplyEnd(float end)
    {
        End = end;
        glFogf(GL_FOG_END, End);
    }
    void ApplyStart(float start)
    {
        Start = start;
        glFogf(GL_FOG_START, Start);
    }
    void ApplyColor(Vec4f& color)
    {
        Color = color;
        glFogfv(GL_FOG_COLOR, Color.ptr());
    }
    void ApplyDensity(float density)
    {
        Density = density;
        glFogf(GL_FOG_DENSITY, Density);
    }
    void ApplyMode(GLenum mode)
    {
        Mode = mode;
        glFogi(GL_FOG_MODE, Mode);
    }
    Vec4f GetColor() const {return Color;}
    GLenum GetMode() const {return Mode;}
    float GetDensity() const {return Density;}
    float GetStart() const {return Start;}
    float GetEnd() const {return End;}

protected:
    Vec4f Color;
    GLenum Mode;
    float Density;
    float Start;
    float End;
};

class PolygonOffset : public RenderState
{
public:
    PolygonOffset():
        Factor(0.0f),
        Units(0.0f)
    {
    }

    PolygonOffset(float factor, float units) :
        Factor(factor),
        Units(units)
    {
    }

    virtual int type() const {return RS_POLYGONOFFSET;}

    virtual void apply() const;

    void Set(float factor, float units) {Factor = factor; Units = units;}
    void SetFactor(float factor) {Factor = factor;}
    void SetUnits(float units) {Units = units;}
    float GetFactor() const {return Factor;}
    float GetUnits() const {return Units;}

protected:
    float Factor;
    float Units;
};

class AlphaFunc : public RenderState
{
public:
    AlphaFunc(GLenum alphafunc = GL_ALWAYS, float refvalue = 0) :
        RefValue(refvalue),
        Func(alphafunc)
    {
    }

    virtual int type() const {return RS_ALPHAFUNC;}
    virtual void apply() const;

    void SetRefValue(float value) {RefValue = value;}
    void SetAlphaFunc(GLenum func) {Func = func;}

    void Set(GLenum alphafunc, float ref_value)
    {
        Func = alphafunc;
        RefValue = ref_value;
    }
    GLenum GetAlphaFunc() const {return Func;}
    float GetRefValue() const {return RefValue;}

protected:
    float RefValue;
    GLenum Func;
};

class DrawBuffer : public RenderState
{
public:
    DrawBuffer(GLenum buffer = GL_BACK) :
      Buffer(buffer)
    {
    }

    virtual int type() const {return RS_DRAWBUFFER;}

    virtual void apply() const;

    void SetBuffer(GLenum buffer) {Buffer = buffer;}
    const GLenum GetBuffer() const {return Buffer;}

protected:
    GLenum Buffer;
};

class ReadBuffer : public RenderState
{
public:
    ReadBuffer(GLenum buffer = GL_BACK) :
      Buffer(buffer)
    {
    }

    virtual int type() const {return RS_READBUFFER;}

    virtual void apply() const;

    void SetBuffer(GLenum buffer) {Buffer = buffer;}
    const GLenum GetBuffer() const {return Buffer;}

protected:
    GLenum Buffer;
};

class BlendFunc : public RenderState
{
public:
    BlendFunc(GLenum src_rgb = GL_SRC_ALPHA,
              GLenum dst_rgb = GL_ONE_MINUS_SRC_ALPHA,
              GLenum src_alpha = GL_SRC_ALPHA,
              GLenum dst_alpha = GL_ONE_MINUS_SRC_ALPHA) :
        SrcRGB(src_rgb),
        DstRGB(dst_rgb),
        SrcAlpha(src_alpha),
        DstAlpha(dst_alpha)
    {
    }

    virtual int type() const {return RS_BLENDFUNC;}

    virtual void apply() const;
    void Set(GLenum src_rgb, GLenum dst_rgb, GLenum src_alpha, GLenum dst_alpha)
    {
        SrcRGB = src_rgb;
        SrcAlpha = src_alpha;
        DstRGB = dst_rgb;
        DstAlpha = dst_alpha;
    }
    void Set(GLenum src_rgba, GLenum dst_rgba)
    {
        SrcRGB = src_rgba;
        SrcAlpha = src_rgba;
        DstRGB = dst_rgba;
        DstAlpha = dst_rgba;
    }

    void SetSrcRGB(GLenum factor) {SrcRGB = factor;}
    void SetDstRGB(GLenum factor) {DstRGB = factor;}
    void SetSrcAlpha(GLenum factor) {SrcAlpha = factor;}
    void SetDstAlpha(GLenum factor) {DstAlpha = factor;}
    GLenum GetSrcRGB() const {return SrcRGB;}
    GLenum GetDstRGB() const {return DstRGB;}
    GLenum GetSrcAlpha() const {return SrcAlpha;}
    GLenum GetDstAlpha() const {return DstAlpha;}

protected:
    GLenum SrcRGB;
    GLenum DstRGB;
    GLenum SrcAlpha;
    GLenum DstAlpha;
};

class Color4F : public RenderState
{
public:
    Color4F() :
        red(1.0f),
        green(1.0f),
        blue(1.0f),
        alpha(1.0f)
    {
    }

    Color4F(unsigned int color)
    {
        red = piRGBA_GETRED(color) * colorONEOVER255;
        green = piRGBA_GETGREEN(color) * colorONEOVER255;
        blue = piRGBA_GETBLUE(color) * colorONEOVER255;
        alpha = piRGBA_GETALPHA(color) * colorONEOVER255;
    }

    Color4F(float r, float g, float b, float a) :
        red(r),
        green(g),
        blue(b),
        alpha(a)
    {
    }

    Color4F(const Vec4f& color) :
        red(color.r()),
        green(color.g()),
        blue(color.b()),
        alpha(color.a())
    {
    }

    void set(float r, float g, float b, float a)
    {
        red = r;
        green = g;
        blue = b;
        alpha = a;
    }

    void setRed(float r) {red = r;}
    void setGreen(float g) {green = g;}
    void setBlue(float b) {blue = b;}
    void setAlpha(float a) {alpha = a;}

    const float GetRed() const {return red;}
    const float GetGreen() const {return green;}
    const float GetBlue() const {return blue;}
    const float GetAlpha() const {return alpha;}

    virtual int type() const {return RS_COLOR4F;}
    virtual void apply() const;

protected:
    float red;
    float green;
    float blue;
    float alpha;
};

class ColorMask: public RenderState
{
public:
    ColorMask(bool red = true, bool green = true, bool blue = true, bool alpha = true) :
        Red(red),
        Green(green),
        Blue(blue),
        Alpha(alpha)
    {
    }

    virtual int type() const {return RS_COLORMASK;}
    virtual void apply() const;
    void set(bool red, bool green, bool blue, bool alpha)
    {
        Red = red;
        Green = green;
        Blue = blue;
        Alpha = alpha;
    }
    void setRed(bool red) {Red = red;}
    void setGreen(bool green) {Green = green;}
    void setBlue(bool blue) {Blue = blue;}
    void setAlpha(bool alpha) {Alpha = alpha;}
    bool GetRed() const {return Red;}
    bool GetGreen() const {return Green;}
    bool GetBlue() const {return Blue;}
    bool GetAlpha() const {return Alpha;}

protected:
    bool Red;
    bool Green;
    bool Blue;
    bool Alpha;
};

class PixelTransfer: public RenderState
{
public:
    PixelTransfer()
    {
        MapColor = false;
        MapStencil = false;
        IndexShift = 0;
        IndexOffset = 0;
        RedScale = 1;
        GreenScale = 1;
        BlueScale = 1;
        AlphaScale = 1;
        DepthScale = 1;
        RedBias = 0;
        GreenBias = 0;
        BlueBias = 0;
        AlphaBias = 0;
        DepthBias = 0;
        PostColorMatrixRedScale = 1;
        PostColorMatrixGreenScale = 1;
        PostColorMatrixBlueScale = 1;
        PostColorMatrixAlphaScale = 1;
        PostColorMatrixRedBias = 0;
        PostColorMatrixGreenBias = 0;
        PostColorMatrixBlueBias = 0;
        PostColorMatrixAlphaBias = 0;
        PostConvolutionRedScale = 1;
        PostConvolutionGreenScale = 1;
        PostConvolutionBlueScale = 1;
        PostConvolutionAlphaScale = 1;
        PostConvolutionRedBias = 0;
        PostConvolutionGreenBias = 0;
        PostConvolutionBlueBias = 0;
        PostConvolutionAlphaBias = 0;
    }

    virtual int type() const {return RS_PIXELTRANSFER;}

    virtual void apply() const;

    bool GetMapColor() const {return MapColor;}
    bool GetMapStencil() const {return MapStencil;}
    int GetIndexShift() const {return IndexShift;}
    int GetIndexOffset() const {return IndexOffset;}
    float GetRedScale() const {return RedScale;}
    float GetGreenScale() const {return GreenScale;}
    float GetBlueScale() const {return BlueScale;}
    float GetAlphaScale() const {return AlphaScale;}
    float GetDepthScale() const {return DepthScale;}
    float GetRedBias() const {return RedBias;}
    float GetGreenBias() const {return GreenBias;}
    float GetBlueBias() const {return BlueBias;}
    float GetAlphaBias() const {return AlphaBias;}
    float GetDepthBias() const {return DepthBias;}
    float GetPostColorMatrixRedScale() const {return PostColorMatrixRedScale;}
    float GetPostColorMatrixGreenScale() const {return PostColorMatrixGreenScale;}
    float GetPostColorMatrixBlueScale() const {return PostColorMatrixBlueScale;}
    float GetPostColorMatrixAlphaScale() const {return PostColorMatrixAlphaScale;}
    float GetPostColorMatrixRedBias() const {return PostColorMatrixRedBias;}
    float GetPostColorMatrixGreenBias() const {return PostColorMatrixGreenBias;}
    float GetPostColorMatrixBlueBias() const {return PostColorMatrixBlueBias;}
    float GetPostColorMatrixAlphaBias() const {return PostColorMatrixAlphaBias;}
    float GetPostConvolutionRedScale() const {return PostConvolutionRedScale;}
    float GetPostConvolutionGreenScale() const {return PostConvolutionGreenScale;}
    float GetPostConvolutionBlueScale() const {return PostConvolutionBlueScale;}
    float GetPostConvolutionAlphaScale() const {return PostConvolutionAlphaScale;}
    float GetPostConvolutionRedBias() const {return PostConvolutionRedBias;}
    float GetPostConvolutionGreenBias() const {return PostConvolutionGreenBias;}
    float GetPostConvolutionBlueBias() const {return PostConvolutionBlueBias;}
    float GetPostConvolutionAlphaBias() const {return PostConvolutionAlphaBias;}

    void SetMapColor(bool map_color) {MapColor = map_color;}
    void SetMapStencil(bool map_stencil) {MapStencil = map_stencil;}
    void SetIndexShift(int index_shift) {IndexShift = index_shift;}
    void SetIndexOffset(int index_offset) {IndexOffset = index_offset;}
    void SetGreenScale(float green_scale) {GreenScale = green_scale;}
    void SetBlueScale(float blue_scale) {BlueScale = blue_scale;}
    void SetAlphaScale(float alpha_scale) {AlphaScale = alpha_scale;}
    void SetDepthScale(float depth_scale) {DepthScale = depth_scale;}
    void SetRedBias(float red_bias) {RedBias = red_bias;}
    void SetGreenBias(float green_bias) {GreenBias = green_bias;}
    void SetBlueBias(float blue_bias) {BlueBias = blue_bias;}
    void SetAlphaBias(float alpha_bias) {AlphaBias = alpha_bias;}
    void SetDepthBias(float depth_bias) {DepthBias = depth_bias;}
    void SetPostColorMatrixRedScale(float scale) {PostColorMatrixRedScale = scale;}
    void SetPostColorMatrixGreenScale(float scale) {PostColorMatrixGreenScale = scale;}
    void SetPostColorMatrixBlueScale(float scale) {PostColorMatrixBlueScale = scale;}
    void SetPostColorMatrixAlphaScale(float scale) {PostColorMatrixAlphaScale = scale;}
    void SetPostColorMatrixRedBias(float bias) {PostColorMatrixRedBias = bias;}
    void SetPostColorMatrixGreenBias(float bias) {PostColorMatrixGreenBias = bias;}
    void SetPostColorMatrixBlueBias(float bias) {PostColorMatrixBlueBias = bias;}
    void SetPostColorMatrixAlphaBias(float bias) {PostColorMatrixAlphaBias = bias;}
    void SetPostConvolutionRedScale(float scale) {PostConvolutionRedScale = scale;}
    void SetPostConvolutionGreenScale(float scale) {PostConvolutionGreenScale = scale;}
    void SetPostConvolutionBlueScale(float scale) {PostConvolutionBlueScale = scale;}
    void SetPostConvolutionAlphaScale(float scale) {PostConvolutionAlphaScale = scale;}
    void SetPostConvolutionRedBias(float bias) {PostConvolutionRedBias = bias;}
    void SetPostConvolutionGreenBias(float bias) {PostConvolutionGreenBias = bias;}
    void SetPostConvolutionBlueBias(float bias) {PostConvolutionBlueBias = bias;}
    void SetPostConvolutionAlphaBias(float bias) {PostConvolutionAlphaBias = bias;}

protected:
    bool MapColor;
    bool MapStencil;

    int IndexShift;
    int IndexOffset;

    float RedScale;
    float GreenScale;
    float BlueScale;
    float AlphaScale;
    float DepthScale;
    float RedBias;
    float GreenBias;
    float BlueBias;
    float AlphaBias;
    float DepthBias;
    float PostColorMatrixRedScale;
    float PostColorMatrixGreenScale;
    float PostColorMatrixBlueScale;
    float PostColorMatrixAlphaScale;
    float PostColorMatrixRedBias;
    float PostColorMatrixGreenBias;
    float PostColorMatrixBlueBias;
    float PostColorMatrixAlphaBias;
    float PostConvolutionRedScale;
    float PostConvolutionGreenScale;
    float PostConvolutionBlueScale;
    float PostConvolutionAlphaScale;
    float PostConvolutionRedBias;
    float PostConvolutionGreenBias;
    float PostConvolutionBlueBias;
    float PostConvolutionAlphaBias;
};

class Hint : public RenderState
{
public:
    Hint() :
        PerspectiveCorrectionHint(GL_DONT_CARE),
        PointSmoothHint(GL_DONT_CARE),
        LineSmoothHint(GL_DONT_CARE),
        PolygonSmoothHint(GL_DONT_CARE),
        FogHint(GL_DONT_CARE),
        GenerateMipmapHint(GL_DONT_CARE)
    {
    }

    virtual int type() const {return RS_HINT;}
    virtual void apply() const;

    void SetPerspectiveCorrectionHint(GLenum mode) {PerspectiveCorrectionHint = mode;}
    void SetPolygonSmoohtHint(GLenum mode) {PolygonSmoothHint = mode;}
    void SetLineSmoothHint(GLenum mode) {LineSmoothHint = mode;}
    void SetPointSmoothHint(GLenum mode) {PointSmoothHint = mode;}
    void SetFogHint(GLenum mode) {FogHint = mode;}
    void SetGenerateMipmapHint(GLenum mode) {GenerateMipmapHint = mode;}

    GLenum GetPerspectiveCorrectionHint() const {return PerspectiveCorrectionHint;}
    GLenum GetPolygonSmoohtHint() const {return PolygonSmoothHint;}
    GLenum GetLineSmoothHint() const {return LineSmoothHint;}
    GLenum GetPointSmoothHint() const {return PointSmoothHint;}
    GLenum GetFogHint() const {return FogHint;}
    GLenum GetGenerateMipmapHint() const {return GenerateMipmapHint;}

protected:
    GLenum PerspectiveCorrectionHint;
    GLenum PointSmoothHint;
    GLenum LineSmoothHint;
    GLenum PolygonSmoothHint;
    GLenum FogHint;
    GLenum GenerateMipmapHint;
};

class CullFace : public RenderState
{
public:
    CullFace(GLenum cullface = GL_BACK) :
        FaceMode(cullface)
    {
    }

    virtual int type() const {return RS_CULLFACE;}
    virtual void apply() const;

    void Set(GLenum facemode) {FaceMode = facemode;}
    GLenum GetFaceMode() const {return FaceMode;}

protected:
    GLenum FaceMode;
};

class FrontFace : public RenderState
{
public:
    FrontFace(GLenum ff = GL_CCW) :
        frontface(ff)
    {
    }

    virtual int type() const {return RS_FRONTFACE;}
    virtual void apply() const;

    void Set(GLenum ff) {frontface = frontface;}
    GLenum GetFrontFace() const {return frontface;}

protected:
    GLenum frontface;
};

class ClearDepth : public RenderState
{
public:
    ClearDepth(float clearvalue = 1.0f) :
      ClearValue(clearvalue)
    {
    }

    virtual int type() const {return RS_CLEARDEPTH;}
    virtual void apply() const;

    void Set(float clearvalue) {ClearValue = clearvalue;}
    float GetClearValue() const {return ClearValue;}

protected:
    float ClearValue;
};

class DepthFunc : public RenderState
{
public:
    DepthFunc(GLenum depthfunc = GL_LESS) :
        Func(depthfunc)
    {
    }

    virtual int type() const {return RS_DEPTHFUNC;}
    virtual void apply() const;

    void Set(GLenum depthfunc) {Func = depthfunc;}
    GLenum GetDepthFunc() const {return Func;}

protected:
    GLenum Func;
};

class DepthMask : public RenderState
{
public:
    DepthMask(bool depthmask = true) :
        Mask(depthmask)
    {
    }

    virtual int type() const {return RS_DEPTHMASK;}
    virtual void apply() const;

    void Set(bool depthmask) {Mask = depthmask;}
    bool depthMask() const {return Mask;}

    static bool DepthWriteEnabled;

protected:

    bool Mask;
};

class PolygonMode : public RenderState
{
public:
    PolygonMode(GLenum frontface = GL_FILL, GLenum backface = GL_FILL) :
        FrontFace(frontface),
        BackFace(backface)
    {
    }

    virtual int type() const {return RS_POLYGONMODE;}
    virtual void apply() const;
    void Set(GLenum frontface, GLenum backface)
    {
        FrontFace = frontface;
        BackFace = backface;
    }

    void SetFrontFace(GLenum frontface) {FrontFace = frontface;}
    void SetBackFace(GLenum backface) {BackFace = backface;}

    GLenum GetFrontFace() const {return FrontFace;}
    GLenum GetBackFace() const {return BackFace;}

protected:
    GLenum FrontFace;
    GLenum BackFace;
};

class ShadeModel : public RenderState
{
public:
    ShadeModel(GLenum shademodel = GL_SMOOTH) :
          Model(shademodel)
    {
    }

    virtual int type() const {return RS_SHADEMODEL;}
    virtual void apply() const;
    void Set(GLenum shademodel) {Model = shademodel;}
    GLenum GetShadeModel() const {return Model;}

protected:
    GLenum Model;
};

class BlendEquation : public RenderState
{
public:
    BlendEquation(GLenum mode_rgb = GL_FUNC_ADD, GLenum mode_alpha = GL_FUNC_ADD) :
        ModeRGB(mode_rgb),
        ModeAlpha(mode_alpha)
    {
    }

    virtual int type() const {return RS_BLENDEQUATION;}
    virtual void apply() const;

    void Set(GLenum mode_rgba)
    {
        ModeRGB = mode_rgba;
        ModeAlpha = mode_rgba;
    }

    void Set(GLenum mode_rgb, GLenum mode_alpha)
    {
        ModeRGB = mode_rgb;
        ModeAlpha = mode_alpha;
    }
    GLenum GetModeRGB() const {return ModeRGB;}
    GLenum GetModeAlpha() const {return ModeAlpha;}

protected:
    GLenum ModeRGB;
    GLenum ModeAlpha;
};

class SampleCoverage : public RenderState
{
public:
    SampleCoverage(GLclampf value = 1.0f, bool invert = false) :
        Value(value),
        Invert(invert)
    {
    }

    virtual int type() const {return RS_SAMPLECOVERAGE;}
    virtual void apply() const;
    void Set(GLclampf value, bool invert)
    {
        Value = value;
        Invert = invert;
    }
    void SetValue(GLclampf value) {Value = value;}
    void SetInvert(bool invert) {Invert = invert;}
    GLclampf GetValue() const {return Value;}
    bool GetInvert() const {return Invert;}

protected:
    GLclampf Value;
    bool Invert;
};

class LogicOp : public RenderState
{
public:
    LogicOp(GLenum logicop = GL_COPY) :
        Op(logicop)
    {
    }

        virtual int type() const {return RS_LOGICOP;}
    virtual void apply() const;

    void Set(GLenum logicop) {Op = logicop;}
    GLenum GetLogicOp() const {return Op;}

protected:
    GLenum Op;
};

class DepthRange : public RenderState
{
public:
    DepthRange() :
        ZNear(0),
        ZFar(1.0f)
    {
    }

    DepthRange(float znear, float zfar) :
        ZNear(znear),
        ZFar(zfar)
    {
    }

    virtual int type() const {return RS_DEPTHRANGE;}
    virtual void apply() const;

    void Set(float znear, float zfar)
    {
        ZNear = znear;
        ZFar = zfar;
    }
    void SetZNear(float znear) {ZNear = znear;}
    void SetZFar(float zfar) {ZFar = zfar;}
    float GetZNear() const {return ZNear;}
    float GetZFar() const {return ZFar;}

protected:
    float ZNear;
    float ZFar;
};

class LineWidth : public RenderState
{
public:
    LineWidth(float linewidth = 1.0f) :
        Width(linewidth)
    {
    }

    virtual int type() const {return RS_LINEWIDTH;}
    virtual void apply() const;

    void Set(float linewidth) {Width = linewidth;}
    float GetLineWidth() const {return Width;}

protected:
    float Width;
};

class PointSize : public RenderState
{
public:
    PointSize(float pointsize = 1.0f) :
        Size(pointsize)
    {
    }

    virtual int type() const {return RS_POINTSIZE;}
    virtual void apply() const;

    void Set(float pointsize) {Size = pointsize;}
    float GetPointSize() const {return Size;}

protected:
    float Size;
};

class PolygonStipple : public RenderState
{
public:
    PolygonStipple();
    PolygonStipple(const unsigned char* mask);

    virtual int type() const {return RS_POLYGONSTIPPLE;}
    virtual void apply() const;

    void Set(const unsigned char* mask);
    const unsigned char* GetMask() const {return Mask;}

protected:
    unsigned char Mask[32 * 32 / 8];
};

class LineStipple : public RenderState
{
public:
    LineStipple(int factor = 1, GLushort pattern = ~(GLushort)0) :
        Factor(factor),
        Pattern(pattern)
    {
    }

    virtual int type() const {return RS_LINESTIPPLE;}
    virtual void apply() const;

    void Set(int factor, GLushort pattern)
    {
        Factor = factor;
        Pattern = pattern;
    }
    void SetFactor(int factor) {Factor = factor;}
    void SetPattern(GLushort pattern) {Pattern = pattern;}
    int GetFactor() const {return Factor;}
    GLushort GetPattern() const {return Pattern;}

protected:
    int Factor;
    GLushort Pattern;
};

class PointParameter : public RenderState
{
public:
    PointParameter(float sizemin = 0,
                   float sizemax = 1024.0f,
                   float fadethresholdsize = 1.0f,
                   Vec3f distanceattenuation = Vec3f(1, 0, 0)) :

        DistanceAttenuation(distanceattenuation),
        SizeMin(sizemin),
        SizeMax(sizemax),
        FadeThresholdSize(fadethresholdsize),
        PointSpriteCoordOrigin(GL_UPPER_LEFT)
    {
    }

    virtual int type() const {return RS_POINTPARAMETER;}
    virtual void apply() const;

    void Set(float sizemin, float sizemax, float fadethresholdsize, Vec3f distanceattenuation)
    {
        DistanceAttenuation = distanceattenuation;
        SizeMin = sizemin;
        SizeMax = sizemax;
        FadeThresholdSize = fadethresholdsize;
    }

    void SetDistanceAttenuation(Vec3f attenuation) {DistanceAttenuation = attenuation;}
    void SetSizeMin(float sizemin) {SizeMin = sizemin;}
    void SetSizeMax(float sizemax) {SizeMax = sizemax;}
    void SetFadeThresholdSize(float threshold) {FadeThresholdSize = threshold;}
    Vec3f GetDistanceAttenuation() const {return DistanceAttenuation;}
    float GetSizeMin() const {return SizeMin;}
    float GetSizeMax() const {return SizeMax;}
    float GetFadeThresholdSize() const {return FadeThresholdSize;}
    GLenum GetPointSpriteCoordOrigin() const {return PointSpriteCoordOrigin;}
    void SetPointSpriteCoordOrigin(GLenum orig) {PointSpriteCoordOrigin = orig;}

protected:
    Vec3f DistanceAttenuation;
    float SizeMin;
    float SizeMax;
    float FadeThresholdSize;
    GLenum PointSpriteCoordOrigin;
};

class StencilFunc : public RenderState
{
public:
    StencilFunc(GLenum function = GL_ALWAYS,
                int refvalue = 0,
                unsigned int mask = ~(unsigned int)0) :
        Function_Front(function),
        Function_Back(function),
        RefValue_Front(refvalue),
        RefValue_Back(refvalue),
        Mask_Front(mask),
        Mask_Back(mask)
    {
    }

    virtual int type() const {return RS_STENCILFUNC;}
    virtual void apply() const;

    void set(GLenum face, GLenum function, int refvalue, unsigned int mask)
    {
        if(face == GL_FRONT || face == GL_FRONT_AND_BACK)
        {
            Function_Front = function;
            RefValue_Front = refvalue;
            Mask_Front     = mask;
        }

        if(face == GL_BACK || face == GL_FRONT_AND_BACK)
        {
            Function_Back = function;
            RefValue_Back = refvalue;
            Mask_Back     = mask;
        }
    }

    GLenum GetFunction_Front() const {return Function_Front;}
    int GetRefValue_Front() const {return RefValue_Front;}
    unsigned int GetMask_Front() const {return Mask_Front;}
    GLenum GetFunction_Back() const {return Function_Back;}
    int GetRefValue_Back() const {return RefValue_Back;}
    unsigned int GetMask_Back() const {return Mask_Back;}

protected:
    GLenum Function_Front;
    GLenum Function_Back;
    int RefValue_Front;
    int RefValue_Back;
    unsigned int Mask_Front;
    unsigned int Mask_Back;
};

class StencilOp : public RenderState
{
public:
    StencilOp(GLenum sfail = GL_KEEP,
              GLenum dpfail = GL_KEEP,
              GLenum dppass = GL_KEEP) :
        SFail_Front(sfail),
        SFail_Back(sfail),
        DpFail_Front(dpfail),
        DpFail_Back(dpfail),
        DpPass_Front(dppass),
        DpPass_Back(dppass)
    {
    }

    virtual int type() const {return RS_STENCILOP;}
    virtual void apply() const;

    void set(GLenum face, GLenum sfail, GLenum dpfail, GLenum dppass)
    {
        if(face == GL_FRONT || face == GL_FRONT_AND_BACK)
        {
            SFail_Front  = sfail;
            DpFail_Front = dpfail;
            DpPass_Front = dppass;
        }

        if(face == GL_BACK || face == GL_FRONT_AND_BACK)
        {
            SFail_Back  = sfail;
            DpFail_Back = dpfail;
            DpPass_Back = dppass;
        }
    }

    GLenum GetSFail_Front()  const {return SFail_Front;  }
    GLenum GetDpFail_Front() const {return DpFail_Front;}
    GLenum GetDpPass_Front() const {return DpPass_Front;}
    GLenum GetSFail_Back()   const {return SFail_Front;  }
    GLenum GetDpFail_Back()  const {return DpFail_Front;}
    GLenum GetDpPass_Back()  const {return DpPass_Front;}

protected:
    GLenum SFail_Front;
    GLenum SFail_Back;
    GLenum DpFail_Front;
    GLenum DpFail_Back;
    GLenum DpPass_Front;
    GLenum DpPass_Back;
};

class StencilMask : public RenderState
{
public:
    StencilMask(unsigned int mask = ~(unsigned int)0) :
        Mask_Front(mask),
        Mask_Back(mask)
    {
    }

    virtual int type() const {return RS_STENCILMASK;}
    virtual void apply() const;

    void set(GLenum face, unsigned int mask)
    {
        if(face == GL_FRONT || face == GL_FRONT_AND_BACK)
            Mask_Front = mask;
        if(face == GL_BACK  || face == GL_FRONT_AND_BACK)
            Mask_Back = mask;
    }

    unsigned int GetMask_Front() const {return Mask_Front;}
    unsigned int GetMask_Back() const {return Mask_Back;}

protected:
    unsigned int Mask_Front;
    unsigned int Mask_Back;
};

class BlendColor : public RenderState
{
public:
    BlendColor(Vec4f blendcolor = Vec4f(0, 0, 0, 0)) :
        Color(blendcolor)
    {
    }

    virtual int type() const {return RS_BLENDCOLOR;}
    virtual void apply() const;

    void Set(Vec4f blendcolor) {Color = blendcolor;}
    Vec4f GetBlendColor() const {return Color;}

protected:
    Vec4f Color;
};

class TexEnv : public TextureStates
{
public:
    TexEnv(int texunit);

    virtual int type() const {return (int)(RS_TEXENV0 + texunit);}
    virtual void apply() const;

    void SetMode(GLenum mode) {Mode = mode;}
    GLenum GetMode() const {return Mode;}
    void SetCombineRGB(GLenum combineRGB) {CombineRGB = combineRGB;}
    GLenum GetCombineRGB() const {return CombineRGB;}
    void SetCombineAlpha(GLenum combineAlpha) {CombineAlpha = combineAlpha;}
    GLenum GetCombineAlpha() const {return CombineAlpha;}
    void SetColor(Vec4f color) {Color = color;}
    Vec4f GetColor() const {return Color;}
    void SetRGBScale(float rgbscale) {RGBScale = rgbscale;}
    float GetRGBScale() const {return RGBScale;}
    void SetAlphaScale(float alphascale) {AlphaScale = alphascale;}
    float GetAlphaScale() const {return AlphaScale;}

    void SetSource0RGB(GLenum source) {Source0RGB = source;}
    void SetSource1RGB(GLenum source) {Source1RGB = source;}
    void SetSource2RGB(GLenum source) {Source2RGB = source;}
    GLenum GetSource0RGB() const {return Source0RGB;}
    GLenum GetSource1RGB() const {return Source1RGB;}
    GLenum GetSource2RGB() const {return Source2RGB;}
    void SetSource0Alpha(GLenum source) {Source0Alpha = source;}
    void SetSource1Alpha(GLenum source) {Source1Alpha = source;}
    void SetSource2Alpha(GLenum source) {Source2Alpha = source;}
    GLenum GetSource0Alpha() const {return Source0Alpha;}
    GLenum GetSource1Alpha() const {return Source1Alpha;}
    GLenum GetSource2Alpha() const {return Source2Alpha;}

    void SetOperand0RGB(GLenum operand) {Operand0RGB = operand;}
    void SetOperand1RGB(GLenum operand) {Operand1RGB = operand;}
    void SetOperand2RGB(GLenum operand) {Operand2RGB = operand;}
    GLenum GetOperand0RGB() const {return Operand0RGB;}
    GLenum GetOperand1RGB() const {return Operand1RGB;}
    GLenum GetOperand2RGB() const {return Operand2RGB;}
    void SetOperand0Alpha(GLenum operand) {Operand0Alpha = operand;}
    void SetOperand1Alpha(GLenum operand) {Operand1Alpha = operand;}
    void SetOperand2Alpha(GLenum operand) {Operand2Alpha = operand;}
    GLenum GetOperand0Alpha() const {return Operand0Alpha;}
    GLenum GetOperand1Alpha() const {return Operand1Alpha;}
    GLenum GetOperand2Alpha() const {return Operand2Alpha;}

    void SetPointSpriteCoordReplace(bool replace) {PointSpriteCoordReplace = replace;}
    bool GetPointSpriteCoordReplace() const {return PointSpriteCoordReplace;}

    void SetLodBias(float lodbias) {LodBias = lodbias;}
    float GetLodBias() const {return LodBias;}

public:
    Vec4f Color;
    float RGBScale;
    float AlphaScale;
    GLenum Mode;
    GLenum CombineRGB;
    GLenum CombineAlpha;
    GLenum Source0RGB;
    GLenum Source1RGB;
    GLenum Source2RGB;
    GLenum Source0Alpha;
    GLenum Source1Alpha;
    GLenum Source2Alpha;
    GLenum Operand0RGB;
    GLenum Operand1RGB;
    GLenum Operand2RGB;
    GLenum Operand0Alpha;
    GLenum Operand1Alpha;
    GLenum Operand2Alpha;
    float LodBias;
    bool PointSpriteCoordReplace;
};

class TexGen : public TextureStates
{
public:
    TexGen(int unit);

    virtual int type() const {return (int)(RS_TEXGEN0 + texunit);}
    virtual void apply() const;

    void SetPlanesFromMatrix(const Matrixf& m);

    void SetEyePlaneS(Vec4f plane) {EyePlaneS = plane;}
    void SetObjectPlaneS(Vec4f plane) {ObjectPlaneS = plane;}
    void SetGenModeS(GLenum mode) {GenModeS = mode;}
    Vec4f GetEyePlaneS() const {return EyePlaneS;}
    Vec4f GetObjectPlaneS() const {return ObjectPlaneS;}
    GLenum GetGenModeS() const {return GenModeS;}

    void SetEyePlaneT(Vec4f plane) {EyePlaneT = plane;}
    void SetObjectPlaneT(Vec4f plane) {ObjectPlaneT = plane;}
    void SetGenModeT(GLenum mode) {GenModeT = mode;}
    Vec4f GetEyePlaneT() const {return EyePlaneT;}
    Vec4f GetObjectPlaneT() const {return ObjectPlaneT;}
    GLenum GetGenModeT() const {return GenModeT;}

    void SetEyePlaneR(Vec4f plane) {EyePlaneR = plane;}
    void SetObjectPlaneR(Vec4f plane) {ObjectPlaneR = plane;}
    void SetGenModeR(GLenum mode) {GenModeR = mode;}
    Vec4f GetEyePlaneR() const {return EyePlaneR;}
    Vec4f GetObjectPlaneR() const {return ObjectPlaneR;}
    GLenum GetGenModeR() const {return GenModeR;}

    void SetEyePlaneQ(Vec4f plane) {EyePlaneQ = plane;}
    void SetObjectPlaneQ(Vec4f plane) {ObjectPlaneQ = plane;}
    void SetGenModeQ(GLenum mode) {GenModeQ = mode;}
    Vec4f GetEyePlaneQ() const {return EyePlaneQ;}
    Vec4f GetObjectPlaneQ() const {return ObjectPlaneQ;}
    GLenum GetGenModeQ() const {return GenModeQ;}

public:
    Vec4f EyePlaneS;
    Vec4f ObjectPlaneS;
    Vec4f EyePlaneT;
    Vec4f ObjectPlaneT;
    Vec4f EyePlaneR;
    Vec4f ObjectPlaneR;
    Vec4f EyePlaneQ;
    Vec4f ObjectPlaneQ;
    GLenum GenModeS;
    GLenum GenModeT;
    GLenum GenModeR;
    GLenum GenModeQ;
};

class Viewport : public RenderState
{
public:
    typedef float value_type;

    Viewport();

    Viewport(value_type x, value_type y, value_type width, value_type height) :
        X(x),
        Y(y),
        width(width),
        height(height)
        {
        }

    Viewport(int x, int y, int width, int height) :
        X((value_type)x),
        Y((value_type)y),
        width((value_type)width),
        height((value_type)height)
        {
        }

        virtual int type() const {return (int)(RS_VIEWPORT);}
        virtual void apply() const;

        inline void SetViewport(value_type x, value_type y, value_type width, value_type height)
        {
            X = x;
            Y = y;
            width = width;
            height = height;
        }

        inline value_type& x() {return X;}
        inline value_type x() const {return X;}

        inline value_type& y() {return Y;}
        inline value_type y() const {return Y;}

        inline value_type& GetWidth() {return width; }
        inline value_type GetWidth() const {return width; }

        inline value_type& GetHeight() {return height;}
        inline value_type GetHeight() const {return height;}

        inline bool Valid() const {return width > 0 && height > 0;}

        inline float AspectRatio() const
        {
            if(height != 0)
                return (float)width / (float)height;
            else return 1.0;
        }

        inline const Matrixf ComputeWindowMatrix() const
        {
            return Matrixf::translate(1.0f, 1.0f, 1.0f) *
                   Matrixf::scale(0.5f * GetWidth(), 0.5f * GetHeight(), 0.5f) *
                   Matrixf::translate(x(), y(), 0.0f);
        }

protected:
    value_type X;
    value_type Y;
    value_type width;
    value_type height;
};

class ClearNode : public Referenced
{
public:
    ClearNode();
    ClearNode(GLbitfield mask, bool requiresclear, Vec4f& color) :
        clearMask(mask),
        clearColor(color),
        requiresClear(requiresclear)
    {
        depth = new DepthMask;
    }

    ClearNode(GLbitfield mask, bool requiresclear, unsigned int color) :
        clearMask(mask),
        requiresClear(requiresclear)
    {
        clearColor.r() = piRGBA_GETRED(color) * colorONEOVER255;
        clearColor.g() = piRGBA_GETGREEN(color) * colorONEOVER255;
        clearColor.b() = piRGBA_GETBLUE(color) * colorONEOVER255;
        clearColor.a() = piRGBA_GETALPHA(color) * colorONEOVER255;

        depth = new DepthMask;
    }

    virtual void apply() const;

    inline void SetRequiresClear(bool requiresClear) {requiresClear = requiresClear;}
    inline bool GetRequiresClear() const {return requiresClear;}
    inline void SetClearColor(const Vec4f& color) {clearColor = color;}
    inline const Vec4f& GetClearColor() const {return clearColor;}
    inline void SetClearMask(GLbitfield mask) {clearMask = mask;}

    inline GLbitfield GetClearMask() const {return clearMask;}

protected :
    bool requiresClear;
    Vec4f clearColor;
    GLbitfield clearMask;
    ref_ptr<DepthMask> depth;
};

#if(0)
//------------------------------------------------------------------------------
// Shader
//------------------------------------------------------------------------------
/** Manages most of the OpenGL rendering states responsible of the final aspect of the rendered objects.
*
* A Shader represents a rendering pass. Using the Effect class you can assign
* more rendering passes to an Actor. You can also specify different sets of
* rendering passes for each Effect LOD, for more information see Effect.
*
* \remarks
* The Uniforms defined in the Actor and the ones defined in the Shader must not
* overlap, that is, an Actor must  not define Uniforms present in the
* Shader and vice versa.
*
* \sa Effect, Actor */
class Shader: public Object
{
public:

/** Constructor. */
Shader();

/** Copy constructor. Calls \p operator=(other). */
Shader(const Shader& other): Object(other) { operator=(other);}

/** Destructor */
virtual ~Shader();

/** Copy RenderStates, Enables, Uniforms and Scissor from another Shader.
    * Note that the destination Shader still keeps its own copy of the RenderStateSet, EnableSet and UniformSet objects. */
Shader& operator=(const Shader& other)
{
    Object::operator=(other);

    // we don't copy the update time
    // mLastUpdateTime = other.mLastUpdateTime;

    if (other.mRenderStateSet.get())
    {
    if (mRenderStateSet.get() == nullptr)
        mRenderStateSet = new RenderStateSet;
    *mRenderStateSet = *other.mRenderStateSet;
    }
    else
    mRenderStateSet = nullptr;

    if (other.mEnableSet.get())
    {
    if (mEnableSet.get() == nullptr)
        mEnableSet = new EnableSet;
    *mEnableSet = *other.mEnableSet;
    }
    else
    mEnableSet = nullptr;

    if (other.mUniformSet.get())
    {
    if (mUniformSet.get() == nullptr)
        mUniformSet = new UniformSet;
    *mUniformSet = *other.mUniformSet;
    }
    else
    mUniformSet = nullptr;

    mScissor = other.mScissor;
    mShaderAnimator = other.mShaderAnimator;

    #if VL_SHADER_USER_DATA
    mShaderUserData = other.mShaderUserData;
    #endif

    return *this;
}

/** Equivalent to \p "Shader& operator=(const Shader&)" */
Shader& copy(const Shader& other)
{
    return operator=(other);
}

//! Disables everything, erases all the render states, erases all the uniforms.
void reset()
{
    disableAll();
    eraseAllRenderStates();
    eraseAllUniforms();
}

// state getters

/** Gets or creates a GLSLProgram and returns it. */
GLSLProgram* gocGLSLProgram();

/** Returns a GLSLProgram if it exists or nullptr otherwise. */
const GLSLProgram* getGLSLProgram() const;

/** Returns a GLSLProgram if it exists or nullptr otherwise. */
GLSLProgram* getGLSLProgram();
PixelTransfer* gocPixelTransfer();
const PixelTransfer* GetPixelTransfer() const {return dynamic_cast<const PixelTransfer*>(GetRenderStateSet()->GetRenderState(RS_PIXELTRANSFER));}
PixelTransfer* getPixelTransfer() {return dynamic_cast<PixelTransfer*>(GetRenderStateSet()->GetRenderState( RS_PIXELTRANSFER) );}
Hint* gocHint();
const Hint* getHint() const {return dynamic_cast<const Hint*>( getRenderStateSet()->renderState( RS_Hint) );}
Hint* getHint() {return dynamic_cast<Hint*>( getRenderStateSet()->renderState( RS_Hint) );}

CullFace* gocCullFace();
const CullFace* getCullFace() const {return dynamic_cast<const CullFace*>( getRenderStateSet()->renderState( RS_CullFace ) );}
CullFace* getCullFace() {return dynamic_cast<CullFace*>( getRenderStateSet()->renderState( RS_CullFace ) );}

FrontFace* gocFrontFace();
const FrontFace* getFrontFace() const {return dynamic_cast<const FrontFace*>( getRenderStateSet()->renderState( RS_FrontFace ) );}
FrontFace* getFrontFace() {return dynamic_cast<FrontFace*>( getRenderStateSet()->renderState( RS_FrontFace ) );}

DepthFunc* gocDepthFunc();
const DepthFunc* getDepthFunc() const {return dynamic_cast<const DepthFunc*>( getRenderStateSet()->renderState( RS_DepthFunc ) );}
DepthFunc* getDepthFunc() {return dynamic_cast<DepthFunc*>( getRenderStateSet()->renderState( RS_DepthFunc ) );}

DepthMask* gocDepthMask();
const DepthMask* getDepthMask() const {return dynamic_cast<const DepthMask*>( getRenderStateSet()->renderState( RS_DepthMask ) );}
DepthMask* getDepthMask() {return dynamic_cast<DepthMask*>( getRenderStateSet()->renderState( RS_DepthMask ) );}

ColorMask* gocColorMask();
const ColorMask* getColorMask() const {return dynamic_cast<const ColorMask*>( getRenderStateSet()->renderState( RS_ColorMask ) );}
ColorMask* getColorMask() {return dynamic_cast<ColorMask*>( getRenderStateSet()->renderState( RS_ColorMask ) );}

PolygonMode* gocPolygonMode();
const PolygonMode* getPolygonMode() const {return dynamic_cast<const PolygonMode*>( getRenderStateSet()->renderState( RS_PolygonMode ) );}
PolygonMode* getPolygonMode() {return dynamic_cast<PolygonMode*>( getRenderStateSet()->renderState( RS_PolygonMode ) );}

ShadeModel* gocShadeModel();
const ShadeModel* getShadeModel() const {return dynamic_cast<const ShadeModel*>( getRenderStateSet()->renderState( RS_ShadeModel ) );}
ShadeModel* getShadeModel() {return dynamic_cast<ShadeModel*>( getRenderStateSet()->renderState( RS_ShadeModel ) );}

BlendEquation* gocBlendEquation();
const BlendEquation* getBlendEquation() const {return dynamic_cast<const BlendEquation*>( getRenderStateSet()->renderState( RS_BlendEquation ) );}
BlendEquation* getBlendEquation() {return dynamic_cast<BlendEquation*>( getRenderStateSet()->renderState( RS_BlendEquation ) );}

AlphaFunc* gocAlphaFunc();
const AlphaFunc* getAlphaFunc() const {return dynamic_cast<const AlphaFunc*>( getRenderStateSet()->renderState( RS_AlphaFunc ) );}
AlphaFunc* getAlphaFunc() {return dynamic_cast<AlphaFunc*>( getRenderStateSet()->renderState( RS_AlphaFunc ) );}

Material* gocMaterial();
const Material* getMaterial() const {return dynamic_cast<const Material*>( getRenderStateSet()->renderState( RS_Material ) );}
Material* getMaterial() {return dynamic_cast<Material*>( getRenderStateSet()->renderState( RS_Material ) );}

LightModel* gocLightModel();
const LightModel* getLightModel() const {return dynamic_cast<const LightModel*>( getRenderStateSet()->renderState( RS_LightModel ) );}
LightModel* getLightModel() {return dynamic_cast<LightModel*>( getRenderStateSet()->renderState( RS_LightModel ) );}

Fog* gocFog();
const Fog* getFog() const {return dynamic_cast<const Fog*>( getRenderStateSet()->renderState( RS_Fog ) );}
Fog* getFog() {return dynamic_cast<Fog*>( getRenderStateSet()->renderState( RS_Fog ) );}

PolygonOffset* gocPolygonOffset();
const PolygonOffset* getPolygonOffset() const {return dynamic_cast<const PolygonOffset*>( getRenderStateSet()->renderState( RS_PolygonOffset ) );}
PolygonOffset* getPolygonOffset() {return dynamic_cast<PolygonOffset*>( getRenderStateSet()->renderState( RS_PolygonOffset ) );}

LogicOp* gocLogicOp();
const LogicOp* getLogicOp() const {return dynamic_cast<const LogicOp*>( getRenderStateSet()->renderState( RS_LogicOp ) );}
LogicOp* getLogicOp() {return dynamic_cast<LogicOp*>( getRenderStateSet()->renderState( RS_LogicOp ) );}

DepthRange* gocDepthRange();
const DepthRange* getDepthRange() const {return dynamic_cast<const DepthRange*>( getRenderStateSet()->renderState( RS_DepthRange ) );}
DepthRange* getDepthRange() {return dynamic_cast<DepthRange*>( getRenderStateSet()->renderState( RS_DepthRange ) );}

LineWidth* gocLineWidth();
const LineWidth* getLineWidth() const {return dynamic_cast<const LineWidth*>( getRenderStateSet()->renderState( RS_LineWidth ) );}
LineWidth* getLineWidth() {return dynamic_cast<LineWidth*>( getRenderStateSet()->renderState( RS_LineWidth ) );}

PointSize* gocPointSize();
const PointSize* getPointSize() const {return dynamic_cast<const PointSize*>( getRenderStateSet()->renderState( RS_PointSize ) );}
PointSize* getPointSize() {return dynamic_cast<PointSize*>( getRenderStateSet()->renderState( RS_PointSize ) );}

LineStipple* gocLineStipple();
const LineStipple* getLineStipple() const {return dynamic_cast<const LineStipple*>( getRenderStateSet()->renderState( RS_LineStipple ) );}
LineStipple* getLineStipple() {return dynamic_cast<LineStipple*>( getRenderStateSet()->renderState( RS_LineStipple ) );}

PolygonStipple* gocPolygonStipple();
const PolygonStipple* getPolygonStipple() const {return dynamic_cast<const PolygonStipple*>( getRenderStateSet()->renderState( RS_PolygonStipple ) );}
PolygonStipple* getPolygonStipple() {return dynamic_cast<PolygonStipple*>( getRenderStateSet()->renderState( RS_PolygonStipple ) );}

PointParameter* gocPointParameter();
const PointParameter* getPointParameter() const {return dynamic_cast<const PointParameter*>( getRenderStateSet()->renderState( RS_PointParameter ) );}
PointParameter* getPointParameter() {return dynamic_cast<PointParameter*>( getRenderStateSet()->renderState( RS_PointParameter ) );}

StencilFunc* gocStencilFunc();
const StencilFunc* getStencilFunc() const {return dynamic_cast<const StencilFunc*>( getRenderStateSet()->renderState( RS_StencilFunc ) );}
StencilFunc* getStencilFunc() {return dynamic_cast<StencilFunc*>( getRenderStateSet()->renderState( RS_StencilFunc ) );}

StencilOp* gocStencilOp();
const StencilOp* getStencilOp() const {return dynamic_cast<const StencilOp*>( getRenderStateSet()->renderState( RS_StencilOp ) );}
StencilOp* getStencilOp() {return dynamic_cast<StencilOp*>( getRenderStateSet()->renderState( RS_StencilOp ) );}

StencilMask* gocStencilMask();
const StencilMask* getStencilMask() const {return dynamic_cast<const StencilMask*>( getRenderStateSet()->renderState( RS_StencilMask ) );}
StencilMask* getStencilMask() {return dynamic_cast<StencilMask*>( getRenderStateSet()->renderState( RS_StencilMask ) );}

BlendColor* gocBlendColor();
const BlendColor* getBlendColor() const {return dynamic_cast<const BlendColor*>( getRenderStateSet()->renderState( RS_BlendColor ) );}
BlendColor* getBlendColor() {return dynamic_cast<BlendColor*>( getRenderStateSet()->renderState( RS_BlendColor ) );}

BlendFunc* gocBlendFunc();
const BlendFunc* getBlendFunc() const {return dynamic_cast<const BlendFunc*>( getRenderStateSet()->renderState( RS_BlendFunc ) );}
BlendFunc* getBlendFunc() {return dynamic_cast<BlendFunc*>( getRenderStateSet()->renderState( RS_BlendFunc ) );}

SampleCoverage* gocSampleCoverage();
const SampleCoverage* getSampleCoverage() const {return dynamic_cast<const SampleCoverage*>( getRenderStateSet()->renderState( RS_SampleCoverage ) );}
SampleCoverage* getSampleCoverage() {return dynamic_cast<SampleCoverage*>( getRenderStateSet()->renderState( RS_SampleCoverage ) );}

// indexed render states

// lights

Light* gocLight(int light_index);

const Light* getLight(int light_index) const;

Light* getLight(int light_index);

// clip planes

ClipPlane* gocClipPlane(int plane_index);

const ClipPlane* getClipPlane(int plane_index) const;

ClipPlane* getClipPlane(int plane_index);

// texture unit

TextureUnit* gocTextureUnit(int unit_index);

const TextureUnit* getTextureUnit(int unit_index) const {return dynamic_cast<const TextureUnit*>( getRenderStateSet()->renderState( (int)(RS_TextureUnit0+unit_index) ) );}

TextureUnit* getTextureUnit(int unit_index) {return dynamic_cast<TextureUnit*>( getRenderStateSet()->renderState( (int)(RS_TextureUnit0+unit_index) ) );}

// tex env

TexEnv* gocTexEnv(int unit_index);

const TexEnv* getTexEnv(int unit_index) const {return dynamic_cast<const TexEnv*>( getRenderStateSet()->renderState( (int)(RS_TexEnv0+unit_index) ) );}

TexEnv* getTexEnv(int unit_index) {return dynamic_cast<TexEnv*>( getRenderStateSet()->renderState( (int)(RS_TexEnv0+unit_index) ) );}

// tex gen

TexGen* gocTexGen(int unit_index);

const TexGen* getTexGen(int unit_index) const {return dynamic_cast<const TexGen*>( getRenderStateSet()->renderState( (int)(RS_TexGen0+unit_index) ) );}

TexGen* getTexGen(int unit_index) {return dynamic_cast<TexGen*>( getRenderStateSet()->renderState( (int)(RS_TexGen0+unit_index) ) );}

// texture matrix

TextureMatrix* gocTextureMatrix(int unit_index);

const TextureMatrix* getTextureMatrix(int unit_index) const {return dynamic_cast<const TextureMatrix*>( getRenderStateSet()->renderState( (int)(RS_TextureMatrix0+unit_index) ) );}

TextureMatrix* getTextureMatrix(int unit_index) {return dynamic_cast<TextureMatrix*>( getRenderStateSet()->renderState( (int)(RS_TextureMatrix0+unit_index) ) );}

// enable methods

void enable(EEnable capability)  { gocEnableSet()->enable(capability);}

void disable(EEnable capability) { gocEnableSet()->disable(capability);}

const std::vector<EEnable>& enables() const {return getEnableSet()->enables();}

int isEnabled(EEnable capability) const { if (!getEnableSet()) return false; return getEnableSet()->isEnabled(capability);}

void disableAll() { if (getEnableSet()) getEnableSet()->disableAll();}

bool blendingEnabled() const { if (!getEnableSet()) return false; return getEnableSet()->blendingEnabled();}

// render states methods

void setRenderState(RenderState* renderstate) { gocRenderStateSet()->setRenderState(renderstate);}

const RenderState* renderState( int type ) const { if (!getRenderStateSet()) return nullptr; return getRenderStateSet()->renderState(type);}

RenderState* renderState( int type ) {return gocRenderStateSet()->renderState(type);}

const std::vector< ref<RenderState> >& renderStates() const {return getRenderStateSet()->renderStates();}

void erasint(int type) { gocRenderStateSet()->erasint(type);}

void erasint(RenderState* rs) { if (rs) gocRenderStateSet()->erasint(rs->type());}

void eraseAllRenderStates() { if(getRenderStateSet()) getRenderStateSet()->eraseAllRenderStates();}

//! Returns the GLSLProgram associated to a Shader (if any)
const GLSLProgram* glslProgram() const { if (!getRenderStateSet()) return nullptr; return getRenderStateSet()->glslProgram();}

//! Returns the GLSLProgram associated to a Shader (if any)
GLSLProgram* glslProgram() {return gocRenderStateSet()->glslProgram();}

// uniforms methods

//! Equivalent to gocUniformSet()->setUniform(...)
void setUniform(Uniform* uniform) { VL_CHECK(uniform); gocUniformSet()->setUniform(uniform);}

//! Equivalent to gocUniformSet()->uniforms(...)
const std::vector< ref<Uniform> >& uniforms() const {return getUniformSet()->uniforms();}

//! Equivalent to gocUniformSet()->eraseUniform(...)
void eraseUniform(const std::string& name) { gocUniformSet()->eraseUniform(name);}

//! Equivalent to gocUniformSet()->eraseUniform(...)
void eraseUniform(const Uniform* uniform) { gocUniformSet()->eraseUniform(uniform);}

//! Equivalent to gocUniformSet()->eraseAllUniforms(...)
void eraseAllUniforms() { if (getUniformSet()) getUniformSet()->eraseAllUniforms();}

//! Equivalent to gocUniformSet()->gocUniform(...)
Uniform* gocUniform(const std::string& name) {return gocUniformSet()->gocUniform(name);}

//! Equivalent to gocUniformSet()->getUniform(...)
Uniform* getUniform(const std::string& name) {return getUniformSet()->getUniform(name);}

//! Equivalent to gocUniformSet()->getUniform(...)
const Uniform* getUniform(const std::string& name) const {return getUniformSet()->getUniform(name);}

// sets

EnableSet* gocEnableSet() { if (!mEnableSet) mEnableSet = new EnableSet; return mEnableSet.get();}

EnableSet* getEnableSet() {return mEnableSet.get();}

const EnableSet* getEnableSet() const {return mEnableSet.get();}

RenderStateSet* gocRenderStateSet() { if (!mRenderStateSet) mRenderStateSet = new RenderStateSet; return mRenderStateSet.get();}

RenderStateSet* getRenderStateSet() {return mRenderStateSet.get();}

const RenderStateSet* getRenderStateSet() const {return mRenderStateSet.get();}

/**
    * Returns the UniformSet installed (creating it if no UniformSet has been installed)
    * \sa
    * - setUniform()
    * - uniforms()
    * - eraseUniform(const std::string& name)
    * - eraseUniform(const Uniform* uniform)
    * - eraseAllUniforms()
    * - getUniform()
*/
UniformSet* gocUniformSet() { if (!mUniformSet) mUniformSet = new UniformSet; return mUniformSet.get();}

/**
    * Returns the UniformSet installed
    * \sa
    * - setUniform()
    * - uniforms()
    * - eraseUniform(const std::string& name)
    * - eraseUniform(const Uniform* uniform)
    * - eraseAllUniforms()
    * - getUniform()
*/
UniformSet* getUniformSet() {return mUniformSet.get();}

/**
    * Returns the UniformSet installed
    * \sa
    * - setUniform()
    * - uniforms()
    * - eraseUniform(const std::string& name)
    * - eraseUniform(const Uniform* uniform)
    * - eraseAllUniforms()
    * - getUniform()
*/
const UniformSet* getUniformSet() const {return mUniformSet.get();}

void setEnableSet(EnableSet* es) {EnableSet = es;}

void setRenderStateSet(RenderStateSet* rss) {RenderStateSet = rss;}

/**
    * Installs a new UniformSet
    * \sa
    * - setUniform()
    * - uniforms()
    * - eraseUniform(const std::string& name)
    * - eraseUniform(const Uniform* uniform)
    * - eraseAllUniforms()
    * - getUniform()
*/
void setUniformSet(UniformSet* us) {UniformSet = us;}

/**
    * Sets the Scissor to be used when rendering an Actor.
    * \note
    * You can also define a Scissor on a per-Actor basis using the function Actor::setScissor().
    * In case both the Shader's and the Actor's Scissor are defined the Actor's Scissor is used.
    * \sa
    * - Scissor
    * - Actor::setScissor()
    */
void setScissor(Scissor* scissor) {Scissor = scissor;}

/**
    * Returns the Scissor to be used when rendering an Actor.
    * \sa
    * - Scissor
    * - Shader::setScissor()
    * - Actor::setScissor()
    */
const Scissor* scissor() const {return mScissor.get();}

/**
    * Returns the Scissor to be used when rendering an Actor.
    * \sa
    * - Scissor
    * - Shader::setScissor()
    * - Actor::setScissor()
    */
Scissor* scissor() {return mScissor.get();}

/** Installs the ShaderAnimator used to update/animate a Shader (see vl::ShaderAnimator documentation). */
void setShaderAnimator(ShaderAnimator* animator) {ShaderAnimator = animator;}

/** Returns the ShaderAnimator used to update/animate a Shader (see vl::ShaderAnimator documentation). */
ShaderAnimator* shaderAnimator() {return mShaderAnimator.get();}

/** Returns the ShaderAnimator used to update/animate a Shader (see vl::ShaderAnimator documentation). */
const ShaderAnimator* shaderAnimator() const {return mShaderAnimator.get();}

/** Last time this Actor was animated/updated using a shaderAnimator(). */
Real lastUpdateTime() const {return mLastUpdateTime;}

//! Used internally.
void setLastUpdateTime(Real time) {LastUpdateTime = time;}

#if VL_SHADER_USER_DATA
public:
void* shaderUserData() {return mShaderUserData;}
const void* shaderUserData() const {return mShaderUserData;}
void setShaderUserData(void* user_data) {ShaderUserData = user_data;}

private:
void* mShaderUserData;
#endif

protected:
ref<RenderStateSet> mRenderStateSet;
ref<EnableSet> mEnableSet;
ref<UniformSet> mUniformSet;
ref<Scissor> mScissor;
ref<ShaderAnimator> mShaderAnimator;
Real mLastUpdateTime;
};
}
#endif

#endif
/*-------------------------------------------------------------------------------------------------------*/

struct VertexArrayObjectBind
{
    static unsigned int oldname;

    static bool Bind(unsigned int name)
    {
        if(GLEW_ARB_vertex_array_object)
        {
            if(name != oldname)
            {
                glBindVertexArray(name);
                oldname = name;
                return true;
            }
        }

        return false;
    }
};

struct BindBufferBase
{
    static unsigned int oldname;
    static unsigned int oldtarget;
    static unsigned int oldindex;

    static bool Bind(unsigned int target, unsigned int index, int name)
    {
        if(oldtarget != target || oldname != name || oldindex != index)
        {
            glBindBufferBase(target, index, name);

            oldtarget = target;
            oldname = name;
            oldindex = index;

            return true;
        }

        return false;
    }
};

struct ArrayBufferBind
{
    static unsigned int oldname;

    static bool Bind(unsigned int name)
    {
        if(oldname != name)
        {
            glBindBufferARB(GL_ARRAY_BUFFER_ARB, name);
            oldname = name;

            return true;
        }

        return false;
    }
};

struct ElementBufferBind
{
    static unsigned int oldname;

    static bool Bind(unsigned int name)
    {
        if(oldname != name)
        {
            glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, name);
            oldname = name;

            return true;
        }

        return false;
    }
};

struct TextureBufferBind
{
    static unsigned int oldname;

    static bool Bind(unsigned int name)
    {
        if(oldname != name)
        {
            glBindBufferARB(GL_TEXTURE_BUFFER, name);
            oldname = name;

            return true;
        }

        return false;
    }
};

struct UniformBufferBind
{
    static bool BindBufferBase(unsigned int name, int base)
    {
        if(GLEW_ARB_uniform_buffer_object)
        {
            return BindBufferBase::Bind(GL_UNIFORM_BUFFER, base, name);
        }

        return false;
    }
};

struct TextureBufferObjectBind
{
    static unsigned int oldbuffer;
    static void BindBuffer(unsigned int name);
};

struct ProgramState
{
    static unsigned int oldname;

    static bool Bind(unsigned int name);
};

struct TextureState
{
    static int oldbinds[MAX_TEXTURE_UNITS];
    static int newbinds[MAX_TEXTURE_UNITS];
    static GLenum target[MAX_TEXTURE_UNITS];
    static unsigned int enabled;
    static unsigned int newenabled;

    static int currentunit;
    static bool locked;

    static void Lock(bool lock);

    static void Start();
    static void Disable();
    static void Disable(GLenum tgt);
    static void Enable(GLenum tgt = GL_TEXTURE_2D);
    static void End();
    static void SetTextureUnit(int unit);
    static void Bind(int unit, int name, GLenum tgt);
    static void Bind(int name, GLenum tgt);
};

