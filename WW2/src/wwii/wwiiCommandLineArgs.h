#pragma once

//////////////////////////////////////////////////////////////////////
// Process command line switches.

struct Argument
{
public:
	static void ProcessCommandLine(size_t argc, char* const argv[]);
	static void Add(char _flag, const char* const _option, const char* _valueName, void (*handler)(const char*));
	static void Add(const char* const _valueName, void (*handler)(const char*));

	bool IsLast() const { return handler != nullptr; }
	bool IsRequired() const { return valueName != nullptr && flag == 0 && option == nullptr; }

	std::string GetDescription() const;

	char flag;
	const char* option;
	const char* valueName;
	void (*handler)(const char*);
};


