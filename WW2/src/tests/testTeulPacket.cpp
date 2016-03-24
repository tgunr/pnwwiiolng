#include <syscfg.h> // Path is project specific.
// Created: Wed Mar 21 12:50:31 CDT 2012
//
// Tests for Teul::Packet
//
// Copyright (C) PlayNet Inc, 2012 -- All rights reserved.
//
// Originating Programmer: Oliver "kfsone" Smith

#include "tests/testUnits.h"
#include "teul/teulPacketHeader.h"
#include "teul/teulPacket.h"
#include "teul/teulEndpointDefs.h"	// For START_RAW_TOKEN etc

#pragma pack(push, 1)		//> start wire format
class TestStruct
{
	float			m_result;
	unsigned int	m_int;
	unsigned char	m_char;
	float			m_float;

public:
	// Try to make the constructor avoid putting the arguments on the stack
	// so that we don't accidentally detect the stack somehow.
	TestStruct(const unsigned int& int_, const unsigned char& char_, const float* float_)
		: m_result(0), m_int(int_), m_char(char_), m_float(*float_)
		{
			m_result = m_float * 100.0f;
			m_result += (float)m_int;
			m_result += (float)m_char;
		}

	unsigned int getInt() const { return m_int; }
	unsigned char getChar() const { return m_char; }
	float getFloat() const { return m_float; }
	float getResult() const { return m_result; }
};
#pragma pack(pop)			//< end wire format

