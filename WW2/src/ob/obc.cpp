#include <syscfg.h> // Path is project specific.

#include "pi/pi.h"
#include "include/vars.h"
#include "mem/mem.h"
#include "fio/fio.h"
#include "ob/obc.h"
#include "ob/obmain.h"
#include "ob/obsph.h"
#include "ld/ldpix.h"
#include "ld/ldbmp.h"
#include "str/strpool.h"
#include "libxml/parser.h"
#include "libxml/tree.h"
#include "ogl/ogl.h"
#include "math/SuperFastHash.h"

#define CREATE_MATERIAL_XML
#undef CREATE_MATERIAL_XML

#ifdef CREATE_MATERIAL_XML
#include <string>
#endif

extern SINT32 DefaultMaterialId;

static SINT32 xmlinputmatch(const SCHAR* name) {return 1;}

void _materialElements(xmlNodePtr root)
{
    xmlNodePtr node = root;

    MATERIAL mat;

    memset(&mat, 0, sizeof(MATERIAL));

    piCOLORVALUE* d = &mat.material.diffuse;
    piCOLORVALUE* s = &mat.material.specular;
    piCOLORVALUE* a = &mat.material.ambient;
    piCOLORVALUE* e = &mat.material.emissive;

	for(; node != NULL; node = node->next)
	{
        if(node->type != XML_ELEMENT_NODE)
        {
            continue;
        }

        if(!xmlStrcmp((xmlChar*)"MATNAME", node->name))
        {
            xmlChar* str = xmlNodeGetContent(node);

            if(str)
            {
                strncpy(mat.name, (char*)str, 32);
                xmlFree(str);
            }

            continue;
        }

        if(!xmlStrcmp((xmlChar*)"TEXNAME", node->name))
        {
            xmlChar* str = xmlNodeGetContent(node);

            if(str)
            {
                strncpy(mat.texname, (char*)str, _MAX_PATH);
                xmlFree(str);
            }

            continue;
        }

        if(!xmlStrcmp((xmlChar*)"ALPHANAME", node->name))
        {
            xmlChar* str = xmlNodeGetContent(node);

            if(str)
            {
                strncpy(mat.alphaname, (char*)str, _MAX_PATH);
                xmlFree(str);
            }

            continue;
        }

        if(!xmlStrcmp((xmlChar*)"DIFFUSE", node->name))
        {
            xmlChar* str = xmlNodeGetContent(node);

            if(str)
            {
                sscanf((char*)str, "%f %f %f %f", &d->r, &d->g, &d->b, &d->a);
                xmlFree(str);
            }

            continue;
        }

        if(!xmlStrcmp((xmlChar*)"AMBIENT", node->name))
        {
            xmlChar* str = xmlNodeGetContent(node);

            if(str)
            {
               sscanf((char*)str, "%f %f %f %f", &a->r, &a->g, &a->b, &a->a);
               xmlFree(str);
            }

            continue;
        }

        if(!xmlStrcmp((xmlChar*)"EMISSIVE", node->name))
        {
            xmlChar* str = xmlNodeGetContent(node);

            if(str)
            {
                sscanf((char*)str, "%f %f %f %f", &e->r, &e->g, &e->b, &e->a);
                xmlFree(str);
            }

            continue;
        }

        if(!xmlStrcmp((xmlChar*)"SPECULAR", node->name))
        {
            xmlChar* str = xmlNodeGetContent(node);

            if(str)
            {
                sscanf((char*)str, "%f %f %f %f", &s->r, &s->g, &s->b, &s->a);
                xmlFree(str);
            }

            continue;
        }

        if(!xmlStrcmp((xmlChar*)"SHININESS", node->name))
        {
            xmlChar* str = xmlNodeGetContent(node);

            if(str)
            {
                mat.material.power = (float)atof((char*)str);
                xmlFree(str);
            }

            continue;
        }
    }

    obcMaterial(&mat);
}

#ifdef MAP_MATS

