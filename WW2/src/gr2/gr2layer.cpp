#include <syscfg.h> // Path is project specific.

#include <string>
#include <vector>

#include "gr2/gr2layer.h"
#include "gr2/gr2Physics.h"
#include "mem/mem.h"
#include "include/vars.h"
#include "ld/ldpix.h"
#include "math/math3d.h"
#include "fio/fio.h"
#include "fp/fpTPS.h"
#include "ml/mlutility.h"
#include "veh/vehdecal.h"
#include "ld/ldtex.h"
#include "fp/fpFPS.h"
#include "spt/Random.h"
#include "xml/XMLLoader.h"
#include "ob/obc.h"
#include "shader/glslNormal.h"
#include <stack>
#include "gr2/gr2Main.h"
#include "ogl/oglMaterial.h"
#include "scen/ScenarioManager.h"
#include "ogl/States.h"
#include "ld/ldtexture.h"

#include "ui/uiChat.h"

#include "str/stringLib.h"
#include "ogl/oglGlobalStates.h"
#include "ob/obdraw.h"
#include "ld/lddds.h"
#include "ogl/GLCounter.h"

#define ALLOW_RESURRECT
//#undef ALLOW_RESURRECT

#if USE_STATESET
static ref_ptr<RenderStateSet> depthpass;
#endif

#ifdef BETA_RELEASE
#include "draw/draw3d.h"
#include "rn/rnmain.h"
#endif

#include "fp/fpTPSdata.h"

static char texname[512];

#define GRANNY_VERSION(x,y) (GrannyProductMajorVersion >= x && GrannyProductMinorVersion >= y)
#define GR2_TEXTURE_PATH "gr2Data/textures/"

static ref_ptr<oglMaterial> fpsmaterial;

#ifdef GR2_TEXTURE_DEBUG
static std::map<std::string,std::string> _gr2Textures;
#endif

struct GR2RenderLine
{
	GR2RenderLine()
	{
		from[0] = from[0] = from[0] = 0.0f;
		to[0] = to[0] = to[0] = 0.0f;
	}

	GR2RenderLine(float f[4][4], float t[4][4])
	{
		SetFromMatrix(f, t);
	}

	void SetFromMatrix(float f[4][4], float t[4][4])
	{
		from[0] = f[3][0];
		from[1] = f[3][1];
		from[2] = f[3][2];

		to[0] = t[3][0];
		to[1] = t[3][1];
		to[2] = t[3][2];
	}

	float from[3];
	float to[3];
};

struct gr2Material
{
	granny_real32	ambient[3];
	granny_real32	diffuse[3];
	granny_real32	specular[3];
	granny_real32	emissive[3];
	granny_real32	shininess;
	granny_real32	shininess_strength;

};

granny_data_type_definition MaterialFieldsType[] =
{
	{GrannyReal32Member, "Ambient Color", 0, 3},
	{GrannyReal32Member, "Diffuse Color", 0, 3},
	{GrannyReal32Member, "Specular Color", 0, 3},
	{GrannyReal32Member, "Shininess"},
	{GrannyReal32Member, "Shininess Strength"},
	{GrannyReal32Member, "Self Illumination Color", 0, 3},
	{GrannyReal32Member, "Self Illumination"},
	{GrannyInt32Member, "SelfIlluminationIsOn"}
};

struct gr2Scene
{
	// An animation blending buffer that I use for all models, since
	// it's contents don't need to be preserved
	granny_local_pose* SharedLocalPose;

	// needed to handle the fact that the poses are different between the
	// base skeleton and the arms pose
	float*	compositeTempBuffer;
	int		compositeTempBufferSize;

};

struct gr2FileCacheElement
{
	SCHAR*				filename;
	granny_file_info*	fileInfo;
};

// blending specific stuff
struct animSpec
{
	char		animationFile[255];
	float		defaultWeight;
	float		tracks[255];		//bone number is index
};

typedef GR2_POOL<gr2Model> GR2_MODEL_POOL;
static GR2_MODEL_POOL _gr2ModelPool = GR2_MODEL_POOL(2000, 32, GR2_MODEL_POOL::NEW);

//typedef GR2_POOL<gr2AnimModel> GR2_ANIMMODEL_POOL;
//static GR2_ANIMMODEL_POOL _gr2AnimModelPool = GR2_ANIMMODEL_POOL(2000, 32, GR2_ANIMMODEL_POOL::NEW);

typedef GR2_POOL<gr2Mesh> GR2_MESH_POOL;
static GR2_MESH_POOL _gr2MeshPool = GR2_MESH_POOL(2500, 32, GR2_MESH_POOL::NEW);

typedef GR2_POOL<gr2Animation> GR2_ANIMATION_POOL;
static GR2_ANIMATION_POOL _gr2AnimationPool = GR2_ANIMATION_POOL(2500);

typedef GR2_POOL<gr2AnimationBlend> GR2_BLEND_POOL;
static GR2_BLEND_POOL _gr2BlendPool = GR2_BLEND_POOL(1500);

typedef GR2_POOL<gr2ControlBlend> GR2_CONBLEND_POOL;
static GR2_CONBLEND_POOL _gr2BlendControlPool = GR2_CONBLEND_POOL(300);

typedef GR2_POOL<gr2Control> GR2_CONTROL_POOL;
static GR2_CONTROL_POOL _gr2ControlPool;

typedef GR2_POOL<gr2QueuedEvent> GR2_QEVENT_POOL;
static GR2_QEVENT_POOL _gr2QueuedEventPool;

typedef GR2_POOL<gr2Event> GR2_EVENT_POOL;
static GR2_EVENT_POOL _gr2EventPool;

typedef std::map<std::string,granny_file_info*> GR2_FILE_CACHE;
static GR2_FILE_CACHE _fileCache;

#define		UNUSED_TRACK_VALUE	-999.0

// cache of granny animation to event lists
static GR2_ANIM_EVENTS _animEvents;

// end blending stuff


typedef std::pair<granny_mesh*,granny_skeleton*> MeshDataKey;
typedef std::map<MeshDataKey,gr2MeshData*> MeshDataMap;
static MeshDataMap _meshDataMap;

// end rotation matrix crap
// local data
static gr2Scene localScene;
//static gr2FileCacheElement	fileCache[MAX_GR2_FILECACHE_SIZE];
//static int gr2FileCacheSize;

//static int	defaultInfantryMaterialID;

static piMATRIX		arbitraryRotationMatrix;

#ifdef BETA_RELEASE
static LOGICAL colTestFlag;
static piVECTOR	lastColPt;
static piVECTOR lastColLineP1, lastColLineP2;
static LOGICAL animOutput;
#endif

// prototypes
static void createMesh(gr2Mesh* Mesh, granny_mesh* GrannyMesh, granny_skeleton* wornOn);
static void createMeshData(granny_mesh* GrannyMesh, granny_skeleton* mySkeleton, granny_skeleton* wornOnSkeleton, bool wantNormals);
static void createMeshTextures(gr2MeshData* data, bool wantNormals);

static SINT32 createTexture(granny_texture* texture);
static void loadTextures(granny_file_info* fileInfo, const char* path);

static const char* getBaseName(const char* path, const char* prePath = nullptr);

static granny_file_info* getFileInfo(const char* basename, SINT32 useCache, granny_file** file = nullptr);
static void gr2Error(granny_log_message_type type, granny_log_message_origin origin, char const* file, granny_int32x line, char const* error, void* userData);
static gr2AnimationBlend* gr2LoadBlendedAnimationXML(const SCHAR* blendfile, SINT32 boneCount);
static gr2AnimationBlend* gr2LoadBlendedAnimationTXT(SCHAR* blendfile, SINT32 boneCount);

static XML_TAG_HANDLER(xmlBlendTagHandler);
static XML_TAG_HANDLER(xmlBlendAnimTagHandler);
static XML_TAG_HANDLER(xmlBlendBoneTagHandler);

#include "libxml/parser.h"
#include "libxml/tree.h"
#include <map>

struct TextureReplacement
{
    std::map<std::string, std::vector<std::string> > fromto;

};

static std::map<std::string, TextureReplacement *> _replacements;

#if(0)
static SINT32 xmlinputmatch(const SCHAR* name) {return 1;}
#endif

static void* _workBuffer(size_t size)
{
	// Round up to the next 1k block
	size = (size + 1023) & ~1023;

	static void* workingBuffer = malloc(size);
	static size_t workingSize = size;

	if(size > workingSize)
	{
		workingBuffer = realloc(workingBuffer, (size | 1023) + 1);
		workingSize = size;
	}

	return workingBuffer;
}

#if(0)
static void _parseReplacements(xmlNodePtr root, TextureReplacement* r)
{
    xmlNodePtr node = root;

	for(; node != nullptr; node = node->next)
	{
        if(node->type != XML_ELEMENT_NODE)
        {
            continue;
        }

        if(!xmlStrcmp((xmlChar*)"replace", node->name))
        {
            xmlChar* key = xmlGetProp(node, (xmlChar*)"name");

            // CreateTexture removes extension, so do it here too.
            char* p = (char*)strchr((const char*)key, '.');

            if(p)
            {
                *p = 0;
            }

            xmlNodePtr child = node->children;
            std::vector<std::string> with;

            with.clear();

            for(; child != nullptr; child = child->next)
            {
                if(child->type != XML_ELEMENT_NODE)
                {
                    continue;
                }

                if(!xmlStrcmp((xmlChar*)"with", child->name))
                {
                    xmlChar* texreplace = xmlNodeGetContent(child);

                    // CreateTexture removes extension, so do it here too.
                    char* p = (char*)strchr((const char*)texreplace, '.');

                    if(p)
                    {
                        *p = 0;
                    }

                    with.push_back((const char*)texreplace);
                    xmlFree(texreplace);
                }
            }

            r->fromto.insert(std::make_pair((const char*)key, with));

            xmlFree(key);
        }
    }
}
#endif

#if(0)
static void _properties(xmlNodePtr root)
{
    xmlNodePtr node = root;

	for(; node != nullptr; node = node->next)
	{
        if(node->type != XML_ELEMENT_NODE)
        {
            continue;
        }

        if(!xmlStrcmp((xmlChar*)"for", node->name))
        {
            xmlChar* key = xmlGetProp(node, (xmlChar*)"name");
            TextureReplacement* r = new TextureReplacement;

            _parseReplacements(node->children, r);

            _replacements[(const char*)key] = r;
            xmlFree(key);
        }
    }
}
#endif

#if(0)
static void _loadReplaceXML()
{
    xmlRegisterInputCallbacks(xmlinputmatch,
                             (xmlInputOpenCallback)fioOpen,
                             (xmlInputReadCallback)fioRead,
                             (xmlInputCloseCallback)fioClose);

    SCHAR* name = "gr2Data/TPS/meshes/infset/replace.xml";

    xmlDocPtr doc = xmlReadFile(name, nullptr, XML_PARSE_DTDLOAD | XML_PARSE_RECOVER);

    if(doc)
    {
	    xmlNodePtr root = xmlDocGetRootElement(doc);

        if(!root)
        {
            Write_Log(LOG_OLD, LOG_INFO, "*** %s FAILED ***", name);
            return;
        }

        if(!xmlStrcmp((xmlChar*)"infset", root->name))
        {
            _properties(root->children);
        }

	    xmlFreeDoc(doc);
    }

    xmlCleanupInputCallbacks();
}
#endif

#ifdef BETA_RELEASE
#if GR2_MEMORY_PAD
static void _byteAlignCheck(size_t size, const char* type)
{
	if(size % GR2_MEMORY_MULTIPLE > 0)
	{
		char buf[256];
		snprintf(buf, 255, "%s is not byte aligned: size(%ld) %% multiple(%d) = %ld bytes leftover", type, size, GR2_MEMORY_MULTIPLE, (size % GR2_MEMORY_MULTIPLE));
		piExitProgram(_FL, buf);
	}
}
#endif//GR2_MEMORY_PAD
#endif//BETA_RELEASE

void gr2Init (void)
{
	granny_log_callback Callback;
//	FLOAT32		threshold;

/*	if(!GrannyVersionsMatch)
	{
		piExitProgram(_FL, "The Granny DLL currently loaded doesn't match the .h file used during compilation");
	}*/

	Callback.Function = gr2Error;
	Callback.UserData = 0;
	GrannySetLogCallback(&Callback);

	memset (&localScene, 0, sizeof (gr2Scene));

	localScene.SharedLocalPose = GrannyNewLocalPose(MAX_GR2_BONE_COUNT);

	//defaultInfantryMaterialID = piFindMaterialName ("forest1");

	// threshold defaults to 0.2
//	threshold = GrannyGetLocalPoseFillThreshold(localScene.SharedLocalPose);
//	Write_Log(LOG_FPS, LOG_INFO, "Threshold:  %f", threshold);

	generateRotationMatrix (AROUND_Z, 90.0, &arbitraryRotationMatrix);

#ifdef REPLACE_MODEL_TEXTURES
    _loadReplaceXML();
#endif

#ifdef BETA_RELEASE
#if GR2_MEMORY_PAD
	_byteAlignCheck(sizeof(gr2Model), "gr2Model");
	_byteAlignCheck(sizeof(gr2Mesh), "gr2Mesh");
	_byteAlignCheck(sizeof(gr2Animation), "gr2Animation");
	_byteAlignCheck(sizeof(gr2AnimationBlend), "gr2AnimationBlend");
	_byteAlignCheck(sizeof(gr2ControlBlend), "gr2ControlBlend");
	_byteAlignCheck(sizeof(gr2Control), "gr2Control");
	_byteAlignCheck(sizeof(gr2QueuedEvent), "gr2QueuedEvent");
	_byteAlignCheck(sizeof(gr2Event), "gr2Event");
#endif //GR2_MEMORY_PAD
#endif //BETA_RELEASE

    fpsmaterial = new oglMaterial;

#if USE_STATESET
    depthpass = new RenderStateSet;
    depthpass->SetRenderState(States::GetColorMaskFalse());
    depthpass->SetRenderState(new Program("opaquedepth"));
    depthpass->SetRenderState(new ShadeModel(GL_FLAT));
    depthpass->SetRenderState(new FrontFace(GL_CW));
    depthpass->SetMode(GL_LIGHTING, 0);
    depthpass->SetMode(GL_FOG, 0);
#endif
}

static void
gr2Error(granny_log_message_type type, granny_log_message_origin origin, char const* file, granny_int32x line, char const* error, void* userData)
{
  if ( error == (const char*)2 || line == 0 )
  {
    error="";
  }
	Write_Log(LOG_FPS, LOG_INFO, "GRANNY: %d, %d, %s line %d: \"%s\"", type, origin, file, line, error);
}

/*
*
* NOTE:  useCache should eventually be always on, as without it, we lose the ability to release a file's resources
*
*/

granny_file_info* findFileInCache(const char* basename, granny_file** file = nullptr)
{
	GR2_FILE_CACHE::iterator found = _fileCache.find(basename);

	if(found != _fileCache.end())
	{
		//Write_Log (LOG_FPS, LOG_INFO, "Fileinfo FROM CACHE for file %s = 0x%x", gr2File, found->second);
		return found->second;
	}

	// if we're here, it's a miss

	// load the file
	granny_file_info* fileInfo = getFileInfo(basename, 0, (file) ? &(*file) : nullptr);

	// add it to the cache, even if its null
	_fileCache[basename] = fileInfo;

	//	Write_Log (LOG_FPS, LOG_INFO, "Fileinfo LOADED for file %s = 0x%x", gr2File, fileInfo);

	// return it
	return fileInfo;
}

granny_file_info* getFileInfo(const char* basename, SINT32 useCache, granny_file** file)
{
	granny_file_info*	fileInfo;
	granny_file*		loadedFile;
	size_t				fileSize, readSize;
	void*				buffer;
	SINT32				stream;
	SCHAR				filename[255];

	if(useCache)
	{
		fileInfo = findFileInCache(basename, (file) ? &(*file) : nullptr);
	}
	else
	{
		snprintf(filename, 255, "%s%s%s", GR2DATA_PREPATH, basename, GRANNY_EXTENSION);

		// some troopers have regional specific models
		const char* regional = fioGetRegionalPath(filename);

		fileSize = fioFileSize(regional);

		if(static_cast<int>(fileSize) < 0)
		{
			Write_Log (LOG_FPS, LOG_INFO, "getFileInfo:  Failed to size file %s (probably file not found)!", regional);
			return nullptr;
		}

		stream = fioOpen(regional);

		if(stream < 0)
		{
			Write_Log (LOG_FPS, LOG_INFO, "getFileInfo:  Failed to open %s!", regional);
			return nullptr;
		}

		buffer = _workBuffer(fileSize);

		if(!buffer)
		{
			Write_Log (LOG_FPS, LOG_INFO, "getFileInfo:  Failed to allocate read buffer for %s", regional);
			return nullptr;
		}

		readSize = fioRead(stream, buffer, fileSize);

		if(readSize != fileSize)
		{
			Write_Log (LOG_FPS, LOG_INFO, "getFileInfo:  Bytes read != bytes in file (%d != %d) in file %s", readSize, fileSize, regional);
			memFree (buffer);
			return nullptr;
		}

		loadedFile = GrannyReadEntireFileFromMemory(fileSize, buffer);

		fioClose(stream);

//		Don't free buffer, its a workBuffer
//		memFree (buffer);

		fileInfo = GrannyGetFileInfo(loadedFile);

		if(!fileInfo)
		{
			Write_Log (LOG_FPS, LOG_INFO, "getFileInfo:  Failed to get model data out of %s!", regional);
			GrannyFreeFile(loadedFile);
			return nullptr;
		}

		// if we requested the granny file, return it too
		if(file)
			*file = loadedFile;

		// load any textures in this granny file
		loadTextures(fileInfo, basename);
	}

	return fileInfo;
}

void createNormalTexture(granny_texture* texture)
{
    if(!texture)
    {
        return;
    }

    SCHAR original[64] = {0};
    const granny_pixel_layout* layout = nullptr;
    piSURFACEDESC pisd;

    memset(&pisd, 0, sizeof(piSURFACEDESC));

    char* p1 = (char*)strrchr(texture->FromFileName, '\\');
    char* p2 = (char*)strstr(texture->FromFileName, "_normal");

    if(!p1 || !p2)
    {
        return;
    }

    p1++;

    strncpy(original, p1, p2 - p1);
	// The non-normal texture is put to lower case...so we do it here too.
	_strlwr(original);

    SINT32 id = piGetNamedTexture(original);

    if(!piIsTexture(id, 0))
    {
        return;
    }

    TEXTURE* t = piGetTextureData(id);

    if(piIsTexture(t->normalMap, 0))
    {
        piAddTexRef(t->normalMap);
        return;
    }

    SINT32 w = texture->Width;
    SINT32 h = texture->Height;

    t->normalMap = piCreateRawTexture(-1, 0, w, h, 24, 0, CRT_FLAG_NONE);

    strcat(original, "_normal");
    piSetTextureName(t->normalMap, original);

    piLockTextureMemorySurface(t->normalMap, 0, &pisd);

#if platOS == OS_WIN32
    layout = GrannyBGR888PixelFormat;
#else
    layout = GrannyRGB888PixelFormat;
#endif

    UINT8* data = (UINT8*)_workBuffer(3 * texture->Width * texture->Height);

    GrannyCopyTextureImage(texture, 0, 0, layout, texture->Width, texture->Height, texture->Width * 3, data);

    ldpixCopyBitsToSurface(data, 0, texture->Width, texture->Height, &pisd, colorMAGENTA);

    piUnlockTextureMemorySurface(t->normalMap, 0);

//	Do not free - it's a workerBuffer
//	memFree(data);

    piGetTextureHandle(t->normalMap, 0);
}

void createSpecularTexture(granny_texture* texture)
{
    if(!texture)
    {
        return;
    }

    SCHAR original[64] = {0};
    const granny_pixel_layout* layout = nullptr;
    piSURFACEDESC pisd;

    memset(&pisd, 0, sizeof(piSURFACEDESC));

    char* p1 = (char*)strrchr(texture->FromFileName, '\\');
    char* p2 = (char*)strstr(texture->FromFileName, "_spec");

    if(!p1 || !p2)
    {
        return;
    }

    p1++;

    strncpy(original, p1, p2 - p1);
	// The non-normal texture is put to lower case...so we do it here too.
	_strlwr(original);

    SINT32 id = piGetNamedTexture(original);

    if(!piIsTexture(id, 0))
    {
        return;
    }

    TEXTURE* t = piGetTextureData(id);

    if(piIsTexture(t->specularMap, 0))
    {
        piAddTexRef(t->specularMap);
        return;
    }

    SINT32 w = texture->Width;
    SINT32 h = texture->Height;

    t->specularMap = piCreateRawTexture(-1, 0, w, h, 24, 0, CRT_FLAG_NONE);

    strcat(original, "_spec");
    piSetTextureName(t->specularMap, original);

    piLockTextureMemorySurface(t->specularMap, 0, &pisd);

#if platOS == OS_WIN32
    layout = GrannyBGR888PixelFormat;
#else
    layout = GrannyRGB888PixelFormat;
#endif

    UINT8* data = (UINT8*)_workBuffer(3 * texture->Width * texture->Height);

    GrannyCopyTextureImage(texture, 0, 0, layout, texture->Width, texture->Height, texture->Width * 3, data);

    ldpixCopyBitsToSurface(data, 0, texture->Width, texture->Height, &pisd, colorMAGENTA);

    piUnlockTextureMemorySurface(t->specularMap, 0);

//	Don't free data! It's a work buffer.
//	memFree(data);

    piGetTextureHandle(t->specularMap, 0);
}

gr2Model* gr2LoadModel(const char* modelFile, gr2Model* wornOnModel, SINT32* boneCount, SINT32 instantiate, int modelIndex, bool wantNormals)
{
	// file path
	static char			path[255+1];
	char*				at = nullptr;

	strncpy(path, modelFile, 255);

	at = strchr(path, '@');

	if(at)
	{
		*at = 0;

		if(modelIndex < 0)
			modelIndex = atoi(++at);
	}

	if(modelIndex < 0)
		modelIndex = 0;

	granny_file* file = nullptr;
	granny_file_info* fileInfo = getFileInfo(path, TRUE, &file);

	if(!fileInfo)
	{
		char errMsg[255];
		sprintf (errMsg, "gr2LoadModel: failed to load model %s", modelFile);
		piExitProgram (_FL, errMsg);
	}

	//if (fileInfo->ModelCount != 1)
	if(fileInfo->ModelCount <= 0)
	{
		Write_Log (LOG_FPS, LOG_INFO, "gr2LoadModel: Model file %s doesn't contain any models .. it has %d", modelFile, fileInfo->ModelCount);
		return nullptr;
	}

	if(modelIndex < 0 || modelIndex >= fileInfo->ModelCount)
	{
		Write_Log (LOG_FPS, LOG_INFO, "gr2LoadModel: Model file %s doesn't contain a model at index %d .. it has %d models", modelFile, modelIndex, fileInfo->ModelCount);
		return nullptr;
	}

	granny_model* grannyModel = fileInfo->Models[modelIndex];

	if(boneCount)
		*boneCount = grannyModel->Skeleton->BoneCount;

	int meshCount = grannyModel->MeshBindingCount;

	// file is only set when the file is first loaded
	if(file)
	{
		// cache all models and textures in this gr2 file
		for(int mo = 0; mo < fileInfo->ModelCount; mo++)
		{
			granny_model* m = fileInfo->Models[mo];

			if(m->MeshBindingCount)
			{
				granny_skeleton* wos = (wornOnModel) ? wornOnModel->skeleton : m->Skeleton;

				for(int me = 0; me < m->MeshBindingCount; ++me)
					createMeshData(m->MeshBindings[me].Mesh, m->Skeleton, wos, wantNormals);
			}
		}

		if(fileInfo->TextureCount > 0)
			GrannyFreeFileSection(file, GrannyStandardTextureSection);

		GrannyFreeFileSection(file, GrannyStandardDiscardableSection);
	}

	// if we don't want to instantiate (i.e. just load the cache) we're done, just have to set the # of bones
	if(!instantiate)
		return nullptr;

	gr2Model* newModel = _gr2ModelPool.Pop();

	if(!newModel)
	{
		Write_Log (LOG_FPS, LOG_INFO, "gr2LoadModel: Failed to allocate memory for model %s", modelFile);
		piExitProgram (_FL, "gr2LoadModel: Failed to allocate memory for model");
		return nullptr;
	}

	newModel->reset();

	newModel->name			= modelFile;
	newModel->grannyModel	= grannyModel;

	if (grannyModel->Skeleton->BoneCount > MAX_GR2_BONE_COUNT)
	{
		Write_Log (LOG_FPS, LOG_INFO, "gr2LoadModel: Bone count too high (%d > %d) for %s", grannyModel->Skeleton->BoneCount, MAX_GR2_BONE_COUNT, modelFile);
		piExitProgram (_FL, "gr2LoadModel: Bonecount too high");
		return nullptr;
	}

	newModel->GrannyInstance = GrannyInstantiateModel(grannyModel);
	newModel->WorldPose = GrannyNewWorldPose(grannyModel->Skeleton->BoneCount);
	newModel->skeleton = GrannyGetSourceSkeleton(newModel->GrannyInstance);
	newModel->boneCount = newModel->skeleton->BoneCount;
	newModel->active = 1;

	Write_Log (LOG_FPS, LOG_INFO, "Granny2:  Instance for %s is 0x%x", modelFile, newModel->GrannyInstance);

	granny_skeleton* wornOnSkeleton = nullptr;

	if (wornOnModel == nullptr)
	{
		newModel->usesAnotherSkeleton = 0;
		newModel->wornOnModel = nullptr;
		wornOnSkeleton = newModel->skeleton;
	}
	else
	{
		newModel->usesAnotherSkeleton = 1;
		newModel->wornOnModel = wornOnModel;
		wornOnSkeleton = wornOnModel->skeleton;
	}

	/* Now, I loop through all the meshes in the model and process
	them. */
	if (meshCount)
	{
		for(int meshIndex = 0; meshIndex < meshCount; ++meshIndex)
		{
			gr2Mesh* mesh = _gr2MeshPool.Pop();

			if(mesh)
			{
				mesh->reset();
				newModel->meshes.push_back(mesh);
				createMesh(mesh, grannyModel->MeshBindings[meshIndex].Mesh, wornOnSkeleton);
			}
		}
	}

	newModel->attachToModel = nullptr;
	newModel->attachBone = 0;
	newModel->attachToBone = 0;
	//newModel->attachOffset = (float*)IdentityMatrix;

	return newModel;
}

