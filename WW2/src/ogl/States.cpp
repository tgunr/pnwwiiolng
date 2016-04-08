#include <syscfg.h> // Path is project specific.
/*--------------------------------------------------------------------------------------globalprogram
Module: States.cpp
Description:
---------------------------------------------------------------------------------------*/
#include "ogl/States.h"
#include "ogl/ogl.h"
#include "ogl/oglGlobalStates.h"
#include "shader/glsl.h"
#include "osgImport/Texture.h"
#include "osgImport/FrameBufferObject.h"

int TextureState::newbinds[MAX_TEXTURE_UNITS] = {-1, -1, -1, -1, -1, -1, -1};
int TextureState::oldbinds[MAX_TEXTURE_UNITS] = {-1, -1, -1, -1, -1, -1, -1};
int TextureState::currentunit = 0;

bool TextureState::locked = false;

unsigned int TextureState::enabled = 0x0;
unsigned int TextureState::newenabled = 0x0;
unsigned int ProgramState::oldname = 0;
unsigned int VertexArrayObjectBind::oldname = 0;
unsigned int TextureBufferObjectBind::oldbuffer = 0;
unsigned int BindBufferBase::oldname = 0;
unsigned int BindBufferBase::oldtarget = 0;
unsigned int BindBufferBase::oldindex = 0;
unsigned int ArrayBufferBind::oldname = 0;
unsigned int ElementBufferBind::oldname = 0;
unsigned int TextureBufferBind::oldname = 0;

GLenum TextureState::target[MAX_TEXTURE_UNITS] = {0, 0, 0, 0, 0, 0, 0};

#if USE_STATESET
static unsigned int TextureTargetBinds[MAX_TEXTURE_UNITS] = {0, 0, 0, 0, 0, 0, 0};
static bool LockedStates[RS_COUNT] = {false};
static std::set<int> HaveAppliedState;

static ModeMap modemap;
static ref_ptr<RenderStateSet> GlobalStateSet;
static ref_ptr<RenderState> DefaultRenderStates[RS_COUNT];
bool DepthMask::DepthWriteEnabled = true;

/*--------------------------------------------------------------------------------------
Function: SetGlobalDefaultModeValue
Description:
---------------------------------------------------------------------------------------*/
void States::HaveAppliedAttribute(int state)
{
    if(HaveAppliedState.find(state) == HaveAppliedState.end())
        HaveAppliedState.insert(state);
}

/*--------------------------------------------------------------------------------------
Function: LockState
Description:
---------------------------------------------------------------------------------------*/
void States::LockState(int state, bool lock)
{
    LockedStates[state] = lock;

    if(!lock)
        DefaultRenderStates[state]->apply();
}

/*--------------------------------------------------------------------------------------
Function: LockMode
Description:
---------------------------------------------------------------------------------------*/
void States::LockMode(GLenum mode, bool lock)
{
    ModeMap::iterator itr = modemap.find(mode);

    if(itr != modemap.end())
    {
        if(lock)
            itr->second.locked = true;
        else
        {
            itr->second.locked = false;
            itr->second.changed = true;
        }
    }
}

/*--------------------------------------------------------------------------------------
Function: SetGlobalDefaultModeValue
Description:
---------------------------------------------------------------------------------------*/
void States::SetGlobalDefaultModeValue(GLenum mode, bool enabled)
{
    if(!GlobalStateSet.valid())
        GlobalStateSet = new RenderStateSet;

    ModeStack& ms = modemap[mode];
    ms.global_default_value = enabled;

    GlobalStateSet->SetMode(mode, enabled);
}

/*--------------------------------------------------------------------------------------
Function: InitDefaultAttributes
Description:
---------------------------------------------------------------------------------------*/
void States::InitDefaultAttributes()
{
    DefaultRenderStates[RS_GLSLPROGRAM] = new Program;
    DefaultRenderStates[RS_MATERIAL] = new Material;
    DefaultRenderStates[RS_LIGHTMODEL] = new LightModel;
    DefaultRenderStates[RS_FOG] = new Fog;
    DefaultRenderStates[RS_POLYGONOFFSET] = new PolygonOffset;
    DefaultRenderStates[RS_ALPHAFUNC] = new AlphaFunc(GL_GREATER, 0.0f);
    DefaultRenderStates[RS_BLENDFUNC] = new BlendFunc;
    DefaultRenderStates[RS_COLORMASK] = new ColorMask;
    DefaultRenderStates[RS_CULLFACE] = new CullFace(GL_FRONT);
    DefaultRenderStates[RS_FRONTFACE] = new FrontFace;
    DefaultRenderStates[RS_DEPTHFUNC] = new DepthFunc(GL_GREATER);
    DefaultRenderStates[RS_CLEARDEPTH] = new ClearDepth(0.0f);
    DefaultRenderStates[RS_DEPTHMASK] = new DepthMask;
    DefaultRenderStates[RS_POLYGONMODE] = new PolygonMode;
    DefaultRenderStates[RS_SHADEMODEL] = new ShadeModel;

    if(GLEW_VERSION_1_4)
        DefaultRenderStates[RS_BLENDEQUATION] = new BlendEquation;

    if(GLEW_VERSION_1_3 || GLEW_ARB_multisample)
        DefaultRenderStates[RS_SAMPLECOVERAGE] = new SampleCoverage;

    DefaultRenderStates[RS_PIXELTRANSFER] = new PixelTransfer;
    DefaultRenderStates[RS_HINT] = new Hint;
    DefaultRenderStates[RS_LOGICOP] = new LogicOp;
    DefaultRenderStates[RS_DEPTHRANGE] = new DepthRange(1.0f, 0.0f);
    DefaultRenderStates[RS_LINEWIDTH] = new LineWidth;
    DefaultRenderStates[RS_POINTSIZE] = new PointSize;
    DefaultRenderStates[RS_LINESTIPPLE] = new LineStipple;
    DefaultRenderStates[RS_POLYGONSTIPPLE] = new PolygonStipple;

    if(GLEW_VERSION_1_4 || GLEW_ARB_point_parameters)
        DefaultRenderStates[RS_POINTPARAMETER] = new PointParameter;

    if(GLEW_VERSION_1_4 || GLEW_EXT_blend_color)
        DefaultRenderStates[RS_BLENDCOLOR] = new BlendColor;

    DefaultRenderStates[RS_VIEWPORT] = new Viewport(0, 0, WantedWidth, WantedHeight);
    DefaultRenderStates[RS_FRAMEBUFFEROBJECT] = new FrameBufferObject;
    DefaultRenderStates[RS_COLOR4F] = new Color4F;
    DefaultRenderStates[RS_DRAWBUFFER] = new DrawBuffer;
    DefaultRenderStates[RS_READBUFFER] = new ReadBuffer;
    DefaultRenderStates[RS_STENCILFUNC] = new StencilFunc;
    DefaultRenderStates[RS_STENCILOP] = new StencilOp;
    DefaultRenderStates[RS_STENCILMASK] = new StencilMask;

    for(int i = 0; i < MAX_TEXTURE_UNITS; i++)
    {
        DefaultRenderStates[RS_TEXTURE_UNIT0 + i] = new TextureUnit(i);
        DefaultRenderStates[RS_TEXTURE_MATRIX0 + i] = new TextureMatrix(i);
        DefaultRenderStates[RS_TEXGEN0 + i] = new TexGen(i);
        DefaultRenderStates[RS_TEXENV0 + i] = new TexEnv(i);
    }

    if(!GlobalStateSet.valid())
        GlobalStateSet = new RenderStateSet;

    for(int i = 0; i < RS_COUNT; i++)
    {
        LockedStates[i] = false;
        GlobalStateSet->SetRenderState(DefaultRenderStates[i].get());
    }

    States::ApplyRenderStates(GlobalStateSet.get());
}

/*--------------------------------------------------------------------------------------
Function: IsModeEnabled
Description:
---------------------------------------------------------------------------------------*/
bool States::IsModeEnabled(GLenum mode)
{
    ModeStack& ms = modemap[mode];

    return ms.last_applied_value;
}

