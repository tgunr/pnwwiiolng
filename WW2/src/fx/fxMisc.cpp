#include <syscfg.h> // Path is project specific.
//////////////////////////////////////////////////////////////////////
// Module: fxMisc.cpp
// Description:
//////////////////////////////////////////////////////////////////////
#include "fx/fxMisc.h"
#include "include/vars.h"
#include "pi/pi.h"
#include "math/math3d.h"
#include "math/mathnum.h"
#include "math/mathUnitConversion.h"
#include "pers/persmain.h"
#include "fp/fpFPS.h"
#include "shader/glsl.h"
#include "ob/obmesh.h"
#include "fio/fio.h"
#include "mem/mem.h"
#include "ogl/oglGlobalStates.h"
#include "ogl/States.h"
#include "draw/sneaky.h"

#if USE_STATESET
#include "fx/postRender.h"
#endif

static piLVERTEX* _mesh;
static SINT32 _meshsize;

static glslProgram* _blackOut = NULL;

struct MISCTEXTURE
{
    GLuint w;
    GLuint h;
    GLuint texid;
    GLenum target;
    LOGICAL rectangle;

    MISCTEXTURE()
	: w(0), h(0)
	, texid(0)
	, target(GL_TEXTURE_2D)
	, rectangle(FALSE)
    {
    }

    ~MISCTEXTURE()
    {
        glDeleteTextures(1, &texid);
    }

    void bind()
    {
        TextureState::Bind(0, texid, target);
    }

    void unbind()
    {
        TextureState::Disable();
    }

    void enable()
    {
        TextureState::Enable(target);
    }

    void disable()
    {
        TextureState::Disable();
    }

};

struct CONCUSSION
{
    GLenum target;

    LOGICAL on;
    LOGICAL shock;

    SINT32 timer;
    SINT32 timelimit;

    UINT32 playerposition;

    FLOAT32 rotate;

    CONCUSSION()
	: target(0)
	, on(FALSE)
	, shock(FALSE)
	, timer(0)
	, timelimit(0)
	, playerposition(0)
	, rotate(0.0f)
    {
    }
};

static CONCUSSION _expconcussion[ML_MAX_CREW_POSITIONS];
static CONCUSSION _woundconcussion[ML_MAX_CREW_POSITIONS];

static MISCTEXTURE* _miscTexture;