static SINT32 createTexture (granny_texture* grannyTexture)
{
	SCHAR* ptr;
	SINT32 texid;
	const granny_pixel_layout* layout;
	piSURFACEDESC	pisd;
	UINT8* imgData;


	granny_texture_image* grannyImage = &grannyTexture->Images[0];

	if (grannyImage->MIPLevelCount > 1)
	{
		Write_Log (LOG_FPS, LOG_INFO, "createTexture: Multiple MIP levels detected for image %s", grannyTexture->FromFileName);
	}

	strcpy(texname, grannyTexture->FromFileName);
	_strlwr(texname);
	ptr = strrchr(texname, '\\');
	if (ptr != nullptr)
	{
		ptr += 1;
		strcpy(texname, ptr);
	}

	ptr = strrchr(texname, '.');
	if (ptr != nullptr)
	{
		*ptr = 0;
	}

	if (strlen(texname) == 0)
	{
		Write_Log(LOG_FPS, LOG_INFO, "Animation texture has no name! (w=%d,h=%d)", grannyTexture->Width, grannyTexture->Height);
//		rnmainListText(loclGetLocalizationStringByID(loclANIMATION_TEXTURE_MSG));
	}

	// search to see if we're about to load/instance a texture we'll need uncompressed
	// in this case, the patch textures
    SINT32 num = 0;

#ifdef USE_NEW_DECALS
    num = NUM_INFANTRY_TYPES * 3;
#else
    num = NUM_INFANTRY_TYPES;
#endif

	// add in our texture data path
	std::string path = GR2_TEXTURE_PATH;
	path += texname;

	// not bounded...ugh
	strcpy(texname, path.c_str());

	texid = piGetNamedTexture(texname);

	if (piIsTexture(texid, 0))
	{
		Write_Log (LOG_FPS, LOG_INFO, "createTexture: Using existing game texture %s", texname);
		piAddTexRef(texid);
	}
	else
	{
		Write_Log (LOG_FPS, LOG_INFO, "createTexture: Creating game texture %s", texname);
		// some hardcoding here .. 24 bytes per pixel, 0 alpha bits

		texid = piCreateRawTexture(-1, 0, grannyTexture->Width, grannyTexture->Height, 24, 0, CRT_FLAG_WRITABLE);

		if (!piIsTexture(texid, 0))
		{
			piExitProgram (_FL, "createTexture: failed to piCreateRawTexture");
		}

		piSetTextureName(texid, texname);

		if (!piLockTextureMemorySurface(texid, 0, &pisd))
		{
			piExitProgram (_FL, "createTexture: failed to piLockTextureMemorySurface");
		}

#if platOS == OS_WIN32
		layout = GrannyBGR888PixelFormat;
#else
		layout = GrannyRGB888PixelFormat;
#endif
		imgData = (UINT8*)_workBuffer(3*grannyTexture->Width*grannyTexture->Height);

		if (!imgData)
		{
			piExitProgram (_FL, "createTexture: failed to allocate temporary memory for texture image");
		}

		if (!GrannyCopyTextureImage(grannyTexture, 0, 0, layout, grannyTexture->Width, grannyTexture->Height, grannyTexture->Width*3, imgData))
		{
			piExitProgram (_FL, "createTexture: failed in GrannyCopyTextureImage");
		}

		if (!ldpixCopyBitsToSurface(imgData, 0, grannyTexture->Width, grannyTexture->Height, &pisd, colorMAGENTA))
		{
			piExitProgram (_FL, "createTexture: failed in ldpixCopyBitsToSurface");
		}

		piUnlockTextureMemorySurface(texid, 0);

//		Don't free imgData - it's a work buffer
//		memFree (imgData);

#if defined(OPENGL3D)
		// Force the texture to be loaded into video memory
		// This prevents the stutters seen in-game by loading the texture early
		if (piGetTextureHandle(texid, 0) < 0)
		{
			piFreeMipTextures(texid);
			texid = -1;
		}
#endif

	}

	return texid;
	// used to handle materials at this stage, but it's now handled at render time
}

static void createMesh(gr2Mesh* mesh, granny_mesh* grannyMesh, granny_skeleton* wornOn)
{
	MeshDataKey key = std::make_pair(grannyMesh, wornOn);
	MeshDataMap::iterator found = _meshDataMap.find(key);

	if(found != _meshDataMap.end())
	{
		mesh->data = found->second;
		mesh->active = 1;
	}
	else
	{
		mesh->active = 0;
	}
}

static void createMeshData(granny_mesh* grannyMesh, granny_skeleton* mySkeleton, granny_skeleton* wornOnSkeleton, bool wantNormals)
{
	MeshDataKey key = std::make_pair(grannyMesh, wornOnSkeleton);
	MeshDataMap::iterator found = _meshDataMap.find(key);

	// if we already have one, bail
	if(found != _meshDataMap.end())
		return;

	gr2MeshData* data = new gr2MeshData;

	SINT32 indexCount	= 0;
	SINT32 vertexCount	= 0;

	data->GrannyMesh = grannyMesh;

	data->GrannyBinding = GrannyNewMeshBinding(grannyMesh, mySkeleton, wornOnSkeleton);

	data->rigidMesh = 0;

	indexCount = GrannyGetMeshIndexCount(grannyMesh);

	data->indexArray = nullptr;

	if(indexCount)
	{
		data->indexArray = new SINT16[indexCount];

		/* ... and I copy them in.  The GrannyCopyMeshIndices routine
		can do arbitrary bytes-per-index conversion, so the 2 just says
		"make sure it's 2-byte (16-bit) indices".  If it was stored
		in the file as 16-bit indices, it's a block copy, but if it wasn't,
		it does index-by-index conversion. */

		GrannyCopyMeshIndices(grannyMesh, 2, data->indexArray);
	}

    /* Now I process the vertex data. */
	vertexCount = GrannyGetMeshVertexCount(grannyMesh);

	data->vertexArray = nullptr;
    data->pngt = nullptr;

	if(vertexCount)
		data->vertexArray = new piGVERTEX[vertexCount];

	if(GrannyMeshIsRigid(grannyMesh))
	{
		data->rigidMesh = 1;

		if(vertexCount)
            GrannyCopyMeshVertices(grannyMesh, GrannyPNT332VertexType, data->vertexArray);

		// Since the mesh is rigid, I won't	be needing a deformer.
        data->GrannyDeformer = nullptr;
	}
	else
	{
        data->GrannyDeformer = GrannyNewMeshDeformer(GrannyGetMeshVertexType(grannyMesh), GrannyPNGT3332VertexType, GrannyDeformPositionNormalTangent
#if GRANNY_VERSION(2,7)
			,GrannyDontAllowUncopiedTail
#endif
		);

        if(!data->GrannyDeformer)
        {
            // No tangent data
            data->GrannyDeformer = GrannyNewMeshDeformer(GrannyGetMeshVertexType(grannyMesh), GrannyPNT332VertexType, GrannyDeformPositionNormal
#if GRANNY_VERSION(2,7)
			,GrannyDontAllowUncopiedTail
#endif
			);
        }
        else
        {
			//Has tangent data for normal mapping
			data->pngt = new piTVERTEX[vertexCount];
        }

        if(!data->GrannyDeformer)
		{
			Write_Log(LOG_FPS, LOG_INFO, "Granny didn't find a matching deformer for the vertex format used by mesh \"%s\".  This mesh won't be rendered properly.",
				grannyMesh->Name);
		}
	}

#if USE_STATESET
    static FrontFace* frontface = new FrontFace(GL_CW);

    data->stateset = new RenderStateSet[grannyMesh->MaterialBindingCount];

    for(int i = 0; i < grannyMesh->MaterialBindingCount; i++)
    {
        data->stateset[i].SetRenderState(frontface);
    }
#endif

	createMeshTextures(data, wantNormals);

	// save our shared data
	_meshDataMap[key] = data;
}

#if(0)
static void createMeshOld(gr2Model* model, gr2MeshData* mesh, granny_mesh* grannyMesh, granny_skeleton* mySkeleton, granny_skeleton* wornOnSkeleton)
{
	SINT32 indexCount;
	SINT32 vertexCount;

	mesh->GrannyMesh = grannyMesh;

	mesh->GrannyBinding = GrannyNewMeshBinding(grannyMesh, mySkeleton, wornOnSkeleton);

	mesh->rigidMesh = 0;
	//mesh->active = 1;

	indexCount = GrannyGetMeshIndexCount(grannyMesh);

	mesh->indexArray = nullptr;

	if(indexCount)
	{
		mesh->indexArray = new SINT16[indexCount];

		/* ... and I copy them in.  The GrannyCopyMeshIndices routine
		can do arbitrary bytes-per-index conversion, so the 2 just says
		"make sure it's 2-byte (16-bit) indices".  If it was stored
		in the file as 16-bit indices, it's a block copy, but if it wasn't,
		it does index-by-index conversion. */

		GrannyCopyMeshIndices(grannyMesh, 2, mesh->indexArray);
	}

    /* Now I process the vertex data. */
	vertexCount = GrannyGetMeshVertexCount(grannyMesh);

	mesh->vertexArray = nullptr;
    mesh->pngt = nullptr;

	if (vertexCount)
	{
		mesh->vertexArray = new piGVERTEX[vertexCount];
	}

	if(GrannyMeshIsRigid(grannyMesh))
	{
		mesh->rigidMesh = 1;

		if (vertexCount)
		{
            GrannyCopyMeshVertices(grannyMesh, GrannyPNT332VertexType, mesh->vertexArray);
		}

		// Since the mesh is rigid, I won't	be needing a deformer.
        mesh->GrannyDeformer = nullptr;
	}
	else
	{
        mesh->GrannyDeformer = GrannyNewMeshDeformer(GrannyGetMeshVertexType(grannyMesh), GrannyPNGT3332VertexType, GrannyDeformPositionNormalTangent
#if GRANNY_VERSION(2,7)
			,GrannyDontAllowUncopiedTail
#endif
		);

        if(!mesh->GrannyDeformer)
        {
            // No tangent data
            mesh->GrannyDeformer = GrannyNewMeshDeformer(GrannyGetMeshVertexType(grannyMesh), GrannyPNT332VertexType, GrannyDeformPositionNormal
#if GRANNY_VERSION(2,7)
			,GrannyDontAllowUncopiedTail
#endif
			);
        }
        else
        {
            //Has tangent data for normal mapping
            mesh->pngt = new piTVERTEX[vertexCount];
        }

        if(!mesh->GrannyDeformer)
		{
			Write_Log(LOG_FPS, LOG_INFO, "Granny didn't find a matching deformer for the vertex format used by mesh \"%s\".  This mesh won't be rendered properly.",
				grannyMesh->Name);
		}
	}

	//initMeshTextures(model, mesh);
}
#endif

gr2Animation* gr2LoadAnimation (const SCHAR* animationFile, SINT32 useCache)
{
	granny_file_info*	fileInfo;
	gr2Animation*		newAnimation;
	SCHAR				buf[255];

	fileInfo = getFileInfo(animationFile, useCache);

	if (!fileInfo || fileInfo->AnimationCount < 1)
	{
		Write_Log (LOG_FPS, LOG_INFO, "gr2LoadAnimation: Failed to load animation %s", animationFile);
		return nullptr;
	}

//	Write_Log (LOG_FPS, LOG_INFO, "Fileinfo for animation file %s = 0x%x", animationFile, fileInfo);

	newAnimation = _gr2AnimationPool.Pop();

	if (!newAnimation)
	{
		Write_Log (LOG_FPS, LOG_INFO, "gr2LoadAnimation: Failed to allocate memory for animation %s", animationFile);
		piExitProgram (_FL, "gr2LoadAnimation: Failed to allocate memory for animation");
		return nullptr;
	}

	newAnimation->animation = fileInfo->Animations[0];
	newAnimation->events	= nullptr;

	const SCHAR* fileName = nullptr;
	const char* slash = strchr(animationFile, '\\');

	while(slash)
	{
		fileName = slash + 1;
		slash = strchr(slash+1, '\\');
	}

	if(!fileName)
		fileName = animationFile;

	strncpy(newAnimation->name, fileName, MAX_GR2_PATH_SIZE);
//	newAnimation->animType = GR2_ANIMATION_GENERIC;

	// now load the events data from the .evt file if it exists
	// FIXME - really should make it so that these don't get reloaded for every instance of
	// the same animation
	GR2_ANIM_EVENTS::iterator foundEvents = _animEvents.find(newAnimation->animation);

	if(foundEvents == _animEvents.end())
	{
		SCHAR evtFile[255];

		snprintf(evtFile, 255, "%s%s%s", GR2DATA_PREPATH, animationFile, ".evt");

		if(fioAccess(evtFile))
		{
			SINT32 evtFileID = fioOpen(evtFile);

			if(evtFileID >= 0)
			{
				GR2_EVENT_LIST events;

				gr2Event* newEvent = nullptr;

		//		Write_Log (LOG_FPS, LOG_INFO, "gr2LoadAnimation: Loading event data for animation %s", animationFile);

				while(fioGetLine(evtFileID, buf, 255) != 0)
				{
					newEvent = _gr2EventPool.Pop();

					if(!newEvent)
						piExitProgram (_FL, "gr2LoadAnimation: Failed to allocate memory for event");

					sscanf(buf, "%d %f %32s", &newEvent->eventType, &newEvent->eventTime, newEvent->eventData);

					events.push_back(newEvent);

					if(events.size() >= MAX_NUM_ANIM_EVENTS)
						break;
				}

				_animEvents[newAnimation->animation] = events;

				foundEvents = _animEvents.find(newAnimation->animation);

				fioClose(evtFileID);
			}
		}
	}

	if(foundEvents != _animEvents.end())
		newAnimation->events = &foundEvents->second;

	return newAnimation;
}

void gr2DoneLoadingModels (void)
{
	// nothing until the cache shows up later
	// this is where I can close file sections I no longer need
}

LOGICAL gr2AttachModel (gr2Model* model, const SCHAR* attachBone, gr2Model* attachToModel, const SCHAR* attachToBone)
{
	granny_int32	boneIndex;

	if (!model)
	{
		Write_Log (LOG_FPS, LOG_INFO, "Null model passed to gr2AttachModel!");
		return FALSE;
	}

	bool attached = true;

	model->attachToModel = attachToModel;

	if(!attachBone)
	{
		model->attachBone = 0;
	}
	else if(GrannyFindBoneByName(model->skeleton, attachBone, &boneIndex))
	{
		model->attachBone = boneIndex;
	}
	else
	{
		Write_Log (LOG_FPS, LOG_INFO, "Bone %s not found in model %s", attachBone, model->grannyModel->Name);
		//piExitProgram (_FL, "gr2LoadModel: Failed to find bone for model");
		attached = false;
	}

	if(GrannyFindBoneByName(attachToModel->skeleton, attachToBone, &boneIndex))
	{
		model->attachToBone = boneIndex;
	}
	else
	{
		Write_Log (LOG_FPS, LOG_INFO, "Bone %s not found in model %s", attachToBone, attachToModel->grannyModel->Name);
		//piExitProgram (_FL, "gr2AttachModel: Failed to find bone for attach model");
		attached = false;
	}

	if(!attached)
	{
		gr2DetachModel(model);
		return FALSE;
	}
	else
	{
		model->updateAttachment = 1;
	}

	return TRUE;
}

LOGICAL gr2AttachModelByIndex (gr2Model* model, int attachBone, gr2Model* attachToModel, int attachToBone)
{
	if (!model)
	{
		Write_Log (LOG_FPS, LOG_INFO, "Null model passed to gr2AttachModel!");
		return FALSE;
	}

	model->attachToModel	= attachToModel;
	model->attachBone		= attachBone;
	model->attachToBone		= attachToBone;

	return TRUE;
}

void gr2AttachmentUpdate(gr2Model* model)
{
	GrannyGetAttachmentOffset(model->skeleton, model->attachBone, localScene.SharedLocalPose, 0, (float*)&model->attachOffset
#if GRANNY_VERSION(2,7)
		,0,0
#endif
	);
}

void gr2DetachModel (gr2Model* model)
{
	model->attachToModel = nullptr;
	model->attachBone = 0;
	model->attachToBone = 0;
}

void gr2UpdateAttachment(gr2Model* model)
{
	if(model)
		model->updateAttachment = 1;
}

gr2Control* gr2PlayAnimation (gr2Model* model, gr2Animation* animation)
{
	granny_control*	control;
	gr2Control*		newControl;
	granny_controlled_animation_builder*	builder;

	if (!model)
	{
		Write_Log (LOG_FPS, LOG_INFO, "ERROR:  Null model passed to gr2PlayAnimation!");
		return nullptr;
	}

	if (!animation)
	{
		Write_Log (LOG_FPS, LOG_INFO, "ERROR:  Null animation passed to gr2PlayAnimation!");
		return nullptr;
	}

	if (!(animation->animation))
	{
		Write_Log (LOG_FPS, LOG_INFO, "ERROR:  Null gr2 animation passed to gr2PlayAnimation!");
		return nullptr;
	}

//	control = GrannyPlayControlledAnimation((frametime / 1000.0f), animation->animation, model->GrannyInstance);
	builder = GrannyBeginControlledAnimation((frametime / 1000.0f), animation->animation);
	GrannySetTrackGroupTarget (builder, 0, model->GrannyInstance);
	control = GrannyEndControlledAnimation(builder);

	GrannySetControlLoopCount(control, 0);

	newControl = _gr2ControlPool.Pop();

    if (!newControl)
	{
		piExitProgram (_FL, "gr2PlayAnimation: Failed to allocate memory for animation");
		return nullptr;
	}

	newControl->control = control;
	newControl->model = model;
	newControl->easeOutDuration = 0;
	newControl->easeInDuration = 0;
	newControl->animation = animation;

//	Write_Log (LOG_FPS, LOG_INFO, "Generated control 0x%x for animation 0x%x", newControl->control, animation);

//	Write_Log (LOG_FPS, LOG_INFO, "Playing (%s) on (%s) at %f", animation->name, model->name, (frametime / 1000.0f));

	return newControl;
}

LOGICAL gr2PlayBlendedAnimation (gr2Model* model, gr2AnimationBlend* blend, bool allowResurrect)
{
	return gr2PlayBlendedAnimationFull (model, blend, 1.0f, 1.0f, allowResurrect);
}

LOGICAL gr2PlayBlendedAnimationFull (gr2Model* model, gr2AnimationBlend* blend, FLOAT32 easeInTime, FLOAT32 easeOutTime, bool allowResurrect)
{
	if ((!blend) || (!blend->animations[0]) || (!blend->numBlends))
	{
		if (!blend)
		{
			Write_Log (LOG_FPS, LOG_INFO, "gr2PlayBlendedAnimation: Null blend!");
			return FALSE;
		}
		if (!blend->animations[0])
		{
			Write_Log (LOG_FPS, LOG_INFO, "gr2PlayBlendedAnimation: no animations in blend!");
			return FALSE;
		}
		if (!blend->numBlends)
		{
			Write_Log (LOG_FPS, LOG_INFO, "gr2PlayBlendedAnimation: numBlends = 0");
			return FALSE;
		}
	}

#ifdef ALLOW_RESURRECT
	if(allowResurrect && model->blends)
	{
		GR2_BLEND_MAP::iterator found = model->blends->find(blend);

		if (found != model->blends->end())
		{
			// we still have an old instance of this animation running (easing out) .. let's use it
			gr2BlendControlResurrect (model, blend, easeInTime);

			// nothing else to do ..
			return TRUE;
		}
	}
#endif

	Write_Log (LOG_FPS, LOG_INFO, "gr2PlayBlendedAnimation: Starting blend 0x%x (%s) on model %s at %f", blend, blend->blendFile, model->name, (frametime/1000.0f));

	granny_controlled_animation_builder* builder		= nullptr;
	granny_control	* control								= nullptr;
	gr2Control* newControl									= nullptr;
	gr2ControlBlend* blendControl							= _gr2BlendControlPool.Pop();

	if(!model->blends)
		model->blends = new GR2_BLEND_MAP;

	(*model->blends)[blend] = blendControl;

	blendControl->active		= TRUE;
	blendControl->startTime		= frametime;
	blendControl->killTime		= 0;
	blendControl->killCallback	= nullptr;
	blendControl->killData		= nullptr;
	blendControl->modifier		= 0.0f;
	blendControl->startWeight	= 1.0f;

	for (int i = 0; i < blend->numBlends; i++)
	{
		if(!blend->animations[i])
			continue;

		builder = GrannyBeginControlledAnimation((frametime / 1000.0f), blend->animations[i]->animation);
		GrannySetTrackGroupTarget(builder, 0, model->GrannyInstance);
		GrannySetTrackGroupModelMask(builder, 0, blend->animations[i]->mask);
		control = GrannyEndControlledAnimation(builder);

		GrannySetControlLoopCount(control, 0);

		// blends don't allocate memory for controls, so grab a pointer
		newControl = &(blendControl->controls[i]);

		newControl->control = control;
		newControl->model = model;
		newControl->easeOutDuration = 0;
		newControl->easeInDuration = 0;
		newControl->animation = blend->animations[i];

		//if (blend->animations[i]->animType == GR2_ANIMATION_BASELOOP)
		//{
			gr2AnimationSyncBaseLoop (model, blend, newControl);
		//}
	}

	gr2BlendControlSetEaseInDuration (model, blend, easeInTime);
	gr2BlendControlSetEaseOutDuration (model, blend, easeOutTime);

	gr2UpdateBlendEvents(model);

	return TRUE;
}

// searches through the model for an existing version of the animation being controlled by the provided
// control, and if it finds one, modifies the control so that it is "in sync" with the currently running
// animation.  this should help with the foot-shifts
void gr2AnimationSyncBaseLoop (gr2Model* model, gr2AnimationBlend* blend, gr2Control* inControl)
{
	if ((model == nullptr) || (model->blends == nullptr) || (inControl == nullptr) || (inControl->animation == nullptr))
		return;

	// searching by actual granny animation address .. the gr2Animation*s are unique (due to masks and such)
	granny_animation* searchAnim = inControl->animation->animation;

	GR2_BLEND_MAP::iterator it = model->blends->begin();

	for(; it != model->blends->end(); ++it)
	{
		if(it->first == blend)
			continue;

		gr2ControlBlend* blendControl = it->second;

		for (int j = 0; j < GR2_MAX_ANIMS_PER_BLEND; j++)
		{
			// a little crash protection
			if (!blendControl->controls[j].animation)
				continue;

			if (blendControl->controls[j].animation->animation == searchAnim)
			{
				granny_control* foundControl		= blendControl->controls[j].control;
				granny_real32 syncTime			= GrannyGetControlRawLocalClock(foundControl);

				GrannySetControlRawLocalClock (inControl->control, syncTime);
				//return;
			}
		}
	}
}

// returns the # of the running blend, or -1 if not found or error
gr2ControlBlend* gr2BlendControlFindRunningBlend (gr2Model* model, gr2AnimationBlend* animationBlend)
{
	if (!model)
	{
		Write_Log (LOG_FPS, LOG_INFO, "gr2BlendControlFindRunningBlend: Null model passed");
		return nullptr;
	}

	if (!model->blends)
	{
		Write_Log (LOG_FPS, LOG_INFO, "gr2BlendControlFindRunningBlend: model has no blends");
		return nullptr;
	}

	if ((!animationBlend) || (!animationBlend->animations[0])  || (!animationBlend->numBlends))
	{
		Write_Log (LOG_FPS, LOG_INFO, "gr2BlendControlFindRunningBlend: Null data passed");
		return nullptr;
	}

	GR2_BLEND_MAP::iterator found = model->blends->find(animationBlend);

	if(found != model->blends->end())
		return found->second;

	return nullptr;
}

void gr2BlendControlSetMixin (gr2Model* model, gr2AnimationBlend* animationBlend)
{
	gr2ControlBlend* blendControl = gr2BlendControlFindRunningBlend(model, animationBlend);

	if (!blendControl)
	{
		Write_Log (LOG_FPS, LOG_INFO, "gr2BlendControlSetMixin: failed to find control!");
		return;
	}

	blendControl->mixin = TRUE;
}

void gr2BlendControlSetModifier(gr2Model* model, gr2AnimationBlend* animationBlend, FLOAT32 weight, FLOAT32 modifier)
{
	gr2ControlBlend* blendControl = gr2BlendControlFindRunningBlend(model, animationBlend);

	if(!blendControl)
		return;

	blendControl->startWeight = weight;
	blendControl->modifier = modifier;
}