/*--------------------------------------------------------------------------------------
Function: HaveAppliedMode
Description:
---------------------------------------------------------------------------------------*/
void States::HaveAppliedMode(GLenum mode, int value)
{
    ModeStack& ms = modemap[mode];

    ms.last_applied_value = value;

    // Will need to disable this mode on next apply so set it to changed.
    ms.changed = true;
}

/*--------------------------------------------------------------------------------------
Function: ApplyMode
Description:
---------------------------------------------------------------------------------------*/
static bool ApplyMode(GLenum mode, bool enabled, ModeStack& ms)
{
    if(ms.valid && ms.last_applied_value != enabled && !ms.locked)
    {
        ms.last_applied_value = enabled;

        if(enabled)
            glEnable(mode);
        else
            glDisable(mode);

        return true;
    }
    else
        return false;
}

/*--------------------------------------------------------------------------------------
Function: ApplyRenderMode
Description:
---------------------------------------------------------------------------------------*/
void States::ApplyRenderMode(GLenum mode, unsigned int value)
{
    ModeStack& ms = modemap[mode];

    if(ApplyMode(mode, value, ms))
    {
        // Will need to disable this mode on next apply so set it to changed.
        ms.changed = true;
    }
}

/*--------------------------------------------------------------------------------------
Function: GetDefaultAttribute
Description:
---------------------------------------------------------------------------------------*/
RenderState* States::GetDefaultAttribute(int attrib)
{
    if(attrib >= RS_COUNT)
        return nullptr;

    return DefaultRenderStates[attrib].get();
}

/*--------------------------------------------------------------------------------------
Function: ApplyRenderModes
Description:
---------------------------------------------------------------------------------------*/
void States::ApplyRenderModes(RenderStateSet* cur)
{
    logTimerBeginMacro();

    RenderStateSet::ModeList& modes = GlobalStateSet->GetModes();

    if(cur != nullptr)
        modes = cur->GetModes();

    RenderStateSet::ModeList::iterator modeitr = modes.begin();
    ModeMap::iterator modemapitr = modemap.begin();

    while(modemapitr != modemap.end() && modeitr != modes.end())
    {
        if(modemapitr->first < modeitr->first)
        {
            ModeStack& ms = modemapitr->second;

            if(ms.changed)
            {
                ms.changed = false;

                if(!ms.valueVec.empty())
                {
                    ApplyMode(modemapitr->first, ms.valueVec.back(), ms);
                }
                else
                {
                    ApplyMode(modemapitr->first, ms.global_default_value, ms);
                }
            }

            ++modemapitr;
        }
        else if(modeitr->first < modemapitr->first)
        {
            ModeStack& ms = modemap[modeitr->first];

            bool new_value = modeitr->second;
            ApplyMode(modeitr->first, new_value, ms);

            ms.changed = true;

            ++modeitr;
        }
        else
        {
            ModeStack& ms = modemapitr->second;

            if(!ms.valueVec.empty())
            {
                if(ms.changed)
                {
                    ms.changed = false;
                    bool new_value = ms.valueVec.back();
                    ApplyMode(modemapitr->first, new_value, ms);
                }
            }
            else
            {
                bool new_value = modeitr->second;

                if(ApplyMode(modeitr->first, new_value, ms))
                {
                    ms.changed = true;
                }
            }

            ++modemapitr;
            ++modeitr;
        }
    }

    // iterator over the remaining state modes to apply any previous changes.
    for(; modemapitr != modemap.end(); ++modemapitr)
    {
        // note GLMode = this_mitr->first
        ModeStack& ms = modemapitr->second;

        if(ms.changed)
        {
            ms.changed = false;

            if(!ms.valueVec.empty())
            {
                bool new_value = ms.valueVec.back();
                ApplyMode(modemapitr->first, new_value, ms);
            }
            else
            {
                // assume default of disabled.
                ApplyMode(modemapitr->first, ms.global_default_value, ms);
            }
        }
    }

    // iterator over the remaining incoming modes to apply any new mode.
    for(; modeitr != modes.end(); ++modeitr)
    {
        ModeStack& ms = modemap[modeitr->first];

        bool new_value = modeitr->second;
        ApplyMode(modeitr->first, new_value, ms);

        // will need to disable this mode on next apply so set it to changed.
        ms.changed = true;
    }

    logTimerEndMacro();
}

/*--------------------------------------------------------------------------------------
Function: ApplyRenderStates
Description:
---------------------------------------------------------------------------------------*/
void States::ApplyRenderStates(const RenderStateSet* cur)
{
    static int StateTable[RS_COUNT];
    static RenderState* CurrentStates[RS_COUNT];
    static const RenderStateSet* prev = nullptr;
    static bool first = true;

    logTimerBeginMacro();

    if(first)
    {
        first = false;
        memset(CurrentStates, 0, sizeof(RenderState*) * RS_COUNT);
    }

    if(prev == nullptr)
        memset(StateTable, 0, sizeof(int) * RS_COUNT);

    if(cur)
    {
        if(!HaveAppliedState.empty())
        {
            std::set<int>::iterator itr = HaveAppliedState.begin();

            // Set these to default.
            for(; itr != HaveAppliedState.end(); ++itr)
            {
                int type = *itr;

                CurrentStates[type] = DefaultRenderStates[type].get();

                if(!LockedStates[type])
                    DefaultRenderStates[type]->apply();
            }

            HaveAppliedState.clear();
        }

        for(unsigned int i = 0; i < cur->GetRenderStateSize(); i++)
        {
            const RenderState* rs = cur->GetRenderState(i);

            StateTable[rs->type()]++;

            if(CurrentStates[rs->type()] != rs || rs->IsDirty())
            {
                const_cast<RenderState*>(rs)->SetDirty(false);

                if(rs->IsEnabled())
                {
                    CurrentStates[rs->type()] = (RenderState*)rs;

                    if(!LockedStates[rs->type()])
                        rs->apply();
                }
                else
                {
                    CurrentStates[rs->type()] = DefaultRenderStates[rs->type()].get();

                    if(!LockedStates[rs->type()])
                        DefaultRenderStates[rs->type()]->apply();
                }
            }
        }
    }

    if(prev)
    {
        for(unsigned int i = 0; i < prev->GetRenderStateSize(); i++)
        {
            const RenderState* rs = prev->GetRenderState(i);

            if(StateTable[rs->type()] == 1)
            {
                CurrentStates[rs->type()] = DefaultRenderStates[rs->type()].get();

                if(!LockedStates[rs->type()])
                    DefaultRenderStates[rs->type()]->apply();
            }

            StateTable[rs->type()] >>= 1;
        }
    }
    else
    {
        memset(CurrentStates, 0, sizeof(RenderState*) * RS_COUNT);
    }

    prev = cur;

    logTimerEndMacro();
}
#endif

/*--------------------------------------------------------------------------------------
Function: TextureState::Lock
Description:
---------------------------------------------------------------------------------------*/
void TextureState::Lock(bool lock)
{
    locked = lock;
}

/*--------------------------------------------------------------------------------------
Function: TextureState::Bind
Description:
---------------------------------------------------------------------------------------*/
void TextureState::Bind(int name, GLenum tgt)
{
    if(locked)
        return;

    newbinds[currentunit] = name;

    if(oldbinds[currentunit] != name || target[currentunit] != tgt)
    {
        oldbinds[currentunit] = name;

        if(name)
        {
            Enable(tgt);
            glBindTexture(tgt, name);
        }
        else
            Disable(target[currentunit]);
    }
}

/*--------------------------------------------------------------------------------------
Function: TextureState::Bind
Description:
---------------------------------------------------------------------------------------*/
void TextureState::Bind(int unit, int name, GLenum tgt)
{
    if(locked)
        return;

    newbinds[unit] = name;

    if(name)
        newenabled |= 0x1 << unit;

    if(name == 0 && oldbinds[unit] == -1)
    {
        // Do nothing. Texture unit is not bound
    }
    else if(oldbinds[unit] != name || target[unit] != tgt)
    {
        oldbinds[unit] = name;
        SetTextureUnit(unit);

        if(name)
        {
            Enable(tgt);
            glBindTexture(tgt, name);
        }
        else
            Disable(target[currentunit]);

#if USE_STATESET
        States::HaveAppliedAttribute(RS_TEXTURE_UNIT0 + currentunit);
#endif
    }
}

