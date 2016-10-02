#include <syscfg.h> // Path is project specific.

#include "mem/mem.h"
#include "include/vars.h"
#include "ob/obmain.h"
#include "ob/obpool.h"
#include "ob/obMgr.h"
#include "ld/ldflt.h"
#include "fio/fio.h"
#include "ld/ldpix.h"
#include "ob/obc.h"
#include "ld/ldmdl.h"
#include "ogl/oglMesh.h"
#include "trn/trncd.h"
#include "ob/obdraw.h"
#include "math/math3d.h"
#include "trn/trncloud.h"
#include "trn/trnsky.h"
#include "str/strpool.h"
#include "trn/trnop.h"
#include "trn/trngrass.h"
#include "ogl/oglMaterial.h"
#include "ld/ldtexture.h"
#include "ogl/oglVertexBuffer.h"
#include "ogl/oglTextureObject.h"
#include "osgImport/ShadowMap.h"
#include "sdm/sdmDefines.h"

#if USE_STATESET
#include "ogl/States.h"
#endif

#ifdef MODEL_INFO
#include "ob/obinfo.h"
#endif
#ifdef SUPPORT_WWO
#include "ld/ldwwo.h"
#endif

#include "shader/glslNormal.h"
#include "shader/glslWater.h"
#include "shader/glslSpecular.h"
#include "shader/glslCloth.h"
#include "tools/TangentSpaceGenerator.h"
#include "osgImport/BoundingBox.h"
#include "trn/trntypes.h"
#include "sdm/sdm.h"
#include "fio/fiod.h"

#include "math/Matrixf.h"
#include "physics/physicsmain.h"

static int defaultSpecMap = -1;
static int white = -1;

using namespace	ogl;
using namespace	ogl::VertexAttributes;

#define xCOLLECT_TEXTURE_STATS

/************************************************************************************************
	obMgr
*************************************************************************************************/

#define PLAYNET_COMPANY		('P' + ('N' << 8))		// short for Playnet
#define PLAYNET_OBJECTFILE	('O' + ('B' << 8))		// short for Object (File Type)
#define OBJECTFILE_MAJOR	7
#define OBJECTFILE_MINOR	2
#define OBJECTFILE_VERSION	((OBJECTFILE_MAJOR << 8) + OBJECTFILE_MINOR)	// version; MSB is major, LSB is minor
#define MDL_MAX_PATH 260

#if(0)
static void _CreateODEColliderBase(PoolModel* pm);
#endif

// No preload ==> texture creation during runtime. This contributes to stutters.
//#define NO_PRELOAD
#undef NO_PRELOAD

static bool AlphaColliderImage = false;

PoolMap _poolmap;

void obmgrKill()
{
    PoolMap::iterator itr = _poolmap.begin();

    for(; itr != _poolmap.end(); ++itr)
    {
        delete itr->second;
    }
}

/************************************************************************************************
	obmgrAddModelToPool
*************************************************************************************************/
PoolModel*  obmgrAddModelToPool(const char* path, LOGICAL forceModelLoading, UINT32 flags)
{
	PoolModel*	entry			= 0;
	std::string		shortName	= path;
	size_t		lastbslash		= shortName.rfind('\\');
    bool showinfo = false;


	if(lastbslash == std::string::npos)    lastbslash = shortName.rfind('/');
	if(lastbslash != shortName.npos) shortName.erase(0, lastbslash+1);
	size_t		lastDot			= shortName.rfind('.');
	if(lastDot != shortName.npos) shortName.erase(shortName.rfind('.'), std::string::npos);

	PoolMap::iterator it = _poolmap.find(shortName);
	if(it == _poolmap.end()) // NEEDS TO SUPPORT WWO
	{
		// If loading from the archive, check for '.mdl'. If not there
		// print error message to the log.
		if(fioQuery() == ISARCHIVE)
		{
			std::string str = path;
 			size_t ext = str.rfind('.');
			if(ext != std::string::npos) str.erase(ext);

			str = str + ".mdl";

			int fd = fioOpen(str.c_str());

			if(fd < 0)
			{
				Write_Log(LOG_OLD, LOG_INFO, "* %s missing from the archive *", str.c_str());
				return 0;
			}

			fioClose(fd);
		}
		else
		{
			int fd = fioOpen(path);
			if(fd < 0) return 0;
			fioClose(fd);
		}

		entry = new PoolModel(path, flags);
		std::string	str(path);

		size_t start = str.rfind('\\');
		size_t end = str.rfind('.');

		if(start == std::string::npos)
		{
			entry->directory = "";
			entry->modelname = str.substr(0,end);
		}
		else
		{
			entry->directory = str.substr(0, start+1);
			entry->modelname = str.substr(start+1,end-start-1);
		}

		PoolMap::iterator it = _poolmap.find(entry->modelname);

		if(it == _poolmap.end())
            _poolmap[entry->modelname] = entry;

        showinfo = true;
	}
	else
        entry = it->second;

	if(forceModelLoading || (!LowMemorySystem && !NoBlitzWorPreload))
	{
		if(!entry->node)
            entry->load(); // Comment out to stop object and texture preload

		if(!entry->node)
            return 0;

		for(unsigned int i=0; i < entry->subModelReferences.size(); i++)
		{
			if(!entry->subModelReferences[i].poolModel->node)
                entry->subModelReferences[i].poolModel->load();

            if(!entry->subModelReferences[i].poolModel->node)
                return 0;
		}
	}

#ifdef MODEL_INFO
    if ( showinfo ) // && (entry->modelname=="h39" || entry->modelname=="stonehut1c") )
    {
        getModelInfo( entry );
    }
#endif

	return entry;
}


//////////////////////////////////////////////////////////////////////
//	PoolModel::CreateInstance(const Matrixf&)
//////////////////////////////////////////////////////////////////////
ModelInstance* PoolModel::CreateInstance(const piMATRIX& transform)
{
	ModelInstance* modelInstance = new ModelInstance(this, transform);

	modelInstance->rootObject = node->createPoolInstance(modelInstance, transform);

	switch(modelType)
	{
		case ModelTypes::CLOUD:

			trnCloudRegisterCloudInstance(modelInstance->rootObject);
			break;
		case ModelTypes::VEHICLE:

			//vehmainInitModelInstance(modelInstance->rootObject);
			break;
		default:

			break;
	}

	return modelInstance;
}


/************************************************************************************************
	obmgrCreateModelInstance
*************************************************************************************************/
struct OBJECT* obmgrCreateModelInstance(const char* modelname)
{
	return obmgrCreateModelInstance(modelname, IdentityMatrix, false);
}

void obmgrHandleSpecialObjects(OBJECT* o)
{
    // Do not handle speed tree through here. This will add duplicate
    // trees in some instances.
#if(0)
	if(o->flag2 & OBJECT_SPEEDTREE)
	{
		obmainMoveToNotObjectTree(o);
		if(trnopHandleSpeedTree(o) != 0)	return;
	}
#endif

	#ifdef USE_NEW_GRASS
    if(o->flag2 & OBJECT_IS_GRASS)
    {
        trngrassAddGrass(o);
    }
	#endif

	if(o->child)	obmgrHandleSpecialObjects(o->child);
	if(o->sibling)	obmgrHandleSpecialObjects(o->sibling);
	if(o->attached)	obmgrHandleSpecialObjects(o->attached);
}

struct	OBJECT*	obmgrCreateModelInstance(const char* modelName, const piMATRIX& transform, bool handleSpecialObjects)
{
	PoolModel*	poolModel		= nullptr;
	OBJECT*		topInstanceNode	= nullptr;
	std::string	name(modelName);
	size_t		lastbslash		= name.rfind('\\');

    if(lastbslash == std::string::npos)     lastbslash = name.rfind('/');
	if(lastbslash != name.npos) name.erase(0, lastbslash+1);

	PoolMap::iterator it = _poolmap.find(name);
	if(it == _poolmap.end()) return 0;

	poolModel = it->second;

	if(poolModel->node == 0)	poolModel->load();
	if(poolModel->node == 0)	return 0;

    // HACK: DEV-787 ==>>
    piMATRIX matMove = transform;

    if (poolModel->modelType == ModelTypes::CLOUD && poolModel->node->name[0] == 'r')
    {
        matMove = IdentityMatrix;
        matMove._43 -= RAIN_CLOUD_HEIGHT_OFFSET;

        math3dMatrixMultiply(&matMove, &transform, &matMove);
    }
    // <<== HACK: DEV-787

	topInstanceNode = poolModel->CreateInstance(matMove)->rootObject;
	//if(handleSpecialObjects)	HandleSpecialObjects(topInstanceNode);

	return topInstanceNode;
}


/************************************************************************************************
	PoolNode
*************************************************************************************************/
PoolNode::PoolNode()
{
	parent = nullptr;
	sibling = nullptr;
	child = nullptr;
	attached = nullptr;

	memset(name, 0, sizeof(char)*32);
	flags = 0;
	flag2 = 0;
    flag3 = 0;
    vboid = -1;

	objdata = nullptr;
	vertices = nullptr;
	indices = nullptr;
	faceNormals = nullptr;
	collisionNormals = nullptr;
	dof = nullptr;
	comment = nullptr;
	texid = matid = -1;

	meshContext = nullptr;
    material = nullptr;
	collider = nullptr;
}

PoolNode::~PoolNode()
{
	if(collider)
		delete collider;

// This is a bit recursive, but we're not using too much stack space here
	if(sibling)
		delete sibling;
	if(attached)
		delete attached;
	if(child)
		delete child;

	if(objdata)
		memFree(objdata);
	if(vertices)
		memFree(vertices);
	if(indices)
		memFree(indices);
	if(faceNormals)
		memFree(faceNormals);
	if(collisionNormals)
		memFree(collisionNormals);
	if(dof)
		memFree(dof);

#if(0) //strpool
	if(comment)
		delete [] comment;
#endif

	if(meshContext)
	{
		delete meshContext;
		meshContext = nullptr;
	}

    material = nullptr;
}

static int imageLoader(const char* Filename, UINT32 TransparentColor, UINT32 Flags)
{
    return ldpixLoadImageFileToSurface( Filename, TransparentColor, Flags );
}

#define LOAD_IMAGE_TO_SURFACE(path, color, flag) imageLoader(path, color, flag)