//////////////////////////////////////////////////////////////////////
// Module: _createTexture
// Description:
//////////////////////////////////////////////////////////////////////
static LOGICAL _createTexture(SINT32 width, SINT32 height, GLenum target, GLuint& texid)
{
    glEnable(target);

    gl_GenTextures(1, &texid);

    // Initialize color texture
    TextureState::Bind(0, texid, target);

    glTexParameteri(target, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(target, GL_TEXTURE_WRAP_T, GL_CLAMP);

    glTexParameteri(target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(target, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

	glPixelStorei(GL_PACK_ALIGNMENT, 1);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    glTexImage2D(target,
                 0,
                 GL_RGBA8,
                 width,
                 height,
                 0,
                 GL_RGBA,
                 GL_UNSIGNED_BYTE,
                 NULL);

    if(!glIsTexture(texid))
    {
        return FALSE;
    }

    TextureState::Disable();

    return TRUE;
}

//////////////////////////////////////////////////////////////////////
// Module: _drawMesh
// Description:
//////////////////////////////////////////////////////////////////////
static void _drawMesh(UINT32 outsidecolor,
                      UINT32 insidecolor,
                      FLOAT32 cx,
                      FLOAT32 cy,
                      piUV uv[2])
{
    piTLVERTEX lv[9];
    UINT16 indices[10];

    FLOAT32 u1 = uv[0].u;
    FLOAT32 v1 = uv[0].v;
    FLOAT32 u2 = uv[1].u;
    FLOAT32 v2 = uv[1].v;

    lv[0].color = outsidecolor;
    lv[0].specular = colorALPHA100;
    lv[0].sx = -1.0f;
    lv[0].sy = 1.0f;
    lv[0].sz = 0.0f;
    lv[0].tu = u1;
    lv[0].tv = v2;

    lv[1].color = outsidecolor;
    lv[1].specular = colorALPHA100;
    lv[1].sx = 0.0f;
    lv[1].sy = 1.0f;
    lv[1].sz = 0.0f;
    lv[1].tu = u2 * 0.5f;
    lv[1].tv = v2;

    lv[2].color = outsidecolor;
    lv[2].specular = colorALPHA100;
    lv[2].sx = 1.0f;
    lv[2].sy = 1.0f;
    lv[2].sz = 0.0f;
    lv[2].tu = u2;
    lv[2].tv = v2;

    lv[3].color = outsidecolor;
    lv[3].specular = colorALPHA100;
    lv[3].sx = 1.0f;
    lv[3].sy = 0.0f;
    lv[3].sz = 0.0f;
    lv[3].tu = u2;
    lv[3].tv = v2 * 0.5f;

    lv[4].color = outsidecolor;
    lv[4].specular = colorALPHA100;
    lv[4].sx = 1.0f;
    lv[4].sy = -1.0f;
    lv[4].sz = 0.0f;
    lv[4].tu = u2;
    lv[4].tv = v1;

    lv[5].color = outsidecolor;
    lv[5].specular = colorALPHA100;
    lv[5].sx = 0.0f;
    lv[5].sy = -1.0f;
    lv[5].sz = 0.0f;
    lv[5].tu = u2 * 0.5f;
    lv[5].tv = v1;

    lv[6].color = outsidecolor;
    lv[6].specular = colorALPHA100;
    lv[6].sx = -1.0f;
    lv[6].sy = -1.0f;
    lv[6].sz = 0.0f;
    lv[6].tu = u1;
    lv[6].tv = v1;

    lv[7].color = outsidecolor;
    lv[7].specular = colorALPHA100;
    lv[7].sx = -1.0f;
    lv[7].sy = 0.0f;
    lv[7].sz = 0.0f;
    lv[7].tu = u1;
    lv[7].tv = v2 * 0.5f;

    lv[8].color = insidecolor;
    lv[8].specular = colorALPHA100;
    lv[8].sx = cx;
    lv[8].sy = cy;
    lv[8].sz = 0.0f;
    lv[8].tu = u2 / 2.0f;
    lv[8].tv = v2 / 2.0f;

    indices[0] = 8;
    indices[1] = 0;
    indices[2] = 1;
    indices[3] = 2;
    indices[4] = 3;
    indices[5] = 4;
    indices[6] = 5;
    indices[7] = 6;
    indices[8] = 7;
    indices[9] = 0;

    ogl::SetVertexPointer(3, GL_FLOAT, sizeof(piTLVERTEX), &lv[0].sx);
    ogl::SetColorPointer(4, GL_UNSIGNED_BYTE, sizeof(piTLVERTEX), &lv[0].color);
    ogl::SetTextureUnitCoordPointer(GL_TEXTURE0, 2, GL_FLOAT, sizeof(piTLVERTEX), &lv[0].tu);

	ogl::DrawElements(GL_TRIANGLE_FAN, 10, GL_UNSIGNED_SHORT, indices);
}

//////////////////////////////////////////////////////////////////////
// Module: _drawConcussion
// Description:
//////////////////////////////////////////////////////////////////////
void _drawConcussion(UINT32 outsidecolor,
                     UINT32 insidecolor,
                     FLOAT32 rotate,
                     FLOAT32 cx,
                     FLOAT32 cy)
{
    _miscTexture->bind();

    glRotatef(rotate, 0, 0, 1);

    piUV uv[2];

    uv[0].u = 0.0f;
    uv[0].v = 0.0f;
    uv[1].u = _miscTexture->rectangle ? (FLOAT32)_miscTexture->w : 1;
    uv[1].v = _miscTexture->rectangle ? (FLOAT32)_miscTexture->h : 1;

    _drawMesh(outsidecolor, insidecolor, cx, cy, uv);
}

//////////////////////////////////////////////////////////////////////
// Module: _copyBackBufferToSceneTexture
// Description:
//////////////////////////////////////////////////////////////////////
static void _copyBackBufferToSceneTexture(GLuint texid, GLenum target, UINT32 w, UINT32 h)
{
    TextureState::Bind(0, texid, target);

    glCopyTexSubImage2D(target,
                        0,
                        0,
                        0,
                        0,
                        0,
                        w,
                        h);

    TextureState::Disable();
}

//////////////////////////////////////////////////////////////////////
// Module: _grabScene
// Description:
//////////////////////////////////////////////////////////////////////
static void _grabScene()
{
    _copyBackBufferToSceneTexture(_miscTexture->texid,
                                  _miscTexture->target,
                                  _miscTexture->w,
                                  _miscTexture->h);
}

//////////////////////////////////////////////////////////////////////
// Module: _drawConcussionIfNecessary
// Description:
//////////////////////////////////////////////////////////////////////
static void _drawConcussionIfNecessary(CONCUSSION* c)
{
    persPersona* p = persGetPersonaDataPtr();
	if ( p == NULL ) return;

    if(!c->on)
    {
        return;
    }

    if(!c->timer)
    {
        c->timer = frametime;
    }

    FLOAT32 x0 = 1.0f;
    FLOAT32 x1 = 0.0f;

    SINT32 diff = frametime - c->timer;

    FLOAT32 alpha = 0;

    FLOAT32 t = (FLOAT32)diff / c->timelimit;

    alpha = x0 - t * (x0 - x1);

    if(alpha >= 0)
    {
        piUV uv[2];

        uv[0].u = 0.0f;
        uv[0].v = 0.0f;
        uv[1].u = 1;
        uv[1].v = 1;

        UINT32 outsidecolor = piRGBA(0.9f * alpha, 0, 0, alpha * 0.75f);
        UINT32 insidecolor = colorBLACK;

        _drawMesh(outsidecolor, insidecolor, 0, 0, uv);
    }

    if(alpha > 0.05f)
    {
        _miscTexture->enable();

        _grabScene();

        FLOAT32 r = c->rotate * alpha;
        FLOAT32 rads = 0;

        rads = DEG_TO_RAD(c->rotate) * alpha;

        UINT32 color = piRGBA(1, 1, 1, alpha);

        _drawConcussion(colorBLACK, color,  r, -rads,  rads);
        _drawConcussion(colorBLACK, color, -r,  rads,  rads);
        _drawConcussion(colorBLACK, color,  r,  rads, -rads);
        _drawConcussion(colorBLACK, color, -r, -rads, -rads);

        _miscTexture->disable();
    }
}

//////////////////////////////////////////////////////////////////////
// Module: _drawBlackout
// Description:
//////////////////////////////////////////////////////////////////////
static void _drawBlackout(UINT32 outsidecolor, UINT32 insidecolor)
{
    // Center mesh about origin
    glTranslatef(-1, -1, 0);

    _blackOut->Apply();

    piCOLORVALUE color;
    color.r = piRGBA_GETRED(insidecolor) * colorONEOVER255;
    color.g = piRGBA_GETGREEN(insidecolor) * colorONEOVER255;
    color.b = piRGBA_GETBLUE(insidecolor) * colorONEOVER255;
    color.a = piRGBA_GETALPHA(insidecolor) * colorONEOVER255;

    _blackOut->GetUniform("insidecolor")->Set(color);

    color.r = piRGBA_GETRED(outsidecolor) * colorONEOVER255;
    color.g = piRGBA_GETGREEN(outsidecolor) * colorONEOVER255;
    color.b = piRGBA_GETBLUE(outsidecolor) * colorONEOVER255;
    color.a = piRGBA_GETALPHA(outsidecolor) * colorONEOVER255;

    _blackOut->GetUniform("outsidecolor")->Set(color);

    _blackOut->ApplyUniformMap();

    ogl::SetVertexPointer(3, GL_FLOAT, sizeof(piLVERTEX), &_mesh[0].x);
    ogl::SetTextureUnitCoordPointer(GL_TEXTURE0, 2, GL_FLOAT, sizeof(piLVERTEX), &_mesh[0].tu);

	ogl::DrawArrays(GL_TRIANGLES, 0, _meshsize);

    ProgramState::Bind(0);
}

//////////////////////////////////////////////////////////////////////
// Module: _drawDeathFade
// Description:
//////////////////////////////////////////////////////////////////////
static void _drawDeathFade(SINT32 reset, UINT32 position, UINT32 playposition)
{
#define FADE_OUTSIDE_TIME   2000
#define FADE_INSIDE_TIME   4000

    static UINT32 insidecolor[ML_MAX_CREW_POSITIONS] = {colorBLACK};
    static UINT32 outsidecolor[ML_MAX_CREW_POSITIONS] = {colorBLACK};

    static SINT32 starttimer[ML_MAX_CREW_POSITIONS] = {0};
    static SINT32 outsidetimer[ML_MAX_CREW_POSITIONS] = {0};
    static SINT32 insidetimer[ML_MAX_CREW_POSITIONS] = {0};

    static SINT32 lastupdate[ML_MAX_CREW_POSITIONS] = {0};

    if(reset)
    {
        insidecolor[position] = colorBLACK;
        outsidecolor[position] = colorBLACK;

        starttimer[position] = 0;
        lastupdate[position] = 0;
        outsidetimer[position] = 0;
        insidetimer[position] = 0;

        return;
    }

    // Start timer to begin
    if(starttimer[position] == 0)
    {
        starttimer[position] = frametime;
    }

    // Start after 1 second delay
    if(frametime - starttimer[position] < 1000)
    {
        return;
    }

    if(outsidetimer[position] == 0)
    {
        outsidetimer[position] = frametime;
    }

    FLOAT32 alpha = 0.0f;

    SINT32 diff = frametime - outsidetimer[position];

    alpha = (FLOAT32)diff / (FLOAT32)FADE_OUTSIDE_TIME;

    alpha = alpha > 1.0f ? 1.0f : alpha;

    outsidecolor[position] = piRGBA(0, 0, 0, alpha);

    if(diff >= FADE_OUTSIDE_TIME)
    {
        if(insidetimer[position] == 0)
        {
            insidetimer[position] = frametime;
        }
    }

    if(insidetimer[position] != 0)
    {
        diff = frametime - insidetimer[position];

        alpha = (FLOAT32)diff / (FLOAT32)FADE_INSIDE_TIME;

        alpha = alpha > 1.0f ? 1.0f : alpha;

        insidecolor[position] = piRGBA(0, 0, 0, alpha);
    }

    if(position == playposition)
    {
        _drawBlackout(outsidecolor[position], insidecolor[position]);
    }
}

#define MAX_TUNNEL_TIME 2000

//////////////////////////////////////////////////////////////////////
// Module: _drawBlackoutIfNecessary
// Description:
//////////////////////////////////////////////////////////////////////
static void _drawBlackoutIfNecessary()
{
    persPersona* p = persGetPersonaDataPtr();
	if ( p == NULL ) return;

    static UINT32 insidecolor = colorBLACK;
    static UINT32 outsidecolor = colorBLACK;

    static SINT32 starttimer = 0;
    static SINT32 lastupdate = 0;

    static FLOAT32 alpha = 0;

    if(p->BlackoutPercent < 0.001f && p->PainLevel < 0.001f)
    {
        insidecolor = colorBLACK;
        outsidecolor = colorBLACK;
        return;
    }

    // G's on aircraft
    if(p->BlackoutPercent >= 0.99f)
    {
        if(!starttimer)
        {
            starttimer = frametime;
            lastupdate = frametime;
        }

        if(frametime - starttimer > MAX_TUNNEL_TIME)
        {
            // Increment alpha up to the blackout percentage to match outside color
            if(frametime - lastupdate > 50)
            {
                lastupdate = frametime;
                alpha += 0.05f;

                alpha = alpha > p->BlackoutPercent ? p->BlackoutPercent : alpha;
            }

            FLOAT32 r = 0;

            if(p->Redout)
            {
                FLOAT32 red = piRGBA_GETRED(insidecolor) * colorONEOVER255;

                // Increment red slowly to fade from black
                r = 0.9f * red + 0.05f;

                r = r > p->BlackoutPercent ? p->BlackoutPercent : r;
            }
            else
            {
                FLOAT32 red = piRGBA_GETRED(insidecolor) * colorONEOVER255;

                // Decrement red slowly to fade from red
                r = red - 0.05f;

                r = r < 0 ? 0 : r;
            }

            insidecolor = piRGBA(r, 0, 0, alpha);
        }
    }
    else
    {
        starttimer = 0;

        if(alpha > 0)
        {
            // Increment alpha up to the blackout percentage to match outside color
            if(frametime - lastupdate > 50)
            {
                lastupdate = frametime;
                alpha -= 0.05f;

                alpha = alpha < 0 ? 0 : alpha;
            }

            FLOAT32 r = 0;

            if(p->Redout)
            {
                FLOAT32 red = piRGBA_GETRED(insidecolor) * colorONEOVER255;

                // Increment red slowly to fade from black
                r = 0.9f * red + 0.05f;

                r = r > p->BlackoutPercent ? p->BlackoutPercent : r;
            }
            else
            {
                FLOAT32 red = piRGBA_GETRED(insidecolor) * colorONEOVER255;

                // Decrement red slowly to fade from red
                r = red - 0.05f;

                r = r < 0 ? 0 : r;
            }

            insidecolor = piRGBA(r, 0, 0, alpha);
        }
    }

    FLOAT32 r = 0;

    if(p->Redout)
    {
        FLOAT32 red = piRGBA_GETRED(outsidecolor) * colorONEOVER255;

        // Increment red slowly to fade from black
        r = 0.9f * red + 0.05f;

        r = r > p->BlackoutPercent ? p->BlackoutPercent : r;
    }
    else
    {
        FLOAT32 red = piRGBA_GETRED(outsidecolor) * colorONEOVER255;

        // Decrement red slowly to fade from red
        r = red - 0.05f;

        r = r < 0 ? 0 : r;
    }

    if(p->PainLevel > 0.001f)
    {
        outsidecolor = piRGBA(0, 0, 0, p->PainLevel);
    }
    else if(p->BlackoutPercent > 0.001f)
    {
        outsidecolor = piRGBA(r, 0, 0, p->BlackoutPercent);
    }

    _drawBlackout(outsidecolor, insidecolor);
}

#if(0)
//////////////////////////////////////////////////////////////////////
// Module: _reloadShader
// Description:
//////////////////////////////////////////////////////////////////////
static void _reloadShader()
{
}
#endif

//////////////////////////////////////////////////////////////////////
// Module: fxMiscUpdate
// Description:
//////////////////////////////////////////////////////////////////////
void fxMiscUpdate()
{
}

//////////////////////////////////////////////////////////////////////
// Module: fxMiscDrawEffects
// Description:
//////////////////////////////////////////////////////////////////////
void fxMiscDrawEffects()
{
    if(persIsCameraInternalMode())
    {
        UINT32 position = 0;

        persGetPersonaPosition(&position);

        oglSetMatrixMode(GL_MODELVIEW);
        glPushMatrix();
        glLoadIdentity();

        oglSetMatrixMode(GL_PROJECTION);
        glPushMatrix();
        glLoadIdentity();

        glOrtho(-1, 1, -1, 1, -1.0f, 1.0f);

        UINT32 zenable = piGetEngineRenderState(piRENDERSTATE_ZENABLE);
        UINT32 blendon = piGetEngineRenderState(piRENDERSTATE_ALPHABLENDENABLE);
        UINT32 lighton = piGetEngineRenderState(piRENDERSTATE_LIGHTING);

        piSetRenderState(piRENDERSTATE_ZENABLE, FALSE);
        piSetRenderState(piRENDERSTATE_ALPHABLENDENABLE, TRUE);
        piSetRenderState(piRENDERSTATE_LIGHTING, FALSE);

        oglDisableTexturesAboveAndIncluding(0);
        piDisableMaterial();

        piDisableRenderStates();

        if(_expconcussion[position].on)
        {
            _drawConcussionIfNecessary(&_expconcussion[position]);
        }
        else
        {
            _drawConcussionIfNecessary(&_woundconcussion[position]);
        }

        for(UINT32 i = 0; i < ML_MAX_CREW_POSITIONS; i++)
        {
            if(_expconcussion[i].on)
            {
                if(frametime - _expconcussion[i].timer >= _expconcussion[i].timelimit)
                {
                    _expconcussion[i].on = FALSE;
                    _expconcussion[i].timer = 0;
                }
            }

            if(_woundconcussion[i].on)
            {
                if(frametime - _woundconcussion[i].timer >= _woundconcussion[i].timelimit)
                {
                    _woundconcussion[i].on = FALSE;
                    _woundconcussion[i].timer = 0;
                }
            }

            if(fpFPSIsDead() || persGetCrewIsDead(i))
            {
                _drawDeathFade(0, i, position);
            }
            else
            {
                // Reset
                _drawDeathFade(1, i, 999);

#ifdef DOROTHY
				if(position == i && !persIsPersonaVehicleType(VEH_TYPE_MECH))
#else
                if(position == i)
#endif
                {
                    _drawBlackoutIfNecessary();
                }
            }
        }

        piSetRenderState(piRENDERSTATE_ZENABLE, zenable);
        piSetRenderState(piRENDERSTATE_ALPHABLENDENABLE, blendon);
        piSetRenderState(piRENDERSTATE_LIGHTING, lighton);

        glPopMatrix();
        oglSetMatrixMode(GL_MODELVIEW);
        glPopMatrix();
    } // persIsCameraInteralMode
}

//////////////////////////////////////////////////////////////////////
// Module: fxMiscInit
// Description:
//////////////////////////////////////////////////////////////////////
LOGICAL fxMiscInit()
{
    fxMiscResetAll();

    _miscTexture = new MISCTEXTURE;

    _miscTexture->w = (GLuint)piGetWidth();
    _miscTexture->h = (GLuint)piGetHeight();

    if(GLEW_ARB_texture_non_power_of_two)
    {
        // Do nothing
    }
    else if(GLEW_NV_texture_rectangle)
    {
        _miscTexture->target = GL_TEXTURE_RECTANGLE_NV;
        _miscTexture->rectangle = TRUE;
    }
    else if(GLEW_ARB_texture_rectangle)
    {
        _miscTexture->target = GL_TEXTURE_RECTANGLE_ARB;
        _miscTexture->rectangle = TRUE;
    }
    else if(GLEW_EXT_texture_rectangle)
    {
        _miscTexture->target = GL_TEXTURE_RECTANGLE_EXT;
        _miscTexture->rectangle = TRUE;
    }
    else
    {
        _miscTexture->w = closestPowerOfTwo(_miscTexture->w);
        _miscTexture->h = closestPowerOfTwo(_miscTexture->h);
    }

    if(!_createTexture(_miscTexture->w,
                       _miscTexture->h,
                       _miscTexture->target,
                       _miscTexture->texid))
    {
        return FALSE;
    }

    _blackOut = GLSL.loadProgram("blackout", NULL, NULL);

    char* v = "fx/shaders/GLSL/postrender/blackout.vert";
    char* f = "fx/shaders/GLSL/postrender/blackout.frag";

    ref_ptr<glslShader> s1 = GLSL.loadVertexShader(v);
    ref_ptr<glslShader> s2 = GLSL.loadFragmentShader(f);

    _blackOut->AddShader(s1.get());
    _blackOut->AddShader(s2.get());

    _blackOut->GetOrCreateUniform("insidecolor", glslUniform::FLOAT_VEC4);
    _blackOut->GetOrCreateUniform("outsidecolor", glslUniform::FLOAT_VEC4);

    _meshsize = obmeshCreate(6, 6, 2, 2, 0, 0, 1, 1, &_mesh);

    if(_meshsize == 0)
    {
        Write_Log(LOG_OLD, LOG_INFO, "[fxMiscInit] **FAILED** to create mesh");
        return FALSE;
    }

    // Coordinates are not normalized
    if(_miscTexture->rectangle)
    {
        for(SINT32 i = 0; i < _meshsize; i++)
        {
            _mesh[i].tu *= _miscTexture->w;
            _mesh[i].tv *= _miscTexture->h;
        }
    }

    return TRUE;
}

//////////////////////////////////////////////////////////////////////
// Module: fxMiscKill
// Description:
//////////////////////////////////////////////////////////////////////
void fxMiscKill()
{
    if(_mesh)
    {
        memFree(_mesh);
    }

    if(_miscTexture)
    {
        delete _miscTexture;
    }
}

//////////////////////////////////////////////////////////////////////
// Module: fxMiscStartExpConcussion
// Description:
//////////////////////////////////////////////////////////////////////
void fxMiscStartExpConcussion(FLOAT32 rotatevalue, SINT32 timelimit)
{
    if(persIsPersonaVehicleType(VEH_TYPE_FLOATER))
    {
        return;
    }

    UINT32 position = 0;

    persGetPersonaPosition(&position);

    if(fpFPSIsDead() || persGetCrewIsDead(position))
    {
        return;
    }

    rotatevalue = rotatevalue > 32 ? 32 : rotatevalue;

    _expconcussion[position].rotate = rotatevalue;
    _expconcussion[position].timer = 0;

    _expconcussion[position].on = TRUE;
    _expconcussion[position].timelimit = timelimit;
}

//////////////////////////////////////////////////////////////////////
// Module: fxMiscStartHitConcussion
// Description:
//////////////////////////////////////////////////////////////////////
void fxMiscStartHitConcussion(FLOAT32 rotatevalue, SINT32 timelimit)
{
    UINT32 position = 0;

    persGetPersonaPosition(&position);

    if(fpFPSIsDead() || persGetCrewIsDead(position))
    {
        return;
    }

    rotatevalue = rotatevalue > 32 ? 32 : rotatevalue;

    _woundconcussion[position].rotate = rotatevalue;
    _woundconcussion[position].timer = 0;

    _woundconcussion[position].on = TRUE;
    _woundconcussion[position].timelimit = timelimit;
}

//////////////////////////////////////////////////////////////////////
// Module: fxMiscResetAll
// Description:
//////////////////////////////////////////////////////////////////////
void fxMiscResetAll()
{
    memset(_expconcussion, 0, sizeof(CONCUSSION) * ML_MAX_CREW_POSITIONS);
    memset(_woundconcussion, 0, sizeof(CONCUSSION) * ML_MAX_CREW_POSITIONS);
 }
