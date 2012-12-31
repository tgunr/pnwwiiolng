#include <syscfg.h> // Path is project specific.
/////////////////////////////////////////////////
// Module: fxUtl.cpp
// Description: Common FX utilities module.
/////////////////////////////////////////////////
#include "fx/fxUtl.h"

#include "include/vars.h"
#include "math/math3d.h"
#include "math/mathvect.h"
#include "ld/ldpix.h"
#include "ml/mltypes.h"
#include "pers/persmain.h"
#include "ml/mlutility.h"
#include "math/mathnum.h"
#include "fio/fio.h"
#include "trn/trnmain.h"
#include "mem/mem.h"
#include "libxml/parser.h"
#include "libxml/tree.h"
#include "math/Vec4f.h"
#include "fx/DataFx.h"
#include "trn/trnvars.h"

#include <string>

#define MAX_UV  64

static FLOAT32 g_fCosTbl[360];
static FLOAT32 g_fSinTbl[360];
static FLOAT32 g_fPI = PI;

static SINT32 g_iTexID = -1;
static SINT32 g_uvCount = 0;

static piUV g_uv[MAX_UV][2];

struct MATERIALPROP
{
    std::string hitsoundwave[3];
    std::string bulleteffectname;
    std::string materialname;

    SINT32 bulletType;

    FLOAT32 minvolume;
    FLOAT32 maxenergyvolume;

    MATERIALPROP()
	: bulleteffectname(), materialname()
	, bulletType(0)
	, minvolume(0.0f), maxenergyvolume(0.0f)
    {
	hitsoundwave[0] = hitsoundwave[1] = hitsoundwave[2] = "";
    }

} ;

typedef std::map<std::string, MATERIALPROP *> MaterialPropertiesMap;
static MaterialPropertiesMap g_materialProperties;

static SINT32 xmlinputmatch(const SCHAR* name) {return 1;}

static LOGICAL _initMaterialSounds();

static void _parseTree(xmlNodePtr root);
static void _parseElements(xmlNodePtr node, MATERIALPROP* m);

//////////////////////////////////////////////////////////////////////
// Function: fxUtlInit
// Description:
//////////////////////////////////////////////////////////////////////
LOGICAL fxUtlInit()
{
    for(SINT32 i = 0; i < 360; i++)
    {
        g_fCosTbl[i] = cosf((g_fPI * i) / 180.0f);
        g_fSinTbl[i] = sinf((g_fPI * i) / 180.0f);
    }

    if(!_initMaterialSounds())
    {
        Write_Log(LOG_OLD, LOG_WARN, "[fxUtlInit] **FAILED** to load material file.");
        return FALSE;
    }

    return TRUE;
}

////////////////////////////////////////////////////////
// Function: _parseElements
// Description:
////////////////////////////////////////////////////////
void _parseElements(xmlNodePtr root, MATERIALPROP* m)
{
    xmlNodePtr node = root;

	for(; node != NULL; node = node->next)
	{
        if(node->type != XML_ELEMENT_NODE)
        {
            continue;
        }

        if(!xmlStrcmp((xmlChar*)"sound1", node->name))
        {
            xmlChar* s = xmlNodeGetContent(node);

            if(s)
            {
                m->hitsoundwave[0] = (SCHAR*)s;
                xmlFree(s);
            }
        }

        if(!xmlStrcmp((xmlChar*)"sound2", node->name))
        {
            xmlChar* s = xmlNodeGetContent(node);

            if(s)
            {
                m->hitsoundwave[1] = (SCHAR*)s;
                xmlFree(s);
            }
        }

        if(!xmlStrcmp((xmlChar*)"sound3", node->name))
        {
            xmlChar* s = xmlNodeGetContent(node);

            if(s)
            {
                m->hitsoundwave[2] = (SCHAR*)s;
                xmlFree(s);
            }
        }

        if(!xmlStrcmp((xmlChar*)"minvolume", node->name))
        {
            xmlChar* s = xmlNodeGetContent(node);

            if(s)
            {
                m->minvolume = (FLOAT32)atof((SCHAR*)s);
                xmlFree(s);
            }
        }

        if(!xmlStrcmp((xmlChar*)"maxvolumeenergy", node->name))
        {
            xmlChar* s = xmlNodeGetContent(node);

            if(s)
            {
                m->maxenergyvolume = (FLOAT32)atof((SCHAR*)s);
                xmlFree(s);
            }
        }

        if(!xmlStrcmp((xmlChar*)"bullettype", node->name))
        {
            xmlChar* s = xmlNodeGetContent(node);

            if(s)
            {
                m->bulletType = atol((SCHAR*)s);
                xmlFree(s);
            }
        }

        if(!xmlStrcmp((xmlChar*)"bulleteffectname", node->name))
        {
            xmlChar* s = xmlNodeGetContent(node);

            if(s)
            {
                m->bulleteffectname = (char*)s;
            }
        }
    }
}