/*--------------------------------------------------------------------------------------
Function: TextureState::SetTextureUnit
Description:
---------------------------------------------------------------------------------------*/
void TextureState::SetTextureUnit(int unit)
{
    if(currentunit != unit)
    {
        currentunit = unit;
        glActiveTextureARB(GL_TEXTURE0_ARB + unit);
    }
}

/*--------------------------------------------------------------------------------------
Function: TextureState::Start
Description:
---------------------------------------------------------------------------------------*/
void TextureState::Start()
{
    if(locked)
        return;

    newenabled = 0;
    memset(newbinds, -1, MAX_TEXTURE_UNITS * sizeof(int));
}

/*--------------------------------------------------------------------------------------
Function: TextureState::Disable
Description:
---------------------------------------------------------------------------------------*/
void TextureState::Disable(GLenum tgt)
{
    if(locked)
        return;

    if(enabled & (0x1 << currentunit))
    {
        enabled &= ~(0x1 << currentunit);

	    if(tgt != GL_TEXTURE_BUFFER &&
           tgt != GL_TEXTURE_2D_ARRAY_EXT)
        {
            glDisable(tgt);
        }

        glBindTexture(tgt, 0);

        target[currentunit] = 0;
        newbinds[currentunit] = -1;
    }

    oldbinds[currentunit] = -1;
}

/*--------------------------------------------------------------------------------------
Function: TextureState::Disable
Description:
---------------------------------------------------------------------------------------*/
void TextureState::Disable()
{
    if(locked)
        return;

    if(enabled & (0x1 << currentunit))
    {
        enabled &= ~(0x1 << currentunit);

	    if(target[currentunit] != GL_TEXTURE_BUFFER &&
           target[currentunit] != GL_TEXTURE_2D_ARRAY_EXT)
        {
            glDisable(target[currentunit]);
        }

        glBindTexture(target[currentunit], 0);

        target[currentunit] = 0;
        newbinds[currentunit] = -1;
    }

    oldbinds[currentunit] = -1;
}

/*--------------------------------------------------------------------------------------
Function: TextureState::Enable
Description:
---------------------------------------------------------------------------------------*/
void TextureState::Enable(GLenum tgt)
{
    if(locked)
        return;

    if(!(enabled & (0x1 << currentunit)) || tgt != target[currentunit])
    {
        if(tgt != target[currentunit] && target[currentunit])
        {
            if(target[currentunit] != GL_TEXTURE_BUFFER &&
               target[currentunit] != GL_TEXTURE_2D_ARRAY_EXT)
            {
                Disable(target[currentunit]);
            }
        }

        enabled |= 0x1 << currentunit;

        if(tgt != GL_TEXTURE_BUFFER &&
           tgt != GL_TEXTURE_2D_ARRAY_EXT)
        {
            glEnable(tgt);
        }

        target[currentunit] = tgt;
    }
}

/*--------------------------------------------------------------------------------------
Function: TextureState::End
Description:
---------------------------------------------------------------------------------------*/
void TextureState::End()
{
    if(locked)
        return;

    logTimerBeginMacro();

    unsigned int different;
    unsigned int test = 0x1;
    unsigned int unit = 0;

    if(newenabled != enabled)
    {
        // Units which are different
        different = newenabled ^ enabled;

        while(test <= different)
        {
            if(!(test & newenabled))
            {
                if(different & test)
                {
                    SetTextureUnit(unit);
                    Disable(target[currentunit]);
                }
            }

            unit++;
            test <<= 0x1;
        }
    }

    logTimerEndMacro();
}

/*--------------------------------------------------------------------------------------
Function: ProgramState::Bind
Description:
---------------------------------------------------------------------------------------*/
bool ProgramState::Bind(unsigned int name)
{
    if(name != oldname)
    {
        glUseProgram(name);

        oldname = name;

        return true;
    }

    return false;
}

/*--------------------------------------------------------------------------------------
Function: TextureBufferObjectBind::BindBuffer
Description:
---------------------------------------------------------------------------------------*/
void TextureBufferObjectBind::BindBuffer(unsigned int name)
{
    if(GLEW_ARB_texture_buffer_object)
    {
        TextureBufferBind::Bind(name);
    }
}

#if USE_STATESET
/*--------------------------------------------------------------------------------------
Function: TextureMatrix::apply
Description:
---------------------------------------------------------------------------------------*/
void TextureMatrix::apply() const
{
    if(ogl::UseDeprecatedPath())
    {
        TextureState::SetTextureUnit(TextureUnit);
        oglSetMatrixMode(GL_TEXTURE);
        glLoadMatrixf(matrix.ptr());
        oglSetMatrixMode(GL_MODELVIEW);
    }
    else
    {
        glslSetTextureMatrix(TextureUnit, matrix);
    }
}

/*--------------------------------------------------------------------------------------
Function: TextureUnit::apply
Description:
----------------------------------------------------------------------ss-----------------*/
void TextureUnit::apply() const
{
    extern TEXTURE* textures;
    GLenum prevtarget = TextureTargetBinds[texunit];

    TextureState::SetTextureUnit(texunit);

    if(prevtarget)
    {
        switch(prevtarget)
        {
        case GL_TEXTURE_1D_ARRAY:
        case GL_TEXTURE_2D_ARRAY:
        case GL_TEXTURE_2D_MULTISAMPLE:
        case GL_TEXTURE_2D_MULTISAMPLE_ARRAY:
        case GL_TEXTURE_BUFFER:
            {
                break;
            }
        default:
            {
                if(target != prevtarget)
                {
                    TextureState::Disable(prevtarget);
                    TextureState::Bind(0, prevtarget);
                }
            }
        }
    }

    switch(target)
    {
    case GL_TEXTURE_1D_ARRAY:
    case GL_TEXTURE_2D_ARRAY:
    case GL_TEXTURE_2D_MULTISAMPLE:
    case GL_TEXTURE_2D_MULTISAMPLE_ARRAY:
    case GL_TEXTURE_BUFFER:
        {
            break;
        }
    default:
        {
            if(target != prevtarget)
                TextureState::Enable(target);
        }
    }

    TextureState::Bind(id, target);
    TextureTargetBinds[texunit] = target;
}

/*--------------------------------------------------------------------------------------
Function: Program::apply
Description:
---------------------------------------------------------------------------------------*/
void Program::apply() const
{
    if(program.valid())
    {
        program->Apply();
        program->ApplyUniformMap();
    }
    else
    {
        ProgramState::Bind(0);
    }
}

/*--------------------------------------------------------------------------------------
Function: Material::Material
Description:
---------------------------------------------------------------------------------------*/
Material::Material() :
    matid(-1)
{
    ColorMaterialEnabled = false;
    ColorMaterialFace = GL_FRONT_AND_BACK;
    ColorMaterial = GL_AMBIENT_AND_DIFFUSE;

    int id = oglFindMaterialName("default");

    if(id > -1)
    {
        SetMaterialID(id);
    }
    else
    {
        FrontAmbient.set(0.5f, 0.5f, 0.5f, 1.0f);
        FrontDiffuse.set(1.0f, 1.0f, 1.0f, 1.0f);
        FrontSpecular.set(0.0f, 0.0f, 0.0f, 1.0f);
        FrontEmission.set(0.0f, 0.0f, 0.0f, 1.0f);
        FrontShininess = 0.0f;
    }

    if(!FrontAmbient.valid())
    {
        int stoop  = 0;
    }

    matid = id;
}

/*--------------------------------------------------------------------------------------
Function: Material::Material
Description:
---------------------------------------------------------------------------------------*/
Material::Material(int id) :
    matid(-1)
{
    if(!oglIsMaterial(id))
    {
        id = oglFindMaterialName("default");
    }

    SetMaterialID(id);
}