void gr2BlendControlSetWeight (gr2Model* model,
							   gr2AnimationBlend* animationBlend,
							   FLOAT32 weight1,
							   FLOAT32 weight2,
							   FLOAT32 weight3,
							   FLOAT32 weight4)
{
	gr2ControlBlend* blendControl = gr2BlendControlFindRunningBlend(model, animationBlend);

	if(!blendControl)
	{
		/*
		if (animationBlend)
			Write_Log (LOG_FPS, LOG_INFO, "gr2BlendControlSetWeight: failed to find control %s!", animationBlend->blendFile);
		else
			Write_Log (LOG_FPS, LOG_INFO, "gr2BlendControlSetWeight: Null animationBlend");
		*/

		return;
	}

//	Write_Log (LOG_FPS, LOG_INFO, "gr2BlendControlSetWeight: Blend %s, weight %f", animationBlend->blendFile, weight);

	UINT32 blendCount = animationBlend->numBlends;

	switch(animationBlend->type)
	{
		case gr2AnimationBlend::ARBITRARY:
		case gr2AnimationBlend::TRANSITION:
		{
			// should only ever have up to 4 animations to blend together
			if(blendCount > 0) gr2ControlSetWeight(&blendControl->controls[0], weight1);
			if(blendCount > 1) gr2ControlSetWeight(&blendControl->controls[1], weight2);
			if(blendCount > 2) gr2ControlSetWeight(&blendControl->controls[2], weight3);
			if(blendCount > 3) gr2ControlSetWeight(&blendControl->controls[3], weight4);
			if(blendCount > 4) gr2ControlSetWeight(&blendControl->controls[4], 0.0f);
		}
		break;
		case gr2AnimationBlend::BASE:
		{
			FLOAT32 baseWeight = (weight1 || weight2 || weight3 || weight4) ? 1.0f : 0.0f;

			// first animation is the base, others are weighted against it
			if(blendCount > 0) gr2ControlSetWeight(&blendControl->controls[0], baseWeight);
			if(blendCount > 1) gr2ControlSetWeight(&blendControl->controls[1], weight1);
			if(blendCount > 2) gr2ControlSetWeight(&blendControl->controls[2], weight2);
			if(blendCount > 3) gr2ControlSetWeight(&blendControl->controls[3], weight3);
			if(blendCount > 4) gr2ControlSetWeight(&blendControl->controls[4], weight4);
		}
		break;
		default: // everything else
		{
			for(UINT32 i = 0; i < blendCount; ++i)
				gr2ControlSetWeight(&blendControl->controls[i], weight1);
		}
		break;
	}
}

void gr2BlendControlSetLastFrame (gr2Model* model, gr2AnimationBlend* animationBlend )
{
	gr2ControlBlend* blendControl = gr2BlendControlFindRunningBlend(model, animationBlend);

	if (!blendControl)
	{
		if (animationBlend)
			Write_Log (LOG_FPS, LOG_INFO, "gr2BlendControlSetLastFrame: failed to find control %s!", animationBlend->blendFile);
		else
			Write_Log (LOG_FPS, LOG_INFO, "gr2BlendControlSetLastFrame: Null animationBlend");
		return;
	}

	for (int i = 0; i < animationBlend->numBlends; i++)
	{
		gr2ControlSetLastFrame (&blendControl->controls[i]);
	}
}


void gr2BlendControlSetLoopCount (gr2Model* model, gr2AnimationBlend* animationBlend, SINT32 count)
{
	gr2ControlBlend* blendControl = gr2BlendControlFindRunningBlend(model, animationBlend);

	if (!blendControl)
	{
		if (animationBlend)
			Write_Log (LOG_FPS, LOG_INFO, "gr2BlendControlSetLoopCount: failed to find control %s!", animationBlend->blendFile);
		else
			Write_Log (LOG_FPS, LOG_INFO, "gr2BlendControlSetLoopCount: Null animationBlend");
		return;
	}

	blendControl->loopCount = count;

	for (int i = 0; i < animationBlend->numBlends; i++)
	{
		gr2ControlSetLoopCount (&blendControl->controls[i], count);
	}
}

// this one is specific for grenades (for now) ..
// sets the loop count for everything BUT the first animation .. which is the looping one
// needed so that grenades can be thrown while moving yet allowing the legs to keep moving
void gr2BlendControlSetLoopCountForGrenade (gr2Model* model, gr2AnimationBlend* animationBlend, SINT32 count)
{
	gr2ControlBlend* blendControl = gr2BlendControlFindRunningBlend(model, animationBlend);

	if (!blendControl)
	{
		if (animationBlend)
			Write_Log (LOG_FPS, LOG_INFO, "gr2BlendControlSetLoopCountForGrenade: failed to find control %s!", animationBlend->blendFile);
		else
			Write_Log (LOG_FPS, LOG_INFO, "gr2BlendControlSetLoopCountForGrenade: Null animationBlend");
		return;
	}

	blendControl->loopCount = count;

	for (int i = 1; i < animationBlend->numBlends; i++)
	{
		gr2ControlSetLoopCount (&blendControl->controls[i], count);
	}
}

void gr2BlendControlSetSpeed(gr2Model* model, gr2AnimationBlend* animationBlend, FLOAT32 speed, bool reverseBaseOnly)
{
	gr2ControlBlend* blendControl = gr2BlendControlFindRunningBlend(model, animationBlend);

	if (!blendControl)
	{
		/*
		if (animationBlend)
			Write_Log (LOG_FPS, LOG_INFO, "gr2BlendControlSetSpeed: failed to find control %s!", animationBlend->blendFile);
		else
			Write_Log (LOG_FPS, LOG_INFO, "gr2BlendControlSetSpeed: Null animationBlend");
		*/
		return;
	}

	for (int i = 0; i < animationBlend->numBlends; i++)
	{
		// if a negative speed comes in and reverseBaseOnly is true
		// only reverse the first animation; handy for blends where
		// the base (movement) animation is played backwards but we
		// wanted the animations played on top of it to play normally
		// eg: when we throw a grenade while moving backwards
		if(speed < 0 && reverseBaseOnly && i > 0 && animationBlend->type == gr2AnimationBlend::BASE)
			speed = -speed;

		gr2ControlSetSpeed(&blendControl->controls[i], speed);
	}
}

void gr2BlendControlSetEaseOutDuration (gr2Model* model, gr2AnimationBlend* animationBlend, FLOAT32 time)
{
	gr2ControlBlend* blendControl = gr2BlendControlFindRunningBlend(model, animationBlend);

	if (!blendControl)
	{
		if (animationBlend)
			Write_Log (LOG_FPS, LOG_INFO, "gr2BlendControlSetEaseOutDuration: failed to find control %s!", animationBlend->blendFile);
		else
			Write_Log (LOG_FPS, LOG_INFO, "gr2BlendControlSetEaseOutDuration: Null animationBlend");
		return;
	}

	for (int i = 0; i < animationBlend->numBlends; i++)
	{
		gr2ControlSetEaseOutDuration (&blendControl->controls[i], time);
	}

//	Write_Log (LOG_FPS, LOG_INFO, "gr2BlendControlSetEaseOutDuration:  ease OUT time %f for blend %s at %d", time, animationBlend->blendFile, frametime);

	blendControl->easeOutDuration = time;
}

void gr2BlendControlSetEaseInDuration (gr2Model* model, gr2AnimationBlend* animationBlend, FLOAT32 time)
{
	gr2ControlBlend* blendControl = gr2BlendControlFindRunningBlend(model, animationBlend);

	if (!blendControl)
	{
		if (animationBlend)
			Write_Log (LOG_FPS, LOG_INFO, "gr2BlendControlSetEaseInDuration: failed to find control %s!", animationBlend->blendFile);
		else
			Write_Log (LOG_FPS, LOG_INFO, "gr2BlendControlSetEaseInDuration: Null animationBlend");
		return;
	}

	if (time == 0.0)
	{
		for (int i = 0; i < animationBlend->numBlends; i++)
		{
			GrannySetControlEaseIn (blendControl->controls[i].control, false);
			blendControl->controls[i].easeInDuration = 0;
		}
	}
	else
	{
		for (int i = 0; i < animationBlend->numBlends; i++)
		{
			gr2ControlSetEaseInDuration (&blendControl->controls[i], time);
		}
	}

//	Write_Log (LOG_FPS, LOG_INFO, "gr2BlendControlSetEaseInDuration:  ease IN time %f for blend %s at %d", time, animationBlend->blendFile, frametime);

	blendControl->easeInDuration = time;
}

// same as gr2BlendControlSetEaseInDuration .. immediately ease in
void gr2BlendControlEaseIn (gr2Model* model, gr2AnimationBlend* animationBlend, FLOAT32 time)
{
	gr2BlendControlSetEaseInDuration (model, animationBlend, time);
}

// NOT the same as gr2BlendControlSetEaseOutDuration.. this one immediately eases out
void gr2BlendControlEaseOut (gr2Model* model, gr2AnimationBlend* animationBlend, FLOAT32 time)
{
	gr2ControlBlend* blendControl = gr2BlendControlFindRunningBlend(model, animationBlend);

	if (!blendControl)
	{
		if (animationBlend)
			Write_Log (LOG_FPS, LOG_INFO, "gr2BlendControlEaseOut: failed to find control %s!", animationBlend->blendFile);
		else
			Write_Log (LOG_FPS, LOG_INFO, "gr2BlendControlEaseOut: Null animationBlend");
		return;
	}

	for (int i = 0; i < animationBlend->numBlends; i++)
	{
		GrannyEaseControlOut (blendControl->controls[i].control, time);
	}

//	Write_Log (LOG_FPS, LOG_INFO, "gr2BlendControlEaseOut:  ease OUT time %f for blend %s at %d", time, animationBlend->blendFile, frametime);
}

LOGICAL gr2BlendIsComplete (gr2Model* model, gr2AnimationBlend* animationBlend)
{
	gr2ControlBlend* blendControl = gr2BlendControlFindRunningBlend(model, animationBlend);

	if (!blendControl)
	{
		if (animationBlend)
			Write_Log (LOG_FPS, LOG_INFO, "gr2BlendIsControlComplete: failed to find control %s!", animationBlend->blendFile);
		else
			Write_Log (LOG_FPS, LOG_INFO, "gr2BlendIsControlComplete: Null animationBlend");
		return FALSE;
	}

	granny_real32	duration;
	granny_control* control = nullptr;

	for (int i = 0; i < animationBlend->numBlends; i++)
	{
		duration = GrannyGetControlLocalDuration(blendControl->controls[i].control);
		// 1/30th of a second .. 0.0333 .. seems to be "one frame" ..
		if (duration > 0.04)
		{
			// ok, we got one that isn't a single frame, use it
			control = blendControl->controls[i].control;
			break;
		}
	}

	return (GrannyGetControlDurationLeft(control) < 0.0f);
}

FLOAT32 gr2BlendControlCompletePercent (gr2Model* model, gr2AnimationBlend* animationBlend)
{
	gr2ControlBlend* blendControl = gr2BlendControlFindRunningBlend(model, animationBlend);

	if (!blendControl)
	{
		if (animationBlend)
			Write_Log (LOG_FPS, LOG_INFO, "gr2BlendControlCompletePercent: failed to find control %s!", animationBlend->blendFile);
		else
			Write_Log (LOG_FPS, LOG_INFO, "gr2BlendControlCompletePercent: Null animationBlend");
		return -1.0f;
	}

	granny_real32	duration, localclock, percent;

	for (int i = 0; i < animationBlend->numBlends; i++)
	{
		duration = GrannyGetControlLocalDuration(blendControl->controls[i].control);
		// 1/30th of a second .. 0.0333 .. seems to be "one frame" ..
		if (duration > 0.04)
		{
			// ok, we got one that isn't a single frame, use it
			localclock = GrannyGetControlClampedLocalClock (blendControl->controls[i].control);
			percent = localclock/duration;

#ifdef DEBUG
			Write_Log(LOG_FPS, LOG_INFO, "Percent complete %2.2f", percent);
#endif

			return percent;
		}
	}

	return -1.0f;
}

void gr2BlendControlSetStartOffset (gr2Model* model, gr2AnimationBlend* animationBlend, FLOAT32 percent)
{
	gr2ControlBlend* blendControl = gr2BlendControlFindRunningBlend(model, animationBlend);

	if (!blendControl)
	{
		if (animationBlend)
			Write_Log (LOG_FPS, LOG_INFO, "gr2BlendControlSetStartOffset: failed to find control %s!", animationBlend->blendFile);
		else
			Write_Log (LOG_FPS, LOG_INFO, "gr2BlendControlSetStartOffset: Null animationBlend");
		return;
	}

	granny_real32	duration, localclock;

	for (int i = 0; i < animationBlend->numBlends; i++)
	{
		duration = GrannyGetControlLocalDuration(blendControl->controls[i].control);
		// 1/30th of a second .. 0.0333 .. seems to be "one frame" ..
		if (duration > 0.04)
		{
			// ok, we got one that isn't a single frame, use it
			localclock = duration * percent;
			GrannySetControlRawLocalClock (blendControl->controls[i].control, localclock);
			//Write_Log (LOG_FPS, LOG_INFO, "Setting new local clock to %2.2f", localclock);
		}
	}
}

void gr2BlendControlSetEndOffset(gr2Model* model, gr2AnimationBlend* animationBlend, FLOAT32 percent)
{
	gr2ControlBlend* blendControl = gr2BlendControlFindRunningBlend(model, animationBlend);

	if (!blendControl)
	{
		if (animationBlend)
			Write_Log (LOG_FPS, LOG_INFO, "gr2BlendControlSetEndOffset: failed to find control %s!", animationBlend->blendFile);
		else
			Write_Log (LOG_FPS, LOG_INFO, "gr2BlendControlSetEndOffset: Null animationBlend");

		return;
	}

	granny_real32 length, clock;

	for (int i = 0; i < animationBlend->numBlends; i++)
	{
		length = GrannyGetControlLocalDuration(blendControl->controls[i].control);

		// 1/30th of a second .. 0.0333 .. seems to be "one frame" ..
		if (length > 0.04)
		{
			// ok, we got one that isn't a single frame, use it
			// calculate how much time the offset is
			length *= percent;

			// grab when the animation will finish its ease out period
			clock = GrannyEaseControlOut(blendControl->controls[i].control, blendControl->controls[i].easeOutDuration);

			// calculate the new end time
			clock -= length;

			// set the animation's stop time to the new stop time
			GrannyCompleteControlAt(blendControl->controls[i].control, clock);
		}
	}
}


#ifdef BETA_RELEASE
void BBoxVertex (piVECTOR* target, float* Transform, float Cx, float Cy, float Cz)
{
	if (!Transform)
		return;

	target->x = Cx*Transform[0] + Cy*Transform[4] + Cz*Transform[8] + Transform[12];
	target->y = Cx*Transform[1] + Cy*Transform[5] + Cz*Transform[9] + Transform[13];
	target->z = (Cx*Transform[2] + Cy*Transform[6] + Cz*Transform[10] + Transform[14]);
}

void DrawBBox (gr2Model* model, int MeshIndex, int BoneBindingIndex)
{
	piVECTOR p1, p2, p3, p4, p5, p6, p7, p8;
	piVECTOR zeropos;
	granny_world_pose* WorldPose;

	gr2Mesh* currentMesh = model->meshes[MeshIndex];

	granny_bone_binding BoneBinding = currentMesh->data->GrannyMesh->BoneBindings[BoneBindingIndex];

//	int* Index = (int*)GrannyGetMeshIndices(currentMesh->GrannyMesh);

	float* OBBMin = BoneBinding.OBBMin;
	float* OBBMax = BoneBinding.OBBMax;

	int BoneIndex = GrannyGetMeshBindingToBoneIndices(currentMesh->data->GrannyBinding)[BoneBindingIndex];
	float* Transform;

	if (model->usesAnotherSkeleton)
	{
		WorldPose = model->wornOnModel->WorldPose;
	}
	else
	{
		WorldPose = model->WorldPose;
	}

	Transform = GrannyGetWorldPose4x4(WorldPose, BoneIndex);

	if (!Transform)
		return;


	memset (&zeropos, 0, sizeof (piVECTOR));

	BBoxVertex (&p1, Transform, OBBMin[0], OBBMin[1], OBBMin[2]);
	BBoxVertex (&p2, Transform, OBBMin[0], OBBMax[1], OBBMin[2]);
	BBoxVertex (&p3, Transform, OBBMax[0], OBBMax[1], OBBMin[2]);
	BBoxVertex (&p4, Transform, OBBMax[0], OBBMin[1], OBBMin[2]);
	BBoxVertex (&p5, Transform, OBBMin[0], OBBMin[1], OBBMax[2]);
	BBoxVertex (&p6, Transform, OBBMin[0], OBBMax[1], OBBMax[2]);
	BBoxVertex (&p7, Transform, OBBMax[0], OBBMax[1], OBBMax[2]);
	BBoxVertex (&p8, Transform, OBBMax[0], OBBMin[1], OBBMax[2]);

	draw3dLineDebug(&p1, &p2, colorYELLOWALPHA);
	draw3dLineDebug(&p2, &p3, colorYELLOWALPHA);
	draw3dLineDebug(&p3, &p4, colorYELLOWALPHA);
	draw3dLineDebug(&p4, &p1, colorYELLOWALPHA);
	draw3dLineDebug(&p1, &p5, colorYELLOWALPHA);
	draw3dLineDebug(&p2, &p6, colorYELLOWALPHA);
	draw3dLineDebug(&p3, &p7, colorYELLOWALPHA);
	draw3dLineDebug(&p4, &p8, colorYELLOWALPHA);
	draw3dLineDebug(&p5, &p6, colorYELLOWALPHA);
	draw3dLineDebug(&p6, &p7, colorYELLOWALPHA);
	draw3dLineDebug(&p7, &p8, colorYELLOWALPHA);
	draw3dLineDebug(&p8, &p5, colorYELLOWALPHA);
}

LOGICAL gr2RenderDamageModel (gr2Model* currentModel)
{
	UINT32						meshIndex;
	gr2Mesh*						currentMesh;

	for(meshIndex = 0; meshIndex < currentModel->meshes.size();++meshIndex)
	{
		currentMesh = currentModel->meshes[meshIndex];

		for (int bbindex=0; bbindex < currentMesh->data->GrannyMesh->BoneBindingCount; ++bbindex)
		{
			DrawBBox (currentModel, meshIndex, bbindex);
		}
	}

	draw3dLineDebug (&lastColLineP1, &lastColLineP2, colorGREENALPHA);

	draw3dDebugMarkPoint (&lastColPt, colorREDALPHA, 1.0);

	return TRUE;
}

void gr2ToggleAnimOutput (void)
{
	animOutput = !animOutput;
}
#endif // BETA_RELEASE

void gr2RenderShadowModel(gr2Model* model, bool update)
{
    if(!model->active)
        return;

    bool matrixchanged = false;
    granny_matrix_4x4* CompositeBuffer = nullptr;
	piMATRIX world = IdentityMatrix;
    gr2Mesh* mesh = nullptr;

	// either force it or update it if we haven't this frame
	update = (update || model->lastDeformTime != frametime);

    if(gCustomTerrain)
    {
        oglDisableTexturesAboveAndIncluding(0);
        ProgramState::Bind(0);
    }

	for(unsigned int i = 0; i < model->meshes.size(); i++)
    {
        mesh = model->meshes[i];

        if(!mesh->active)
            continue;

		if(update || mesh->data->rigidMesh)
		{
			if(model->usesAnotherSkeleton)
			{
				int count = GrannyGetMeshBindingBoneCount(mesh->data->GrannyBinding);

				if(localScene.compositeTempBuffer == nullptr)
				{
					localScene.compositeTempBufferSize = GrannyGetMeshBinding4x4ArraySize(mesh->data->GrannyBinding, count);
					localScene.compositeTempBuffer = (granny_real32*)memAlloc(localScene.compositeTempBufferSize);
				}
				else if(localScene.compositeTempBufferSize < GrannyGetMeshBinding4x4ArraySize(mesh->data->GrannyBinding, count))
				{
					memFree(localScene.compositeTempBuffer);

					localScene.compositeTempBufferSize = GrannyGetMeshBinding4x4ArraySize(mesh->data->GrannyBinding, count);
					localScene.compositeTempBuffer = (granny_real32*)memAlloc(localScene.compositeTempBufferSize);
				}

				GrannyBuildMeshBinding4x4Array(mesh->data->GrannyBinding,
											   model->wornOnModel->WorldPose,
											   0,
											   count,
											   localScene.compositeTempBuffer);

				CompositeBuffer = (granny_matrix_4x4*)localScene.compositeTempBuffer;
			}
			else // nothing else is worn
			{
				CompositeBuffer = GrannyGetWorldPoseComposite4x4Array(model->WorldPose);
			}

			// Then, I de*erence the index table that maps mesh bone indices
			// to bone indices in the model.
			int* toBoneIndices = (int*)GrannyGetMeshBindingToBoneIndices(mesh->data->GrannyBinding);

			// Next I load the mesh's vertex buffer, or deform
			// into a temporary buffer and load that, depending on
			// whether the mesh is rigid or not.
			int vertexCount = (int)GrannyGetMeshVertexCount(mesh->data->GrannyMesh);

			if(vertexCount <= 0)
			{
				continue;
			}

			world = *oglGetWorldMatrix();

			if(mesh->data->rigidMesh)
			{
				SINT32 boneIndex = (model->attachToModel) ? toBoneIndices[0] : 0;
				granny_matrix_4x4* transform = &CompositeBuffer[boneIndex];

				piMATRIX newMatrix;
				math3dMatrixMultiply(&newMatrix,(piMATRIX*)transform, oglGetWorldMatrix());

				oglSetWorldMatrix(&newMatrix);
				matrixchanged = true;
			}
			else if(mesh->data->GrannyDeformer)
			{
				if(mesh->PNGT())
				{
					GrannyDeformVertices(mesh->data->GrannyDeformer,
											0,
											(FLOAT32*)CompositeBuffer,
											vertexCount,
											GrannyGetMeshVertices(mesh->data->GrannyMesh),
											mesh->PNGT());
				}
				else
				{
					GrannyDeformVertices(mesh->data->GrannyDeformer,
											0,
											(float*)CompositeBuffer,
											vertexCount,
											GrannyGetMeshVertices(mesh->data->GrannyMesh),
											mesh->Vertices());
				}
			}
			else
			{
				// If we got here, we had a bad mesh (the CreateMesh call
				// should've popped up an error message announcing this
				// earlier), so we skip it.
				continue;
			}
		}

		/* Now both the indices and vertices are loaded, so I can
		render.  I grab the material groups and spin over them,
		changing to the appropriate texture and rendering each batch.
		A more savvy rendering loop might have instead built a
		sorted list of material groups to minimize texture changes,
		etc., but this is the most basic way to render. */

		granny_tri_material_group* group = GrannyGetMeshTriangleGroups(mesh->data->GrannyMesh);
		int groupCount = GrannyGetMeshTriangleGroupCount(mesh->data->GrannyMesh);

		while(groupCount--)
		{
			short* indexPtr;
			//void* vertexPtr;

			indexPtr = mesh->data->indexArray;
			indexPtr += 3*group->TriFirst;

			//vertexPtr = mesh->Vertices();

			if(mesh->PNGT())
            {
                ogl::SetVertexPointer(3, GL_FLOAT, sizeof(oglTVERTEX), &mesh->PNGT()->x);
                ogl::DrawElements(GL_TRIANGLES, group->TriCount * 3, GL_UNSIGNED_SHORT, indexPtr);
            }
            else
            {
                ogl::SetVertexPointer(3, GL_FLOAT, sizeof(oglVERTEX), mesh->Vertices());
                ogl::DrawElements(GL_TRIANGLES, group->TriCount * 3, GL_UNSIGNED_SHORT, indexPtr);
            }

		    ++group;
		}

        if(matrixchanged)
        {
            oglSetWorldMatrix(&world);
            matrixchanged = false;
        }
    }

	if(update)
		model->lastDeformTime = (float)frametime;
}

