#pragma once

#include "teul/teulConnection.h"

namespace ww2 { class Client; };

///! @file client/clientConnections.h
///! @brief Describes the ClientConnection, MapConnection, ChatConnection and CellConnection classes.
///! @note Function defs are in ww2client, because of course they are...
class ClientHostConnection : public Teul::Connection
{
	public: ClientHostConnection(HostType _type
								, const Teul::ServerAddr& _serverInfo
								, const Teul::PlayerIdent& _playerInfo
								, UINT32 _timeoutMS
								)
								: Teul::Connection(_type, _serverInfo, _playerInfo, _timeoutMS) {}
	protected: ~ClientHostConnection() {}

   // our callbacks that lookup our client and call its process function
	protected: virtual void onLogin(ww2::Client* client);
	protected: virtual void onLoginFail(ww2::Client* client, SINT32 result);
	protected: virtual void onConnect(ww2::Client* client);
	protected: virtual void onError(ww2::Client* client, SINT32 error);

	public: void SetError(ww2::Client* client, SINT32 error);
};

//==============
// TODO: Consider moving to own file.

class LoginData {
public:
	LoginData(void);
	~LoginData();

	wchar_t m_username[20];
	UINT64  m_passMD5[2];   // Password MD5
	//UINT32  m_arenaID;
	bool    m_savePassword;
	bool    m_needsWrite;
	uint32_t m_steamAccountId;

	void GetPasswordMask(wchar_t* maskDest) const;
	void GetPasswordMD5(UCHAR* md5Dest, const size_t md5Size) const;
	bool GetUsernameAsUTF8(char* usernameDest, const size_t destSize) const;

	void SetPassword(bool savePassword, UINT32 passlen, const UCHAR* passMD5);

	// Load data from disk - does not perform any validation or conversion,
	// resulting data may not be in healthy state yet - see Validate()
	bool Read();

	// Check that what we read is valid and perform any revision conversions.
	bool Validate();

	void MarkForWrite() { m_needsWrite = true; }
private:
	// Save what we have - returns false if it is unable to save.
	bool Write();
};

//==============
// TODO: Consider moving

struct UsernamePassword {
	UsernamePassword(void) {}
	UsernamePassword(const std::string& un, const std::wstring& pass);

	std::string username = "";
	unsigned char password_md5[64+1] = {0};

	void SetPasswordString(const std::wstring&);
};

UsernamePassword GetSavedUserInfo(void);
void SaveUserInfo(const UsernamePassword&);

//==============

#include "auth/authPlaygateTypes.h" // TODO: Move

namespace Auth {
	struct GameEntry;
	struct LoginReply;
};

class AuthConnection {
public:
	// The last action we attempted to perform.
	enum ActionType {
		ACT_IDLE, ACT_ERROR, ACT_CONNECT, ACT_LOGIN, ACT_POLL_ARENAS, ACT_CHOOSE_ARENA, ACT_COUNT
	};

	// The state of the last/current action. This also doubles as the UI localized string describing the state.
	// Any states not mentioned here are errors.
	struct States {
		static const std::string Waiting;
		static const std::string Complete;
		static const std::string URLResponse;
		static const std::string FTPResponse;
	};

	struct Action {
		ActionType action;
		std::string state;
		std::wstring arg1;
		// HACK
		std::string url;
		std::string ftp;
	};

public:
	AuthConnection(const std::string& addr);
	~AuthConnection();

	const Action& GetAction(void) const { return action; }
	void ClearAction(void) { action = { ACT_IDLE }; }

	/// @note Distinct from Connected() because we might still be in the process of connecting.
	bool ConnectionGood(void) const;
	bool Connected(void) const;
	bool LoggedIn(void) const;

	void Login(const UsernamePassword&);
	void Login(uint32_t steamid);

	void PollArenas(void);

	const Auth::GameList& GetArenas(void) const { return arenas; }
	void SelectArena(const Auth::GameEntry&);

//private: // For teul callbacks
	void ConnectionOpened(const ENDPOINT& endpoint);
	void ConnectionError(SINT32 error);
	void LoginReplyReceived(const Auth::LoginReply&);
	void ArenaUpdateReceived(ACTION_OBJECT* a);
	void AppVerifyReceived(const APP_VERIFY& verify);
	void LaunchReplyReceived(const APP_LAUNCH&);
private:
	bool is_connected = false;
	bool is_logged_in = false;
	SINT32 teul_index = -1;
	SINT32 conn_error = 0;

	UsernamePassword login_info;
	uint32_t steam_id = 0;

	Action action;
	Auth::GameList arenas;
};
