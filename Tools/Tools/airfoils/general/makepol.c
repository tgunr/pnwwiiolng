#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "systypes.h" // for standard playnet c types

// this program "melds" two polar files
// we are assuming that from 45 degrees to 315 degrees
// the airfoil behaves just like a flat plate (for simplicity)
// from 30 to 45 and from 315 to 330 degrees we are merging
// the airfoil polar with the flatplate polar using a 
// sin curve.  From -30 to 30 degrees is straight airfoil
// polar.
//
// Usage is:
//
// makepol inputpolar flatpolar outputfile [flatpoleoffset]
//
// where "makepol" is the executable filename
// "inputpolar" is the -30 to 30 degree airfoil polar,
// "flatpolar" is the 360 degree flat plate polar file
// "outputfile" is the output merged polar
// and "[flatpoleoffset]" is an optional parameter that
//  offsets the flat polar by the specified number of degrees
//  in order to simulate a deflected control surface in
//  a tailslide situation.

static FLOAT64 _ClMult = 1.0;

static FLOAT64 _Cl[361];
static FLOAT64 _Cd[361];
static FLOAT64 _Cm[361];

static UCHAR _InputPolar[80];
static UCHAR _OutputPolar[80];
static UCHAR _FlatReferencePolar[80];
static SINT32 _FlatPolarDegreeOffset;

void _ReadFiles(void);
void _WriteOutputFile(void);


int main(int argv, char ** argc)
{

	if (argv < 4)
	{
		printf("Usage: makepol inputpolar flatpolar outputfile [flatplateoffset] [polarmult]\n");
		return 0;
	}

	strncpy(_InputPolar, argc[1],75);
	_InputPolar[75] = '\0';

	strncpy(_FlatReferencePolar, argc[2],75);
	_InputPolar[75] = '\0';
	
	strncpy(_OutputPolar, argc[3],75);
	_InputPolar[75] = '\0';

	if (argv > 4)
	{
		_FlatPolarDegreeOffset = atoi(argc[4]);
	}
	else
	{
		_FlatPolarDegreeOffset = 0;
	}

	if (argv > 5)
	{
		_ClMult = atof(argc[5]);
	}
	else
	{
		_ClMult = 1.0;
	}

	_ReadFiles();
	_WriteOutputFile();

	printf("Done creating merged polar file %s\n",_OutputPolar);

	return 0;
}

