///////////////////////////////////////////////////////////////////////  
//  Allocator.cpp
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

#ifdef SPEEDTREE_MEMORY_STATS

#ifdef WIN32
#pragma warning (disable : 4702 4221 4996) // disable warning about unreachable code in xtree
#endif
#define ALLOCATOR_SOURCE_FILE
#include "SpeedTreeAllocator.h"
#ifdef PS3
#include <stdio.h>
#endif

CSpeedTreeAllocator::stats_map* m_pStatsMap = NULL;
CSpeedTreeAllocator::SAllocStats m_sGlobalStats;
CSpeedTreeAllocator::leak_map m_mLeakMap;


///////////////////////////////////////////////////////////////////////  
//  CSpeedTreeAllocator::TrackAlloc

void CSpeedTreeAllocator::TrackAlloc(const char* pDescription, void* pBlock, size_t sAmount)
{
    if (!m_pStatsMap)
        m_pStatsMap = new stats_map;

    // local statistics
    (*m_pStatsMap)[pDescription].m_siNumAllocates++;
    (*m_pStatsMap)[pDescription].m_siAmountAllocated += sAmount;

    // global statistics
    m_sGlobalStats.m_siNumAllocates++;
    m_sGlobalStats.m_siAmountAllocated += sAmount;

    // leak statistics
    leak_map::iterator iFind = m_mLeakMap.find(pBlock);
    m_mLeakMap[pBlock].m_nCount++;
    m_mLeakMap[pBlock].m_nAmount += int(sAmount);
    m_mLeakMap[pBlock].m_strDesc = std::string(pDescription);
}


///////////////////////////////////////////////////////////////////////  
//  CSpeedTreeAllocator::TrackFree

void CSpeedTreeAllocator::TrackFree(const char* pDescription, void* pBlock, size_t sAmount)
{
    if (pBlock)
    {
        if (!m_pStatsMap)
            m_pStatsMap = new stats_map;

        // local statistics
        (*m_pStatsMap)[pDescription].m_siNumFrees++;
        (*m_pStatsMap)[pDescription].m_siAmountFreed += sAmount;

        // global statistics
        m_sGlobalStats.m_siNumFrees++;
        m_sGlobalStats.m_siAmountFreed += sAmount;

        // leak statistics
        leak_map::iterator iFind = m_mLeakMap.find(pBlock);
        if (iFind != m_mLeakMap.end( ))
        {
            iFind->second.m_nCount--;
            iFind->second.m_nAmount -= int(sAmount);
            if (iFind->second.m_nCount == 0)
                (void) m_mLeakMap.erase(iFind);
        }
        else
            fprintf(stderr, "CSpeedTreeAllocator::TrackFree cannot find [%s]\n", pDescription);
    }
}


///////////////////////////////////////////////////////////////////////  
//  CSpeedTreeAllocator::Report

bool CSpeedTreeAllocator::Report(const char* pFilename, bool bFreeTrackingData)
{
    bool bSuccess = false;
    std::vector<SAllocStats> vOrdered;
    if (m_pStatsMap)
    {
        for (stats_map::const_iterator i = m_pStatsMap->begin( ); i != m_pStatsMap->end( ); ++i)
        {
            vOrdered.push_back(i->second);
            vOrdered.back( ).m_strDesc = i->first;
        }
        std::sort(vOrdered.begin( ), vOrdered.end( ));
    }

    FILE* pFile = stdout;
    if (pFilename)
    {
        pFile = fopen(pFilename, "w");
        if (!pFile)
            pFile = stdout;
    }
    if (pFile)
    {
        fprintf(pFile, "\nallocator,alloc/free delta (#),alloc/free delta (KB),# allocs,# frees,alloced (KB),freed (KB),%% of all allocs,%% of all frees\n");

        for (int i = 0; i < int(vOrdered.size( )); ++i)
        {
            fprintf(pFile, "%s,%d,%0.2f,%d,%d,%0.2f,%0.2f,%0.1f,%0.1f\n",
                vOrdered[i].m_strDesc.c_str( ),                                                 // allocator
                vOrdered[i].m_siNumAllocates - vOrdered[i].m_siNumFrees,                        // alloc/free delta (#)
                (vOrdered[i].m_siAmountAllocated - vOrdered[i].m_siAmountFreed) / 1024.0f,      // alloc/free delta (KB)
                vOrdered[i].m_siNumAllocates,                                                   // # allocs
                vOrdered[i].m_siNumFrees,                                                       // # frees
                vOrdered[i].m_siAmountAllocated / 1024.0f,                                      // alloced (KB)
                vOrdered[i].m_siAmountFreed / 1024.0f,                                          // freed (KB)
                100.0f * vOrdered[i].m_siNumAllocates / float(m_sGlobalStats.m_siNumAllocates), // % of all allocs
                100.0f * vOrdered[i].m_siNumFrees / float(m_sGlobalStats.m_siNumFrees));        // % of all frees
        }

        fprintf(pFile, "\nGlobal statistics:\n");
        fprintf(pFile, "\t[%d] total allocations,[%1.f KB] allocated\n", m_sGlobalStats.m_siNumAllocates, m_sGlobalStats.m_siAmountAllocated / 1024.0f);
        fprintf(pFile, "\t[%d] total free calls,[%1.f KB] freed\n", m_sGlobalStats.m_siNumFrees, m_sGlobalStats.m_siAmountFreed / 1024.0f);
        fprintf(pFile, "\n");

        // fill vector with values and sort
        int nIndex = 0;
        int nTotalLeaked = 0;
        std::vector<SLeakStats> vLeaks(m_mLeakMap.size( ));
        for (leak_map::const_iterator j = m_mLeakMap.begin( ); j != m_mLeakMap.end( ); ++j)
        {
            vLeaks[nIndex++] = j->second;
            nTotalLeaked += j->second.m_nAmount;
        }
        sort(vLeaks.begin( ), vLeaks.end( ));

        if (nTotalLeaked != 0)
        {
            fprintf(pFile, "\tTotal leaked using SpeedTree allocator: %.2fKB\n", nTotalLeaked / 1024.0f);
            for (nIndex = 0; nIndex < int(vLeaks.size( )); ++nIndex)
                fprintf(pFile, "\t\t%d. [%s],total amount: %.2fKB\n", 
                    nIndex + 1, vLeaks[nIndex].m_strDesc.c_str( ), vLeaks[nIndex].m_nAmount / 1024.0f);
        }
        else 
            fprintf(pFile, "\t[No leaks detected]\n");

        fclose(pFile);
        bSuccess = true;
    }

    if (m_pStatsMap && bFreeTrackingData)
    {
        delete m_pStatsMap;
        m_pStatsMap = NULL;
    }

    return bSuccess;
}

#endif // SPEEDTREE_MEMORY_STATS