typedef std::map<std::string,std::string> MaterialMapper;
MaterialMapper materialMap;

void _materialMapTree(xmlNodePtr root)
{
    xmlNodePtr node = root;

	for(; node != NULL; node = node->next)
	{
        if(node->type != XML_ELEMENT_NODE)
        {
            continue;
        }

        if(!xmlStrcmp((xmlChar*)"map", node->name))
        {
			xmlChar* from = xmlGetProp(node, (xmlChar*)"from");
			xmlChar* to = xmlGetProp(node, (xmlChar*)"to");

            if ( from && to )
            {
                materialMap[(const char*)from]=(const char*)to;
            }

			if(from)
			{
				xmlFree(from);
			}
			if(to)
			{
				xmlFree(to);
			}

        }
	}
}

LOGICAL materialsMapInit()
{
    xmlRegisterInputCallbacks(xmlinputmatch,
                              (xmlInputOpenCallback)fioOpen,
                              (xmlInputReadCallback)fioRead,
                              (xmlInputCloseCallback)fioClose);

    SCHAR* name = "materialmap.xml";

    xmlDocPtr doc = xmlReadFile(name, NULL, XML_PARSE_DTDLOAD | XML_PARSE_RECOVER);

    if(doc)
    {
	    xmlNodePtr root = xmlDocGetRootElement(doc);

        if(!root)
        {
            return FALSE;
        }

        if(!xmlStrcmp((xmlChar*)"materialmap", root->name))
        {
            _materialMapTree(root->children);
        }

	    xmlFreeDoc(doc);
    }

    xmlCleanupInputCallbacks();

    for ( MaterialMapper::iterator it = materialMap.begin(); it!=materialMap.end(); it++ )
    {
        printf( "map %s to %s\n", it->first.c_str(), it->second.c_str() );
    }

    return TRUE;
}

#endif

void _materialTree(xmlNodePtr root)
{
    xmlNodePtr node = root;

	for(; node != NULL; node = node->next)
	{
        if(node->type != XML_ELEMENT_NODE)
        {
            continue;
        }

        if(!xmlStrcmp((xmlChar*)"MATERIAL", node->name))
        {
            _materialElements(node->children);
        }
	}
}

LOGICAL obcInit()
{
    // Load materials from materials.xml
    xmlRegisterInputCallbacks(xmlinputmatch,
                             (xmlInputOpenCallback)fioOpen,
                             (xmlInputReadCallback)fioRead,
                             (xmlInputCloseCallback)fioClose);

    SCHAR* name = "materials.xml";

    xmlDocPtr doc = xmlReadFile(name, NULL, XML_PARSE_DTDLOAD | XML_PARSE_RECOVER);

    if(doc)
    {
	    xmlNodePtr root = xmlDocGetRootElement(doc);

        if(!root)
        {
            return FALSE;
        }

        if(!xmlStrcmp((xmlChar*)"MATERIALS", root->name))
        {
            _materialTree(root->children);
        }

	    xmlFreeDoc(doc);
    }

    xmlCleanupInputCallbacks();

    return TRUE;
}

// callback routine that sets player location and direction
LOGICAL obcPlayerInfo(piVECTOR* lppos, piVECTOR* lprot)
{
	return(TRUE);
}

// callback routine that creates a light
LOGICAL obcLight(LIGHT* light)
{
	if (piCreateLight(light) == -1)
	{
		return(FALSE);
	}

	return(TRUE);
}

