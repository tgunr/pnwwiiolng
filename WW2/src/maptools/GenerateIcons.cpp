#include <syscfg.h> // Path is project specific.
#include "maptools/GenerateIcons.h"
#ifdef GENICONS
/*---------------------------------------------------------------------------
 Module: GenerateIcons.cpp
 Description:
 ----------------------------------------------------------------------------*/
#include "rn/rnmain.h"
#include "rn/rnvis.h"
#include "gmlp/gmlpmain.h"
#include "pers/persmain.h"
#include "osgImport/ref_ptr.h"
#include "ui/uiChat.h"
#include "ui/uiDotCommands.h"
#include "imap/ww2func.h"
#include "ob/obplyr.h"
#include "math/math3d.h"
#include "trn/trncd.h"
#include "sdm/sdm.h"
#include "trn/trnmain.h"
#include "pers/pershud.h"

static ref_ptr<GenerateIcons> generateIcons;

#if platOS == OS_MACOS
#define GEN_DIR "/Users/ahwulf/"
#else
#define GEN_DIR "c:\\My Documents\\"
#endif

GenerateIcons& GetIconGenerator()
{
  if(!generateIcons.valid())
  {
    generateIcons = new GenerateIcons;
  }

  return *generateIcons.get();
}

GenerateIcons::GenerateIcons() :
_enabled(false),
altitude(SDM_OCTET_SIZE_2),
doObjects(true)
{
  TerrainObjectSpecials.insert("infantrypit1");
  TerrainObjectSpecials.insert("atgunpit1");
  TerrainObjectSpecials.insert("farmruin");
}

GenerateIcons::~GenerateIcons()
{
}

// mark the ones we want
void GenerateIcons::markAllIconifiedModels()
{
  for ( PoolMap::const_iterator it = obMgrGetPoolMap()->begin(); it!=obMgrGetPoolMap()->end(); it++ )
  {
    std::string name = 	it->first;
    PoolModel* model = 	it->second;
    UINT32 mt = model->modelType;
    bool mark = false;

    if ( doObjects )
    {
      if ( mt == ModelTypes::BUILDING
          || mt == ModelTypes::AUTOGUNNER
          || mt == ModelTypes::BRIDGE
          || mt == ModelTypes::CONTROL_POINT
          || mt == ModelTypes::FACTORY )
      {
        mark = true;
      }
      else if ( mt == ModelTypes::TERRAIN_OBJECT )
      {
        if ( TerrainObjectSpecials.find(name) != TerrainObjectSpecials.end() )
        {
          mark = true;
        }
      }
    }
    else
    {
      if ( mt == ModelTypes::TERRAIN_TILE )
      {
        mark = true;
      }
    }

    if ( mark )
    {
      model->propertyFlags |= PoolModel::PropertyFlags::MARK_FOR_ICON;
    }
  }
}

void GenerateIcons::Start(bool objects)
{
  _enabled = true;
  doObjects = objects;

  altitude = doObjects ? SDM_OCTET_SIZE_2 : 800;

  _oldpostfilter = g_postRenderFilter;
  _oldshadowmapflags = ShadowMapFlags;
  _oldradclutter = g_bRadialClutter;
  _oldwatershader = gWaterShadersEnabled;

  g_postRenderFilter = 0;
  ShadowMapFlags = 0;
  g_bRadialClutter = 0;
  gWaterShadersEnabled = 0;
  bClearViewport = TRUE;
  trnmainSetDraw(FALSE);

  markAllIconifiedModels();

  _poolitr = obMgrGetPoolMap()->begin();

  if(HUDVisible)
    persHUDToggleHUD(1, IMAP_MOM_TOGGLE_HUD, 0);
}

static void DebugStop(GLenum err)
{
//  printf("err=%u\n",err);
}

static bool compare( texture_data a, texture_data b )
{
  return a.drawHeight < b.drawHeight;
}

#define CHECK_ERR { err = glGetError(); if ( err!=0 ) {DebugStop(err);} }

