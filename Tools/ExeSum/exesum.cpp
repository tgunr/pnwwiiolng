#include "syscfg.h"

#include <iostream>
#include <fstream>
#include <cstdint>
#include <exception>
#include <vector>
#include "include/io_mapped_file.h"

using check_t = uint64_t;
static const size_t CHECKSUM_BIT_LEN = sizeof(check_t) * 8;

int main(int argc, const char* const argv[])
{
	static const char* const exeName = "ww2.exe";

	IO::MappedFile exe;
	if (!exe.mapFile(exeName)) {
		MessageBox(nullptr, "Unable to open ww2.exe", "FAILED", MB_ICONERROR | MB_OK);
		exit(1);
	}

	std::ofstream output("exesum.txt", std::ios::out, std::ios::trunc);
	if (!output.good()) {
		MessageBox(nullptr, "Unable to open exesum.txt!", "FAILED", MB_ICONERROR|MB_OK);
		exit(2);
	}

	check_t checksum(0);
	for (const char *ptr = exe.startPtr(), *end = exe.endPtr(); ptr < end; ++ptr) {
		// Rotate 5 bits right and add the new value, this gives some weight to positional value
		// of each value.
		checksum = ((checksum & 0x1f) << (CHECKSUM_BIT_LEN - 5)) | (checksum >> 5);
		checksum += *ptr;
	}

	output << exeName << ' ' << checksum << std::endl;
	output.close();

	std::cout << exeName << ' ' << checksum << std::endl;
}
