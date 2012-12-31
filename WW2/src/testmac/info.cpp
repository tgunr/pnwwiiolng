#include <syscfg.h> // Path is project specific.
/*
 *  info.cpp
 *  WW2OL
 *
 *  Created by Andrew Wulf on 11/13/10.
 *  Copyright 2010 Idle Diversions. All rights reserved.
 *
 */

#include "info.h"
#include <string>
#include <iostream>
#include <set>
#include <list>

using namespace std;

static set<string> PathSet;
static list<string> PathList;

void addTexturePath( const char* path )
{
  pair<set<string>::iterator,bool> ret;
  string s(path);
  ret = PathSet.insert(s);
//  if (ret.second==true)
  {
    PathList.push_back (s);
  }

}

extern "C" void dumpTexturePaths();

void dumpTexturePaths()
{
  for ( list<string>::iterator it=PathList.begin() ; it != PathList.end(); it++ )
    cout << *it << '\n';

}