#if USE_STATESET
/************************************************************************************************
	load
*************************************************************************************************/
void PoolNode::load(PoolModel* model, int fd)
{
	PoolNode* node;
	int attachedOffset, siblingOffset, childOffset;

	{
		int size = fioGetl(fd);
		fioRead(fd, name,32);
		flags 	= fioGetl(fd);
		flag2 	= fioGetl(fd);
        flag3   = fioGetl(fd);
		attachedOffset = fioGetl(fd);
		siblingOffset	= fioGetl(fd);
		childOffset		= fioGetl(fd);
		int objdataOffset	= fioGetl(fd);
		int dofOffset		= fioGetl(fd);
		int commentOffset	= fioGetl(fd);
		int spareOffset		= fioGetl(fd);

        if(flag2 & OBJECT_SPEEDTREE)
        {
            model->hastrees = true;
        }

		if(objdataOffset)
		{
			objdata = new POOLOBJDATA;
			objdata->feature = fioGets(fd);
			objdata->layer = fioGets(fd);
			objdata->level = fioGets(fd);
			objdata->reserved = fioGets(fd);
			fioReadf(fd, (float*)&objdata->loc, 3);
			fioReadf(fd, (float*)&objdata->pos, 3);
			fioRead(fd, objdata->matname, 32);
			fioRead(fd, objdata->texname, 32);
			objdata->minrad = fioGetf(fd);
			objdata->maxrad = fioGetf(fd);
			objdata->minlod = fioGetf(fd);
			objdata->maxlod = fioGetf(fd);
			fioReadf(fd, (float*)&objdata->obox, 12);
			objdata->numverts = fioGets(fd);
			objdata->numtris = fioGets(fd);
			objdata->puverts = fioGetl(fd);
			objdata->tangentData = fioGetl(fd);
			objdata->ptris = fioGetl(fd);
			objdata->psnorms = fioGetl(fd);
			objdata->pscdn = fioGetl(fd);

			// instead of -1 to indicate load never attempted
			matid = -2;
			texid = -2;

			if(objdata->numverts)
			{
				vertices = new piUNIFIED_VERTEX[objdata->numverts];
                fioRead(fd, vertices, sizeof(piUNIFIED_VERTEX) * objdata->numverts);

                if(!material.valid())
                {
                    if(model->modelType == ModelTypes::CLOUD)
                    {
                        // Hack for clouds, since there are a billion cloud objects. Bad for rendering
                        static std::map<std::string, RenderStateSet*> cloudmaterials;
                        std::map<std::string, RenderStateSet*>::iterator itr = cloudmaterials.find(objdata->texname);

                        if(itr == cloudmaterials.end())
                        {
                            material = new RenderStateSet;
                            material->SetRenderState(new DepthMask(false));

                            material->SetMode(GL_BLEND, 1);

                            material->SetIsPool(true);

                            cloudmaterials[objdata->texname] = material.get();
                        }
                        else
                        {
                            material = itr->second;
                        }

                        if(!ogl::UseDeprecatedPath())
                        {
                            material->SetRenderState(new Program("cloud"));
                        }
                    }
                    else
                    {
                        material = new RenderStateSet;
                        material->SetIsPool(true);

                        if(objdata->level > 0)
                        {
                            material->SetRenderState(new AlphaFunc(GL_GREATER, objdata->level * colorONEOVER255));
                            material->SetMode(GL_ALPHA_TEST, 1);
                        }
                    }
                }

                if(objdata->tangentData)
                {
                    fioSeek(fd, sizeof(piQUAT) * objdata->numverts, SEEK_CUR);
                }
			}

			if(objdata->numtris)
			{
				indices = new piINDICES[objdata->numtris];
				fioReads(fd, (SINT16*)indices, 3*objdata->numtris);

				if(objdata->psnorms)
				{
					faceNormals = new piVECTOR[objdata->numtris];
					fioReadf(fd, (float*)faceNormals, 3*objdata->numtris);
				}

				if(objdata->pscdn)
				{
					collisionNormals = new piVECTOR[3 * objdata->numtris];
					fioReadf(fd, (float*)collisionNormals, 9 * objdata->numtris);
				}
			}

			if(objdata->numverts && !(flag2 & OBJECT_HIDDEN))
			{
				meshContext = new ogl::MeshContext(fd, model->staticBuffers);

                if(!ogl::UseDeprecatedPath())
                {
                    if(model->modelname.find("_vcpit") != std::string::npos)
                    {
                        if(*objdata->texname)
                            material->SetRenderState(new Program("cockpittexture"));
                        else
                            material->SetRenderState(new Program("cockpit"));
                    }
                }
            }
		}

		if(dofOffset)
		{
			// let's see how many times we have to type dof
			dof = new DOF;
			fioReadf(fd, (float*)&dof->doforigin,3);
			fioReadf(fd, (float*)&dof->dofaxis,3);
			fioReadf(fd, (float*)&dof->dofplane,3);
			fioReadf(fd, (float*)&dof->dofmintrans,3);
			fioReadf(fd, (float*)&dof->dofmaxtrans,3);
			fioReadf(fd, (float*)&dof->dofcurtrans,3);
			fioReadf(fd, (float*)&dof->dofinctrans,3);
			fioReadf(fd, (float*)&dof->dofminrotate,3);
			fioReadf(fd, (float*)&dof->dofmaxrotate,3);
			fioReadf(fd, (float*)&dof->dofcurrotate,3);
			fioReadf(fd, (float*)&dof->dofincrotate,3);
			fioReadf(fd, (float*)&dof->dofminscale,3);
			fioReadf(fd, (float*)&dof->dofmaxscale,3);
			fioReadf(fd, (float*)&dof->dofcurscale,3);
			fioReadf(fd, (float*)&dof->dofincscale,3);

			dof->dofvarvalue = fioGetf(fd);
			dof->dofvarnum = fioGets(fd);
			dof->dofvarsubnum = fioGets(fd);
			dof->dofmaxval = fioGetf(fd);
			dof->dofneutralval = fioGetf(fd);
			dof->dofminval = fioGetf(fd);
			fioReadf(fd, (float*)&dof->dofmatrix1, 16);
			fioReadf(fd, (float*)&dof->dofmatrix2, 16);
			fioReadf(fd, (float*)&dof->dofposmatrix, 16);
			dof->dofflags = fioGetl(fd);
		}

		if(commentOffset)
		{
			int end = spareOffset ? spareOffset : size;
			int len = end - commentOffset;
            char buff[128];
			fioRead(fd, buff, len);
			comment = (char*)strpool(buff);
		}

		if(spareOffset)
		{
			fioSeek(fd, size - spareOffset, SEEK_CUR);
		}

		if(objdata && *objdata->texname)
		{
            if(defaultSpecMap == -1)
                defaultSpecMap = ldTextureLoad("default_spec.dds", 0);

            if(white == -1)
                white = ldTextureLoad("white.dds", 0);

			SINT32 id = -1;
			std::string path = model->directory + objdata->texname;

            unsigned int ldpixflag = LDPIX_MIPMAP;

            if(AlphaColliderImage)
                ldpixflag |= LDPIX_CREATE_SURFACE_DATA;

            id = LOAD_IMAGE_TO_SURFACE(path.c_str(), colorBLACK, ldpixflag);

			if(!piIsTexture(id, 0))
			{
				Write_Log(LOG_OLD, LOG_WARN, "[obpoolqueueHandleTexture] %s** FAILED**", path.c_str());
			}

            // Ticket #4627
            if(AlphaColliderImage)
            {
                // Special case where we have to collide with this alpha texture
                if(path.find("ftank1") != std::string::npos)
                {
                    TEXTURE* t = oglGetTextureData(id);

                    if(t)
                    {
                        t->hasalpha = 0;
                    }
                }
            }

            if (flag3 & OBJECT_IS_ROAD )
            {
                model->hasRoads = true;
            }

            material->SetRenderState(new TextureUnit(0, id));

			texid = id;

            // Ticket #4789 Multitextured objects can't be normal/spec mapped as of now.
            int count = obdrawGetMultiTextureCount(texid);

            // Set shaders
            if(count > 0 && (model->modelType == ModelTypes::TERRAIN_TILE || model->modelType == ModelTypes::TERRAIN_OBJECT))
            {
                extern Texture2D* obdrawGetSurfaceTexture();

                material->SetRenderState(new Program("multitextureterrain"));

                // Need dummy texture in case there is no second texture
                material->SetRenderState(new TextureUnit(2, white));
                material->SetRenderState(new TextureUnit(3, obdrawGetSurfaceTexture())); // Surface effects
            }
            else if(count > 1)
            {
                material->SetRenderState(new Program("multitexture3"));
            }
            else if(count > 0)
            {
                material->SetRenderState(new Program("multitexture2"));
            }

            // Set texture params
            if(count > 0)
            {
                flag2 &= ~OBJECT_NORMALMAP;
                flag3 &= ~OBJECT_SPECULARMAP;

                int multiid = texid;
                int offset = 0;

                material->SetMode(GL_LIGHTING, 0);

                obdrawFindMultiTextureById(multiid, &offset);
                multiid = obdrawGetMultiTextureID(offset);
                material->SetRenderState(new TextureUnit(1, multiid));

                Matrixf m;
                obdrawGetMultiTextureMatrix(offset, m);

                material->SetRenderState(new TextureMatrix(1, m));

                TexEnv* te = new TexEnv(1);

                te->SetMode(GL_COMBINE);
                te->SetCombineRGB(GL_MODULATE);
                te->SetSource0RGB(GL_TEXTURE);
                te->SetSource1RGB(GL_PREVIOUS);
                te->SetOperand0RGB(GL_SRC_COLOR);
                te->SetOperand1RGB(GL_SRC_COLOR);
                te->SetCombineAlpha(GL_MODULATE);
                te->SetSource0Alpha(GL_TEXTURE);
                te->SetSource1Alpha(GL_PREVIOUS);
                te->SetOperand0Alpha(GL_SRC_ALPHA);
                te->SetOperand1Alpha(GL_SRC_ALPHA);

                material->SetRenderState(te);

                if(count > 1)
                {
                    obdrawFindMultiTextureById(multiid, &offset);
                    multiid = obdrawGetMultiTextureID(offset);
                    material->SetRenderState(new TextureUnit(2, multiid));

                    Matrixf m;
                    obdrawGetMultiTextureMatrix(offset, m);

                    material->SetRenderState(new TextureMatrix(2, m));

                    TexEnv* te = new TexEnv(2);

                    te->SetMode(GL_COMBINE);
                    te->SetCombineRGB(GL_MODULATE);
                    te->SetSource0RGB(GL_TEXTURE);
                    te->SetSource1RGB(GL_PREVIOUS);
                    te->SetOperand0RGB(GL_SRC_COLOR);
                    te->SetOperand1RGB(GL_SRC_COLOR);
                    te->SetCombineAlpha(GL_MODULATE);
                    te->SetSource0Alpha(GL_TEXTURE);
                    te->SetSource1Alpha(GL_PREVIOUS);
                    te->SetOperand0Alpha(GL_SRC_ALPHA);
                    te->SetOperand1Alpha(GL_SRC_ALPHA);

                    material->SetRenderState(te);
                }
            }
            else
            {
                if(!ogl::UseDeprecatedPath())
                {
                    if(flag2 & OBJECT_HASALPHA && !(flags & OBJECT_NOFILTER))
                    {
                        if(texid < 0)
                            material->SetRenderState(new Program("transparent"));
                        else
                            material->SetRenderState(new Program("transparenttexture"));

                        material->SetMode(GL_BLEND, 1);
                        material->SetMode(GL_LIGHTING, 0);
                        material->SetRenderState(new DepthMask(false));
                    }
                    else
                    {
                        if(texid < 0)
                            material->SetRenderState(new Program("draw"));
                        else
                            material->SetRenderState(new Program("drawtexture"));

                        oglTextureObject* tbo = oglTextureObject::GetTextureObject("dynamicglobal");
                        material->SetRenderState(new TextureUnit(1, GL_TEXTURE_BUFFER, tbo->GetTextureObjectName()));
                    }
                }
                else
                {
                    if(flag2 & OBJECT_HASALPHA && !(flags & OBJECT_NOFILTER))
                    {
                        material->SetMode(GL_BLEND, 1);
                        material->SetRenderState(new DepthMask(false));
                    }
                }
            }

            if(flag2 & OBJECT_NORMALMAP)
            {
                material->SetRenderState(new TextureUnit(2, defaultSpecMap));

                std::string str = path;
                int pos = str.rfind('.');

                if(pos != std::string::npos)
                {
                    str.erase(pos, str.length()-pos);
                }

                str += "_normal";

		        id = LOAD_IMAGE_TO_SURFACE(str.c_str(), colorBLACK, LDPIX_MIPMAP);

                if(piIsTexture(id, 0))
                {
                    if(piIsTexture(texid, 0))
                    {
                        material->SetRenderState(new TextureUnit(1, id));
                    }
                }
            }

            if(flag3 & OBJECT_SPECULARMAP)
            {
                std::string str = path;
                int pos = str.rfind('.');

                if(pos != std::string::npos)
                {
                    str.erase(pos, str.length()-pos);
                }

                str += "_spec";

		        id = LOAD_IMAGE_TO_SURFACE(str.c_str(), colorBLACK, LDPIX_MIPMAP);

                if(piIsTexture(id, 0))
                {
                    if(piIsTexture(texid, 0))
                    {
                        // Normal maps may have a spec map too.
                        if(flag2 & OBJECT_NORMALMAP)
                            material->SetRenderState(new TextureUnit(2, id));
                        else
                            material->SetRenderState(new TextureUnit(1, id));
                    }
                }
            }

            if(flag2 & OBJECT_NORMALMAP)
            {
                material->SetRenderState(new Program("normalmap"));

                if(!ogl::UseDeprecatedPath())
                {
                    oglTextureObject* tbo = oglTextureObject::GetTextureObject("dynamicglobal");
                    material->SetRenderState(new TextureUnit(3, GL_TEXTURE_BUFFER, tbo->GetTextureObjectName()));
                }
            }
            else if(flag3 & OBJECT_SPECULARMAP)
            {
                material->SetRenderState(new Program("specularmap"));

                if(!ogl::UseDeprecatedPath())
                {
                    oglTextureObject* tbo = oglTextureObject::GetTextureObject("dynamicglobal");
                    material->SetRenderState(new TextureUnit(2, GL_TEXTURE_BUFFER, tbo->GetTextureObjectName()));
                }
            }
            else if(flag2 & OBJECT_FLAG)
            {
                material->SetRenderState(new Program("flagprogram"));
            }
		}

        if(material.valid())
            material->SetRenderState(new Material);

		if(objdata && *objdata->matname)
		{
			matid = piFindMaterialName(objdata->matname);

            Material* m = dynamic_cast<Material*>(material->GetRenderState(RS_MATERIAL));
            m->SetMaterialID(matid);

            // Interior objects don't get spec or normal mapped
            if(flag2 & OBJECT_NORMALMAP | flag3 & OBJECT_SPECULARMAP)
            {
                if(piIsMaterialInterior(matid))
                {
                    material->RemoveRenderState(RS_GLSLPROGRAM);
                    material->RemoveRenderState(RS_TEXTURE_UNIT2);
                    material->RemoveRenderState(RS_TEXTURE_UNIT1);

                    flag2 &= ~OBJECT_NORMALMAP;
                    flag3 &= ~OBJECT_SPECULARMAP;
                }
            }

            if(!ogl::UseDeprecatedPath())
            {
                if(flag2 & OBJECT_HASALPHA && !(flags & OBJECT_NOFILTER))
                {
                    material->SetRenderState(new Program("transparent"));

                    material->SetMode(GL_BLEND, 1);
                    material->SetMode(GL_LIGHTING, 0);
                    material->SetRenderState(new DepthMask(false));
                }
                else
                {
                    material->SetRenderState(new Program("draw"));

                    oglTextureObject* tbo = oglTextureObject::GetTextureObject("dynamicglobal");
                    material->SetRenderState(new TextureUnit(1, GL_TEXTURE_BUFFER, tbo->GetTextureObjectName()));
                }
            }
            else
            {
                if(flag2 & OBJECT_HASALPHA && !(flags & OBJECT_NOFILTER))
                {
                    material->SetMode(GL_BLEND, 1);
                    material->SetRenderState(new DepthMask(false));
                }
            }
		}
	}
// This is a bit recursive, but I don't think too much stack space is used in this function,
// especially in optimized builds. If it becomes a problem we can always rewrite, it'd
// still be the same total number of function calls, just less stack space used.
	if(siblingOffset)
	{
		node = new PoolNode;
		node->parent = parent;

		sibling = node;
		node->load(model, fd);
	}

	if(attachedOffset)
	{
		node = new PoolNode;
		node->parent = (parent->attached == this) ? parent : this;

		attached = node;
		node->load(model, fd);
	}

	if(childOffset)
	{
		node = new PoolNode;
		node->parent = this;

		child = node;
		node->load(model, fd);
	}
}
#else
/************************************************************************************************
	load
*************************************************************************************************/
void PoolNode::load(PoolModel* model, int fd)
{
	PoolNode* node;
	int attachedOffset, siblingOffset, childOffset;

	{
		int size = fioGetl(fd);
		fioRead(fd, name,32);
		flags 	= fioGetl(fd);
		flag2 	= fioGetl(fd);
        flag3   = fioGetl(fd);
		attachedOffset = fioGetl(fd);
		siblingOffset	= fioGetl(fd);
		childOffset		= fioGetl(fd);
		int objdataOffset	= fioGetl(fd);
		int dofOffset		= fioGetl(fd);
		int commentOffset	= fioGetl(fd);
		int spareOffset		= fioGetl(fd);

        if(flag2 & OBJECT_SPEEDTREE)
        {
            model->hastrees = true;
        }

		if(objdataOffset)
		{
			objdata = (POOLOBJDATA*)memAlloc(sizeof(POOLOBJDATA));
			objdata->feature = fioGets(fd);
			objdata->layer = fioGets(fd);
			objdata->level = fioGets(fd);
			objdata->reserved = fioGets(fd);
			fioReadf(fd, (float*)&objdata->loc, 3);
			fioReadf(fd, (float*)&objdata->pos, 3);
			fioRead(fd, objdata->matname, 32);
			fioRead(fd, objdata->texname, 32);
			objdata->minrad = fioGetf(fd);
			objdata->maxrad = fioGetf(fd);
			objdata->minlod = fioGetf(fd);
			objdata->maxlod = fioGetf(fd);
			fioReadf(fd, (float*)&objdata->obox, 12);
			objdata->numverts = fioGets(fd);
			objdata->numtris = fioGets(fd);
			objdata->puverts = fioGetl(fd);
			objdata->tangentData = fioGetl(fd);
			objdata->ptris = fioGetl(fd);
			objdata->psnorms = fioGetl(fd);
			objdata->pscdn = fioGetl(fd);

			// instead of -1 to indicate load never attempted
			matid = -2;
			texid = -2;

			if(objdata->numverts)
			{
				vertices = (piUNIFIED_VERTEX*)memAlloc(sizeof(piUNIFIED_VERTEX) * objdata->numverts);
                fioRead(fd, vertices, sizeof(piUNIFIED_VERTEX) * objdata->numverts);

                if(!material.valid())
                {
                    if(model->modelType == ModelTypes::CLOUD)
                    {
                        // Hack for clouds, since there are a billion cloud objects. Bad for rendering
                        static std::map<std::string, oglMaterial*> cloudmaterials;
                        std::map<std::string, oglMaterial*>::iterator itr = cloudmaterials.find(objdata->texname);

                        if(itr == cloudmaterials.end())
                        {
                            material = new oglMaterial;
                            material->SetIsPoolMaterial(true);

                            cloudmaterials[objdata->texname] = material.get();
                        }
                        else
                        {
                            material = itr->second;
                        }

                        if(!ogl::UseDeprecatedPath())
                            material->SetShader("cloud");
                    }
                    else
                    {
                        material = new oglMaterial;
                        material->SetIsPoolMaterial(true);
                    }
                }

                if(objdata->tangentData)
                {
                    fioSeek(fd, sizeof(piQUAT) * objdata->numverts, SEEK_CUR);
                }
			}

			if(objdata->numtris)
			{
                indices = (piINDICES*)memAlloc(sizeof(piINDICES) * objdata->numtris);
				fioReads(fd, (SINT16*)indices, 3*objdata->numtris);

				if(objdata->psnorms)
				{
                    faceNormals = (piVECTOR*)memAlloc(sizeof(piVECTOR) * objdata->numtris);
					fioReadf(fd, (float*)faceNormals, 3*objdata->numtris);
				}

				if(objdata->pscdn)
				{
                    collisionNormals = (piVECTOR*)memAlloc(sizeof(piVECTOR) * 3 * objdata->numtris);
					fioReadf(fd, (float*)collisionNormals, 9 * objdata->numtris);
				}
			}

			if(objdata->numverts && !(flag2 & OBJECT_HIDDEN))
			{
				meshContext = new ogl::MeshContext(fd, model->staticBuffers);

                if(!ogl::UseDeprecatedPath())
                {
                    if(model->modelname.find("_vcpit") != std::string::npos)
                    {
                        if(*objdata->texname)
                            material->SetShader("cockpittexture");
                        else
                            material->SetShader("cockpit");
                    }
                }
            }
		}

		if(dofOffset)
		{
			// let's see how many times we have to type dof
			dof = (DOF*)memAlloc(sizeof(DOF));
			fioReadf(fd, (float*)&dof->doforigin,3);
			fioReadf(fd, (float*)&dof->dofaxis,3);
			fioReadf(fd, (float*)&dof->dofplane,3);
			fioReadf(fd, (float*)&dof->dofmintrans,3);
			fioReadf(fd, (float*)&dof->dofmaxtrans,3);
			fioReadf(fd, (float*)&dof->dofcurtrans,3);
			fioReadf(fd, (float*)&dof->dofinctrans,3);
			fioReadf(fd, (float*)&dof->dofminrotate,3);
			fioReadf(fd, (float*)&dof->dofmaxrotate,3);
			fioReadf(fd, (float*)&dof->dofcurrotate,3);
			fioReadf(fd, (float*)&dof->dofincrotate,3);
			fioReadf(fd, (float*)&dof->dofminscale,3);
			fioReadf(fd, (float*)&dof->dofmaxscale,3);
			fioReadf(fd, (float*)&dof->dofcurscale,3);
			fioReadf(fd, (float*)&dof->dofincscale,3);

			dof->dofvarvalue = fioGetf(fd);
			dof->dofvarnum = fioGets(fd);
			dof->dofvarsubnum = fioGets(fd);
			dof->dofmaxval = fioGetf(fd);
			dof->dofneutralval = fioGetf(fd);
			dof->dofminval = fioGetf(fd);
			fioReadf(fd, (float*)&dof->dofmatrix1, 16);
			fioReadf(fd, (float*)&dof->dofmatrix2, 16);
			fioReadf(fd, (float*)&dof->dofposmatrix, 16);
			dof->dofflags = fioGetl(fd);
		}

		if(commentOffset)
		{
			int end = spareOffset ? spareOffset : size;
			int len = end - commentOffset;
            char buff[128];
			fioRead(fd, buff, len);
			comment = (char*)strpool(buff);
		}

		if(spareOffset)
		{
			fioSeek(fd, size - spareOffset, SEEK_CUR);
		}

		if(objdata && *objdata->texname)
		{
            if(defaultSpecMap == -1)
                defaultSpecMap = ldTextureLoad("default_spec.dds", 0);

            if(white == -1)
                white = ldTextureLoad("white.dds", 0);

			SINT32 id = -1;
			std::string path = model->directory + objdata->texname;

            unsigned int ldpixflag = LDPIX_MIPMAP;

            if(AlphaColliderImage)
                ldpixflag |= LDPIX_CREATE_SURFACE_DATA;

            id = LOAD_IMAGE_TO_SURFACE(path.c_str(), colorBLACK, ldpixflag);

			if(!piIsTexture(id, 0))
			{
				Write_Log(LOG_OLD, LOG_WARN, "[obpoolqueueHandleTexture] %s** FAILED**", path.c_str());
			}

            // Ticket #4627
            if(AlphaColliderImage)
            {
                // Special case where we have to collide with this alpha texture
                if(path.find("ftank1") != std::string::npos)
                {
                    TEXTURE* t = oglGetTextureData(id);

                    if(t)
                    {
                        t->hasalpha = 0;
                    }
                }
            }

            if (flag3 & OBJECT_IS_ROAD )
            {
                model->hasRoads = true;
            }

            material->AddTexture(0, id);

			texid = id;

            // Ticket #4789 Multitextured objects can't be normal/spec mapped as of now.
            bool ismultitexture = obdrawGetMultiTextureCount(texid) > 0 ? true : false;

            if(ismultitexture)
            {
                flag2 &= ~OBJECT_NORMALMAP;
                flag3 &= ~OBJECT_SPECULARMAP;
            }

            if(flag2 & OBJECT_NORMALMAP)
            {
                material->AddTexture(2, defaultSpecMap);

                std::string str = path;
                int pos = str.rfind('.');

                if(pos != std::string::npos)
                {
                    str.erase(pos, str.length()-pos);
                }

                str += "_normal";

		        id = LOAD_IMAGE_TO_SURFACE(str.c_str(), colorBLACK, LDPIX_MIPMAP);

                if(piIsTexture(id, 0))
                {
                    if(piIsTexture(texid, 0))
                    {
                        material->AddTexture(1, id);
                    }
                }
            }

            if(flag3 & OBJECT_SPECULARMAP)
            {
                std::string str = path;
                int pos = str.rfind('.');

                if(pos != std::string::npos)
                {
                    str.erase(pos, str.length()-pos);
                }

                str += "_spec";

		        id = LOAD_IMAGE_TO_SURFACE(str.c_str(), colorBLACK, LDPIX_MIPMAP);

                if(piIsTexture(id, 0))
                {
                    if(piIsTexture(texid, 0))
                    {
                        // Normal maps may have a spec map too.
                        if(flag2 & OBJECT_NORMALMAP)
                            material->AddTexture(2, id);
                        else
                            material->AddTexture(1, id);
                    }
                }
            }

            if(flag2 & OBJECT_NORMALMAP)
            {
                material->SetShader("normalmap");

                if(!ogl::UseDeprecatedPath())
                {
                    oglTextureObject* tbo = oglTextureObject::GetTextureObject("dynamicglobal");
                    material->AddTexture(3, GL_TEXTURE_BUFFER, tbo->GetTextureObjectName());
                }
            }
            else if(flag3 & OBJECT_SPECULARMAP)
            {
                material->SetShader("specularmap");

                if(!ogl::UseDeprecatedPath())
                {
                    oglTextureObject* tbo = oglTextureObject::GetTextureObject("dynamicglobal");
                    material->AddTexture(2, GL_TEXTURE_BUFFER, tbo->GetTextureObjectName());
                }
            }
            else if(flag2 & OBJECT_FLAG)
            {
                material->SetShader("flagprogram");
            }
		}

		if(objdata && *objdata->matname)
		{
			matid = piFindMaterialName(objdata->matname);
     // printf("mat=%s\n",objdata->matname);
            material->SetMaterial(matid);

            // Interior objects don't get spec or normal mapped
            if((flag2 & OBJECT_NORMALMAP) || (flag3 & OBJECT_SPECULARMAP))
            {
                if(piIsMaterialInterior(matid))
                {
                    material->TextureResize(1);
                    material->SetShader("");

                    flag2 &= ~OBJECT_NORMALMAP;
                    flag3 &= ~OBJECT_SPECULARMAP;
                }
            }
		}
	}
// This is a bit recursive, but I don't think too much stack space is used in this function,
// especially in optimized builds. If it becomes a problem we can always rewrite, it'd
// still be the same total number of function calls, just less stack space used.
	if(siblingOffset)
	{
		node = new PoolNode;
		node->parent = parent;

		sibling = node;
		node->load(model, fd);
	}

	if(attachedOffset)
	{
		node = new PoolNode;
		node->parent = (parent->attached == this) ? parent : this;

		attached = node;
		node->load(model, fd);
	}

	if(childOffset)
	{
		node = new PoolNode;
		node->parent = this;

		child = node;
		node->load(model, fd);
	}
}
#endif