class TestTeulPacket : public TestUnit
{
public:	TestTeulPacket() : TestUnit("Teul::Packet") {}
public: virtual ~TestTeulPacket() {}

public:	virtual bool Run() override
		{
			enum { TestCallX = 1, TestCallY = 10 };
			START_TEST_STAGE( "Constructor" )

				START_SUB_STAGE( "Basic" )

					// Test assumptions about the default configuration of the packet.
					Teul::Packet packet(Teul::PacketId(TestCallX, TestCallY));

					// In terms of data, the packet should be empty.
					CRITICAL( ExpectTrue( packet.empty() ) );

					// Capacity should be the default allocation.
					CRITICAL( ExpectEqual( packet.packetCapacityBytes(), Teul::Packet::DefaultLocalDataSize ) );

					// Header should have the X and Y values we supplied in the ctor.
					CRITICAL( ExpectEqual( packet.header().m_callX, TestCallX ) );
					CRITICAL( ExpectEqual( packet.header().m_callY, TestCallY ) );

					// Tag should be zero.
					CRITICAL( ExpectEqual( packet.header().m_tag, 0 ) );
					// Token should be "RAW"
					CRITICAL( ExpectEqual( packet.header().packetType(), Teul::Tokens::Encrypted ) );
					// Size should look good.
					CRITICAL( ExpectEqual( packet.header().packetSizeBytes(), packet.headerSizeBytes() ) );

					// Write pointer should look healthy.
					CRITICAL( ExpectEqual( (char*)packet.writePointer(), (char*)packet.packet() + packet.headerSizeBytes() ) );

					// Changing to unencryptable should change the token.
					packet.setUnencrypted();
					CRITICAL( ExpectEqual( packet.header().packetType(), Teul::Tokens::Unencrypted ) );

				END_SUB_STAGE( "Basic" )

			END_TEST_STAGE( "Constructor" )

			START_TEST_STAGE( "Reserve" )

				// Check that providing an initial capacity creates a capacity allocation.
				static const size_t requestCapacityBytes = Teul::Packet::GrowthRate * 8 + 1;
				static const size_t expectCapacityBytes = Teul::Packet::GrowthRate * 9;
				Teul::Packet packet(Teul::PacketId(TestCallX, TestCallY));
				packet.reserve(requestCapacityBytes);
				CRITICAL(ExpectEqual(packet.packetCapacityBytes(), expectCapacityBytes));
				CRITICAL(ExpectTrue(packet.empty()));

			END_TEST_STAGE( "Reserve" )

			// Test that adding data causes growth and/or reallocation.
			START_TEST_STAGE( "Growth" )

				Teul::Packet packet(Teul::PacketId(TestCallX, TestCallY));
				CRITICAL( ExpectEqual( packet.payloadCapacityBytes(), Teul::Packet::DefaultLocalDataSize - packet.headerSizeBytes() ) );

				// The packet starts using it's own, small internal data store for the
				// write space. We'll need a pointer to that to ensure an allocation happens.
				const unsigned int* const packetOwnData = (unsigned int*)packet.writePointer();

				// Requesting more than the default local size should cause growth to the next
				// multiple of growth size.
				static const size_t capacityGrowBytes = Teul::Packet::GrowthRate + 1;
				packet.setPacketCapacityBytes(capacityGrowBytes);
				CRITICAL( ExpectEqual( packet.packetCapacityBytes(), Teul::Packet::GrowthRate * 2 ) );
				// Check that reallocation happened.
				const unsigned int* const packetAllocData = (unsigned int*)packet.writePointer();
				CRITICAL( ExpectTrue( packetAllocData != packetOwnData ) );

				// But the payload size should be unchanged.
				CRITICAL( ExpectTrue( packet.empty() ) );

				// Grow the size to just shy of needing another allocation.
				static const size_t sizeGrowBytes = Teul::Packet::GrowthRate * 2 - sizeof(unsigned int);
				packet.setPacketSizeBytes(sizeGrowBytes);

				// This should NOT have modified capacity.
				CRITICAL( ExpectEqual( packet.packetCapacityBytes(), Teul::Packet::GrowthRate * 2 ) );

				packet.append((unsigned int)1);
				// This should NOT have modified capacity.
				CRITICAL( ExpectEqual( packet.packetCapacityBytes(), Teul::Packet::GrowthRate * 2 ) );
				// But size should be larger.
				CRITICAL( ExpectEqual( packet.packetSizeBytes(), sizeGrowBytes + sizeof(unsigned int) ) );

				packet.append((unsigned int)0);
				// This SHOULD have modified capacity.
				CRITICAL( ExpectEqual( packet.packetCapacityBytes(), Teul::Packet::GrowthRate * 3 ) );
				// And size should be larger.
				CRITICAL( ExpectEqual( packet.packetSizeBytes(), sizeGrowBytes + sizeof(unsigned int) + sizeof(unsigned int) ) );


			END_TEST_STAGE( "Growth" )

			START_TEST_STAGE( "Usage" )

				static const unsigned int testUint = 1;
				static const unsigned char testUchar = 4;
				static const float testFloat = 1.2f;
				static const float expectResult = (testFloat * 100) + (float)testUint + (float)testUchar;

				// Test the in-place constructor.
				Teul::Packet packet(Teul::PacketId(TestCallX, TestCallY));
				TestStruct& ts = packet.construct<TestStruct>(testUint, testUchar, &testFloat);

				// Ts should have the same address as the start of the payload.
				CRITICAL( ExpectEqual( (void*)&ts, packet.payload() ) );

				// The values should look correct.
				CRITICAL( ExpectEqual( ts.getInt(), testUint ) );
				CRITICAL( ExpectEqual( ts.getChar(), testUchar ) );
				CRITICAL( ExpectEqual( ts.getFloat(), testFloat ) );
				CRITICAL( ExpectEqual( ts.getResult(), expectResult ) );

				// Check capacity hasn't been incremented.
				CRITICAL( ExpectEqual( packet.packetCapacityBytes(), Teul::Packet::DefaultLocalDataSize ) );
				// But payload size should have increased.
				CRITICAL( ExpectFalse( packet.empty() ) );
				CRITICAL( ExpectEqual( packet.payloadSizeBytes(), sizeof(TestStruct) ) );

			END_TEST_STAGE( "Usage" )

			return true;
		}
};

static TestTeulPacket test;
