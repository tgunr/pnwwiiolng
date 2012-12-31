#pragma once

#include "osgImport/Referenced.h"
#include "math/Vec2d.h"
#include "math/Vec3d.h"
#include "math/Quat.h"
#include "math/Matrixf.h"

struct OBJECT;

class HeightField : public Referenced
{
public:
    HeightField();
    virtual ~HeightField();

    void Create();
    void SetImagePath(const std::string& path) {imgpath = path;}
    void SetHeightScale(float scale = 1.0f) {heightscale = scale;}
    void SetImageScaleX(float scale = 1.0f) {imgscalex = scale;}
    void SetImageScaleY(float scale = 1.0f) {imgscaley = scale;}
    void Allocate(unsigned int numColumns, unsigned int numRows);

    void Collide(double x, double y, Vec3d& pos, Vec3d& normal);
    float Collide(double x, double y);
    bool CollideRay(const Vec3d& newpos, const Vec3d& oldpos, Vec3d& pos, Vec3d& normal);

    Vec2d GetHeightDelta(unsigned int c, unsigned int r) const;
    Vec3d GetNormal(unsigned int c, unsigned int r) const;

    inline void SetOrigin(const Vec3d& o) {origin = o;}
    inline const Vec3d& GetOrigin() const {return origin;}
    inline unsigned int GetNumColumns() const {return columns;}
    inline unsigned int GetNumRows() const {return rows;}
    inline void SetXInterval(float ddx) {dx = ddx;}
    inline float GetXInterval() const {return dx; }
    inline void SetYInterval(float ddy) {dy = ddy; }
    inline float GetYInterval() const {return dy;}

    inline void SetHeight(unsigned int c, unsigned int r, float value)
    {
        height[c + r * columns] = value;
    }

    inline float GetHeight(unsigned int c, unsigned int r) const
    {
        if(c >= GetNumColumns())
            c = GetNumColumns() - 1;

        if(r >= GetNumRows())
            r = GetNumRows() - 1;

        return height[c + r * columns];
    }

    inline Vec3d GetVertex(unsigned int c, unsigned int r) const
    {
        return Vec3d(GetXInterval() * (float)c - origin.x(),
                     GetYInterval() * (float)r - origin.y(),
                    (origin.z() + height[c + r * columns]));
    }

    static HeightField* GetHeightField(const std::string& name);
    static HeightField* GetHeightField(const OBJECT* o);
    static HeightField& RegisterHeightField(const std::string& name);
    static HeightField& RegisterHeightField(const OBJECT* o);
    static void UnRegisterHeightField(const std::string& name);
    static void UnRegisterHeightField(const OBJECT* o);

protected:
    bool RayIntersectsTriangle(int q, const Vec3d& start, const Vec3d& ray, const Vec3d& p0, const Vec3d& p1, const Vec3d& p2, Vec3d& pos, Vec3d& normal);

    std::vector<float> height;
    std::string imgpath;

    Vec3d origin;

    unsigned int columns;
    unsigned int rows;

    float heightscale;
    float imgscalex;
    float imgscaley;
    float dx;
    float dy;
};