#if USE_STATESET
LOGICAL gr2RenderModel(gr2Model* currentModel, bool update)
{
	if(!currentModel->active)
		return TRUE;

    if(obdrawIsRenderPass(RENDER_REFLECTION))
        return TRUE;

    if(obdrawIsRenderPass(RENDER_FINAL_SHADOW) ||  obdrawIsRenderPass(RENDER_SHADOW_MAP))
    {
        if(!fpFPSIsSwimming() && currentModel->lastUpdateTime)
        {
            gr2RenderShadowModel(currentModel);
        }

        return 1;
    }

    static LOGICAL PrevNormalMaps = -1;

	granny_matrix_4x4*			CompositeBuffer = nullptr;
	UINT32						meshIndex = 0;
	gr2Mesh*						currentMesh = nullptr;
	SINT32*						toBoneIndices = nullptr;
	SINT32						vertexCount = 0;
	SINT32						groupCount = 0;
	granny_tri_material_group*	group = nullptr;

    piMATRIX newMatrix = IdentityMatrix;
	piMATRIX world = IdentityMatrix;

    bool matrixchanged = false;

	// either force it or update it if we haven't this frame
	update = (update || currentModel->lastDeformTime != frametime);

	for(meshIndex = 0; meshIndex < currentModel->meshes.size();++meshIndex)
	{
		currentMesh = currentModel->meshes[meshIndex];

		if(!currentMesh->active)
			continue;

		if(update || currentMesh->data->rigidMesh)
		{
			// wearing code here
			if (currentModel->usesAnotherSkeleton)
			{
				int count = GrannyGetMeshBindingBoneCount (currentMesh->data->GrannyBinding);

				if (localScene.compositeTempBuffer == nullptr)
				{
					localScene.compositeTempBufferSize = GrannyGetMeshBinding4x4ArraySize(currentMesh->data->GrannyBinding, count);
					localScene.compositeTempBuffer = (granny_real32*) memAlloc(localScene.compositeTempBufferSize);
				}
				else if (localScene.compositeTempBufferSize < GrannyGetMeshBinding4x4ArraySize(currentMesh->data->GrannyBinding, count))
				{
					memFree (localScene.compositeTempBuffer);

					localScene.compositeTempBufferSize = GrannyGetMeshBinding4x4ArraySize(currentMesh->data->GrannyBinding, count);
					localScene.compositeTempBuffer = (granny_real32*) memAlloc(localScene.compositeTempBufferSize);
				}

				GrannyBuildMeshBinding4x4Array (currentMesh->data->GrannyBinding,
					currentModel->wornOnModel->WorldPose,
					0,
					count,
					localScene.compositeTempBuffer);

				CompositeBuffer = (granny_matrix_4x4*) localScene.compositeTempBuffer;
			}
			else // nothing else is worn
			{
				CompositeBuffer = GrannyGetWorldPoseComposite4x4Array(currentModel->WorldPose);
			}

			// Then, I de*erence the index table that maps mesh bone indices
			// to bone indices in the model.
			toBoneIndices = (SINT32*)GrannyGetMeshBindingToBoneIndices(currentMesh->data->GrannyBinding);

			// Next I load the mesh's vertex buffer, or deform
			// into a temporary buffer and load that, depending on
			// whether the mesh is rigid or not.
			vertexCount = (SINT32)GrannyGetMeshVertexCount(currentMesh->data->GrannyMesh);

			if(vertexCount <= 0)
			{
				Write_Debug("[gr2RenderModel] *** VERTEX COUNT IS ZERO for %s ***", currentModel->name);
				continue;
			}

			world = *oglGetWorldMatrix();

			if (currentMesh->data->rigidMesh)
			{
	// FIXME!   This needs to be &CompositeBuffer[0] but I'm nervous to change it until it's necessary
				SINT32 boneIndex = (currentModel->attachToModel) ? toBoneIndices[0] : 0;
				granny_matrix_4x4* transform = &CompositeBuffer[boneIndex];

				math3dMatrixMultiply(&newMatrix,(piMATRIX*)transform, oglGetWorldMatrix());

				oglSetWorldMatrix(&newMatrix);
				matrixchanged = true;
			}
			else if(currentMesh->data->GrannyDeformer)
			{
				if(currentMesh->PNGT())
				{
					GrannyDeformVertices(currentMesh->data->GrannyDeformer,
											0,
											(FLOAT32*)CompositeBuffer,
											vertexCount,
											GrannyGetMeshVertices(currentMesh->data->GrannyMesh),
											currentMesh->PNGT());
				}
				else
				{
					GrannyDeformVertices(currentMesh->data->GrannyDeformer,
											0,
											(float*)CompositeBuffer,
											vertexCount,
											GrannyGetMeshVertices(currentMesh->data->GrannyMesh),
											currentMesh->Vertices());
				}
			}
			else
			{
				// If we got here, we had a bad mesh (the CreateMesh call
				// should've popped up an error message announcing this
				// earlier), so we skip it.
				continue;
			}
		}

		/* Now both the indices and vertices are loaded, so I can
		render.  I grab the material groups and spin over them,
		changing to the appropriate texture and rendering each batch.
		A more savvy rendering loop might have instead built a
		sorted list of material groups to minimize texture changes,
		etc., but this is the most basic way to render. */

		group = GrannyGetMeshTriangleGroups(currentMesh->data->GrannyMesh);
		groupCount = GrannyGetMeshTriangleGroupCount(currentMesh->data->GrannyMesh);

        bool useInteriorMaterial = false;
        static ref_ptr<Material> interior;

        if(piIsMaterialInterior(oglGetActiveMaterial()))
        {
            if(!interior.valid())
            {
                interior = new Material(oglFindMaterialName("interior"));
            }

            useInteriorMaterial = true;
        }

		while(groupCount--)
		{
			SINT16* indexPtr;

            RenderStateSet& ss = currentMesh->data->stateset[group->MaterialIndex];

            if(currentMesh->data->is_normal && currentMesh->data->NormalEnabled != g_wantNormalMaps)
            {
                ss.GetRenderState(RS_GLSLPROGRAM)->SetEnabled(g_wantNormalMaps);
                ss.GetRenderState(RS_TEXTURE_UNIT1)->SetEnabled(g_wantNormalMaps);
                ss.GetRenderState(RS_TEXTURE_UNIT2)->SetEnabled(g_wantNormalMaps);
                ss.SetMode(GL_LIGHTING, !g_wantNormalMaps);

                currentMesh->data->NormalEnabled = g_wantNormalMaps;
            }

            ss.Apply();

            if(useInteriorMaterial)
            {
                interior->apply();
                States::HaveAppliedAttribute(RS_MATERIAL);
            }

			indexPtr = currentMesh->data->indexArray;
			indexPtr += 3*group->TriFirst;

            // Having pgnt data does not always mean it's normal mapped.
            if(currentMesh->PNGT())
            {
                oglRenderIndexScene(oglPT_TRIANGLELIST,
                                    oglVT_TVERTEX,
                                    (void*)currentMesh->PNGT(),
                                     vertexCount,
                                     indexPtr,
                                     group->TriCount * 3);
            }
            else
            {
                oglRenderIndexScene(oglPT_TRIANGLELIST,
                                    oglVT_GVERTEX,
                                    (void*)currentMesh->Vertices(),
                                    vertexCount,
                                    (UINT16*) indexPtr,
                                    group->TriCount*3);
            }

		    ++group;
		}

        if(matrixchanged)
        {
            oglSetWorldMatrix(&world);
            matrixchanged = false;
        }
	}

	if(update)
		currentModel->lastDeformTime = (float)frametime;

	return TRUE;
}
#else
LOGICAL gr2RenderModel(gr2Model* currentModel, bool update)
{
	if(!currentModel->active)
		return TRUE;

    if(obdrawIsRenderPass(RENDER_REFLECTION))
        return TRUE;

    if(obdrawIsRenderPass(RENDER_FINAL_SHADOW) ||  obdrawIsRenderPass(RENDER_SHADOW_MAP))
    {
        if(!fpFPSIsSwimming() && currentModel->lastUpdateTime)
        {
            gr2RenderShadowModel(currentModel);
        }

        return 1;
    }

	granny_matrix_4x4*			CompositeBuffer = nullptr;
	UINT32						meshIndex = 0;
	gr2Mesh*						currentMesh = nullptr;
	SINT32*						toBoneIndices = nullptr;
	SINT32						vertexCount = 0;
	SINT32						groupCount = 0;
	granny_tri_material_group*	group = nullptr;

    piMATRIX newMatrix = IdentityMatrix;
	piMATRIX world = IdentityMatrix;

    bool matrixchanged = false;

    oglPushRenderState(oglRENDERSTATE_ALPHABLENDENABLE);
    oglSetRenderState(oglRENDERSTATE_ALPHABLENDENABLE, piALPHABLENDENABLE_DEFAULT);

	// either force it or update it if we haven't this frame
	update = (update || currentModel->lastDeformTime != frametime);

	for(meshIndex = 0; meshIndex < currentModel->meshes.size();++meshIndex)
	{
		currentMesh = currentModel->meshes[meshIndex];

		if(!currentMesh->active)
			continue;

		if(update || currentMesh->data->rigidMesh)
		{
			// wearing code here
			if (currentModel->usesAnotherSkeleton)
			{
				int count = GrannyGetMeshBindingBoneCount (currentMesh->data->GrannyBinding);

				if (localScene.compositeTempBuffer == nullptr)
				{
					localScene.compositeTempBufferSize = GrannyGetMeshBinding4x4ArraySize(currentMesh->data->GrannyBinding, count);
					localScene.compositeTempBuffer = (granny_real32*) memAlloc(localScene.compositeTempBufferSize);
				}
				else if (localScene.compositeTempBufferSize < GrannyGetMeshBinding4x4ArraySize(currentMesh->data->GrannyBinding, count))
				{
					memFree (localScene.compositeTempBuffer);

					localScene.compositeTempBufferSize = GrannyGetMeshBinding4x4ArraySize(currentMesh->data->GrannyBinding, count);
					localScene.compositeTempBuffer = (granny_real32*) memAlloc(localScene.compositeTempBufferSize);
				}

				GrannyBuildMeshBinding4x4Array (currentMesh->data->GrannyBinding,
					currentModel->wornOnModel->WorldPose,
					0,
					count,
					localScene.compositeTempBuffer);

				CompositeBuffer = (granny_matrix_4x4*) localScene.compositeTempBuffer;
			}
			else // nothing else is worn
			{
				CompositeBuffer = GrannyGetWorldPoseComposite4x4Array(currentModel->WorldPose);
			}

			// Then, I de*erence the index table that maps mesh bone indices
			// to bone indices in the model.
			toBoneIndices = (SINT32*)GrannyGetMeshBindingToBoneIndices(currentMesh->data->GrannyBinding);

			// Next I load the mesh's vertex buffer, or deform
			// into a temporary buffer and load that, depending on
			// whether the mesh is rigid or not.
			vertexCount = (SINT32)GrannyGetMeshVertexCount(currentMesh->data->GrannyMesh);

			if(vertexCount <= 0)
			{
				Write_Debug("[gr2RenderModel] *** VERTEX COUNT IS ZERO for %s ***", currentModel->name);
				continue;
			}

			world = *oglGetWorldMatrix();

			if (currentMesh->data->rigidMesh)
			{
	// FIXME!   This needs to be &CompositeBuffer[0] but I'm nervous to change it until it's necessary
				SINT32 boneIndex = (currentModel->attachToModel) ? toBoneIndices[0] : 0;
				granny_matrix_4x4* transform = &CompositeBuffer[boneIndex];

				math3dMatrixMultiply(&newMatrix,(piMATRIX*)transform, oglGetWorldMatrix());

				oglSetWorldMatrix(&newMatrix);
				matrixchanged = true;
			}
			else if(currentMesh->data->GrannyDeformer)
			{
				if(currentMesh->PNGT())
				{
					GrannyDeformVertices(currentMesh->data->GrannyDeformer,
											0,
											(FLOAT32*)CompositeBuffer,
											vertexCount,
											GrannyGetMeshVertices(currentMesh->data->GrannyMesh),
											currentMesh->PNGT());
				}
				else
				{
					GrannyDeformVertices(currentMesh->data->GrannyDeformer,
											0,
											(float*)CompositeBuffer,
											vertexCount,
											GrannyGetMeshVertices(currentMesh->data->GrannyMesh),
											currentMesh->Vertices());
				}
			}
			else
			{
				// If we got here, we had a bad mesh (the CreateMesh call
				// should've popped up an error message announcing this
				// earlier), so we skip it.
				continue;
			}
		}

		/* Now both the indices and vertices are loaded, so I can
		render.  I grab the material groups and spin over them,
		changing to the appropriate texture and rendering each batch.
		A more savvy rendering loop might have instead built a
		sorted list of material groups to minimize texture changes,
		etc., but this is the most basic way to render. */

		group = GrannyGetMeshTriangleGroups(currentMesh->data->GrannyMesh);
		groupCount = GrannyGetMeshTriangleGroupCount(currentMesh->data->GrannyMesh);

		while (groupCount--)
		{
			SINT16* indexPtr;
			//void* vertexPtr;

			SINT32 texID = -1;
            SINT32 matid = -1;

            //... in case interior material is set
            fpsmaterial->SetMaterial(oglGetActiveMaterial());

			if(group->MaterialIndex < (int)currentMesh->data->textureRefs.size())
			{
				texID = currentMesh->data->textureRefs.at(group->MaterialIndex);

                fpsmaterial->AddTexture(0, texID);

                if(!piIsMaterialInterior(oglGetActiveMaterial()))
                {
                    if(group->MaterialIndex < (int)currentMesh->data->materialRefs.size())
                    {
                        matid = currentMesh->data->materialRefs.at(group->MaterialIndex);
                        fpsmaterial->SetMaterial(matid);
                    }
                }
			}

			/*
			if(texID == -1)
			{
				static SINT32 nextPrint = 0;

				if(frametime >= nextPrint)
				{
					uiChatPrintLine("No texture found for: %s", currentModel->name);
					nextPrint = frametime + 2000;
				}
			}
			*/

			indexPtr = currentMesh->data->indexArray;
			indexPtr += 3*group->TriFirst;

			//vertexPtr = currentMesh->Vertices();

            // gr2RenderModel needs cleanup
            if(currentMesh->data->drawcallback)
            {
                currentMesh->data->drawcallback(currentMesh, indexPtr, vertexCount, group);
		    }
			else
			{
                fpsmaterial->Apply();

                // Having pgnt data does not always mean it's normal mapped.
                if(currentMesh->PNGT())
                {
                     oglRenderIndexScene(oglPT_TRIANGLELIST,
                                         oglVT_TVERTEX,
                                         (void*)currentMesh->PNGT(),
                                         vertexCount,
                                         indexPtr,
                                         group->TriCount * 3);
               }
                else
                {
                    if(!oglRenderIndexScene(oglPT_TRIANGLELIST,
                                            oglVT_GVERTEX,
                                            (void*)currentMesh->Vertices(),
                                            vertexCount,
                                            (UINT16*) indexPtr,
                                            group->TriCount*3))
				    {
					    Write_Log (LOG_FPS, LOG_INFO, "gr2RenderModel: piRenderIndexScene failed on mesh %d!", meshIndex);
					    oglPopRenderState(oglRENDERSTATE_ALPHABLENDENABLE);
					    return FALSE;
				    }
                }
			}

		    ++group;
		}

        if(matrixchanged)
        {
            oglSetWorldMatrix(&world);
            matrixchanged = false;
        }
	}

    oglPopRenderState(piRENDERSTATE_ALPHABLENDENABLE);

	if(update)
		currentModel->lastDeformTime = (float)frametime;

	return TRUE;
}
#endif

void gr2FreeModel (gr2Model* model)
{
	if (!model)
		return;

	//Write_Log (LOG_FPS, LOG_INFO, "Freeing Granny Model: %s", model->name);

	if(_gr2ModelPool.Push(model))
		model->reset();
}

void gr2FreeMesh (gr2Mesh* mesh)
{
	if(!mesh)
		return;

	//Write_Log (LOG_FPS, LOG_INFO, "     Freeing Granny Mesh: %s", mesh->GrannyMesh->Name);

	if(_gr2MeshPool.Push(mesh))
		mesh->reset();
}

void gr2FreeQueuedEvent(gr2QueuedEvent* evt)
{
	if(!evt)
		return;

	_gr2QueuedEventPool.Push(evt);
}

/*
static FLOAT64 gr2UpdateTwist(FLOAT64& target, FLOAT64& twist, FLOAT64& speed, FLOAT32 delta, FLOAT64 parentMove = 0, FLOAT32 snap = 0, const char* debug = nullptr)
{
	if (target != twist || parentMove)
	{
		FLOAT64	 diff, move;

		if(snap == 0)
			snap = tpsTroopData.fireSnapUpTime;

		// add out parent's motion to our target so we're in sync
		target += parentMove;
		twist += parentMove;

		gr2NormalizeAngle(target);
		gr2NormalizeAngle(twist);

		diff	= target - twist;
		move = delta * speed / snap;

		if (diff < 0)
			move *= -1.0;

		if (move * move > diff * diff)
			move = diff;

		// compensate for parental movement
		//move += parentMove;

		// adjust our twist
		twist += move;

		gr2NormalizeAngle(twist);

		if(move && debug)
		{
			FLOAT64 tmt	= target;
			FLOAT64 mt	= twist;

			gr2NormalizeAngle(tmt);
			gr2NormalizeAngle(mt);

			//Write_Log (LOG_FPS, LOG_INFO, "Model Twist data:  targ = %f  now = %f  diff = %f  move = %f  time = %f",
			//	tmt, mt, diff, move, deltaTime);
		}

		return move;
	}

	return 0;
}
*/
#if(0)
static FLOAT64 gr2UpdateTwist(FLOAT64& target, FLOAT64& twist, FLOAT64& speed, FLOAT32 delta, FLOAT64 parentMove = 0, FLOAT32 snap = 0, const char* debug = nullptr)
{
	if (target != twist)
	{
		FLOAT64	 diff, move;

		if(snap == 0)
			snap = tpsTroopData.fireSnapUpTime;

		gr2NormalizeAngle(target);

		diff	= target - twist;
		move = delta * speed / snap;

		if (diff < 0)
			move *= -1.0;

		if (move * move > diff * diff)
			twist = target;
		else
			twist += move;

		gr2NormalizeAngle(twist);

		if(move && debug)
		{
			FLOAT64 tmt	= target;
			FLOAT64 mt	= twist;

			gr2NormalizeAngle(tmt);
			gr2NormalizeAngle(mt);

			//Write_Log (LOG_FPS, LOG_INFO, "Model Twist data:  targ = %f  now = %f  diff = %f  move = %f  time = %f",
			//	tmt, mt, diff, move, deltaTime);
		}

		return move;
	}

	return 0;
}
#endif

// tps always reset events so resetEvents defaults to true
// fps stuff resets its own events in fpFPSDraw via gr2DoControlEvents calls
void gr2UpdateModelOld(gr2Model* model, bool force, bool granular)
{
	if(!model)
	{
		Write_Log (LOG_FPS, LOG_INFO, "gr2UpdateModel: *** ERROR *** NULL MODEL");
		return;
	}

	// stos can cause this to hit too early
	if(!localScene.SharedLocalPose)
		return;

	SINT32 boneCount			= model->skeleton->BoneCount;
	granny_real32 gameClock		= frametime / 1000.0f;
	FLOAT32 deltaTime			= gameClock - model->lastUpdateTime;

	// check if we've already updated this frame
	if(!force && !deltaTime)
		return;

//	Write_Log (LOG_FPS, LOG_INFO, "Setting clock for 0x%x to %f", model->GrannyInstance, gameClock);

	GrannySetModelClock(model->GrannyInstance, gameClock);
	GrannySampleModelAnimations(model->GrannyInstance, 0, boneCount, localScene.SharedLocalPose);

	granny_matrix_4x4 finalTransform;

	// attached models can't handle final transforms
	if (model->attachToModel)
	{
		//granny_matrix_4x4	modelTransform;

		//granny_real32*		skeletonPtr;

		// was passing in (float*)model->finalModelTransform
		if(force || model->updateAttachment)
		{
			GrannyGetAttachmentOffset(model->skeleton, model->attachBone, localScene.SharedLocalPose, 0, (float*)&model->attachOffset
#if GRANNY_VERSION(2,7)
				,0,0
#endif
			);

			model->updateAttachment = 0;
		}

		granny_real32* skeletonPtr = (granny_real32*)GrannyGetWorldPose4x4(model->attachToModel->WorldPose, model->attachToBone);

		GrannyColumnMatrixMultiply4x3 ((float*)&finalTransform, (float*)&model->attachOffset, skeletonPtr);
/*
		Write_Log (LOG_FPS, LOG_INFO, "%f %f %f %f\n%f %f %f %f\n%f %f %f %f\n%f %f %f %f\n",
			finalTransform[0][0], finalTransform[0][1], finalTransform[0][2], finalTransform[0][3],
			finalTransform[1][0], finalTransform[1][1], finalTransform[1][2], finalTransform[1][3],
			finalTransform[2][0], finalTransform[2][1], finalTransform[2][2], finalTransform[2][3],
			finalTransform[3][0], finalTransform[3][1], finalTransform[3][2], finalTransform[3][3]);
*/
		GrannyBuildWorldPose(model->skeleton, 0, boneCount, localScene.SharedLocalPose, (float*)&finalTransform, model->WorldPose);
	}
	else
	{
		granny_real32*	finalModelTransformAddress;

		// FIXME - really need to sort all this out
		//
		if (model->rootPositioningBone)
		{
			granny_transform*	rootBoneTransform;
//			granny_transform*	moveBoneTransform;

			rootBoneTransform = GrannyGetLocalPoseTransform(localScene.SharedLocalPose, model->rootPositioningBone);
//			moveBoneTransform = GrannyGetLocalPoseTransform(localScene.SharedLocalPose, model->movePositioningBone);

			if (model->finalModelTransform)
			{
			}
			else
			{
//				generateRotationMatrix (AROUND_Z, 90.0, (piMATRIX*)&finalTransform);
				memcpy (&finalTransform, &arbitraryRotationMatrix, sizeof (piMATRIX));
//				finalTransform[3][0] = moveBoneTransform->Position[0] - rootBoneTransform->Position[0];
//				finalTransform[3][1] = moveBoneTransform->Position[1] - rootBoneTransform->Position[1];
//				finalTransform[3][2] = -rootBoneTransform->Position[2];	// yay, the ubiquitous flipped z axis!
			}

			finalModelTransformAddress = (float*)&finalTransform;
		}
		else if (model->finalModelTransform)
		{
			finalModelTransformAddress = (float*)model->finalModelTransform;
		}
		else
		{
			finalModelTransformAddress = nullptr;
		}

		GrannyBuildWorldPose(model->skeleton, 0, boneCount, localScene.SharedLocalPose, finalModelTransformAddress, model->WorldPose);
	}

	/* Now Model.WorldPose contains the world-space pose of the model, meaning that all of the local transforms have been
	composited into per-bone world-space matrices.  I'm saving this per-model, because I imagine most games will
	want to (since you will want to access it for scripting, collision detection, etc.), but in an extremely memory-
	constrained scenario, I could have written it so the world-space pose is also buffered, and I just render
	right here to save space. */

	model->lastUpdateTime = gameClock;
}