////////////////////////////////////////////////////////
// Function: _parseTree
// Description:
////////////////////////////////////////////////////////
void _parseTree(xmlNodePtr root)
{
    xmlNodePtr node = root;

	for(; node != NULL; node = node->next)
	{
        if(node->type != XML_ELEMENT_NODE)
        {
            continue;
        }

        if(!xmlStrcmp((xmlChar*)"material", node->name))
        {
            xmlChar* s = xmlGetProp(node, (xmlChar*)"name");

            if(!s)
            {
                continue;
            }

            MATERIALPROP* m = new MATERIALPROP;

            m->materialname = (SCHAR*)s;

            xmlFree(s);

            _parseElements(node->children, m);

            g_materialProperties[m->materialname] = m;
        }
	}
}

////////////////////////////////////////////////////////
// Function: _initMaterialSounds
// Description:
////////////////////////////////////////////////////////
LOGICAL _initMaterialSounds()
{
    xmlRegisterInputCallbacks(xmlinputmatch,
                             (xmlInputOpenCallback)fioOpen,
                             (xmlInputReadCallback)fioRead,
                             (xmlInputCloseCallback)fioClose);

    SCHAR* name = "fx/xml/materialprops.xml";

    xmlDocPtr doc = xmlReadFile(name, NULL, XML_PARSE_DTDLOAD | XML_PARSE_RECOVER);

    if(doc)
    {
	    xmlNodePtr root = xmlDocGetRootElement(doc);

        if(!root)
        {
            return FALSE;
        }

        if(!xmlStrcmp((xmlChar*)"MATERIALPROPS", root->name))
        {
            _parseTree(root->children);
        }

	    xmlFreeDoc(doc);
    }

    xmlCleanupInputCallbacks();

    return TRUE;
}

////////////////////////////////////////////////////////
// Function: fxUtlGetRandomHitSound
// Description:
////////////////////////////////////////////////////////
LOGICAL fxUtlGetRandomHitSound(const SCHAR* material,
                               SCHAR** sound,
                               FLOAT32* minvolume,
                               FLOAT32* maxvolumeenergy)
{
    if(!material)
    {
        return FALSE;
    }

    MaterialPropertiesMap::iterator find = g_materialProperties.find(material);

    if(find == g_materialProperties.end())
    {
        return FALSE;
    }

    MATERIALPROP* data = find->second;

    SINT32 random = abs(mathGetChaoticNumber()) % 3;

    *sound = (SCHAR*)data->hitsoundwave[random].c_str();

    if(minvolume)
    {
        *minvolume = data->minvolume;
    }

    if(maxvolumeenergy)
    {
        *maxvolumeenergy = data->maxenergyvolume;
    }

    return TRUE;
}

////////////////////////////////////////////////////////
// Function: fxUtlGetBulletEffectFromMaterial
// Description:
////////////////////////////////////////////////////////
const char* fxUtlGetBulletEffectFromMaterial(const char* material)
{
    MaterialPropertiesMap::iterator find = g_materialProperties.find(material);

    if(find == g_materialProperties.end())
    {
        return NULL;
    }

    if(find->second->bulleteffectname.empty())
    {
        return NULL;
    }

    return find->second->bulleteffectname.c_str();
}

////////////////////////////////////////////////////////
// Function: fxUtlGetBulletType
// Description:
////////////////////////////////////////////////////////
SINT32 fxUtlGetBulletType(const SCHAR* material)
{
    if(!material)
    {
        return -1;
    }

    MaterialPropertiesMap::iterator find = g_materialProperties.find(material);

    if(find == g_materialProperties.end())
    {
        return -1;
    }

    return find->second->bulletType;
}

/////////////////////////////////////////////////
// Function: fxUtlSetTexture
// Description:
/////////////////////////////////////////////////
void fxUtlSetTexture()
{
    oglSetActiveTexture(0, FxGet().getTexture());
}

/////////////////////////////////////////////////
// Function: fxUtlKill
// Description:
/////////////////////////////////////////////////
void fxUtlKill()
{
    if(piIsTexture(g_iTexID, 0))
    {
        piFreeMipTextures(g_iTexID);
        g_iTexID = -1;
    }

    MaterialPropertiesMap::iterator begin = g_materialProperties.begin();
    MaterialPropertiesMap::iterator end = g_materialProperties.end();
    MaterialPropertiesMap::iterator free;

    while(begin != end)
    {
        free = begin;
        ++begin;

        delete free->second;
    }

    g_materialProperties.clear();
}

