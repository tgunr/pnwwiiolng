#pragma once
/// \file
/// \brief \b [Internal] Random number generator
///
/// This file is part of RakNet Copyright 2003 Jenkins Software LLC
///
/// Usage of RakNet is subject to the appropriate license agreement.

/// Initialise seed for Random Generator
/// \note not threadSafe, use an instance of RakNetRandom if necessary per thread
/// \param[in] seed The seed value for the random number generator.
extern void seedMT( unsigned int seed );

/// \internal
/// \note not threadSafe, use an instance of RakNetRandom if necessary per thread
extern unsigned int reloadMT();

/// Gets a random unsigned int
/// \note not threadSafe, use an instance of RakNetRandom if necessary per thread
/// \return an integer random value.
extern unsigned int randomMT();

/// Gets a random float
/// \note not threadSafe, use an instance of RakNetRandom if necessary per thread
/// \return 0 to 1.0f, inclusive
extern float frandomMT();

/// Randomizes a buffer
/// \note not threadSafe, use an instance of RakNetRandom if necessary per thread
extern void fillBufferMT( void *buffer, unsigned int bytes );