// updated an animated model, handles animations, etc...
void gr2UpdateModel(gr2Model* model, bool force, bool granular)
{
	if(!model)
	{
		Write_Log (LOG_FPS, LOG_INFO, "gr2UpdateModel: *** ERROR *** NULL MODEL");
		return;
	}

	// stos can cause this to hit too early
	if(!localScene.SharedLocalPose)
		return;

	SINT32 boneCount			= model->skeleton->BoneCount;
	granny_real32 gameClock		= frametime / 1000.0f;
	FLOAT32 deltaTime			= gameClock - model->lastUpdateTime;

	// check if we've already updated this frame
	if(!force && !deltaTime)
		return;

	if(model->blends)
	{
		GR2_BLEND_MAP::iterator it = model->blends->begin();
		std::vector<gr2AnimationBlend*> blendsToKill;

		for(; it != model->blends->end(); ++it)
		{
			gr2AnimationBlend* blend = it->first;
			gr2ControlBlend* blendControl = it->second;
			bool done = false;

			// scan for completed controls we can remove
			if (blendControl->killTime && (blendControl->killTime < frametime))
			{
				// the granny version of the control will either already have been freed, or it will free itself once we advance the clock
				// courtesy of the GrannyCompleteControlAt call

				Write_Log (LOG_FPS, LOG_INFO, "gr2UpdateModel: CLEARING blend 0x%x (%s).. %d < %d", blend, blend->blendFile, blendControl->killTime, frametime);

				for (int j = 0; j < blend->numBlends; j++)
				{
					gr2StopControl (&blendControl->controls[j], TRUE);
				}

				if(blendControl->killCallback)
					blendControl->killCallback(blendControl->killData);

				blendsToKill.push_back(blend);
				done = true;
			}
			else if(blendControl->killTime && blendControl->modifier && blendControl->startWeight)
			{
				SINT32 duration = blendControl->killTime - blendControl->startTime;
				SINT32 played	= frametime - blendControl->startTime;

				FLOAT32 newWeight = ((FLOAT32)played / (FLOAT32)duration);

				newWeight *= blendControl->modifier;
				newWeight = (1.0f - newWeight) * blendControl->startWeight;

				if(newWeight < 0)
				{
					blendControl->killTime = frametime;
					newWeight = 0.0f;
				}

				Write_Log (LOG_FPS, LOG_INFO, "gr2UpdateModel: FADING blend 0x%x (%s).. %.2f", blend, blend->blendFile, newWeight);

				gr2BlendControlSetWeight(model, blend, newWeight);
			}

#ifdef BETA_RELEASE
			// since the above can remove a blend, need to check again
			// update event timers
			if (!done && animOutput)
			{
				for (int j = 0; j < blend->numBlends; j++)
				{
					if (GrannyGetControlEffectiveWeight(blendControl->controls[j].control) > 0)
					{
						Write_Log (LOG_FPS, LOG_INFO, "AnimOutput: time (%d) anim (%s)[%d] weights %f %f",
							frametime,
							blend->blendFile,
							j,
							GrannyGetControlEaseCurveMultiplier(blendControl->controls[j].control),
							GrannyGetControlEffectiveWeight(blendControl->controls[j].control)	);
					}
				}
			}
#endif
		}

		if(!blendsToKill.empty())
		{
			// kill the blends that are complete
			for(unsigned int i = 0; i < blendsToKill.size(); ++i)
			{
				it = model->blends->find(blendsToKill[i]);

				if(it != model->blends->end())
				{
					// we don't free the blended animation because that is done at game exit
					gr2FreeBlendedControl(it->second);

					// erase them from our blends
					model->blends->erase(it);
				}
			}

			gr2UpdateBlendEvents(model);
		}
	}

//	Write_Log (LOG_FPS, LOG_INFO, "Setting clock for 0x%x to %f", model->GrannyInstance, gameClock);

	GrannySetModelClock(model->GrannyInstance, gameClock);

#ifdef LOD_ANIMATIONS

	int BoneCountForLOD = GrannyGetBoneCountForLOD(model->skeleton, model->lodError);

	GrannySampleModelAnimationsLOD(model->GrannyInstance, 0, BoneCountForLOD, localScene.SharedLocalPose, model->lodError);
#else
	GrannySampleModelAnimations(model->GrannyInstance, 0, boneCount, localScene.SharedLocalPose);
#endif

	// scan the events to see if any need to be triggered due to elapsed time
	if(granular && !model->events.empty())
		gr2UpdateEvents(model);

	// update twists
	if(!model->twists.empty())
	{
		for(size_t i = 0; i < model->twists.size(); i++)
		{
			gr2BoneTwist& bt = model->twists.at(i);

			// root bone (0) is always used to denote the model rotation (special case)
			if(bt.bone == 0)
			{
				piMATRIX tempMat;

				FLOAT64 finalModelTwist = bt.angle + 90.0f;

				gr2NormalizeAngle(finalModelTwist);

				generateRotationMatrix(AROUND_Z, finalModelTwist, &tempMat);
				gr2SetModelRotation (model, &tempMat);
			}
			else if(bt.angle)
			{
				double axis[3] = {0.0, 0.0, 0.0};
				axis[bt.axis] = 1.0f;

				// add the rotational twist to the bone in the proper axis
				gr2AddAxisRotationToBoneInLocalPose(localScene.SharedLocalPose, bt.bone, axis, mlDEG_TO_RAD(bt.angle));
			}
		}
	}

	/*
	// test for active, effecting controls on the model
	UINT32 controlCount = 0;
	for(granny_model_control_binding* Binding = GrannyModelControlsBegin(model->GrannyInstance);
			Binding != GrannyModelControlsEnd(model->GrannyInstance);
			Binding = GrannyModelControlsNext(Binding))
	{
		granny_control* Control = GrannyGetControlFromBinding(Binding);

		if (GrannyControlIsActive(Control) && !GrannyControlIsComplete(Control) && (GrannyGetControlEffectiveWeight > 0))
		{
			controlCount = 1;
			break;
		}
	}

	if (!controlCount)
	{
		Write_Log (LOG_FPS, LOG_INFO, "No active controls found on model");
	}
	else

	*/

	granny_matrix_4x4	finalTransform;

	// attached models can't handle final transforms
	if (model->attachToModel)
	{
		//granny_matrix_4x4	modelTransform;

		//granny_real32*		skeletonPtr;

		// was passing in (float*)model->finalModelTransform
		if(force || model->updateAttachment)
		{
			GrannyGetAttachmentOffset(model->skeleton, model->attachBone, localScene.SharedLocalPose, 0, (float*)&model->attachOffset
#if GRANNY_VERSION(2,7)
				,0,0
#endif
			);

			model->updateAttachment = 0;
		}

		granny_real32* skeletonPtr = (granny_real32*)GrannyGetWorldPose4x4(model->attachToModel->WorldPose, model->attachToBone);

		GrannyColumnMatrixMultiply4x3 ((float*)&finalTransform, (float*)&model->attachOffset, skeletonPtr);
/*
		Write_Log (LOG_FPS, LOG_INFO, "%f %f %f %f\n%f %f %f %f\n%f %f %f %f\n%f %f %f %f\n",
			finalTransform[0][0], finalTransform[0][1], finalTransform[0][2], finalTransform[0][3],
			finalTransform[1][0], finalTransform[1][1], finalTransform[1][2], finalTransform[1][3],
			finalTransform[2][0], finalTransform[2][1], finalTransform[2][2], finalTransform[2][3],
			finalTransform[3][0], finalTransform[3][1], finalTransform[3][2], finalTransform[3][3]);
*/
#ifdef LOD_ANIMATIONS
		GrannyBuildWorldPoseLOD(model->skeleton,
								0, boneCount,
								0, BoneCountForLOD,
								localScene.SharedLocalPose,
								(float*)&finalTransform,
								model->WorldPose);
#else
		GrannyBuildWorldPose(model->skeleton,
							 0, boneCount,
							 localScene.SharedLocalPose,
							 (float*)&finalTransform,
							 model->WorldPose);
#endif
	}
	else
	{
		granny_real32*	finalModelTransformAddress;

		// FIXME - really need to sort all this out
		//
		if (model->rootPositioningBone)
		{
			granny_transform*	rootBoneTransform;
//			granny_transform*	moveBoneTransform;

			rootBoneTransform = GrannyGetLocalPoseTransform(localScene.SharedLocalPose, model->rootPositioningBone);
//			moveBoneTransform = GrannyGetLocalPoseTransform(localScene.SharedLocalPose, model->movePositioningBone);

			if (model->finalModelTransform)
			{
			}
			else
			{
//				generateRotationMatrix (AROUND_Z, 90.0, (piMATRIX*)&finalTransform);
				memcpy (&finalTransform, &arbitraryRotationMatrix, sizeof (piMATRIX));
//				finalTransform[3][0] = moveBoneTransform->Position[0] - rootBoneTransform->Position[0];
//				finalTransform[3][1] = moveBoneTransform->Position[1] - rootBoneTransform->Position[1];
//				finalTransform[3][2] = -rootBoneTransform->Position[2];	// yay, the ubiquitous flipped z axis!
			}

			finalModelTransformAddress = (float*)&finalTransform;
		}
		else if (model->finalModelTransform)
		{
			finalModelTransformAddress = (float*)model->finalModelTransform;
		}
		else
			finalModelTransformAddress = nullptr;

		/* terrainize feet; doesn't quite work
		piVECTOR nt;
		nt.x = nt.y = nt.z = 0;
		nt.y = 1.0f;

		if (model->leftFootAdjust != 0.0)
		{
			granny_int32x EEBoneIndex = 12;
			piMATRIX tempMat;

			if(gr2GetBonePositionByIndex(model, EEBoneIndex, &tempMat))
			{
				piVECTOR pos;
				pos.x = tempMat._41;
				pos.y = tempMat._42;
				pos.z = tempMat._43 + model->leftFootAdjust;

				GrannyIKUpdate(2, EEBoneIndex, (granny_real32*)&pos,
						  16, model->skeleton, finalModelTransformAddress,
						  localScene.SharedLocalPose, model->WorldPose);
			}
		}

		if (model->rightFootAdjust != 0.0)
		{
			granny_int32x EEBoneIndex = 8;
			piMATRIX tempMat;

			if(gr2GetBonePositionByIndex(model, EEBoneIndex, &tempMat))
			{
				piVECTOR pos;
				pos.x = tempMat._41;
				pos.y = tempMat._42;
				pos.z = tempMat._43 + model->rightFootAdjust;

				GrannyIKUpdate(2, EEBoneIndex, (granny_real32*)&pos,
						  16, model->skeleton, finalModelTransformAddress,
						  localScene.SharedLocalPose, model->WorldPose);
			}
		}
		*/

		// update adjustments
		if(!model->adjustments.empty())
		{
#ifdef LOD_ANIMATIONS
			GrannyBuildWorldPoseNoCompositeLOD(model->skeleton,
											0, boneCount,
											0, BoneCountForLOD,
											localScene.SharedLocalPose,
											finalModelTransformAddress,
											model->WorldPose);
#else
			GrannyBuildWorldPoseNoComposite(model->skeleton,
											0, boneCount,
											localScene.SharedLocalPose,
											finalModelTransformAddress,
											model->WorldPose);
#endif

			//granny_matrix_4x4* WorldMatrices = GrannyGetWorldPose4x4Array(model->WorldPose);

			GR2_ADJUST_LIST::iterator it = model->adjustments.begin();

			for(; it != model->adjustments.end(); it++)
			{
				gr2BoneAdjust& ba = *it; //model->adjustments.at(i);

				if(frametime >= ba.stoptime)
					continue;

				/*
				granny_real32* mat	= (float*)WorldMatrices[ba.bone];

				mat[12] = ba.adjustment.x(); // x
				mat[13] = ba.adjustment.y(); // y
				mat[14] = ba.adjustment.z(); // z
				*/

				//granny_real32[3] Target = { ba.adjustment.x(), ba.adjustment.y(), ba.adjustment.z()};

				granny_int32x EEBoneIndex = ba.bone; //(the index of the end effector bone);
				granny_real32 Target[3] = { ba.adjustment.x(), ba.adjustment.y(), ba.adjustment.z()}; //(where the EE should move to);
				granny_real32 RestrictedPlaneNormal[3] = {0, 0, 1.0f}; //(the normal to the plane of movement of the middle joint);
				granny_skeleton* Skeleton = model->skeleton; //(the same skeleton you passed to GrannyBuildWorldPose);
				//granny_matrix_4x4* ModelTransform = finalModelTransformAddress; //(the same transform you passed to GrannyBuildWorldPose);
				granny_real32* ModelTransform = finalModelTransformAddress; //(the same transform you passed to GrannyBuildWorldPose);
				granny_local_pose* LocalPose = localScene.SharedLocalPose; //(the same local pose you passed to GrannyBuildWorldPose);
				granny_world_pose* WorldPose = model->WorldPose; //(the same world pose you passed to GrannyBuildWorldPose);
				//granny_skeleton* Skeleton = model->skeleton; //(the same skeleton you passed to GrannyBuildWorldPose);

				GrannyIKUpdate2Bone(EEBoneIndex,
									Target,
									RestrictedPlaneNormal,
									Skeleton,
									ModelTransform,
									LocalPose,
									WorldPose,
									0.1f,
									0.5f);

				//if(frametime >= ba.stoptime)
				//	it = model->adjustments.erase(it);
			}

			// build the composites
			//GrannyBuildWorldPoseComposites(model->skeleton, 0, boneCount, model->WorldPose);

			// these get removed after every frame
			//model->adjustments.clear();
		}
		else
		{
#ifdef LOD_ANIMATIONS
			GrannyBuildWorldPoseLOD(model->skeleton,
								0, boneCount,
								0, BoneCountForLOD,
								localScene.SharedLocalPose,
								finalModelTransformAddress,
								model->WorldPose);
#else
			GrannyBuildWorldPose(model->skeleton, 0, boneCount, localScene.SharedLocalPose, finalModelTransformAddress, model->WorldPose);
#endif
		}
	}

	/* Now Model.WorldPose contains the world-space pose of the model, meaning that all of the local transforms have been
	composited into per-bone world-space matrices.  I'm saving this per-model, because I imagine most games will
	want to (since you will want to access it for scripting, collision detection, etc.), but in an extremely memory-
	constrained scenario, I could have written it so the world-space pose is also buffered, and I just render
	right here to save space. */

	/* At the end of each update, I tell Granny it's OK to free any controls that have completed on this model.  That is,
	any control on which I said GrannyCompleteControlAt() to kill it at a specified time.  I haven't done that in this
	sample app, but it's best to always keep it in the loop in case you decide to use GrannyCompleteControlAt() later
	(which you almost certainly will in practice). */
	GrannyFreeCompletedModelControls(model->GrannyInstance);

	/* Note also that the reason GrannyFreeCompletedModelControls is a separate call, and not automatically handled by
	GrannySetModelClock and so forth, is because some applications might set the clock mulitple times per frame,
	sometimes going backwards, in order to do prediction or collision detection and so on.  Obviously, in that case you
	wouldn't want GrannySetModelClock to delete some of your completed controls, since you may be about to set time back
	a few tenths of a second where the controls would've still existed, and hence get wrong results. */

	model->lastUpdateTime = gameClock;
}

UINT32 gr2IsControlActive (gr2Control* gr2Control)
{
	if (!gr2Control)
	{
		piExitProgram (_FL, "gr2IsControlActive: Nullptr Control");
		return FALSE;
	}

	return GrannyControlIsActive(gr2Control->control);
}

UINT32 gr2IsControlComplete (gr2Control* gr2Control)
{
//	SINT32 loopCount, loopIndex;

	if (!gr2Control)
	{
		piExitProgram (_FL, "gr2IsControlComplete: Nullptr Control");
		return FALSE;
	}

//	Write_Log (LOG_FPS, LOG_INFO, "gr2IsControlComplete:  Control 0x%x (%s)  %f", gr2Control->control, gr2Control->animation->name, GrannyGetControlDurationLeft(gr2Control->control));

	return (GrannyGetControlDurationLeft(gr2Control->control) < 0.0f);

//	return GrannyControlIsComplete(gr2Control->control);
}

void gr2FreeAnimation (gr2Animation* anim)
{
	if(!anim)
		return; // why exit game if null?

	/*
	if (!anim)
	{
		piExitProgram (_FL, "gr2FreeAnimation: Nullptr anim");
	}
	*/

	/*
    for(SINT32 i = 0; i < MAX_NUM_ANIM_EVENTS; i++)
    {
        if(anim->events[i])
        {
			_gr2EventPool.Push(anim->events[i]);
            anim->events[i] = nullptr;
        }
    }
	*/

	anim->events = nullptr;

	_gr2AnimationPool.Push(anim);
}

void gr2FreeBlendedAnimation (gr2AnimationBlend* blend)
{
	if (!blend)
		return;

	for (int i = 0; i < blend->numBlends; i++)
	{
		gr2FreeAnimation (blend->animations[i]);
	}

	_gr2BlendPool.Push(blend);
}

void gr2FreeBlendedControl (gr2ControlBlend* blendControl)
{
	if(blendControl)
		_gr2BlendControlPool.Push(blendControl);
}

void gr2StopControl (gr2Control* gr2Control, LOGICAL immediate)
{
//	granny_real32	localclock;
	granny_real32	clockWhenEaseWillFinish;


	if (!gr2Control)
	{
		piExitProgram (_FL, "gr2FreeControl: Null Control");
	}

	if (immediate)
	{
//		Write_Log (LOG_FPS, LOG_INFO, "gr2StopControl: Immediate! Killing control 0x%x", gr2Control->control);
		GrannyFreeControl (gr2Control->control);
		return;
	}
	// check to make sure the ease in has had a chance to finish .. if it hasn't deal with it
/*
	localclock = GrannyGetControlClock (gr2Control->control);

	if (localclock < gr2Control->easeInDuration)
	{
		// ease in isn't done yet
		granny_real32	localTimeLeft;
		granny_real32	easeOutStartTime;

		localTimeLeft = gr2Control->easeInDuration - localclock;

		easeOutStartTime = (frametime / 1000.0f) + localTimeLeft;
		clockWhenEaseWillFinish = easeOutStartTime + gr2Control->easeOutDuration;

		GrannySetControlEaseOutCurve(gr2Control->control, easeOutStartTime, clockWhenEaseWillFinish, 1.0f, 1.0f, 0.0f, 0.0f);
		GrannyCompleteControlAt (gr2Control->control, clockWhenEaseWillFinish);

		Write_Log (LOG_FPS, LOG_INFO, "Delayed control:  current: %f  easeIn: %f  easeOut: %f .. %f, %f", (frametime / 1000.0f), gr2Control->easeInDuration, gr2Control->easeOutDuration, easeOutStartTime, clockWhenEaseWillFinish);
	}
	else if (gr2Control->easeOutDuration > 0.0f)
	{
		clockWhenEaseWillFinish = GrannyEaseControlOut(gr2Control->control, gr2Control->easeOutDuration);
		GrannyCompleteControlAt(gr2Control->control, clockWhenEaseWillFinish);
//		Write_Log (LOG_FPS, LOG_INFO, "Setting control 0x%x to die at %f, currently %f, ease %f", gr2Control->control, ClockWhenEaseWillFinish, (frametime/1000.0f), gr2Control->easeOutDuration);
	}
	else
	{
//		granny_real32 ClockWhenEaseWillFinish = GrannyEaseControlOut(gr2Control->control, gr2Control->easeOutDuration);
//		GrannyCompleteControlAt(gr2Control->control, ClockWhenEaseWillFinish+2.0f);
//		GrannySetControlWeight (gr2Control->control, 0.0f);

//		Write_Log (LOG_FPS, LOG_INFO, "Killing control 0x%x", gr2Control->control);
		GrannyFreeControl (gr2Control->control);
		// setcontrolactive = false
	}
*/

	if (gr2Control->easeOutDuration)
	{
		clockWhenEaseWillFinish = GrannyEaseControlOut(gr2Control->control, gr2Control->easeOutDuration);
		GrannyCompleteControlAt(gr2Control->control, clockWhenEaseWillFinish);
//		Write_Log (LOG_FPS, LOG_INFO, "Setting control 0x%x to die at %f, currently %f, ease %f", gr2Control->control, clockWhenEaseWillFinish, (frametime/1000.0f), gr2Control->easeOutDuration);
	}
	else
	{
//		Write_Log (LOG_FPS, LOG_INFO, "Killing control 0x%x", gr2Control->control);
		GrannyFreeControl (gr2Control->control);
	}
}

void gr2FreeControl (gr2Control* control)
{
	if(!control)
		return;

	gr2StopControl (control, FALSE);

	_gr2ControlPool.Push(control);
}

void gr2ControlSetLastFrame (gr2Control* gr2Control)
{
	if (!gr2Control)
	{
		piExitProgram (_FL, "gr2ControlSetLastFrame: Null Control");
	}

	GrannySetControlRawLocalClock(gr2Control->control, GrannyGetControlLocalDuration(gr2Control->control));
}

void gr2ControlSetLoopCount (gr2Control* gr2Control, SINT32 numLoops)
{
	if (!gr2Control)
	{
		piExitProgram (_FL, "gr2ControlSetLoopCount: Null Control");
	}

	if (numLoops)
	{
//		numLoops++;
//		Write_Log (LOG_FPS, LOG_INFO, "gr2ControlSetLoopCount:  control 0x%x (%s) set to loop %d times", gr2Control->control, gr2Control->animation->name, numLoops);
	}

	GrannySetControlLoopCount (gr2Control->control, numLoops);
}

void gr2ControlSetEaseInDuration (gr2Control* gr2Control, FLOAT32 time)
{
	if (!gr2Control)
	{
		piExitProgram (_FL, "gr2ControlSetEaseInDuration: Null Control");
	}

	gr2Control->easeInDuration = time;

//	GrannySetControlEaseIn(gr2Control->control, true);
	GrannyEaseControlIn (gr2Control->control, time, false);

//	Write_Log (LOG_FPS, LOG_INFO, "gr2ControlSetEaseInDuration:  control 0x%x (%s) set to ease in over %f", gr2Control->control, gr2Control->animation->name, time);

}

void gr2ControlSetEaseOutDuration (gr2Control* gr2Control, FLOAT32 time)
{
	if (!gr2Control)
	{
		piExitProgram (_FL, "gr2ControlSetEaseOutDuration: Null Control");
	}

	gr2Control->easeOutDuration = time;

//	Write_Log (LOG_FPS, LOG_INFO, "gr2ControlSetEaseOutDuration:  control 0x%x (%s) set to ease out over %f", gr2Control->control, gr2Control->animation->name, time);

	// getcontrolduration - how long the entire control will last for
	// getcontroldurationleft (how much longer is left)

//	GrannySetControlEaseOut(gr2Control->control, true);
//	GrannyEaseControlOut (gr2Control->control, time);
}

void gr2ControlSetSpeed (gr2Control* gr2Control, FLOAT32 speed)
{
	if (!gr2Control)
	{
		piExitProgram (_FL, "gr2ControlSetSpeed: Null Control");
	}

	GrannySetControlSpeed (gr2Control->control, speed);
}

void gr2ControlSetWeight (gr2Control* gr2Control, FLOAT32 weight)
{
	if (!gr2Control)
	{
		piExitProgram (_FL, "gr2ControlSetSpeed: Null Control");
	}

	GrannySetControlWeight (gr2Control->control, weight);
}

void gr2Kill (void)
{
	GR2_ANIM_EVENTS::iterator ait = _animEvents.begin();

	for(; ait != _animEvents.end(); ait++)
	{
		for(size_t i = 0; i < ait->second.size(); i++)
			_gr2EventPool.Push(ait->second[i]);
	}

	_animEvents.clear();

	// granny free's its own resources?
	_fileCache.clear();

	// this memory needs to be freed
	MeshDataMap::iterator it = _meshDataMap.begin();

	for(; it != _meshDataMap.end(); it++)
		delete it->second;

	_meshDataMap.clear();

	if (localScene.compositeTempBuffer)
		memFree (localScene.compositeTempBuffer);

	if (localScene.SharedLocalPose)
		GrannyFreeLocalPose (localScene.SharedLocalPose);

	gr2PhysicsKill();

	// manually free our object pools so we can control the order
	_gr2ModelPool.Free();
	_gr2MeshPool.Free();
	_gr2BlendPool.Free();
	_gr2BlendControlPool.Free();
	_gr2AnimationPool.Free();
	_gr2ControlPool.Free();
	_gr2QueuedEventPool.Free();
	_gr2EventPool.Free();
}

void gr2SetModelRotation (gr2Model* model, piMATRIX* matrix)
{
	if (!model->finalModelTransform)
	{
		model->finalModelTransform = new piMATRIX;

		if (!model->finalModelTransform)
		{
			Write_Log (LOG_FPS, LOG_INFO, "gr2SetModelRotation: Failed to allocate memory for final transform");
			piExitProgram (_FL, "gr2SetModelRotation: Failed to allocate memory for final transform");
		}

		*(model->finalModelTransform) = IdentityMatrix;
	}

	// need to transpose these (column major (g2) vs row major (us), I think) (who knows, though, really, with our fubar setup)
/*
	model->finalModelTransform->_11 = matrix->_11;
	model->finalModelTransform->_12 = matrix->_21;
	model->finalModelTransform->_13 = matrix->_31;
	model->finalModelTransform->_21 = matrix->_12;
	model->finalModelTransform->_22 = matrix->_22;
	model->finalModelTransform->_23 = matrix->_32;
	model->finalModelTransform->_31 = matrix->_13;
	model->finalModelTransform->_32 = matrix->_23;
	model->finalModelTransform->_33 = matrix->_33;
*/
	model->finalModelTransform->_11 = matrix->_11;
	model->finalModelTransform->_12 = matrix->_12;
	model->finalModelTransform->_13 = matrix->_13;
	model->finalModelTransform->_21 = matrix->_21;
	model->finalModelTransform->_22 = matrix->_22;
	model->finalModelTransform->_23 = matrix->_23;
	model->finalModelTransform->_31 = matrix->_31;
	model->finalModelTransform->_32 = matrix->_32;
	model->finalModelTransform->_33 = matrix->_33;

//	gr2UpdateModel (model);
}

void gr2GetModelFinalTransform (gr2Model* model, piMATRIX* matrix)
{
	if (!model->finalModelTransform)
	{
		model->finalModelTransform = new piMATRIX;

		if (!model->finalModelTransform)
		{
			Write_Log (LOG_FPS, LOG_INFO, "gr2SetModelRotation: Failed to allocate memory for final transform");
			piExitProgram (_FL, "gr2SetModelRotation: Failed to allocate memory for final transform");
		}

		*(model->finalModelTransform) = IdentityMatrix;
	}

	*matrix = *model->finalModelTransform;
}

int gr2GetBoneCount(gr2Model* model)
{
	return (model) ? model->skeleton->BoneCount: 0;
}


void gr2SetModelPosition (gr2Model* model, piVECTOR* position)
{
	if (!model->finalModelTransform)
	{
		model->finalModelTransform = new piMATRIX;

		if (!model->finalModelTransform)
		{
			Write_Log (LOG_FPS, LOG_INFO, "gr2SetModelRotation: Failed to allocate memory for final transform");
			piExitProgram (_FL, "gr2SetModelRotation: Failed to allocate memory for final transform");
		}

		*(model->finalModelTransform) = IdentityMatrix;
	}

	model->finalModelTransform->m[3][0] = position->x;
	model->finalModelTransform->m[3][1] = position->y;
	model->finalModelTransform->m[3][2] = position->z;

//	gr2UpdateModel (model);
}

void gr2GetModelPosition (gr2Model* model, piVECTOR* position)
{
	if (!position)
		return;

	if (!model->finalModelTransform)
	{
		position->x = 0;
		position->y = 0;
		position->z = 0;
	}
	else
	{
		position->x = model->finalModelTransform->m[3][0];
		position->y = model->finalModelTransform->m[3][1];
		position->z = model->finalModelTransform->m[3][2];
	}
}

gr2Model* gr2GetWornOnModel(gr2Model* model)
{
	return (model->usesAnotherSkeleton) ? model->wornOnModel: model;
}

granny_skeleton* gr2GetSkeleton(gr2Model* model)
{
	return model->skeleton;
}

granny_model_instance*	gr2GetInstance(gr2Model* model)
{
	return model->GrannyInstance;
}

granny_world_pose* gr2GetWorldPose(gr2Model* model)
{
	return model->WorldPose;
}

void gr2SetWorldPose(gr2Model* from, gr2Model* to)
{
	if(from && to)
		GrannyBuildWorldPose(from->skeleton, 0, to->boneCount, localScene.SharedLocalPose, nullptr, to->WorldPose);
}

const char* gr2GetPath(gr2Model* model)
{
	return model->name;
}

void gr2SetModelActive (gr2Model* model, bool active)
{
	if(model)
		model->active = (active) ? 1: 0;
}

LOGICAL gr2IsModelActive (gr2Model* model)
{
	return (model) ? model->active: 0;
}

void gr2SetMeshActive(gr2Model* model, int mesh, bool active)
{
	if(model && mesh >= 0 && mesh < (int)model->meshes.size())
		model->meshes[mesh]->active = (active) ? 1: 0;
}

void gr2SetMeshActiveByName(gr2Model* model, const char* mesh, bool active)
{
	if(!model || !mesh)
		return;

	for(unsigned int i = 0; i < model->meshes.size(); i++)
	{
		if(!model->meshes[i]->data)
			continue;

		if(!stricmp(model->meshes[i]->data->GrannyMesh->Name, mesh))
		{
#ifdef BETA_RELEASE
			bool current = gr2IsMeshActive(model, i);
			Write_Log (LOG_FPS, LOG_INFO, "gr2SetMeshActiveByName: Setting %s at %d from %s to %s", mesh, frametime, (current) ? "true": "false", (active) ? "true": "false");
#endif
			gr2SetMeshActive(model, i, active);
			break;
		}
	}
}

LOGICAL gr2IsMeshActive(gr2Model* model, int mesh)
{
	if(model && mesh >= 0 && mesh < (int)model->meshes.size())
		return model->meshes[mesh]->active;

	return 0;
}

LOGICAL gr2IsMeshActiveByName(gr2Model* model, const char* mesh)
{
	for(unsigned int i = 0; i < model->meshes.size(); i++)
	{
		if(!stricmp(model->meshes[i]->data->GrannyMesh->Name, mesh))
			return gr2IsMeshActive(model, i);
	}

	return 0;
}

LOGICAL gr2GetBonePosition (gr2Model* model, const SCHAR* boneName, piMATRIX* matrix)
{
	granny_int32	boneIndex;
	granny_real32*	boneTransform;

    if(!model)
        return FALSE;

	if(!(GrannyFindBoneByName(model->skeleton, boneName, &boneIndex)))
	{
		char crap[255];

		snprintf (crap, 255, "gr2GetBonePosition: Bone %s not found in model %s", boneName, model->grannyModel->Name);
		Write_Log (LOG_FPS, LOG_INFO, "%s", crap);
		return FALSE;
	}


	boneTransform = GrannyGetWorldPose4x4 (model->WorldPose, boneIndex);
	memcpy (matrix, boneTransform, sizeof (FLOAT32)*16);

	{
		SINT32	i,j;

		for (i=0; i < 4; i++)
		{
			for (j=0; j < 4; j++)
			{
				if (_isnan(matrix->m[i][j]))
				{
					matrix->m[i][j] = IdentityMatrix.m[i][j];
				}
			}
		}
	}
	return TRUE;
}

LOGICAL gr2GetBonePositionByIndex (gr2Model* model, int boneIndex, piMATRIX* matrix)
{
	granny_real32* boneTransform = GrannyGetWorldPose4x4 (model->WorldPose, boneIndex);
	memcpy (matrix, boneTransform, sizeof (FLOAT32)*16);

	{
		SINT32	i,j;

		for (i=0; i < 4; i++)
		{
			for (j=0; j < 4; j++)
			{
				if (_isnan(matrix->m[i][j]))
				{
					matrix->m[i][j] = IdentityMatrix.m[i][j];
				}
			}
		}
	}

	return TRUE;
}

