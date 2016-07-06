#include <syscfg.h> // Path is project specific.
// Created: Mon Feb 14 16:02:22 CST 2005
//
// Retrieve game config from a database handle
//
// Copyright (C) 2005 Playnet, Inc., all rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

#include "include/mydbQuery.h"
#include "wwii/wwiiConfig.h"
#include "wwii/wwiiConfigMap.h"
#include "wwii/wwiiDBConfig.h"
#include "wwii/wwiiThreadTextQuery.h"

static UINT32 g_numVars = 0;

//////////////////////////////////////////////////////////////////////
// Function for mysql-escaping a string
void
wwiiEscapeString(std::string& text)
{
    
    for ( UINT32 n = 0; n < text.length(); ++n )
    {
        switch ( text[n] )
        {
        case 0:
            text[n] = ' ';
        break;
        case '\'':
        case '\\':
        case '\"':
        case ';':
            text.insert(n, 1, '\\');
            ++n;
        break;
        case '\n':
            text[n] = 'n';
            text.insert(n, 1, '\\');
            ++n;
        break;
        case '\r':
            text[n] = 'r';
            text.insert(n, 1, '\\');
            ++n;
        break;
        }
    }
}

//////////////////////////////////////////////////////////////////////

bool
dbUpdateWWIIConfigValue(const char* varName, const char* value, const char* modstamp)
{
	if ( varName == nullptr || *varName == 0 )
	{
		dbugLog(_FLA, "NULL variableName in config");
		return true;
	}

	if ( g_numVars == 0 )
		LogAbort("managed to call %s with numVars == 0", __FUNCTION__);


	if ( value == nullptr )
		value = "";
	if ( modstamp == nullptr )
		modstamp = "0";

	// Find a match
	UINT32 varNo;
	for ( varNo = 1; varNo < g_numVars; ++varNo )
	{
		if ( configMap[varNo].type == TYPE_UNUSED || configMap[varNo].name == nullptr )
			continue;
		if ( strcasecmp(configMap[varNo].name, varName) == 0 )
			break;
	}

	// Check whether we matched anything
	if ( varNo == 0 || varNo >= g_numVars || configMap[varNo].type >= MAX_WWII_CONFIG_TYPES )
	{
		dbugLog(_FLA, "Unknown config variable %s ignored", varName);
		return false;
	}

	// Unused (legacy) variable
	if ( configMap[varNo].type == TYPE_UNUSED )
		return false;

	UINT32 modified = (UINT32)atol(modstamp);
	if ( modified > host_config.lastModified )
		host_config.lastModified = modified;

	dbugLog(_FLD, "CONFIG: %s(%u)=%s [%s]"
					, configMap[varNo].name
					, varNo
					, value
					, modstamp
				);

	switch ( configMap[varNo].type )
	{
		case TYPE_INT:
			wwiiConfigSetINT((UINT32*)configMap[varNo].data, value, configMap[varNo].max);
		break;

		case TYPE_BOOL:
			wwiiConfigSetBOOL((UCHAR*)configMap[varNo].data, value, configMap[varNo].max);
		break;

		case TYPE_STRING:
			wwiiConfigSetSTRING((SCHAR*)configMap[varNo].data, value, configMap[varNo].max);
		break;

		case TYPE_SINT:
			wwiiConfigSetSINT((UINT16*)configMap[varNo].data, value, configMap[varNo].max);
		break;

		case TYPE_FLOAT:
			wwiiConfigSetFLOAT((FLOAT32*)configMap[varNo].data, value, configMap[varNo].max);
		break;

		case TYPE_BYTE:
			wwiiConfigSetBYTE((UCHAR*)configMap[varNo].data, value, configMap[varNo].max);
			break;

		default:
			dbugLog(_FLA, "Unknown config variable type %d for field %s(%u) ignored"
							, configMap[varNo].type
							, configMap[varNo].name
							, varNo
							);
		break;
	}

	return true;
}

// Function for obtaining the current host_config values
bool
dbGetWWIIConfig(DBHANDLE& dbh)
{
	static bool hasRun = false;
	if ( !hasRun )
	{
		dbugLog(_FLI, "Loading db config for first time (host_config size = %" PRSizeT " bytes)", sizeof(host_config));
		hasRun = true;
	}

	try
	{
		if ( g_numVars == 0 )
		{
			ThreadTextQuery qry(DBThread::Game, "INSERT IGNORE INTO `wwiionline`.`wwii_config` (variableName,value,lastModified,description) VALUES", "");
			// Count the number of variables we have
			for ( g_numVars = 0; configMap[g_numVars].type < MAX_WWII_CONFIG_TYPES; g_numVars++ )
			{
				if ( configMap[g_numVars].name && configMap[g_numVars].defaultValue && configMap[g_numVars].description )
				{
					std::string desc = configMap[g_numVars].description;
					wwiiEscapeString(desc);
					char buffer[255];
					strprintf(buffer, "('%s','%s',NOW(),'%s')",  configMap[g_numVars].name, configMap[g_numVars].defaultValue, desc.c_str());
					qry.addEntry(buffer);
				}
			}
			qry.commit(0);
			dbugLog(_FLI, "counted %u host_config variables", g_numVars);
		}

		static const char* const CONFIG_UPDATE_QUERY = "SELECT variableName, value, UNIX_TIMESTAMP(lastModified) FROM `wwiionline`.`wwii_config` WHERE UNIX_TIMESTAMP(lastModified) > %u";
		DBQuery q(dbh, CONFIG_UPDATE_QUERY, host_config.lastModified);

		if ( q.Rows() == 0 )
			return false;

		ThreadTextQuery qry(DBThread::Game, "DELETE FROM `wwiionline`.`wwii_config` WHERE `variableName` IN (", ")");
		while ( q.FetchRow() )
		{
			const DBRow& row = q.Row();
			if ( dbUpdateWWIIConfigValue(row[0], row[1], row[2]) == true )
				continue;
			char buffer[81];
			strprintf(buffer, "'%s'", row[0]);
			qry.addEntry(buffer);
		}
		qry.commit(0);

		return true;
	}
	catch (std::exception &e)
	{
		dbugLog(_FLA, "error reading host_config: %s", e.what());
		return false;
	}
}

