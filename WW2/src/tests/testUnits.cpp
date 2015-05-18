#include <syscfg.h>	// Path is project dependent.
// Test Unit Wrapper

#include "tests/testUnits.h"
#include "wwii/wwiiUTF.h"

#include <stdexcept>
#include <iostream>
#include <cwchar>

bool enableTestDebug = false;

typedef std::vector<TestUnit*> TestIndex;
// We have to make sure this gets instantiated before we try to use it,
// otherwise it may get instantiated after we've populated it and cleared.
static TestIndex& tests()
{
    static TestIndex s_tests;
    return s_tests;
}

//////////////////////////////////////////////////////////////////////
// Test Unit ctor

TestUnit::TestUnit(const char* unitName)
        : m_unit(unitName), m_stage("Initialization"), m_fails(""), m_success(false)
        , m_stagePasses(0), m_unitPasses(0)
        , m_stageErrors(0), m_unitErrors(0)
{
	teulClock64 = 0ULL;
    tests().push_back(this);
}

//////////////////////////////////////////////////////////////////////
// Process a failed test report


void
TestUnit::_failedTest(const char* file, unsigned int lineNo, const char* operation, const char* obtained, const char* expected, bool withOperands)
{
    // prefix, e.g.: file.cpp:25: testing explosions
    char str[256];
    strprintf(str, "\n%s:%u", file, lineNo);
    std::string text = str;
	text += ": error: ";

	// Indent with substage
    for ( size_t i = 0; i < m_subStage.size(); ++i )
    {
        text += m_subStage[i];
        text += " >> ";
    }

	// What operation was being attempted
    text += operation;

	// Add focus label if there was one
	if ( m_testFocus.empty() == false )
	{
		text += " [";
		text += m_testFocus;
		text += "]";
	}

	// Describe the result we got
    text += " was ";
    text += obtained;
    text += " instead of ";
    text += expected;

    if ( withOperands )
    {
        strprintf(str, " (%s vs %s)", m_lhsVal, m_rhsVal);
        text += str;
    }

    text += ".\n";

    Error(text.c_str());
}

//////////////////////////////////////////////////////////////////////
// Report an error in a test.

void
TestUnit::Error(const char* error, bool /*critical*/)
{
    std::string text = "";

    // First error in a stage, annotate the stage.
    if ( m_stageErrors == 0 )
    {
        text += "- ";
        text += m_stage;
        text += ":\n";
    }

    ++m_stageErrors;
    ++m_unitErrors;

    text += error;
    text += "\n";

    m_fails += text;
}

//////////////////////////////////////////////////////////////////////
// Set TestFocus to a wide character label by converting it to UTF8.

void
TestUnit::TestFocus(const wchar_t* label)
{
	char buffer[1024];
	assert( wcslen(label) * sizeof(wchar_t) < sizeof(buffer) );
	WWII::ConvertChars(label, buffer, sizeof(buffer));
	m_testFocus = buffer;
}

//////////////////////////////////////////////////////////////////////
// Set TestFocus to a format mask.

void
TestUnit::TestFocus(const char* fmt, ...)
{
	char buffer[256];
	buffer[254] = 0;
	buffer[255] = 0;
	va_list args;
	va_start(args, fmt);
	vsnprintf(buffer, 254, fmt, args);
	va_end(args);

	m_testFocus = buffer;
}

//////////////////////////////////////////////////////////////////////
// Create a test pass/fail summary

static const char*
_testResults(UINT32 passes, UINT32 fails)
{
    UINT32 total = passes + fails;
    static char status[128];
    snprintf(status, 127, "%u/%u %s passed", passes, total, (total == 1 ? "test" : "tests"));
    return status;
}

//////////////////////////////////////////////////////////////////////
// Run all registered tests.

void
TestUnit::RunTests()
{
    if ( tests().empty() )
        std::cout << "FAIL: No tests defined." << std::endl;

    if ( enableTestDebug )
        std::cout << "#" << tests().size() << " test(s) defined." << std::endl;

    UINT32 testsPassed = 0;
    UINT32 testsFailed = 0;
    for ( auto it = tests().begin(), end = tests().end(); it != end; ++it )
    {
        TestUnit* unit = (*it);
        try
        {
            if ( enableTestDebug )
                std::cerr << "#Running " << unit->Name() << std::endl;

			// Make sure they start with a consistent, non-zero, teulClock.
			ResetClock();

			// Run the test.
            bool succeeded = unit->Run();

            UINT32 unitPasses = unit->UnitPasses();
            UINT32 unitFails = unit->UnitErrors();

            testsPassed += unitPasses;
            testsFailed += unitFails;

            if ( succeeded && unit->m_fails.empty() )
            {
                std::cout << "OK: " << unit->Name() << " " << _testResults(unitPasses, unitFails) << std::endl;
                continue;
            }

            if ( unit->m_fails.empty() )
            {
                ++unitFails;
                ++testsFailed;
            }

            // Unit test failed.
            std::cerr << "FAIL: " << unit->Name() << " (" << _testResults(unitPasses, unitFails) << ")";
            if ( !succeeded )
                std::cerr << " [Run() returned 'false']";
            std::cerr << std::endl << unit->Fails();

            continue;
        }
        catch ( std::exception& e )
        {
            std::cerr << "FAIL: " << unit->Name() << ": " << e.what() << std::endl;
            ++testsFailed;
        }
        catch ( ... )
        {

            std::cerr << "FAIL: " << unit->Name() << ": Unhandled exception!" << std::endl;
            ++testsFailed;
        }

        testsFailed += unit->UnitErrors();
    }

    if ( testsFailed > 0 )
    {
        std::cerr << "FAILED: " << _testResults(testsPassed, testsFailed) << std::endl;
        exit(-1);
    }

    std::cout << "SUCCESS: " << _testResults(testsPassed, testsFailed) << std::endl;
}

