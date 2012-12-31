#include <syscfg.h>	// path is platform specific.

#include "wwii/wwiiClusterConfig.h"

/*
 * The "Boss" process is the master for launching all other
 * game processes that are supposed to run on any given server,
 * if multiple machines are involved in a given cluster, then
 * one of the bosses has to be designated as master, and that
 * boss is used to coordinate services between machines.
 */

int main(int argc, const char* argv[])
{
	Cluster::Config::Load();

//	auto noopCallbackFn = [](SINT32 /* teulIndex */, const UCHAR* const /*data*/, SINT32 /*size*/, const TEUL_CALLBACK* const /*object*/, SINT32 /*error*/) {};
//	teulInitTransport(LOG_TO_FILE, "clusterboss", noopCallbackFn, noopCallbackFn);
}