void GenerateIcons::MakeAtlas()
{
  GLuint fbo, tex;
  GLenum err;
  glPushAttrib(GL_ALL_ATTRIB_BITS);

  glBindTexture(GL_TEXTURE_2D, 0);

  glGenTextures(1,&tex); CHECK_ERR
  glBindTexture(GL_TEXTURE_2D, tex); CHECK_ERR
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 2048, 2048, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr); CHECK_ERR

  glGenFramebuffersEXT(1, &fbo); CHECK_ERR
  glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, fbo ); CHECK_ERR
  glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_2D, tex, 0); CHECK_ERR

  unsigned char* background = (unsigned char*)calloc(1,2048 * 2048 * 4 * sizeof(unsigned char) );

  GLenum status;
  status = glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT);
  if (status != GL_FRAMEBUFFER_COMPLETE_EXT ) DebugStop(status);

  glDrawBuffer(GL_COLOR_ATTACHMENT0_EXT); CHECK_ERR

  glClearColor( 1, 1, 1, 0 );  CHECK_ERR

  oglSetViewport(0,0,2048, 2048);
  glMatrixMode(GL_PROJECTION);  CHECK_ERR
  glLoadIdentity();
  glOrtho(0.0, 2048, 0.0, 2048, -1.0, 1.0);
  glMatrixMode(GL_MODELVIEW);  CHECK_ERR
  glLoadIdentity();

  glEnable(GL_TEXTURE_2D);  CHECK_ERR
  glDisable(GL_BLEND);  CHECK_ERR
  glDisable(GL_DEPTH_TEST);  CHECK_ERR
  glDisable(GL_LIGHTING);  CHECK_ERR
  //glBlendFunc(GL_SRC_COLOR, GL_ZERO);
  glBlendFunc(GL_SRC_COLOR, GL_ZERO);  CHECK_ERR

  glTexEnvf(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_REPLACE);  CHECK_ERR

  glClear(GL_COLOR_BUFFER_BIT);  CHECK_ERR
  //glColor3ub( 0, 255, 0 );

  unsigned int curH=0, curW=0, maxH = 0, count = 0;
  std::vector<texture_data>::iterator it;

  for (it = textures.begin(); it != textures.end(); it++)
  {
    int W=it->width, H=it->height;
    float M = it->mpp;

    if ( doObjects )
    {
      while ( W > 512 || H > 512 )
      {
        W /= 2;
        H /= 2;
        M *= 2;
      }
    }
    else
    {
      float f = (float)W / 112.0f; // tiles are square
      W /= (int)f;
      H /= (int)f;
      M *= (int)f;
    }

    it->drawWidth = W;
    it->drawHeight = H;
    it->drawMpp = M;
  }

  sort(textures.begin(),textures.end(),compare);

  std::string xyInfo;
  std::string atlasInfo;

  for (it = textures.begin(); it != textures.end(); it++, count++)
  {
    glColor4f(1, 1, 1, 1);  CHECK_ERR
		glBindTexture(GL_TEXTURE_2D, it->texid);  CHECK_ERR

    int W=it->drawWidth, H=it->drawHeight;

    if ( curW+W >= 2048 )
    {
      curW = 0;
      curH += maxH+2;
      maxH = H;
    }
    else
    {
      if ( (int)maxH < H )
      {
        maxH = H;
      }
    }

    if ( curH+H >= 2048 )
    {
      printf("too big = %d\n",count);
      break;
    }

    glBegin(GL_QUADS);

    glVertex2f((float)(0+curW), (float)(0+curH)); glTexCoord2f(0.0f, 1.0f);
    glVertex2f((float)(0+curW), (float)(H+curH)); glTexCoord2f(1.0f, 1.0f);
    glVertex2f((float)(W+curW), (float)(H+curH)); glTexCoord2f(1.0f, 0.0f);
    glVertex2f((float)(W+curW), (float)(0+curH)); glTexCoord2f(0.0f, 0.0f);

    glEnd(); CHECK_ERR

    float l,r,t,b;

    char s[256];
    l = (0+curW) / 2048.0f;
    r = (W+curW) / 2048.0f;
    t = (0+curH) / 2048.0f;
    b = (H+curH) / 2048.0f;
    strprintf(s, "%s_icon %.6f %.6f %.6f %.6f\n", it->name.c_str(), l, t, r, b );
    atlasInfo.append(s);

    strprintf(s, "%s_icon %d %d %.6f\n", it->name.c_str(), int(it->minx), int(it->miny), it->drawMpp );
    xyInfo.append(s);

    curW += W+2;
  }

  glReadBuffer(GL_COLOR_ATTACHMENT0_EXT); CHECK_ERR

  glReadPixels(0, 0, 2048, 2048, GL_RGBA, GL_UNSIGNED_BYTE, background); CHECK_ERR

  glPopAttrib();

  unsigned char* src = background;  // convert to ARGB for GraphicConvertor
  for ( int row=0; row<2048; row++)
  {
    for ( int col=0; col<2048; col++)
    {
      unsigned char a = src[3];
      src[3] = src[2];
      src[2] = src[1];
      src[1] = src[0];
      src[0] = a;
      src += 4;
    }
  }

  char* path = GEN_DIR"icons.raw";
  FILE*	cf = fopen(path, "wb");
  if ( cf == nullptr )
  {
    printf("File Error: %d\n",errno );
  }
  else
  {
    fwrite(background, 1, 2048 * 2048 * 4 * sizeof(unsigned char), cf);
    fclose(cf);
  }

  if ( doObjects )
  {
    path = GEN_DIR"icons.xy";
    cf = fopen(path, "w");
    if ( cf == nullptr )
    {
      printf("File Error: %d\n",errno );
    }
    else
    {
      fwrite(xyInfo.c_str(), 1, strlen(xyInfo.c_str()), cf);
      fclose(cf);
    }
  }

  path = GEN_DIR"icons.atlas";
  cf = fopen(path, "w");
  if ( cf == nullptr )
  {
    printf("File Error: %d\n",errno );
  }
  else
  {
    fwrite(atlasInfo.c_str(), 1, strlen(atlasInfo.c_str()), cf);
    fclose(cf);
  }

  glBindTexture(GL_TEXTURE_2D, 0);
  glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
  free(background);

}