/*--------------------------------------------------------------------------------------
Function: Material::SetMaterialID
Description:
---------------------------------------------------------------------------------------*/
void Material::SetMaterialID(int id)
{
    MATERIAL* materials;
    oglGetMaterials(&materials);

    if(id != matid)
    {
        matid = id;

        if(oglIsMaterial(matid))
        {
            const piMATERIAL& m = oglGetMaterial(matid);

            FrontAmbient.set(m.ambient.r, m.ambient.g, m.ambient.b, m.ambient.a);
            FrontDiffuse.set(m.diffuse.r, m.diffuse.g, m.diffuse.b, m.diffuse.a);
            FrontSpecular.set(m.specular.r, m.specular.g, m.specular.b, m.specular.a);
            FrontEmission.set(m.emissive.r, m.emissive.g, m.emissive.b, m.emissive.a);
            FrontShininess = m.power;

#if(0)
            BackAmbient.set(m.ambient.r, m.ambient.g, m.ambient.b, m.ambient.a);
            BackDiffuse.set(m.diffuse.r, m.diffuse.g, m.diffuse.b, m.diffuse.a);
            BackSpecular.set(m.specular.r, m.specular.g, m.specular.b, m.specular.a);
            BackEmission.set(m.emissive.r, m.emissive.g, m.emissive.b, m.emissive.a);
            BackShininess = m.power;
#endif

            ColorMaterialEnabled = false;
            ColorMaterialFace = GL_FRONT_AND_BACK;
            ColorMaterial = GL_AMBIENT_AND_DIFFUSE;
        }
        else
        {
            matid = oglFindMaterialName("default");

            if(oglIsMaterial(matid))
            {
                const piMATERIAL& m = oglGetMaterial(matid);

                FrontAmbient.set(m.ambient.r, m.ambient.g, m.ambient.b, m.ambient.a);
                FrontDiffuse.set(m.diffuse.r, m.diffuse.g, m.diffuse.b, m.diffuse.a);
                FrontSpecular.set(m.specular.r, m.specular.g, m.specular.b, m.specular.a);
                FrontEmission.set(m.emissive.r, m.emissive.g, m.emissive.b, m.emissive.a);
                FrontShininess = m.power;
            }
#if(0)
            BackAmbient = Vec4f(0.2f, 0.2f, 0.2f, 1.0f);
            BackDiffuse = Vec4f(0.8f, 0.8f, 0.8f, 1.0f);
            BackSpecular = Vec4f(0.0f, 0.0f, 0.0f, 1.0f);
            BackEmission = Vec4f(0.0f, 0.0f, 0.0f, 1.0f);
            BackShininess = 0;
#endif

            ColorMaterialEnabled = false;
            ColorMaterialFace = GL_FRONT_AND_BACK;
            ColorMaterial = GL_AMBIENT_AND_DIFFUSE;
        }
    }
    else if(!oglIsMaterial(id))
    {
        matid = oglFindMaterialName("default");

        if(oglIsMaterial(matid))
        {
            const piMATERIAL& m = oglGetMaterial(matid);

            FrontAmbient.set(m.ambient.r, m.ambient.g, m.ambient.b, m.ambient.a);
            FrontDiffuse.set(m.diffuse.r, m.diffuse.g, m.diffuse.b, m.diffuse.a);
            FrontSpecular.set(m.specular.r, m.specular.g, m.specular.b, m.specular.a);
            FrontEmission.set(m.emissive.r, m.emissive.g, m.emissive.b, m.emissive.a);
            FrontShininess = m.power;
        }
    }
}

/*--------------------------------------------------------------------------------------
Function: Material::SetTransparency
Description:
---------------------------------------------------------------------------------------*/
void Material::SetTransparency(float alpha)
{
    FrontAmbient.a() = BackAmbient.a() = alpha;
    FrontDiffuse.a() = BackDiffuse.a() = alpha;
    FrontSpecular.a() = BackSpecular.a() = alpha;
    FrontEmission.a() = BackEmission.a() = alpha;
}

/*--------------------------------------------------------------------------------------
Function: Material::SetFrontTransparency
Description:
---------------------------------------------------------------------------------------*/
void Material::SetFrontTransparency(float alpha)
{
    FrontAmbient.a() = alpha;
    FrontDiffuse.a() = alpha;
    FrontSpecular.a() = alpha;
    FrontEmission.a() = alpha;
}

/*--------------------------------------------------------------------------------------
Function: Material::SetBackTransparency
Description:
---------------------------------------------------------------------------------------*/
void Material::SetBackTransparency(float alpha)
{
    BackAmbient.a()  = alpha;
    BackDiffuse.a()  = alpha;
    BackSpecular.a() = alpha;
    BackEmission.a() = alpha;
}

/*--------------------------------------------------------------------------------------
Function: Material::SetFrontFlatColor
Description:
---------------------------------------------------------------------------------------*/
void Material::SetFrontFlatColor(const Vec4f& color)
{
    FrontAmbient = Vec4f(0, 0, 0, 0);
    FrontDiffuse = Vec4f(0, 0, 0, 0);
    FrontSpecular = Vec4f(0, 0, 0, 0);
    FrontEmission = color;
    FrontShininess = 0;

    SetFrontTransparency(color.a());
}

/*--------------------------------------------------------------------------------------
Function: Material::SetBackFlatColor
Description:
---------------------------------------------------------------------------------------*/
void Material::SetBackFlatColor(const Vec4f& color)
{
    BackAmbient = Vec4f(0, 0, 0, 0);
    BackDiffuse = Vec4f(0, 0, 0, 0);
    BackSpecular = Vec4f(0, 0, 0, 0);
    BackEmission = color;
    BackShininess = 0;

    SetBackTransparency(color.a());
}

/*--------------------------------------------------------------------------------------
Function: Material::SetFlatColor
Description:
---------------------------------------------------------------------------------------*/
void Material::SetFlatColor(const Vec4f& color)
{
    SetFrontFlatColor(color);
    SetBackFlatColor(color);
}

/*--------------------------------------------------------------------------------------
Function: Material::apply
Description:
---------------------------------------------------------------------------------------*/
void Material::apply() const
{
    static int PrevMaterial = 999999;

    if(PrevMaterial != matid)
    {
        if(ColorMaterialEnabled)
        {
            glColorMaterial(GetColorMaterialFace(), GetColorMaterial());
            States::ApplyRenderMode(GL_COLOR_MATERIAL, 1);
        }
        else
            States::ApplyRenderMode(GL_COLOR_MATERIAL, 0);

        if(ogl::UseDeprecatedPath())
        {
            if(matid > -1)
            {
#if(0)
                MATERIAL* materials;
                oglGetMaterials(&materials);

                if(materials[matid].displayList)
                {
                    glCallList(materials[matid].displayList);
                }
                else
#endif
                {
#if(0)
                    const_cast<Material*>(this)->SetMaterialID(matid);

                    materials[matid].displayList = glGenLists(1);
                    glNewList(materials[matid].displayList, GL_COMPILE);
#endif
                    glMaterialfv(GL_FRONT, GL_AMBIENT, (float*)FrontAmbient.ptr());
                    glMaterialfv(GL_FRONT, GL_DIFFUSE, (float*)FrontDiffuse.ptr());
                    glMaterialfv(GL_FRONT, GL_SPECULAR, (float*)FrontSpecular.ptr());
                    glMaterialfv(GL_FRONT, GL_EMISSION, (float*)FrontEmission.ptr());
                    glMaterialf(GL_FRONT, GL_SHININESS, FrontShininess);
#if(0)
                    glMaterialfv(GL_BACK, GL_AMBIENT, BackAmbient.ptr());
                    glMaterialfv(GL_BACK, GL_DIFFUSE, BackDiffuse.ptr());
                    glMaterialfv(GL_BACK, GL_SPECULAR, BackSpecular.ptr());
                    glMaterialfv(GL_BACK, GL_EMISSION, BackEmission.ptr());
                    glMaterialf(GL_BACK, GL_SHININESS, BackShininess);
#endif
#if(0)
                    glEndList();
#endif
                }
            }
            else
            {
                const_cast<Material*>(this)->matid = oglFindMaterialName("default");
                const_cast<Material*>(this)->SetMaterialID(matid);
            }
        }

        PrevMaterial = matid;
    }
}

