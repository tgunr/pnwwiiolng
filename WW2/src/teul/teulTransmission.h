#pragma once
// Created: Thu Sep  8 11:12:04 CDT 2005
//
// Wrapper class for transmitting the same data to multiple
// recipients without repeating the teulPack overhead each time
//
// Copyright (C) PlayNet, Inc 1999-2005 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

struct FilterFn
{
};

class TeulTransmission
{
private:
	// Address of a shared, all-instances, buffer
	static void* _staticBuffer();

public:
	// Constructor - provide the data buffer or if you don't,
	// we'll use a single common buffer -- a nice cache hit
	// optimization.
	TeulTransmission(struct TEUL_CALLBACK* callback, void* withBuffer=_staticBuffer())
		: m_callback(callback), m_buffer(withBuffer), m_size(-1)
		{}

public:
	void* buffer() { return m_buffer; }
	SINT32 populate(void* data, SINT32 teulIndex=-1);	// Calls the function's send function with an ACTION_OBJECT

private:
	bool _checkData() const;

	// Handle a direct send request with a simple SINT32 argument
	// Private because there is no data validation in the send function
	// Call sendOne(teulIndex) if you want to one-shot
	SINT32 send(SINT32 teulIndex) const;

public:
	// Handle sequence container of SINT32 teulIndexes
	template<typename _Iterator>
	SINT32 send(const _Iterator& start, const _Iterator& end) const
	{
		if ( start == end ) return 0;
		if ( !_checkData() ) return -EINVAL;
		UINT32 sent = 0;
		_Iterator next = start;
		do
		{
			_Iterator cur = next;
			++next;
			if ( send(*cur) >= 0 )
				sent++;
		}
		while ( next != end );
		return sent;
	}

	// Handle sequence container of SINT32 teulIndexes
	template<typename _Iterator, typename _Filter>
	SINT32 send(const _Iterator& start, const _Iterator& end, const _Filter& filter) const
	{
		if ( start == end ) return 0;
		if ( !_checkData() ) return -EINVAL;
		UINT32 sent = 0;
		_Iterator next = start;
		do
		{
			_Iterator cur = next;
			++next;
			if ( filter(*cur) && send(*cur) )
				sent++;
		}
		while ( next != end );
		return sent;
	}

	template<typename _Key, typename _Data> SINT32 send(std::pair<_Key,_Data>& t) const { return send(t.second); }
	template<typename _Type> SINT32 send(const _Type* t) const { return send(t->teulIndex()); }
	template<typename _Type> SINT32 send(_Type* t) const { return send(t->teulIndex()); }
	template<typename _Type> SINT32 send(const _Type& t) const { return send(t.teulIndex()); }

	SINT32 sendOne(SINT32 teulIndex) { return (_checkData()) ? send(teulIndex) : -EINVAL; }

private:
	TEUL_CALLBACK*	m_callback;
	void*			m_buffer;
	SINT32			m_size;
};
