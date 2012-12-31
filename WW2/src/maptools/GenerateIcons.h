#pragma once

#ifdef GENICONS

#include "osgImport/Referenced.h"
#include "ob/obMgr.h"
#include <vector>
#include <set>

struct texture_data
{
  std::string name;
  GLuint texid;
  int width;
  int height;
  float mpp;
  int drawWidth;
  int drawHeight;
  float drawMpp;

  float minx;
  float miny;

  texture_data() {width=height=0; mpp=0.5; texid=-1;}
};

class GenerateIcons : public Referenced
{
public:
  GenerateIcons();
  virtual ~GenerateIcons();
  void Start(bool objects);
  void Apply();
  void End();
  void LoadModel();
  void Capture();
  bool IsEnabled() {return _enabled;}

private:

  void markAllIconifiedModels();
  void MakeAtlas();
  void SaveToTexture(const char*name, int x, int y, int w, int h, double minx, double miny);
  void SetToOrigin(float& x, float& y);
  bool wantModel(UINT32 mt);

  bool _enabled;
  std::vector<texture_data> textures;
  PoolMap::const_iterator _poolitr;
  std::set<std::string> TerrainObjectSpecials;

  BoundingBox bb;

  LOGICAL _oldpostfilter;
  LOGICAL _oldradclutter;
  LOGICAL _oldwatershader;
  unsigned int _oldshadowmapflags;

  float altitude;
  bool doObjects;
};

GenerateIcons& GetIconGenerator();
#endif

#if platOS == OS_WIN32
#ifdef BETA_RELEASE
#include "osgImport/Referenced.h"
#include "scen/ScenarioManager.h"
#include "osgImport/Image.h"
#include "ob/obdlite.h"

class GenerateScenarioMap : public Referenced
{
public:

    enum
    {
        GENMAP_GET_NUMBER_OF_TERRAIN_TILES,
        GENMAP_SET_DEFAULTS,
        GENMAP_GET_CAM_POSITIONS,
        GENMAP_SCREEN_CAPTURE,
        GENMAP_MOVE_CAMERA,
        GENMAP_SAVE_TEXTURES,
        GENMAP_END
    };

    GenerateScenarioMap() :
        start(false),
        state(GENMAP_GET_NUMBER_OF_TERRAIN_TILES),
        oldpostfilter(FALSE),
        resolution(400.0f),
        positionindex(0),
        oldshadowflags(0),
        listindex(0),
        oldwaterreflections(FALSE)
    {
    }

    virtual ~GenerateScenarioMap()
    {
    };

    void Start(float res = 400.0f)
    {
        resolution = res;
        start = true;
    }

    void End()
    {
        start = false;
    }

    void Apply();
    void StartRenderDefaults();
    void EndRenderDefaults();
    void MoveCamera();
    void ScreenCapture();
    void GetOctetCamPositions();
    void SaveImages();

    static GenerateScenarioMap& GetInstance();

protected:
    int state;
    int positionindex;
    int listindex;
    bool start;
    float resolution;
    std::vector<Vec3f> positions;
    std::vector<Image*> images;
    LOGICAL oldpostfilter;
    LOGICAL oldwaterreflections;
    LIGHT oldsun;
    unsigned int oldshadowflags;
    SCENOBJ_LIST list;
};
#endif
#endif
