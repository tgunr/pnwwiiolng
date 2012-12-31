// ========================================================================
// $File: //jeffr/granny/rt/granny_bspline_solver_debugging.cpp $
// $DateTime: 2009/08/24 10:27:03 $
// $Change: 22865 $
// $Revision: #6 $
//
// (C) Copyright 1999-2008 by RAD Game Tools, All Rights Reserved.
// ========================================================================
#include "granny_bspline_solver_debugging.h"

#if DEBUG_BSPLINE_SOLVER

#include <stdio.h>
#include "granny_assert.h"

// This should always be the last header included
#include "granny_cpp_settings.h"

static FILE *GlobalDump = 0;

void GRANNY
DebugBeginSolve(void)
{
    GRANNY_ENTER();

#if DEBUG_BSPLINE_SOLVER
    GlobalDump = fopen("g:/debug/curve_fit.txt", "w");
    Assert(GlobalDump);
#endif
}

void GRANNY
DebugEndSolve(void)
{
    GRANNY_ENTER();

#if DEBUG_BSPLINE_SOLVER
    fclose(GlobalDump);
#endif
}

void GRANNY
DebugDumpATb(int32x KnotCount, int32x bWidth, real32 *ATb)
{
    GRANNY_ENTER();

    {for(int32x i = 0;
         i < KnotCount;
         ++i)
    {
        {for(int32x e = 0;
             e < bWidth;
             ++e)
        {
            fprintf(GlobalDump, "%+.3f ", ATb[i*bWidth + e]);
        }}
        fprintf(GlobalDump, "\n");
    }}

    fprintf(GlobalDump, "\n\n");
}

void GRANNY
DebugDumpATA(int32x ATn, int32x BandWidth,
        int32x *KnotStart, real32 *Knots,
        real32 *ATA)
{
    GRANNY_ENTER();

    {for(int32x i = 0;
         i < ATn;
         ++i)
    {
        if(KnotStart && Knots)
        {
            fprintf(GlobalDump, "%3d (%4d) %8f : ", i, KnotStart[i], Knots[i]);
        }

#if DUMP_IN_MATRIX_FORM
        {for(int32x j = 0;
             j < (i + BandWidth - 1);
             ++j)
        {
            fprintf(GlobalDump, "------ ", 0.0f);
        }}
#endif
        {for(int32x j = 0;
             j < BandWidth;
             ++j)
        {
            fprintf(GlobalDump, "%+.3f ", ATA[i*BandWidth + j]);
        }}

#if DUMP_IN_MATRIX_FORM
        {for(int32x j = i + BandWidth;
             j < ATn;
             ++j)
        {
            fprintf(GlobalDump, "------ ", 0.0f);
        }}
#endif
        fprintf(GlobalDump, "\n");
    }}

    fprintf(GlobalDump, "\n\n");
}

void GRANNY
DebugDumpb(int32x bCount, int32x bWidth, real32 *b)
{
    GRANNY_ENTER();

    {for(int32x i = 0;
         i < bCount;
         ++i)
    {
        {for(int32x e = 0;
             e < bWidth;
             ++e)
        {
            fprintf(GlobalDump, "%+.3f ", b[i*bWidth + e]);
        }}
        fprintf(GlobalDump, "\n");
    }}

    fprintf(GlobalDump, "\n\n");
}

void GRANNY
DebugDumpA(int32x KnotCount, int32x *KnotStart, int32x BandWidth, real32 *A)
{
    GRANNY_ENTER();

    {for(int32x i = 0;
         i < (KnotCount + BandWidth);
         ++i)
    {
        if(i == KnotCount)
        {
            fprintf(GlobalDump, " - (overhang) - \n");
        }

        {for(int32x j = KnotStart[i];
             j < KnotStart[i + 1];
             ++j)
        {
#if DUMP_IN_MATRIX_FORM
            {for(int32x k = 0;
                 k < i;
                 ++k)
            {
                fprintf(GlobalDump, "------ ", 0.0f);
            }}
#endif

            {for(int32x k = 0;
                 k < BandWidth;
                 ++k)
            {
                fprintf(GlobalDump, "%+.3f ", A[BandWidth*j + k]);
            }}

#if DUMP_IN_MATRIX_FORM
            {for(int32x k = (i + 1);
                 k < KnotCount;
                 ++k)
            {
                fprintf(GlobalDump, "------ ", 0.0f);
            }}
#endif

            fprintf(GlobalDump, "\n");
        }}
    }}

    fprintf(GlobalDump, "\n\n");
}

void GRANNY
DebugDumpC(int32x ATn, int32x BandWidth, real32 *ATA)
{
    GRANNY_ENTER();

    DebugDumpATA(ATn, BandWidth, 0, 0, ATA);
}

void GRANNY
DebugDumpFSATb(int32x ATn, int32x ATbStride, real32 *ATb)
{
    GRANNY_ENTER();

    DebugDumpATb(ATn, ATbStride, ATb);
}

void GRANNY
DebugDumpBSATb(int32x ATn, int32x ATbStride, real32 *ATb)
{
    GRANNY_ENTER();

    DebugDumpATb(ATn, ATbStride, ATb);
}

#endif
