Big Changes since the last CRS1.0 commits to 1.35:
==================================================

* Visual Studio Support

The hosts now build and run under Visual Studio 2015

* 64 Bit Support

The hosts now build and run as 32 or 64 bit processes, the 64-bit processes use more memory but are generally more performant

* Run-time static configuration "ConfigVars"

Previously, the hosts used environment variables and parameters to configure various things. I needed to add a lot more to support the Windows port, and I hated the fact that they weren't validated.

So I added a "ConfigVars" type to the server code which reads settings from a file. They can be overridden by environment variables to make it easier to migrate to this option but ultimately they should only be read from the file(s).

By default files are read from a generic "environment.cfg" but then a second file, "environment.${platform}.cfg" is consulted for overrides.

* Cluster-on-a-box

Previous versions required on the network BROADCAST_ADDR for much of the inter-process communications. This was both lossy and prevented two processes on the same machine from "seeing" each other.

I introduced a new process, "controller", for the purpose of managing processes running on the current machine but also providing a very light-weight repeater port: any packet you send to it is automatically forwarded to all other connected processes.

"server.cpp" was refactored to decide whether to open a broadcast socket or to connect that socket to the controller/repeater.

Upshot is that you can now run an entire cluster on a single box. It is limited to one instance of each host type.

* Numerous fixes

Upgrading compilers and porting revealed the causes of many subtle and strange/minor issues, all of which are fixed.

* Support for "Object Send"

Made it possible to register a receiver for a specific object, this removes a layer of boiler plate.

* Improvements to OMG

Translated OMG into a formal grammar using lexx/bison, and improved the code it generates by several factors. See Work In Progress. OMG is now, though, capable of assigning CallbackY to the messages it generates, making them easier to deploy.
- OMG classes now have a Callback() and CALLBACK* s_callback members.
- I added a REGISTER_OMGRECEIVER macro for eliminating a bunch of crap from receiving OMG messages.
- Moved omgChatMissionWhatever.omg to omgChat.omg and started creating new messages in it,


Lots more - I need to dig up original emails to Dana where I listed all the changes.



Work In Progress
================

Build Everything:
- The aim is to eliminate all static libraries from WW2\lib (3rdparty etc) to avoid build tool issues,
- For libraries we can't eliminate (directx) we may need to make it lib32 and lib64,

TCMalloc instead of SmartHeap:
- I haven't eliminated the libexpat library (xmlparse2) which causes linker errors when trying to link with TCMalloc in a Debug Build, so for now I have simply made it so that we don't use TCMalloc in debug builds.

Controller:
- The controller currently has only limited control over processes, this needs a small amount of TLC.

OMG:
I'm gradually iterating on this, but the idea is that instead of having dozens of "chatXyZMessage.h" files, you should define all your Chat messages in one .omg file.
- OMG should generate a function that REGISTER_...s all of the objects, this requires enabling specification of client->server, server->client, client<->server, etc, properties in the .omg file,
- OMG should generate more of the boilerplate code

Mac Build:
I haven't tried it, at all. It probably requires Clang 3.5 or something and it's probably going to be a headache for someone.