granny_real32*	gr2GetBoneTransform (gr2Model* model, SCHAR* boneName)
{
	granny_int32 boneIndex;

	if(!(GrannyFindBoneByName(model->skeleton, boneName, &boneIndex)))
	{
		char crap[255];

		snprintf (crap, 255, "gr2GetBoneTransform: Bone %s not found in model %s", boneName, model->grannyModel->Name);
		Write_Log (LOG_FPS, LOG_INFO, "%s", crap);
		return nullptr;
	}

	return gr2GetBoneTransformByIndex(model, boneIndex);
}

granny_real32*	gr2GetBoneTransformByIndex (gr2Model* model, int boneIndex)
{
	return GrannyGetWorldPose4x4(model->WorldPose, boneIndex);
}

granny_real32* gr2GetCompositeBoneTransform (gr2Model* model, SCHAR* boneName)
{
	granny_int32 boneIndex;

	if(!(GrannyFindBoneByName(model->skeleton, boneName, &boneIndex)))
	{
		char crap[255];

		snprintf (crap, 255, "gr2GetBoneTransform: Bone %s not found in model %s", boneName, model->grannyModel->Name);
		Write_Log (LOG_FPS, LOG_INFO, "%s", crap);
		return nullptr;
	}

	return gr2GetCompositeBoneTransformByIndex(model, boneIndex);
}

granny_real32* gr2GetCompositeBoneTransformByIndex (gr2Model* model, int boneIndex)
{
	return GrannyGetWorldPoseComposite4x4(model->WorldPose, boneIndex);
}

//
// NOTE:  this only goes one level deep on the attachments .. will not work for a model that's attached to an attached model
//
LOGICAL gr2GetRawBonePosition (gr2Model* model, SCHAR* boneName, piMATRIX* matrix)
{
	piMATRIX	savedTransform;
	LOGICAL		retVal;

	if (model->attachToModel)
	{
		if (!model->attachToModel->finalModelTransform)
		{
			return gr2GetBonePosition (model, boneName, matrix);
		}

		savedTransform = *(model->attachToModel->finalModelTransform);
		*(model->attachToModel->finalModelTransform) = IdentityMatrix;
		gr2UpdateModel (model->attachToModel, true);
		gr2UpdateModel (model, true);
		retVal = gr2GetBonePosition (model, boneName, matrix);
		*(model->attachToModel->finalModelTransform) = savedTransform;
		gr2UpdateModel (model->attachToModel, true);
		gr2UpdateModel (model, true);
	}
	else
	{
		if (!model->finalModelTransform)
		{
			return gr2GetBonePosition (model, boneName, matrix);
		}

		savedTransform = *(model->finalModelTransform);
		*(model->finalModelTransform) = IdentityMatrix;
		gr2UpdateModel (model, true);
		retVal = gr2GetBonePosition (model, boneName, matrix);
		*(model->finalModelTransform) = savedTransform;
		gr2UpdateModel (model, true);
	}

	return retVal;
}

LOGICAL gr2ModelHasBone (gr2Model* model, SCHAR* boneName)
{
	if(!model || !*boneName) return FALSE;

	int boneIndex = 0;
	return GrannyFindBoneByName(model->skeleton, boneName, &boneIndex);
}

LOGICAL gr2ModelHasMesh(gr2Model* model, const char* mesh, int* meshIndex, int* boneIndex)
{
	if (!model) return FALSE;

	gr2Mesh* currentMesh = nullptr;

	for(unsigned int m = 0; m < model->meshes.size(); m++)
	{
		currentMesh = model->meshes[m];

		if(!stricmp(currentMesh->data->GrannyMesh->Name, mesh))
		{
			if(meshIndex)
				*meshIndex = m;

			if(boneIndex)
			{
				for(int bbIndex = 0; bbIndex < currentMesh->data->GrannyMesh->BoneBindingCount; bbIndex++)
					*boneIndex = GrannyGetMeshBindingToBoneIndices(currentMesh->data->GrannyBinding)[bbIndex];
			}

			return TRUE;
		}
	}

	return FALSE;
}

// boneName can be a partial match from the beginning
// if exact is false; all matches are case-insensitive
int gr2ModelFindBone (gr2Model* model, const SCHAR* boneName, LOGICAL exact)
{
	int boneIndex				= -1;
	gr2Model* skeletonModel	= gr2GetWornOnModel(model);

	if(!skeletonModel || !boneName || !*boneName)
		return boneIndex;

	if(exact)
	{
		if(!GrannyFindBoneByNameLowercase(skeletonModel->skeleton, boneName, &boneIndex))
			boneIndex = -1;
	}
	else
	{
		int nameSize = strlen((const char*)boneName);

		// only look for bone names 128 characters or less
		if(nameSize <= 128)
		{
			granny_skeleton* skeleton = skeletonModel->skeleton;

			char buffer[128+1];
			buffer[0] = 0;

			for(int i = 0; i < skeleton->BoneCount; i++)
			{
				if(strlen(skeleton->Bones[i].Name) < (size_t)nameSize)
					continue;

				strncpy(buffer, skeleton->Bones[i].Name, nameSize);
				buffer[nameSize] = 0;

				if(!stricmp(buffer, (const char*)boneName))
				{
					boneIndex = i;
					break;
				}
			}
		}
	}

	return boneIndex;
}

// boneName can be a partial match from the beginning
// if exact is false; all matches are case-insensitive
int gr2ModelFindBones (std::vector<int>& found, gr2Model* model, SCHAR* boneName, LOGICAL exact)
{
	UINT32 count = found.size();
	gr2Model* skeletonModel = gr2GetWornOnModel(model);

	if(!skeletonModel || !boneName || !*boneName)
		return 0;

	if(exact)
	{
		granny_skeleton* skeleton = skeletonModel->skeleton;

		for(int i = 0; i < skeleton->BoneCount; i++)
		{
			if(!stricmp(skeleton->Bones[i].Name, boneName))
				found.push_back(i);
		}
	}
	else
	{
		int nameSize = strlen((const char*)boneName);

		// only look for bone names 128 characters or less
		if(nameSize <= 128)
		{
			granny_skeleton* skeleton = skeletonModel->skeleton;

			char buffer[128+1];
			buffer[0] = 0;

			for(int i = 0; i < skeleton->BoneCount; i++)
			{
				if(strlen(skeleton->Bones[i].Name) < (size_t)nameSize)
					continue;

				strncpy(buffer, skeleton->Bones[i].Name, nameSize);
				buffer[nameSize] = 0;

				if(!stricmp(buffer, (const char*)boneName))
					found.push_back(i);
			}
		}
	}

	return found.size() - count;
}

// Friggen HACK! Some transforms are 0 matrices. Very bad. Ramp is emailing Granny about this
inline bool transformIsBad(float* transform)
{
	return transform[0] == 0.0 &&
           transform[2] == 0.0 &&
           transform[4] == 0.0 &&
           transform[6] == 0.0 &&
           transform[8] == 0.0 &&
           transform[10] == 0.0 &&
           transform[12] == 0.0 &&
           transform[14] == 0.0;
}

LOGICAL gr2CollideRayWithModel (gr2Model* collisionModel, piVECTOR* rayStart, piVECTOR* rayNormal, SINT32* location, piVECTOR* collisionPnt)
{
    unsigned int			meshIndex = 0;
    int bbIndex = 0, currentBoneIndex = 0;
	gr2Mesh*					currentMesh = nullptr;
	granny_bone_binding*		currentBoneBinding = nullptr;
	// oriented bounding box mins & maxs
	float*					OBBMin = nullptr, *OBBMax = nullptr;
	float*					transform = nullptr;
	granny_world_pose*		worldPose = nullptr;
	granny_box_intersection	intersection;

	memset(&intersection, 0, sizeof(granny_box_intersection));

	// don't need to sanity check others as they're checked in caller
    if (!collisionModel || !collisionModel->active)
		return FALSE;

	if (collisionModel->usesAnotherSkeleton)
	{
		worldPose = collisionModel->wornOnModel->WorldPose;
	}
	else
	{
		worldPose = collisionModel->WorldPose;
	}

	for (meshIndex = 0; meshIndex < collisionModel->meshes.size(); meshIndex++)
	{
		currentMesh = collisionModel->meshes[meshIndex];

		for (bbIndex = 0; bbIndex < currentMesh->data->GrannyMesh->BoneBindingCount; bbIndex++)
		{
			currentBoneBinding = &(currentMesh->data->GrannyMesh->BoneBindings[bbIndex]);

			OBBMin = currentBoneBinding->OBBMin;
			OBBMax = currentBoneBinding->OBBMax;

			currentBoneIndex = GrannyGetMeshBindingToBoneIndices(currentMesh->data->GrannyBinding)[bbIndex];

 			transform = GrannyGetWorldPose4x4(worldPose, currentBoneIndex);

            if(transformIsBad(transform))
            {
                continue;
            }

			if(GrannyRayIntersectsBoxAt(transform, OBBMin, OBBMax, (granny_real32*)rayStart, (granny_real32*) rayNormal, &intersection))
			{
				if(fpHitMap[currentBoneIndex] >= 0 && fpHitMap[currentBoneIndex] < FP_HIT_LOCATION_NUM)
				{
					if (fpHitMap[currentBoneIndex] == FP_HIT_LOCATION_NONE)
						Write_Log (LOG_FPS, LOG_INFO, "gr2CollideRayWithModel: Bone %d invalid hit location", bbIndex);
#ifdef BETA_RELEASE
					if (colTestFlag)
					{
						if (fpHitMap[currentBoneIndex] == FP_HIT_LOCATION_TORSO) 	{
							uiChatPrintLine (L"t = %.3lf s (0x%x) (%d) (%d) torso hit", (frametime/1000.0), collisionModel, bbIndex, currentBoneIndex);
						} else if (fpHitMap[currentBoneIndex] == FP_HIT_LOCATION_HEAD) {
							uiChatPrintLine (L"t = %.3lf s (0x%x) (%d) (%d) head hit", (frametime/1000.0), collisionModel, bbIndex, currentBoneIndex);
						} else if (fpHitMap[currentBoneIndex] == FP_HIT_LOCATION_ARMS) {
							uiChatPrintLine (L"t = %.3lf s (0x%x) (%d) (%d) arms hit", (frametime/1000.0), collisionModel, bbIndex, currentBoneIndex);
						} else if (fpHitMap[currentBoneIndex] == FP_HIT_LOCATION_LEGS) {
							uiChatPrintLine (L"t = %.3lf s (0x%x) (%d) (%d) legs hit", (frametime/1000.0), collisionModel, bbIndex, currentBoneIndex);
						} else {
							uiChatPrintLine (L"t = %.3lf s (0x%x) (%d) (%d) UNKNOWN hit", (frametime/1000.0), collisionModel, bbIndex, currentBoneIndex);
						}
					}
#endif
					*location = currentBoneIndex;

					collisionPnt->x = rayStart->x + rayNormal->x * intersection.MinT;
					collisionPnt->y = rayStart->y + rayNormal->y * intersection.MinT;
					collisionPnt->z = rayStart->z + rayNormal->z * intersection.MinT;


#ifdef BETA_RELEASE
					lastColPt = *collisionPnt;
#endif
					return TRUE;
				}
			}
		}
	}
#ifdef BETA_RELEASE
	lastColLineP1 = *rayStart;
	lastColLineP2.x = rayStart->x + rayNormal->x;
	lastColLineP2.y = rayStart->y + rayNormal->y;
	lastColLineP2.z = rayStart->z + rayNormal->z;
#endif

	return FALSE;
}

/*
	NOTE:  this currently doesn't work but we might need it in the future
*/
void gr2SetPositioningBone (gr2Model* model, SCHAR* boneName)
{
	granny_int32	boneIndex;

	if(!(GrannyFindBoneByName(model->skeleton, boneName, &boneIndex)))
	{
		Write_Log (LOG_FPS, LOG_INFO, "gr2SetPositioningBone: Bone %s not found in model %s", model->grannyModel->Name, boneName);
		piExitProgram (_FL, "gr2SetPositioningBone: Failed to find bone for model");
	}

	model->rootPositioningBone = boneIndex;

//	if(!(GrannyFindBoneByName(model->skeleton, "Bip01", &boneIndex)))
//	{
//		Write_Log (LOG_FPS, LOG_INFO, "gr2SetPositioningBone: Bone %s not found in model %s", model->grannyModel->Name, boneName);
//		piExitProgram (_FL, "gr2SetPositioningBone: Failed to find bone for model");
//	}

//	model->movePositioningBone = boneIndex;
}

void loadTextures(granny_file_info* fileInfo, const char* path)
{
	if(!fileInfo || !fileInfo->TextureCount)
		return;

	// normals and speculars are created after the diffuse map is created
	std::vector<granny_texture*> normals;
	std::vector<granny_texture*> speculars;

	// determine what textures to load
	for(int i = 0; i < fileInfo->TextureCount; ++i)
	{
		if((fileInfo->Textures[i]->TextureType == GrannyColorMapTextureType) && (fileInfo->Textures[i]->ImageCount == 1))
		{
#ifdef GR2_TEXTURE_DEBUG
			_gr2Textures[fileInfo->Textures[i]->FromFileName] = path;
#endif

            if(strstr(fileInfo->Textures[i]->FromFileName, "_normal"))
            {
				normals.push_back(fileInfo->Textures[i]);
            }
            else if(strstr(fileInfo->Textures[i]->FromFileName, "_spec"))
            {
				speculars.push_back(fileInfo->Textures[i]);
            }
			else
			{
				// diffuseMap
				createTexture(fileInfo->Textures[i]);
			}
		}
		else
		{
			Write_Log (LOG_FPS, LOG_INFO, "whoa, what kind of texture is this?!?");
			//return nullptr;
		}
	}

	for(size_t i = 0; i < normals.size(); i++)
        createNormalTexture(normals[i]);

    for(size_t i = 0; i < speculars.size(); i++)
        createSpecularTexture(speculars[i]);
}

const char* getBaseName(const char* path, const char* prePath)
{
    std::string name = path;
    size_t sepPos = std::string::npos;
    
    if ((sepPos = name.rfind('\\')) != std::string::npos){
        name = name.substr(sepPos + 1);
    }
    std::for_each(name.begin(), name.end(), [] (char &c) { c = static_cast<char>(tolower(c)); });
    if ((sepPos = name.rfind('.')) != std::string::npos) {
        name = name.substr(0, sepPos);
    }
    name = (prePath ? prePath : "") + name;
	strsafecpy(texname, name.c_str());
	return texname;
}

static SINT32 loadSubMaterial(granny_variant& data)
{
	// reset our texname buffer
	texname[0] = 0;

    granny_variant lookup;

	if(!GrannyFindMatchingMember(data.Type, data.Object, "File Name", &lookup))
		return -1;

    // LightInfo should be a reference to the data
    if(lookup.Type[0].Type != GrannyStringMember)
		return -1;

    void const* definitionObject = *((void const**)(lookup.Object));

	if(!definitionObject)
		return -1;

	// setup our texture path
	const char* path = getBaseName((const char*)definitionObject, GR2_TEXTURE_PATH);

	// see if it already exists
	SINT32 texid = piGetNamedTexture(path);

	if(!piIsTexture(texid, 0))
	{
		// only look for dds files for now
		strcat(texname, ".dds");

		// if a file exists, try to load it
		if(fioAccess(texname))
		{
			texid = ldddsLoadDDSTexture(texname, GL_TEXTURE_2D, 0);

			if(piIsTexture(texid, 0))
			{
				char* ptr = strrchr(texname, '.');
				if(ptr != nullptr) *ptr = 0;

				// make sure and set our name so other models can use it to
				piSetTextureName(texid, texname);
			}
		}
	}

	return texid;
}

static SINT32 loadSubMaterial(granny_texture* texture)
{
	// reset our texname buffer
	texname[0] = 0;

	// setup our texture path
	const char* path = getBaseName(texture->FromFileName, GR2_TEXTURE_PATH);

	// see if it already exists
	SINT32 texid = piGetNamedTexture(path);

	if(!piIsTexture(texid, 0))
	{
		// only look for dds files for now
		strcat(texname, ".dds");

		// if a file exists, try to load it
		if(fioAccess(texname))
		{
			texid = ldddsLoadDDSTexture(texname, GL_TEXTURE_2D, 0);

			if(piIsTexture(texid, 0))
			{
				char* ptr = strrchr(texname, '.');
				if(ptr != nullptr) *ptr = 0;

				// make sure and set our name so other models can use it to
				piSetTextureName(texid, texname);
			}
		}
	}

	return texid;
}

// this function returns the diffuse map texid
static SINT32 loadMaterials(granny_material* material, bool& hasNormals)
{
#define LOG_NO_SPEC_OR_NORMALS 0

	if(!material) return -1;

	SINT32 diffuse	= -1;
	SINT32 normal	= -1;
	SINT32 specular = -1;

	// loop over the maps in this material and load them
	// also find the diffuse color map associated with this material
	for(int i = 0; i < material->MapCount; i++)
	{
		// use the first loaded texture as the diffuse color map
		// other textures might be loaded such as a normal map or a specular map
		SINT32 texid = loadSubMaterial(material->Maps[i].Material->ExtendedData);

		if(!piIsTexture(texid, 0) && material->Maps[i].Material->Texture)
			texid = loadSubMaterial(material->Maps[i].Material->Texture);

		if(piIsTexture(texid, 0))
		{
			// at this poiunt texname (a static buffer) has the texname in it
			if(strstr(texname, "_spec") && !piIsTexture(specular, 0))
				specular = texid;
			else if(strstr(texname, "_normal") && !piIsTexture(normal, 0))
				normal = texid;
			else if(diffuse == -1)
				diffuse = texid;
		}
	}

	// if we found a diffuse map; check for normals
	// and speculars not specified in the granny file
	if(piIsTexture(diffuse, 0))
	{
		TEXTURE* dt = piGetTextureData(diffuse);
		char filename[512];

		if(!piIsTexture(dt->specularMap, 0))
		{
			if(!piIsTexture(specular, 0))
			{
				snprintf(texname, 511, "%s_spec", dt->name);
				snprintf(filename, 511, "%s.dds", texname);

				// if a file exists, try to load it
				if(fioAccess(filename))
				{
					specular = ldddsLoadDDSTexture(filename, GL_TEXTURE_2D, 0);

					if(piIsTexture(specular, 0))
						piSetTextureName(specular, texname);
				}
			}

			dt->specularMap = specular;

#if LOG_NO_SPEC_OR_NORMALS
            if(!piIsTexture(specular, 0))
            {
                Enable_Log(LOG_OLD, 1);
                Write_Debug("[GRANNY] %s does not have a specular map", dt->name);
                Enable_Log(LOG_OLD, WantedUserLogEnabled);
            }
#endif
		}

		if(!piIsTexture(dt->normalMap, 0))
		{
			if(!piIsTexture(normal, 0))
			{
				snprintf(texname, 511, "%s_normal", dt->name);
				snprintf(filename, 511, "%s.dds", texname);

				// if a file exists, try to load it
				if(fioAccess(filename))
				{
					normal = ldddsLoadDDSTexture(filename, GL_TEXTURE_2D, 0);

					if(piIsTexture(normal, 0))
						piSetTextureName(normal, texname);
				}
			}

			dt->normalMap = normal;

#if LOG_NO_SPEC_OR_NORMALS
            if(!piIsTexture(normal, 0))
            {
                Enable_Log(LOG_OLD, 1);
                Write_Debug("[GRANNY] %s does not have a normal map", dt->name);
                Enable_Log(LOG_OLD, WantedUserLogEnabled);
            }
#endif
		}

		if(piIsTexture(dt->normalMap, 0))
			hasNormals = true;
	}

	// return our diffuse map
	return diffuse;
}

void createMeshTextures(gr2MeshData* data, bool wantNormals)
{
	/* Now that all the mesh data is submitted, I spin through the
	textures* erenced by this mesh and look up their in-memory
	versions that I've created with CreateTexture. */
	granny_mesh* grannyMesh = data->GrannyMesh;

	int textureCount = grannyMesh->MaterialBindingCount;

	bool hasNormals = false;

	for(int t = 0; t < textureCount; ++t)
	{
		granny_material* material = grannyMesh->MaterialBindings[t].Material;

		/* I ask Granny for the diffuse color texture of this material,	if there is one.  For a more complicated shader system,
		you would probably ask for more maps (like bump maps) and maybe query extra properties like shininess and so on. */
		if(material)
		{
			SINT32 texid = loadMaterials(material, hasNormals);

			if(piIsTexture(texid, 0))
			{
				data->textureRefs.push_back(texid);

				std::string name = grannyMesh->MaterialBindings[t].Material->Name;

				// HACK we came up with to use same materials with different textures: See Toto
				UINT32 pos = name.find('*');

				if(pos != std::string::npos)
					name[pos] = 0;

				data->materialRefs.push_back(piFindMaterialName(name.c_str()));

				Write_Log (LOG_FPS, LOG_INFO, "Material (%d) >%s< loading texture >%s<",
					t, grannyMesh->MaterialBindings[t].Material->Name, texname);

#if USE_STATESET
                if(data->materialRefs[t] == -1)
                    data->stateset[t].SetRenderState(new Material(oglFindMaterialName("forest1")));
                else
                    data->stateset[t].SetRenderState(new Material(data->materialRefs[t]));

                data->stateset[t].SetRenderState(new TextureUnit(0, texid));

                if(hasNormals && oglGetTextureData(texid)->normalMap > 0)
                {
                    data->stateset[t].SetRenderState((new Program("fpsnormal")));
                    data->stateset[t].SetMode(GL_LIGHTING, 0);
                    data->stateset[t].SetRenderState(new TextureUnit(1, oglGetTextureData(texid)->normalMap));
                    data->is_normal = true;

                    if(oglGetTextureData(texid)->specularMap > 0)
                    {
                        data->stateset[t].SetRenderState(new TextureUnit(2, oglGetTextureData(texid)->specularMap));
                    }
                    else
                    {
                        int id = oglGetNamedTexture("default_spec");

                        if(!oglIsTexture(id, 0))
                            id = ldTextureLoad("default_spec.dds", 0);

                        data->stateset[t].SetRenderState(new TextureUnit(2, id));
                    }
                }
#endif
			}
			else
			{
				Write_Log (LOG_FPS, LOG_INFO, "Material (%d) >%s< has no texture!",
					t, grannyMesh->MaterialBindings[t].Material->Name);

				data->textureRefs.push_back(-1);
			}
		}
		else
		{
			data->textureRefs.push_back(-1);
		}
	}

	// make sure we have normal data setup
	if(wantNormals && hasNormals)
	{
		if(!data->pngt)
		{
			int vertexCount = GrannyGetMeshVertexCount(grannyMesh);
			data->pngt = new piTVERTEX[vertexCount]{};
			GrannyCopyMeshVertices(grannyMesh, GrannyPNGT3332VertexType, data->pngt);
		}

#if USE_STATESET
#else
		glslNormalFpsSetup(data);
#endif
	}
}

gr2AnimationBlend* gr2LoadBlendedAnimation(const SCHAR* blendfile, SINT32 boneCount)
{
	char path[MAX_GR2_PATH_SIZE+1];

	// setup the expected path
	snprintf(path, MAX_GR2_PATH_SIZE, "%s%s", GR2DATA_PREPATH, blendfile);

	// strip off the extension if found
	char* extension = strchr(path, '.');
	if(extension) *extension = 0;

	// try the xml version of the file
	snprintf(path, MAX_GR2_PATH_SIZE, "%s.xml", path);

	if(GR2_USE_XML_BLENDS && fioAccess(path))
		return gr2LoadBlendedAnimationXML(path, boneCount);

	// strip off the extension if found
	extension = strchr(path, '.');
	if(extension) *extension = 0;

	// try the .txt version of the file
	snprintf(path, MAX_GR2_PATH_SIZE, "%s.txt", path);

	if(fioAccess(path))
		return gr2LoadBlendedAnimationTXT(path, boneCount);

	// blend file not found
	return nullptr;
}

static SINT32 _translateBoneIndex(SINT32 bone)
{
	static gr2Model* oldSkeleton = nullptr;
	static gr2Model* newSkeleton = nullptr;

	if(oldSkeleton == nullptr)
	{
		oldSkeleton = gr2LoadModel("TPS\\troopskel", nullptr, nullptr, 1);
		newSkeleton = gr2Model::Load(fpTPSSkeletonFile, nullptr, nullptr, 1);
	}

	if(bone < oldSkeleton->skeleton->BoneCount)
		return gr2ModelFindBone(newSkeleton, (SCHAR*)oldSkeleton->skeleton->Bones[bone].Name);

	return -1;
}

static const char* _translateAnimation(const char* animFile)
{
	static char path[254+1];;
	const char* ptr = path;

	if(!stricmp(animFile, "TPS\\animations\\trot"))
		strncpy(path, "TPS\\animations\\Anim_Stand_Run01", MAX_GR2_PATH_SIZE);
	else if(!stricmp(animFile, "TPS\\animations\\walk"))
		strncpy(path, "TPS\\animations\\anim_stand_walk01", MAX_GR2_PATH_SIZE);
	else if(!stricmp(animFile, "TPS\\animations\\sprint"))
		strncpy(path, "TPS\\animations\\Anim_Stand_Sprint01", MAX_GR2_PATH_SIZE);
	else if(!stricmp(animFile, "TPS\\animations\\crawl") || !stricmp(animFile, "TPS\\animations\\crawlrifle"))
		strncpy(path, "TPS\\animations\\anim_prone_rifle_crawl", MAX_GR2_PATH_SIZE);
	else if(!stricmp(animFile, "TPS\\animations\\kneel"))
		strncpy(path, "TPS\\animations\\Anim_Kneel_Base", MAX_GR2_PATH_SIZE);
	else if(!stricmp(animFile, "TPS\\animations\\rifleprone"))
		strncpy(path, "TPS\\animations\\anim_prone_rifle_base", MAX_GR2_PATH_SIZE);
	else
		strncpy(path, animFile, MAX_GR2_PATH_SIZE);

	return ptr;
}

