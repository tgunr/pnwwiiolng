#ifndef _TRNTESTING_H_
#define _TRNTESTING_H_

#include "math/Vec3d.h"
#include "osgImport/Referenced.h"
#include "osgImport/ref_ptr.h"
#include "osgImport/Image.h"
#include "ob/obmain.h"

#define TS_NO_GRASS 0x00000001

class TerrainScenario : public Referenced
{
public:
    TerrainScenario() :
        max_bound_box_height(24.626f)
        {}
    virtual ~TerrainScenario() {}

    void LoadTrees(const OBJECT* o, void* data);
    void LoadTerrain(const std::string& fltname, const std::string& key);
    void LoadBuildings();
    void LoadClutterObjects(const OBJECT* o, void* data);
    void LoadGrass(const OBJECT* o, void* data);
    void GetFogParams(float& start, float& end, unsigned int& fogcolor);
    void LoadEnvironment(unsigned int flags);

    float GetBBoxHeight() const {return max_bound_box_height;}

    const char* GetMaterial(double x, double y, int id);

    static TerrainScenario& GetInstance();

protected:
    void MaskWalkBlock(Image* img,
                       void* data,
                       const Vec2f& center,
                       int goodred,
                       int goodgrn,
                       int goodblu,
                       int redweight,
                       int grnweight,
                       int bluweight,
                       unsigned short redclamp,
                       unsigned short grnclamp,
                       unsigned short bluclamp,
                       void (*callback)(const std::vector<Vec2f>& bucket, int color, void* data));

    void MaskWalkTexel(Image* img,
                       void* data,
                       const Vec2f& center,
                       int goodred,
                       int goodgrn,
                       int goodblu,
                       int redweight,
                       int grnweight,
                       int bluweight,
                       unsigned short redclamp,
                       unsigned short grnclamp,
                       unsigned short bluclamp,
                       void (*callback)(const std::vector<Vec2f>& bucket, int color, void* data));

    float max_bound_box_height;
};

#endif
