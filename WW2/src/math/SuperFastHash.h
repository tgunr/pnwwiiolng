#pragma once
// From http://www.azillionmonkeys.com/qed/hash.html
// Author of main code is Paul Hsieh
// I just added some convenience functions
// Also note http://burtleburtle.net/bob/hash/doobs.html, which shows that this is 20% faster than the one on that page but has more collisions

extern unsigned int SuperFastHash (const char * data, int length);
extern unsigned int SuperFastHashIncremental (const char * data, int len, unsigned int lastHash );
extern unsigned int SuperFastHashFile (const char * filename);