/*--------------------------------------------------------------------------------------
Function: LightModel::apply
Description:
---------------------------------------------------------------------------------------*/
void LightModel::apply() const
{
    if(GLEW_VERSION_1_2 || GLEW_EXT_separate_specular_color)
    {
        glLightModeli(GL_LIGHT_MODEL_COLOR_CONTROL, ColorControl);
    }

    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, AmbientColor.ptr());
    glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER, LocalViewer?1:0);
    glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, TwoSide ? 1: 0);
}

/*--------------------------------------------------------------------------------------
Function: Fog::apply
Description:
---------------------------------------------------------------------------------------*/
void Fog::apply() const
{
    glFogi(GL_FOG_MODE, Mode);
    glFogfv(GL_FOG_COLOR, Color.ptr());
    glFogf(GL_FOG_DENSITY, Density);
    glFogf(GL_FOG_START, Start);
    glFogf(GL_FOG_END, End);
}

/*--------------------------------------------------------------------------------------
Function: PolygonOffset::apply
Description:
---------------------------------------------------------------------------------------*/
void PolygonOffset::apply() const
{
    static float PrevFactor = 0.0f;
    static float PrevUnits = 0.0f;

    if(PrevFactor != Factor || PrevUnits != Units)
    {
        glPolygonOffset(Factor, Units);
        PrevFactor = Factor;
        PrevUnits = Units;
    }
}

/*--------------------------------------------------------------------------------------
Function: AlphaFunc::apply
Description:
---------------------------------------------------------------------------------------*/
void AlphaFunc::apply() const
{
    static GLenum PrevFunc = GL_ALWAYS;
    static GLclampf PrevRef = 0.0f;

    if(PrevFunc != Func || PrevRef != RefValue)
    {
        glAlphaFunc(Func, RefValue);
        PrevFunc = Func;
        PrevRef = RefValue;
    }
}

/*--------------------------------------------------------------------------------------
Function: BlendFunc::apply
Description:
---------------------------------------------------------------------------------------*/
void BlendFunc::apply() const
{
    static GLenum PrevSrcRGB = GL_ONE;
    static GLenum PrevDstRGB = GL_ZERO;
    static GLenum PrevSrcAlpha = GL_ONE;
    static GLenum PrevDstAlpha = GL_ZERO;

    if(GLEW_VERSION_1_4 || GLEW_EXT_blend_func_separate)
    {
        if(PrevSrcRGB != SrcRGB || PrevDstRGB != DstRGB || PrevSrcAlpha != SrcAlpha || PrevDstAlpha != DstAlpha)
        {
            glBlendFuncSeparate(SrcRGB, DstRGB, SrcAlpha, DstAlpha);

            PrevSrcRGB = SrcRGB;
            PrevDstRGB = DstRGB;
            PrevSrcAlpha = SrcAlpha;
            PrevDstAlpha = DstAlpha;
        }

    }
    else
    {
        if(PrevSrcRGB != SrcRGB || PrevDstRGB != DstRGB)
        {
            glBlendFunc(SrcRGB, DstRGB);

            PrevSrcRGB = SrcRGB;
            PrevDstRGB = DstRGB;
        }
    }
}

/*--------------------------------------------------------------------------------------
Function: CullFace::apply
Description:
---------------------------------------------------------------------------------------*/
void CullFace::apply() const
{
    static GLenum PrevFace = GL_BACK;

    if(PrevFace != FaceMode)
    {
        glCullFace(FaceMode);
        PrevFace = FaceMode;
    }
}

/*--------------------------------------------------------------------------------------
Function: FrontFace::apply
Description:
---------------------------------------------------------------------------------------*/
void FrontFace::apply() const
{
    static GLenum PrevFace = GL_CCW;

    if(PrevFace != frontface)
    {
        glFrontFace(frontface);
        PrevFace = frontface;
    }
}

/*--------------------------------------------------------------------------------------
Function: DepthFunc::apply
Description:
---------------------------------------------------------------------------------------*/
void DepthFunc::apply() const
{
    static GLenum PrevFunc = GL_LESS;

    if(PrevFunc != Func)
    {
        glDepthFunc(Func);
        PrevFunc = Func;
    }
}

/*--------------------------------------------------------------------------------------
Function: ClearDepth::apply
Description:
---------------------------------------------------------------------------------------*/
void ClearDepth::apply() const
{
    static float PrevValue = -1.0;

    if(PrevValue != ClearValue)
    {
        glClearDepth(ClearValue);
        PrevValue = ClearValue;
    }
}

/*--------------------------------------------------------------------------------------
Function: DepthMask::apply
Description:
---------------------------------------------------------------------------------------*/
void DepthMask::apply() const
{
    if(DepthWriteEnabled != Mask)
    {
        glDepthMask(Mask ? GL_TRUE : GL_FALSE);
        DepthWriteEnabled = Mask;
    }
}

/*--------------------------------------------------------------------------------------
Function: PolygonMode::apply
Description:
---------------------------------------------------------------------------------------*/
void PolygonMode::apply() const
{
    // Required by GL 3.1 CORE
    if(FrontFace == BackFace)
    {
        glPolygonMode(GL_FRONT_AND_BACK, FrontFace);
    }
    else
    {
        glPolygonMode(GL_FRONT, FrontFace);
        glPolygonMode(GL_BACK, BackFace);
    }
}

/*--------------------------------------------------------------------------------------
Function: ShadeModel::apply
Description:
---------------------------------------------------------------------------------------*/
void ShadeModel::apply() const
{
    static GLenum PrevModel = GL_SMOOTH;

    if(PrevModel != Model)
    {
        glShadeModel(Model);
        PrevModel = Model;
    }
}

/*--------------------------------------------------------------------------------------
Function: BlendEquation::apply
Description:
---------------------------------------------------------------------------------------*/
void BlendEquation::apply() const
{
    if (GLEW_VERSION_2_0 || GLEW_EXT_blend_equation_separate)
        glBlendEquationSeparate(ModeRGB, ModeAlpha);
    else
        glBlendEquation(ModeRGB);
}

/*--------------------------------------------------------------------------------------
Function: glSampleCoverage::apply
Description:
---------------------------------------------------------------------------------------*/
void SampleCoverage::apply() const
{
    glSampleCoverage(Value, Invert ? GL_TRUE : GL_FALSE);
}

/*--------------------------------------------------------------------------------------
Function: PixelTransfer::apply
Description:
---------------------------------------------------------------------------------------*/
void PixelTransfer::apply() const
{
    glPixelTransferi(GL_MAP_COLOR, GetMapColor() ? GL_TRUE : GL_FALSE);
    glPixelTransferi(GL_MAP_STENCIL, GetMapStencil() ? GL_TRUE : GL_FALSE);
    glPixelTransferi(GL_INDEX_SHIFT, GetIndexShift());
    glPixelTransferi(GL_INDEX_OFFSET, GetIndexOffset());
    glPixelTransferf(GL_RED_SCALE, GetRedScale());
    glPixelTransferf(GL_GREEN_SCALE, GetGreenScale());
    glPixelTransferf(GL_BLUE_SCALE, GetBlueScale());
    glPixelTransferf(GL_ALPHA_SCALE, GetAlphaScale());
    glPixelTransferf(GL_DEPTH_SCALE, GetDepthScale());
    glPixelTransferf(GL_RED_BIAS, GetRedBias());
    glPixelTransferf(GL_GREEN_BIAS, GetGreenBias());
    glPixelTransferf(GL_BLUE_BIAS, GetBlueBias());
    glPixelTransferf(GL_ALPHA_BIAS, GetAlphaBias());
    glPixelTransferf(GL_DEPTH_BIAS, GetDepthBias());

    if(GLEW_ARB_imaging)
    {
        glPixelTransferf(GL_POST_COLOR_MATRIX_RED_SCALE, GetPostColorMatrixRedScale());
        glPixelTransferf(GL_POST_COLOR_MATRIX_GREEN_SCALE, GetPostColorMatrixGreenScale());
        glPixelTransferf(GL_POST_COLOR_MATRIX_BLUE_SCALE, GetPostColorMatrixBlueScale());
        glPixelTransferf(GL_POST_COLOR_MATRIX_ALPHA_SCALE, GetPostColorMatrixAlphaScale());
        glPixelTransferf(GL_POST_COLOR_MATRIX_RED_BIAS, GetPostColorMatrixRedBias());
        glPixelTransferf(GL_POST_COLOR_MATRIX_GREEN_BIAS, GetPostColorMatrixGreenBias());
        glPixelTransferf(GL_POST_COLOR_MATRIX_BLUE_BIAS, GetPostColorMatrixBlueBias());
        glPixelTransferf(GL_POST_COLOR_MATRIX_ALPHA_BIAS, GetPostColorMatrixAlphaBias());
        glPixelTransferf(GL_POST_CONVOLUTION_RED_SCALE, GetPostConvolutionRedScale());
        glPixelTransferf(GL_POST_CONVOLUTION_GREEN_SCALE, GetPostConvolutionGreenScale());
        glPixelTransferf(GL_POST_CONVOLUTION_BLUE_SCALE, GetPostConvolutionBlueScale());
        glPixelTransferf(GL_POST_CONVOLUTION_ALPHA_SCALE, GetPostConvolutionAlphaScale());
        glPixelTransferf(GL_POST_CONVOLUTION_RED_BIAS, GetPostConvolutionRedBias());
        glPixelTransferf(GL_POST_CONVOLUTION_GREEN_BIAS, GetPostConvolutionGreenBias());
        glPixelTransferf(GL_POST_CONVOLUTION_BLUE_BIAS, GetPostConvolutionBlueBias());
        glPixelTransferf(GL_POST_CONVOLUTION_ALPHA_BIAS, GetPostConvolutionAlphaBias());
    }
}

