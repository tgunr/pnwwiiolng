#include <syscfg.h> // Path is project specific.
#include "wwii/wwiiCommandLineArgs.h"

static const char* programName;
typedef std::vector<Argument> Arguments;
static Arguments arguments;

//////////////////////////////////////////////////////////////////////
// Generate a textual description of a command line argument.

std::string
Argument::GetDescription() const
{
	std::string description = "";

	if ( flag )
	{
		description += '-';
		description += flag;
		if ( option )
			description += '|';
	}

	if ( option )
	{
		description += "--";
		description += option;
	}

	if ( valueName )
	{
		if ( description.empty() == false )
			description += ' ';
		description += '<';
		description += valueName;
		description += '>';
	}

	return description;
}

//////////////////////////////////////////////////////////////////////
// Report on command line usage.

static void NORETURN_FN _usage(const char* info)
{
	printf("Usage: %s", programName);
	for ( auto it = arguments.begin(); it != arguments.end(); ++it )
	{
		printf(" [%s]", (*it).GetDescription().c_str());
	}
	printf("\n");
	exit((info == nullptr) ? 0 : -1);
}

//////////////////////////////////////////////////////////////////////
// Report unknown arguments.

static void NORETURN_FN _unknownArgument(const char* a)
{
	printf("Unrecognized command line option '%s'.\n", a);
	_usage(a);
}

static void NORETURN_FN _unknownArgument(char a)
{
	char name[2] = { a, 0 };
	_unknownArgument(name);
}

//////////////////////////////////////////////////////////////////////
// Look up a flag argument

static Argument* _getArgument(char a)
{
	for ( auto it = arguments.begin(); it != arguments.end(); ++it )
	{
		if ( (*it).flag == a )
			return &(*it);
	}
	if ( a == '?' || a == 'h' )
		_usage(nullptr);
	return nullptr;
}

//////////////////////////////////////////////////////////////////////
// Look up a named argument

static Argument* _getArgument(const char* const a)
{
	const char* const option = a + 2;		// Skip the '--'.
	for ( auto it = arguments.begin(); it != arguments.end(); ++it )
	{
		if ( (*it).option == nullptr )
			continue;
		if ( strcmp((*it).option, option) == 0 )
			return &(*it);
	}
	if ( strcmp(a, "help") == 0 )
		_usage(nullptr);
	return nullptr;
}

//////////////////////////////////////////////////////////////////////
// Report an argument missing a value.

static void _missingValue(Argument* arg)
{
	printf("Command line option %s expects a value (%s).\n", arg->GetDescription().c_str(), arg->valueName);
	_usage(arg->valueName);
}

//////////////////////////////////////////////////////////////////////
// Arguments which don't have a switch or a name are considered
// positional, required arguments.

static void _getRequiredArgument(const char* value, size_t position)
{
	for ( auto it = arguments.begin(); it != arguments.end(); ++it )
	{
		if ( (*it).IsRequired() == false )
			continue;
		if ( position > 0 )
		{
			--position;
			continue;
		}
		(*it).handler(value);
		return;
	}

	printf("Unrecognized/unexpected command line argument '%s'.\n", value);
	_usage(value);
}

//////////////////////////////////////////////////////////////////////
// Add arguments to the vector.

void
Argument::Add(char flag, const char* const name, const char* const valueName, void (*handler)(const char*))
{
	Argument addition = { flag, name, valueName, handler };
	arguments.push_back(addition);
}

void
Argument::Add(const char* const valueName, void (*handler)(const char*))
{
	Argument addition = { 0, nullptr, valueName, handler };
	arguments.push_back(addition);
}

//////////////////////////////////////////////////////////////////////
// Process Command Line.

void
Argument::ProcessCommandLine(size_t argc, char* const argv[])
{
	programName = argv[0];

	size_t requiredArg = 0;

	for ( size_t i = 1; i < argc; ++i )
	{
		if ( argv[i][0] != '-' )
		{
			_getRequiredArgument(argv[i], requiredArg++);
			continue;
		}

		if ( argv[i][1] == '-' )
		{
			char* value = strchr(argv[i], '=');
			if ( value != nullptr )
				*(value++) = 0;
			Argument* arg = _getArgument(argv[i]);
			if ( arg == nullptr )
				_unknownArgument(argv[i]);

			if ( arg->valueName )
			{
				if ( value == nullptr )
				{
					if ( (i + 1) >= argc )
						_missingValue(arg);
					value = argv[++i];
				}
			}
			else if ( value )
			{
				printf("Command line option %s does not expect a value [%s].\n", arg->GetDescription().c_str(), value);
			}

			arg->handler(value);
		}
		else
		{
			// One or more switches.
			size_t switchList = i;
			for ( size_t a = 1; argv[switchList][a] != 0; ++a )
			{
				Argument* arg = _getArgument(argv[switchList][a]);
				if ( arg == nullptr )
					_unknownArgument(argv[switchList][a]);
				if ( arg->valueName )
				{
					if ( ++i >= (size_t)argc || argv[i][0] == '-' )
						_missingValue(arg);
					arg->handler(argv[i]);
				}
				else
					arg->handler(nullptr);
			}
		}
	}

	for ( auto it = arguments.begin(); it != arguments.end(); ++it )
	{
		if ( (*it).IsRequired() && requiredArg-- == 0 )
		{
			printf("Missing command line argument %s.\n", (*it).valueName);
			_usage((*it).valueName);
		}
	}
}

