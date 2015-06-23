#pragma once

enum SERVER_TYPE;

#include <functional>

namespace Server
{
	struct CommandLineOption
	{
		const char*	command;
		const char*	help;
		bool		wantParameter;
		std::function<void(const char*)> callback;
	};

	using CommandLineOptions = std::vector<CommandLineOption>;
	void InitializeProcess(const char* appName, SERVER_TYPE type, ERROR_FUNCTION* errorFnPtr, void(*killFnPtr)(int), int argc, const char* argv[], const CommandLineOptions& commandLine);
}
