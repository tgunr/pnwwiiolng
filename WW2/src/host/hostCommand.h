#pragma once

#include "chat/chatName.h"

namespace Host
{
	struct CommandMsg
	{
		CommandMsg(const char* fromNick, const char* withRecipient, const char* withInput, UINT64 withInstID=0ULL);
		virtual ~CommandMsg() {}

		typedef std::vector<std::string> ArgVector;

		std::string nick, recipient, cmd;
		ArgVector args;
		std::string input;
		UINT64 instID;

		const char* From() const { return nick.c_str(); }
		const char* Tell() const { return recipient.c_str(); }
		const char* Command() const { return cmd.c_str(); }
		const ArgVector& Args() const { return args; }
		const std::string& Args(size_t n) const { return args[n]; }
 		const char* Input() const { return input.c_str(); }
		void EraseArg(size_t n)
		{
			if ( n >= args.size() )
				return;
			if ( n == args.size() - 1 )
				args.pop_back();
			else
			{
				args.erase(args.begin() + n);
			}
		}

		enum ResponseType { _Caution, _Error, _Usage, _Reply };
		virtual void Reply(ResponseType, const char* format, ...) const = 0;
	};

	struct CellCommand : public CommandMsg
	{
		CellCommand(UINT32 playerID, const ChatHandle handle, const char* withInput, UINT64 withInstID)
			: CommandMsg(Chat::Name(handle).c_str(), nullptr, withInput, withInstID)
			, m_playerID(playerID)
			, m_handle(handle)
			{}
		virtual ~CellCommand() {}

		UINT32 m_playerID;
		ChatHandle m_handle;

		virtual void Reply(ResponseType, const char* fmt, ...) const override;
	};
}