//////////////////////////////////////////////////////////////////////
// Function: fxUtlGetTextureUV
// Description:
//////////////////////////////////////////////////////////////////////
void fxUtlGetTextureUV(piUV uv[2], SINT32 texindex)
{
    if(texindex < 0) return;

    Vec4f uvs;
    FxGet().getuv(uvs, texindex);

    uv[0].u = uvs.x();
    uv[0].v = uvs.y();
    uv[1].u = uvs.z();
    uv[1].v = uvs.w();
}

//////////////////////////////////////////////////////////////////////
// Function: fxUtlGetTextureUV
// Description:
//////////////////////////////////////////////////////////////////////
void fxUtlGetTextureUV(piUV uv[2], const char* str)
{
    Vec4f uvs;

    if(FxGet().getuv(uvs, str))
    {
        uv[0].u = uvs.x();
        uv[0].v = uvs.y();
        uv[1].u = uvs.z();
        uv[1].v = uvs.w();
    }
    else
    {
        uv[0].u = 0.0f;
        uv[0].v = 0.0f;
        uv[1].u = 1.0f;
        uv[1].v = 1.0f;
    }
}

//////////////////////////////////////////////////////////////////////
// Function: fxUtlGetBillowSmoke
// Description: Return random billow smoke sprite from fx.bmp
//////////////////////////////////////////////////////////////////////
SINT32 fxUtlGetBillowSmoke()
{
    // Billow smoke on texture is [0 - 4]
    return abs(mathGetChaoticNumber() % 5);
}

//////////////////////////////////////////////////////////////////////
// Function: fxUtlGetFire
// Description: Return random fire from fx.bmp
//////////////////////////////////////////////////////////////////////
SINT32 fxUtlGetFire()
{
    // fire on texture is [9 - 13]
    return abs(mathGetChaoticNumber() % 5) + 9;
}

//////////////////////////////////////////////////////////////////////
// Function: fxUtlGetHitSmoke
// Description: Return random hit smoke from fx.bmp
//////////////////////////////////////////////////////////////////////
SINT32 fxUtlGetHitSmoke()
{
    // hit smoke on texture is [5 - 8]
    return (mathGetChaoticNumber() & 3) + 5;	// & 3 = modulo 4
}

//////////////////////////////////////////////////////////////////////
// Function: fxUtlGetFireC
// Description: Return randome fire 'C' sprite for volume effects.
//////////////////////////////////////////////////////////////////////
SINT32 fxUtlGetFireC()
{
    // fire 'C' is [14 - 18]
    return abs(mathGetChaoticNumber() % 5) + 14;
}

/////////////////////////////////////////////////
// Function: fxUtlGetCos
// Description:
/////////////////////////////////////////////////
FLOAT32 fxUtlGetCos(SINT32 iDegrees)
{
    iDegrees = iDegrees >= 360 ? 0 : iDegrees < 0 ? 359 : iDegrees;

    return g_fCosTbl[iDegrees];
}

/////////////////////////////////////////////////
// Function: fxUtlGetSin
// Description:
/////////////////////////////////////////////////
FLOAT32 fxUtlGetSin(SINT32 iDegrees)
{
    iDegrees = iDegrees >= 360 ? 0 : iDegrees < 0 ? 359 : iDegrees;

    return g_fSinTbl[iDegrees];
}