/*---------------------------------------------------------------------------
 Function: GenerateIcons::End()
 Description:
 ----------------------------------------------------------------------------*/
void GenerateIcons::End()
{
  _enabled = false;
  bClearViewport = FALSE;

  MakeAtlas();

  oglLockViewMatrix(0);
  oglLockProjectionMatrix(0);
  oglLockWorldMatrix(0);

  g_postRenderFilter = _oldpostfilter;
  ShadowMapFlags = _oldshadowmapflags;
  g_bRadialClutter = _oldradclutter;
  gWaterShadersEnabled = _oldwatershader;

  oglLockState(oglRENDERSTATE_LIGHTING, 0);
  oglLockState(oglRENDERSTATE_FOGENABLE, 0);

  gmlpHandleFieldOfView();
  rnvisSetupClipPlanes(true);
  oglSetRenderState(oglRENDERSTATE_CLEARCOLOR, FogColor);
  oglSetRenderState(oglRENDERSTATE_LIGHTING, piLIGHTING_DEFAULT);

  trnmainSetDraw(TRUE);

  // Clear anything leftovers
 // uiDotCommand(".tspawn");
}

/*---------------------------------------------------------------------------
 Function: GenerateIcons::Apply()
 Description:
 ----------------------------------------------------------------------------*/
