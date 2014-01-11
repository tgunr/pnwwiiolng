#include <syscfg.h> // Path is project specific.
#include "tests/testUnits.h"
// Test Module for chat's CommandWithArgs class.

#include "chat/chatCommandWithArgs.h"
#include <stdexcept>

using namespace std;
using namespace WWII;

static const UINT32 MAX_MESSAGE_LEN = 400;

class MatchError : public logic_error
{
public: explicit MatchError(const char* problem, const char* got, const char* wanted)
					: logic_error(problem), m_got(got), m_wanted(wanted) {}
public:	const char* m_got;
public: const char* m_wanted;
};

class CommandWithArgsTest : public TestUnit
{
public:
	CommandWithArgsTest() : TestUnit("CommandWithArgs") {}

public: bool TestCommand(const char* source, const char* expectCommand, const char* expectArgs)
	{
		START_TEST_STAGE( source )

			char command[MAX_MESSAGE_LEN + 1];
			strcpy(command, source);

			Chat::CommandWithArgs result(command);

			CRITICAL( ExpectFalse( result.Args() == nullptr ) )
			CRITICAL( ExpectEqual(strcmp(result.Command(), expectCommand), 0) );
			CRITICAL( ExpectEqual(strcmp(result.Args(), expectArgs), 0) );

		END_TEST_STAGE( source )

		return true;
	}

public: virtual bool Run()
	{
		TestCommand("", "", "");
		TestCommand("a", "a", "");
		TestCommand("A", "a", "");
		TestCommand("aAa", "aaa", "");
		TestCommand(" aAaA", "aaaa", "");
		TestCommand("AaBbCcDd ", "aabbccdd", "");
		TestCommand(" \t \t AxYzA   \t \t", "axyza", "");
		TestCommand(" \t \t AxYzA     \tAbCdE", "axyza", "AbCdE");
		TestCommand(" \t \t AxYzA   \t \tAbCdE ", "axyza", "AbCdE");
		TestCommand(" \t \t AxYzA     \tAbCdE    ", "axyza", "AbCdE");
		TestCommand(" \t \t AxYzA   \t \tAbCdE \t     \t ", "axyza", "AbCdE");
		TestCommand(" \t \t AxYzA     \tAbCdE     \t     FGH  ijk", "axyza", "AbCdE FGH ijk");
		TestCommand("      \t     AxYzA \t     \tAbCdE         \t FGH  ijk\t   \t     ", "axyza", "AbCdE FGH ijk");
		TestCommand("七A丌C", "七a丌c", "");
		TestCommand(" 七A丌C", "七a丌c", "");
		TestCommand("七A丌C ", "七a丌c", "");
		TestCommand("  \t  七A丌C  \t  ", "七a丌c", "");
		TestCommand(" 七A丌C 七A丌C 七A丌C   七A丌C    七A丌C     ", "七a丌c", "七A丌C 七A丌C 七A丌C 七A丌C");
		TestCommand(" 凸冖亠龠1 七A丌C   凸冖亠龠1 七A丌C   七A丌C    七A丌C     ", "凸冖亠龠1", "七A丌C 凸冖亠龠1 七A丌C 七A丌C 七A丌C");
		TestCommand("spam +Spawning will remain locked while we wait for server population to build post patch process", "spam", "+Spawning will remain locked while we wait for server population to build post patch process");
		TestCommand("spam +As soon as we have sufficient population in game, spawning will be unlocked", "spam", "+As soon as we have sufficient population in game, spawning will be unlocked");
		TestCommand("spam +We hope the delay will not be very long", "spam", "+We hope the delay will not be very long");
		TestCommand("spam start 12 5m", "spam", "start 12 5m");
		return true;
	}
};

static CommandWithArgsTest test;