//////////////////////////////////////////////////////////////////////
// Function: fxUtlInterpolateVector
// Description:
//////////////////////////////////////////////////////////////////////
void fxUtlInterpolateVector(INTFLOAT32* lpTime, INTVECTOR* lpIn, piVECTOR* lpOut)
{
    FLOAT32 t = 0.0f;

    if((FLOAT32)frametime < lpTime->p2)
    {
        FLOAT32 fNow = (FLOAT32)frametime;
        FLOAT32 fEnd = lpTime->p2;
        FLOAT32 fStart = lpTime->p1;

        t = (fNow - fStart) / (fEnd - fStart);

        lpOut->x = lpIn->p2.x - lpIn->p1.x;
        lpOut->x = lpOut->x * t + lpIn->p1.x;
        lpOut->y = lpIn->p2.y - lpIn->p1.y;
        lpOut->y = lpOut->y * t + lpIn->p1.y;
        lpOut->z = lpIn->p2.z - lpIn->p1.z;
        lpOut->z = lpOut->z * t + lpIn->p1.z;
    }
    else if((FLOAT32)frametime < lpTime->p3)
    {
        FLOAT32 fNow = (FLOAT32)frametime;
        FLOAT32 fEnd = (FLOAT32)lpTime->p3;
        FLOAT32 fStart = (FLOAT32)lpTime->p2;

        t = (fNow - fStart) / (fEnd - fStart);

        lpOut->x = lpIn->p3.x - lpIn->p2.x;
        lpOut->x = lpOut->x * t + lpIn->p2.x;
        lpOut->y = lpIn->p3.y - lpIn->p2.y;
        lpOut->y = lpOut->y * t + lpIn->p2.y;
        lpOut->z = lpIn->p3.z - lpIn->p2.z;
        lpOut->z = lpOut->z * t + lpIn->p2.z;
    }
    else if((FLOAT32)frametime < lpTime->p4)
    {
        FLOAT32 fNow = (FLOAT32)frametime;
        FLOAT32 fEnd = (FLOAT32)lpTime->p4;
        FLOAT32 fStart = (FLOAT32)lpTime->p3;

        t = (fNow - fStart) / (fEnd - fStart);

        lpOut->x = lpIn->p4.x - lpIn->p3.x;
        lpOut->x = lpOut->x * t + lpIn->p3.x;
        lpOut->y = lpIn->p4.y - lpIn->p3.y;
        lpOut->y = lpOut->y * t + lpIn->p3.y;
        lpOut->z = lpIn->p4.z - lpIn->p3.z;
        lpOut->z = lpOut->z * t + lpIn->p3.z;
    }
    else
    {
        lpOut->x = lpIn->p4.x;
        lpOut->y = lpIn->p4.y;
        lpOut->z = lpIn->p4.z;
    }
}

//////////////////////////////////////////////////////////////////////
// Function: fxUtlInterpolateRadius
// Description:
//////////////////////////////////////////////////////////////////////
void fxUtlInterpolateRadius(INTFLOAT32* lpTime, INTFLOAT32* lpIn, FLOAT32* pOut)
{
    FLOAT32 t = 0.0f;

    if((FLOAT32)frametime < lpTime->p2)
    {
        FLOAT32 fNow = (FLOAT32)frametime;
        FLOAT32 fEnd = lpTime->p2;
        FLOAT32 fStart = lpTime->p1;

        t = (fNow - fStart) / (fEnd - fStart);

        *pOut = lpIn->p2 - lpIn->p1;
        *pOut = *pOut * t + lpIn->p1;
    }
    else if((FLOAT32)frametime < lpTime->p3)
    {
        FLOAT32 fNow = (FLOAT32)frametime;
        FLOAT32 fEnd = (FLOAT32)lpTime->p3;
        FLOAT32 fStart = (FLOAT32)lpTime->p2;

        t = (fNow - fStart) / (fEnd - fStart);

        *pOut = lpIn->p3 - lpIn->p2;
        *pOut = *pOut * t + lpIn->p2;
    }
    else if((FLOAT32)frametime < lpTime->p4)
    {
        FLOAT32 fNow = (FLOAT32)frametime;
        FLOAT32 fEnd = (FLOAT32)lpTime->p4;
        FLOAT32 fStart = (FLOAT32)lpTime->p3;

        t = (fNow - fStart) / (fEnd - fStart);

        *pOut = lpIn->p4 - lpIn->p3;
        *pOut = *pOut * t + lpIn->p3;
    }
    else
    {
        *pOut = lpIn->p4;
    }
}

//////////////////////////////////////////
// Function: fxUtlInterpolateColor
// Desctiption:
//////////////////////////////////////////
void fxUtlInterpolateColor(INTFLOAT32* lpTime, INTCOLOR* lpIn, piCOLORVALUE* lpOut)
{
    FLOAT32 t = 0.0f;

    if((FLOAT32)frametime < lpTime->p2)
    {
        FLOAT32 fNow = (FLOAT32)frametime;
        FLOAT32 fEnd = lpTime->p2;
        FLOAT32 fStart = lpTime->p1;

        t = (fNow - fStart) / (fEnd - fStart);

        lpOut->r = (lpIn->p2.r - lpIn->p1.r) * t + lpIn->p1.r;
        lpOut->b = (lpIn->p2.b - lpIn->p1.b) * t + lpIn->p1.b;
        lpOut->g = (lpIn->p2.g - lpIn->p1.g) * t + lpIn->p1.g;
    }
    else if((FLOAT32)frametime < lpTime->p3)
    {
        FLOAT32 fNow = (FLOAT32)frametime;
        FLOAT32 fEnd = (FLOAT32)lpTime->p3;
        FLOAT32 fStart = (FLOAT32)lpTime->p2;

        t = (fNow - fStart) / (fEnd - fStart);

        lpOut->r = (lpIn->p3.r - lpIn->p2.r) * t + lpIn->p2.r;
        lpOut->b = (lpIn->p3.b - lpIn->p2.b) * t + lpIn->p2.b;
        lpOut->g = (lpIn->p3.g - lpIn->p2.g) * t + lpIn->p2.g;
    }
    else if((FLOAT32)frametime < lpTime->p4)
    {
        FLOAT32 fNow = (FLOAT32)frametime;
        FLOAT32 fEnd = (FLOAT32)lpTime->p4;
        FLOAT32 fStart = (FLOAT32)lpTime->p3;

        t = (fNow - fStart) / (fEnd - fStart);

        lpOut->r = (lpIn->p4.r - lpIn->p3.r) * t + lpIn->p3.r;
        lpOut->b = (lpIn->p4.b - lpIn->p3.b) * t + lpIn->p3.b;
        lpOut->g = (lpIn->p4.g - lpIn->p3.g) * t + lpIn->p3.g;
    }
    else
    {
        lpOut->r = lpIn->p4.r;
        lpOut->b = lpIn->p4.b;
        lpOut->g = lpIn->p4.g;
    }
}