void GenerateIcons::Apply()
{
  GLenum err;

  oglSetWorldMatrix(&IdentityMatrix);

  mlVehicleData* data = persGetPersonaModelDataPtr();

  data->VehiclePos.x = 0;
  data->VehiclePos.y = 0;
  data->VehiclePos.z = altitude;

  static float pitch = PI/2;

  data->VehicleRealAtt.Roll = 0;
  data->VehicleRealAtt.Yaw = 0;
  data->VehicleRealAtt.Pitch = pitch;

  data->VehicleAtt.Roll = 0;
  data->VehicleAtt.Yaw = 0;
  data->VehicleAtt.Pitch = pitch;

  rnUpdateView();

  // Orthogonal culling planes
  LeftPlaneX = -1.0f;
  LeftPlaneZ = -1.0f;
  TopPlaneY = 1.0f;
  TopPlaneZ = -1.0f;

  //oglLockViewMatrix(1);
  oglLockProjectionMatrix(1);
  //oglLockWorldMatrix(1);

  oglSetRenderState(oglRENDERSTATE_CLEARCOLOR, colorBLACK);CHECK_ERR
  //oglSetRenderState(oglRENDERSTATE_ANTIALIAS, 0);CHECK_ERR
  oglSetRenderState(oglRENDERSTATE_ALPHABLENDENABLE, FALSE);
  oglSetRenderState(oglRENDERSTATE_FOGENABLE, 0);
  oglSetRenderState(oglRENDERSTATE_LIGHTING, 1);

  //oglSetRenderState(oglRENDERSTATE_FOGENABLE, 0);
  //oglSetRenderState(oglRENDERSTATE_LIGHTING, 0);
  //oglSetRenderState(oglRENDERSTATE_ANTIALIAS, 0);CHECK_ERR
  //oglSetRenderState(oglRENDERSTATE_COLOR4F, colorGREENALPHA );
  //oglSetRenderState(oglRENDERSTATE_TEXTUREPERSPECTIVE, 0 );
  //oglSetRenderState(oglRENDERSTATE_ZENABLE, FALSE);
  //oglSetRenderState(oglRENDERSTATE_CULLFACEENABLE, 0);
  //oglSetRenderState(oglRENDERSTATE_ALPHABLENDENABLE, FALSE);

  //oglLockState(oglRENDERSTATE_LIGHTING, 1);
  //oglLockState(oglRENDERSTATE_FOGENABLE, 1);
  //oglLockState(oglRENDERSTATE_COLOR4F, 1);

  //glEnable(GL_LIGHTING);
  //glEnable(GL_LIGHT0);
  //glDisable(GL_LIGHT1);

  // Create light components

  // Assign created components to GL_LIGHT0
  //glLightfv(GL_LIGHT0, GL_AMBIENT, ambientLight);
  //glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuseLight);

  oglLockState(oglRENDERSTATE_LIGHTING, 1);
  oglLockState(oglRENDERSTATE_CLEARCOLOR, 1);
  //oglLockState(oglRENDERSTATE_ANTIALIAS, 1);
  oglLockState(oglRENDERSTATE_ALPHABLENDENABLE, 1);
  oglLockState(oglRENDERSTATE_FOGENABLE, 1);

  GLfloat LightAmbient[]  = {1.0f, 1.0f, 1.0f, 1.0f};  // Gray (constant factor)
  GLfloat LightDiffuse[]  = {1.0f, 1.0f, 1.0f, 1.0f};  // White (all directions)
  GLfloat LightPosition[] = {0.0f, 0.0f, 2.0f, 1.0f};  // behind camera (relative to camera)

  glLightfv(GL_LIGHT0, GL_AMBIENT,  LightAmbient);  // Setup the ambient component
  glLightfv(GL_LIGHT0, GL_DIFFUSE,  LightDiffuse);  // Setup the diffuse component
  glLightfv(GL_LIGHT0, GL_POSITION, LightPosition);
  glEnable(GL_LIGHT0);

}

static unsigned int nextPowerOf2(unsigned int v )
{
  v--;
  v |= v >> 1;
  v |= v >> 2;
  v |= v >> 4;
  v |= v >> 8;
  v |= v >> 16;
  v++;
  return v;
}

static unsigned int nextMultipleOf4(unsigned int v )
{
  if (v&3)
  {
    v &= ~3;
    v+=4;
  }
  return v;
}

/*---------------------------------------------------------------------------
 Function: GenerateIcons::Capture()
 Description:
 ----------------------------------------------------------------------------*/
