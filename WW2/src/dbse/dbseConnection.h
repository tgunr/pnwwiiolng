#pragma once
// Originating Programmer: Shane "Elmo" Wickson

//This allows us to write the redundancy that Marty has such a hard on for
//OGS: Smart guy, that Marty. Bit of a prick, that Elmo.
struct DBCONNECTION
{
	SCHAR host[128];
	SCHAR port[128];
	SCHAR username[128];
	SCHAR password[128];
	SCHAR dbName[128];
};

