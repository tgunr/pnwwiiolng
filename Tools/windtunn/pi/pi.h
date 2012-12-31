// pi.h
//

#ifndef pi_HEADER

  #define pi_HEADER

  #include "syscfg.h"
  #include <ddraw.h>
  #include <d3d.h>

#ifdef __cplusplus
extern "C"
{
#endif




  typedef struct
  {
    FLOAT32   x;
    FLOAT32   y;
    FLOAT32   z;

  } piVECTOR, *piLPVECTOR;

 
  typedef struct
  {
    union
    {
      struct
      {
        FLOAT32        _11, _12, _13, _14;
        FLOAT32        _21, _22, _23, _24;
        FLOAT32        _31, _32, _33, _34;
        FLOAT32        _41, _42, _43, _44;
      };

      FLOAT32 m[4][4];
    };

  } piMATRIX, *piLPMATRIX;


#ifdef __cplusplus
}
#endif

#endif