#undef LOAD_IMAGE_TO_SURFACE

#if USE_STATESET
/************************************************************************************************
	createInstance
*************************************************************************************************/
OBJECT* PoolNode::createPoolInstance(ModelInstance* modelInstance, const piMATRIX& transform)
{
	OBJECT* lpobj = obmainGetFreeObject();

    lpobj->modelInstance	= modelInstance;
    lpobj->modelNode			= this;
    lpobj->matid = -1;
    lpobj->texid = -1;
    lpobj->texname = "";
    lpobj->matname = "";
    lpobj->mat = transform;

    memcpy(lpobj->name, name, 32);
    lpobj->flags	= flags;
    lpobj->flag2	= flag2;
    lpobj->flag3    = flag3;

    if(!(flags & OBJECT_NOTOBJECT))
    {
        lpobj->feature	= objdata->feature;
        lpobj->layer	= objdata->layer;
        lpobj->level	= objdata->level;
        lpobj->loc		= objdata->loc;
        lpobj->pos		= objdata->pos;
        lpobj->matname 	= objdata->matname;
        lpobj->texname	= objdata->texname;
        lpobj->minrad	= objdata->minrad;
        lpobj->maxrad	= objdata->maxrad;
        lpobj->minlod	= objdata->minlod;
        lpobj->maxlod	= objdata->maxlod;
        memcpy(lpobj->obox, objdata->obox, 2 * sizeof(piVECTOR));
        memcpy(lpobj->bbox, objdata->bbox, 2 * sizeof(piVECTOR));

        lpobj->material = this->material.get();

        lpobj->texid = texid > -1 ? texid : -1;
        lpobj->matid = matid > -1 ? matid : -1;

        if(texid > -1)
            piAddTexRef(texid);

        // vertices
        if (objdata->numverts != 0)
        {
            lpobj->numverts = objdata->numverts;
            lpobj->puverts = vertices;

            if(flag3 & OBJECT_COAST_WATER)
            {
                glslCoastSetup(lpobj);
            }

            if(flag3 & OBJECT_RIVER_WATER)
            {
                glslRiverSetup(lpobj);
            }
        }

        if(lpobj->flag2 & OBJECT_SPEEDTREE)
        {
            // Remove from standard rendering system
            obmainMoveToNotObjectTree(lpobj);
        }

        // indices, normals
        if (objdata->numtris != 0)
        {
            lpobj->numtris = objdata->numtris;
            lpobj->ptris = indices;
            lpobj->psnorig = lpobj->psnorms = faceNormals;
            lpobj->pscdon = lpobj->pscdn = collisionNormals;
        }

        // The completely retarded multitexture code doesn't worth anything more than a ugly hack!
        // Only take up to two multitextures
        UINT32 count = obdrawGetMultiTextureCount(texid);

        if(count >= 1)
        {
            if(meshContext)
                meshContext->AddVertexAttribute(nullptr, TEX1);
        }

        if(count >= 2)
        {
            if(meshContext)
                meshContext->AddVertexAttribute(nullptr, TEX2);
        }

        if(meshContext)
        {
            meshContext->Apply();
            lpobj->meshContext = new ogl::MeshContext(*meshContext);
        }
    }

    // dof <-- WTF kind of comment is this?
    if(flags & OBJECT_DOF)
    {
        lpobj->dof = (OBJECTDOF*)memAlloc(sizeof(OBJECTDOF));
        lpobj->dof->dofstatic = (OBJECTSTATICDOF*)dof;
        memcpy(&lpobj->dof->dofvarvalue, &dof->dofvarvalue, sizeof(OBJECTDOF) - sizeof(OBJECTSTATICDOF*));
        lpobj->dof->dofflags |= DOF_FREE_MEMORY;
    }

    // comment <-- WTF kind of comment is this?
    lpobj->comment = comment;

    if(lpobj->flag2 & OBJECT_NORMALMAP)
    {
        glslNormalSetup(lpobj);
    }
    else if(lpobj->flag3 & OBJECT_SPECULARMAP)
    {
        glslSpecularSetup(lpobj);
    }
    else if(lpobj->flag2 & OBJECT_FLAG)
    {
        glslClothFlagSetup(lpobj);
    }
    else if(lpobj->flag3 & OBJECT_IS_ROAD)
    {
    }

	if(lpobj->prev == nullptr && lpobj->next == nullptr)
	{
		if(lpobj->flags & OBJECT_NOTOBJECT)
			obmainMoveToNotObjectTree(lpobj);
		else
			obmainMoveToObjectTree(lpobj);
	}

	// recursion away!
	if(child)		lpobj->child = child->createPoolInstance(modelInstance, transform);
	if(attached)	lpobj->attached = attached->createPoolInstance(modelInstance, transform);
	if(sibling)		lpobj->sibling = sibling->createPoolInstance(modelInstance, transform);

	return lpobj;
}
#else
/************************************************************************************************
	createInstance
*************************************************************************************************/
OBJECT* PoolNode::createPoolInstance(ModelInstance* modelInstance, const piMATRIX& transform)
{
	OBJECT* lpobj = obmainGetFreeObject();

    lpobj->modelInstance	= modelInstance;
    lpobj->modelNode			= this;
    lpobj->matid = -1;
    lpobj->texid = -1;
    lpobj->texname = "";
    lpobj->matname = "";
    lpobj->mat = transform;

    memcpy(lpobj->name, name, 32);
    lpobj->flags	= flags;
    lpobj->flag2	= flag2;
    lpobj->flag3    = flag3;

    if(!(flags & OBJECT_NOTOBJECT))
    {
        lpobj->feature	= objdata->feature;
        lpobj->layer	= objdata->layer;
        lpobj->level	= objdata->level;
        lpobj->loc		= objdata->loc;
        lpobj->pos		= objdata->pos;
        lpobj->matname 	= objdata->matname;
        lpobj->texname	= objdata->texname;
        lpobj->minrad	= objdata->minrad;
        lpobj->maxrad	= objdata->maxrad;
        lpobj->minlod	= objdata->minlod;
        lpobj->maxlod	= objdata->maxlod;
        memcpy(lpobj->obox, objdata->obox, 2 * sizeof(piVECTOR));
        memcpy(lpobj->bbox, objdata->bbox, 2 * sizeof(piVECTOR));

        lpobj->material = this->material.get();

        lpobj->texid = texid > -1 ? texid : -1;
        lpobj->matid = matid > -1 ? matid : -1;

        if(texid > -1)
            piAddTexRef(texid);

        // vertices
        if (objdata->numverts != 0)
        {
            lpobj->numverts = objdata->numverts;
            lpobj->puverts = vertices;

            if(flag3 & OBJECT_COAST_WATER)
            {
                glslCoastSetup(lpobj);
            }

            if(flag3 & OBJECT_RIVER_WATER)
            {
                glslRiverSetup(lpobj);
            }
        }

        if(lpobj->flag2 & OBJECT_SPEEDTREE)
        {
            // Remove from standard rendering system
            obmainMoveToNotObjectTree(lpobj);
        }

        // indices, normals
        if (objdata->numtris != 0)
        {
            lpobj->numtris = objdata->numtris;
            lpobj->ptris = indices;
            lpobj->psnorig = lpobj->psnorms = faceNormals;
            lpobj->pscdon = lpobj->pscdn = collisionNormals;
        }

        // The completely retarded multitexture code doesn't worth anything more than a ugly hack!
        // Only take up to two multitextures
        UINT32 count = obdrawGetMultiTextureCount(texid);

        if(!ogl::UseDeprecatedPath())
        {
            if(count == 0)
            {
                if(!material->HasShader())
                {
                    if(lpobj->flag2 & OBJECT_HASALPHA && !(lpobj->flags & OBJECT_NOFILTER))
                    {
                        if(lpobj->texid < 0)
                            material->SetShader("transparent");
                        else
                            material->SetShader("transparenttexture");
                    }
                    else
                    {
                        if(lpobj->texid < 0)
                            material->SetShader("draw");
                        else
                            material->SetShader("drawtexture");

                        oglTextureObject* tbo = oglTextureObject::GetTextureObject("dynamicglobal");
                        material->AddTexture(1, GL_TEXTURE_BUFFER, tbo->GetTextureObjectName());
                    }
                }
            }
        }

        if(count > 0 && (modelInstance->GetModelType() == ModelTypes::TERRAIN_TILE || modelInstance->GetModelType() == ModelTypes::TERRAIN_OBJECT))
        {
            extern Texture2D* obdrawGetSurfaceTexture();

            lpobj->flags &= ~OBJECT_DYNALITE;
            lpobj->flags &= ~OBJECT_LITE;

            material->SetShader("multitextureterrain");
            // Set default unit 2 texture
            material->AddTexture(2, white);
            material->AddTexture(3, obdrawGetSurfaceTexture()); // Surface effects
        }
        else if(count > 1)
        {
            lpobj->flags &= ~OBJECT_DYNALITE;
            lpobj->flags &= ~OBJECT_LITE;

            material->SetShader("multitexture3");
        }
        else if(count > 0)
        {
            lpobj->flags &= ~OBJECT_DYNALITE;
            lpobj->flags &= ~OBJECT_LITE;

            material->SetShader("multitexture2");
        }

        if(count > 0)
        {
            int multiid = texid;
            int offset = 0;

            obdrawFindMultiTextureById(multiid, &offset);
            multiid = obdrawGetMultiTextureID(offset);
            material->AddTexture(1, multiid);

            if(count > 1)
            {
                obdrawFindMultiTextureById(multiid, &offset);
                multiid = obdrawGetMultiTextureID(offset);
                material->AddTexture(2, multiid);
            }
        }

        if(meshContext)
        {
            meshContext->Apply();
            lpobj->meshContext = new ogl::MeshContext(*meshContext);
        }
    }

    // dof <-- WTF kind of comment is this?
    if(flags & OBJECT_DOF)
    {
        lpobj->dof = (OBJECTDOF*)memAlloc(sizeof(OBJECTDOF));
        lpobj->dof->dofstatic = (OBJECTSTATICDOF*)dof;
        memcpy(&lpobj->dof->dofvarvalue, &dof->dofvarvalue, sizeof(OBJECTDOF) - sizeof(OBJECTSTATICDOF*));
        lpobj->dof->dofflags |= DOF_FREE_MEMORY;
    }

    // comment <-- WTF kind of comment is this?
    lpobj->comment = comment;

    if(lpobj->flag2 & OBJECT_NORMALMAP)
    {
        glslNormalSetup(lpobj);
    }
    else if(lpobj->flag3 & OBJECT_SPECULARMAP)
    {
        glslSpecularSetup(lpobj);
    }
    else if(lpobj->flag2 & OBJECT_FLAG)
    {
        glslClothFlagSetup(lpobj);
    }
    else if(lpobj->flag3 & OBJECT_IS_ROAD)
    {
    }

	if(lpobj->prev == nullptr && lpobj->next == nullptr)
	{
		if(lpobj->flags & OBJECT_NOTOBJECT)
			obmainMoveToNotObjectTree(lpobj);
		else
			obmainMoveToObjectTree(lpobj);
	}

	// recursion away!
	if(child)		lpobj->child = child->createPoolInstance(modelInstance, transform);
	if(attached)	lpobj->attached = attached->createPoolInstance(modelInstance, transform);
	if(sibling)		lpobj->sibling = sibling->createPoolInstance(modelInstance, transform);

	return lpobj;
}
#endif