/*--------------------------------------------------------------------------------------
Function: Hint::apply
Description:
---------------------------------------------------------------------------------------*/
void Hint::apply() const
{
    glHint(GL_PERSPECTIVE_CORRECTION_HINT, PerspectiveCorrectionHint);
    glHint(GL_FOG_HINT, FogHint);

    if(GLEW_SGIS_generate_mipmap || GLEW_VERSION_1_4)
    {
        glHint(GL_GENERATE_MIPMAP_HINT, GenerateMipmapHint);
    }

    glHint(GL_POLYGON_SMOOTH_HINT, PolygonSmoothHint);
    glHint(GL_LINE_SMOOTH_HINT, LineSmoothHint);
    glHint(GL_POINT_SMOOTH_HINT, PointSmoothHint);
}

/*--------------------------------------------------------------------------------------
Function: LogicOp::apply
Description:
---------------------------------------------------------------------------------------*/
void LogicOp::apply() const
{
    glLogicOp(Op);
}

/*--------------------------------------------------------------------------------------
Function: DepthRange::apply
Description:
---------------------------------------------------------------------------------------*/
void DepthRange::apply() const
{
    glDepthRange(ZNear, ZFar);
}

/*--------------------------------------------------------------------------------------
Function: LineWidth::apply
Description:
---------------------------------------------------------------------------------------*/
void LineWidth::apply() const
{
    static float PrevWidth = 1.0f;

    if(PrevWidth != Width)
    {
        glLineWidth(Width);
        PrevWidth = Width;
    }
}

/*--------------------------------------------------------------------------------------
Function: PointSize::apply
Description:
---------------------------------------------------------------------------------------*/
void PointSize::apply() const
{
    static float PrevSize = 1.0f;

    if(PrevSize != Size)
    {
        glPointSize(Size);
        PrevSize = Size;
    }
}

/*--------------------------------------------------------------------------------------
Function: PolygonStipple::PolygonStipple
Description:
---------------------------------------------------------------------------------------*/
PolygonStipple::PolygonStipple()
{
    memset(Mask, 0xFF, sizeof(unsigned char) * 32 * 32 / 8);
}

/*--------------------------------------------------------------------------------------
Function: PolygonStipple::PolygonStipple
Description:
---------------------------------------------------------------------------------------*/
PolygonStipple::PolygonStipple(const unsigned char* mask)
{
    Set(mask);
}

/*--------------------------------------------------------------------------------------
Function: PolygonStipple::Set
Description:
---------------------------------------------------------------------------------------*/
void PolygonStipple::Set(const unsigned char* mask)
{
    memcpy(Mask, mask, sizeof(unsigned char) * 32 * 32 / 8);
}

/*--------------------------------------------------------------------------------------
Function: PolygonStipple::apply
Description:
---------------------------------------------------------------------------------------*/
void PolygonStipple::apply() const
{
    glPolygonStipple(GetMask());
}

/*--------------------------------------------------------------------------------------
Function: LineStipple::apply
Description:
---------------------------------------------------------------------------------------*/
void LineStipple::apply() const
{
    glLineStipple(Factor, Pattern);
}

/*--------------------------------------------------------------------------------------
Function: PointParameter::apply
Description:
---------------------------------------------------------------------------------------*/
void PointParameter::apply() const
{
    if(GLEW_VERSION_1_4)
    {
        glPointParameterf(GL_POINT_SIZE_MIN, SizeMin);
        glPointParameterf(GL_POINT_SIZE_MAX, SizeMax);
        glPointParameterfv(GL_POINT_DISTANCE_ATTENUATION, (const float*)DistanceAttenuation.ptr());
    }

    if(GLEW_VERSION_1_4 || GLEW_VERSION_3_0)
    {
        glPointParameterf(GL_POINT_FADE_THRESHOLD_SIZE, FadeThresholdSize);
    }

    if (GLEW_VERSION_2_0||GLEW_VERSION_3_0||GLEW_VERSION_4_0)
    {
        glPointParameteri(GL_POINT_SPRITE_COORD_ORIGIN, PointSpriteCoordOrigin);
    }
}

/*--------------------------------------------------------------------------------------
Function: BlendColor::apply
Description:
---------------------------------------------------------------------------------------*/
void BlendColor::apply() const
{
    glBlendColor(Color.r(), Color.g(), Color.b(), Color.a());
}

/*--------------------------------------------------------------------------------------
Function: StencilFunc::apply
Description:
---------------------------------------------------------------------------------------*/
void StencilFunc::apply() const
{
    if(GLEW_VERSION_2_0)
    {
        glStencilFuncSeparate(GL_FRONT, Function_Front, RefValue_Front, Mask_Front);
        glStencilFuncSeparate(GL_BACK, Function_Back, RefValue_Back, Mask_Back);
    }
    else
    {
        glStencilFunc(Function_Front, RefValue_Front, Mask_Front);
    }
}

/*--------------------------------------------------------------------------------------
Function: StencilOp::apply
Description:
---------------------------------------------------------------------------------------*/
void StencilOp::apply() const
{
    if(GLEW_VERSION_2_0)
    {
        glStencilOpSeparate(GL_FRONT, SFail_Front, DpFail_Front, DpPass_Front);
        glStencilOpSeparate(GL_BACK, SFail_Back, DpFail_Back, DpPass_Back);
    }
    else
    {
        glStencilOp(SFail_Front, DpFail_Front, DpPass_Front);
    }
}

/*--------------------------------------------------------------------------------------
Function: StencilMask::apply
Description:
---------------------------------------------------------------------------------------*/
void StencilMask::apply() const
{
    if(GLEW_VERSION_2_0)
    {
        glStencilMaskSeparate(GL_FRONT, Mask_Front);
        glStencilMaskSeparate(GL_BACK,  Mask_Back);
    }
    else
    {
        glStencilMask(Mask_Front);
    }
}

/*--------------------------------------------------------------------------------------
Function: DrawBuffer::apply
Description:
---------------------------------------------------------------------------------------*/
void DrawBuffer::apply() const
{
    static GLenum PrevBuffer = 0;

    if(PrevBuffer != Buffer)
    {
        glDrawBuffer(Buffer);
        PrevBuffer = Buffer;
    }
}

/*--------------------------------------------------------------------------------------
Function: ReadBuffer::apply
Description:
---------------------------------------------------------------------------------------*/
void ReadBuffer::apply() const
{
    static GLenum PrevBuffer = 0;

    if(PrevBuffer != Buffer)
    {
        glReadBuffer(Buffer);
        PrevBuffer = Buffer;
    }
}

