#include <syscfg.h> // Path is project specific.
// Created: Fri Sep  9 14:10:00 CDT 2005
//
// Send the data from a teulTransmission class
// to an individual teulIndex
//
// Copyright (C) PlayNet, Inc 1999-2005 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

#include "teul/teulCallbackStruct.h"
#include "teul/teulEndpoint.h"
#include "teul/teulPackers.h"
#include "teul/teulPacket.h"
#include "teul/teulTransmission.h"

static char s_buffer[Teul::MaxPacket*2];

void*
TeulTransmission::_staticBuffer()
{
	return s_buffer;
}

bool
TeulTransmission::_checkData() const
{
	if ( m_callback == nullptr )
	{
		dbugLog(_FLA, "%s: tried to send TeulTransmission with NULL callback", __FUNCTION__);
		return false;
	}
	if ( m_buffer == nullptr )
	{
		dbugLog(_FLA, "%s tried to send TeulTransmission with NULL buffer", __FUNCTION__);
		return false;
	}
	if ( m_size < 0 )
	{
		dbugLog(_FLA, "%s tried to send TeulTransmission with invalid packed buffer [%d bytes]", __FUNCTION__, m_size);
		return false;
	}
	return true;
}

SINT32
TeulTransmission::send(SINT32 teulIndex) const
{
	ENDPOINT* const endpoint = teulGetEndpoint(teulIndex);
	if ( endpoint == nullptr )
		return -EINVAL;

	if ( endpoint->state() != IS_OPEN )
		return -EINVAL;

	Teul::Packet outgoing(m_size, Teul::PacketId(m_callback->callX(), m_callback->callY()));
	outgoing.append((UCHAR*)m_buffer, m_size);
	return outgoing.send(*endpoint);
}

//////////////////////////////////////////////////////////////////////
// Create an action object and pass it to the send function so that
// it populates our buffer with valid data.

SINT32
TeulTransmission::populate(void* data, SINT32 teulIndex)
{
	if ( m_callback == nullptr )
	{
		dbugLog(_FLA, "%s: no callback to populate transmission from", __FUNCTION__);
		return -ENOENT;
	}
	if ( m_buffer == nullptr )
	{
		dbugLog(_FLA, "%s: no buffer to populate transmission into", __FUNCTION__);
		return -EINVAL;
	}

	m_callback->outData = data;

	// All packers need an ACTION_OBJECT, and we
	ACTION_OBJECT actionObj;
	actionObj.teulIndex = teulIndex;
	actionObj.endpoint = teulGetEndpoint(teulIndex);
	actionObj.actionObj = m_callback;
	actionObj.localRawData = data;
	actionObj.packed = (UCHAR*)m_buffer;
	actionObj.offset = 0;
	actionObj.m_size_bytes = -1;

	m_size = (m_callback->m_sendAction)(&actionObj);
	if ( m_size < 0 )
	{
		dbugLog(_FLA, "t:%d cb:%u,%u pack failed:%d probable data corruption", teulIndex, m_callback->callX(), m_callback->callY(), m_size);
	}

	return m_size;
}
