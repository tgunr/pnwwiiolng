#pragma once

#include <map>
#include <string>
#include "osgImport/ref_ptr.h"
#include "shader/glslProgram.h"

class glslShader;

class OpenGLShader
{
public:
    OpenGLShader() {}
    virtual ~OpenGLShader();

    ref_ptr<glslShader> loadVertexShader(const char* shader);
    ref_ptr<glslShader> loadFragmentShader(const char* shader);
    ref_ptr<glslShader> loadGeometryShader(const char* shader);

    glslProgram* loadProgram(const char* name, const char* v = nullptr, const char* f = nullptr);
    glslProgram* getProgram(const char* name);

    void update();
    void Kill()
    {
        _programs.clear();
        _vshaders.clear();
        _fshaders.clear();
        _gshaders.clear();
    }

private:

    void _loadShaderSource(glslShader* shader, const char* file);

    std::map<std::string, ref_ptr<glslProgram> > _programs;
    std::map<std::string, ref_ptr<glslShader> > _vshaders;
    std::map<std::string, ref_ptr<glslShader> > _fshaders;
    std::map<std::string, ref_ptr<glslShader> > _gshaders;
};

extern OpenGLShader GLSL;

extern bool glslInit();
extern bool glslPostRenderUpdate();
extern bool glslPreRenderUpdate();
extern bool glslLoadShaderXML(const char* file);

extern void glslSetModelViewMatrix();
extern void glslSetProjectionMatrix();
extern void glslSetTextureMatrix(int unit, const piMATRIX& mat);
extern void glslSetTextureMatrix(int unit, const Matrixf& mat);
extern void glslKill();

extern void glslLoadGlobalUniforms(glslProgram* pgm);