/*--------------------------------------------------------------------------------------
Function: Color4F::apply
Description:
---------------------------------------------------------------------------------------*/
void Color4F::apply() const
{
    static float PrevRed = 1.0f;
    static float PrevGrn = 1.0f;
    static float PrevBlu = 1.0f;
    static float PrevAlf = 1.0f;

    if(PrevRed != red || PrevGrn != green || PrevBlu != blue || PrevAlf != alpha)
    {
        glColor4f(red, green, blue, alpha);

        PrevRed = red;
        PrevGrn = green;
        PrevBlu = blue;
        PrevAlf = alpha;
    }
}

/*--------------------------------------------------------------------------------------
Function: ColorMask::apply
Description:
---------------------------------------------------------------------------------------*/
void ColorMask::apply() const
{
    static bool PrevRed = true;
    static bool PrevGrn = true;
    static bool PrevBlu = true;
    static bool PrevAlf = true;

    if(PrevRed != Red || PrevGrn != Green || PrevBlu != Blue || PrevAlf != Alpha)
    {
        glColorMask(Red ? GL_TRUE : GL_FALSE,
                    Green ? GL_TRUE : GL_FALSE,
                    Blue ? GL_TRUE : GL_FALSE,
                    Alpha ? GL_TRUE : GL_FALSE);

        PrevRed = Red;
        PrevGrn = Green;
        PrevBlu = Blue;
        PrevAlf = Alpha;
    }
}

/*--------------------------------------------------------------------------------------
Function: TexEnv::TexEnv
Description:
---------------------------------------------------------------------------------------*/
TexEnv::TexEnv(int unit)
{
    texunit = unit;
    Mode = GL_MODULATE;
    Color = Vec4f(0, 0, 0, 0);

    // Combiner settings
    RGBScale = 1.0f;
    CombineRGB = GL_REPLACE;
    Source0RGB = GL_TEXTURE;
    Source1RGB = GL_TEXTURE;
    Source2RGB = GL_TEXTURE;
    Operand0RGB = GL_SRC_COLOR;
    Operand1RGB = GL_SRC_COLOR;
    Operand2RGB = GL_SRC_COLOR;

    AlphaScale  = 1.0f;
    CombineAlpha = GL_REPLACE;
    Source0Alpha = GL_TEXTURE;
    Source1Alpha = GL_TEXTURE;
    Source2Alpha = GL_TEXTURE;
    Operand0Alpha = GL_SRC_ALPHA;
    Operand1Alpha = GL_SRC_ALPHA;
    Operand2Alpha = GL_SRC_ALPHA;

    LodBias = 0.0f;
    PointSpriteCoordReplace = false;
}

/*--------------------------------------------------------------------------------------
Function: TexEnv::TexEnv
Description:
---------------------------------------------------------------------------------------*/
void TexEnv::apply() const
{
    TextureState::SetTextureUnit(GetTextureUnit());

    static GLenum PrevMode[8] = {0};

    if(PrevMode[GetTextureUnit()] != GetMode())
    {
        glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GetMode());
        PrevMode[GetTextureUnit()] = GetMode();
    }

    if(GetMode() == GL_BLEND)
    {
        glTexEnvfv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, GetColor().ptr());
    }

    if(GetMode() == GL_COMBINE && (GLEW_EXT_texture_env_combine || GLEW_VERSION_1_3))
    {
        static float PrevRGBScale[8] = {-1.0f};

        if(PrevRGBScale[GetTextureUnit()] != GetRGBScale())
        {
            glTexEnvf(GL_TEXTURE_ENV, GL_RGB_SCALE, GetRGBScale());
            PrevRGBScale[GetTextureUnit()] = GetRGBScale();
        }

        static GLenum PrevCombineRGB[8] = {0};

        if(PrevCombineRGB[GetTextureUnit()] != GetCombineRGB())
        {
            glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB, GetCombineRGB());
            PrevCombineRGB[GetTextureUnit()] = GetCombineRGB();
        }

        static GLenum PrevSource0RGB[8] = {0};

        if(PrevSource0RGB[GetTextureUnit()] != GetSource0RGB())
        {
            glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB, GetSource0RGB());
            PrevSource0RGB[GetTextureUnit()] = GetSource0RGB();
        }

        static GLenum PrevSource1RGB[8] = {0};

        if(PrevSource1RGB[GetTextureUnit()] != GetSource1RGB())
        {
            glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_RGB, GetSource1RGB());
            PrevSource1RGB[GetTextureUnit()] = GetSource1RGB();
        }

        static GLenum PrevOperand0RGB[8] = {0};

        if(PrevOperand0RGB[GetTextureUnit()] != GetOperand0RGB())
        {
            glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_RGB, GetOperand0RGB());
            PrevOperand0RGB[GetTextureUnit()] = GetOperand0RGB();
        }

        static GLenum PrevOperand1RGB[8] = {0};

        if(PrevOperand1RGB[GetTextureUnit()] != GetOperand1RGB())
        {
            glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND1_RGB, GetOperand1RGB());
            PrevOperand1RGB[GetTextureUnit()] = GetOperand1RGB();
        }

        if(GetCombineRGB() == GL_INTERPOLATE)
        {
            static GLenum PrevSource2RGB[8] = {0};
            static GLenum PrevOperand2RGB[8] = {0};

            if(PrevSource2RGB[GetTextureUnit()] != GetSource2RGB())
            {
                glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE2_RGB, GetSource2RGB());
                PrevSource2RGB[GetTextureUnit()] = GetSource2RGB();
            }

            if(PrevOperand2RGB[GetTextureUnit()] != GetOperand2RGB())
            {
                glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND2_RGB, GetOperand2RGB());
                PrevOperand2RGB[GetTextureUnit()] = GetOperand2RGB();
            }
        }

        static float PrevAlphaScale[8] = {-1.0f};

        if(PrevAlphaScale[GetTextureUnit()] != GetAlphaScale())
        {
            glTexEnvf(GL_TEXTURE_ENV, GL_ALPHA_SCALE, GetAlphaScale());
            PrevAlphaScale[GetTextureUnit()] = GetAlphaScale();
        }

        static GLenum PrevCombineAlpha[8] = {0};

        if(PrevCombineAlpha[GetTextureUnit()] != GetCombineAlpha())
        {
            glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA, GetCombineAlpha());
            PrevCombineAlpha[GetTextureUnit()] = GetCombineAlpha();
        }

        static GLenum PrevSource0Alpha[8] = {0};

        if(PrevSource0Alpha[GetTextureUnit()] != GetSource0Alpha())
        {
            glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_ALPHA, GetSource0Alpha());
            PrevSource0Alpha[GetTextureUnit()] = GetSource0Alpha();
        }

        static GLenum PrevSource1Alpha[8] = {0};

        if(PrevSource1Alpha[GetTextureUnit()] != GetSource1Alpha())
        {
            glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_ALPHA, GetSource1Alpha());
            PrevSource1Alpha[GetTextureUnit()] = GetSource1Alpha();
        }

        static GLenum PrevOperand0Alpha[8] = {0};

        if(PrevOperand0Alpha[GetTextureUnit()] != GetOperand0Alpha())
        {
            glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_ALPHA, GetOperand0Alpha());
            PrevOperand0Alpha[GetTextureUnit()] = GetOperand0Alpha();
        }

        static GLenum PrevOperand1Alpha[8] = {0};

        if(PrevOperand1Alpha[GetTextureUnit()] != GetOperand1Alpha())
        {
            glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND1_ALPHA, GetOperand1Alpha());
            PrevOperand1Alpha[GetTextureUnit()] = GetOperand1Alpha();
        }

        if(GetCombineAlpha() == GL_INTERPOLATE)
        {
            static GLenum PrevSource2Alpha[8] = {0};
            static GLenum PrevOperand2Alpha[8] = {0};

            if(PrevSource2Alpha[GetTextureUnit()] != GetSource2Alpha())
            {
                glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE2_ALPHA, GetSource2Alpha());
                PrevSource2Alpha[GetTextureUnit()] = GetSource2Alpha();
            }

            if(PrevOperand2Alpha[GetTextureUnit()] != GetOperand2Alpha())
            {
                glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND2_ALPHA, GetOperand2Alpha());
                PrevOperand2Alpha[GetTextureUnit()] = GetOperand2Alpha();
            }
        }
    }

    // No need to do it if point sprite is disabled but we cannot know it here
    if(GLEW_VERSION_2_0 || GLEW_ARB_point_sprite)
    {
        static bool PrevPointSpriteCoordReplace = true;

        if(PrevPointSpriteCoordReplace != PointSpriteCoordReplace)
        {
            glTexEnvi( GL_POINT_SPRITE_ARB, GL_COORD_REPLACE_ARB, PointSpriteCoordReplace ? GL_TRUE : GL_FALSE );
            PrevPointSpriteCoordReplace = PointSpriteCoordReplace;
        }
    }

    if(GLEW_VERSION_1_4 || GLEW_EXT_texture_lod_bias)
    {
        static float PrevLodBias = -1.0f;

        if(PrevLodBias != LodBias)
        {
            glTexEnvf(GL_TEXTURE_FILTER_CONTROL, GL_TEXTURE_LOD_BIAS, LodBias);
            PrevLodBias = LodBias;
        }
    }
}

