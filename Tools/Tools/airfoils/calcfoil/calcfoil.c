#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

static char _InputFileName[80];
static char _OutputFileName[80];

static FILE * _InputFile;
static FILE * _OutputFile;

static double _StartTransTop;
static double _EndTransTop;
static double _StartTransBottom;
static double _EndTransBottom;

#define _MAX_POS 256

static double _XPos[_MAX_POS];
static double _YtopPos[_MAX_POS];
static double _YbottomPos[_MAX_POS];
static double _CptopPos[_MAX_POS];
static double _CpbottomPos[_MAX_POS];

static double _AoA;
static double _NumPoints;

double _ReadPressureFile(FILE * InputFile);
double _CalcCl(FILE * InputFile);
double _CalcCm(FILE * InputFile);
double _CalcCdPressure(FILE * InputFile);

void main(int argc, char ** argv)
{
double Cl,Cm,CdP;


	if (argc < 3)
	{
		printf("\nUsage: calcfoil inputfile outputfile\n");
		exit(0);
	}

	strncpy(_InputFileName,argv[1],78);
	_InputFileName[79] = '\n';

	strncpy(_OutputFileName,argv[2],78);
	_OutputFileName[79] = '\n';


	strcpy(_InputFileName,"FLAT45.TXT");

	_StartTransTop = 0.10;
	_EndTransTop = 0.90;
	_StartTransBottom = 1.0;
	_EndTransBottom = 1.0;

	_InputFile = NULL;
	_InputFile = fopen(_InputFileName,"r");
	if (_InputFile == NULL)
	{
		printf("\nCan't open input file\n");
		exit(0);
	}

	_OutputFile = NULL;
	_OutputFile = fopen(_OutputFileName,"w");
	if (_OutputFile == NULL)
	{
		printf("\nCan't open output file\n");
		fclose(_InputFile);
		exit(0);
	}

	_ReadPressureFile(_InputFile);

	fclose(_InputFile);

	Cl = _CalcCl(_InputFile);	
	printf("\nCl initially is %lf\n",Cl);
	Cm = _CalcCm(_InputFile);
	
	CdP = _CalcCdPressure(_InputFile);
	printf("\nCdP initially is %lf\n",CdP);
	
	fclose(_OutputFile);
}

double _ReadPressureFile(FILE * InputFile)
{
char Buffer[256];
char * Tok;
int i;

	// ignore first 3 lines
	if (fgets(Buffer,255,InputFile) == NULL)
	{
		return 0.0;
	}
	if (fgets(Buffer,255,InputFile) == NULL)
	{
		return 0.0;
	}
	strtok(Buffer,"=");
	Tok = strtok(NULL,"=");
	if (Tok != NULL)
	{
		sscanf(Tok,"%lf",&_AoA);
	}
	else
	{
		_AoA = 0.0;
	}

	_AoA *= 3.141592654 / 180.0;

	if (fgets(Buffer,255,InputFile) == NULL)
	{
		return 0.0;
	}

printf("%lf %lf\n",_StartTransTop, _EndTransTop);

	i = 0;
	while (fgets(Buffer,255,InputFile) != NULL &&
		   i < _MAX_POS-1)
	{
		sscanf(Buffer,"%lf %lf %lf %lf %lf",
				&_XPos[i],
				&_YtopPos[i],
				&_YbottomPos[i],
				&_CptopPos[i],
				&_CpbottomPos[i]);

		if (_XPos[i] > _StartTransTop && _XPos[i] < _EndTransTop)
		{
			_CptopPos[i] = 0.7;
		}

		if (_XPos[i] > _StartTransBottom && _XPos[i] < _EndTransTop)
		{
			_CpbottomPos[i] = 0.7;
		}
		i++;
	}

	_NumPoints = i-1;

}

double _CalcCl(FILE * InputFile)
{
double Cl;
double DeltaX,DeltaYup,DeltaYdown;
double Cpup,Cpdown;
double Angle;
int i;

	Cl = 0.0;

	for (i=1;i<_NumPoints;i++)
	{
		DeltaX = _XPos[i] - _XPos[i-1];
		DeltaYup = _YtopPos[i] - _YtopPos[i-1];
		DeltaYdown = _YbottomPos[i] - _YbottomPos[i-1];

		Angle = atan2(DeltaYup,DeltaX) - _AoA;
		Cpup = cos(Angle) * (_CptopPos[i-1]);

		Angle = atan2(DeltaYdown,DeltaX) - _AoA;
		Cpdown = cos(Angle) * (_CpbottomPos[i-1]);

		Cl += -(Cpup - Cpdown) * DeltaX;
	}
		
	return Cl;
}

