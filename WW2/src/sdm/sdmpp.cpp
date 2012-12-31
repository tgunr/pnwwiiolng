#include <syscfg.h> // Path is project specific.
// Created: Tue Mar 13 11:58:25 2001
//
// Copyright (c) 2000 Playnet, Inc., All Rights Reserved.
// Not to be distributed in API form except as part of a
// compiled library.
//
// Originating Programmer: Eric "Meat" Hall
// Supporting Programmer:  Shane "Elmo" Wickson

#if platOS == OS_WIN32
# include <direct.h>
#else
# include <dirent.h>
#endif

#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <string>
#include <map>

#include "sdm/sdmpp.h"
#include "sdm/sdmParse.h"
#include "include/defs.h"

static SINT32  scxMin = 0, scxMax = 0, scyMin = 0, scyMax = 0;
static FILE* man;
const char* program = "sdmpp" ;

static bool s_unlinksEnabled = true;
static unsigned int s_verbose;

//typedef std::map<int,std::string> XPATHS ;
//static XPATHS xpaths ;

static NORETURN_FN void
usage(int exitCode)
{
	fprintf(stderr, "Usage: %s [-lN -rN -bN -tN] [-ofilename] [-ddirname] [-u] [-v] [-Cdir]\n"
					"  -lN          Set min X to N.\n"
					"  -rN          Set max X to N.\n"
					"  -bN          Set min Y to N.\n"
					"  -tN          Set max Y to N.\n"
					"  -ofilename   Set output filename (default: manifest, use '-o- for stdout)\n"
					"  -ddirname    Set terrain directory (default: terrain)\n"
					"  -u           Toggle unlinking (default: on)\n"
					"  -v           Enable verbosity\n"
					"  -Cdirname    Change directory before processing (default: .)\n", program);
	exit(exitCode);
}

static bool
_processfile(std::string path, std::string filename)
{
	bool success = false ;
	bool doUnlink = false ;

	SINT32  retval;

	static std::string filepath, fullpath ;

	filepath = path + "/" + filename ;
	fullpath = WWIIOL_DATA_ROOT + filepath ;

	if ( s_verbose > 1 )
		fprintf(stderr, "# processing file: %s => %s => %s\n", filename.c_str(), filepath.c_str(), fullpath.c_str());

	retval = sdmOpenFile(filepath.c_str(), SDMPARSE_ALL);

	switch (retval)
	{
		case SDMFILE_READ_ERR:
			if ( s_verbose )
				fprintf(stderr, "# SDMFILE_READ_ERR: %s\n", filename.c_str());
			doUnlink = s_unlinksEnabled;
			//fprintf(stderr,"Error reading from file %s\n",filepath.c_str());
		break;
		case 0:		// No Error
		case SDMFILE_TRANS_ERR: // Translation error, carry on.
			if ( s_verbose )
				fprintf(stderr, "# SDMFILE_TRANS_ERR: %s\n", filename.c_str());
			fprintf(man, "%s\n", filepath.c_str());
			success = true ;
		break;
		case SDMNO_STRAT_DATA:
			if ( s_verbose > 2 )
				fprintf(stderr, "# SDMNO_STRAT_DATA: %s\n", filename.c_str());
			//fprintf(stderr,"No Strat data found in file %s \n",filename);
			doUnlink = s_unlinksEnabled ;
			success = true ;
		break;

		default:
				fprintf(stderr,"%s: sdmOpenFile(%s) returned an unexpected value", program, filepath.c_str());
	}

	sdmCloseFile();

	if ( doUnlink )
	{
		if ( s_verbose > 2 )
			fprintf(stderr, "# DELETING %s\n", filename.c_str());
		unlink(fullpath.c_str()) ;
	}

	return success ;
}

#if platOS != OS_WIN32
int select_fn(const struct dirent* file)
{
	return !(strncmp(file->d_name + (strlen(file->d_name) - 3),SDM_FILE_EXTENSION,3));
}

static bool
_getListOfSDMFiles(const char* const datadir, std::vector<std::string>& into)
{
	struct dirent** namelist = NULL;
	SINT32 n = scandir(datadir, &namelist, select_fn, alphasort);
	for ( SINT32 i = 0 ; i < n ; ++i )
	{
		into.push_back(namelist[i]->d_name);
		free(namelist[i]);
	}
	if ( namelist != NULL )
		free(namelist) ;
	return (n >= 0);
}
#else
static bool
_getListOfSDMFiles(const char* const datadir, std::vector<std::string>& into)
{
	std::string match = datadir;
	size_t slashPos;
	while ( (slashPos = match.find('/')) != match.npos )
	{
		match[slashPos] = '\\';
	}
	match += "\\*.sdm";

	WIN32_FIND_DATAA data;
	HANDLE hFind = FindFirstFileA(match.c_str(), &data);
	if ( hFind == INVALID_HANDLE_VALUE )
		return false;

	do 
	{
		into.push_back(data.cFileName);
	} while (FindNextFileA(hFind, &data));

	FindClose(hFind);

	return true;
}
#endif

