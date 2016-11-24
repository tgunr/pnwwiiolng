Cluster-in-a-Box setup:

These are the instructions for a generic, non-developer, CiaB install.
If you are looking for the Linux-specific/Developer instructions, please
see README-Dev.txt.

======================================================================
== Summary:
======================================================================

1. Install MySQL Server,
2. Create playnet directory,
3. Get binaries,
4. Get ww2mods data,
5. Populate the database,
6. Launch controller,
7. Launch the game,

======================================================================
== Detailed steps:
======================================================================

------------------------------------------------------------
1. Install MySQL Server
------------------------------------------------------------

FOR WINDOWS:
  See README-MySQL-Win.txt, installation is simple but I've written a
  detailed walk through of the various wizard pages.
FOR LINUX:
  sudo apt-get install mysql-server mysql-client

** MAKE SURE YOU MAKE A NOTE OF THE ROOT PASSWORD **


------------------------------------------------------------
2. Create playnet directory
------------------------------------------------------------

Under Windows the default location for playnet data and binaries is the
C:\Playnet folder. This will need to be created along with:

  C:\Playnet
    C:\Playnet\bin
    C:\Playnet\etc
    C:\Playnet\wwiiol
      c:\Playnet\wwiiol\bin
      c:\Playnet\wwiiol\bin\data
      c:\Playnet\wwiiol\lua
      c:\Playnet\wwiiol\web
        c:\Playnet\wwiiol\web\debug
    c:\Playnet\tmp

You can do this automatically by running the "make-win-playnet.bat" script.


------------------------------------------------------------
3. Get server binaries
------------------------------------------------------------

Retrieve a copy of the current binaries, including libmysql.dll, and place
them into C:\Playnet\wwiiol\bin.

Make sure that they are in the `C:\Playnet\wwiiol\bin` directory and not
in a sub-directory.


------------------------------------------------------------
4. Get ww2mods data
------------------------------------------------------------

Use git or ask for a zip of the current data, this should be installed so
that the `WW2Mods\DATA` directory is at C:\Playnet\wwiiol\data, verify this
by making sure that `blitzWor.csv` is in C:\Playnet\wwiiol\data.


------------------------------------------------------------
5. Populate the database
------------------------------------------------------------

. You'll need Python installed, https://www.python.org/downloads/windows/
. Run the bootstrap.py script,


------------------------------------------------------------
6. Launch controller
------------------------------------------------------------

. Open a DOS prompt,
. Go to the binaries directory:
    > c:           (if the prompt doesn't have C: in it already)
    > cd \playnet\wwiiol\bin
    > controller
. To terminate: press ctrl-c or close the window.
. Wait for the message saying
  ++ Accepting logins from Auth server: 86c8435 (en) ++
  in the "map" window,
. Log files are generated in C:\Playnet\tmp

The first time you launch, you may need to "Accept" holes in your
Windows firewall. After accepting all the dialogs, restart the
controller.


------------------------------------------------------------
7. Launch the game
------------------------------------------------------------

[[ This needs refinement and probably generation of a default client .bat file ]]

When the bootstrap.py finished, it generated a set of ".bat" and ".tkt"
files. These are used to launch the client as various game names in the
CiaB database:
  rat0001p.bat - rat0005p.bat
    Launches the game as a RAT account,
  plyr0001p.bat - plyr0005p.bat
    Launches the game as a non-rat account,

You will need to copy these files into the location with your "WW2.exe"
in it and then run the relevant .bat file.
