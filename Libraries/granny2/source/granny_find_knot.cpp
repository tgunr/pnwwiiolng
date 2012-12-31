// ========================================================================
// $File: //jeffr/granny/rt/granny_find_knot.cpp $
// $DateTime: 2009/10/30 15:03:19 $
// $Change: 23817 $
// $Revision: #5 $
//
// (C) Copyright 1999-2008 by RAD Game Tools, All Rights Reserved.
// ========================================================================
#include "granny_find_knot.h"

#include "granny_assert.h"
#include "granny_floats.h"
#include <algorithm>

// This should always be the last header included
#include "granny_cpp_settings.h"

USING_GRANNY_NAMESPACE;

#define CHECK_FIND_KNOT 0
#if CHECK_FIND_KNOT
static int32x OriginalFindKnot(int32x KnotCount, real32 const *Knots, real32 t);
#endif

/*
  Note: the FindKnot routines return the knot that's _ahead of_ where you
  are at time t. However, it clamps to KnotCount-1, so it will never reference
  off the end of the array.
 */


template <class int_type>
static int32x
FindKnotInteger(int32x KnotCount, int_type const *Knots, int_type IntT)
{
    GRANNY_ENTER();
    Assert(KnotCount != 0);

    intaddrx KnotIndex =
        std::upper_bound(Knots, Knots + KnotCount, IntT) - Knots;

#if DEBUG
    Assert((KnotIndex >= 0) && (KnotIndex <= KnotCount));
    if ( KnotIndex < KnotCount )
    {
        Assert ( Knots[KnotIndex] > IntT );
    }
    if ( KnotIndex > 0 )
    {
        Assert ( Knots[KnotIndex-1] <= IntT );
    }
#endif

    if (KnotIndex == KnotCount)
    {
        // Stop it falling off the end of the knot array.
        // This is slightly artificial, but calling code gets confused otherwise.
        --KnotIndex;
    }

    return int32x(KnotIndex);
}


int32x GRANNY
FindKnot(int32x KnotCount, real32 const *Knots, real32 t)
{
    GRANNY_ENTER();
    Assert(KnotCount != 0);

    uint32 const* IntKnots = (uint32 const*)Knots;

    uint32 IntT = GetInt32FromReal32(t);
    Assert((IntT & 0x80000000) == 0);

    int32x const KnotIndex     = FindKnotInteger(KnotCount, IntKnots, IntT);

#if CHECK_FIND_KNOT
    int32x const OrigKnotIndex = OriginalFindKnot(KnotCount, Knots, t);
    Assert(OrigKnotIndex == KnotIndex);
#endif

    return KnotIndex;
}

int32x GRANNY
FindKnotUint16(int32x KnotCount, uint16 const *Knots, uint16 t)
{
    GRANNY_ENTER();
    Assert(KnotCount != 0);

    return FindKnotInteger(KnotCount, Knots, t);
}

int32x GRANNY
FindKnotUint8(int32x KnotCount, uint8 const *Knots, uint8 t)
{
    GRANNY_ENTER();
    Assert(KnotCount != 0);

    return FindKnotInteger(KnotCount, Knots, t);
}


// Note: Need to make these linear again...

int32x GRANNY
FindCloseKnot(int32x KnotCount, real32 const *Knots,
              real32 t, int32x StartingIndex)
{
    GRANNY_ENTER();

    return FindKnot ( KnotCount, Knots, t );
}

int32x GRANNY
FindCloseKnotUint16(int32x KnotCount, uint16 const *Knots,
                    uint16 t, int32x StartingIndex)
{
    GRANNY_ENTER();

    return FindKnotUint16 ( KnotCount, Knots, t );
}

int32x GRANNY
FindCloseKnotUint8(int32x KnotCount, uint8 const *Knots,
                    uint8 t, int32x StartingIndex)
{
    GRANNY_ENTER();

    return FindKnotUint8 ( KnotCount, Knots, t );
}



#if CHECK_FIND_KNOT
static int32x OriginalFindKnot(int32x KnotCount, real32 const *Knots, real32 t)
{
    GRANNY_ENTER();

#if DEBUG
    real32 const* OriginalKnots = Knots;
#endif
    int32x KnotWindow = KnotCount;
    int32x KnotIndex = 0;

    while(KnotWindow > 1)
    {
        if(KnotWindow & 1)
        {
            // It's odd
            KnotWindow = (KnotWindow / 2);
            Assert ( KnotIndex+KnotWindow < KnotCount );
            if(t >= Knots[KnotWindow])
            {
                KnotIndex += KnotWindow;
                Knots += KnotWindow;
            }

            ++KnotWindow;
        }
        else
        {
            // It's even
            KnotWindow = (KnotWindow / 2);
            Assert ( KnotIndex+KnotWindow < KnotCount );
            if(t >= Knots[KnotWindow])
            {
                if((KnotIndex+KnotWindow+1 >= KnotCount) || (t < Knots[KnotWindow + 1]))
                {
                    KnotIndex += KnotWindow;
                    Knots += KnotWindow;
                    break;
                }
                else
                {
                    KnotIndex += KnotWindow + 1;
                    Knots += KnotWindow + 1;
                    --KnotWindow;
                }
            }
        }
    }

    // Slight adjustment for duplicate knots.
    while ( ( KnotIndex < KnotCount ) && ( Knots[0] <= t ) )
    {
        ++KnotIndex;
        ++Knots;
    }

#if DEBUG
    Assert((KnotIndex >= 0) && (KnotIndex <= KnotCount));
    if ( KnotIndex < KnotCount )
    {
        Assert ( OriginalKnots[KnotIndex] > t );
    }
    if ( KnotIndex > 0 )
    {
        Assert ( OriginalKnots[KnotIndex-1] <= t );
    }
#endif

    if ( ( KnotIndex == KnotCount ) && ( KnotIndex > 0 ) )
    {
        // Stop it falling off the end of the knot array.
        // This is slightly artificial, but calling code gets confused otherwise.
        KnotIndex--;
    }

    return(KnotIndex);
}
#endif