/************************************************************************************************
	PoolModel
*************************************************************************************************/
PoolModel::PoolModel(const char* path, unsigned int flags)
:		node(nullptr),
        vbo(nullptr),
        modelType(ModelTypes::GENERAL),
		placeMode(OBJVAL_NO_PLACEMODE),
		propertyFlags(PropertyFlags::HOLLOW_REFERENCE),
        loadFlags(flags),
        hastrees(false),
        hasRoads(false)
{
}

PoolModel::~PoolModel()
{
	for(UINT32 i=0; i < staticBuffers.size(); i++)
		delete staticBuffers[i];

    if(vbo)
    {
        delete vbo;
        vbo = nullptr;
    }

    if(node)
        delete node;
}


/************************************************************************************************
	load
*************************************************************************************************/

static int loadcounter = 0;

#ifdef SUPPORT_WWO
bool PoolModel::LoadWWO( std::string path )
{
    LdWWO loader(path);
    return loader.load();
}
#endif

void PoolModel::load()
{
#ifdef SUPPORT_WWO
    {
        if ( LoadWWO( directory + modelname + ".wwo" ))
        {
            return;
        }
    }
#endif

	std::string path = directory + modelname + ".mdl";

	char s[512];

	if(fioQuery() == ISDIRECTORY && ModelNeedsRebuild(path.c_str()))
	{
#ifndef COLLECT_TEXTURE_STATS
		dbugLog(_FLD, "%4d: RebuildMDL: %s\n", ++loadcounter, path.c_str());
#endif
		this->RebuildMDL();
    }

	LoadMDL(path.c_str());
}