//////////////////////////////////////////////////////////////////////
// Function: fxUtlFromToRotation
// Description: Create rotation matrix to rotate 'From Vector' to 'To Vector'
// http://www.acm.org/jgt/papers/MollerHughes99/code.html
//////////////////////////////////////////////////////////////////////
void fxUtlFromToRotation(piVECTOR* lpFrom, piVECTOR* lpTo, piMATRIX* lpMat)
{
    piVECTOR v;
    FLOAT32 e, f;

    math3dCrossProduct(&v, lpFrom, lpTo);
    e = math3dDotProduct(lpFrom, lpTo);

    f = (e < 0.0f) ? -e : e;

    if(f > 1.0f - 0.000001f)
    {
        piVECTOR vecOrth;
        piVECTOR u, v;
        FLOAT32 c1, c2, c3;

        vecOrth.x = lpFrom->x > 0.0f ? lpFrom->x : -lpFrom->x;
        vecOrth.y = lpFrom->y > 0.0f ? lpFrom->y : -lpFrom->y;
        vecOrth.z = lpFrom->z > 0.0f ? lpFrom->z : -lpFrom->z;

        if(vecOrth.x < vecOrth.y)
        {
            if(vecOrth.x < vecOrth.z)
            {
                vecOrth.x = 1.0f;
                vecOrth.y = 0.0f;
                vecOrth.z = 0.0f;
            }
            else
            {
                vecOrth.x = 0.0f;
                vecOrth.y = 0.0f;
                vecOrth.z = 1.0f;
            }
        }
        else
        {
            if(vecOrth.y < vecOrth.z)
            {
                vecOrth.x = 0.0f;
                vecOrth.y = 1.0f;
                vecOrth.z = 0.0f;
            }
            else
            {
                vecOrth.x = 0.0f;
                vecOrth.y = 0.0f;
                vecOrth.z = 1.0f;
            }
        }

        u.x = vecOrth.x - lpFrom->x;
        u.y = vecOrth.y - lpFrom->y;
        u.z = vecOrth.z - lpFrom->z;

        v.x = vecOrth.x - lpTo->x;
        v.y = vecOrth.y - lpTo->y;
        v.z = vecOrth.z - lpTo->z;

        c1 = 2.0f / math3dDotProduct(&u, &u);
        c2 = 2.0f / math3dDotProduct(&v, &v);
        c3 = c1 * c2  * math3dDotProduct(&u, &v);

        lpMat->m[0][0] = -c1 * u.x * u.x - c2 * v.x * v.x + c3 * v.x * u.x;
        lpMat->m[0][1] = -c1 * u.x * u.y - c2 * v.x * v.y + c3 * v.x * u.y;
        lpMat->m[0][2] = -c1 * u.x * u.z - c2 * v.x * v.z + c3 * v.x * u.z;

        lpMat->m[1][0] = -c1 * u.y * u.x - c2 * v.y * v.x + c3 * v.y * u.x;
        lpMat->m[1][1] = -c1 * u.y * u.y - c2 * v.y * v.y + c3 * v.y * u.y;
        lpMat->m[1][2] = -c1 * u.y * u.z - c2 * v.y * v.z + c3 * v.y * u.z;

        lpMat->m[2][0] = -c1 * u.z * u.x - c2 * v.z * v.x + c3 * v.z * u.x;
        lpMat->m[2][1] = -c1 * u.z * u.y - c2 * v.z * v.y + c3 * v.z * u.y;
        lpMat->m[2][2] = -c1 * u.z * u.z - c2 * v.z * v.z + c3 * v.z * u.z;

        lpMat->m[0][0] += 1.0f;
        lpMat->m[1][1] += 1.0f;
        lpMat->m[2][2] += 1.0f;
    }
    else
    {
        FLOAT32 hvx, hvz, hvxy, hvxz, hvyz;
        FLOAT32 h;

        h = (1.0f - e) / math3dDotProduct(&v, &v);

        hvx = h * v.x;
        hvz = h * v.z;

        hvxy = hvx * v.y;
        hvxz = hvx * v.z;
        hvyz = hvz * v.y;

        lpMat->m[0][0] = e + hvx * v.x;
        lpMat->m[0][1] = hvxy - v.z;
        lpMat->m[0][2] = hvxz + v.y;

        lpMat->m[1][0] = hvxy + v.z;
        lpMat->m[1][1] = e + h * v.y * v.y;
        lpMat->m[1][2] = hvyz - v.x;

        lpMat->m[2][0] = hvxz - v.y;
        lpMat->m[2][1] = hvyz + v.x;
        lpMat->m[2][2] = e + hvz * v.z;
    }
}