// callback routine that creates a material
#include "math/SuperFastHash.h"
LOGICAL obcMaterial(MATERIAL* lpmaterial)
{
LOGICAL		retval=TRUE;
int			nummats, matid;
MATERIAL*	lpmats;

	matid = piFindMaterialName(lpmaterial->name);

#ifdef CREATE_MATERIAL_XML
    if(!piIsMaterial(matid))
    {
        // Define CREATE_MATERIAL_XML
        // Turn logging off (-log 0)
        // Copy log output to the materials.xml file

        Enable_Log(LOG_OLD, 1);

        Write_Log(LOG_OLD, LOG_INFO, "<MATERIAL>");

        string str;

        str = "<MATNAME>";
        str += lpmaterial->name;
        str += "</MATNAME>";

        Write_Log(LOG_OLD, LOG_INFO, str.c_str());

        str.clear();
        str = "<TEXNAME>";
        str += lpmaterial->texname;
        str += "</TEXNAME>";

        Write_Log(LOG_OLD, LOG_INFO, str.c_str());

        str.clear();
        str = "<ALPHANAME>";
        str += lpmaterial->alphaname;
        str += "</ALPHANAME>";

        Write_Log(LOG_OLD, LOG_INFO, str.c_str());

        FLOAT32 r = lpmaterial->material.diffuse.r;
        FLOAT32 g = lpmaterial->material.diffuse.g;
        FLOAT32 b = lpmaterial->material.diffuse.b;
        FLOAT32 a = lpmaterial->material.diffuse.a;

        Write_Log(LOG_OLD, LOG_INFO, "<DIFFUSE>%f %f %f %f</DIFFUSE>", r, g, b, a);

        r = lpmaterial->material.ambient.r;
        g = lpmaterial->material.ambient.g;
        b = lpmaterial->material.ambient.b;
        a = lpmaterial->material.ambient.a;

        Write_Log(LOG_OLD, LOG_INFO, "<AMBIENT>%f %f %f %f</AMBIENT>", r, g, b, a);

        r = lpmaterial->material.specular.r;
        g = lpmaterial->material.specular.g;
        b = lpmaterial->material.specular.b;
        a = lpmaterial->material.specular.a;

        Write_Log(LOG_OLD, LOG_INFO, "<SPECULAR>%f %f %f %f</SPECULAR>", r, g, b, a);

        r = lpmaterial->material.emissive.r;
        g = lpmaterial->material.emissive.g;
        b = lpmaterial->material.emissive.b;
        a = lpmaterial->material.emissive.a;

        Write_Log(LOG_OLD, LOG_INFO, "<EMISSIVE>%f %f %f %f</EMISSIVE>", r, g, b, a);

        Write_Log(LOG_OLD, LOG_INFO, "<SHININESS>%f</SHININESS>", lpmaterial->material.power);
        Write_Log(LOG_OLD, LOG_INFO, "</MATERIAL>");

        Enable_Log(LOG_OLD, 0);
    }
#endif

	if (!piIsMaterial(matid))
	{
        size_t code = SuperFastHash((const char*)&lpmaterial->material, sizeof(piMATERIAL));

        typedef std::map<size_t, int> Duplicate;
        static Duplicate duplicate;
        Duplicate::iterator itr = duplicate.find(code);

        if(itr != duplicate.end())
        {
            oglAddMaterialName(lpmaterial->name, itr->second);
            return TRUE;
        }

		matid = piCreateMaterial(&lpmaterial->material);
		if (!piIsMaterial(matid))
		{
			return(FALSE);
		}

        duplicate[code] = matid;

		nummats = piGetMaterials(&lpmats);

		lpmats[matid].matid = matid;
		lpmats[matid].material = lpmaterial->material;
		strcpy(lpmats[matid].name, lpmaterial->name);
		strcpy(lpmats[matid].texname, lpmaterial->texname);
		strcpy(lpmats[matid].alphaname, lpmaterial->alphaname);

		Write_Debug("Material loaded: %s", lpmats[matid].name);

		// if a filename is specified, get a handle to existing texture
		if (lpmats[matid].texname[0])
		{
			lpmats[matid].texid = ldpixLoadImageFileToSurface(lpmats[matid].texname, colorBLACK, LDPIX_NONE);
			if (!piIsTexture(lpmats[matid].texid, 0))
			{
				Write_Debug("Material texture file not found: %s", lpmats[matid].texname);
				lpmats[matid].texid = -1;
				retval = FALSE;
			}
		}
		else
		{
			lpmats[matid].texid = -1;
		}

		if (strcmp(lpmats[matid].name, "foliage") == 0)
		{
			FoliageMatId = matid;
		}

		// This is the default material
		if (strcmp(lpmats[matid].name, "default") == 0)
		{
			DefaultMaterialId = matid;
		}

        oglAddMaterialName(lpmaterial->name, matid);
	}
	else
	{
		nummats = piGetMaterials(&lpmats);
	}

	*lpmaterial = lpmats[matid];

	return(retval);
}