/************************************************************************************************
	LoadMDL
*************************************************************************************************/
void PoolModel::LoadMDL(const char* path)
{
	int fd = fioOpen(path);
	if(fd < 0) return;


    // HACK: this is for texture loading only. Texture data must be held for
    // buildings that use the alpha collider. Otherwise, the data can be freed
    // after the texture has been created. This saves quite a bit of memory. When
    // the paper buildings (alpha collider buildings) are redone where they no
    // longer use the alpha collider, they can be removed from this list.
    static std::set<std::string> AlphaColliderList;

    if(AlphaColliderList.empty())
    {
        AlphaColliderList.insert("terrain/bld/factoryblock1.mdl");
        AlphaColliderList.insert("terrain/to/rdw1-4bird3-6.mdl");
    }

    AlphaColliderImage = false;

    std::string str = path;
    std::replace(str.begin(), str.end(), '\\', '/');

    if(AlphaColliderList.find(str) != AlphaColliderList.end())
    {
        AlphaColliderImage = true;
    }

	OBJECTFILE mdlHeader;

	mdlHeader.company = fioGets(fd);
	mdlHeader.filetype = fioGets(fd);
	mdlHeader.version = fioGets(fd);
	mdlHeader.length = fioGetl(fd);

	if(	mdlHeader.company != PLAYNET_COMPANY )
    {
        Write_Debug("[LoadMDL] mdlHeader.company != PLAYNET_COMPANY. Probably need to rebuild MDLs");
        return;
    }

	if(	mdlHeader.filetype != PLAYNET_OBJECTFILE )
    {
        Write_Debug("[LoadMDL] mdlHeader.filetype != PLAYNET_OBJECTFILE. Probably need to rebuild MDLs");
        return;
    }

	if(	mdlHeader.version != OBJECTFILE_VERSION )
    {
        Write_Debug("[LoadMDL] mdlHeader.version != OBJECTFILE_VERSION. Probably need to rebuild MDLs");
        return;
    }

	int nummats = fioGetl(fd);
	MATERIAL mat;
	mat.matid = mat.texid = -1;
	for (int i=0 ; i < nummats ; i++)
	{
		fioRead(fd, mat.name, 32);
		fioRead(fd,	mat.texname, MDL_MAX_PATH);
		fioRead(fd, mat.alphaname, MDL_MAX_PATH);
		fioReadf(fd, (float*)&mat.material, sizeof(piMATERIAL) / sizeof(FLOAT32));

		fioGetl(fd);
		fioGetl(fd);
		fioGetl(fd);

		obcMaterial(&mat);
	}

	SINT32	staticBufferCnt = fioGetl(fd);
	for(int i=0; i < staticBufferCnt ; i++)
	{
		staticBuffers.push_back(new ogl::StaticBuffer(fd));
 	}

	fioRead(fd, &propertyFlags, sizeof(int));
	SubModelReference	subModelReference;
	OBJECT::Name		subModelName;
	OBJECT::Name		subModelPath;
	std::string			subModelFileName;
	int					subModelCount	= fioGetl(fd);

	for(int i=0; i < subModelCount; i++)
	{
		fioRead(fd, &subModelName, sizeof(subModelName));
		fioRead(fd, &subModelPath, sizeof(subModelName));
		fioRead(fd, &subModelReference.transform, sizeof(piMATRIX));
		fioRead(fd, &subModelReference.placeOntoMasterObject, sizeof(bool));

		subModelFileName = subModelPath;
		subModelFileName.append(subModelName);
		subModelFileName.append(".flt");

		obmgrAddModelToPool(subModelFileName.c_str(), false, loadFlags);
		subModelReference.poolModel = (_poolmap.find(subModelName))->second;

		if(!subModelReference.poolModel)
		{
			Write_Debug("Instance model %s not found!", subModelFileName.c_str());
			piExitProgram(_FL, "Instance model not found!");
		}

		subModelReferences.push_back(subModelReference);
	}

	fioRead(fd, &modelType, sizeof(int));
	fioRead(fd, &placeMode, sizeof(int));

	Vec3f	bounds[2];
	fioRead(fd, &bounds[0], sizeof(Vec3f));
	fioRead(fd, &bounds[1], sizeof(Vec3f));

    this->bounds.set(bounds[0], bounds[1]);

	node = new PoolNode;
	node->load(this, fd);

    if(modelType == ModelTypes::CLOUD)
    {
        vbo = new oglVertexBuffer;

        if(ogl::UseDeprecatedPath())
            vbo->Apply(node, true, oglVertexBuffer::MODE_ARB_VBO);
        else
            vbo->Apply(node, true, oglVertexBuffer::MODE_ARB_VAO);

        vbo->Buffer();
    }

	for(UINT32 i=0; i < staticBuffers.size() ; i++)
	{
		staticBuffers[i]->Buffer();
 	}

	fioClose(fd);

  /* I now create the pool node's collider at runtime if needed in
  // physicsCreateObjectCollider; it check's if the modelNode has a collider or not
  // and then creates one if need be; this works better since "what is a collide"
  // object varies from object to object and some place modes aren't
  // compatible with pool node colliders (sheared and non-static models)
  // loop over node and create ode colliders
  switch(modelType)
  {
		case ModelTypes::CLOUD:
		case ModelTypes::GRASS:
		case ModelTypes::GENERAL:
			// no collider
		break;
		default:
			_CreateODEColliderBase(this);
		break;
  }
  */
}

void PoolModel::CollectCollideBranches(OBJECT* o)
{
	bool	continueBranch = true;


	if(o->comment)
	{
		if(strstr(o->comment, "speedtree"))
		{
			continueBranch = false;				// Some speedtree branches are flagged as collider
		}
		else if(strstr(o->comment, "collide"))
		{
			collideBranches.push_back(o);
			continueBranch = false;
		}
	}

	if(continueBranch && o->child)	CollectCollideBranches(o->child);
	if(o->sibling)	CollectCollideBranches(o->sibling);
	if(o->attached)	CollectCollideBranches(o->attached);
}


void PoolModel::CompileModelBounds(OBJECT* o)
{
	bool	continueBranch = true;

	if(o->comment)
	{
		if(	strstr(o->comment, "speedtree")		||
			strstr(o->comment, "instance")		||
			(strstr(o->comment, "spawnobject")	&& modelType != ModelTypes::SPAWN)	||
			strstr(o->comment, "nofire")		||
			strstr(o->comment, "entity") )
		{
			continueBranch = false;
		}
	}
	if(!(o->flags & OBJECT_NOTOBJECT) && o->numverts != 0)
	{
		bounds.expandBy(Vec3f(o->bbox[0]) * Matrixf::translate(o->pos));
		bounds.expandBy(Vec3f(o->bbox[1]) * Matrixf::translate(o->pos));
	}

	if(continueBranch && o->child)	CompileModelBounds(o->child);
	if(o->attached)	CompileModelBounds(o->attached);
	if(o->sibling)	CompileModelBounds(o->sibling);
}


void PoolModel::ProcessTerrainObjectCollisionData(OBJECT* o)
{
	CollectCollideBranches(o);
	if(collideBranches.size() > 2)
	{
		std::string	errorMessage = modelname;
		errorMessage.append(".flt: too many collidable LODs found!");
		piExitProgram(_FL, errorMessage.c_str());
	}

	if(collideBranches.empty())	ldmdlProcessBranchCollisionData(o->child);
	else
	{
		for(int i=0; i < (int)collideBranches.size(); i++)
		{
			ldmdlProcessBranchCollisionData(collideBranches[i]->child);
		}
	}
}