void GenerateIcons::Capture()
{
  GLenum err;

  glPopAttrib(); CHECK_ERR

  if(_poolitr == obMgrGetPoolMap()->end())
  {
    End();
    return;
  }

  double minx = bb.xMin();
  double maxx = bb.xMax();
  double miny = bb.yMin();
  double maxy = bb.yMax();
  double zz;

  LOGICAL lll = oglGetWorldToScreen((double)bb.xMin(), (double)bb.yMin(), 0.0, minx, miny, zz);
  lll = oglGetWorldToScreen((double)bb.xMax(), (double)bb.yMax(), 0.0, maxx, maxy, zz);

  float w = (float)(maxx-minx);
  float h = (float)(maxy-miny);
  unsigned int sx = WantedWidth / 2 - (unsigned int)(w / 2);
  unsigned int sy = WantedHeight / 2 - (unsigned int)(h / 2);

  if ( sx >= 0 && sy >= 0 && w > 1 && h > 1 )
  {
    sx = nextMultipleOf4(sx)-4;
    sy = nextMultipleOf4(sy)-4;

    unsigned int sw = 12+nextMultipleOf4(((unsigned int) w));
    unsigned int sh = 12+nextMultipleOf4(((unsigned int) h));
    if ( sw < 8 ) sw = 8;
    if ( sh < 8 ) sh = 8;

    SaveToTexture(_poolitr->first.c_str(), sx, sy, sw,  sh, bb.xMin(), bb.yMin());
  }

  ++_poolitr;
}

static int k= 0;

void GenerateIcons::SaveToTexture(const char*name, int x, int y, int w, int h, double minx, double miny)
{
  GLenum err;

  unsigned char* bitmapA = (unsigned char*)memAlloc(w * h * 4 * sizeof(unsigned char) );
  glReadPixels(x, y, w, h, GL_RGBA, GL_UNSIGNED_BYTE, bitmapA); CHECK_ERR

  texture_data tx;
  tx.name = name;
  tx.width = w;
  tx.height = h;
  tx.minx = (float)minx;
  tx.miny = (float)miny;
  glGenTextures(1,&tx.texid); CHECK_ERR
  glBindTexture(GL_TEXTURE_2D, tx.texid); CHECK_ERR
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, bitmapA); CHECK_ERR

  textures.push_back(tx);

  //if ( doObjects )
  {
    unsigned char* src = bitmapA;
    for ( int row=0; row<h; row++)
    {
      for ( int col=0; col<w; col++)
      {
        unsigned char a = src[3];
        src[3] = src[2];
        src[2] = src[1];
        src[1] = src[0];
        src[0] = a;
        src += 4;
      }
    }

    char filename[256];
	strprintf(filename, "%s/%s-%dx%d.raw", GEN_DIR"tmp", _poolitr->first.c_str(),w, h );
    FILE* cf = fopen(filename, "wb+");
	if ( !cf ) {
		LogAbort("Couldn't save texture file %s", filename);
	}
    fwrite(bitmapA, 1, w * h * 4 * sizeof(unsigned char), cf);
    fclose(cf);
  }


  //memFree(bitmap);
  memFree(bitmapA);

}

using mapOBJList = std::map<OBJECT*, trnCDTerrainObject *>;
extern  mapOBJList mapOBJ; // All objects associated with a TO

bool GenerateIcons::wantModel(UINT32 mt)
{
  return doObjects ?
  (
    mt == ModelTypes::BUILDING
    || mt == ModelTypes::AUTOGUNNER
    || mt == ModelTypes::BRIDGE
    || mt == ModelTypes::CONTROL_POINT
    || mt == ModelTypes::FACTORY
    //|| mt == ModelTypes::TERRAIN_OBJECT
   )
  :
  (
    mt == ModelTypes::TERRAIN_TILE
   )
 ;
}

/*---------------------------------------------------------------------------
 Function: GenerateIcons::SetToOrigin()
 Description:
 ----------------------------------------------------------------------------*/
