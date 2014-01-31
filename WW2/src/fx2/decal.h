#pragma once

struct DECAL
{
    SINT32 createtime;
    SINT32 textureindex;
    SINT32 lifetime;
    SINT32 id;
    SINT32 uvindex;
    SINT32 vertexindex;
    SINT32 normalindex;
    SINT32 tangentindex;
    SINT32 hitmapuv;
    SINT32 stride;
    SINT32 polygon;

    bool dead;

    OBJECT* o;

    piVECTOR pos;
    piVECTOR normal;

    piMATRIX world;

    FLOAT32 alpha;
    FLOAT32 zdepth;

    std::vector<FLOAT32> _data;

    DECAL()
	: createtime(frametime)
	, textureindex(0)
	, lifetime(0)
	, id(0)
	, uvindex(0)
	, vertexindex(0)
	, normalindex(0)
	, tangentindex(0)
	, hitmapuv(0)
	, stride(0)
	, polygon(0)
	, dead(false)
	, o(NULL)
	, world(identity)
	, alpha(0.0f)
	, zdepth(0.0f)
	, _data()
    {
        memset(&pos, 0, sizeof(piVECTOR));
        memset(&normal, 0, sizeof(piVECTOR));
    }

    ~DECAL()
    {
        _data.clear();
    }

    SINT32 age() {return frametime - createtime;}
    bool expired() {return lifetime != -1 && age() > lifetime;}

};

class DecalMgr
{
public:
    DecalMgr();
    virtual ~DecalMgr();

    void init();
    void draw();
    void kill();
    void update();
    void remove(SINT32 key);
    void setup(DECAL* d);
    void getMatrixFromNormal(DECAL& decal);
    void setupPoint(piVECTOR& v, FLOAT32 x, FLOAT32 y, FLOAT32 z, DECAL* d);
    void getuv(OBJECT* o, SINT32 polygon, const piVECTOR& pos, piUV& uv);

    SINT32 add(OBJECT* o, const piVECTOR& pos, SINT32 polygon);

private:
    std::map<SINT32, DECAL *> _decals;
    std::vector<DECAL *> _drawlist;
    std::vector<UINT16> _indices;
};

extern DecalMgr _decalmgr;
