#pragma once

#include "osgImport/Referenced.h"

#define MAX_TBO_SIZE    65536

class oglTextureObject;

class oglTextureObject : public Referenced
{
public:
	oglTextureObject();
	virtual ~oglTextureObject();

    void SetName(const std::string& name) {tboname = name;}
    void Kill();

    static oglTextureObject* GetOrCreateTextureObject(const std::string& name,
                                                      unsigned int size,
                                                      int format,
                                                      GLenum usage);
    static oglTextureObject* GetTextureObject(const std::string& name);

    unsigned int GetTextureObjectName() {return texture;}
    unsigned int GetBufferName() {return buffer;}

    unsigned int GetSize() {return bytesize;}

    GLenum GetTextureTarget() {return GL_TEXTURE_BUFFER;}

    bool UpdateBuffer(int size, void* data, int offset = 0);

protected:
	void Create(int bytesize, int format, GLenum usage);

    unsigned int buffer;
	unsigned int texture;
	unsigned int bytesize;
	int internalformat;
    std::string tboname;
};