// callback routine that creates an object
// for each object in a model file
OBJECT* obcObject(OBJECT* attached, OBJECT* tv)
{
// Variables for execute buffer generation
int			numverts, matid, texid;
OBJECT*	lpobj;
char str[32];

	matid = tv->matid;
	if (!piIsMaterial(matid))
	{
		if (tv->matname != NULL && tv->matname[0] != 0)
		{
			matid = piFindMaterialName(tv->matname);
			if (!piIsMaterial(matid))
			{
				Write_Debug("Material not found: %s\n", tv->matname);
				return(NULL);
			}
		}
	}

	tv->matid = matid;

    // This will set the texid to -1 when running the conversion
	// program. We don't want that to happen.
	texid = tv->texid;

#ifndef CONVERSION_PROGRAM
    if(!piIsTexture(texid, 0))
	{
		texid = piGetMaterialTexId(matid);
		piGetMaterialTexName(matid, str);
		tv->texname = strpool(str);
	}

	tv->texid = texid;
#endif

	numverts = piGetNumberVertices();
	if (numverts == 0 || numverts < tv->numverts)
	{
		Write_Debug("Entity has too many vertices: %d, max: %d\n", tv->numverts, numverts);
		return(NULL);
	}

	obcTextureFixup(tv);

	lpobj = obmainGetNewObject();

    if(attached && attached->comment)
    {
		tv->comment = (SCHAR*)memAlloc(strlen(attached->comment) + 1);
		strcpy(tv->comment, attached->comment);
    }

	obmainSetObject(lpobj, tv);

    if (attached != NULL)
	{
		obmainAttachObject(attached, lpobj);
	}

	return(lpobj);
}

OBJECT* obcSphere(const char* name, const char* texname, int MeshSize, piVECTOR* Location, float Scale, UINT32 Color)
{
//LOGICAL		loadalpha=FALSE;
char        filename[256]="";
int         texid;
OBJECT*    lpobj;

	if (texname != NULL)
	{
		strcpy(filename, texname);

		// If the name is not a filename (for a textured surface),
		// then the sphere will be untextured...
		texid = ldpixLoadImageFileToSurface(filename, colorBLACK, LDPIX_MIPMAP);
	}
	else
	{
		texid = -1;
	}

	lpobj = obmainGetNewObject();
	if (lpobj == NULL)
	{
		return(NULL);
	}

	lpobj->matid = -1;
	lpobj->texname = strpool(filename);
	lpobj->texid = texid;

	if (!obsphCreate(name, MeshSize, Location, Scale, Color, lpobj))
	{
		return(NULL);
	}

	obcTextureFixup(lpobj);

	return(lpobj);
}

OBJECT* obcCylinder(const char* name, const char* texname, int MeshSize, piVECTOR* Location, float Scale, float Ellipse, UINT32 Color)
{
//LOGICAL		loadalpha=FALSE;
char        filename[256]="";
int         texid;
OBJECT*    lpobj;

  if (texname != NULL)
  {
		strcpy(filename, texname);

		// If the name is not a filename (for a textured surface),
		// then the sphere will be untextured...
		texid = ldpixLoadImageFileToSurface(filename, colorBLACK, LDPIX_MIPMAP);
		if (!piIsTexture(texid, 0))
		{
			return(NULL);
		}
	}
	else
	{
		texid = -1;
	}

	lpobj = obmainGetNewObject();
	if (lpobj == NULL)
	{
		return(NULL);
	}

	lpobj->matid = -1;
	lpobj->texname = strpool(filename);
	lpobj->texid = texid;

	if (!obsphCreateCylinder(name, MeshSize, Location, Scale, Ellipse, Color, lpobj))
	{
		return(NULL);
	}

	obcTextureFixup(lpobj);

	return(lpobj);
}

