#if !defined(GRANNY_PARAMETER_CHECKING_H)
#include "header_preamble.h"
// ========================================================================
// $File: //jeffr/granny/rt/granny_parameter_checking.h $
// $DateTime: 2009/09/10 16:15:41 $
// $Change: 23186 $
// $Revision: #20 $
//
// (C) Copyright 1999-2008 by RAD Game Tools, All Rights Reserved.
// ========================================================================

#if !defined(GRANNY_NAMESPACE_H)
#include "granny_namespace.h"
#endif

#if !defined(GRANNY_LOG_H)
#include "granny_log.h"
#endif

#if NO_VERBOSE_CHECK_LOGS

#define CheckLog0(Type, Code, Msg)                                                              \
    Log0(ErrorLogMessage, SubsystemCode, "Parameter check failed.  (Verbose logging disabled)")
#define CheckLog1(Type, Code, Msg, arg0)                                                        \
    Log0(ErrorLogMessage, SubsystemCode, "Parameter check failed.  (Verbose logging disabled)")
#define CheckLog2(Type, Code, Msg, arg0, arg1)                                                  \
    Log0(ErrorLogMessage, SubsystemCode, "Parameter check failed.  (Verbose logging disabled)")
#define CheckLog3(Type, Code, Msg, arg0, arg1, arg2)                                            \
    Log0(ErrorLogMessage, SubsystemCode, "Parameter check failed.  (Verbose logging disabled)")
#define CheckLog4(Type, Code, Msg, arg0, arg1, arg2, arg3)                                      \
    Log0(ErrorLogMessage, SubsystemCode, "Parameter check failed.  (Verbose logging disabled)")

#else

#define CheckLog0(Type, Code, Msg)              \
    Log0(ErrorLogMessage, SubsystemCode, Msg)
#define CheckLog1(Type, Code, Msg, arg0)            \
    Log1(ErrorLogMessage, SubsystemCode, Msg, arg0)
#define CheckLog2(Type, Code, Msg, arg0, arg1)              \
    Log2(ErrorLogMessage, SubsystemCode, Msg, arg0, arg1)
#define CheckLog3(Type, Code, Msg, arg0, arg1, arg2)            \
    Log3(ErrorLogMessage, SubsystemCode, Msg, arg0, arg1, arg2)
#define CheckLog4(Type, Code, Msg, arg0, arg1, arg2, arg3)              \
    Log4(ErrorLogMessage, SubsystemCode, Msg, arg0, arg1, arg2, arg3)

#endif

BEGIN_GRANNY_NAMESPACE;

