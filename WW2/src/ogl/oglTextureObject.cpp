#include "syscfg.h" // Path is project specific.
/*--------------------------------------------------------------
Module: oglTextureObject.cpp
Description:
----------------------------------------------------------------*/
#include "ogl/oglTextureObject.h"
#include "ogl/States.h"
#include "osgImport/ref_ptr.h"
#include "draw/sneaky.h"

using TboMap = std::map<std::string, ref_ptr<oglTextureObject>>;
static TboMap tbos;

/*--------------------------------------------------------------
Function: oglTextureObjectInit
Description:
----------------------------------------------------------------*/
void oglTextureObjectInit()
{
    oglTextureObject::GetOrCreateTextureObject("dynamicglobal", MAX_TBO_SIZE, GL_RGBA32F_ARB, GL_DYNAMIC_DRAW);
}

/*--------------------------------------------------------------
Function: oglTextureObject
Description:
----------------------------------------------------------------*/
oglTextureObject::oglTextureObject() :
	buffer(0),
	texture(0),
    bytesize(0),
    internalformat(0)
{
}

/*--------------------------------------------------------------
Function: ~oglTextureObject
Description:
----------------------------------------------------------------*/
oglTextureObject::~oglTextureObject()
{
}

/*--------------------------------------------------------------
Function: Kill
Description:
----------------------------------------------------------------*/
void oglTextureObject::Kill()
{
    if(!tbos.empty())
    {
    TboMap::iterator itr = tbos.find(tboname);

    if(itr != tbos.end())
    {
        tbos.erase(itr);
    }
    }

	glDeleteTextures(1, &texture);
    texture = 0;

	glDeleteBuffersARB(1, &buffer);
    buffer = 0;
}

/*--------------------------------------------------------------
Function: GetTextureObject
Description:
----------------------------------------------------------------*/
oglTextureObject* oglTextureObject::GetTextureObject(const std::string& name)
{
    if(tbos.find(name) != tbos.end())
    {
        return tbos.find(name)->second.get();
    }

    return nullptr;
}

/*--------------------------------------------------------------
Function: GetOrCreateTextureObject
Description:
----------------------------------------------------------------*/
oglTextureObject* oglTextureObject::GetOrCreateTextureObject(const std::string& name,
                                                             unsigned int size,
                                                             int format,
                                                             GLenum usage)
{
    if(tbos.find(name) != tbos.end())
    {
        return tbos.find(name)->second.get();
    }

    oglTextureObject* tbo = new oglTextureObject;
    tbo->SetName(name);
    tbo->Create(size, format, usage);
    tbos[name] = tbo;

    return tbo;
}

/*--------------------------------------------------------------
Function: Create
Description:
----------------------------------------------------------------*/
void oglTextureObject::Create(int size, int format, GLenum usage)
{
	bytesize = size;
	internalformat = format;

	glGenBuffers(1, &buffer);
    TextureBufferObjectBind::BindBuffer(buffer);
    glBufferData(GL_TEXTURE_BUFFER, bytesize, 0, usage);
    TextureBufferObjectBind::BindBuffer(0);

	gl_GenTextures(1, &texture);
    TextureState::Bind(0, texture, GL_TEXTURE_BUFFER);
	glTexBufferEXT(GL_TEXTURE_BUFFER, internalformat, buffer);
    TextureState::Disable();
}

/*--------------------------------------------------------------
Function: UpdateBuffer
Description:
----------------------------------------------------------------*/
bool oglTextureObject::UpdateBuffer(int size, void* data, int offset)
{
#ifdef BETA_RELEASE
    static SINT32 timerid = -1; 			\
    logTimerBegin(__FUNCTION__, "", &timerid);
#endif

	if((unsigned int)(offset + size) <= bytesize)
	{
        TextureBufferObjectBind::BindBuffer(buffer);
		glBufferSubData(GL_TEXTURE_BUFFER, offset, size, data);

#ifdef BETA_RELEASE
        logTimerEnd(timerid);
#endif

        return true;
	}

#ifdef BETA_RELEASE
    logTimerEnd(timerid);
#endif

    return false;
}
