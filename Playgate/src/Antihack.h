#ifndef ANTIHACK_H
#define ANTIHACK_H 1
#ifdef ANTIHACK
#include <string>

class Antihack
{
public:
	Antihack(void);
	std::wstring check();
	std::wstring hide(std::wstring text, UINT32 seed);
};

#endif
#endif
