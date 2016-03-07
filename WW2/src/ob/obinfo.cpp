/*
 *  obinfo.cpp
 *  WW2OL
 *
 *  Created by Andrew Wulf on 7/6/11.
 *  Copyright 2011 Playnet Inc. All rights reserved.
 *
 */
#include <syscfg.h> // Path is project specific.

#ifdef MODEL_INFO

#include <fstream>
#include <map>
#include "ogl/oglMaterial.h"
#include "obinfo.h"
#include "obmain.h"
#include "obMgr.h"
#include "obpool.h"
using namespace std;

extern std::map<GLuint, std::string> reverseTextureIndex;
extern std::map<int,std::string> reverse_materialnames;

#define INFO_OUTFILE "models.txt"

enum is_type
{
    ISNONE,ISROOT,ISSIBLING,ISATTACHED,ISCHILD
};

static void objectInfo( iostream& ss, PoolNode* obj, is_type type, is_type prevType, int indent=0 );
static void modelInfo( iostream& ss, PoolModel* obj );

static bool first = true;
static bool nooutput = true;

std::string getModelInfo( PoolModel* obj )
{
    if ( nooutput ) return "";

    fstream outfile(INFO_OUTFILE, first?fstream::out|fstream::trunc : fstream::out|fstream::app);
    first = false;

    cout << "info for " << obj->modelname << endl;

    modelInfo(outfile,obj);
    objectInfo(outfile,obj->node,ISROOT,ISNONE,1);

    outfile.flush();
    outfile.close();

    return "";
}

static string removecrlf( const char * s )
{
    string t;

    while ( *s != 0 )
    {
        if ( *s == '\n' )
            t += '|';
        else if ( *s != '\r' )
            t += *s;
        s++;
    }

    return t;

}

static void modelInfo( iostream& ss, PoolModel* obj )
{
    string indent(2,'\t');
    string indent3(3,'\t');

    ss << endl << "--- MODEL: " << obj->directory << obj->modelname << endl << "[" << endl;

    ss << indent << "modelType:" << obj->modelType << endl;
    ss << indent << "placeMode:" << obj->placeMode << endl;
    ss << indent << "propertyFlags:" << hex << obj->propertyFlags << endl;
    ss << indent << "loadFlags:" << hex << obj->loadFlags << dec << endl;
    if ( obj->hastrees )
    {
        ss << indent << "has trees" << endl;
    }
    if ( obj->hasRoads )
    {
        ss << indent << "has roads" << endl;
    }
    if ( obj->subModelReferences.size() > 0 )
    {
        ss << indent << "submodel count:" << obj->subModelReferences.size() << endl;
        for (std::vector<SubModelReference>::iterator it=obj->subModelReferences.begin(); it!= obj->subModelReferences.end(); it++ )
        {
            ss << indent3 << it->poolModel->directory << it->poolModel->modelname << endl;
        }
    }

    if ( obj->staticBuffers.size() || obj->collideBranches.size() || obj->vbo )
    {
        ss << indent;
        if ( obj->staticBuffers.size() )
        {
            ss << "+staticBuffers ";
        }
        if ( obj->collideBranches.size() )
        {
            ss << "+collideBranches ";
        }
        if ( obj->vbo )
        {
            ss << "+vbo ";
        }
        ss << endl;
    }

    ss << indent << "Bounding Box: {"
        << obj->bounds.xMin() << ',' << obj->bounds.yMin() << ',' << obj->bounds.zMin() << "}-{"
        << obj->bounds.xMax() << ',' << obj->bounds.yMax() << ',' << obj->bounds.zMax() << '}' << endl;


    ss << "]" << endl;
}

static void objectInfo( iostream& ss, PoolNode* obj, is_type type, is_type prevType, int indentvalue )
{
    if ( prevType == ISROOT || type == ISCHILD || (type == ISATTACHED && prevType != ISATTACHED ) )
    {
        indentvalue++;
    }

    string indent(indentvalue*2,'\t');
    string indent1((indentvalue+1)*2,'\t');
    string nm = type == ISROOT ? "[ROOT] " : ( type == ISSIBLING ? "[SIBLING] " : ( type == ISATTACHED ? "[ATTACHED] " : ( type == ISCHILD ? "[CHILD] " : "")));

    ss << indent << nm << obj->name;
    if ( obj->comment != 0 )
    {
        ss << "(" << removecrlf(obj->comment) << ")";
    }

    ss << " {" << endl;

    ss << indent1 << "flags:" << hex << obj->flags << ","<< obj->flag2 << ","<< obj->flag3 << dec << endl;

    if ( obj->matid >= 0 )
    {
        ss << indent1 << "matid:" << obj->matid << "(" << reverse_materialnames[obj->matid] << ")" << endl;
    }

    if ( obj->material != 0 )
    {
        int texid = obj->material->GetTexture(0);
        if ( texid > 0 )
        {
            ss << indent1 << "texid:" << texid << "(" << reverseTextureIndex[texid] << ")";
            if ( obj->flag2 & OBJECT_NORMALMAP ) ss << "+normal ";
            if ( obj->flag3 & OBJECT_SPECULARMAP ) ss << "+spec ";
            ss << endl;
        }
    }

    if ( obj->objdata && obj->objdata->numverts )
    {
        ss << indent1 << "numverts:" << obj->objdata->numverts << endl;
    }

    if ( obj->dof )
    {
        ss << indent1 << "DOFs" << endl;
    }

    if ( obj->collider || obj->objdata || obj->vertices || obj->indices || obj->faceNormals || obj->collisionNormals || obj->meshContext )
    {
        ss << indent1;

        if ( obj->collider )
        {
            ss << "+collider ";
        }

        if ( obj->objdata )
        {
            ss << "+objdata ";
        }

        if ( obj->vertices )
        {
            ss << "+vertices ";
        }

        if ( obj->indices )
        {
            ss << "+indices ";
        }

        if ( obj->faceNormals )
        {
            ss << "+faceNormals ";
        }

        if ( obj->collisionNormals )
        {
            ss << "+collisionNormals ";
        }

        if ( obj->meshContext )
        {
            ss << "+meshContext ";
        }
        ss << endl;

    }

    ss << indent << "}" << endl;

    if ( obj->child )
    {
        objectInfo(ss,obj->child,ISCHILD,type, indentvalue);
    }
    if ( obj->attached )
    {
        objectInfo(ss,obj->attached,ISATTACHED,type, indentvalue);
    }
    if ( obj->sibling )
    {
        objectInfo(ss,obj->sibling,ISSIBLING,type, indentvalue);
    }
}
#endif

