#include <syscfg.h> // Path is project specific.
/*------------------------------------------------------------------------
Module: treeData.cpp
Description:
------------------------------------------------------------------------*/
#include "math/Vec3d.h"
#include "math/Vec3f.h"
#include "Tree.h"
#include "treeData.h"
#include "xml/XMLLoader.h"
#include "scen/ScenarioManager.h"
#include "fio/fio.h"
#include "wwii/wwiiHash.h"
USING_EXT_NAMESPACE;

#include <vector>
#include <deque>
#include <map>

#ifdef SPATIAL_DB
#include "SpatialDB.h"
#endif

#define VIEW_DEMO_TREES     0

#if VIEW_DEMO_TREES
static std::string directory = "C:/3rdparty/SpeedTreeRT_SDK_v4.1_Full/Bin/Data/TreeModels.SouthCarolina/";
#endif

static bool _initialized = false;

static std::vector<TreeData*> treedata;
static std::vector<CTree*> basetrees;
static std::map<std::string, CTree*> treecache;

static int texCoordOffset = 0;
static int totaltrees = 0;

static std::vector<float> billboardTexCoords;

static ref_ptr<oglVertexBuffer> vbo;

typedef stdext::hash_set<unsigned int> TreeSet;
static TreeSet activetrees;

#if USE_PREALLOCATED_TREES
static Tree* freetrees;
static Tree freetreesH;
static Tree freetreesT;
static Tree activetreesH;
static Tree activetreesT;
#endif

static void addToFreeList(Tree* t);

/*-----------------------------------------------------------------
 Function: treeDataVBO
 Description:
------------------------------------------------------------------*/
oglVertexBuffer* treeDataGetVBO()
{
    return vbo.get();
}

/*-----------------------------------------------------------------
 Function: _treename
 Description:
------------------------------------------------------------------*/
bool _treename(XMLLoader* loader, xmlNode* node, bool first, void* userData)
{
    if(first)
    {
        xmlChar* s = xmlGetProp(node, (xmlChar*)"name");

        TreeData* td = new TreeData;
        td->name = (const char*)s;

        typedef std::vector<TreeData *> *p;

        treedata.push_back(td);

        XML_PROCESS_CHILDREN(td);

        xmlFree(s);
    }

    return false;
}

/*-----------------------------------------------------------------
 Function: _treenear
 Description:
------------------------------------------------------------------*/
bool _treenear(XMLLoader* loader, xmlNode* node, bool first, void* userData)
{
    if(first)
    {
        xmlChar* s = xmlNodeGetContent(node);
        ((TreeData*)userData)->nearlod = (float)atof((char*)s);

        XML_PROCESS_CHILDREN(userData);

        xmlFree(s);
    }

    return false;
}

/*-----------------------------------------------------------------
 Function: _treefar
 Description:
------------------------------------------------------------------*/
bool _treefar(XMLLoader* loader, xmlNode* node, bool first, void* userData)
{
    if(first)
    {
        xmlChar* s = xmlNodeGetContent(node);
        ((TreeData*)userData)->farlod = (float)atof((char*)s);

        XML_PROCESS_CHILDREN(userData);

        xmlFree(s);
    }

    return false;
}

/*------------------------------------------------------------------------
Module: loadXML
Description:
------------------------------------------------------------------------*/
static void loadXML(const char* path)
{
    std::string dir = path;

#if CHRISTMAS
    dir += "Christmas_Tree/trees.xml";
#else

    if(gCustomTerrain)
    {
        dir = SCEN_MGR->MapPath("trees.xml");

        if(!fioAccess(dir.c_str()))
        {
            dir = path;
            dir += "trees.xml";
        }
    }
    else
    {
        dir += "trees.xml";
    }
#endif

    XMLLoader load(dir.c_str());

    load.AddHandler("tree", _treename);
    load.AddHandler("near", _treenear);
    load.AddHandler("far", _treefar);

    load.Load();
}

/*------------------------------------------------------------------------
Module: _createCollider
Description: TODO: ODE
------------------------------------------------------------------------*/
static void _createCollider(CTree* tree)
{
    int n = tree->GetNumCollisionObjects();

    Vec3f pos, dim, angles;
    CSpeedTreeRT::ECollisionObjectType type;

    // We should only have one collision object per tree, if that tree is collidable.
    for(int i = 0; i < n; i++)
    {
        tree->GetCollisionObject(i, type, pos.ptr(), dim.ptr(), angles.ptr());

        if(dim[0] < 0 || dim[1] < 0 || dim[2] < 0)
        {
	        continue;
        }

        dGeomID geom = 0;

        switch(type)
        {
        case CSpeedTreeRT::CO_SPHERE:
            {
                geom = dCreateSphere(0, dim[0]);
                break;
            }
        case CSpeedTreeRT::CO_CAPSULE:
            {
                geom = dCreateCapsule(0, dim[0] * 1.5f, dim[1]);
                break;
            }
        case CSpeedTreeRT::CO_BOX:
            {
                geom = dCreateBox(0, dim.x(), dim.y(), dim.z());
                break;
            }
        };

        dGeomSetCategoryBits(geom, COLLIDER_TRN_TREE);
        tree->AddCollisionGeometryObject(geom);
    }
}