void _ReadFiles(void)
{
FILE * Inputfile;
FILE * Flatplatefile;
UCHAR Buffer[256];
UCHAR * tok;
SINT32 i;

FLOAT64 Cl,Cm,Cd;
FLOAT64 Index;

FLOAT64 TempNum;

	Inputfile = fopen(_InputPolar,"r");
	if (Inputfile == NULL)
	{
		printf("Cannot open input polar file\n");
		exit(0);
	}

	Buffer[255] = '\0'; // just to be sure string is terminated
	while (fgets(Buffer,255,Inputfile))
	{
		if (Buffer[0] == '#')
		{
			continue;
		}

		if (strncmp(Buffer,"Value",5) != 0)
		{
			continue;
		}

		if (strncmp(Buffer,"Values",6) == 0)
		{
			continue;
		}

		tok = strtok(Buffer,"=");
		if (tok == NULL)
		{
			continue;
		}
		tok = strtok(NULL,"=");
		if (tok == NULL)
		{
			continue;
		}
		TempNum = atof(tok);
		i = (int)TempNum;
		tok = strtok(tok," ");
		if (tok == NULL || i < -180 || i > 180)
		{
			continue;
		}
		tok = strtok(NULL," ");
		if (tok == NULL)
		{
			continue;
		}
		_Cl[i+180] = atof(tok) * _ClMult;
		tok = strtok(NULL," ");
		if (tok == NULL)
		{
			continue;
		}
		_Cd[i+180] = atof(tok);
		tok = strtok(NULL," ");
		if (tok == NULL)
		{
			continue;
		}
		_Cm[i+180] = atof(tok);

	}

	fclose(Inputfile);

	Flatplatefile = fopen(_FlatReferencePolar,"r");
	if (_FlatReferencePolar == NULL)
	{
		printf("Cannot open flatplat polar file\n");
		exit(0);
	}

	Buffer[255] = '\0'; // just to be sure string is terminated

	while (fgets(Buffer,255,Flatplatefile))
	{
		sscanf(Buffer,"%lf %lf %lf %lf",
					&Index,
					&Cl,
					&Cd,
					&Cm);

		i = (int)Index+_FlatPolarDegreeOffset;
		if (i >= 180)
		{
			i -= 360;
		}
		if (i < -180)
		{
			i += 360;
		}

		if (i < -180 || i > 180)
		{
			continue;
		}

		if (i >= -30 && i <= 30)
		{
/*
			if (i < 0)
			{
				if (_Cl[i+180] < Cl)
				{
					_Cl[i+180] = Cl;
				}
				if (_Cd[i+180] < Cd)
				{
					_Cd[i+180] = Cd;
				}
				if (_Cm[i+180] < Cm)
				{
					_Cm[i+180] = Cm;
				}
			}
			if (i >= 0)
			{
				if (_Cl[i+180] > Cl)
				{
					_Cl[i+180] = Cl;
				}
				if (_Cd[i+180] < Cd)
				{
					_Cd[i+180] = Cd;
				}
				if (_Cm[i+180] > Cm)
				{
					_Cm[i+180] = Cm;
				}
				continue;
			}
*/
		}
		else if (i < -60 || i > 60)
		{
			_Cl[i+180] = Cl;
			_Cd[i+180] = Cd;
			_Cm[i+180] = Cm;
		} 
		else if (i < -30 && i >= -60)
		{
			TempNum = _Cl[-30+180] - Cl;
			TempNum *= 1.0 - (-(i+30) / 30.0);
//			TempNum *= (1.0 - sin((-(i+30) / 30.0)*(3.141592654/2.0)));
			_Cl[i+180] = Cl + TempNum;

			TempNum = _Cm[-30+180] - Cm;
			TempNum *= 1.0 - (-(i+30) / 30.0);
//			TempNum *= (1.0 - sin((-(i+30) / 30.0)*(3.141592654/2.0)));
			_Cm[i+180] = Cm + TempNum;

			TempNum = _Cd[-30+180] - Cd;
			TempNum *= 1.0 - (-(i+30) / 30.0);
//			TempNum *= (1.0 - sin((-(i+30) / 30.0)*(3.141592654/2.0)));
			_Cd[i+180] = Cd + TempNum;
		}
		else if (i > 30 && i <= 60)
		{
		FLOAT64 TempNum1;
			TempNum = _Cl[30 + 180] - Cl;
			TempNum *= 1.0 - ((i-30)/30.0);
//			TempNum *= (1.0 - sin(((i-30) / 30.0)*(3.141592654/2.0)));
			_Cl[i+180] = Cl + TempNum;

			TempNum = _Cm[30 + 180] - Cm;
			TempNum *= 1.0 - ((i-30)/30.0);
//			TempNum *= (1.0 - sin(((i-30) / 30.0)*(3.141592654/2.0)));
			_Cm[i+180] = Cm + TempNum;

			TempNum = _Cd[30 + 180] - Cd;
			TempNum *= 1.0 - ((i-30)/30.0);
//			TempNum *= (1.0 - sin(((i-30) / 30.0)*(3.141592654/2.0)));
			_Cd[i+180] = Cd + TempNum;
		}

		
	}
	
	fclose(Flatplatefile);
}

void _WriteOutputFile(void)
{
FILE * OutputFile;
SINT32 i;

	OutputFile = fopen(_OutputPolar,"w");

	if (OutputFile == NULL)
	{
		printf("Cannot write output polar file\n");
		exit(0);
	}

	for (i=-180;i<180;i++)
	{
		fprintf(OutputFile,"%lf %lf %lf %lf\n",
				(double)i,
				_Cl[i+180],
				_Cd[i+180],
				_Cm[i+180]);
	}

	fclose(OutputFile);

}