//////////////////////////////////////////////////////////////////////
//	ProcessVehicleCollisionData
//
//	Vehicles have their dedicated collider meshes.
//////////////////////////////////////////////////////////////////////
void PoolModel::ProcessVehicleCollisionData(OBJECT* o)
{
	LOGICAL		processCollisionData	= FALSE;


	if(o->name)
	{
		if(	strstr(o->name, "hitmap")		||
			strstr(o->name, "hitbox")		||
			strstr(o->name, "visdama")		||
			strstr(o->name, "shadow")		||
			strstr(o->name, "linecollider")	||			// Not sure if that is really required
			strstr(o->name, "gsite")	)
		{
			processCollisionData = TRUE;
		}
	}
	if(o->comment)
	{
		if(strstr(o->comment, "reflectorsight"))
		{
			processCollisionData = TRUE;
		}
	}

	if(processCollisionData)
	{
		if(o->child)	ldmdlProcessBranchCollisionData(o->child);
		else	ldmdlProcessBranchCollisionData(o);
	}

	if(!processCollisionData &&	o->child)	ProcessVehicleCollisionData(o->child);
	if(!processCollisionData &&	o->attached)	ProcessVehicleCollisionData(o->attached);
	if(o->sibling)	ProcessVehicleCollisionData(o->sibling);
}

void PoolModel::SetProperties(const OBJECT* o)
{
	if(o->comment)
	{
		if(strstr(o->comment, "place"))		placeMode =  atoi(strstr(o->comment, "place") + strlen("place"));
		if(strstr(o->comment, "damage"))	propertyFlags |= PropertyFlags::STRAT_DATA;
	}

	if(o->child)	SetProperties(o->child);
	if(o->attached)	SetProperties(o->attached);
	if(o->sibling)	SetProperties(o->sibling);
}

void PoolModel::SetModelType()
{
	if		(strstr(modelname.c_str(), "cloud"))
        modelType = ModelTypes::CLOUD;
	else if	(strstr(directory.c_str(), "terrain\\ag\\"))
        modelType = ModelTypes::AUTOGUNNER;
	else if	(strstr(directory.c_str(), "terrain\\bld\\"))
        modelType = ModelTypes::BUILDING;
	else if	(strstr(directory.c_str(), "terrain\\brdg\\"))
        modelType = ModelTypes::BRIDGE;
	else if	(strstr(directory.c_str(), "terrain\\cap\\"))
        modelType = ModelTypes::CAPTURE_PLACE;
	else if	(strstr(directory.c_str(), "terrain\\cp\\"))
        modelType = ModelTypes::CONTROL_POINT;
	else if	(strstr(directory.c_str(), "terrain\\fac\\"))
        modelType = ModelTypes::FACTORY;
	else if	(strstr(directory.c_str(), "terrain\\grass\\"))
        modelType = ModelTypes::GRASS;
	else if	(strstr(directory.c_str(), "terrain\\spwn\\"))
        modelType = ModelTypes::SPAWN;
	else if	(strstr(directory.c_str(), "terrain\\to\\"))
        modelType = ModelTypes::TERRAIN_OBJECT;
	else if	(strstr(directory.c_str(), "terrain\\"))
        modelType = ModelTypes::TERRAIN_OBJECT;
	else if	(strstr(directory.c_str(), "vehicles\\"))
        modelType = ModelTypes::VEHICLE;

	// Terrain tiles should identify themselves directly instead of course
	if(strstr(modelname.c_str(), "ob3"))
        return;
	if(strstr(modelname.c_str(), "ob2"))
        return;
	if(strstr(modelname.c_str(), "ob1"))
        return;

	if(modelType == ModelTypes::TERRAIN_OBJECT)
	{
		if(	bounds.xMin() <= SDM_OCTET_SIZE/2 + FLT_EPSILON	&&
			bounds.yMin() <= SDM_OCTET_SIZE/2 + FLT_EPSILON	&&
			bounds.xMax() >= SDM_OCTET_SIZE/2 - FLT_EPSILON	&&
			bounds.yMax() >= SDM_OCTET_SIZE/2 - FLT_EPSILON	)
		{
			modelType = ModelTypes::TERRAIN_TILE;
		}
	}
}

//////////////////////////////////////////////////////////////////////
//	RebuildMDL
//////////////////////////////////////////////////////////////////////
void PoolModel::RebuildMDL()
{
	OBJECT*						topmostObj		= nullptr;								// Topmost object of the temporary .flt object tree
	POOLOBJHDR*					poolObjHdr		= nullptr;
	const std::string			fltPath			= directory + modelname + ".flt";
	UINT32						poolOffset		= 0;
	MODELMATERIALS				modelMaterials;


	topmostObj = ldfltLoad(fltPath, 1.0f, 1);
	if(!topmostObj)
	{
		Write_Debug("Error loading FLT file for %s", fltPath.c_str());
		piExitProgram(_FL, "MDLreb failed");
	}

	SetModelType();
	CompileModelBounds(topmostObj);
	SetModelType();		// Again because we have to identify replacement tiles by their model bounds.
	SetProperties(topmostObj);

#ifdef COLLECT_TEXTURE_STATS
    printf("[mdl] %s\n", fltPath.c_str() );

    TextureRefSet refset;
    CollectModelTextures(topmostObj, refset);
    for ( TextureRefSet::iterator it = refset.begin(); it!=refset.end(); it++)
    {
        const char* s = it->c_str();
        printf("[tex] %s\n", it->c_str() );
    }
#endif

    CollectModelMaterials(topmostObj, &modelMaterials);

	CreateContexts(topmostObj, TRUE);

	if(modelType == ModelTypes::VEHICLE)
	{
		propertyFlags &= ~PropertyFlags::HOLLOW_REFERENCE;
		ProcessVehicleCollisionData(topmostObj);
	}
	else
	{
		CollectModelReferences(topmostObj);
		if(!(propertyFlags & PropertyFlags::HOLLOW_REFERENCE))	ProcessTerrainObjectCollisionData(topmostObj);
	}

	//CheckModelSanity();

	poolObjHdr = obpoolObjectToPool(topmostObj, 1, 1);
	obpoolPoolOffsetsCallback(poolObjHdr, nullptr, &poolOffset);


	SaveMDL(poolObjHdr, &modelMaterials);

	obmainDeleteNodeTree(topmostObj);
	obpoolFreeObject(poolObjHdr);
	staticBuffers.clear();

 // printf("loaded %s\n",fltPath.c_str());

#ifdef COLLECT_TEXTURE_STATS
    printf("[end]\n" );
#endif
	return;
}

//////////////////////////////////////////////////////////////////////
//	SaveMDL
//
//Save a raw pool object from the pool to disk.
//Includes compiling the required materials.
//////////////////////////////////////////////////////////////////////
void PoolModel::SaveMDL(const POOLOBJHDR* poolobjhdr,
						 const MODELMATERIALS* modelmaterials)

{
	int							hfile			= 0;
	std::string					mdlpath			= directory + modelname + ".mdl";
	UINT32						length			= 0;
	MODELMATERIALS::const_iterator	cur;


	Write_Debug("Starting to save %s", mdlpath.c_str());

	hfile = fioOpenWrite(mdlpath.c_str());
	if(hfile == -1)
	{
		piExitProgram(_FL, "Could not create .mdl file!");
	}

	fioPuts(hfile, PLAYNET_COMPANY);
	fioPuts(hfile, PLAYNET_OBJECTFILE);
	fioPuts(hfile, 0); // OBJECTFILE_VERSION);
	fioPutl(hfile, 0); // length, fill in later

	fioPutl(hfile, modelmaterials->size());

	for(cur = modelmaterials->begin(); cur != modelmaterials->end(); cur++)
	{
		fioWrite(hfile, cur->name, 32);
		fioWrite(hfile, cur->texname, MDL_MAX_PATH);
		fioWrite(hfile, cur->alphaname, MDL_MAX_PATH);
		fioWritef(hfile, (FLOAT32*)&cur->material, sizeof(piMATERIAL) / sizeof(FLOAT32));
		// remove these if we change the format again.
		fioPutl(hfile, -1);// matlist[i].matid);
		fioPutl(hfile, -1);// matlist[i].texid);
		fioPutl(hfile, 0); // matlist[i].numuses);

#ifdef COLLECT_TEXTURE_STATS
        printf( "[mat] %-32s ", cur->name );
        printf( "{%.6f %.6f %.6f %.6f} "
                "{%.6f %.6f %.6f %.6f} "
                "{%.6f %.6f %.6f %.6f} "
                "{%.6f %.6f %.6f %.6f} "
                "{%.6f}\n",
                cur->material.diffuse.r,
                cur->material.diffuse.g,
                cur->material.diffuse.b,
                cur->material.diffuse.a,
                cur->material.ambient.r,
                cur->material.ambient.g,
                cur->material.ambient.b,
                cur->material.ambient.a,
                cur->material.specular.r,
                cur->material.specular.g,
                cur->material.specular.b,
                cur->material.specular.a,
                cur->material.emissive.r,
                cur->material.emissive.g,
                cur->material.emissive.b,
                cur->material.emissive.a,
                cur->material.power
                );
#endif
	}

	fioPutl(hfile, staticBuffers.size());
	for(UINT32 i=0; i < staticBuffers.size(); i++)
	{
		staticBuffers[i]->SaveToFile(hfile);
	}


	OBJECT::Name	subModelName;
	OBJECT::Name	subModelPath;
	int				subModelCount	= subModelReferences.size();

	assert( (this->modelname.size() * sizeof(char)) <= sizeof(subModelName) );

	fioWrite(hfile, &propertyFlags, sizeof(int));
	fioPutl(hfile, subModelCount);
	for(int i=0; i < subModelCount; i++)
	{
		strcpy(subModelName, subModelReferences[i].poolModel->modelname.c_str());
		strcpy(subModelPath, subModelReferences[i].poolModel->directory.c_str());
		fioWrite(hfile, subModelName, sizeof(subModelName));
		fioWrite(hfile, subModelPath, sizeof(subModelPath));
		fioWrite(hfile, &subModelReferences[i].transform, sizeof(piMATRIX));
		fioWrite(hfile, &subModelReferences[i].placeOntoMasterObject, sizeof(bool));
	}

	fioWrite(hfile, &modelType, sizeof(int));
	fioWrite(hfile, &placeMode, sizeof(int));

	Vec3f	bounds[2] = {this->bounds.corner(0), this->bounds.corner(7)};
	fioWrite(hfile, (void*)&bounds[0], sizeof(Vec3f));
	fioWrite(hfile, (void*)&bounds[1], sizeof(Vec3f));

	obpoolSavePoolCallback((POOLOBJHDR*)poolobjhdr, hfile);

	// fill in length
	length = fioSeek(hfile, 0, SEEK_CUR);
	fioSeek(hfile, 4, SEEK_SET);

	// This is now delayed until* after* successful file creation.
	fioPuts(hfile, OBJECTFILE_VERSION);
	fioPutl(hfile, length);

	fioClose(hfile);
	Write_Debug("Pool object tree saved as %s", mdlpath.c_str());

	return;
}

void PoolModel::CollectModelTextures(OBJECT* o, TextureRefSet& refset)
{
    if(o)
    {
        if(!(o->flags & OBJECT_NOTOBJECT))
	    {
            if ( o->texname[0] )
            {
                char s[256], *p = s;
                strcpy( s, directory.c_str() );
                while (*p != 0 )
                {
                    if ( *p == '\\' ) *p = '/';
                    p++;
                }
                strcat( s, o->texname );

                refset.insert( std::string(s) );

                if ( o->flag2 & OBJECT_NORMALMAP )
                {
                    refset.insert( std::string(s)+"_normal" );
                }
                if ( o->flag3 & OBJECT_SPECULARMAP )
                {
                    refset.insert( std::string(s)+"_spec" );
                }

            }
        }

        CollectModelTextures(o->attached,refset);
        CollectModelTextures(o->child,refset);
        CollectModelTextures(o->sibling,refset);
	}

	return;
}