OBJECT* obcHemiSphere(const char* name, const char* texname, int MeshSize, piVECTOR* Location, float Scale, float Ellipse, UINT32 Color)
{
//LOGICAL		loadalpha=FALSE;
char        filename[256]="";
int         texid;
OBJECT*    lpobj;

	if (texname != NULL)
	{
		strcpy(filename, texname);

		// If the name is not a filename (for a textured surface),
		// then the sphere will be untextured...
		texid = ldpixLoadImageFileToSurface(filename, colorBLACK, LDPIX_MIPMAP);
		if (!piIsTexture(texid, 0))
		{
			return(NULL);
		}
	}
	else
	{
		texid = -1;
	}

	lpobj = obmainGetNewObject();
	if (lpobj == NULL)
	{
		return(NULL);
	}

	lpobj->matid = -1;
	lpobj->texname = strpool(filename);
	lpobj->texid = texid;

	if (!obsphCreateHemi(name, MeshSize, Location, Scale, Ellipse, Color, lpobj))
	{
		return(NULL);
	}

	obcTextureFixup(lpobj);

	return(lpobj);
}

OBJECT* obcSphereVoxel(const char* name, const char* texname, int MeshSize, piVECTOR* Location, float Scale, float Depth, UINT32 Color)
{
char        filename[256]="";
int         texid;
OBJECT*    lpobj;

	if (texname != NULL)
	{
		strcpy(filename, texname);
		texid = ldpixLoadImageFileToSurface(filename, colorBLACK, LDPIX_MIPMAP);
		if (!piIsTexture(texid, 0))
		{
			return(NULL);
		}
	}
	else
	{
		return(NULL);
	}

	lpobj = obmainGetNewObject();
	if (lpobj == NULL)
	{
		return(NULL);
	}

	lpobj->matid = -1;
	lpobj->texname = strpool(filename);
	lpobj->texid = texid;

	if (!obsphCreateVoxel(name, texname, MeshSize, Location, Scale, Depth, Color, lpobj))
	{
		return(NULL);
	}

	obcTextureFixup(lpobj);

	return(lpobj);
}

void obcTextureFixup(OBJECT* lpobj)
{
int   i;
float widthfactor, heightfactor;

	if (lpobj == NULL)
	{
		return;
	}

	if (piIsTexture(lpobj->texid, 0))
	{
		piGetTextureFactors(lpobj->texid, 0, &widthfactor, &heightfactor);

		piTexCoordPtr tex = lpobj->StaticTexCoords();

		for (i=0 ; i < lpobj->numverts ; i++, ++tex)
		{
			tex->u *= widthfactor;
			tex->v *= heightfactor;
		}

		lpobj->SyncMutableData();
	}
}

