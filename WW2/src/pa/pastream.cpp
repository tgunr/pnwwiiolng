#include <syscfg.h> // Path is project specific.
#include "pastream.h"

ps_streambuf::ps_streambuf(const char* filename): _buf(0), _len(0)
{
	if(paFetchMemberByName(paArchive(), filename, &_buf, &_len) == 0 && _buf)
	{
		setg((char*)_buf, (char*)_buf, (char*)_buf+_len);
	}
}

ps_streambuf::~ps_streambuf()
{
	if(_buf)
		paFreeMember(_buf);
}

ps_streambuf::pos_type ps_streambuf::seekoff(off_type off, std::ios_base::seekdir dir, std::ios_base::openmode)
{
	off_type curpos = gptr() - eback();
	off_type pos = 0;

	if(dir == std::ios_base::beg)
	{
		pos = off;
	}else if(dir == std::ios_base::cur)
	{
		pos = curpos + off;
	}else if(dir == std::ios_base::end)
	{
		pos = _len-off;
	}

	if(pos > _len || pos < 0)  return pos_type(-1);

	gbump(pos-curpos);

	return pos_type(pos);
}

ps_streambuf::pos_type ps_streambuf::seekpos(pos_type pos, std::ios_base::openmode)
{
	off_type curpos = gptr() - eback();

	if(pos > _len || pos < 0)  return pos_type(-1);

	gbump(pos-curpos);

	return pos_type(pos);
}

pa_istream::pa_istream(const char* filename, int) : std::istream(0)
, _streambuf(filename)
{
	init(&_streambuf);
}

pa_istream::~pa_istream()
{
}
