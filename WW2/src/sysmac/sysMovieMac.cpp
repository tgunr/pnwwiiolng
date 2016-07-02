#include "sysMovie.h"

sysMovie::sysMovie() : _platimp(nullptr)
{
}

sysMovie::~sysMovie()
{
}

bool sysMovie::Load(const char* filename)
{
    return false;
}

void sysMovie::Start(int ms)
{
}

void sysMovie::Stop()
{
}

void sysMovie::Pause()
{
}

void sysMovie::Enable(bool enable)
{
}

void sysMovie::UpdateFrame(int ms)
{
}

void sysMovie::DrawFrame(float left, float bottom, float right, float top)
{
}

bool sysMovie::Playing()
{
	return false;
}