/* Modify materials changed during debug testing */
#ifdef BETA_RELEASE
void _modifymaterialElements(xmlNodePtr root)
{
    xmlNodePtr node = root;

    MATERIAL* materials = NULL;
    MATERIAL* mat;

    piGetMaterials(&materials);

	for(; node != NULL; node = node->next)
	{
        if(node->type != XML_ELEMENT_NODE)
        {
            continue;
        }

        if(!xmlStrcmp((xmlChar*)"MATNAME", node->name))
        {
            xmlChar* s = xmlNodeGetContent(node);

            if(s)
            {
                SINT32 id = piFindMaterialName((char*)s);

                if(!piIsMaterial(id))
                {
                    // Bad error
                    Write_Log(LOG_OLD, LOG_INFO, "[_modifymaterialElements] did not find %s", (char*)s);
                    return;
                }

                mat = &materials[id];
            }

            continue;
        }

        if(!xmlStrcmp((xmlChar*)"TEXNAME", node->name))
        {
            // No need to modify this at the moment. Not used
            continue;
        }

        if(!xmlStrcmp((xmlChar*)"ALPHANAME", node->name))
        {
            // No need to modify this at the moment. Not used
            continue;
        }

        if(!xmlStrcmp((xmlChar*)"DIFFUSE", node->name))
        {
            xmlChar* s = xmlNodeGetContent(node);

            if(s)
            {
                piCOLORVALUE* d = &mat->material.diffuse;

                sscanf((char*)s, "%f %f %f %f", &d->r, &d->g, &d->b, &d->a);
                xmlFree(s);
            }

            continue;
        }

        if(!xmlStrcmp((xmlChar*)"AMBIENT", node->name))
        {
            xmlChar* s = xmlNodeGetContent(node);

            if(s)
            {
                piCOLORVALUE* a = &mat->material.ambient;

                sscanf((char*)s, "%f %f %f %f", &a->r, &a->g, &a->b, &a->a);
                xmlFree(s);
            }

            continue;
        }

        if(!xmlStrcmp((xmlChar*)"EMISSIVE", node->name))
        {
            xmlChar* s = xmlNodeGetContent(node);

            if(s)
            {
                piCOLORVALUE* e = &mat->material.emissive;

                sscanf((char*)s, "%f %f %f %f", &e->r, &e->g, &e->b, &e->a);
                xmlFree(s);
            }

            continue;
        }

        if(!xmlStrcmp((xmlChar*)"SPECULAR", node->name))
        {
            xmlChar* str = xmlNodeGetContent(node);

            if(str)
            {
                piCOLORVALUE* s = &mat->material.specular;

                sscanf((char*)str, "%f %f %f %f", &s->r, &s->g, &s->b, &s->a);
                xmlFree(str);
            }

            continue;
        }

        if(!xmlStrcmp((xmlChar*)"SHININESS", node->name))
        {
            xmlChar* s = xmlNodeGetContent(node);

            if(s)
            {
                mat->material.power = (float)atof((char*)s);
                xmlFree(s);
            }

            continue;
        }

        if(glIsList(mat->displayList))
        {
            glDeleteLists(mat->displayList, 1);
            mat->displayList = 0;
        }
    }
}

void _modifymaterialTree(xmlNodePtr root)
{
    xmlNodePtr node = root;

	for(; node != NULL; node = node->next)
	{
        if(node->type != XML_ELEMENT_NODE)
        {
            continue;
        }

        if(!xmlStrcmp((xmlChar*)"MATERIAL", node->name))
        {
            _modifymaterialElements(node->children);
        }
	}
}

void _modifyMaterials()
{
    // Load materials from materials.xml
    xmlRegisterInputCallbacks(xmlinputmatch,
                             (xmlInputOpenCallback)fioOpen,
                             (xmlInputReadCallback)fioRead,
                             (xmlInputCloseCallback)fioClose);

    SCHAR* name = "materials.xml";

    xmlDocPtr doc = xmlReadFile(name, NULL, XML_PARSE_DTDLOAD | XML_PARSE_RECOVER);

    if(doc)
    {
	    xmlNodePtr root = xmlDocGetRootElement(doc);

        if(!root)
        {
            return;
        }

        if(!xmlStrcmp((xmlChar*)"MATERIALS", root->name))
        {
            _modifymaterialTree(root->children);
        }

	    xmlFreeDoc(doc);
    }

    xmlCleanupInputCallbacks();
}

void obcUpdate()
{
    static LOGICAL firsttime = TRUE;
    static SINT32 mod1 = 0;
    static SINT32 mod2 = 0;
    static SCHAR* filename = "materials.xml";

    if(firsttime)
    {
        firsttime = FALSE;
        fioModified(&mod1, filename);
        mod2 = mod1;
        return;
    }

    fioModified(&mod2, filename);

    if(mod1 < mod2)
    {
        mod1 = mod2;
        _modifyMaterials();
    }
}
#endif // BETA_RELEASE