void GenerateIcons::SetToOrigin(float& x, float& y)
{
  bb.init();
  for (mapOBJList::iterator it = mapOBJ.begin(); it!= mapOBJ.end(); it++ )
  {
    OBJECT* oo = it->first;
    trnCDTerrainObject* ttt = it->second;
    if ( ttt->TerrainMasterObject->modelInstance->getPropertyFlags() & PoolModel::PropertyFlags::MARK_FOR_ICON )
    {
      bb.expandBy( ttt->BBox1.x, ttt->BBox1.y, ttt->BBox1.z );
      bb.expandBy( ttt->BBox2.x, ttt->BBox2.y, ttt->BBox2.z );
    }
  }

  x = bb.center().x();
  y = bb.center().y();
}

/*---------------------------------------------------------------------------
 Function: GenerateIcons::LoadModel()
 Description:
 ----------------------------------------------------------------------------*/
void GenerateIcons::LoadModel()
{
  GLenum err;

  for(; _poolitr != obMgrGetPoolMap()->end(); ++_poolitr)
  {
    PoolModel* model = 	_poolitr->second;

    //printf("%-40s %d\n", _poolitr->first.c_str(), model->modelType );

    if( model->propertyFlags & PoolModel::PropertyFlags::MARK_FOR_ICON )
    {
      if ( _poolitr->first == "cellheightmap" )
      {
        continue;
      }

      glPushAttrib(GL_ALL_ATTRIB_BITS);  CHECK_ERR
//      oglSetRenderState(oglRENDERSTATE_COLOR4F, colorBLACKALPHA );

      std::string cmd = ".tspawn ";
      cmd += _poolitr->first.c_str();

      uiDotCommand(cmd.c_str());

      printf("%40s ", _poolitr->first.c_str() );

      float x=0, y=0;
      SetToOrigin(x, y);
      if ( (!doObjects) && abs(x)>1 )
      {
        glPopAttrib(); CHECK_ERR
        printf("\n");
        continue;
      }

      printf("%10.2f %10.2f\n", -x*2, -y*2);

      mlVehicleData* data = persGetPersonaModelDataPtr();

      data->VehiclePos.x = x;
      data->VehiclePos.y = y;
      data->VehiclePos.z = altitude;

      static float pitch = PI/2;

      data->VehicleRealAtt.Roll = 0;
      data->VehicleRealAtt.Yaw = 0;
      data->VehicleRealAtt.Pitch = pitch;

      data->VehicleAtt.Roll = 0;
      data->VehicleAtt.Yaw = 0;
      data->VehicleAtt.Pitch = pitch;

      ft50msticks = 1;

      rnUpdateView();

      trnUpdateTerrainObjectLODs(1);

      // Iterator incremented after capture

      break;
    }
  }

  if(_poolitr == obMgrGetPoolMap()->end())
  {
    End();
  }
}

#endif

#if platOS == OS_WIN32
#ifdef BETA_RELEASE
#include "osgImport/ref_ptr.h"
#include "pers/persmain.h"
#include "rn/rnmain.h"
#include "ob/obmain.h"
#include "ob/obMgr.h"
#include "osgImport/BoundingBox.h"
#include "ob/obdraw.h"
#include "pers/pershud.h"
#include "ui/uiChat.h"

#define destw   1024
#define desth   1024

static ref_ptr<GenerateScenarioMap> mapgen;

extern HWND myhwnd;
extern bool writeBMPImage(const Image& img);

/*---------------------------------------------------------------------------------
Function: GenerateScenarioMap::GetInstance
Description:
----------------------------------------------------------------------------------*/
GenerateScenarioMap& GenerateScenarioMap::GetInstance()
{
    if(!mapgen.valid())
        mapgen = new GenerateScenarioMap;

    return *mapgen.get();
}

