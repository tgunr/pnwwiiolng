#pragma once
// Created: Tue Oct  7 21:36:26 CDT 2008
//
// Class for handling requests for instance lists
//
// Copyright (C) PlayNet, Inc 1999-2008 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

#include "inst/instClientStatus.h"
#include "inst/instTypes.h"

// Used by the host.
class wwiiPlayer;

namespace Instance
{
	// The structure used to send the request ID and the descriptor
	// to the host for a listing.
	class ListingRequest
	{
	public:
		ListingRequest(UINT32 _requestID, const Descriptor& _descriptor) : m_requestID(_requestID), m_descriptor(_descriptor) {}
		ListingRequest(ACTION_OBJECT* a);
		~ListingRequest() {}

		int packer(unsigned int mode, ACTION_OBJECT* a);

		UINT32 requestID() const { return m_requestID; }
		const Descriptor& descriptor() const { return m_descriptor; }

		UINT32		m_requestID;
		Descriptor	m_descriptor;
	};

	// The structure used by Create/Select to describe the results
	class Result
	{
	public:
        Result() : m_status(_BAD_DATA), m_descriptor(Descriptor()) {}
        Result(const Descriptor& _descriptor) : m_status(_BAD_DATA), m_descriptor(_descriptor) {}
        Result(ACTION_OBJECT* a) ;
        
        int packer(unsigned int mode, ACTION_OBJECT* a);
        
        ResultCodes status() const { return m_status; }
        const Descriptor& descriptor() const { return m_descriptor; }
        
        ResultCodes		m_status;
        Descriptor		m_descriptor;
	};

	// This is the stub class for handling requests for instance listings.
	// This much is shared between the client and the host. The client is
	// expected to derive from this class and implement the receive
	// function.
	class Listing
	{
	private:
		static UINT32 s_listingID;

	public:

		// expose this so we can use the begin and end calls without warnings
		using Results = std::vector<Instance::ClientStatus>;

		////// Ctor/Dtor
		Listing() : m_results(), m_requestID(0), m_result(_BAD_DATA), m_complete(false) {}
		Listing(ACTION_OBJECT* a);
		virtual ~Listing() {}

		////// Shared implementation functions
		SINT32 packer(unsigned int mode, ACTION_OBJECT* a);

		////// Member access
		void setComplete(bool c) { m_complete = c; }
		bool isComplete() const { return m_complete; }
		static UINT32 getNextRequestID() { return ++s_listingID; }
		static UINT32 getCurrentRequestID() { return s_listingID; }
		UINT32 result() const { return m_result; }
		void setResult(const ResultCodes code_) { m_result = code_; }

		////// Implementation concealment helpers.
		// You don't get direct access to Results(), use these instead.
		bool empty() const { return (m_complete) ? m_results.empty() : true; }
		size_t size() const { return (m_complete && !m_results.empty()) ? m_results.size() : 0; }
		const Instance::ClientStatus& operator[](unsigned int element)
		{
			if ( this->empty() ) throw std::logic_error("Invalid Instance::Listing::Results array access");
			if ( this->size() < element) throw std::domain_error("Out-of-bounds array access");
			return m_results[element];
		}
		Results::const_iterator begin() const { return m_results.begin(); }
		Results::const_iterator end() const { return m_results.end(); }

		// Receive function, host side.
		bool populate(wwiiPlayer* player, const ListingRequest& request);

		// Receive function, client side; the deriving class should
		// start by calling this and proceeding only if it returns true.
		virtual bool receive() const { return !expired(); }

		// Returns true if the requestID of this listing indicates it
		// was not the most recent request.
		bool expired() const
		{
			if ( m_requestID > getCurrentRequestID() ) throw std::domain_error("Instance::Listing requestID out of scope");
			return ( m_requestID == getCurrentRequestID() );
		}

	protected:
		Results			m_results;

		// Each client assigns each request a unique ID
		// so that they can be matched up on the client side.
		UINT32			m_requestID;
		ResultCodes		m_result;			// See inst/instClient.h
		bool			m_complete;
	};

}