double _CalcCm(FILE * InputFile)
{
double Cm;
double CpTop,CpBottom;
double Cpup,Cpdown;
double Temp1;
double TempNum;
double CpTopPos,CpBottomPos;
double DeltaX,DeltaYup,DeltaYdown;
double Angle;
double CentPress;
double TorqueArm;
int i;
double TotalVectorLength;
double Force;
double VectorLength;
double VectorX,VectorY;
double ArmX,ArmY;
double CrossProd;

	CpTop = 0.0;
	CpBottom = 0.0;

	for (i=1;i<_NumPoints;i++)
	{
		DeltaX = _XPos[i] - _XPos[i-1];
		DeltaYup = _YtopPos[i] - _YtopPos[i-1];
		DeltaYdown = _YbottomPos[i] - _YbottomPos[i-1];

		Angle = atan2(DeltaYup,DeltaX);
		Cpup = cos(Angle) * (_CptopPos[i-1]);
		CpTop += (Cpup) * DeltaX;

		Angle = atan2(DeltaYdown,DeltaX);
		Cpdown = cos(Angle) * (_CpbottomPos[i-1]);
		CpBottom += (Cpdown) * DeltaX;
	}

	Temp1 = 0;
	for (i=1;i<_NumPoints;i++)
	{
		DeltaX = _XPos[i] - _XPos[i-1];
		DeltaYup = _YtopPos[i] - _YtopPos[i-1];

		Angle = atan2(DeltaYup,DeltaX);
		Cpup = cos(Angle) * (_CptopPos[i-1]);
		Temp1 += (Cpup) * DeltaX;

		if (fabs(Temp1) > fabs(CpTop / 2.0))
		{
			CpTopPos = _XPos[i-1];
			TempNum = Temp1 - CpTop / 2.0;
			TempNum = Cpup - TempNum;
			CpTopPos += DeltaX * (TempNum / Cpup);
			break;
		}
	}

	Temp1 = 0;
	for (i=1;i<_NumPoints;i++)
	{
		DeltaX = _XPos[i] - _XPos[i-1];
		DeltaYdown = _YbottomPos[i] - _YbottomPos[i-1];

		Angle = atan2(DeltaYdown,DeltaX);
		Cpdown = cos(Angle) * (_CpbottomPos[i-1]);
		Temp1 += (Cpdown) * DeltaX;

		if (fabs(Temp1) > fabs(CpBottom / 2.0))
		{
			CpBottomPos = _XPos[i-1];
			TempNum = Temp1 - CpBottom / 2.0;
			TempNum = Cpdown - TempNum;
			CpBottomPos += DeltaX * (TempNum / Cpdown);
			break;
		}
	}


	printf("CpTopPos = %lf\n",CpTopPos);
	printf("CpBottomPos = %lf\n",CpBottomPos);

	CentPress = (CpTopPos + CpBottomPos)/2.0;
	printf("CentPress = %lf\n",CentPress);

	TorqueArm = fabs(CpTopPos - CentPress);

	Cm = 0;

	TotalVectorLength = 0.0;
	// this is wrong
	for (i=1;i<_NumPoints;i++)
	{
		DeltaX = _XPos[i] - _XPos[i-1];
		DeltaYup = _YtopPos[i] - _YtopPos[i-1];
		DeltaYdown = _YbottomPos[i] - _YbottomPos[i-1];

		Force = _CptopPos[i-1];
		Angle = atan2(DeltaYup,DeltaX);
		Angle -= 3.141592654/2;
		VectorLength = sqrt(DeltaX*DeltaX + DeltaYup*DeltaYup);
		Force *= VectorLength;
		VectorX = cos(Angle) * Force;
		VectorY = sin(Angle) * Force;

		TotalVectorLength += VectorLength;

		ArmX = _XPos[i-1] - CentPress;
		ArmY = _YtopPos[i-1];

		CrossProd = VectorX * ArmY - ArmX * VectorY;
		Cm += CrossProd;


		Force = _CpbottomPos[i-1];
		Angle = atan2(DeltaYdown,DeltaX);
		Angle += 3.141592654/2;
		VectorLength = sqrt(DeltaX*DeltaX + DeltaYdown*DeltaYdown);
		Force *= VectorLength;
		VectorX = cos(Angle) * Force;
		VectorY = sin(Angle) * Force;

		TotalVectorLength += VectorLength;

		ArmX = _XPos[i-1] - CentPress;
		ArmY = _YbottomPos[i-1];

		CrossProd = VectorX * ArmY - ArmX * VectorY;
		Cm += CrossProd;

	}

	Cm /= TotalVectorLength;
	Cm *= 3.141592654;

	printf("\nCm = %lf",Cm);

	return 0.0;
}

double _CalcCdPressure(FILE * InputFile)
{
double Cd;
double DeltaX,DeltaYup,DeltaYdown;
double Cpup,Cpdown;
double Angle;
int i;

	Cd = 0.0;

	for (i=1;i<_NumPoints;i++)
	{
		DeltaX = _XPos[i] - _XPos[i-1];
		DeltaYup = _YtopPos[i] - _YtopPos[i-1];
		DeltaYdown = _YbottomPos[i] - _YbottomPos[i-1];

		Angle = atan2(DeltaYup,DeltaX) - _AoA;
		Cpup = sin(Angle) * (_CptopPos[i-1]);

		Angle = atan2(DeltaYdown,DeltaX) - _AoA;
		Cpdown = sin(Angle) * (_CpbottomPos[i-1]);

		Cd += -(Cpup - Cpdown) * DeltaX;
	}
		
	return Cd;
}
