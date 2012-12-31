#include <stdio.h>
#include <stdlib.h>
#include <string.h>

double Cl[361];
double Cd[361];
double Cm[361];

FILE * Infile;
FILE * Outfile;

char Buffer[256];

char *tok;

int i,j;

int main(int argv, char ** argc)
{
	Infile = fopen("flatpl~1.300","r");

	do
	{
		if (fgets(Buffer,255,Infile) == 0)
		{
			break;
		}

		Buffer[255] = '\0';


	}	while (strncmp(Buffer,"[Set1-Polar1]",10) != 0);
	
	fgets(Buffer,255,Infile);
	fgets(Buffer,255,Infile);
	fgets(Buffer,255,Infile);

	while (fgets(Buffer,255,Infile))
	{
		tok = strtok(Buffer,"=");
		tok = strtok(NULL,"=");
		i = (int)atof(tok);
		tok = strtok(tok," ");
		tok = strtok(NULL," ");
		Cl[i+180] = atof(tok);
		tok = strtok(NULL," ");
		Cd[i+180] = atof(tok);
		tok = strtok(NULL," ");
		Cm[i+180] = atof(tok);
	}

	fclose(Infile);

	for (i=1,j=179;i<90;i++,j--)
	{
		Cl[j+180] = -Cl[i+180];
		Cd[j+180] = Cd[i+180];
		Cm[j+180] = -Cm[i+180];
	}

	for (i=1,j=-1;i<91;i++,j--)
	{
		Cl[j+180] = -Cl[i+180];
		Cd[j+180] = Cd[i+180];
		Cm[j+180] = -Cm[i+180];
	}

	for (i=0,j=-180;i<90;i++,j++)
	{
		Cl[j+180] = Cl[i+180];
		Cd[j+180] = Cd[i+180];
		Cm[j+180] = Cm[i+180];
	}

	Outfile = fopen("outfile.txt","w");

	for (i=-180;i<180;i++)
	{
		fprintf(Outfile,"%lf %lf %lf %lf\n",
				(double)i,
				Cl[i+180],
				Cd[i+180],
				Cm[i+180]);
	}

	fclose(Outfile);

	return 0;
}