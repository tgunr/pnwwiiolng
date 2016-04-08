#include <syscfg.h> // Path is project specific.
/*-------------------------------------------------------------------------------------
Module: HeightField.cpp
Description:
--------------------------------------------------------------------------------------*/
#include "physics/HeightField.h"
#include "osgImport/ref_ptr.h"
#include "ld/ldtexture.h"
#include "scen/ScenarioManager.h"
#include "ld/ldpix.h"
#include "sdm/sdm.h"
#include "ob/obmain.h"

static std::map<std::string, HeightField*> nameRegister;
static std::map<const OBJECT*, HeightField*> objectRegister;

/*-------------------------------------------------------------------------------------
Function: HeightField::GetHeightField
Description:
--------------------------------------------------------------------------------------*/
HeightField* HeightField::GetHeightField(const std::string& name)
{
    std::map<std::string, HeightField*>::iterator itr;

    itr = nameRegister.find(name);

    if(itr != nameRegister.end())
        return itr->second;

    return nullptr;
}

/*-------------------------------------------------------------------------------------
Function: HeightField::GetHeightField
Description:
--------------------------------------------------------------------------------------*/
HeightField* HeightField::GetHeightField(const OBJECT* o)
{
    std::map<const OBJECT*, HeightField*>::iterator itr;

    itr = objectRegister.find(o);

    if(itr != objectRegister.end())
        return itr->second;

    return nullptr;
}

/*-------------------------------------------------------------------------------------
Function: HeightField::RegisterHeightField
Description:
--------------------------------------------------------------------------------------*/
HeightField& HeightField::RegisterHeightField(const std::string& name)
{
    if(nameRegister.find(name) != nameRegister.end())
    {
        return *nameRegister.find(name)->second;
    }

    HeightField* hf = new HeightField;
    nameRegister[name] = hf;

    return *hf;
}

/*-------------------------------------------------------------------------------------
Function: HeightField::RegisterHeightField
Description:
--------------------------------------------------------------------------------------*/
HeightField& HeightField::RegisterHeightField(const OBJECT* o)
{
    if(objectRegister.find(o) != objectRegister.end())
    {
        return *objectRegister.find(o)->second;
    }

    HeightField* hf = new HeightField;
    objectRegister[o] = hf;

    return *hf;
}

/*-------------------------------------------------------------------------------------
Function: HeightField::UnRegisterHeightField
Description:
--------------------------------------------------------------------------------------*/
void HeightField::UnRegisterHeightField(const std::string& name)
{
    std::map<std::string, HeightField*>::iterator itr;

    itr = nameRegister.find(name);

    if(itr != nameRegister.end())
    {
        delete itr->second;
        nameRegister.erase(itr);
    }
}

/*-------------------------------------------------------------------------------------
Function: HeightField::UnRegisterHeightField
Description:
--------------------------------------------------------------------------------------*/
void HeightField::UnRegisterHeightField(const OBJECT* o)
{
    std::map<const OBJECT*, HeightField*>::iterator itr;

    itr = objectRegister.find(o);

    if(itr != objectRegister.end())
    {
        delete itr->second;
        objectRegister.erase(itr);
    }
}

/*-------------------------------------------------------------------------------------
Function: HeightField::HeightField
Description:
--------------------------------------------------------------------------------------*/
HeightField::HeightField() :
    columns(0),
    rows(0),
    heightscale(1.0f),
    imgscalex(1.0f),
    imgscaley(1.0f)
{
}

/*-------------------------------------------------------------------------------------
Function: HeightField::Allocate
Description:
--------------------------------------------------------------------------------------*/
void HeightField::Allocate(unsigned int numColumns, unsigned int numRows)
{
    if(columns != numColumns || rows != numRows)
    {
        height.resize(numColumns * numRows);
    }

    columns = numColumns;
    rows = numRows;
}

/*-------------------------------------------------------------------------------------
Function: HeightField::RayIntersectsTriangle
Description:
--------------------------------------------------------------------------------------*/
bool HeightField::RayIntersectsTriangle(int q, const Vec3d& start, const Vec3d& ray, const Vec3d& p0, const Vec3d& p1, const Vec3d& p2, Vec3d& pos, Vec3d& normal)
{
    // Reversed because of our f-up'd coordinate system
    normal = -((p1 - p0) ^ (p2 - p0));
    normal.normalize();

    if(normal * ray == 0.0)
        return false;

    double t = -((start - p0) * normal) / (normal * ray);

    if(t >= 0.0 && t <= 1.0)
    {
        pos = start + ray * t;

        double inside;

        inside = ((p1 - p0) ^ (pos - p0)) * normal;

        if(inside < 0)
            return false;

        inside = ((p2 - p1) ^ (pos - p1)) * normal;

        if(inside < 0)
            return false;

        inside = ((p0 - p2) ^ (pos - p2)) * normal;

        if(inside < 0)
            return false;

        return true;
    }

    return false;
}

