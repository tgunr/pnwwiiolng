ABOUT
	This is the WWII Online host project; the original many trees
	layout has been flattened into two directories. This may be
	expanded again later (e.g. src/chat, etc) but probably not.

DEPENDENCIES
	Depends on the following libraries:

		lua-5.0.2
		tolua++-1.0.5-1

	These can be found in $/3rdparty/host-libs along with the
	scons package required to install tolua++.

	Note: Fedora Core 6+ ships with these libraries and so many
	not require them to be installed as 3rd party items anymore.

BUILDING
	The applications can be built for one of four targets:

		DEV			Pentium 3, no optimization, heavy debugging
		BETA		Pentium 3, no optimization, debugging info
		TRAINING	Pentium 3, full optimization, no debugging
		LIVE		Pentium 4 + sse2, full opt, zero debugging

	To configure for a target, run
		hostproj/set-build-configuration.sh <target>
	e.g.
		hostproj/set-build-configuration.sh BETA

	Compile with make. Output files are placed in hostproj

