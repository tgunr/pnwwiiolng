#pragma once

enum SERVER_TYPE;

namespace Server
{
	struct CommandLineOption
	{
		const char*	command;
		const char*	help;
		bool		wantParameter;
		void(*callback)(const char* parameter);
	};

	void InitializeProcess(const char* appName, SERVER_TYPE type, ERROR_FUNCTION* errorFnPtr, void(*killFnPtr)(int), int argc, const char* argv[], const std::vector<CommandLineOption>& commandLine);
}
