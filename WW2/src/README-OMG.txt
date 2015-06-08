OMG Language.

OMG is a code generator to eliminate the amount of boiler plate involved in using Teul.

To send a non-trivial message via TeulKit 1.0 you needed the following:

	a structure definition describing the elements,
	a packer function describing how to serialize/deserialize the structure,
	a send-wrapper, a function which marshalls the structure and invokes teulSend,
	a send-callback invoked by teulSend to push the structure's values into a wire buffer,
	a recv-callback invokved by teulSend to unpack wire data into a structure instance,
	a callback category (X) and type (Y) in a "somethingConstants.h" file,
	a TEUL_CALLBACK pointer to store the send/recv callbacks,
	hand-registration of the sender and receiver code,
	an error handler,

All of this usually happens in a "somethingMessages" or "somethingHostMessages" or
"somethingClientMessages" file, and as a result the functions in these files are often facades
that forward data to processing functions elsewhere - introducing more boiler plate.

Using TeulKit 1.0 the overall code might have looked something like this:

	8x --- cut here --- cut here --- cut here --- cut here --- cut here --- x8

	#define EXAMPLE_X 128   // Category for messages in this unit

	enum {
		...
		EXAMPLE_MESSAGE_Y,
		...
	};

	namespace Example
	{

	// The callback structure
	struct Message
	{
		WWII_SIDE		side;
		WWII_COUNTRY	country;
		ChatHandle		callsign;
		int				kills;
		ChatHandle		lastKill;
		int				lastKillPoints;

		SINT32 packer(LOGICAL mode, ACTION_OBJECT* const a);
	};

	}

	// 'mode' tells us whether we are packing or unpacking. I know, the
	// name says "packer" but it does both. The advantage of this is that
	// you often only have to write one routine to do both, they just tend
	// to get hard to read.
	SINT32 Example::Message::packer(LOGICAL mode, ACTION_OBJECT* const a)
	{
		a->packer(mode, side, 3); // pack 'side' as 3 bits,
		a->packer(mode, country, 5); // pack 'country' as 5 bits.
		// we used one byte to store two values
		a->packer(mode, callsign);
		a->packer(mode, kills);
		// if kills is zero, don't waste bandwidth sending other fields.
		if (kills) {
			a->packer(mode, lastKilled);
			a->packer(mode, lastKillPoints);
		} else if (mode == TEUL_UNPACK) {
			lastKilled = 0;
			lastKillPoints = 0;
		}
	}

	#ifdef EXAMPLE_SENDER

	static TEUL_CALLBACK* _SEND_MESSAGE;

	SINT32 exampleSendMessage(SINT32 teulIndex, WWII_SIDE side, WWII_COUNTRY country,
			ChatHandle callsign, int kills, ChatHandle lastKilled, int lastKillPoints)
	{
		Example::Message message;
		message.side = side;
		message.country = country;
		message.callsign = callsign;
		message.kills = kills;
		message.lastKilled = lastKilled;
		message.lastKillPoints = lastKillPoints;

		return teulSend(teulIndex, _SEND_MESSAGE, (void*)&message);
	}

	SINT32 _sendExampleMessage(ACTION_OBJECT* const a)
	{
		auto message = (Example::Message*)(a->localRawData);
		return message->packer(TEUL_PACK, a);
	}

	#else // EXAMPLE_RECEIVER

	#include "client.h"
	
	static TEUL_CALLBACK* _RECV_MESSAGE;

	SINT32 _recvExampleMessage(ACTION_OBJECT* const a)
	{
		Example::Message message;
		if (message.packer(TEUL_UNPACK, a) < 0)
			return a->offset;

		return clientProcessExampleMessage(message);
	}

	#endif

	static void _errorHandler(SINT32 teulIndex, const UCHAR* const data, SINT32 size, const TEUL_CALLBACK* const object, SINT32 error)
	{
		dbugLog(_FLA, "t:%d error %d ...", teulIndex, error);
	}

	SINT32
	exampleInstallBlahCallbacks()
	{
		Teul::CallbackInit ci;

		ci.callX = EXAMPLE_X;
		ci.error = _errorHandler;
		ci.timeout = 60000;

		#ifdef EXAMPLE_SENDER
			// Register a single callback
			ci.callY = EXAMPLE_MESSAGE_Y;
			ci.sendAction = _sendExampleMessage;
			ci.recvAction = nullptr;
			_SEND_MESSAGE = teulAllocCallback(&ci);
			if (!_SEND_MESSAGE) {
				dbugLog(_FLA, "Unable to register %d:%d", ci.callX, ci.callY);
				return -EINVAL;
			}
		#else // RECEIVER
			ci.callY = EXAMPLE_MESSAGE_Y;
			ci.sendAction = nullptr;
			ci.recvAction = _recvExampleMessage;
			_RECV_MESSAGE = teulAllocCallback(&ci);
			if (!_RECV_MESSAGE) {
				dbugLog(_FLA, "Unable to register %d:%d", ci.callX, ci.callY);
				return -EINVAL;
			}
		#endif

		// repeat this last block for all the other messages. It is legal
		// to have both a sender and a receiver use the same Y, it's assumed
		// that they are associated.
	}

	8x --- cut here --- cut here --- cut here --- cut here --- cut here --- x8

Because of this, "wwiiCallbacks" was introduced bringing down some of the headache,
and "Teul::Packet" was introduced to provide a wire-ready buffer so that there are
usually less copies involved in marshalling (you marshal directly to the wire buffer)

Still, a lot of work remains, and the decision made in 1999 to have bi-directional
packers which say

	if (mode == TEUL_PACK)
		...
	else

