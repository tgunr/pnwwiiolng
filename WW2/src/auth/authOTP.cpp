#include <syscfg.h> // Path is project specific.
// Created: Tue Jun 13 15:15:10 2000
//
// DESCRIPTION
//  This module generates the one time passwords (tickets) for use with the
//  Authentication server
//
// THEORY
//  The algorithm
//   n = (n + BigPrime) % SmallerPrime
//  produces a pseudo-randomized sequence of numbers 0 <= n < SmallerPrime
//  with no repeats within a sequence, but which is also the same each
//  iteration of the sequence.
//
//  For example, if LargePrime = 29 and SmallPrime = 11 the sequence is:
//   3 10 6 2 9 5 1 8 4 0 7 3 10 6 2 9 5 1 8 4 0 7 3 10 6 2 9 ...
//  Changing LargePrime to 23 produces
//   4 5 6 7 8 9 10 0 1 2 3 4 5 6 7 8 9 10 0 1 2 3 4 5 6 7 8 9 ...
//
//  We generate the sequences in batches so that the end-of-sequence
//  OTP can be saved to database periodically so that restarting the
//  server will guarantee the new instance does not collide with
//  recently assigned OTPs.
//
// Copyright (c) 2000 Playnet, Inc., All Rights Reserved.
// Not to be distributed in API form except as part of a
// compiled library.
//
// Originating Programmer: Shane "Elmo" Wickson

#include "auth/authDB.h"
#include "auth/authBypassOTP.h"

static const UINT64 OTP_LARGE_PRIME = 1023456987896543201ULL ;
static const UINT32 OTP_SMALL_PRIME = 4294967291U ;

static const UINT32 OTP_QUEUE_SIZE = 121 ;
static UINT32 _otpQueue[OTP_QUEUE_SIZE] ;
static UINT32 _otpQueueEnd = 0 ;

//////////////////////////////////////////////////////////////////////
//

static void
_fillOTPQueue()
{
	static UINT64 nextOTP = OTP_SMALL_PRIME ;

	// Get the end of the last sequence
	if ( nextOTP >= OTP_SMALL_PRIME )
		nextOTP = authdbGetLastOTP() ;

	// Generate a batch of OTPs, possibly backwards
	// The "current" OTP was the last generated OTP,
	// so we want to start with the next generated
	// entry from the sequence
	for ( _otpQueueEnd = 0 ; _otpQueueEnd < OTP_QUEUE_SIZE ; ++_otpQueueEnd )
	{
		nextOTP = (nextOTP + OTP_LARGE_PRIME) % OTP_SMALL_PRIME ;
		_otpQueue[_otpQueueEnd] = (UINT32)nextOTP ;
	}

	// Save the last-used sequence number
	authdbSaveLastOTP(nextOTP) ;
}

//////////////////////////////////////////////////////////////////////
// Get the next authentication sequence One Time Password

UINT32
authGetOTP()
{
	// If there are no queued OTPs, generate a new batch
	// batching is to eliminate the number of updates we
	// make to the database
	if ( _otpQueueEnd <= 0 )
	{
		_fillOTPQueue() ;
		ABORT_IF(_otpQueueEnd <= 0);
	}

	// Randomly select an OTP from the queue and then replace it with
	// the OTP from the end of the queue and shrink the end-of-queue by 1
	// (lazy swap, since we don't care about the used value anymore).
	const UINT32 selection = rand() % _otpQueueEnd ;
	const UINT32 otp = _otpQueue[selection] ;
	_otpQueue[selection] = _otpQueue[--_otpQueueEnd] ;

	// Never return the back door value
	return ( otp == AUTH_BYPASS_OTP ) ? authGetOTP() : otp ;
}