/*---------------------------------------------------------------------------------
Function: GenerateScenarioMap::StartRenderDefaults
Description:
----------------------------------------------------------------------------------*/
void GenerateScenarioMap::StartRenderDefaults()
{
    extern void DisableParticleDraw(bool draw);

    DisableParticleDraw(true);

    oldpostfilter = g_postRenderFilter;
    oldshadowflags = ShadowMapFlags;
    oldwaterreflections = g_wantWaterReflections;

    g_postRenderFilter = 0;
    ShadowMapFlags = 0;
    positionindex = 0;
    listindex = 0;
    bClearViewport = TRUE;

    // Orthogonal culling planes
    LeftPlaneX = -1.0f;
    LeftPlaneZ = -1.0f;
    TopPlaneY = 1.0f;
    TopPlaneZ = -1.0f;

    oglSetViewport(0, 0, destw, desth);
    oglLockViewport(true);

    oglSetRenderState(oglRENDERSTATE_FOGENABLE, 0);
    oglLockState(oglRENDERSTATE_FOGENABLE, 1);

    LIGHT* sun = obdliteFindDynaLight("sun");
    oldsun = *sun;

    sun->light.dvDirection.set(0, 0, -1);
    sun->light.dcvColor.set(1, 1, 1, 1);
    sun->light.dcvAmbient.set(1, 1, 1, 1);

    oglSetLight(0, sun);

    oglLockState(oglRENDERSTATE_LIGHTING, 1);

    SCEN_MGR->SetEditMode(false);
    persHUDHideHUD();
    uiChatHide();

    state = GENMAP_GET_CAM_POSITIONS;
}

/*---------------------------------------------------------------------------------
Function: GenerateScenarioMap::EndRenderDefaults
Description:
----------------------------------------------------------------------------------*/
void GenerateScenarioMap::EndRenderDefaults()
{
    extern void DisableParticleDraw(bool draw);

    DisableParticleDraw(false);

    g_postRenderFilter = oldpostfilter;
    ShadowMapFlags = oldshadowflags;
    g_wantWaterReflections = oldwaterreflections;

    oglLockProjectionMatrix(0);
    oglLockViewMatrix(0);

    oglLockState(oglRENDERSTATE_LIGHTING, 0);
    oglLockState(oglRENDERSTATE_FOGENABLE, 0);

    oglSetRenderState(oglRENDERSTATE_FOGENABLE, piFOGENABLE_DEFAULT);

    LIGHT* sun = obdliteFindDynaLight("sun");
    memcpy(sun, &oldsun, sizeof(LIGHT));

    oglSetLight(0, sun);

    oglLockViewport(false);
    oglSetViewport(0, 0, WantedWidth, WantedHeight);

    positionindex = 0;
    listindex = 0;
    bClearViewport = FALSE;
    list.clear();

    state = GENMAP_GET_NUMBER_OF_TERRAIN_TILES;
}

/*---------------------------------------------------------------------------------
Function: GenerateScenarioMap::GetOctetCamPositions
Description:
----------------------------------------------------------------------------------*/
void GenerateScenarioMap::GetOctetCamPositions()
{
    if(listindex >= (int)list.size())
    {
        state = GENMAP_SAVE_TEXTURES;
        return;
    }

    positionindex = 0;
    positions.clear();

    BoundingBox bb;

    std::string model = list[listindex++]->name;

    OBJECT* o = obmainFindTopmostObject(model.c_str());

    if(o && o->modelInstance)
    {
        bb.expandBy(o->modelInstance->GetBounds());
    }

    // Set camera positions based on user resolution
    if(bb.valid())
    {
        Vec3f pos;

        float x = bb.xMin();
        float y = bb.yMax();
        float halfres = resolution * 0.5f;

        bool done = false;

        while(!done)
        {
            pos.x() = x + halfres;
            pos.y() = y - halfres;
            pos.z() = -halfres;

            positions.push_back(pos);

            x += resolution;

            if(x + 10.0f >= bb.xMax())
            {
                x = bb.xMin();
                y -= resolution;
            }

            if(y - 10.0f <= bb.yMin())
            {
                done = true;
            }
        }
    }

    state = GENMAP_MOVE_CAMERA;
}

