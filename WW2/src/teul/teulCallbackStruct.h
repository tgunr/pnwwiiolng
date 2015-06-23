#pragma once

#include "teul/teulFuncTypes.h"		// For ACTION and ERROR_FUNCTION

/*!
 @brief CALLBACK Objects.

	Each action of sending data back and forth over a connection must be tied
directly to an object entry in the callback array.  This entry tells the
receiving teul transport which object conatining the routine's and filters
to that deal with the data that it receives. Each tuel object also contains
all of the  communication parameters, and references to the specific routines
that manipulate it (packer, send, callback, error handler, etc..).  By
combining this all into one object we save ourselves a lot of repetitive work.
The developer no longer has to worry about all the parameters after an object
is initialized unless he needs to change them (typically these do not change).

	This is critical for acknowledged messages.  The point of an acknowledged
message is that if it times out the sending application can decide weather to
re-send it.  Without a reference to the sending object you do not know what
routine sent it nor how to rebuild it if you did.  By using teul objects, you
not only know what routine sent the object and how to rebuild it, but also can
make generic routines that handle errors and rebuild or re-send objects based
the error type and object parameters.  Write once, use many!

	The call back array is an 256 x 256 array of object references that has
several columns dedicated to the various levels of the teul system.  There is
a column dedicated to the tuel transport and net module.  There is also at
east one column associated with the utility layer.  In all the first 16
columns are dedicated to the teul system.  The remaining columns are available
for the application.  This gives the developer 240 columns X 255 rows of callback
object entries, or up to 61440 entries.

	Each of these callbacks must be initialized before use.  Unused entries
are automatically routed to the endpoint's error handler.  At initialization
you have the option of associating a pack routine that provides the callback
with the size of your static message structure.  This size is checked when a
message is received to ensure that the message is valid.  Messages that have
a variable length must not associate a pack routine with the size so that the
developer's callback routine can check the size instead.

*/

// when we que data we can either que the static image of the data (take
// a snapshot, or we can wait till we can send the data and pull the data
// then.

struct TEUL_CALLBACK
{
	ERROR_FUNCTION* error;			// Error handling function (or null).
	UINT32		timeout;			// how long do we try to send data (zero for connection default)
									// to delayTeimout;
	void*		inData;				// pointer to the input object
	void*		outData;			// pointer to the output object
	void*		filterData;			// pointer to data filled out by a specified filter

	UINT32		lastSent;
	UINT32		xferRate;

	ACTION_FN	m_sendAction;		// how to send data
	ACTION_FN	m_recvAction;		// how to receive data

	UINT8			m_callX;		// the X and Y position in the callback matrix
	UINT8			m_callY;		// the developer gets a pointer to obj,

public:
	ACTION_FN	sendAction() const { return m_sendAction; }
	ACTION_FN	recvAction() const { return m_recvAction; }

	const UINT8&	callX() const { return m_callX; }
	const UINT8&	callY() const { return m_callY; }

public:
	static TEUL_CALLBACK*	s_callbackArray[256][256];

public:
	//! @brief Lookup a callback by it's X/Y values packed as bytes in a stream.
	//! @detail When a callback is transmitted its structure is flattened or packed into a
	//! character array.  The first 2 characters of this array are the indexes
	//! to the callback array containing the callback.  This routine
	//! returns the public TEUL_CALLBACK* for the supplied data.
	static TEUL_CALLBACK* Lookup(const UCHAR* const data_) { return s_callbackArray[data_[0]][data_[1]]; }

	//! @brief Lookup a callback by it's X and Y values.
	static TEUL_CALLBACK* Lookup(const UINT8 x_, const UINT8 y_) { return s_callbackArray[x_][y_]; }

	void Release();

	SINT32 processCallback(struct ENDPOINT& endpoint_, UCHAR* const data_, const SINT32 dataSize_bytes_);
};