/*--------------------------------------------------------------------------------------
Function: TexGen::TexGen
Description:
---------------------------------------------------------------------------------------*/
TexGen::TexGen(int unit)
{
    texunit = texunit;
    EyePlaneS = Vec4f(1,0,0,0);
    ObjectPlaneS = Vec4f(1,0,0,0);
    EyePlaneT = Vec4f(0,1,0,0);
    ObjectPlaneT = Vec4f(0,1,0,0);
    EyePlaneR = Vec4f(0,0,1,0);
    ObjectPlaneR = Vec4f(0,0,1,0);
    EyePlaneQ = Vec4f(0,0,0,1);
    ObjectPlaneQ = Vec4f(0,0,0,1);
    GenModeS = GL_NONE;
    GenModeT = GL_NONE;
    GenModeR = GL_NONE;
    GenModeQ = GL_NONE;
}

/*--------------------------------------------------------------------------------------
Function: TexGen::apply
Description:
---------------------------------------------------------------------------------------*/
void TexGen::SetPlanesFromMatrix(const Matrixf& matrix)
{
    if(GetGenModeS() == GL_OBJECT_LINEAR)
        ObjectPlaneS.set(matrix(0, 0), matrix(1, 0), matrix(2, 0), matrix(3, 0));

    if(GetGenModeT() == GL_OBJECT_LINEAR)
        ObjectPlaneT.set(matrix(0, 1), matrix(1, 1), matrix(2, 1), matrix(3, 1));

    if(GetGenModeR() == GL_OBJECT_LINEAR)
        ObjectPlaneR.set(matrix(0, 2), matrix(1, 2), matrix(2, 2), matrix(3, 2));

    if(GetGenModeQ() == GL_OBJECT_LINEAR)
        ObjectPlaneQ.set(matrix(0, 3), matrix(1, 3), matrix(2, 3), matrix(3, 3));

    if(GetGenModeS() == GL_EYE_LINEAR)
        EyePlaneS.set(matrix(0, 0), matrix(1, 0), matrix(2, 0), matrix(3, 0));

    if(GetGenModeT() == GL_EYE_LINEAR)
        EyePlaneT.set(matrix(0, 1), matrix(1, 1), matrix(2, 1), matrix(3, 1));

    if(GetGenModeR() == GL_EYE_LINEAR)
        EyePlaneR.set(matrix(0, 2), matrix(1, 2), matrix(2, 2), matrix(3, 2));

    if(GetGenModeQ() == GL_EYE_LINEAR)
        EyePlaneQ.set(matrix(0, 3), matrix(1, 3), matrix(2, 3), matrix(3, 3));
}

/*--------------------------------------------------------------------------------------
Function: TexGen::apply
Description:
---------------------------------------------------------------------------------------*/
void TexGen::apply() const
{
    glActiveTexture(GL_TEXTURE0 + GetTextureUnit() );

    if(GetGenModeS() || GetGenModeT() || GetGenModeR() || GetGenModeQ())
    {
        oglSetMatrixMode(GL_MODELVIEW);
        glPushMatrix();
        glLoadIdentity();

        if(GetGenModeS())
        {
            glEnable(GL_TEXTURE_GEN_S);
            glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GetGenModeS());

            if(GetGenModeS() == GL_OBJECT_LINEAR)
                glTexGenfv(GL_S, GL_OBJECT_PLANE, GetObjectPlaneS().ptr());

            if(GetGenModeS() == GL_EYE_LINEAR)
                glTexGenfv(GL_S, GL_EYE_PLANE, GetEyePlaneS().ptr());
        }

        if(GetGenModeT())
        {
            glEnable(GL_TEXTURE_GEN_T);
            glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GetGenModeT());

            if(GetGenModeT() == GL_OBJECT_LINEAR)
                glTexGenfv(GL_T, GL_OBJECT_PLANE, GetObjectPlaneT().ptr());

            if(GetGenModeT() == GL_EYE_LINEAR)
                glTexGenfv(GL_T, GL_EYE_PLANE, GetEyePlaneT().ptr());
        }

        if(GetGenModeR())
        {
            glEnable(GL_TEXTURE_GEN_R);
            glTexGeni(GL_R, GL_TEXTURE_GEN_MODE, GetGenModeR());

            if(GetGenModeR() == GL_OBJECT_LINEAR)
                glTexGenfv(GL_R, GL_OBJECT_PLANE, GetObjectPlaneR().ptr());

            if(GetGenModeR() == GL_EYE_LINEAR)
                glTexGenfv(GL_R, GL_EYE_PLANE, GetEyePlaneR().ptr());
        }

        if(GetGenModeQ())
        {
            glEnable(GL_TEXTURE_GEN_Q);
            glTexGeni(GL_Q, GL_TEXTURE_GEN_MODE, GetGenModeQ());

            if(GetGenModeQ() == GL_OBJECT_LINEAR)
                glTexGenfv(GL_Q, GL_OBJECT_PLANE, GetObjectPlaneQ().ptr());

            if(GetGenModeQ() == GL_EYE_LINEAR)
                glTexGenfv(GL_Q, GL_EYE_PLANE, GetEyePlaneQ().ptr());
        }

        glPopMatrix();
    }

    if(!GetGenModeS())
        glDisable(GL_TEXTURE_GEN_S);

    if(!GetGenModeT())
        glDisable(GL_TEXTURE_GEN_T);

    if(!GetGenModeR())
        glDisable(GL_TEXTURE_GEN_R);

    if(!GetGenModeQ())
        glDisable(GL_TEXTURE_GEN_Q);
}

/*--------------------------------------------------------------------------------------
Function: Viewport::apply
Description:
---------------------------------------------------------------------------------------*/
void Viewport::apply() const
{
    static value_type PrevX = 99999999.0f;
    static value_type PrevY = 99999999.0f;
    static value_type PrevWidth = 99999999.0f;
    static value_type PrevHeight = 99999999.0f;

    if(X != PrevX || Y != PrevY || width != PrevWidth || height != PrevHeight)
    {
        glViewport(static_cast<GLint>(X),
                   static_cast<GLint>(Y),
                   static_cast<GLsizei>(width),
                   static_cast<GLsizei>(height));

        PrevX = X;
        PrevY = Y;
        PrevWidth = width;
        PrevHeight = height;
    }
}

/*--------------------------------------------------------------------------------------
Function: ClearNode::apply
Description:
---------------------------------------------------------------------------------------*/
void ClearNode::apply() const
{
    bool isdepthon = DepthMask::DepthWriteEnabled;

    if(!isdepthon)
    {
        glDepthMask(true);
    }

    if(clearMask & GL_COLOR_BUFFER_BIT)
        glClearColor(clearColor.r(), clearColor.g(), clearColor.b(), clearColor.a());

    glClear(clearMask);

    if(!isdepthon)
    {
        glDepthMask(false);
    }
}

#endif