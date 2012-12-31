#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


int main(int argc, char** argv)
{
	int w = open(argv[1], O_RDONLY);
	int e = open(argv[2], O_RDONLY);
	int outfile = open(argv[3], O_RDWR | O_CREAT | O_TRUNC);

	unsigned char buf[21600*3];
	unsigned char* h;
	for(int y = 0; y < 21600; y++)
	{
		printf("%d\n",y);

		h = buf;
		read(w, buf, sizeof(buf));
			
		write(outfile, buf, sizeof(buf));
	
		h = buf;
		read(e, buf, sizeof(buf));
		write(outfile, buf, sizeof(buf));
	}

/*	short buf[21600];
	short* h;
	for(int y = 0; y < 21600; y++)
	{
		printf("%d\n",y);

		h = buf;
		read(w, buf, sizeof(buf));
		for(int x = 0; x < 21600; x++, h++)
			*h -= 32768;
			
		write(outfile, buf, sizeof(buf));
	
		h = buf;
		read(e, buf, sizeof(buf));
		for(int x = 0; x < 21600; x++, h++)
			*h -= 32768;
		write(outfile, buf, sizeof(buf));
	}
*/
	return 0;
}