static size_t
_processDirectory(const char* const datadir, const char* const trndir)
{
	std::vector<std::string> filenames;
	if ( _getListOfSDMFiles(datadir, filenames) == false )
	{
		fprintf(stderr, "%s: unable to scan %s\n", program, datadir);
		exit(-22);
	}
	if ( filenames.empty() )
	{
		fprintf(stderr, "%s: no sdm files found\n", program);
		exit(-23);
	}

	const size_t numFiles = filenames.size();
	if ( s_verbose > 1 )
		fprintf(stderr, "# Found %u files.\n", numFiles);

	size_t filesParsed = 0;
	for ( size_t i = 0 ; i < numFiles ; ++i )
	{
		if ( _processfile(trndir, filenames[i]) )
			++filesParsed;
	}

	if ( s_verbose )
		fprintf(stderr, "# Parsed %u files.\n", filesParsed);

	return filesParsed;
}

int
main(int argc, char** argv)
{
	program = argv[0] ;

	char filepath[PN_MAX_PATH],  manifile[PN_MAX_PATH], trndir[PN_MAX_PATH];

	strcpy(manifile, "data/importlist.txt");
	sprintf(trndir, "%s", SDM_FILE_PATH);

	bool haveL = false, haveR = false, haveB = false, haveT = false;
	for (int argn = 1; argn < argc; ++argn)
	{
		if(argv[argn][0] != '-')
			continue;
		switch (argv[argn][1])
		{
			case 'l':
				haveL = true;
		 		scxMin = atoi(&argv[argn][2]);
		 		break;
			case 'r':
				haveR = true;
				scxMax = atoi(&argv[argn][2]);
				break;
			case 'b':
				haveB = true;
				scyMin = atoi(&argv[argn][2]);
				break;
			case 't':
				haveT = true;
				scyMax = atoi(&argv[argn][2]);
				break;
			case 'o':
				strcpy(manifile,&argv[argn][2]);
				break;
			case 'd':
				strcpy(trndir,&argv[argn][2]);
				break;
			case 'u':
				s_unlinksEnabled = !s_unlinksEnabled;
				break;
			case 'v':
				++s_verbose;
				break;
			case 'C':
				if ( chdir(argv[argn]+2) < 0 )
				{
					fprintf(stderr, "Unable to change directory to '%s'\n", argv[argn]+2) ;
					exit(-1) ;
				}
				break ;
			default:
				usage(0);
				break;
		}
	}

	const bool haveAllCoords = (haveL & haveR & haveB & haveT);
	const bool haveSomeCoords = (haveL | haveR | haveB | haveT);

	if ( haveSomeCoords == true && haveAllCoords == false )
	{
		fprintf(stderr, "ERROR: %s: Must specify all of -l, -r, -b and -t together.\n", program);
		usage(-22);
	}

	if ( strcmp(manifile, "-") == 0 )
	{
		man = stdout;
		if ( s_verbose )
			fprintf(stderr, "# using stdout for manifest.\n");
	}
	else
	{
		man = fopen(manifile, "w");
		if ( man == NULL )
		{
			fprintf(stderr, "ERROR: %s: unable to write to manifest file, %s.\n", program, manifile);
			exit(-1);
		}

		if ( s_verbose )
			fprintf(stderr, "# manifest file: %s\n", manifile);
	}

	if ( haveSomeCoords == false )
	{
		if ( s_verbose )
			fprintf(stderr, "# Scanning all *.sdm files\n");

		// If you don't specify coordinate-ranges, read all files.
		char datadir[MAXPATHLEN + 1];
		sprintf(datadir, "%s%s", WWIIOL_DATA_ROOT, trndir);

		size_t files = _processDirectory(datadir, trndir);
		if ( files <= 0 )
		{
			fprintf(stderr, "%s: no processed? missing data/terrain/*.sdm?\n", program) ;
			exit(-22) ;
		}
	}
	else
	{
		for (SINT32 x = scxMin;x <= scxMax; x++)
		{
			for (SINT32 y = scyMin; y <= scyMax; y++)
			{
				sprintf(filepath,"%s/%04hx_%04hx.%s", trndir, x, y, SDM_FILE_EXTENSION);
				_processfile(trndir, filepath) ;
			}
		}
	}

	if ( man != stdout )
		fclose(man) ;

	return 0;
}
