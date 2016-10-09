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

#include <sys/stat.h>
#include <sys/types.h>

#include "sdm/sdmpp.h"
#include "sdm/sdmParse.h"
#include "include/defs.h"

#include "sdm/sdmConfigVars.h"

static FILE* man;
const char* program = "sdmpp";

static bool s_unlinksEnabled = true;
static bool s_renamesEnabled = false;
static unsigned int s_verbose;

//using XPATHS = std::map<int,std::string>;
//static XPATHS xpaths;

NORETURN_FN static void
usage(int exitCode)
{
	fprintf(stderr, "Usage: %s [-ofilename] [-ddirname] [-u] [-v] [-Cdir]\n"
					"  -ofilename   Set output filename (default: %s, use '-o- for stdout)\n"
					"  -ddirname    Set terrain sub-directory (default: %s)\n"
					"  -u           Toggle unlinking (default: on)\n"
					"  -v           Enable verbosity\n"
					"  -Cdirname    Working directory (default: %s)\n"
					, program
					, cv_importFile.GetString()
					, cv_terrainSubdir.GetString()
					, cv_dataDir.GetString()
					);
	exit(exitCode);
}

static bool
_processfile(const std::string& filename)
{
	bool success = false;
	bool doUnlink = false;

	SINT32  retval;

	char filepath[PATH_MAX+1];
	strprintf(filepath, "%s/%s", cv_terrainSubdir.GetString(), filename.c_str());

	if ( s_verbose > 1 )
	{
		char fullpath[PATH_MAX+1];
		strprintf(fullpath, "%s/%s", cv_dataDir.GetString(), filepath);
		fprintf(stderr, "# processing file: %s => %s (%s)\n", filename.c_str(), filepath, fullpath);
	}

	retval = sdmOpenFile(filepath, SDMPARSE_ALL);

	switch (retval)
	{
		case SDMFILE_READ_ERR:
			if ( s_verbose )
				fprintf(stderr, "# SDMFILE_READ_ERR: %s\n", filename.c_str());
			doUnlink = true;
			//fprintf(stderr,"Error reading from file %s\n",filepath.c_str());
		break;
		case 0:		// No Error
		case SDMFILE_TRANS_ERR: // Translation error, carry on.
			if ( s_verbose )
				fprintf(stderr, "# SDMFILE_TRANS_ERR: %s\n", filename.c_str());
			fprintf(man, "%s\n", filepath);
			success = true;
		break;
		case SDMNO_STRAT_DATA:
			if ( s_verbose > 2 )
				fprintf(stderr, "# SDMNO_STRAT_DATA: %s\n", filename.c_str());
			//fprintf(stderr,"No Strat data found in file %s \n",filename);
			doUnlink = true;
			success = true;
		break;

		default:
				fprintf(stderr,"%s: sdmOpenFile(%s) returned an unexpected value", program, filepath);
	}

	sdmCloseFile();

	if ( doUnlink )
	{
		if ( s_verbose > 2 )
			fprintf(stderr, "# DELETING %s\n", filename.c_str());
		if ( s_renamesEnabled )
		{
			char newFilename[PATH_MAX+1];
			strprintf(newFilename, "%s.old", filepath);
			rename(filepath, newFilename);
		}
		if ( s_unlinksEnabled )
			unlink(filepath);
	}

	return success;
}

#if platOS != OS_WIN32
int select_fn(const struct dirent* file)
{
	return !(strncmp(file->d_name + (strlen(file->d_name) - 3),SDM_FILE_EXTENSION,3));
}

static bool
_getListOfSDMFiles(std::vector<std::string>& into)
{
	struct dirent** namelist = nullptr;
	SINT32 n = scandir(cv_terrainSubdir.GetString(), &namelist, select_fn, alphasort);
	for ( SINT32 i = 0; i < n; ++i )
	{
		into.push_back(namelist[i]->d_name);
		free(namelist[i]);
	}
	if ( namelist != nullptr )
		free(namelist);
	return (n >= 0);
}
#else
static bool
_getListOfSDMFiles(std::vector<std::string>& into)
{
	std::string match = cv_terrainSubdir.GetString();
	size_t slashPos;
	while ( (slashPos = match.find('/')) != match.npos )
	{
		match[slashPos] = '/';
	}
	match += "/*.sdm";

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
_processDirectory()
{
	std::vector<std::string> filenames;
	if ( _getListOfSDMFiles(filenames) == false )
	{
		fprintf(stderr, "%s: unable to scan %s\n", program, cv_terrainSubdir.GetString());
		exit(-22);
	}
	if ( filenames.empty() )
	{
		fprintf(stderr, "%s: no sdm files found\n", program);
		exit(-23);
	}

	const size_t numFiles = filenames.size();
	if ( s_verbose > 1 )
		fprintf(stderr, "# Found %" PRSizeT " files.\n", numFiles);

	size_t filesParsed = 0;
	for ( size_t i = 0; i < numFiles; ++i )
	{
		if ( _processfile(filenames[i]) )
			++filesParsed;
	}

	if ( s_verbose )
		fprintf(stderr, "# Parsed %" PRSizeT " files.\n", filesParsed);

	return filesParsed;
}

int
main(int argc, char** argv)
{
	program = argv[0];

	Teul::ConfigVar::InitializeConfigVars();

	for (int argn = 1; argn < argc; ++argn)
	{
		if(argv[argn][0] != '-')
			continue;
		if ( argv[argn][1] == 0 )	// "-" argument.
			break;

		const char* argValue = argv[argn][2] ? argv[argn] + 2 : nullptr;

		switch (argv[argn][1])
		{
			case 'o':
				cv_importFile.Set(argValue);
				break;
			case 'd':
				cv_terrainSubdir.Set(argValue);
				break;
			case 'u':
				s_unlinksEnabled = !s_unlinksEnabled;
				break;
			case 'r':
				s_renamesEnabled = !s_renamesEnabled;
				break;
			case 'v':
				++s_verbose;
				break;
			case 'C':
				cv_dataDir.Set(argValue);
				break;
			default:
				usage(0);
				break;
		}
	}

	if ( cv_dataDir.GetString() == nullptr || *cv_dataDir.GetString() == '\0' )
		cv_dataDir.Set(".");

	if ( chdir(cv_dataDir.GetString()) < 0 )
	{
		fprintf(stderr, "FAIL: Unable to access %s (%s working directory)",  cv_dataDir.GetString(), program);
		exit(-1);
	}

	if ( strcmp(cv_importFile.GetString(), "-") == 0 )
	{
		man = stdout;
		if ( s_verbose )
			fprintf(stderr, "# using stdout for manifest.\n");
	}
	else
	{
		man = fopen(cv_importFile.GetString(), "w");
		if ( man == nullptr )
		{
			fprintf(stderr, "ERROR: %s: unable to write to manifest file, %s.\n", program, cv_importFile.GetString());
			exit(-1);
		}

		if ( s_verbose )
			fprintf(stderr, "# manifest file: %s\n", cv_importFile.GetString());
	}

	if ( s_verbose )
		fprintf(stderr, "# Scanning all *.sdm files\n");

	size_t files = _processDirectory();
	if ( files <= 0 )
	{
		fprintf(stderr, "%s: no files processed? No .sdm files in %s/%s ?\n"
							, program, cv_dataDir.GetString(), cv_terrainSubdir.GetString()
							);
		exit(-22);
	}

	fclose(man);

	return 0;
}
