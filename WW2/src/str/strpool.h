#pragma once
//	This module is used to pool strings. It is useful for situations in which there
//	are a large number of identical strings and lot of memory can be saved
//	by using pointers instead of fixed sized char buffers.

//	strpool will add a copy of the passed in string to the string pool if it does not
//	already exist in the pool and will return a pointer to the pool string. *Do not* modify
//	the returned string.

//	Currently this module only conists of this one function, if additional functionality
//	(like removing strings from the pool, multiple pools, ref counting, etc) is required
//	let me know. - rickb

extern const char* strpool(const char*);

extern void strpoolKill();
