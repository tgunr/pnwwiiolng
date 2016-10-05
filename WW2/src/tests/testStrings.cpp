#include <syscfg.h> // Path is project specific.
// Created: Mon Apr 25 11:12:00 CDT 2016
//
// Check that FullObjID conversion is working.
//
// Copyright (C) PlayNet, Inc 1999-2016 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

#include "tests/testUnits.h"

template<size_t Size>
struct ProtectedText
{
	ProtectedText() {
		memset(text, 0, sizeof(text));
		memset(guard, 0, sizeof(guard));
	}
	~ProtectedText() {
		for (size_t i = 0; i < sizeof(guard); ++i) {
			assert(guard[i] == 0);
		}
	}

	char text[Size];
	char guard[64];
};

class StringsTest : public TestUnit
{
public:
	StringsTest() : TestUnit("Strings") {}
public: virtual bool Run() override
{
	// Ensure consistent behavior of strsafecpy - in particular,
	// check the terminator is added when truncation occurs.
	START_TEST_STAGE("strsafecpy")
		START_SUB_STAGE("normal")
			ProtectedText<64> pt;
			CRITICAL(ExpectEqual(pt.text[0], 0));
			CRITICAL(ExpectEqual(pt.guard[0], 0));
			strsafecpy(pt.text, "hello");
			CRITICAL(ExpectEqual(strcmp(pt.text, "hello"), 0));
		END_SUB_STAGE("normal")
		START_SUB_STAGE("empty")
			ProtectedText<6> pt;
			pt.text[0] = 'A';
			strsafecpy(pt.text, "");
			CRITICAL(ExpectEqual(pt.text[0], 0));
		END_SUB_STAGE("empty")
		START_SUB_STAGE("truncating")
			ProtectedText<6> pt;
			strsafecpy(pt.text, "1234567890");
			CRITICAL(ExpectEqual(pt.guard[0], '\0'));
			CRITICAL(ExpectEqual(strcmp(pt.text, "12345"), 0));
		END_SUB_STAGE("truncating")
	END_TEST_STAGE("strsafecpy")

	// Ensure consistent behavior of strprintf - in particular,
	// check the terminator is added when truncation occurs.
	START_TEST_STAGE("strprintf")
		START_SUB_STAGE("normal")
			ProtectedText<64> pt;
			strprintf(pt.text, "!%s!", "testing");
			CRITICAL(ExpectEqual(strcmp(pt.text, "!testing!"), 0));
		END_SUB_STAGE("normal")
		START_SUB_STAGE("truncating")
			ProtectedText<7> pt;
			// pt has space for 7 characters, so it should fix
			// 'hello' + 'w' + '\0'
			strprintf(pt.text, "%s%s", "hello", "world");
			CRITICAL(ExpectEqual(strcmp(pt.text, "hellow"), 0));
		END_SUB_STAGE("truncating")
	END_TEST_STAGE("strprintf")

	return true;
}
};

static StringsTest test;