/*-------------------------------------------------------------------------------------
Function: HeightField::CollideRay
Description: Returns pos, normal
--------------------------------------------------------------------------------------*/
bool HeightField::CollideRay(const Vec3d& newpos, const Vec3d& oldpos, Vec3d& pos, Vec3d& normal)
{
#if(0)
    // Above boundbox height
    if(-newpos.z() > heightscale * 255.0)
        return false;

    unsigned int c1 = unsigned int((oldpos.x() + origin.x()) * imgscalex);
    unsigned int r1 = unsigned int((oldpos.y() + origin.y()) * imgscaley);
    unsigned int c2 = unsigned int((newpos.x() + origin.x()) * imgscalex);
    unsigned int r2 = unsigned int((newpos.y() + origin.y()) * imgscaley);

    Vec3d p0 = GetVertex(c1, r1);
    Vec3d p1 = GetVertex(c1, r2);
    Vec3d p2 = GetVertex(c2, r2);
    Vec3d p3 = GetVertex(c2, r1);

    p0.z() = -p0.z();
    p1.z() = -p1.z();
    p2.z() = -p2.z();
    p3.z() = -p3.z();

    Vec3d ray = newpos - oldpos;

    if(RayIntersectsTriangle(0, oldpos, ray, p0, p1, p2, pos, normal))
    {
        return true;
    }

    if(RayIntersectsTriangle(1, oldpos, ray, p1, p3, p2, pos, normal))
    {
        return true;
    }
#endif

    return false;
}

/*-------------------------------------------------------------------------------------
Function: HeightField::Collide
Description: Returns pos, normal
--------------------------------------------------------------------------------------*/
void HeightField::Collide(double x, double y, Vec3d& pos, Vec3d& normal)
{
    unsigned int c = int((x + origin.x()) * imgscalex);
    unsigned int r = int((y + origin.y()) * imgscaley);

    float h = GetHeight(c, r);

    h += GetHeight(c + 1, r);
    h += GetHeight(c - 1, r);
    h += GetHeight(c, r + 1);
    h += GetHeight(c, r - 1);

    h /= 5.0f;

    normal = GetNormal(c, r);

    pos.x() = x;
    pos.y() = y;
    pos.z() = h;
}

/*-------------------------------------------------------------------------------------
Function: HeightField::Collide
Description: Returns height
--------------------------------------------------------------------------------------*/
float HeightField::Collide(double x, double y)
{
    unsigned int c = int((x + origin.x()) * imgscalex);
    unsigned int r = int((y + origin.y()) * imgscaley);

    float h = GetHeight(c, r);

    return h;
}

/*-------------------------------------------------------------------------------------
Function: HeightField::GetNormal
Description:
--------------------------------------------------------------------------------------*/
Vec3d HeightField::GetNormal(unsigned int c, unsigned int r) const
{
    float dz_dx;

    if(c >= GetNumColumns())
        c = GetNumColumns() - 1;

    if(r >= GetNumRows())
        r = GetNumRows() - 1;

    if(c == 0)
        dz_dx = (GetHeight(c + 1, r) - GetHeight(c, r)) / GetXInterval();
    else if(c == GetNumColumns() - 1)
        dz_dx = (GetHeight(c, r) - GetHeight(c - 1, r)) / GetXInterval();
    else
        dz_dx = 0.5f * (GetHeight(c + 1, r) - GetHeight(c - 1, r)) / GetXInterval();

    float dz_dy;

    if(r == 0)
         dz_dy = (GetHeight(c, r + 1) - GetHeight(c, r)) / GetYInterval();
    else if(r == GetNumRows() - 1)
        dz_dy = (GetHeight(c, r) - GetHeight(c, r - 1)) / GetYInterval();
    else
        dz_dy = 0.5f * (GetHeight(c, r + 1) - GetHeight(c, r - 1)) / GetYInterval();

    Vec3d normal(-dz_dx, -dz_dy, -1.0f);
    normal.normalize();

    return normal;
}

/*-------------------------------------------------------------------------------------
Function: HeightField::GetHeightDelta
Description:
--------------------------------------------------------------------------------------*/
Vec2d HeightField::GetHeightDelta(unsigned int c, unsigned int r) const
{
     Vec2f heightDelta;

    if(c==0)
        heightDelta.x() = (GetHeight(c + 1, r) - GetHeight(c, r));
    else if(c == GetNumColumns() - 1)
        heightDelta.x() = (GetHeight(c, r) - GetHeight(c - 1, r));
    else
        heightDelta.x() = 0.5f * (GetHeight(c + 1, r) - GetHeight(c - 1, r));

    if(r == 0)
        heightDelta.y() = (GetHeight(c, r + 1) - GetHeight(c, r));
    else if(r == GetNumRows() - 1)
        heightDelta.y() = (GetHeight(c, r) - GetHeight(c, r - 1));
    else
        heightDelta.y() = 0.5f * (GetHeight(c, r + 1) - GetHeight(c, r - 1));

    return heightDelta;
}

/*-------------------------------------------------------------------------------------
Function: HeightField::Create
Description: Requires SetHeightScale to be set first if necessary
--------------------------------------------------------------------------------------*/
void HeightField::Create()
{
    if(imgpath.empty())
        return;

    Image* img = ldtextureLoadImage(imgpath.c_str(), 0, 0);

    if(img)
    {
        Allocate(img->s(), img->t());

        for(int r = 0; r < img->t(); r++)
        {
            for(int c = 0; c < img->s(); c++)
            {
                float color = (float)(*img->data(c, r));
                SetHeight(c, r, color * heightscale);
            }
        }
    }
}

/*-------------------------------------------------------------------------------------
Function: HeightField::~HeightField
Description:
--------------------------------------------------------------------------------------*/
HeightField::~HeightField()
{
}