gr2AnimationBlend* gr2LoadBlendedAnimationTXT(SCHAR* blendfile, SINT32 boneCount)
{
	animSpec loadedMasks[GR2_MAX_ANIMS_PER_BLEND]{};

	char		buf[255];
	char		tString[255];
	int			scanReturn;
	int			numAnimFiles = 0;
	SCHAR* ptr;
	SINT32 i, j;
	gr2AnimationBlend* newBlend;

	SINT32 blendFileID = fioOpen(blendfile);
	if (blendFileID == -1) return nullptr;

	bool translateBones = true;
	SINT32 type = gr2AnimationBlend::BASE;

	FLOAT32 mod1 = 0.0f;
	FLOAT32 mod2 = 0.0f;

	while ((fioGetLine (blendFileID, buf, 255) != 0))
	{
		if (buf[0] == '#' || strlen(buf) < 2)
			continue;

		// at least use type, modifier is optional
		if (sscanf(buf, "type %d %f %f", &type, &mod1, &mod2) >= 1)
			continue;

		if (strncmp(buf, "version 2", 9) == 0)
		{
			translateBones = false;
			continue;
		}

		if (strncmp(buf, "anim", 4) == 0)
		{
			scanReturn = sscanf (buf, "%*s %s %f", tString, &(loadedMasks[numAnimFiles].defaultWeight));
			if (scanReturn != 2)
			{
				Write_Log (LOG_FPS, LOG_INFO, "gr2LoadBlendedAnimation: Bad anim line in blendfile %s", blendfile);
				piExitProgram (_FL, "gr2LoadBlendedAnimation: Bad anim line in blendfile");
			}

			ptr = strrchr(tString, '.');
			if (ptr != nullptr)
			{
				*ptr = 0;
			}


			// hack for now - ramp
			if(!stricmp(tString, "standaimwalk"))
				strcpy(tString, "walk");

			snprintf (loadedMasks[numAnimFiles].animationFile, 255, "%s%s%s", TPS_MAIN_DIRECTORY, TPS_ANIMATIONS_DIRECTORY, tString);

			// mark whole track as uninitialized
			for (int i=0; i< 255; i++)
			{
				loadedMasks[numAnimFiles].tracks[i] = UNUSED_TRACK_VALUE;
			}

			while (1)
			{
				int		boneNum;
				float	boneWeight;

				fioGetLine (blendFileID, buf, 255);
				if (!(strncmp(buf, "end", 3)))
				{
					break;
				}

				scanReturn = sscanf (buf, "%d %f", &boneNum, &boneWeight);
				if (scanReturn != 2)
				{
					Write_Log (LOG_FPS, LOG_INFO, "gr2LoadBlendedAnimation: Bad data line in animation %s", loadedMasks[numAnimFiles].animationFile);
					//piExitProgram (_FL, "gr2LoadBlendedAnimation: Bad data line in animation");
					continue;
				}

				int convertedBone = (translateBones) ? _translateBoneIndex(boneNum): -1;

				if(convertedBone >= 0)
					boneNum = convertedBone;

				if (boneWeight != loadedMasks[numAnimFiles].defaultWeight)
					loadedMasks[numAnimFiles].tracks[boneNum] = boneWeight;
			}
			numAnimFiles++;
		}
	}

	fioClose (blendFileID);

#ifndef GR2_BLEND_EDITOR
	if (numAnimFiles < 1)
	{
		Write_Log (LOG_FPS, LOG_INFO, "gr2LoadBlendedAnimation: No animation files specified", loadedMasks[numAnimFiles].animationFile);
		piExitProgram (_FL, "gr2LoadBlendedAnimation: No animation files specified");
	}
#endif

	newBlend = _gr2BlendPool.Pop();

	if (!newBlend)
	{
		piExitProgram (_FL, "gr2LoadBlendedAnimation: failed to allocate memory for newBlend");
	}

	newBlend->mod1 = mod1;
	newBlend->mod2 = mod2;
	newBlend->type = type;
	newBlend->length = 0;

	newBlend->numBlends = 0; //std::min(numAnimFiles, GR2_MAX_ANIMS_PER_BLEND);
	memset (newBlend->animations, 0, sizeof (gr2Animation*) * GR2_MAX_ANIMS_PER_BLEND);

	for (i = 0; i < numAnimFiles; i++)
	{
		const char* animPath = _translateAnimation(loadedMasks[i].animationFile);
		newBlend->animations[i] = gr2LoadAnimation ((SCHAR*)animPath, 1);

		if (!newBlend->animations[i])
		{
			Write_Log (LOG_FPS, LOG_INFO, "gr2LoadBlendedAnimation: Failed to load animation %s in blend %s", animPath, blendfile);
			continue;
		}

		newBlend->animations[i]->mask = GrannyNewTrackMask (loadedMasks[i].defaultWeight, boneCount);

		for (j=0; j < boneCount; j++)
		{
			if (loadedMasks[i].tracks[j] != UNUSED_TRACK_VALUE)
			{
				GrannySetTrackMaskBoneWeight((newBlend->animations[i])->mask, j, loadedMasks[i].tracks[j]);
			}
		}

		// mark the first animation of the blend as the baseloop
		// NOTE: probably will need some more intelligence here
		if (i == 0)
		{
			newBlend->animations[i]->animType = GR2_ANIMATION_BASELOOP;
		}

		newBlend->numBlends++;
	}

	if(newBlend->numBlends > GR2_MAX_ANIMS_PER_BLEND)
		newBlend->numBlends = GR2_MAX_ANIMS_PER_BLEND;

	const SCHAR* fileName = nullptr;
	char* slash = strchr(blendfile, '\\');

	while(slash)
	{
		fileName = slash + 1;
		slash = strchr(slash+1, '\\');
	}

	if(!fileName)
		fileName = blendfile;

	strncpy (newBlend->blendFile, fileName, MAX_GR2_PATH_SIZE);
//	newBlend->blendFile = blendfile;

	return newBlend;
}

/*
gr2AnimationBlend* gr2LoadBlendedAnimationTXT(SCHAR* blendfile, SINT32 boneCount)
{
	animSpec	loadedMasks[GR2_MAX_ANIMS_PER_BLEND];
	SINT32		blendFileID;
	char		buf[255];
	char		tString[255];
	int			scanReturn;
	int			numAnimFiles = 0;
	SCHAR*		ptr;
	SINT32		i, j;
	gr2AnimationBlend*	newBlend;

	memset (loadedMasks, 0, sizeof (loadedMasks));

	blendFileID = fioOpen(blendfile);
	if (blendFileID == -1)
	{
		return nullptr;
	}

	while ((fioGetLine (blendFileID, buf, 255) != 0))
	{
		if (buf[0] == '#' || strlen(buf) < 2)
		{
			continue;
		}

		if (strncmp(buf, "anim", 4) == 0)
		{
			scanReturn = sscanf (buf, "%*s %s %f", tString, &(loadedMasks[numAnimFiles].defaultWeight));
			if (scanReturn != 2)
			{
				Write_Log (LOG_FPS, LOG_INFO, "gr2LoadBlendedAnimation: Bad anim line in blendfile %s", blendfile);
				piExitProgram (_FL, "gr2LoadBlendedAnimation: Bad anim line in blendfile");
			}

			ptr = strrchr(tString, '.');
			if (ptr != nullptr)
			{
				*ptr = 0;
			}


			// hack for now - ramp
			if(!stricmp(tString, "standaimwalk"))
				strcpy(tString, "walk");

			snprintf (loadedMasks[numAnimFiles].animationFile, 255, "%s%s%s", TPS_MAIN_DIRECTORY, TPS_ANIMATIONS_DIRECTORY, tString);

			// mark whole track as uninitialized
			for (int i=0; i< 255; i++)
			{
				loadedMasks[numAnimFiles].tracks[i] = UNUSED_TRACK_VALUE;
			}

			while (1)
			{
				int		boneNum;
				float	boneWeight;

				fioGetLine (blendFileID, buf, 255);
				if (!(strncmp(buf, "end", 3)))
				{
					break;
				}

				scanReturn = sscanf (buf, "%d %f", &boneNum, &boneWeight);
				if (scanReturn != 2)
				{
					Write_Log (LOG_FPS, LOG_INFO, "gr2LoadBlendedAnimation: Bad data line in animation %s", loadedMasks[numAnimFiles].animationFile);
					piExitProgram (_FL, "gr2LoadBlendedAnimation: Bad data line in animation");
				}

				if (boneWeight != loadedMasks[numAnimFiles].defaultWeight)
					loadedMasks[numAnimFiles].tracks[boneNum] = boneWeight;
			}
			numAnimFiles++;
		}
	}

	fioClose (blendFileID);

	if (numAnimFiles < 1)
	{
		Write_Log (LOG_FPS, LOG_INFO, "gr2LoadBlendedAnimation: No animation files specified", loadedMasks[numAnimFiles].animationFile);
		piExitProgram (_FL, "gr2LoadBlendedAnimation: No animation files specified");
	}

	newBlend = _gr2BlendPool.Pop();

	if (!newBlend)
	{
		piExitProgram (_FL, "gr2LoadBlendedAnimation: failed to allocate memory for newBlend");
	}

	newBlend->numBlends = std::min(numAnimFiles, GR2_MAX_ANIMS_PER_BLEND);
	memset (newBlend->animations, 0, sizeof (gr2Animation*) * GR2_MAX_ANIMS_PER_BLEND);

	for (i = 0; i < numAnimFiles; i++)
	{
		newBlend->animations[i] = gr2LoadAnimation (loadedMasks[i].animationFile, 1);

		if (!newBlend->animations[i])
		{
			Write_Log (LOG_FPS, LOG_INFO, "gr2LoadBlendedAnimation: Failed to load animation %s in blend %s", loadedMasks[i].animationFile, blendfile);
			continue;
		}

		newBlend->animations[i]->mask = GrannyNewTrackMask (loadedMasks[i].defaultWeight, boneCount);

		for (j=0; j < boneCount; j++)
		{
			if (loadedMasks[i].tracks[j] != UNUSED_TRACK_VALUE)
			{
				GrannySetTrackMaskBoneWeight((newBlend->animations[i])->mask, j, loadedMasks[i].tracks[j]);
			}
		}

		// mark the first animation of the blend as the baseloop
		// NOTE: probably will need some more intelligence here
		if (i == 0)
		{
			newBlend->animations[i]->animType = GR2_ANIMATION_BASELOOP;
		}
	}

	strncpy (newBlend->blendFile, blendfile, 255);
//	newBlend->blendFile = blendfile;

	return newBlend;
}
*/

gr2AnimationBlend* gr2LoadBlendedAnimationXML(const SCHAR* blendfile, SINT32 boneCount)
{
	gr2AnimationBlend* newBlend = _gr2BlendPool.Pop();

	if(!newBlend)
		piExitProgram (_FL, "gr2LoadBlendedAnimationXML: failed to allocate memory for newBlend");

	memset(newBlend->animations, 0, sizeof(gr2Animation*) * GR2_MAX_ANIMS_PER_BLEND);

	xmlDataMap userData;

	userData["blend"] = newBlend;
	userData["bones"] = &boneCount;

	XMLLoader config(blendfile);

	config.AddHandler("blend", xmlBlendTagHandler);
	config.AddHandler("anim", xmlBlendAnimTagHandler);
	config.AddHandler("bone", xmlBlendBoneTagHandler);

	if(!config.Load(&userData))
	{
		free(newBlend);
		return nullptr;
	}

	strncpy(newBlend->blendFile, blendfile, MAX_GR2_PATH_SIZE);

	return newBlend;
}

gr2AnimationBlend* gr2CreateBlendedAnimation(const SCHAR* animFile, SINT32 boneCount)
{
	gr2AnimationBlend* newBlend = _gr2BlendPool.Pop();

	if (!newBlend)
		piExitProgram (_FL, "gr2LoadBlendedAnimation: failed to allocate memory for newBlend");

	newBlend->type = gr2AnimationBlend::BASE;
	newBlend->numBlends = 0;
	newBlend->mod1 = 0.0f;
	newBlend->mod2 = 0.0f;
	memset(newBlend->animations, 0, sizeof(gr2Animation*)*GR2_MAX_ANIMS_PER_BLEND);

	if(animFile)
	{
		newBlend->animations[0] = gr2LoadAnimation(animFile, 1);
		newBlend->numBlends = 1;

		if (!newBlend->animations[0])
			piExitProgram (_FL, "gr2CreateBlendedAnimation: Failed to load animation in DYNAMIC blend");

		newBlend->animations[0]->mask = GrannyNewTrackMask(1.0f, boneCount);
		newBlend->animations[0]->animType = GR2_ANIMATION_BASELOOP;
	}

	strncpy(newBlend->blendFile, "dynamic", 255);

	return newBlend;
}

LOGICAL gr2RemoveAllBlendedAnimations (gr2Model* model, FLOAT32 easeOutOverrideTime, gr2AnimationBlend* blend)
{
	if(!model->blends)
		return TRUE;

	GR2_BLEND_MAP::iterator it = model->blends->begin();
#if platOS == OS_MACOS
	std::vector<gr2AnimationBlend*> blendsToKill;
#endif

	gr2AnimationBlend* curBlend = nullptr;
	gr2ControlBlend* curControl = nullptr;

	for(; it != model->blends->end(); ++it)
	{
		if(it->first == blend)
			continue;

		curBlend = it->first;
		curControl = it->second;

		if (curControl->active && !curControl->mixin)
		{
			if (curControl->easeOutDuration > 0.0)
			{
				Write_Log (LOG_FPS, LOG_INFO, "gr2RemoveAllBlendedAnimations: blend 0x%x %s  out %f time %d", curBlend, curBlend->blendFile, curControl->easeOutDuration, frametime);

				if (easeOutOverrideTime < 0)
				{
					curControl->killTime = frametime + (SINT32)(curControl->easeOutDuration * 1000.0f);
				}
				else
				{
					curControl->killTime = frametime + (SINT32)(easeOutOverrideTime * 1000.0f);
				}

				for (int j = 0; j < curBlend->numBlends; j++)
				{
//					FLOAT32	easeWeight;
//					easeWeight = GrannyGetControlEaseCurveMultiplier(model->blendControls[i].controls[j].control);

//					GrannySetControlEaseIn(model->blendControls[i].controls[j].control, true);
//					GrannySetControlEaseOutCurve(model->blendControls[i].controls[j].control,
//						(frametime/1000.0f), (model->blendControls[i].killTime/1000.0f), easeWeight, 1.0f, 0.0f, 0.0f);

//					Write_Log (LOG_FPS, LOG_INFO, "gr2RemoveAllBlendedAnimations: anim %d weight %f", j, GrannyGetControlEaseCurveMultiplier(model->blendControls[i].controls[j].control));
					if (easeOutOverrideTime < 0)
					{
						GrannyEaseControlOut(curControl->controls[j].control, curControl->easeOutDuration);
					}
					else
					{
						GrannyEaseControlOut(curControl->controls[j].control, easeOutOverrideTime);
					}

				}

				curControl->active = FALSE;
			}
			else
			{
				// clear immediately
#if platOS == OS_MACOS
				blendsToKill.push_back(curBlend);
#else
				Write_Log (LOG_FPS, LOG_INFO, "gr2RemoveAllBlendedAnimations: blend 0x%x %s  CLEAR", curBlend, curBlend->blendFile);

				for (int j = 0; j < curBlend->numBlends; j++)
				{
//					GrannyEaseControlOut(&model->blendControls[i].controls[j], model->blendControls[i].easeOutDuration)
					gr2StopControl (&curControl->controls[j], TRUE);
				}

				if(curControl->killCallback)
					curControl->killCallback(curControl->killData);

				// we don't free the blended animation because that is done at game exit
				gr2FreeBlendedControl(curControl);

				it = model->blends->erase(it);
#endif
			}
		}
	}

#if platOS == OS_MACOS
	if(!blendsToKill.empty())
	{
		for(int i = 0; i < (int)blendsToKill.size(); ++i)
		{
			it = model->blends->find(blendsToKill[i]);

			curBlend = it->first;
			curControl = it->second;

			Write_Log (LOG_FPS, LOG_INFO, "gr2RemoveAllBlendedAnimations: blend 0x%x %s  CLEAR", curBlend, curBlend->blendFile);

			for (int j = 0; j < curBlend->numBlends; j++)
			{
				// GrannyEaseControlOut(&model->blendControls[i].controls[j], model->blendControls[i].easeOutDuration)
				gr2StopControl (&curControl->controls[j], TRUE);
			}

			if(curControl->killCallback)
				curControl->killCallback(curControl->killData);

			// we don't free the blended animation because that is done at game exit
			gr2FreeBlendedControl(curControl);

			// delete it from the map
			model->blends->erase(it);
		}
	}
#endif

	gr2UpdateBlendEvents(model);

	return TRUE;
}

LOGICAL gr2ClearBlendedAnimations(gr2Model* model)
{
	if(!model->blends)
		return TRUE;

	GR2_BLEND_MAP::iterator it = model->blends->begin();
	std::vector<gr2AnimationBlend*> blendsToKill;

	gr2AnimationBlend* curBlend = nullptr;
	gr2ControlBlend* curControl = nullptr;

	for(; it != model->blends->end(); ++it)
		blendsToKill.push_back(it->first);

	if(!blendsToKill.empty())
	{
		for(int i = 0; i < (int)blendsToKill.size(); ++i)
		{
			it = model->blends->find(blendsToKill[i]);

			curBlend = it->first;
			curControl = it->second;

			Write_Log (LOG_FPS, LOG_INFO, "gr2RemoveAllBlendedAnimations: blend 0x%x %s  CLEAR", curBlend, curBlend->blendFile);

			for (int j = 0; j < curBlend->numBlends; j++)
			{
				// GrannyEaseControlOut(&model->blendControls[i].controls[j], model->blendControls[i].easeOutDuration)
				gr2StopControl (&curControl->controls[j], TRUE);
			}

			// we don't free the blended animation because that is done at game exit
			gr2FreeBlendedControl(curControl);

			// delete it from the map
			model->blends->erase(it);
		}
	}

	gr2UpdateBlendEvents(model);

	return TRUE;
}

void gr2BlendControlResurrect (gr2Model* model, gr2AnimationBlend* animationBlend, FLOAT32 maxEaseInTime)
{
#ifdef ALLOW_RESURRECT
	gr2ControlBlend* blendControl = gr2BlendControlFindRunningBlend(model, animationBlend);

	if(!blendControl)
	{
		Write_Log (LOG_FPS, LOG_INFO, "gr2BlendControlResurrect: control not found");
		return;
	}

	SINT32	killtimeLeft;
	FLOAT32	realtimeLeft;
	FLOAT32	percentLeft;
	FLOAT32	easeInTime;

	// if we have a killtime, it means we had some ease out going
	// we'll start the easein curve at the current strength, and reduce
	// the ease in time by (1-complete%)
	if (blendControl->killTime)
	{
		killtimeLeft = blendControl->killTime - frametime;
		realtimeLeft = ((FLOAT32)killtimeLeft) / 1000.0f;
		// a little sanity checking ..
		if (realtimeLeft < 0.0)
		{
			Write_Log (LOG_FPS, LOG_INFO, "gr2BlendControlResurrect: negative realtime %f %d %d", realtimeLeft, killtimeLeft, frametime);
			realtimeLeft = 0;
		}
		else if (realtimeLeft > blendControl->easeOutDuration)
		{
			Write_Log (LOG_FPS, LOG_INFO, "gr2BlendControlResurrect: excessive realtime %f %f %d %d", realtimeLeft, blendControl->easeOutDuration, killtimeLeft, frametime);
			realtimeLeft = blendControl->easeOutDuration;
		}
		else
		{
//			Write_Log (LOG_FPS, LOG_INFO, "gr2BlendControlResurrect: normal realtime %f %f %d %d", realtimeLeft, model->blendControls[blendNum].easeOutDuration, killtimeLeft, frametime);
		}

		percentLeft = 1.0f - (realtimeLeft / blendControl->easeOutDuration);

		easeInTime = blendControl->easeInDuration * percentLeft;
		if (easeInTime > maxEaseInTime)
			easeInTime = maxEaseInTime;
//		Write_Log (LOG_FPS, LOG_INFO, "gr2BlendControlResurrect: data %f %f %d", percentLeft, easeInTime, frametime);
	}
	else
	{
//		Write_Log (LOG_FPS, LOG_INFO, "gr2BlendControlResurrect: no killtime %f %d", model->blendControls[blendNum].easeOutDuration, frametime);
//		easeInTime = model->blendControls[blendNum].easeInDuration;
		easeInTime = maxEaseInTime;
	}

	for (int i = 0; i < animationBlend->numBlends; i++)
	{
		// if the time is too short to be noticeable, just turn off the easein curve
		if (easeInTime < 0.1)
		{
			GrannySetControlEaseIn (blendControl->controls[i].control, false);
		}
		else
		{
			FLOAT32	easeWeight;
			easeWeight = GrannyGetControlEaseCurveMultiplier(blendControl->controls[i].control);

			GrannySetControlEaseIn (blendControl->controls[i].control, true);
			GrannySetControlEaseInCurve (blendControl->controls[i].control,
				(frametime/1000.f), (frametime/1000.f) + easeInTime, easeWeight, 0.0f, 1.0f, 1.0f);
//			Write_Log (LOG_FPS, LOG_INFO, "gr2BlendControlResurrect: curve %f -> %f, weight %f", (frametime/1000.f), (frametime/1000.f) + easeInTime, easeWeight);
		}

		GrannySetControlEaseOut (blendControl->controls[i].control, false);

//		Write_Log (LOG_FPS, LOG_INFO, "gr2BlendControlResurrect: anim weights %f %f",
//			GrannyGetControlEaseCurveMultiplier(model->blendControls[blendNum].controls[i].control),
//			GrannyGetControlEffectiveWeight(model->blendControls[blendNum].controls[i].control)	);

		// if this isn't an infinite blend, we need to reset the local clock
		// and index otherwise it will just continue where it left off; most
		// likely this will be very near the end of the animation
		if(blendControl->loopCount > 0)
		{
			GrannySetControlLoopIndex(blendControl->controls[i].control, 0);
			GrannySetControlRawLocalClock(blendControl->controls[i].control, 0.0f);
		}
	}

	Write_Log (LOG_FPS, LOG_INFO, "gr2BlendControlResurrect: blend 0x%x %s  time = %f",
		animationBlend, animationBlend->blendFile, easeInTime);

	blendControl->active = true;
	blendControl->killTime = 0;
#else
	return;
#endif
}

// NOTE:  this assumes it's being called right as the animation is started!!!
void gr2BlendControlRemoveWhenDone (gr2Model* model, gr2AnimationBlend* animationBlend, CONTROL_CALLBACK c, void* data)
{
	gr2ControlBlend* blendControl = gr2BlendControlFindRunningBlend(model, animationBlend);

	if (!blendControl)
	{
		Write_Log (LOG_FPS, LOG_INFO, "gr2BlendControlRemoveWhenDone: failed to find control!");
		return;
	}

	// this is a no-op if the animation has a loopCount of 0
	if (blendControl->loopCount == 0)
		return;

	granny_real32	animationLength;
	granny_real32	easeOutStartTime, easeOutEndTime, currentTime;
	SINT32			timeOffset;

	// use the first animation in the blend as the "timing" animation
	animationLength = GrannyGetControlLocalDuration (blendControl->controls[0].control);
	animationLength *= blendControl->loopCount;

	timeOffset = (SINT32)(animationLength * 1000.0f);

	currentTime = frametime / 1000.0f;

	easeOutEndTime = currentTime + animationLength;

	easeOutStartTime = easeOutEndTime - blendControl->easeOutDuration;

	for (int i = 0; i < animationBlend->numBlends; i++)
	{
		GrannySetControlEaseOut (blendControl->controls[i].control, true);
		GrannySetControlEaseOutCurve(blendControl->controls[i].control, easeOutStartTime, easeOutEndTime,
        1.0f, 1.0f, 0.0f, 0.0f);

	}

	blendControl->killTime	= (SINT32)(easeOutEndTime * 1000.0f);
	blendControl->killCallback = c;
	blendControl->killData = data;
}

#ifdef BETA_RELEASE
void gr2ToggleColTest (void)
{
	colTestFlag = !colTestFlag;

	if (colTestFlag)
		uiChatPrintLine (L"Trooper hits to chat buffer ON");
	else
		uiChatPrintLine (L"Trooper hits to chat buffer OFF");

}

void gr2MemStats(int& count, int& memSize)
{
	uiChatPrintLine("Model Pool reports: %d objects, %dk memory used", _gr2ModelPool.Count(), int(_gr2ModelPool.MemSize()/1000));
	uiChatPrintLine("Mesh Pool reports: %d objects, %dk memory used", _gr2MeshPool.Count(), int(_gr2MeshPool.MemSize()/1000));
	uiChatPrintLine("Animation Pool reports: %d objects, %dk memory used", _gr2AnimationPool.Count(), int(_gr2AnimationPool.MemSize()/1000));
	uiChatPrintLine("Blend Pool reports: %d objects, %dk memory used", _gr2BlendPool.Count(), int(_gr2BlendPool.MemSize()/1000));
	uiChatPrintLine("Blend Control Pool reports: %d objects, %dk memory used", _gr2BlendControlPool.Count(), int(_gr2BlendControlPool.MemSize()/1000));
	uiChatPrintLine("Control Pool reports: %d objects, %dk memory used", _gr2ControlPool.Count(), int(_gr2ControlPool.MemSize()/1000));

	count = _gr2ModelPool.Count() + _gr2MeshPool.Count() + _gr2AnimationPool.Count() + _gr2BlendPool.Count() + _gr2BlendControlPool.Count() + _gr2ControlPool.Count();
	memSize = _gr2ModelPool.MemSize() + _gr2MeshPool.MemSize() + _gr2AnimationPool.MemSize() + _gr2BlendPool.MemSize() + _gr2BlendControlPool.MemSize() + _gr2ControlPool.MemSize();
}

#endif  // beta