//////////////////////////////////////////////////////////////////////
//	CollectModelMaterials
//////////////////////////////////////////////////////////////////////
void PoolModel::CollectModelMaterials(OBJECT* o, MODELMATERIALS* modelmaterials)
{
	MODELMATERIALS::iterator	cur				= modelmaterials->begin();
	MATERIAL*					globalmats		= nullptr;
	piGetMaterials(&globalmats);

    if(o)
    {
        if(!(o->flags & OBJECT_NOTOBJECT))
	    {
            int id = oglFindMaterialName(o->matname);

            if(id > 0)
            {
                cur = find(modelmaterials->begin(), modelmaterials->end(), globalmats[id]);

		        if(cur == modelmaterials->end())
		        {
			        modelmaterials->push_back(globalmats[id]);
		        }
            }
        }

        CollectModelMaterials(o->attached, modelmaterials);
        CollectModelMaterials(o->child, modelmaterials);
        CollectModelMaterials(o->sibling, modelmaterials);
	}

	return;
}


//////////////////////////////////////////////////////////////////////
//	CollectModelReferences
//////////////////////////////////////////////////////////////////////
void PoolModel::CollectModelReferences(OBJECT* o)
{
	SubModelReference	subModelReference;
	std::string			fullModelFileName;
	std::string			modelName;

	if(!(o->flags & OBJECT_NOTOBJECT))
	{
		if(o->comment && strstr(o->comment, "instance "))
		{
			fullModelFileName = o->comment;
			fullModelFileName.erase(fullModelFileName.begin(), fullModelFileName.begin() + 9);	// Get rid of the "instance " string.

			size_t start = fullModelFileName.rfind('\\');
			size_t end = fullModelFileName.rfind('.');
			if(start == std::string::npos)	modelName = fullModelFileName.substr(0,end);
			else	modelName = fullModelFileName.substr(start+1,end-start-1);

			if(modelName.compare(this->modelname))												// Must not refer to itself.
			{
				obmgrAddModelToPool(fullModelFileName.c_str(), false, loadFlags);
				subModelReference.poolModel = (_poolmap.find(modelName))->second;
				if(!subModelReference.poolModel)
				{
					Write_Debug("Instance model %s not found!", modelName.c_str());
					piExitProgram(_FL, "Instance model not found!");
				}

				memcpy(&subModelReference.transform, &o->mat, sizeof(piMATRIX));					// This object also carries the transform matrix.
				subModelReference.placeOntoMasterObject = false;
				if(strstr(o->comment, "ontomaster"))	subModelReference.placeOntoMasterObject = true;

				subModelReferences.push_back(subModelReference);
			}
		}
	}
	if(o->flags & OBJECT_LOD)	propertyFlags &= ~PropertyFlags::HOLLOW_REFERENCE;

	if(o->attached)	CollectModelReferences(o->attached);
	if(o->child)	CollectModelReferences(o->child);
	if(o->sibling)	CollectModelReferences(o->sibling);

	return;
}


//////////////////////////////////////////////////////////////////////
//	ModelNeedsRebuild
//////////////////////////////////////////////////////////////////////
LOGICAL PoolModel::ModelNeedsRebuild(const char* path)
{
	OBJECTFILE		mdlHeader;
	SINT32			fltmod, mdlmod;
	int				fd					= -1;

	if (fioQuery() == ISARCHIVE) return FALSE;

    std::string mdlpath = path;

    size_t ext = mdlpath.rfind('.');
    if(ext != std::string::npos) mdlpath.erase(ext);

    std::string fltpath(mdlpath);
    fltpath += ".flt";
    mdlpath += ".mdl";

    fd = fiodOpen(mdlpath.c_str(), false, sizeof(mdlHeader));
    if(fd < 0) return TRUE;

    fiodRead( fd, &mdlHeader.company, sizeof(short) );
    fiodRead( fd, &mdlHeader.filetype, sizeof(short) );
    fiodRead( fd, &mdlHeader.version, sizeof(short) );

    fiodClose(fd);

    if(mdlHeader.version != OBJECTFILE_VERSION) return TRUE;

    fiodModified(&fltmod, fltpath.c_str());
    fiodModified(&mdlmod, mdlpath.c_str());

    return (fltmod > mdlmod);
}


//////////////////////////////////////////////////////////////////////
//	InterleaveTangentData
//////////////////////////////////////////////////////////////////////
void PoolNode::InterleaveTangentData(const std::vector<Vec4f>& t,
                                     const std::vector<Vec4f>& b,
                                     const std::vector<Vec4f>& n,
                                     PoolModel& poolModel)
{
	std::vector<RawVertex>	rawVertices;
	RawVertex				tmpRawVertex	= {0};
	piUNIFIED_VERTEX*		puvert			= vertices;
	int						bufferIdx		= 0;


	meshContext->AddVertexAttribute(nullptr, ATTRIB6 | ATTRIB7 | ATTRIB15);

    rawVertices.resize(objdata->numverts);

	for(int i=0; i < objdata->numverts; i++, puvert++)
	{
		//Position
		tmpRawVertex.p_x	= puvert->x;
		tmpRawVertex.p_y	= puvert->y;
		tmpRawVertex.p_z	= puvert->z;

		//Normal
		tmpRawVertex.n_x	= puvert->nx;
		tmpRawVertex.n_y	= puvert->ny;
		tmpRawVertex.n_z	= puvert->nz;

		//Texture Coordinate 0
		tmpRawVertex.t0_u	= puvert->tu;
		tmpRawVertex.t0_v	= puvert->tv;

		tmpRawVertex.tan_x = t[i].x();
		tmpRawVertex.tan_y = t[i].y();
		tmpRawVertex.tan_z = t[i].z();

		tmpRawVertex.bin_x = b[i].x();
		tmpRawVertex.bin_y = b[i].y();
		tmpRawVertex.bin_z = b[i].z();

		tmpRawVertex.nrm_x = n[i].x();
		tmpRawVertex.nrm_y = n[i].y();
		tmpRawVertex.nrm_z = n[i].z();

		rawVertices[i] = tmpRawVertex;
	}

	while(true)
	{
		if((int)poolModel.staticBuffers.size() == bufferIdx)
            poolModel.staticBuffers.push_back(new StaticBuffer());

		if(poolModel.staticBuffers[bufferIdx]->AddMesh(rawVertices, (const ogl::VertexIdx*)indices, objdata->numtris*3, meshContext, bufferIdx))
            break;

		bufferIdx++;
	}
}


//////////////////////////////////////////////////////////////////////
//	CreateContexts
//////////////////////////////////////////////////////////////////////
void PoolModel::CreateContexts(OBJECT* o, LOGICAL isStaticMeshBranch)
{
	std::vector<RawVertex>					rawVertices;
	RawVertex								tmpRawVertex		= {0};
	piUNIFIED_VERTEX*						puvert				= o->puverts;
	UINT32									vertexType			= 0;
	MeshContext*							meshContext			= nullptr;
	LOGICAL									isBufferable		= TRUE;				// Indicates if the objects mesh can be buffered at preloading
	UINT32									bufferIdx			= 0;


	if(	!(o->flags & OBJECT_NOTOBJECT)	&&
		!(o->flag2 & OBJECT_HIDDEN)		&&
		o->numverts > 0	)
	{
		if(!isStaticMeshBranch	||	!IsStaticTerrainModel())
            isBufferable = FALSE;

        if(o->flags & OBJECT_LINESTRIP || 	o->flags & OBJECT_LINECLOSED)
            isBufferable = false;

        if(	strstr(o->name, "left tread")	||
           !strncmp(o->name, "ind", 3)      || // Not all 'decals' are under the 'decal' object (Needs to be fixed)
           !strncmp(o->name, "per", 3)      || // would be nice if we had a f'ng standard...
			strstr(o->name, "right tread") )
		{
			isBufferable = false;
		}

		if(o->comment)
		{
			if(	strstr(o->comment, "trackmove")		||
				strstr(o->comment, "connect")       ||
                strstr(o->comment, "reflectorsight") )
			{
				isBufferable = false;
			}
		}

		vertexType |= POSITION;
		vertexType |= NORMAL;
		vertexType |= TEX0;

        TangentSpaceGenerator tsg;
        std::vector<Vec4f>* t = nullptr;
        std::vector<Vec4f>* b = nullptr;
        std::vector<Vec4f>* n = nullptr;

        if(isBufferable && (o->flag2 & OBJECT_NORMALMAP))
        {
            vertexType |= (ATTRIB6 | ATTRIB7 | ATTRIB15);

            piVectorConstPtr pos = o->RenderPositions();
            piVectorConstPtr norms = o->RenderNormals();
            piTexCoordConstPtr tex = o->RenderTexCoords();

            // Generate tangent data for normal mapping
            tsg.generate(pos, norms, tex, o->ptris, o->numverts, o->numtris, o->flags);

            t = &tsg.getTangentArray();
            b = &tsg.getBinormalArray();
            n = &tsg.getNormalArray();
        }
        else if(o->flag2 & OBJECT_NORMALMAP)
        {
            // Only VBO objects to normal map for now
            o->flag2 &= ~OBJECT_NORMALMAP;
        }

		meshContext = new MeshContext(vertexType);

		if(isBufferable)
		{
			o->flag2 |= OBJECT_USEBUFFERS;

            rawVertices.resize(o->numverts);

			for(UINT32 i=0; i < o->numverts; i++, puvert++)
			{
				// Skip the color data for 32 Byte alignment

				//Position
				tmpRawVertex.p_x	= puvert->x;
				tmpRawVertex.p_y	= puvert->y;
				tmpRawVertex.p_z	= puvert->z;

				//Normal
				tmpRawVertex.n_x	= puvert->nx;
				tmpRawVertex.n_y	= puvert->ny;
				tmpRawVertex.n_z	= puvert->nz;

				//Texture Coordinate 0
				tmpRawVertex.t0_u	= puvert->tu;
				tmpRawVertex.t0_v	= puvert->tv;

                // Add tangent data for normal mapping
                if((o->flag2 & OBJECT_NORMALMAP) && (t &&  b && n))
                {
                    tmpRawVertex.tan_x = (*t)[i].x();
                    tmpRawVertex.tan_y = (*t)[i].y();
                    tmpRawVertex.tan_z = (*t)[i].z();

		            tmpRawVertex.bin_x = (*b)[i].x();
		            tmpRawVertex.bin_y = (*b)[i].y();
		            tmpRawVertex.bin_z = (*b)[i].z();

		            tmpRawVertex.nrm_x = (*n)[i].x();
		            tmpRawVertex.nrm_y = (*n)[i].y();
		            tmpRawVertex.nrm_z = (*n)[i].z();
                }

				rawVertices[i] = tmpRawVertex;
			}

			while(true)
			{
				if(staticBuffers.size() == bufferIdx)
                    staticBuffers.push_back(new StaticBuffer());

                if(staticBuffers[bufferIdx]->AddMesh(rawVertices, (const ogl::VertexIdx*)o->ptris, o->numtris*3, meshContext, bufferIdx))
                    break;

                bufferIdx++;
			}
		}//isBufferable

		o->meshContext = meshContext;
	}

	if(o->child)
	{
		LOGICAL	isStaticMeshParent	= TRUE;

		if(!isStaticMeshBranch)	isStaticMeshParent = FALSE;

		// Check if this a top node (group) of a branch that
		if(o->comment)
		{
			if( !strcmp(o->comment, "speedtree")		||
				strstr(o->comment, "lefttread")			||
				strstr(o->comment, "righttread")		||
				strstr(o->comment, "canopy")			||
				strstr(o->comment, "bombbaydoor")		||
				strstr(o->comment, "enginefast")		||
				strstr(o->comment, "engineslow")		||
				strstr(o->comment, "trackmove")			||
				strstr(o->comment, "seperate")          ||
                strstr(o->comment, "viewpoint")         ||
                strstr(o->comment, "separate")) // ...in kase it's speld korectly
			{
				isStaticMeshParent = FALSE;
			}
		}

		if(o->name)
		{
			if(	!strcmp(o->name, "fx")				||
				!strcmp(o->name, "lodc")			||
				!strcmp(o->name, "vcpit")			||
				!strncmp(o->name, "decal", 5)		||
				!strcmp(o->name, "lodloadout"))
			{
				isStaticMeshParent = FALSE;
			}
		}
		CreateContexts(o->child, isStaticMeshParent);
	}

    if(o->attached)
        CreateContexts(o->attached, isStaticMeshBranch);

    if(o->sibling)
        CreateContexts(o->sibling, isStaticMeshBranch);

	return;
}

