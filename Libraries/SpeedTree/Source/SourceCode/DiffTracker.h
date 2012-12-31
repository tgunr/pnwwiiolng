///////////////////////////////////////////////////////////////////////  
//  DiffTracker.h
//
//  *** INTERACTIVE DATA VISUALIZATION (IDV) CONFIDENTIAL AND PROPRIETARY INFORMATION ***
//
//  This software is supplied under the terms of a license agreement or
//  nondisclosure agreement with Interactive Data Visualization, Inc. and
//  may not be copied, disclosed, or exploited except in accordance with 
//  the terms of that agreement.
//
//      Copyright (c) 2003-2008 IDV, Inc.
//      All rights reserved in all media.
//
//      IDV, Inc.
//      http://www.idvinc.com


///////////////////////////////////////////////////////////////////////  
//  Diagnostic utility used by IDV during development; not intended or 
//  useful for end users

#ifdef DIFF_TRACKING

#pragma once
#include "SpeedTreeMemory.h"


///////////////////////////////////////////////////////////////////////  
//  class CDiffTracker declaration

class CDiffTracker
{
public:
static  void                        BeginTree(const char* pTreeName);
static  void                        EndTree(void);

static  void                        AddPOI(const st_string& strPOI); // 'POI' stands for Point Of Interest
static  int                         CurrentCount(void)  { return int(m_vPOIs.size( )); }

private:
static  std::string                 m_strTreeName;
static  std::vector<std::string>    m_vPOIs;
};

#endif
