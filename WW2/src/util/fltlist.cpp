#include <syscfg.h> // Path is project specific.
#include <set>
#include <string>

#include "sdm/sdm.h"

int readfn(int fd, void* buf, int n)
{
	return fread(buf, 1, n,(FILE*)fd);
}

int seekfn(int fd, int offset)
{
  return fseek((FILE*)fd,offset,SEEK_CUR);
}

int main(int argc, char** argv)
{
	std::set<std::string> flts;

	for(int i = 1; i < argc; i++)
	{
		sdm_replacement_octet_data replace;

		FILE* f = fopen(argv[i], "rb");
		if(!f) continue;

		sdm_seek_replacement_data((int)f, readfn, seekfn);
		sdm_read_replacement_data((int)f, readfn, &replace);

		for(int j = 0; j < replace.numobjinfo; j++)
		{
			sdm_trnobj_info& info = replace.objinfo[j];

			char* s = info.objname;
			for(;*s;s++)
			{
				if(*s == '\\') *s= '/';
				*s = tolower(*s);
			}
			flts.insert(info.objname);
		}

		fclose(f);
	}

	std::set<std::string>::iterator it = flts.begin();
	for(; it != flts.end(); ++it)
	{
		printf("%s\n", it->c_str());
	}

	return 0;
}
