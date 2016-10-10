#pragma once

#include <map>
#include <string>

struct piTextureInfo
{
	int texid;
	float uv[4];
};

class piTextureAtlasSet
{
public:
	int loadAtlas(std::string const&);
	void getTexture(std::string const&, piTextureInfo&, bool repeatingTexture=false);

	static piTextureAtlasSet* getInstance();

protected:
	using TextureMap = std::map<std::string, piTextureInfo>;
	TextureMap _textureMap;
};