void _cleartestpatch(gr2Model* model, SINT32 index)
{
    const char* path = "vehicles/vo/dummy.bmp";

	// Increment * count

    SINT32 dummy = ldpixLoadImageFileToSurface(path, colorBLACK, LDPIX_MIPMAP);
	gr2ModelFindAndReplaceTexture(model, fpTPSPatchTextureIDs[index], dummy);
}

void _cleartestpatches(gr2Model* model, SINT32 id)
{
	_cleartestpatch(model, id + 0);
	_cleartestpatch(model, id + 1);
	_cleartestpatch(model, id + 2);
}

void _replaceModelTexturesIfNecessary(gr2Model* model)
{
    if(_replacements.empty())
    {
        return;
    }

    std::map<std::string, TextureReplacement *>::iterator find;
    find = _replacements.find(model->name);

    // If this model is in our list, get the texture replacements.
    if(find == _replacements.end())
    {
        return;
    }

    TextureReplacement* tr = find->second;
    std::map<std::string, std::vector<std::string> >::iterator itr;
    itr = tr->fromto.begin();

    for(; itr != tr->fromto.end(); ++itr)
    {
        if(itr->second.empty())
        {
            continue;
        }

        int id = GetRandom(0, itr->second.size() - 1);

        int oldtex = piGetNamedTexture(itr->first.c_str());
        int newtex = piGetNamedTexture(itr->second[id].c_str());

        if(piIsTexture(oldtex, 0) && piIsTexture(newtex, 0))
        {
            gr2ModelFindAndReplaceTexture(model, oldtex, newtex);
        }
    }
}

// some hardcoding in here ..
void gr2ModelPersonalize (gr2ModelAlt& model, SINT32 countryID, SINT32 branch, SCHAR* handle, SINT32 decalID)
{
	GR2_MODEL_LIST list = model.GetAllModels();

	for(size_t i = 0; i < list.size(); ++i)
		gr2ModelPersonalize(list.at(i), countryID, branch, handle, decalID);
}

void gr2ModelPersonalize (gr2Model* model, SINT32 countryID, SINT32 branch, SCHAR* handle, SINT32 decalID)
{
	return; // no personalization for now

	//SINT32			i;
	//UINT32			personaNum;
	SINT32			decalTexID;

	if(!model)
        return;

	/*
    if(!Online)
    {
        _cleartestpatches(model, countryID);
        return;
    }
	*/

	if ((!handle) || (handle[0] == 0))
    {
        _cleartestpatches(model, countryID);
		return;
    }

	// personalize FACE first
	/* no faces for now
	if (!fpTPSNumFaces || ((countryID >= BRITISH_HC) && (countryID <= FRENCH_HC)))
    {
        // Do nothing. HC has default face;
    }
    else
    {
	    adder = 0;
	    adder2 = 0;

	    // branch 0 is not used, real values are 1..3
	    // 3 because there are 3 countries
	    personaNum = (branch - 1) + countryID * 3;

	    for (i=0; i < (SINT32)strlen(handle); i++)
	    {
		    adder += (UINT32)handle[i];
		    if (i & 1)	// Fast modulo 2
		    {
			    adder2 -= (UINT32)handle[i];
		    }
		    else
		    {
			    adder2 += (UINT32)handle[i];
		    }

	    }

#ifdef REPLACE_MODEL_TEXTURES
        _replaceModelTexturesIfNecessary(model);
#endif

	    faceNum = (adder + (adder2 * personaNum)) % fpTPSNumFaces;

	    if (model->currentFaceID == -1)
	    {
		    if (fpTPSFaceTexIDs[faceNum] == fpTPSDefaultFaceTexIDs[countryID])
            {
                _cleartestpatches(model, countryID);
			    return;
            }

		    gr2ModelFindAndReplaceTexture (model, fpTPSDefaultFaceTexIDs[countryID], fpTPSFaceTexIDs[faceNum]);
	    }
	    else
	    {
		    if (model->currentFaceID == (int)faceNum)
            {
                _cleartestpatches(model, countryID);
			    return;
            }

		    gr2ModelFindAndReplaceTexture (model, fpTPSFaceTexIDs[model->currentFaceID], fpTPSFaceTexIDs[faceNum]);
	    }

	    model->currentFaceID = faceNum;

	    Write_Log (LOG_FPS, LOG_INFO, "gr2ModelPersonalize: <%s>, <%d> <%d> <%d> to 0x%x", handle, countryID, branch, decalID, model);
    }
	*/

	// now do patches
	// check to make sure it's a valid patch
	if (fpTPSPatchTextureIDs[countryID] < 0)
	{
		model->patchTexID = -1;
        _cleartestpatches(model, countryID);
		return;
	}

	decalTexID = vehdecalGetTrooperPersonalDecal(decalID);

	if (decalTexID < 0)
	{
        _cleartestpatches(model, countryID);
		model->patchTexID = -1;
		return;
	}

	SINT32 infTexSize = 128, infTexLOD = 1;

    // Set these externally
//	SINT32 sx = 0;
//	SINT32 sy = 0;
	SINT32 dx = 52;
	SINT32 dy = 64;

	if ((countryID == GERMAN_INFANTRY) || (countryID == GERMAN_ML))
	{
		infTexSize = 256;
		dx = 40;
		dy = 128;
		infTexLOD = 0;
	}
	else if (countryID == FRENCH_INFANTRY)
	{
		dx = 20;
		dy = 64;
	}
	else if (countryID == BRITISH_INFANTRY)
	{
		dx = 52;
		dy = 64;
	}

#if(0)
    SINT32 newTexID = piCreateRawTexture(-1, 0, infTexSize, infTexSize, TexBpp, 4, CRT_FLAG_NONE);
    piSURFACEDESC base, patch, dest;

    ldtexOpenWork(fpTPSPatchTextureIDs[countryID], 0, &base);
	ldtexOpenWork(decalTexID, infTexLOD, &patch);
    ldtexOpenWork(newTexID, 0, &dest);

	ldpixCopySurfaceToSurface (&base, &dest);

	SINT32 srcOffset = sy * patch.Height + sx;
	SINT32 dstOffset = dy * dest.Height + dx;

//  ldtexCopySubsurfaceToSubsurface(patch.Width, patch.Height, srcOffset, &patch, dstOffset, &dest, colorBLACK);
	TEXTURE* t = piGetTextureData(decalTexID);

    UINT32 transparentColor = colorBLACK;

    if(t)
    {
        if(t->isdds() && t->comp[infTexLOD] == 4)
        {
            transparentColor = colorBLACKALPHA;
        }
    }

    ldtexCopyTex1toTex2Rotated(patch.Width,
                               patch.Height,
                               srcOffset,
                               &patch,
                               dstOffset,
                               &dest,
                               transparentColor,
                               TRUE);

	// Load updated texture.
	ldtexCloseWork(newTexID, 0);

	gr2ModelFindAndReplaceTexture (model, fpTPSPatchTextureIDs[countryID], newTexID);
	model->patchTexID = newTexID;

#else
    gr2ModelFindAndReplaceTexture(model, fpTPSPatchTextureIDs[countryID], decalTexID);
    piAddTexRef(decalTexID);

	model->patchTexID = decalTexID;

#ifdef USE_NEW_DECALS
    // Alpha out affiliate 1 and affiliate 2 until functionality is implemented
    _cleartestpatch(model, countryID + 1);
    _cleartestpatch(model, countryID + 2);
#endif

#endif
}

// does what it sounds like .. searches the model for a textureID original and replaces it with textureID new
// used for the face swap, but I figured make it generic..
void gr2ModelFindAndReplaceTexture(gr2ModelAlt& model, SINT32 origTexID, UINT32 replaceTexID)
{
	GR2_MODEL_LIST list = model.GetAllModels();

	for(unsigned int i = 0; i < list.size(); ++i)
		gr2ModelFindAndReplaceTexture(list.at(i), origTexID, replaceTexID);
}

void gr2ModelFindAndReplaceTexture (gr2Model* model, SINT32 origTexID, UINT32 replaceTexID)
{
	/*
	UINT32			meshIndex;
	gr2Mesh*			mesh;
	SINT32			textureIndex;

	for(meshIndex = 0; meshIndex < model->meshes.size(); ++meshIndex)
	{
		mesh = model->meshes[meshIndex];

		for (textureIndex = 0; textureIndex < mesh->textureCount; textureIndex++)
		{
			if (mesh->textureReferences[textureIndex] == origTexID)
			{
				mesh->textureReferences[textureIndex] = replaceTexID;
				// let it spin?  might as well
			}
		}
	}
	*/
}

SINT32* gr2ModelGetLoadedTextures (gr2Model* model, SINT32* numTextures)
{
	return nullptr;
	/*
	if ((!model) || (!numTextures))
		return nullptr;

	*numTextures = model->numTextures;

	return (model->loadedTextures);
	*/
}

FLOAT32 gr2BlendControlGetTime(gr2Model* model, gr2AnimationBlend* animationBlend)
{
	gr2ControlBlend* blendControl = gr2BlendControlFindRunningBlend(model, animationBlend);

	if(!blendControl)
		return 0;

	return (FLOAT32)GrannyGetControlRawLocalClock(blendControl->controls[0].control);
}

FLOAT32 gr2BlendGetBaseLength(gr2AnimationBlend* animationBlend)
{
	for(int i = 0; i < animationBlend->numBlends; i++)
	{
		if (!animationBlend->animations[i])
			return 0;

		if (animationBlend->animations[i]->animType == GR2_ANIMATION_BASELOOP)
		{
			return (animationBlend->animations[i]->animation->Duration);
		}
	}
	return 0;
}

FLOAT32 gr2BlendGetLength(gr2AnimationBlend* animationBlend)
{
	if(animationBlend->length)
		return animationBlend->length;

	for(int i = 0; i < animationBlend->numBlends; i++)
	{
		if(!animationBlend->animations[i])
			continue;

		if(animationBlend->animations[i]->animation->Duration > animationBlend->length)
			animationBlend->length = animationBlend->animations[i]->animation->Duration;
	}

	return animationBlend->length;
}

FLOAT32 gr2BlendGetAnimLength(gr2AnimationBlend* animationBlend, UINT32 index)
{
	if((int)index >= animationBlend->numBlends || !animationBlend->animations[index])
		return 0.0f;

	return animationBlend->animations[index]->animation->Duration;
}

FLOAT32 gr2BlendGetAnimTime(gr2Model* model, gr2AnimationBlend* animationBlend, UINT32 index)
{
	if((int)index >= animationBlend->numBlends || !animationBlend->animations[index])
		return 0.0f;

	gr2ControlBlend* control = gr2BlendControlFindRunningBlend(model, animationBlend);

	if(!control || !control->controls[index].control)
		return 0.0f;

	return GrannyGetControlRawLocalClock(control->controls[index].control);
}

void gr2HandleEvent (gr2Model* model, SINT32 eventID)
{
	if (model->eventHandler)
		model->eventHandler(model->pObject, model, eventID);
//	Write_Log (LOG_FPS, LOG_INFO, "gr2HandleEvent: %d", eventID);
}

void gr2ModelSetEventHandler (gr2Model* model, OBJECT* pObject, GR2_EVENTHANDLER eventHandler)
{
	model->eventHandler = eventHandler;
	model->pObject = pObject;
}

const char* gr2GetEventData(gr2Model* model, SINT32 eventID)
{
	if(eventID >= 0 && eventID < (SINT32)model->events.size())
		return model->events.at(eventID)->data;

	return 0;
}

SINT32 gr2GetEventType(gr2Model* model, SINT32 eventID)
{
	if(eventID >= 0 && eventID < (SINT32)model->events.size())
		return model->events.at(eventID)->type;

	return -1;
}

// used by fps models to init events
void gr2UpdateControlEvents(gr2Control* control)
{
	if(!control || !control->model)
		return;

	for(UINT32 i = 0; i < control->model->events.size(); i++)
		gr2FreeQueuedEvent(control->model->events[i]);

	control->model->events.clear();

	if(!control->model->eventHandler || !control->animation->events)
		return;

	for (size_t k = 0; k < control->animation->events->size(); k++)
	{
		gr2Event* e = control->animation->events->at(k);

		if(e)
		{
			gr2QueuedEvent* evt = _gr2QueuedEventPool.Pop();

			evt->type = e->eventType;
			evt->timeToOccur = e->eventTime;
			evt->prevAnimLoopTime = GrannyGetControlRawLocalClock (control->control);
			evt->control = control->control;

			strncpy(evt->data, e->eventData, MAX_EVENT_DATA);

			control->model->events.push_back(evt);
		}
	}
}

// used by tps models to init events
void gr2UpdateBlendEvents(gr2Model* model)
{
	if(!model)
		return;

	for(UINT32 i = 0; i < model->events.size(); i++)
		gr2FreeQueuedEvent(model->events[i]);

	model->events.clear();

	if(!model->blends || !model->eventHandler)
		return;

	GR2_BLEND_MAP::iterator it = model->blends->begin();

	for(; it != model->blends->end(); ++it)
	{
		gr2AnimationBlend* blend = it->first;
		gr2ControlBlend* blendControl = it->second;

		for (int j = 0; j < blend->numBlends; j++)
		{
			if(!blend->animations[j]->events)
				continue;

			for(size_t k = 0; k < blend->animations[j]->events->size(); k++)
			{
				gr2Event* e = blend->animations[j]->events->at(k);

				if(e)
				{
					gr2QueuedEvent* evt = _gr2QueuedEventPool.Pop();

					evt->type = e->eventType;
					evt->timeToOccur = e->eventTime;
					evt->prevAnimLoopTime = GrannyGetControlRawLocalClock(blendControl->controls[j].control);
					evt->control = blendControl->controls[j].control;

					strncpy(evt->data, e->eventData, MAX_EVENT_DATA);

					model->events.push_back(evt);
				}
			}
		}
	}
}

void gr2UpdateEvents(gr2Model* model)
{
	for (int i = 0; i < (int)model->events.size(); i++)
	{
		gr2QueuedEvent* evt = model->events.at(i);

		// if we've got a control for the event, check it's time based on the times we saved above
		if (evt->control && GrannyGetControlDurationLeft(evt->control) > 0.0f)
		{
			// remember, the animation clock hasn't been moved forward yet (happens below) so we use the delta time
			// computed from lastupdatetime and gameclock to determine if the event needs to be triggered

			bool doEvent = false;
			FLOAT32	currentControlTime = GrannyGetControlRawLocalClock(evt->control);
			FLOAT32	currentControlSpeed = GrannyGetControlSpeed(evt->control);

			// check for edge case
			if (currentControlSpeed >= 0)
			{
				if (currentControlTime <  evt->prevAnimLoopTime)
				{
					if (evt->timeToOccur > evt->prevAnimLoopTime || evt->timeToOccur < currentControlTime)
						doEvent = true;
				}
				// normal test
				else if ((evt->prevAnimLoopTime < evt->timeToOccur) && (currentControlTime > evt->timeToOccur))
				{
					doEvent = true;
				}
			}
			else
			{
				if (currentControlTime >  evt->prevAnimLoopTime)
				{
					if (evt->timeToOccur < evt->prevAnimLoopTime || evt->timeToOccur > currentControlTime)
						doEvent = true;
				}
				// normal test
				else if ((evt->prevAnimLoopTime > evt->timeToOccur) && (currentControlTime < evt->timeToOccur))
				{
					doEvent = true;
				}
			}

			if(doEvent)
				gr2HandleEvent(model, i);

			// update the previous anim loop time
			evt->prevAnimLoopTime = GrannyGetControlRawLocalClock(evt->control);
		}
	}
}

void gr2ModelSetTwists (gr2Model* from, gr2Model* to)
{
	for(size_t i = 0; i < from->twists.size(); i++)
	{
		gr2BoneTwist& bt = from->twists.at(i);
		to->AddBoneTwist(bt.bone, bt.axis, bt.angle);
	}
	/*
	to->modelTwist = from->modelTwist;
	to->torsoTwist = from->torsoTwist;
	to->headTwist = from->headTwist;
	to->headPitch = from->headPitch;
	*/
}

void gr2ModelSetTorsoRotation (gr2Model* model, FLOAT64 angle, LOGICAL useAltAxis)
{
	model->twists.at(1).angle = angle;
	//model->torsoTwist = angle;
	//model->useAltTorsoAxis = useAltAxis;
}

void gr2ModelSetHeadRotation (gr2Model* model, FLOAT64 angle)
{
	model->twists.at(2).angle = angle;
	//model->headTwist = angle;
}

void gr2ModelSetHeadPitch (gr2Model* model, FLOAT64 angle)
{
	model->twists.at(3).angle = angle;
	//model->headPitch = angle;
}

void gr2ModelSetModelPitch (gr2Model* model, FLOAT64 angle)
{
	//model->modelPitch = angle;
}

/*
void gr2ModelSetFeetAdjustments(gr2Model* model, FLOAT64 l, FLOAT64 r)
{
	model->leftFootAdjust = l;
	model->rightFootAdjust = r;
}
*/

FLOAT64 gr2ModelSetModelRotation (gr2Model* model, FLOAT64 angle)
{
	gr2BoneTwist& bt = model->twists.at(0);

	bt.angle = angle;

	gr2NormalizeAngle(bt.angle);

	return bt.angle;

	/*
	model->modelTwist = angle;// + 90.0;
	//model->modelTwistSpeed = (speed) ? speed: 60.0f; // little slower than the other twists

	gr2NormalizeAngle(model->modelTwist);

	return model->modelTwist;
	*/
}

FLOAT64 gr2ModelGetModelRotationDiff(gr2Model* model, FLOAT64 angle)
{
	/*
	FLOAT64 adjAngle = angle;// + 90;
	FLOAT64 diff = fabs(adjAngle - model->modelTwist);
	return diff;
	*/
	return 0.0f;
}

FLOAT64 gr2ModelGetTorsoRotationDiff(gr2Model* model, FLOAT64 angle)
{
	//return fabs(angle - model->torsoTwist);
	return 0.0f;
}

static piQUAT* _RotationToQuaternion(piQUAT* q, piVECTOR const* axis, double angle)
{
	float scale = (float)(sin(angle / 2.0f) / sqrt(axis->x * axis->x + axis->y * axis->y + axis->z * axis->z));

	q->w = (float)cos(angle / 2.0f);

	q->x = scale * axis->x;
	q->y = scale * axis->y;
	q->z = scale * axis->z;

	return q;
}

static piQUAT* _QuaternionMultiply(piQUAT* a, const piQUAT* b, const piQUAT* c)
{
	float w = b->w * c->w - b->x * c->x - b->y * c->y - b->z * c->z;
	float x = b->w * c->x + c->w * b->x + b->y * c->z - b->z * c->y;
	float y = b->w * c->y + c->w * b->y + b->z * c->x - b->x * c->z;
	float z = b->w * c->z + c->w * b->z + b->x * c->y - b->y * c->x;

	a->w = w; a->x = x; a->y = y; a->z = z;

	return a;
}

void gr2AddAxisRotationToBoneInLocalPose (granny_local_pose* pose, SINT32 boneNum, double axis[3], double angle)
{
	if(!pose) pose = localScene.SharedLocalPose;

 	granny_transform* boneTransform = GrannyGetLocalPoseTransform(pose, boneNum);

	piQUAT	a, b;
	piVECTOR qAxis;

	qAxis.x = (float)axis[0];
	qAxis.y = (float)axis[1];
	qAxis.z = (float)axis[2];

	a.x = boneTransform->Orientation[0];
	a.y = boneTransform->Orientation[1];
	a.z = boneTransform->Orientation[2];
	a.w = boneTransform->Orientation[3];

	_RotationToQuaternion (&b, &qAxis, angle);
	_QuaternionMultiply (&a, &a, &b);

	boneTransform->Orientation[0] = (granny_real32)a.x;
	boneTransform->Orientation[1] = (granny_real32)a.y;
	boneTransform->Orientation[2] = (granny_real32)a.z;
	boneTransform->Orientation[3] = (granny_real32)a.w;
}

const char* gr2GetPathFromBlend(gr2AnimationBlend* blend)
{
	static char		buffer[255+1];
	static char*	path;

	*buffer = 0;

	strncpy(buffer, blend->blendFile, 255);

	path = buffer + 4; // strip off "TPS\"

	char* p = strchr(path, '.');

    if(p) *p = 0;

	return path;
}


gr2Model* gr2SetGR2Object(gr2Model* model, gr2Object* object)
{
	model->object = object;
	return model;
}

void gr2DrawSkeleton(gr2Model* model, UINT32 color)
{
	std::vector<GR2RenderLine> lines;

	granny_matrix_4x4* boneLocs = GrannyGetWorldPose4x4Array(model->WorldPose);

	for(int i = 0; i < model->skeleton->BoneCount; i++)
	{
		if(model->skeleton->Bones[i].ParentIndex >= 0)
			lines.push_back(GR2RenderLine(boneLocs[i], boneLocs[model->skeleton->Bones[i].ParentIndex]));
	}

    glPushMatrix();
	glPushAttrib(GL_ALL_ATTRIB_BITS);
	glPushClientAttrib(GL_ALL_ATTRIB_BITS);

	//if(_world)
    //   glMultMatrixf(_world);

    glDisable(GL_LIGHTING);
    glPointSize(4.0f);
    glDisable(GL_TEXTURE_2D);
	//glDisable(GL_DEPTH_TEST);

    float pos[4];
    glGetFloatv(GL_CURRENT_RASTER_POSITION, pos);
	oglSetRenderState(oglRENDERSTATE_COLOR4F, color);
	oglSetRenderState(oglRENDERSTATE_LIGHTING, 0);

    glBegin(GL_LINES);
    {
		for(int i = 0; i < (int)lines.size(); i++)
		{
			GR2RenderLine& line = lines.at(i);

	        glVertex3f(line.from[0], line.from[1], line.from[2]);
		    glVertex3f(line.to[0], line.to[1], line.to[2]);
		}
    }
    glEnd();

	for(int i = 0; i < (int)lines.size(); i++)
	{
		GLUquadricObj* obj = gluNewQuadric();
		GR2RenderLine& line = lines.at(i);
		glTranslatef(line.from[0], line.from[1], line.from[2]);
		gluSphere(obj, .01f, 20, 20);
		glTranslatef(-line.from[0], -line.from[1], -line.from[2]);
	}

    glRasterPos3f(pos[0], pos[1], pos[2]);
    glPointSize(1.0f);
    glEnable(GL_LIGHTING);
	//glEnable(GL_DEPTH_TEST);

	glPopClientAttrib();
	glPopAttrib();
    glPopMatrix();
}

void gr2ObjectHandleEvent(OBJECT* pObject, gr2Model* model, SINT32 eventID)
{
	if(model->object)
		model->object->HandleAnimEvent(model, gr2GetEventType(model, eventID), eventID);
}

#ifdef GR2_TEXTURE_DEBUG
std::map<std::string,std::string>& gr2GetTextures()
{
	return _gr2Textures;
}
#endif

XML_TAG_HANDLER(xmlBlendTagHandler)
{
	gr2AnimationBlend* blend = XML_GET_USERDATA(gr2AnimationBlend*,blend);

	if(blend)
	{
		for(xmlNodePtr child = node->children; child; child = child->next)
		{
			if(!loader->Process(child, userData))
				break;

			blend->numBlends++;
		}
	}

	return false;
}

XML_TAG_HANDLER(xmlBlendAnimTagHandler)
{
	if(!userData) return false;

	gr2AnimationBlend* blend = XML_GET_USERDATA(gr2AnimationBlend*,blend);
	if(!blend) return false;

	xmlChar* animFile = xmlGetProp(node, (xmlChar*)"file");
	if(!animFile) return false;

	// get a pointer to the current animation address
	gr2Animation* animation = blend->animations[blend->numBlends];

	// animation will be nullptr so create it
	animation = gr2LoadAnimation((SCHAR*)animFile, 1);

	if(animation)
	{
		SINT32* bones			= XML_GET_USERDATA(SINT32*,bones);
		xmlChar* defaultWeight	= xmlGetProp(node, (xmlChar*)"default");
		xmlChar* baseLoop		= xmlGetProp(node, (xmlChar*)"baseloop");

		double weight = (defaultWeight) ? atof((const char*)defaultWeight): 1.0f;

		animation->mask = GrannyNewTrackMask((granny_real32)weight, *bones);

		if(baseLoop && !stricmp((const char*)baseLoop,"true"))
			animation->animType = GR2_ANIMATION_BASELOOP;

		XML_PROCESS_CHILDREN(animation);

		if(defaultWeight) xmlFree(defaultWeight);
		if(baseLoop) xmlFree(baseLoop);
	}

	if(animFile) xmlFree(animFile);

	return false;
}

XML_TAG_HANDLER(xmlBlendBoneTagHandler)
{
	gr2Animation* animation = (gr2Animation*)userData;

	if(animation)
	{
		xmlChar* name	= xmlGetProp(node, (xmlChar*)"name");
		xmlChar* weight = xmlGetProp(node, (xmlChar*)"weight");

		int boneIndex		= atoi((const char*)name);
		double boneWeight	= atof((const char*)weight);

		if(boneIndex >= 0 && boneIndex < MAX_GR2_BONE_COUNT)
			GrannySetTrackMaskBoneWeight(animation->mask, boneIndex, (granny_real32)boneWeight);

		if(name) xmlFree(name);
		if(weight) xmlFree(weight);
	}

	return false;
}


FLOAT64 gr2NormalizeAngle(FLOAT64& angle)
{
	while(angle * angle > 180.0 * 180.0)
	{
		if (angle < 0)
			angle += 360.0;
		else if (angle >= 0)
			angle -= 360.0;
	}

	return angle;
}

#ifdef GR2_BLEND_EDITOR
gr2Event* gr2GetNewEvent(gr2Animation* anim)
{
	if(!anim->events)
	{
		_animEvents[anim->animation] = GR2_EVENT_LIST();
		GR2_ANIM_EVENTS::iterator found = _animEvents.find(anim->animation);
		anim->events = &found->second;
	}

	gr2Event* newEvent = _gr2EventPool.Pop();

	anim->events->push_back(newEvent);

	return newEvent;
}
#endif
