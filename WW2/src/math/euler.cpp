#include <syscfg.h> // Path is project specific.
/**** Euler.c - Convert Euler angles to/from matrix or quat ****/
/* Ken Shoemake, 1993 */
#include <math.h>
#include <float.h>
#include "pi/pi.h"
#include "math/euler.h"

/* Construct matrix from Euler angles (in radians). */
void Eul_ToMatrix(int order, piVECTOR ea, piMATRIX M)
{
int		i,j,k,h,n,s,f;
float ti, tj, th, ci, cj, ch, si, sj, sh, cc, cs, sc, ss;
#if platOS == OS_LINUX
	FLOAT32 m[4][4];

	memcpy((UCHAR*)m,(UCHAR*)&M,sizeof(piMATRIX));
#endif

  EulGetOrd(order,i,j,k,h,n,s,f);
  if (f == EulFrmR)
	{
		float t;
		t = ea.x;
		ea.x = ea.z;
		ea.z = t;
	}

  if (n != EulParOdd)	// was ==
	{
		ea.x = -ea.x;
		ea.y = -ea.y;
		ea.z = -ea.z;
	}

  ti = ea.x;
	tj = ea.y;
	th = ea.z;
  ci = (float)cos(ti);
	cj = (float)cos(tj);
	ch = (float)cos(th);
  si = (float)sin(ti);
	sj = (float)sin(tj);
	sh = (float)sin(th);
  cc = ci*ch;
	cs = ci*sh;
	sc = si*ch;
	ss = si*sh;
#if platOS == OS_LINUX
  if (s == EulRepYes)
	{
		m[i][i] = cj;	    m[j][i] =  sj*si;    m[k][i] =  sj*ci;
		m[i][j] = sj*sh;  m[j][j] = -cj*ss+cc; m[k][j] = -cj*cs-sc;
		m[i][k] = -sj*ch; m[j][k] =  cj*sc+cs; m[k][k] =  cj*cc-ss;
	}
	else
	{
		m[i][i] = cj*ch; m[j][i] = sj*sc-cs; m[k][i] = sj*cc+ss;
		m[i][j] = cj*sh; m[j][j] = sj*ss+cc; m[k][j] = sj*cs-sc;
		m[i][k] = -sj;	 m[j][k] = cj*si;    m[k][k] = cj*ci;
  }

	m[3][0]=m[3][1]=m[3][2]=m[0][3]=m[1][3]=m[2][3]=0.0;
	m[3][3]=1.0;

	memcpy((UCHAR*)&M,(UCHAR*)m,sizeof(piMATRIX));
#else
  if (s == EulRepYes)
	{
		M.m[i][i] = cj;	    M.m[j][i] =  sj*si;    M.m[k][i] =  sj*ci;
		M.m[i][j] = sj*sh;  M.m[j][j] = -cj*ss+cc; M.m[k][j] = -cj*cs-sc;
		M.m[i][k] = -sj*ch; M.m[j][k] =  cj*sc+cs; M.m[k][k] =  cj*cc-ss;
	}
	else
	{
		M.m[i][i] = cj*ch; M.m[j][i] = sj*sc-cs; M.m[k][i] = sj*cc+ss;
		M.m[i][j] = cj*sh; M.m[j][j] = sj*ss+cc; M.m[k][j] = sj*cs-sc;
		M.m[i][k] = -sj;	 M.m[j][k] = cj*si;    M.m[k][k] = cj*ci;
  }

	M.m[3][0]=M.m[3][1]=M.m[3][2]=M.m[0][3]=M.m[1][3]=M.m[2][3]=0.0;
	M.m[3][3]=1.0;
#endif
}

/* Convert matrix to Euler angles (in radians). */
piVECTOR Eul_FromMatrix(piMATRIX M, int order)
{
piVECTOR ea;
int i,j,k,h,n,s,f;
#if platOS == OS_LINUX
	FLOAT32 m[4][4];

	memcpy((UCHAR*)m,(UCHAR*)&M,sizeof(piMATRIX));
#endif

	EulGetOrd(order,i,j,k,h,n,s,f);
#if platOS == OS_LINUX
	if (s == EulRepYes)
	{
		float sy = (float)sqrt(m[j][i]*m[j][i] + m[k][i]*m[k][i]);
		if (sy > 16*FLT_EPSILON)
		{
			ea.x = (float)atan2(m[j][i], m[k][i]);
			ea.y = (float)atan2(sy, m[i][i]);
			ea.z = (float)atan2(m[i][j], -m[i][k]);
		}
		else
		{
			ea.x = (float)atan2(-m[k][j], m[j][j]);
			ea.y = (float)atan2(sy, m[i][i]);
			ea.z = 0;
		}
	}
	else
	{
		float cy = (float)sqrt(m[i][i]*m[i][i] + m[i][j]*m[i][j]);
		if (cy > 16*FLT_EPSILON)
		{
			ea.x = (float)atan2(m[j][k], m[k][k]);
			ea.y = (float)atan2(-m[i][k], cy);
			ea.z = (float)atan2(m[i][j], m[i][i]);
		}
		else
		{
			ea.x = (float)atan2(-m[k][j], m[j][j]);
			ea.y = (float)atan2(-m[i][k], cy);
			ea.z = 0;
		}
	}

	memcpy((UCHAR*)&M,(UCHAR*)m,sizeof(piMATRIX));
#else
	if (s == EulRepYes)
	{
		float sy = (float)sqrt(M.m[j][i]*M.m[j][i] + M.m[k][i]*M.m[k][i]);
		if (sy > 16*FLT_EPSILON)
		{
			ea.x = (float)atan2(M.m[j][i], M.m[k][i]);
			ea.y = (float)atan2(sy, M.m[i][i]);
			ea.z = (float)atan2(M.m[i][j], -M.m[i][k]);
		}
		else
		{
			ea.x = (float)atan2(-M.m[k][j], M.m[j][j]);
			ea.y = (float)atan2(sy, M.m[i][i]);
			ea.z = 0;
		}
	}
	else
	{
		float cy = (float)sqrt(M.m[i][i]*M.m[i][i] + M.m[i][j]*M.m[i][j]);
		if (cy > 16*FLT_EPSILON)
		{
			ea.x = (float)atan2(M.m[j][k], M.m[k][k]);
			ea.y = (float)atan2(-M.m[i][k], cy);
			ea.z = (float)atan2(M.m[i][j], M.m[i][i]);
		}
		else
		{
			ea.x = (float)atan2(-M.m[k][j], M.m[j][j]);
			ea.y = (float)atan2(-M.m[i][k], cy);
			ea.z = 0;
		}
	}
#endif

  if (n != EulParOdd)	// was ==
	{
		ea.x = -ea.x;
		ea.y = -ea.y;
		ea.z = -ea.z;
	}

  if (f == EulFrmR)
	{
		float t;
		t = ea.x;
		ea.x = ea.z;
		ea.z = t;
	}

  return (ea);
}
