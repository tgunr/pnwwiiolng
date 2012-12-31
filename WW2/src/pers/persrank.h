#pragma once

#include <string>
#include <vector>

#include "libxml/parser.h"
#include "libxml/tree.h"

/************************************************************************************************
	persrank
*************************************************************************************************/

class persRank
{
public:
	int Level() const { return level; }
	int Type() const { return type; }

	const wchar_t* WideName(int country, int branch) const;
	const char* Name(int country, int branch) const;

enum
{
	ENLISTED, OFFICER, COMMAND, STAFF
};

	int level;
	int type;

	std::vector<std::vector<std::wstring> > names;
};

extern int persrankLoad(const char* name, xmlInputOpenCallback, xmlInputReadCallback, xmlInputCloseCallback);
extern persRank const* persrankFind(int level);
extern int persrankMax();