//////////////////////////////////////////////////////////////////////
// Function: fxUtlRotateZ
// Description:
//////////////////////////////////////////////////////////////////////
void fxUtlRotateZ(piVECTOR vecOut[], piMATRIX* lpRot)
{
    piVECTOR vecRet[4];

    vecRet[0].x = lpRot->_11 * vecOut[0].x + lpRot->_12 * vecOut[0].y;
    vecRet[0].y = lpRot->_21 * vecOut[0].x + lpRot->_22 * vecOut[0].y;
    vecRet[0].z = 0.0f;

    vecRet[1].x = lpRot->_11 * vecOut[1].x + lpRot->_12 * vecOut[1].y;
    vecRet[1].y = lpRot->_21 * vecOut[1].x + lpRot->_22 * vecOut[1].y;
    vecRet[1].z = 0.0f;

    vecRet[2].x = lpRot->_11 * vecOut[2].x + lpRot->_12 * vecOut[2].y;
    vecRet[2].y = lpRot->_21 * vecOut[2].x + lpRot->_22 * vecOut[2].y;
    vecRet[2].z = 0.0f;

    vecRet[3].x = lpRot->_11 * vecOut[3].x + lpRot->_12 * vecOut[3].y;
    vecRet[3].y = lpRot->_21 * vecOut[3].x + lpRot->_22 * vecOut[3].y;
    vecRet[3].z = 0.0f;

    vecOut[0] = vecRet[0];
    vecOut[1] = vecRet[1];
    vecOut[2] = vecRet[2];
    vecOut[3] = vecRet[3];
}

//////////////////////////////////////////////////////////////////////
// Function: fxUtlInterpolateAlpha
// Description: Interpolates alpha between 4 points.
//////////////////////////////////////////////////////////////////////
void fxUtlInterpolateAlpha(INTFLOAT32* lpTime, INTCOLOR* lpIn, piCOLORVALUE* lpOut)
{
    FLOAT32 fAlpha = 0.0f;
    FLOAT32 t = 0.0f;

    if((FLOAT32)frametime < lpTime->p2)
    {
        FLOAT32 fNow = (FLOAT32)frametime;
        FLOAT32 fEnd = lpTime->p2;
        FLOAT32 fStart = lpTime->p1;

        t = (fNow - fStart) / (fEnd - fStart);

        fAlpha = lpIn->p2.a - lpIn->p1.a;
        fAlpha = fAlpha * t + lpIn->p1.a;
    }
    else if((FLOAT32)frametime < lpTime->p3)
    {
        FLOAT32 fNow = (FLOAT32)frametime;
        FLOAT32 fEnd = (FLOAT32)lpTime->p3;
        FLOAT32 fStart = (FLOAT32)lpTime->p2;

        t = (fNow - fStart) / (fEnd - fStart);

        fAlpha = lpIn->p3.a - lpIn->p2.a;
        fAlpha = fAlpha * t + lpIn->p2.a;
    }
    else if((FLOAT32)frametime < lpTime->p4)
    {
        FLOAT32 fNow = (FLOAT32)frametime;
        FLOAT32 fEnd = (FLOAT32)lpTime->p4;
        FLOAT32 fStart = (FLOAT32)lpTime->p3;

        t = (fNow - fStart) / (fEnd - fStart);

        fAlpha = lpIn->p4.a - lpIn->p3.a;
        fAlpha = fAlpha * t + lpIn->p3.a;
    }
    else
    {
        fAlpha = lpIn->p4.a;
    }

    lpOut->a = fAlpha;
}

