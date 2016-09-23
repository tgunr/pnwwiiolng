#include <syscfg.h> // Path is project specific.
#include "pi/piTextureAtlas.h"
#include "pi/pi.h"
#include "ld/ldpix.h"
#include "fio/fio.h"
#include "ui/uiMain.h"

int piTextureAtlasSet::loadAtlas(std::string const& name)
{
	int texid = ldpixLoadImageFileToSurface(name.c_str(), colorMAGENTA | colorALPHA100, LDPIX_NONE);
	if(texid < 0) return -1;

	std::string atlasName = name;
	atlasName.erase(atlasName.find_last_of('.'));
	atlasName += ".atlas";

	int fd = fioOpen(atlasName.c_str());
	if(fd < 0) return -1;

	piTextureInfo info;
	info.texid = texid;

	char line[256];
	char mapName[32];
	for(;;)
	{
		if(fioGetLine(fd, line,255) < 1) break;
		if(sscanf(line, "%31s %f %f %f %f", mapName, &info.uv[0], &info.uv[1], &info.uv[2], &info.uv[3]) != 5) break;
		_textureMap[mapName] = info;
	}

	fioClose(fd);

	return 0;

}

void piTextureAtlasSet::getTexture(std::string const& filename, piTextureInfo& info, bool repeatingTexture)
{
	size_t start = filename.find_last_of("/\\");
	start = (start == std::string::npos) ? 0 : start + 1;
	size_t end = filename.find_last_of('.');
	size_t n = (end == std::string::npos) ? filename.length()-start : end-start;
	std::string name = filename.substr(start,n);

	if(!repeatingTexture)
	{ // dynamically create if separate texture doesn't exist? (once most everything in atlas)
		TextureMap::iterator it = _textureMap.find(name);
		if(it != _textureMap.end())
		{
			info.texid = it->second.texid;
			info.uv[0] = it->second.uv[0];
			info.uv[1] = it->second.uv[1];
			info.uv[2] = it->second.uv[2];
			info.uv[3] = it->second.uv[3];
			return;
		}
	}
#if defined(NOATLAS_DEBUG)
	info.texid = uiGetTexture("null1.bmp");
#else
	info.texid = uiGetTexture(filename.c_str());
#endif
	TEXTURE* texture = piGetTextureData(info.texid );
	if(!texture) return;

	info.uv[0] = 0;
	info.uv[1] = 0;
	info.uv[2] = texture->widthfactor[0];
	info.uv[3] = texture->heightfactor[0];
}

piTextureAtlasSet* piTextureAtlasSet::getInstance()
{
	static piTextureAtlasSet* atlasSet;
	if(!atlasSet) atlasSet = new piTextureAtlasSet;
	return atlasSet;
}