//////////////////////////////////////////////////////////////////////
//	PoolModel::IsStaticTerrainModel
//////////////////////////////////////////////////////////////////////
LOGICAL PoolModel::IsStaticTerrainModel()	const
{
	switch(modelType)
	{
		case ModelTypes::TERRAIN_TILE:
            {
                switch(placeMode)
                {
                    // These place modes do not have mutable vertices, so
                    // it's safe to VBO.
                case OBJVAL_PLACE_BASE:
                case OBJVAL_PLACE_CONCAVE:
                case OBJVAL_PLACE_SHEARED:
                case OBJVAL_PLACE_STRAIGHT_DOWN:
                case OBJVAL_NO_PLACEMODE:
                    {
                        return true;
                    }
                }

                return false;
            }
		case ModelTypes::CLOUD:
		case ModelTypes::SPAWN:
			return false;
	}

    if(placeMode == OBJVAL_BOTTOM_CONFORM)
        return false;

    if(modelType == ModelTypes::TERRAIN_OBJECT && placeMode <= OBJVAL_OCTET_CONFORMRADIAL)
        return false;

    return true;
}

LOGICAL PoolModel::IsReplacementTile() const
{
	return ((loadFlags & LOAD_FLAG_REPLACEMENTTILE) > 0) ? TRUE: FALSE;
}

LOGICAL PoolModel::IsEnterable() const
{
	return ((loadFlags & LOAD_FLAG_ENTERABLE) > 0) ? TRUE: FALSE;
}


//////////////////////////////////////////////////////////////////////
//	PoolModel::CheckModelSanity
//////////////////////////////////////////////////////////////////////
void PoolModel::CheckModelSanity()	const
{
#ifdef BETA_RELEASE
	if(propertyFlags & PropertyFlags::HOLLOW_REFERENCE && subModelReferences.empty())
	{
		if(	modelname.find("flagtable")		== std::string::npos	&&
			modelname.find("debris")		== std::string::npos	&&
			modelname.find("sunglare")		== std::string::npos	&&
			modelname.find("fullmoon")		== std::string::npos	&&
			modelname.find("tracker")		== std::string::npos	&&
			modelname.find("compassring")	== std::string::npos	&&
			modelname.find("clutter")		== std::string::npos	&&
			modelname.find("muzzleflash")	== std::string::npos	)
		{
			Write_Log(LOG_FPS, LOG_INFO, "Empty model: %s", modelname.c_str());
			piExitProgram(_FL, "Empty model found!");
		}
	}
#endif	// #ifdef BETA_RELEASE
}

bool PoolModel::GetSubObjectBoundsByName(BoundingBox* bounds, const char* name, const PoolNode* o) const
{
	assert(bounds);
	assert(name);

    static FindByName<PoolNode> fpn;

    fpn.set(name);
    fpn.find(!o ? this->node : o);

    if(!fpn.node)
        return false;

    const PoolNode* oo = fpn.node;

    for(; oo != nullptr; oo = oo->attached)
    {
        bounds->expandBy(oo->objdata->bbox[0].x + oo->objdata->pos.x,
                         oo->objdata->bbox[0].y + oo->objdata->pos.y,
                         oo->objdata->bbox[0].z + oo->objdata->pos.z);

        bounds->expandBy(oo->objdata->bbox[1].x + oo->objdata->pos.x,
                         oo->objdata->bbox[1].y + oo->objdata->pos.y,
                         oo->objdata->bbox[1].z + oo->objdata->pos.z);
    }

    return bounds->valid();
}


bool PoolModel::GetSubObjectBoundsByComment(BoundingBox* bounds, const char* comment, const PoolNode* o) const
{
	assert(bounds);
	assert(comment);

    static FindByComment<PoolNode> fpn;
    fpn.set(comment);
    fpn.find(!o ? this->node : o);

    if(!fpn.node)
        return false;

    const PoolNode* oo = fpn.node;

    for(; oo != nullptr; oo = oo->attached)
    {
        bounds->expandBy(oo->objdata->bbox[0].x + oo->objdata->pos.x,
                         oo->objdata->bbox[0].y + oo->objdata->pos.y,
                         oo->objdata->bbox[0].z + oo->objdata->pos.z);

        bounds->expandBy(oo->objdata->bbox[1].x + oo->objdata->pos.x,
                         oo->objdata->bbox[1].y + oo->objdata->pos.y,
                         oo->objdata->bbox[1].z + oo->objdata->pos.z);
    }

    return bounds->valid();
}

// Only for debugging in BuildTerrain
const PoolNode* GetNextLODPoolNode(const PoolNode* node)
{
	static const PoolNode*	branchTopNode	= nullptr;
	static const PoolNode*	nextLODnode		= nullptr;
	bool					stepInto		= true;


	if(!branchTopNode)	branchTopNode = node;
	if(node->comment)
	{
		if(strstr(node->comment, "speedtree") || strstr(node->comment, "spawnobject"))	stepInto = false;
	}
	if(node->flags & OBJECT_LOD)
	{
		if(!node->comment)	nextLODnode = node;
		else if(!strstr(node->comment, "separate")	&&
				!strstr(node->comment, "seperate")	&&
				!strstr(node->name, "fx"))	nextLODnode = node;
	}

	if(stepInto)
	{
		if(node->child && !nextLODnode)		GetNextLODPoolNode(node->child);
		if(node->attached && !nextLODnode)	GetNextLODPoolNode(node->attached);
	}
	if(node->sibling && !nextLODnode)	GetNextLODPoolNode(node->sibling);

	if(node == branchTopNode)
	{
		const PoolNode* nextLODnodeRetVal = nextLODnode;
		branchTopNode = nullptr;
		nextLODnode = nullptr;
		return nextLODnodeRetVal;
	}
	else return nullptr;
}


// Only for debugging in BuildTerrain
void GetBranchStats(int& numVertices, int& numObjects, float& maxlodRange, const PoolNode* node)
{
	if(node->objdata && node->objdata->numverts)
	{
		numVertices += node->objdata->numverts;
		numObjects++;
		maxlodRange = sqrt(node->objdata->maxlod);
	}

	if(node->child)		GetBranchStats(numVertices, numObjects, maxlodRange, node->child);
	if(node->attached)	GetBranchStats(numVertices, numObjects, maxlodRange, node->attached);
	if(node->sibling)	GetBranchStats(numVertices, numObjects, maxlodRange, node->sibling);
}


// Only for debugging in BuildTerrain
void PoolModel::GetMinLODStats(int& minNumVertices, int& minNumObjects, float& maxlodRange) const
{
	const PoolNode*		lodNode			= node;


	while(true)
	{
		int numVertices	= 0;
		int numObjects	= 0;
		float lodRange	= 0;

		lodNode = GetNextLODPoolNode(lodNode);
		if(!lodNode)	break;
		GetBranchStats(numVertices, numObjects, lodRange, lodNode->child);

		if(numVertices < minNumVertices || !minNumVertices)
		{
			minNumVertices = numVertices;
			if(numObjects < minNumObjects || !minNumObjects)	minNumObjects = numObjects;
			maxlodRange = lodRange;
		}
		if(!lodNode->sibling)	break;
		else lodNode = lodNode->sibling;
	}
}


//////////////////////////////////////////////////////////////////////
//	ModelInstance::ModelInstance
//////////////////////////////////////////////////////////////////////
ModelInstance::ModelInstance(PoolModel* pM, const piMATRIX& m)
	:	poolModel(pM),
		bounds(pM->bounds),
        transform(m)
{
}


//////////////////////////////////////////////////////////////////////
//	ModelInstance::~ModelInstance
//////////////////////////////////////////////////////////////////////
ModelInstance::~ModelInstance()
{
}

LOGICAL ModelInstance::IsStaticTerrainModel()	const
{
	return poolModel->IsStaticTerrainModel();
}

const char*	ModelInstance::GetModelName() const
{
	return poolModel->modelname.c_str();
}

const char*	ModelInstance::GetModelDirectory() const
{
	return poolModel->directory.c_str();
}

bool ModelInstance::IsShearPlacement() const
{
    switch(GetPlaceMode())
    {
    case OBJVAL_PLACE_CONCAVE:
    case OBJVAL_PLACE_SHEARED:
        {
            return true;
        }
    case OBJVAL_PLACE_DOWN:
    case OBJVAL_OCTET_CONFORMRADIAL:
        {
            int modeltype = GetModelType();

			if(modeltype != ModelTypes::TERRAIN_TILE && modeltype != ModelTypes::TERRAIN_OBJECT)
            {
                return true;
            }
        }
    }

    return false;
}


PoolModel* obmgrGetPoolModel(const char* path)
{
	return obmgrAddModelToPool(path, true);
}

const PoolMap* 	obMgrGetPoolMap()
{
	return &_poolmap;
}

static void _CreateODEVehicleCollider(PoolModel* pm)
{
	PoolNode* root = pm->node;

	FindByName<PoolNode> finder;

	// setup our vehicle colliders
	finder.set("hitmap");
	finder.find(root);

	PoolNode* hitmap = (PoolNode*)finder.node;

	if(hitmap)
	{
		finder.set("inside");
		finder.find(hitmap);

		if(finder.node)
			physicsRecurseAndCreateNodeCollider(pm, (PoolNode*)finder.node);

		finder.set("outside");
		finder.find(hitmap);

		if(finder.node)
			physicsRecurseAndCreateNodeCollider(pm, (PoolNode*)finder.node);
	}

	// setup our hulk collider
	finder.set("lodhulk");
	finder.find(root);

	if(finder.node)
		physicsRecurseAndCreateNodeCollider(pm, (PoolNode*)finder.node);
}

static void _CreateODETerrainCollider(PoolModel* pm)
{
	PoolNode* root = pm->node;

	FindByComment<PoolNode> finder;

	int created = 0;

	// setup our vehicle colliders
	finder.set("collideobject");
	finder.find(root);

	if(finder.node)
		created += physicsRecurseAndCreateNodeCollider(pm, (PoolNode*)finder.node);

	finder.set("damagecollideobject");
	finder.find(root);

	if(finder.node)
		created += physicsRecurseAndCreateNodeCollider(pm, (PoolNode*)finder.node);

	// if no colliders were created from specific collide objects, use the lod1 collide object
	if(created == 0)
	{
		finder.set("lod1collideobject");
		finder.find(root);

		if(finder.node)
			created = physicsRecurseAndCreateNodeCollider(pm, (PoolNode*)finder.node);

		// if that fails, try the entire object
		if(created == 0)
			physicsRecurseAndCreateNodeCollider(pm, root);
	}
}

void _CreateODEColliderBase(PoolModel* pm)
{
	if(!pm->node)
		return;

	// recurse over pool model nodes and create physics colliders for them
	if(pm->modelType != ModelTypes::VEHICLE) // common case first
	{
		_CreateODETerrainCollider(pm);
	}
	else
	{
		_CreateODEVehicleCollider(pm);
	}
}