//////////////////////////////////////////////////////////////////////
// Function: fxUtlGetObject
// Description: Parse through non-objects.
//////////////////////////////////////////////////////////////////////
OBJECT* fxUtlGetObject(OBJECT* lpObj)
{
    if(lpObj == NULL)
        return NULL;

    OBJECT* lpReturnObj = lpObj;

    while(lpReturnObj != NULL)
    {
		if(!obmainValidateObject(lpReturnObj))
			return NULL;

        if(!(lpReturnObj->flags & OBJECT_NOTOBJECT))
        {
            return lpReturnObj;
        }

        lpReturnObj = lpReturnObj->child;
    }

    return NULL;
}

//////////////////////////////////////////////////////////////////////
// Function: fxUtlUpdatePosition
// Description:
//////////////////////////////////////////////////////////////////////
void fxUtlUpdatePosition(piVECTOR* pvecPos, FLOAT64VECTOR* pvecOriginal)
{
    memset(pvecPos, 0, sizeof(piVECTOR));

    (*pvecPos).x = (FLOAT32)( pvecOriginal->x - WorldOriginOffset.x);
    (*pvecPos).y = (FLOAT32)( pvecOriginal->y - WorldOriginOffset.y);
    (*pvecPos).z = (FLOAT32)(-pvecOriginal->z);
}

//////////////////////////////////////////
// Function: fxUtlGetDistanceSqrd
// Description:
//////////////////////////////////////////
FLOAT32 fxUtlGetDistanceSqrd(piVECTOR* pvecPos)
{
    piVECTOR vecPlayer;
    piVECTOR vecDiff;

    vecPlayer.x = -playpos.x;
    vecPlayer.y = -playpos.y;
    vecPlayer.z = -playpos.z;

    mathSubVectors(&vecDiff, &vecPlayer, pvecPos);

    return vecDiff.x * vecDiff.x + vecDiff.y * vecDiff.y + vecDiff.z * vecDiff.z;
}

//////////////////////////////////////////////////////////////////////
// Function: fxUtlGetDOFDirection
// Description: Assume DOF direction is out the Y axis.
//////////////////////////////////////////////////////////////////////
void fxUtlGetDOFDirection(OBJECTDOF* dof, piMATRIX* mat, piVECTOR* dir)
{
    if(dof && mat)
    {
        dir->set(dof->dofmatrix2._21, dof->dofmatrix2._22, dof->dofmatrix2._23);
        math3dMultiplyRotation(dir, dir, mat);
    }
}

//////////////////////////////////////////////////////////////////////
// Function: fxUtlGetDOFPos
// Description:
//////////////////////////////////////////////////////////////////////
void fxUtlGetDOFPos(OBJECTDOF* dof, piMATRIX* mat, FLOAT64VECTOR* pos)
{
    if(dof && mat)
    {
        pos->x = dof->dofmatrix2._41;
        pos->y = dof->dofmatrix2._42;
        pos->z = dof->dofmatrix2._43;

        mathVectorMultiply64(pos, pos, mat);

        pos->z = -pos->z;

        pos->x += WorldOriginOffset.x;
        pos->y += WorldOriginOffset.y;
        pos->z += WorldOriginOffset.z;
    }
}

//////////////////////////////////////////////////////////////////////
// Function: fxUtlGetVehicleVel
// Description:
//////////////////////////////////////////////////////////////////////
SINT32 fxUtlGetVehicleVel(vehVehicle*veh, FLOAT64VECTOR* vel)
{
    if(veh)
    {
        if(veh->ModelData)
        {
            *vel = veh->ModelData->VehicleVel;
        }
        else
        {
            *vel = veh->PosData.Vel;
        }
    }

    return 0;
}

//////////////////////////////////////////////////////////////////////
// Function: fxUtlGetVehiclePos
// Description:
//////////////////////////////////////////////////////////////////////
SINT32 fxUtlGetVehiclePos(vehVehicle* veh, FLOAT64VECTOR* pos)
{
    if(veh)
    {
        if(veh->ModelData)
        {
            *pos = veh->ModelData->VehiclePos;
        }
        else
        {
            *pos = veh->PosData.Pos;
        }
    }

    return 0;
}

//////////////////////////////////////////
// Function: fxUtlSetMatrixInTree
// Description:
//////////////////////////////////////////
void fxUtlSetMatrixInTree(OBJECT* o, const piMATRIX* mat)
{
    if(o)
    {
        o->mat = *mat;

        fxUtlSetMatrixInTree(o->child, mat);
        fxUtlSetMatrixInTree(o->sibling, mat);
        fxUtlSetMatrixInTree(o->attached, mat);
    }
}