consistently hurt networking performance, however: the bi-directional packers have
saved WWIIOL from a lot of boiler plate and boiler-plate related bugs that can only
be avoided by having the code generated.

The goal of OMG is to replace ALL of the above with an OMG definition, something like:

	NAMESPACE Example;

	USING "typedefs.omg";

	MESSAGE Message
		[[src(server), dst(client), recv(clientHandleExample(teulIndex, obj))]]
	{
		WWII_SIDE		side;
		WWII_COUNTRY	country;
		ChatHandle		callsign;
		int				kills;
		if (kills) {
			ChatHandle		lastKill;
			int				lastKillPoints;
		}
	};

And from this, OMG should generate all of the neccessary .cpp and .h files based on the
command line parameters.

All of the structs defined by the .omg file would go into a single .h file. These .h files
are ideal candidates for inclusion per-project in the .pch file, which would help reduce
rather than increase compile times.


NAMESPACE <identifier>;

	Declares the namespace for the current .omg file. One per file. Required.

	e.g. NAMESPACE Chat;

DEBUG;

	Enables omg debug output.

USING "filename.omg";

	Imports data from another omg file.

	e.g. USING "typedefs.omg";

INCLUDE "filename";

	Adds an #include to the generated code.

	e.g. INCLUDE "sys/sysTypes.h";

TYPE <identifier> [AS <identifier>] [BY REF] [INCLUDE "filename"];

	Tells OMG about a type.

	AS provides the real, compile-time type name to be used, e.g.
		TYPE foo AS UINT64;
	will allow use of "foo" in the current unit to be used as an alias for UINT64.

	BY REF indicates that the type should be passed by reference.

	INCLUDE "filename" specifies an include file associated with a type. If the unit
	references this type, it will automatically add a #include statement to the
	generated code.

	e.g. TYPE UINT64;
	e.g. TYPE NetTimeMS AS Teul::NetTimeMS;
	e.g. TYPE WEAPON_SERIAL BY REF;
	e.g. TYPE SERIALNO AS WEAPON_SERIAL BY REF INCLUDE "weap/weapSerial.h";

START-Y <number>;

	Sets the next 'Y' value to use for the next message declared in this unit.

	e.g. START-Y 100;

ENUM <identifier> { <value> [, <value> [, ...]] [,] };

	Declares a named enumerator, but also tracks the value ranges so that packers
	can predict how many bits are required to transmit them.

	e.g. ENUM Side { None, Allied, Axis, Max };
	e.g. ENUM Packing { Unpack, Pack, };

MESSAGE <identifier> { <elements> };

	Declares a message. A message is a structure with several additional properties:
		- A CallbackY enum,
		- A default constructor,
		- For simple types, a memberwise-constructor,
		- pack/unpack functions,
		- Static TEUL_CALLBACK pointer,
		- A Callback() function that returns the TEUL_CALLBACK pointer,

	Elements can be fields, arrays, groups, conditional blocks and function prototypes.

Elements of MESSAGE:

	<typename> <identifier>;

		Declares a member variable.

	GROUP <identifier> { <elements> };

		Declares a VarGroup: A structure that tracks changes to a series of members,
		such that a single bit is used to detect whether any have changed and when.
		Especially useful for properties that are less likely to be changed and
		benefit from tracking timestamps.

		e.g. GROUP Strat {
			FULL_OBJ_ID origin_cp;
			FULL_OBJ_ID origin_fac;
			FULL_OBJ_ID target_cp;
			FULL_OBJ_ID target_fac;
		};

		On the wire, each GROUP in a MESSAGE adds an additional bit overhead to indicate
		whether or not that group is being packed. When packed, each group adds one bit
		per member to flag whether or not they are describing that member. These bits are
		rounded up to the next whole byte (see wwiiSyncVar.cpp). Thus a MESSAGE with
		3 GROUPs requires a BYTE of bit-fields to describe which groups are being packed.
		A group with 10 fields requires requires 2 BYTES to describe which fields are
		being packed.

		Group elements can be fields or function prototypes.

	ARRAY <identifier>[<dimension>] { <elements>; }

		Declares a sub-type, "struct <identifier>", for which a corresponding variable-
		sized array (using inplace_vector) will be added, including a population count.

		Over the wire, we transmit count plus only those copies that are active.

		Given ARRAY FOO[32] { ... } the following are declared:

		    struct Foo { ... };
			using Foos = WWII::inplace_vector<Foo, 32>;
			Foos foos;

		Array elements can be fields, arrays or conditional blocks.

		e.g. ARRAY Player[32] {
			ChatHandle		callsign;
			bool			online;
			if (online) {
				missionid_t missionID;
				FULL_OBJ_ID	nearCP;
				ARRAY Kill[8] {
					WEAPON_SPEC	weaponUsed;
					WEAPON_SPEC weaponKilled;
					time_t		timestamp;
				};
			}
		};

		Describes an array of [0, 32] "Player" objects in an array type "Players" stored
		as an element "players", and within each Player there can be an array of [0, 8]
		"Kill" objects which describe (perhaps) a player's last <= 8 kills. But these
		are only transferred over the wire if the player is marked as being online.

Field Definitions:

	Currently the syntax for field definitions is simply

		<typename> <identifiername>;

	e.g. UINT32 kills;
	e.g. FLOAT32 x;

	ABSENT:
		comma separated values, e.g. FLOAT32 x, y, z;
		bit-specifiers, e.g. UINT32 kills : 3;
		array support, e.g. char name[64];
		character array specific support, e.g. char name[..64];