/*---------------------------------------------------------------------------------
Function: GenerateScenarioMap::MoveCamera
Description:
----------------------------------------------------------------------------------*/
void GenerateScenarioMap::MoveCamera()
{
    if(positionindex >= (int)positions.size())
    {
        state = GENMAP_GET_CAM_POSITIONS;
        return;
    }

    mlVehicleData* data = persGetPersonaModelDataPtr();

    oglLockProjectionMatrix(0);
    oglLockViewMatrix(0);

    Vec3f pos = positions[positionindex++];

    data->VehiclePos.x = pos.x();
    data->VehiclePos.y = pos.y();
    data->VehiclePos.z = -pos.z();

    static float pitch = PI/2;

    data->VehicleRealAtt.Roll = 0;
    data->VehicleRealAtt.Yaw = 0;
    data->VehicleRealAtt.Pitch = pitch;

    data->VehicleAtt.Roll = 0;
    data->VehicleAtt.Yaw = 0;
    data->VehicleAtt.Pitch = pitch;

    rnUpdateView();

    float height = resolution * 0.5f;

    float zfar = height * 2.0f;
    float znear = -1.0f;

    float top = height;
    float bottom = -height;
    float right = height;
    float left = -height;

    oglSetOrthoProjectionMatrix(right, left, top, bottom, znear, zfar);

    oglLockProjectionMatrix(1);
    oglLockViewMatrix(1);

    state = GENMAP_SCREEN_CAPTURE;
}

/*---------------------------------------------------------------------------------
Function: GenerateScenarioMap::SaveImages
Description:
----------------------------------------------------------------------------------*/
void GenerateScenarioMap::SaveImages()
{
    for(unsigned int i = 0; i < images.size(); i++)
    {
        writeBMPImage(*images[i]);
        images[i]->unref();
    }

    images.clear();

    state = GENMAP_END;
}

/*---------------------------------------------------------------------------------
Function: GenerateScenarioMap::ScreenCapture
Description:
----------------------------------------------------------------------------------*/
void GenerateScenarioMap::ScreenCapture()
{
    ref_ptr<Image> srcimg = new Image;

    // Read image from buffer
    srcimg->allocateImage(destw, desth, 1, GL_RGB, GL_UNSIGNED_BYTE);

    char buf[256];
    strprintf(buf, "%s_%s%d.bmp", SCEN_MGR->MapName(), list[listindex - 1]->name.c_str(), positionindex);

    srcimg->setFileName(buf);
    srcimg->ref();

    glReadPixels(0, 0, destw, desth, GL_RGB, GL_UNSIGNED_BYTE, srcimg->getMipmapData(0));

    images.push_back(srcimg.get());

    state = GENMAP_MOVE_CAMERA;
}

/*---------------------------------------------------------------------------------
Function: GenerateScenarioMap::Apply
Description:
----------------------------------------------------------------------------------*/
void GenerateScenarioMap::Apply()
{
    if(start && obdrawIsRenderPass(RENDER_SCENE))
    {
        switch(state)
        {
        case GENMAP_GET_NUMBER_OF_TERRAIN_TILES:
            {
                SCEN_MGR->FindObjects(list, ScenarioObject::TERRAIN);

                if(!list.empty())
                {
                    state = GENMAP_SET_DEFAULTS;
                }

                break;
            }
        case GENMAP_SET_DEFAULTS:
            {
                StartRenderDefaults();
                break;
            }
        case GENMAP_GET_CAM_POSITIONS:
            {
                GetOctetCamPositions();
                break;
            }
        case GENMAP_MOVE_CAMERA:
            {
                MoveCamera();
                break;
            }
        case GENMAP_SCREEN_CAPTURE:
            {
                ScreenCapture();
                break;
            }
        case GENMAP_SAVE_TEXTURES:
            {
                SaveImages();
                break;
            }
        case GENMAP_END:
            {
                End();
                EndRenderDefaults();
                break;
            }
        }
    }
}
#endif // BETA_RELEASE
#endif // platOS
