#include "syscfg.h" // Path is project specific.
//////////////////////////////////////////////////////////////////////
// Module: lddds.cpp
// Description:
//////////////////////////////////////////////////////////////////////
#include "pi/pi.h"
#include "ld/lddds.h"

#include "ld/ldtexture.h"

extern std::string optionalFileSuffix;

//////////////////////////////////////////////////////////////////////
// Function: ldddsLoadDDSTexture2D
// Description: Used for legacy texure loading through ldpix.
//////////////////////////////////////////////////////////////////////
SINT32 ldddsLoadDDSTexture2D(const SCHAR* name, UINT32 transColor, UINT32 flags)
{
    return ldddsLoadDDSTexture(name, GL_TEXTURE_2D, flags);
}

//////////////////////////////////////////////////////////////////////
// Function: ldddsGenerateTexture
// Description: Used for creating a opengl texture only.
//////////////////////////////////////////////////////////////////////
LOGICAL ldddsGenerateTexture(SINT32 texid)
{
    return TRUE;
}

//////////////////////////////////////////////////////////////////////
// Function: ldddsLoadDDSTexture
// Description: Used for directly loading in texture, not through
// ldpix. Uses engine texture object.
//////////////////////////////////////////////////////////////////////
SINT32 ldddsLoadDDSTexture(const SCHAR* name, GLenum target, UINT32 flags)
{	
	std::string str = name;
	std::string::size_type pos = str.rfind('.');
	std::string ext = ".";
	if(pos != std::string::npos)
	{
		ext = ext + str.substr(pos+1);
		str.erase(pos, str.length()-pos);
	}
	
	// If suffix is defined (theme) and file exists, load that instead
	std::string fileName = name;
	
	if (fioFileExists(str, optionalFileSuffix, ext))
		fileName = str + optionalFileSuffix + ext;
	
    return ldTextureLoad(fileName, flags);
}

//////////////////////////////////////////////////////////////////////
// Function: ldddsCopyDDSTexture
// Description:
//////////////////////////////////////////////////////////////////////
SINT32 ldddsCopyDDSTexture(SINT32 from, SCHAR* to, UINT32 flags)
{
    return -1;
}

//////////////////////////////////////////////////////////////////////
// Function: ldddsTextureKill
// Description:
//////////////////////////////////////////////////////////////////////
void ldddsTextureKill(TEXTURE* t)
{
}