//////////////////////////////////////////
// Function: fxUtlTerrainCollide
// Description:
//////////////////////////////////////////
LOGICAL fxUtlTerrainCollide(FASTCOLLIDE* pFastCollision)
{
    UINT32 uNumCollides = 32;
    UINT32 i;

    FASTCOLLIDE collisionList[32];
    FASTCOLLIDE* pCollisionList = NULL;

    COLLISION collision;

    memset(&collision, 0, sizeof(COLLISION));

    piVECTOR vecOldPos = pFastCollision->OldPos;
    piVECTOR vecNewPos = pFastCollision->NewPos;

    mlVehicleData* pVehData = persGetPersonaModelDataPtr();

    if(pVehData != NULL)
    {
        pCollisionList = collisionList;

        trncdCollideWithListOfTerrainObjects(&pCollisionList,
                                            &uNumCollides,
                                            pVehData->MedTrnList,
                                            ML_NUM_MED_LIST_ELEMENTS,
                                            &vecOldPos,
                                            &vecNewPos,
                                            0);

        uNumCollides = 32 - uNumCollides;

        mlUtilitySortCollideList(&collisionList[0], uNumCollides);

        for(i = 0; i < uNumCollides; i++)
        {
            if (collisionList[i].Alpha > 0.5f)
            {
                *pFastCollision = collisionList[i];

                math3dVectorMultiply(&pFastCollision->CollidePnt,
                                     &pFastCollision->CollidePnt,
                                     &pFastCollision->CollideObject->mat);

                pFastCollision->CollidePnt.z = -pFastCollision->CollidePnt.z;

               return TRUE;
            }
        }
    }

    collision.oldpos = pFastCollision->OldPos;
    collision.newpos = pFastCollision->NewPos;

    if(trncdCheckTerrainDown(&collision))
    {
        pFastCollision->CollideObject = collision.colobj;
        pFastCollision->CollidePnt = collision.colpos;

        pFastCollision->CollidePnt.z = -pFastCollision->CollidePnt.z;

        return TRUE;
    }

    return FALSE;
}

//////////////////////////////////////////////
// Function: fxUtlGetGroundPos
// Description:
//////////////////////////////////////////////
void fxUtlGetGroundPos(piVECTOR* pFrom, FLOAT64VECTOR* pGround)
{
    FASTCOLLIDE fastCollide;

    memset(&fastCollide, 0, sizeof(FASTCOLLIDE));

    fastCollide.OldPos = *pFrom;
    fastCollide.NewPos = fastCollide.OldPos;

    fastCollide.NewPos.z += 10000.0f;

    if(!fxUtlTerrainCollide(&fastCollide))
        return;

    pGround->x = (FLOAT64)fastCollide.CollidePnt.x + WorldOriginOffset.x;
    pGround->y = (FLOAT64)fastCollide.CollidePnt.y + WorldOriginOffset.y;
    pGround->z = (FLOAT64)fastCollide.CollidePnt.z - 1.0f;
}

//////////////////////////////////////////
// Function: fxUtlApplyDrag
// Desctiption:
//////////////////////////////////////////
void fxUtlApplyDrag(piVECTOR* lpVel, FLOAT32* pDragCoef, FLOAT32* pLastUpdate)
{
    FLOAT32 fTimeDiff = (FLOAT32)frametime - *pLastUpdate;

    fTimeDiff *= 0.001f;

    FLOAT32 fMag = (FLOAT32)mathExactVectorLength(lpVel);
    FLOAT32 fSpeed = fMag * (FLOAT32)fexp(-*pDragCoef * fTimeDiff);

    if(fMag > 0.00001f || fMag < -0.00001f)
    {
        lpVel->x = fSpeed * lpVel->x / fMag;
        lpVel->y = fSpeed * lpVel->y / fMag;
        lpVel->z = fSpeed * lpVel->z / fMag;
    }
}

//////////////////////////////////////////
// Function: fxUtlApplyVelocity
// Desctiption:
//////////////////////////////////////////
void fxUtlApplyVelocity(piVECTOR* lpVel,
                        FLOAT64VECTOR* pPos,
                        FLOAT32* pLastUpdate,
                        UINT32 uFlags)
{
    FLOAT32 fTimeDiff = (FLOAT32)frametime - *pLastUpdate;

    fTimeDiff *= 0.001f;

    if(uFlags & DEF_FXFLAG_RISE)
    {
        lpVel->z += 9.8f * fTimeDiff;
    }

    FLOAT32 fX = lpVel->x;
    FLOAT32 fY = lpVel->y;
    FLOAT32 fZ = lpVel->z;

    fX += fxMgrGetWind()->x;
    fY += fxMgrGetWind()->y;

    pPos->x += fTimeDiff * fX;
    pPos->y += fTimeDiff * fY;
    pPos->z += fTimeDiff * fZ;

    if(uFlags & DEF_FXFLAG_USEGRAVITY)
    {
        lpVel->z -= 9.8f * fTimeDiff;
    }
}
