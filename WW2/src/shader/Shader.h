#pragma once

#include "pi/pi.h"
#include "Cg/cg.h"
#include "Cg/cgGL.h"

class CWW2Shader
{
public:
    CWW2Shader();
    CWW2Shader(const SCHAR* shader, SINT32 shaderType);
    virtual ~CWW2Shader();

    const SCHAR* getShaderName() { return m_shadername; }

    LOGICAL initialize();
    LOGICAL isCorrectVersion();
    LOGICAL isProgram();

    void activate();
    void deactivate();
    void killProgram();
    void disableTexture(const SCHAR* param);
    void enableTexture(const SCHAR* param);
    void loadProgram(const SCHAR* program, SINT32 shaderType);

    void setModelViewProjectionMatrix(const SCHAR* param);
    void setModelViewMatrix(const SCHAR* param);
    void setModelViewITMatrix(const SCHAR* param);
    void setTextureMatrix(const SCHAR* param);

    void setMatrix(const SCHAR* param, piMATRIX& m);
    void setMatrix(const SCHAR* param, const FLOAT32 m[16]);
    void setParam4f(const SCHAR* param, FLOAT32 x, FLOAT32 y, FLOAT32 z, FLOAT32 w);
    void setParam4f(const SCHAR* param, piCOLORVALUE& color);
    void setParam4f(const SCHAR* param, piVECTOR& v);
    void setParam4f(const SCHAR* param, FLOAT32 x);
    void setParam4f(const SCHAR* param, UINT32 color);
    void setParam3f(const SCHAR* param, FLOAT32 x, FLOAT32 y, FLOAT32 z);
    void setParam3f(const SCHAR* param, piVECTOR& v);
    void setParam3f(const SCHAR* param, piCOLORVALUE& color);
    void setParam2f(const SCHAR* param, FLOAT32 x, FLOAT32 y);
    void setParam1f(const SCHAR* param, FLOAT32 x);
    void setTexture(const SCHAR* param, SINT32 texture);
    void setTexture2(const SCHAR* param, UINT32 genlist);

    void getMatrix(const SCHAR* param, piMATRIX& m);

    static void lockShader(LOGICAL lock);

    enum
    {
        NOT_USED,
        TYPE_VERTEX,
        TYPE_FRAGMENT
    };

    static LOGICAL m_lock;

private:
    CGparameter getParam(const SCHAR* param);

    CGprogram m_cgPgm;

    CGprofile m_cgProfile;

    CGprofile m_cgVprofile;
    CGprofile m_cgFprofile;

    CGcontext m_cgContext;

    SCHAR m_shadername[32];
};