//////////////////////////////////////////////////////////////////////
// Entry point

#define TEST_BITCOUNT(val_type, value, bits)	\
	static_assert(Teul::BitCount(val_type(value)) == bits, "BitCount(" #val_type "(" #value ")) != " #bits)

TEST_BITCOUNT(uint8_t, 0, 0);
TEST_BITCOUNT(uint8_t, 1, 1);
TEST_BITCOUNT(uint8_t, 2, 2);
TEST_BITCOUNT(uint8_t, 3, 2);
TEST_BITCOUNT(uint8_t, 4, 3);
TEST_BITCOUNT(uint8_t, 5, 3);
TEST_BITCOUNT(uint8_t, 6, 3);
TEST_BITCOUNT(uint8_t, 7, 3);
TEST_BITCOUNT(uint8_t, 8, 4);
TEST_BITCOUNT(uint8_t, 125, 7);
TEST_BITCOUNT(uint8_t, 126, 7);
TEST_BITCOUNT(uint8_t, 127, 7);
TEST_BITCOUNT(uint8_t, 128, 8);
TEST_BITCOUNT(uint8_t, 129, 8);
TEST_BITCOUNT(uint8_t, 130, 8);
TEST_BITCOUNT(uint8_t, 255, 8);

TEST_BITCOUNT(uint32_t, 0, 0);
TEST_BITCOUNT(uint32_t, 255, 8);
TEST_BITCOUNT(uint32_t, 256, 9);
TEST_BITCOUNT(uint32_t, 65534, 16);
TEST_BITCOUNT(uint32_t, 65535, 16);
TEST_BITCOUNT(uint32_t, 65536, 17);
TEST_BITCOUNT(uint32_t, 65537, 17);
TEST_BITCOUNT(uint32_t, (65536 | 256 | 31), 17);
TEST_BITCOUNT(uint32_t, (1 << 31) | (1 << 20) | (1 << 14) | (1 << 9) | (1 << 3), 32);

TEST_BITCOUNT(uint64_t, 0, 0);
TEST_BITCOUNT(uint64_t, 1, 1);
TEST_BITCOUNT(uint64_t, 2, 2);

TEST_BITCOUNT(int8_t, 0, 1);
TEST_BITCOUNT(int8_t, 1, 2);
TEST_BITCOUNT(int8_t, 2, 3);
TEST_BITCOUNT(int8_t, 3, 3);
TEST_BITCOUNT(int8_t, 4, 4);
TEST_BITCOUNT(int8_t, 5, 4);
TEST_BITCOUNT(int8_t, 6, 4);
TEST_BITCOUNT(int8_t, 7, 4);
TEST_BITCOUNT(int8_t, 8, 5);
TEST_BITCOUNT(int8_t, 125, 8);
TEST_BITCOUNT(int8_t, 126, 8);
TEST_BITCOUNT(int8_t, 127, 8);
TEST_BITCOUNT(int8_t, -1, 2);
TEST_BITCOUNT(int8_t, -2, 3);
TEST_BITCOUNT(int8_t, -3, 3);
TEST_BITCOUNT(int8_t, -4, 4);

TEST_BITCOUNT(int32_t, 0, 1);
TEST_BITCOUNT(int32_t, 1, 2);
TEST_BITCOUNT(int32_t, -1, 2);

TEST_BITCOUNT(int64_t, 0, 1);
TEST_BITCOUNT(int64_t, 1, 2);
TEST_BITCOUNT(int64_t, -1, 2);

int
main(int argc, char* argv[])
{
    if ( argc > 1 && strcmp(argv[1], "-d") == 0 )
    {
        std::cout << "#TestDebuggingEnabled" << std::endl;
        enableTestDebug = true;
    }

	Teul::ConfigVar::InitializeConfigVars();
    TestUnit::RunTests();

    exit(0);
}
