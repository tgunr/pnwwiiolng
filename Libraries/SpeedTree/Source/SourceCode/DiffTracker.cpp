///////////////////////////////////////////////////////////////////////  
//  DiffTracker.cpp
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

#ifdef WIN32
#pragma warning (disable : 4996)
#endif

#include "LibGlobals_Source/IdvGlobals.h"
#include "DiffTracker.h"
using namespace std;


///////////////////////////////////////////////////////////////////////  
//  Static variables

string CDiffTracker::m_strTreeName;
std::vector<std::string> CDiffTracker::m_vPOIs;


///////////////////////////////////////////////////////////////////////  
//  CDiffTracker::BeginTree

void CDiffTracker::BeginTree(const char* pTreeName)
{
    m_strTreeName = pTreeName;
}


///////////////////////////////////////////////////////////////////////  
//  CDiffTracker::EndTree
//
//  Writes a report to the end of a given file

void CDiffTracker::EndTree(void)
{
#ifdef _XBOX
    const char* pFilename = "game:\\DiffTracker_360.txt";
#elif PS3
    const char* pFilename = "DiffTracker_PS3.txt";
#else
    #ifdef IDV_VISUALC_6
    const char* pFilename = "DiffTracker_Windows_VC6.txt";
    #else
    const char* pFilename = "DiffTracker_Windows_VC8.txt";
    #endif
#endif

    FILE* pFile = fopen(pFilename, "a");
    if (pFile)
    {
        fprintf(pFile, "Tree Model: [%s]\n\n", m_strTreeName.c_str( ));

        int nNumEntries = int(m_vPOIs.size( ));
        fprintf(pFile, "\t%d POI Entries\n", nNumEntries);
        for (int i = 0; i < nNumEntries; ++i)
        {
            fprintf(pFile, "\t%d) %s\n", i + 1, m_vPOIs[i].c_str( ));
        }

        fclose(pFile);
    }
    else
        fprintf(stderr, "fopen [%s] failed\n", pFilename);
}


///////////////////////////////////////////////////////////////////////  
//  CDiffTracker::AddPOI

void CDiffTracker::AddPOI(const st_string& strPOI)
{
    m_vPOIs.push_back(string(strPOI.c_str( )));
}

#endif