/*------------------------------------------------------------------------
Module: loadDirectory
Description: For Debugging usage in loading SpeedTree demo trees
------------------------------------------------------------------------*/
#if(0)
static void loadDirectory(const std::string& dir)
{
#if platOS == OS_WIN32
    WIN32_FIND_DATAA entry;
    std::string path = dir + "*";

    HANDLE h = FindFirstFileA(path.c_str(), &entry);

    if(h != INVALID_HANDLE_VALUE)
    {
        do
        {
            std::string name = entry.cFileName;

            if(name.find(".spt") != std::string::npos)
            {
                TreeData* td = new TreeData;

                td->name = name;
                td->nearlod = 100.0f;
                td->farlod = 150.0f;

                typedef std::vector<TreeData *> *p;

                treedata.push_back(td);
            }

        }while(FindNextFileA(h, &entry));
    }

    FindClose(h);

#endif
}
#endif

/*------------------------------------------------------------------------
Module: loadTrees
Description:
------------------------------------------------------------------------*/
static void loadTrees(CSpeedWind* wind)
{
    CTree* base = nullptr;

    for(unsigned int i = 0; i < treedata.size(); i++)
    {
        std::map<std::string, CTree*>::iterator find;

        find = treecache.find(treedata[i]->name);

        // Already loaded
        if(find != treecache.end())
        {
            basetrees.push_back(find->second);
            continue;
        }

        base = new CTree;

        bool ret = base->LoadSPTFile(treedata[i]->name.c_str(),
                                     1,
                                     treedata[i]->nearlod,
                                     treedata[i]->farlod,
                                     wind);

        if(!ret)
        {
            delete base;
            continue;
        }

        _createCollider(base);

        if(ogl::UseDeprecatedPath())
        {
            base->SetupBranchGeometryDeprecated();
            base->SetupFrondGeometryDeprecated();
            base->SetupLeafCardGeometryDeprecated();
        }
        else
        {
            base->SetupBranchGeometry();
            base->SetupFrondGeometry();
            base->SetupLeafCardGeometry();
        }

        base->LoadBranchTexture();
        base->LoadLeafFrondTexture();
        base->LoadBillboardTexture();

        base->SetupBillboardGeometry(texCoordOffset, billboardTexCoords);

        base->DeleteTransientData();

        treecache[treedata[i]->name] = base;
        basetrees.push_back(base);
    }

    vbo->Buffer();
}

/*------------------------------------------------------------------------
Module: treeClampBaseIfNecessary
Description:
------------------------------------------------------------------------*/
void treeClampBaseIfNecessary(int& base)
{
    if(base < 0)
        base = 0;
    else if(base >= (int)basetrees.size())
        base = (int)basetrees.size() - 1;
}

/*------------------------------------------------------------------------
Module: treeDataGetBaseTree
Description:
------------------------------------------------------------------------*/
CTree* treeDataGetBaseTree(int base)
{
    if(base < 0 || base >= (int)basetrees.size())
        return nullptr;

    return basetrees[base];
}

/*------------------------------------------------------------------------
Module: treeDataGetBillboardTexCoords
Description:
------------------------------------------------------------------------*/
const std::vector<float>& treeDataGetBillboardTexCoords()
{
    return billboardTexCoords;
}

/*------------------------------------------------------------------------
Module: initTreeInstances
Description:
------------------------------------------------------------------------*/
static void initTreeInstances()
{
#if USE_PREALLOCATED_TREES
    freetreesH.next = &freetreesT;
    freetreesT.prev = &freetreesH;

    activetreesH.next = &activetreesT;
    activetreesT.prev = &activetreesH;
#endif
}

/*------------------------------------------------------------------------
Module: treeDataInit
Description:
------------------------------------------------------------------------*/
void treeDataInit(const char* path, CSpeedWind* wind)
{
    vbo = new oglVertexBuffer;

    CTree::SetTreeDirectory(path);

    initTreeInstances();

#if VIEW_DEMO_TREES
    CTree::SetTreeDirectory(directory);
    loadDirectory(directory);
#else
    // Read trees.xml for tree specific properties
    loadXML(path);
#endif

    // Load trees specified in trees.xml
    loadTrees(wind);
}

