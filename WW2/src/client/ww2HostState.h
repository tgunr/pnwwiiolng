#pragma once

namespace Teul { class Connection; }

namespace ww2
{
	class Client;

	/************************************************************************************************
	ww2::HostState - describe the state of a host connection
	*************************************************************************************************/

	struct HostState
	{
		enum CONDITIONS
		{
			ECONDITION = -1,		// error condition
			DISCONNECTED,		// we don't have a teul index
			CONNECTING,			// we're in the middle of logging in
			CONNECTED,			// we're logged in
			FATAL,				// we're in a fatal error condition
			EXITING,			// we are exiting the game
			PENDING				// we are waiting on a parent connection to resolve itself
		};

		enum ERROR_ACTIONS
		{
			DISREGARD = 0,
			LOG = 1,				// spit a message out to the log
			CHAT = LOG << 1,			// spit a loc message out to chat
			POPUP = CHAT << 1,			// fire off a localized popup
			DISCONNECT = POPUP << 1,			// go ahead and disconnect the connection
			RECONNECT = DISCONNECT << 1,	// try to reconnect
			EXIT = RECONNECT << 1,		// a fatal error has occurred, exit the game

			// some handy action levels
			// level1 simply logs the error and sends out a chat
			alREPORT = LOG | CHAT,
			// level2 also disconnects and reconnects
			alRETRY = alREPORT | DISCONNECT | RECONNECT,
			// level3 is bail time, no workie
			alABORT = alREPORT | POPUP | DISCONNECT | EXIT,
		};

	private:
		// no default constructor
		HostState();

	public:
		HostState(Client* c, int type) :
			client(c),
			connection(NULL),
			hostType(type),
			lastError(0),
			lastIndex(-1),
			connectState(DISCONNECTED),
			lastConnectState(DISCONNECTED),
			reportState(DISCONNECTED),
			connectCount(0),
			retryCount(0),
			errorName(""),
			parent(NULL),
			active(true),
			exiting(false),
			pendingActions(0)
		{
		}

		virtual ~HostState() {}

		// associated our client and type
		void Init(Client* c, int type);

		// this connection is dependent on another
		// connection; affects how error actions are handled
		void SetParent(HostState* hs);

		bool Error() const; // are we in an error state
		bool Open() const;	// do we have a teulIndex
		bool Connecting() const; // are we Open and trying to connect
		bool Connected() const; // are we Open and have been logged in

		bool IsType(int type) const; // are we this type of host?
		bool IsReconnecting() const; // are we trying to reconnect

		virtual bool CanConnect() const; // can we reconnect or not
		virtual bool Reconnect(); // try to reconnect
		virtual const char* FatalCallback() const; // which call back do we have the ui call when we have a pop
		virtual const char* HostName();
		virtual const wchar_t* DisplayName();
		virtual const char* ErrorName(SINT32 error);
		virtual SINT32 ErrorActions(SINT32 error);
		virtual void DoActions(SINT32 error, SINT32 actions);
		virtual void SetFatalError();
		virtual void Update();// handle any pending actions

		// get our teul index, if we have one
		// returns -1 if we're not connected
		int Index() const;
		int LastIndex() const;


		void SetWait();
		SINT32 GetWait() const; // get our wait type
		void ClearWait();

		bool Connect(Teul::Connection* conn, SINT32 condition);
		void Disconnect(SINT32 errorCode = 0);

		// returns true if the current state is different from the last
		// state; used to handle different transitions states
		bool OnTransition();

		// each subclass handles error conditionals possibly a little
		// differently; some have contect, some will try to reconnect
		virtual void OnError(Teul::Connection* conn, SINT32 error);

		// called when we despawn for each host state type
		virtual void OnDespawn() {}
		virtual void OnExit();

		// helpers
		void Reset();
		bool Ready(TeulTimeMS timeout);

		// client we're associated with
		ww2::Client* client;

		// my current teul connection
		Teul::Connection* connection;

		int hostType; // type of host this state represents
		int lastError; // last error condition received (connectionError)
		int lastIndex; // our last teul index
		int connectState; // see above
		int lastConnectState; // for transitions
		int reportState; // for reporting purposes
		int connectCount; // number of times this host has been connected to
		int retryCount; // number of reconnect retries
		std::string errorName; // loc string name of the last error
		HostState* parent; // host state this state depends on
		bool active; // can this state active (can connect to it)
		bool exiting; // are we exiting the game?
		SINT32 pendingActions; // what actions to take once our parent's situation is resolved
	};

	struct MapHostState : public HostState
	{
		MapHostState(ww2::Client* c);
		virtual ~MapHostState() {}

		// map specific handling
		virtual void OnDespawn() OVERRIDE_FN;
		virtual bool CanConnect() const OVERRIDE_FN; // can we reconnect or not
		virtual bool Reconnect() OVERRIDE_FN;
		virtual const char* HostName() OVERRIDE_FN;
		virtual SINT32 ErrorActions(SINT32 error) OVERRIDE_FN;
	};

	struct ChatHostState : public HostState
	{
		ChatHostState(ww2::Client* c);
		virtual ~ChatHostState() {}

		// chat specific handling
		virtual bool CanConnect() const OVERRIDE_FN; // can we reconnect or not
		virtual bool Reconnect() OVERRIDE_FN;
		virtual const char* HostName() OVERRIDE_FN;
		virtual SINT32 ErrorActions(SINT32 error) OVERRIDE_FN;
	};

	struct CellHostState : public HostState
	{
		CellHostState(ww2::Client* c);
		virtual ~CellHostState() {}

		// cell specific handling
		virtual void OnDespawn() OVERRIDE_FN;
		//virtual bool CanConnect() const; // can we reconnect or not
		virtual bool Reconnect() OVERRIDE_FN;
		virtual const char* FatalCallback() const OVERRIDE_FN;
		virtual const char* HostName() OVERRIDE_FN;
		virtual SINT32 ErrorActions(SINT32 error) OVERRIDE_FN;
		virtual void SetFatalError()  OVERRIDE_FN {}
	};


}
