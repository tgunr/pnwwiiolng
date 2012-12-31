TestTeul: TeulKit testing truss
===============================

TeulKit is PlayNet Inc's proprietary network/messaging API.

TestTeul provides a simple architecture for programmers to validate
teulKit functionality and/or simulate clients/servers for testing.

Note: Client and Server have to be run seperately.


Usage
===============================

Server Mode:
  testteul [options] server

Client Mode:
  testteul [options] client

Options:
  --host=aa.bb.cc.dd
	[Client only, Default: 127.0.0.1]
	Specifies the IP address to connect to.

  --test=<test name>
	Explicitly enables running of a test.
	Default: All tests are default enabled.


Implementation
===============================

See $(SolutionDir)/WW2/Src/tests/testTeul.cpp

The core of the implementation is the "Role" class which implements
a common framework for both client and server code, provides a
series of exception types, and various static functions required
as callbacks by teulKit.

The application instantiates either a ServerRole or ClientRole
depending on command line arguments, and then invokes the "Run()"
method of that instance.

See the code for more detail.


- Oliver