#if USE_PREALLOCATED_TREES
/*------------------------------------------------------------------------
Module: addToFreeList
Description:
------------------------------------------------------------------------*/
static void addToFreeList(Tree* t)
{
    Tree* n = freetreesH.next;

    freetreesH.next = t;
    t->prev = &freetreesH;

    t->o = nullptr;
    t->octet = nullptr;
    t->collidemat.clear();
    t->isbush_isclutter_base = 0;

    t->next = n;
    n->prev = t;
}

/*------------------------------------------------------------------------
Module: releaseActiveTree
Description:
------------------------------------------------------------------------*/
static void releaseActiveTree(Tree* t)
{
    Tree* p = t->prev;
    Tree* n = t->next;

    p->next = n;
    n->prev = p;

    t->next = nullptr;
    t->prev = nullptr;
}

/*------------------------------------------------------------------------
Module: addToActiveList
Description:
------------------------------------------------------------------------*/
static void addToActiveList(Tree* t)
{
    Tree* n = activetreesH.next;

    activetreesH.next = t;
    t->prev = &activetreesH;

    n->prev = t;
    t->next = n;
}
#endif // USE_PREALLOCATED_TREES

/*------------------------------------------------------------------------
Module: treeDataGetFreeTree
Description:
------------------------------------------------------------------------*/
Tree* treeDataGetFreeTree()
{
    Tree* t = nullptr;

#if USE_PREALLOCATED_TREES
    if(freetreesH.next == &freetreesT)
    {
        t = new Tree;
    }
    else
    {
        t = freetreesH.next;
        Tree* n = t->next;

        freetreesH.next = n;
        n->prev = &freetreesH;
    }

    t->next = nullptr;
    t->prev = nullptr;

    addToActiveList(t);
#else
    t = new Tree;
#endif

    activetrees.insert((unsigned int)t);

    totaltrees++;

    return t;
}

/*------------------------------------------------------------------------
Module: treeDataFreeTree
Description:
------------------------------------------------------------------------*/
void treeDataFreeTree(Tree* t)
{
    if(!t) return;

#ifdef SPATIAL_DB
    SDB_RemoveTree(t);
#endif

    TreeSet::iterator itr = activetrees.find((unsigned int)t);

    if(itr != activetrees.end())
    {
#if USE_PREALLOCATED_TREES
        releaseActiveTree(t);
        addToFreeList(t);
#else
        delete t;
#endif
        activetrees.erase(itr);
        totaltrees--;
    }
}

/*------------------------------------------------------------------------
Module: treeDataIsValid
Description:
------------------------------------------------------------------------*/
bool treeDataIsValid(unsigned int address)
{
    TreeSet::iterator itr = activetrees.find(address);
    return itr != activetrees.end();
}

/*------------------------------------------------------------------------
Module: treeDataFreeTree
Description:
------------------------------------------------------------------------*/
void treeDataFreeTree(unsigned int address)
{
    Tree* t = (Tree*)address;
    treeDataFreeTree(t);
}

/*------------------------------------------------------------------------
Module: treeDataGetBaseSize
Description:
------------------------------------------------------------------------*/
unsigned int treeDataGetBaseSize()
{
    return basetrees.size();
}

/*------------------------------------------------------------------------
Module: treeDataGetTotalTrees
Description:
------------------------------------------------------------------------*/
int treeDataGetTotalTrees()
{
    return totaltrees;
}

/*------------------------------------------------------------------------
Module: treeDataKill
Description:
------------------------------------------------------------------------*/
void treeDataKill()
{
    for(unsigned int i = 0; i < treedata.size(); i++)
    {
        TreeData* td = treedata[i];
        delete td;
    }

    std::map<std::string, CTree*>::iterator itr = treecache.begin();

    for(; itr != treecache.end(); ++itr)
    {
        CTree* bt = itr->second;
        delete bt;
    }

    basetrees.clear();
    treedata.clear();
    billboardTexCoords.clear();
    treecache.clear();

    texCoordOffset = 0;
    totaltrees = 0;

    vbo = nullptr;

#if USE_PREALLOCATED_TREES
    Tree* n = activetreesH.next;

    while(n != &activetreesT)
    {
        Tree* t = n;
        n = n->next;
        delete t;
    }

    n = freetreesH.next;

    while(n != &freetreesT)
    {
        Tree* t = n;
        n = n->next;
        delete t;
    }
#endif
}