#define CheckBoundedInt32(Min, Value, Max, Return)                  \
    do {                                                            \
        int32x StorMin__   = (Min);                                 \
        int32x StorValue__ = (Value);                               \
        int32x StorMax__   = (Max);                                 \
        if (StorValue__ < StorMin__ || StorValue__ > StorMax__)     \
        {                                                           \
            CheckLog4(ErrorLogMessage, SubsystemCode,               \
                      "%s %d is out of range [%d, %d]",             \
                      #Value, StorValue__, StorMin__, StorMax__);   \
            Return;                                                 \
        }                                                           \
    } while (false)

#define CheckBoundedReal32(Min, Value, Max, Return)                 \
    do {                                                            \
        real32 StorMin__   = Min;                                   \
        real32 StorValue__ = Value;                                 \
        real32 StorMax__   = Max;                                   \
        if (StorValue__ < StorMin__ || StorValue__ > StorMax__)     \
        {                                                           \
            CheckLog4(ErrorLogMessage, SubsystemCode,               \
                      "%s %f is out of range [%f, %f]",             \
                      #Value, StorValue__, StorMin__, StorMax__);   \
            Return;                                                 \
        }                                                           \
    } while (false)

#define CheckInt32Index(Index, Count, Return)                       \
    do {                                                            \
        int32x StoreIndex__ = (Index);                              \
        int32x StoreCount__ = (Count);                              \
        if ((StoreIndex__ < 0) || (StoreIndex__ >= StoreCount__))   \
        {                                                           \
            CheckLog3(ErrorLogMessage, SubsystemCode,               \
                      "%s %d is out of range [0, %d)", #Index,      \
                      StoreIndex__, StoreCount__);                  \
            Return;                                                 \
        }                                                           \
    } while (false)

#define CheckIntAddrIndex(Index, Count, Return)                     \
    do {                                                            \
        intaddrx StoreIndex__ = (Index);                            \
        intaddrx StoreCount__ = (Count);                            \
        if ((StoreIndex__ < 0) || (StoreIndex__ >= StoreCount__))   \
        {                                                           \
            CheckLog3(ErrorLogMessage, SubsystemCode,               \
                      "%s %d is out of range [0, %d)", #Index,      \
                      StoreIndex__, StoreCount__);                  \
            Return;                                                 \
        }                                                           \
    } while (false)

#define CheckUInt64Index(Index, Count, Return)                                              \
    do {                                                                                    \
        uint64x StoreIndex__ = (Index);                                                     \
        uint64x StoreCount__ = (Count);                                                     \
        if (StoreIndex__ >= StoreCount__)                                                   \
        {                                                                                   \
            CheckLog3(ErrorLogMessage, SubsystemCode,                                       \
                      "%s %d is out of range [0, %d)", #Index, StoreIndex__, StoreCount__); \
            Return;                                                                         \
        }                                                                                   \
    } while (false)

#define CheckUInt32Index(Index, Count, Return)                                              \
    do {                                                                                    \
        uint32x StoreIndex__ = (Index);                                                     \
        uint32x StoreCount__ = (Count);                                                     \
        if (StoreIndex__ >= StoreCount__)                                                   \
        {                                                                                   \
            CheckLog3(ErrorLogMessage, SubsystemCode,                                       \
                      "%s %d is out of range [0, %d)", #Index, StoreIndex__, StoreCount__); \
            Return;                                                                         \
        }                                                                                   \
    } while (false)

#define CheckPointerNotNull(Pointer, Return)                        \
    do {                                                            \
        if (!(Pointer))                                             \
        {                                                           \
            CheckLog1(ErrorLogMessage, SubsystemCode,               \
                      "%s is not allowed to be NULL", #Pointer);    \
            Return;                                                 \
        }                                                           \
    } while (false)

#define CheckCondition(Condition, Return)               \
    do {                                                \
        if (!(Condition))                               \
        {                                               \
            CheckLog1(ErrorLogMessage, SubsystemCode,   \
                      "%s was not true", #Condition);   \
            Return;                                     \
        }                                               \
    } while (false)

#define CheckGreater0(Value, Return)                        \
    do {                                                    \
        if ((Value) <= 0)                                   \
        {                                                   \
            CheckLog1(ErrorLogMessage, SubsystemCode,       \
                      "%s is not allowed be <= 0", #Value); \
            Return;                                         \
        }                                                   \
    } while (false)

#define CheckNotSignCoerced(Value, Return)                          \
    do {                                                            \
        if ((Value) & ((uint64x)1 << ((SizeOf(Value) * 8) - 1)))    \
        {                                                           \
            CheckLog1(ErrorLogMessage, SubsystemCode,               \
                      "%s is likely badly sign coerced", #Value);   \
            Return;                                                 \
        }                                                           \
    } while (false)

#define CheckGreaterEqual0(Value, Return)                   \
    do {                                                    \
        if (Value < 0)                                     \
        {                                                   \
            CheckLog1(ErrorLogMessage, SubsystemCode,       \
                      "%s is not allowed be < 0", #Value); \
            Return;                                         \
        }                                                   \
    } while (false)

#define CheckAlignment(Pointer, Alignment, Return)                                  \
    do {                                                                            \
        if ((intaddrx(Pointer) % (Alignment)) != 0)                                 \
        {                                                                           \
            CheckLog2(ErrorLogMessage, SubsystemCode,                               \
                      "Pointer alignment not correct: should be: %s, value: 0x%x",  \
                      #Alignment, Pointer);                                         \
            Return;                                                                 \
        }                                                                           \
    } while (false)


#define CheckConvertToIntBits(NumBits, Target, Value, Return)       \
    do {                                                            \
        int64 StoreValue__ = (Value);                               \
        if (StoreValue__ < (int64)(Int ## NumBits ## Minimum) ||    \
            StoreValue__ > (int64)(Int ## NumBits ## Maximum))      \
        {                                                           \
            CheckLog4(ErrorLogMessage, SubsystemCode,               \
                      "%s %I64d is out of range for int%d %s",      \
                      #Value, StoreValue__, NumBits,  #Target);     \
            Return;                                                 \
        } else                                                      \
        {                                                           \
            (Target) = (int ## NumBits)StoreValue__;                \
        }                                                           \
    } while (false)

#define CheckConvertToUIntBits(NumBits, Target, Value, Return)      \
    do {                                                            \
        uint64 StoreValue__ = (uint64)(int64)(Value);               \
        if (StoreValue__ > (uint64)(UInt ## NumBits ## Maximum))    \
        {                                                           \
            CheckLog4(ErrorLogMessage, SubsystemCode,               \
                      "%s %I64d is out of range for uint%d %s",     \
                      #Value, StoreValue__, NumBits,  #Target);     \
            Return;                                                 \
        } else                                                      \
        {                                                           \
            (Target) = (uint ## NumBits)StoreValue__;               \
        }                                                           \
    } while (false)

#define CheckConvertToInt32(Target, Value, Return) CheckConvertToIntBits(32, Target, Value, Return)
#define CheckConvertToInt16(Target, Value, Return) CheckConvertToIntBits(16, Target, Value, Return)
#define CheckConvertToInt8(Target, Value, Return)  CheckConvertToIntBits(8, Target, Value, Return)

#define CheckConvertToUInt32(Target, Value, Return) CheckConvertToUIntBits(32, Target, Value, Return)
#define CheckConvertToUInt16(Target, Value, Return) CheckConvertToUIntBits(16, Target, Value, Return)
#define CheckConvertToUInt8(Target, Value, Return)  CheckConvertToUIntBits(8, Target, Value, Return)

END_GRANNY_NAMESPACE;

#include "header_postfix.h"
#define GRANNY_PARAMETER_CHECKING_H
#endif
