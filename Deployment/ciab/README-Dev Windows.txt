Cluster-in-a-Box Setup:

These are the instructions for a Developer CiaB install.
If you are looking for the generic Non-Developer instructions, please
see ~README-First.txt.

=== Windows Quick Start ===

- Obtain a copy of the below folder (git, download, etc) from Development:
-- WW2Mods/DATA
- Run the "Dev Windows Setup.bat" (batch file)
-- Follow the directions

For details of setting up manually, read on...


======================================================================
== Summary:
======================================================================

1. Install MySQL Server
2. Create the Playnet Directory Structure
3. Build the Server Binaries
4. Get ww2mods Data
5. Get the Lua Scripts
6. Populate the Databases
7. Launch the Controller
8. Launch the Game

======================================================================
== Detailed steps:
======================================================================

------------------------------------------------------------
1. Install MySQL Server
------------------------------------------------------------

FOR WINDOWS:
  See README-MySQL-Win.txt, installation is simple but KFS1 has written a
  detailed walk through of the various wizard pages.
FOR LINUX:
  sudo apt-get install mysql-server mysql-client

** MAKE SURE YOU MAKE A NOTE OF THE ROOT PASSWORD **


------------------------------------------------------------
2. Create Playnet Directory Structure
------------------------------------------------------------

For Windows, the default location for the Playnet Data and Binaries is the
C:\playnet folder. This will need to be created along with:

  C:\playnet
    C:\playnet\bin
    C:\playnet\etc
    C:\playnet\lua
    C:\playnet\scripts
    C:\playnet\tmp
    C:\playnet\wwiiol
      C:\playnet\wwiiol\bin
      C:\playnet\wwiiol\data
      C:\playnet\wwiiol\lua
      C:\playnet\wwiiol\uweb
        C:\playnet\wwiiol\uweb\debug

You can do this automatically by running the "Scripts\Playnet Directory Structure.bat" script.


------------------------------------------------------------
3. Get the Server Binaries
------------------------------------------------------------

Using a Command Prompt
    Navigate to wwiiolng\build
    Run:
        cmake -DPN_DISABLE_AUTH=ON -DPN_BUILD_32BIT=OFF -DCMAKE_INSTALL_PREFIX=/playnet ../WW2/src
Open wwiiolng\build\WW2GameHosts.sln in VS
    Build

Copy "Installers\libmysql.dll" into C:\playnet\wwiiol\bin.

Make sure that the Server Binaries are in the `C:\playnet\wwiiol\bin` directory and not
in a sub-directory.


------------------------------------------------------------
4. Get the ww2mods Data
------------------------------------------------------------

Retrieve a copy of the current data, this should be installed so
that the `WW2Mods\DATA` directory is at C:\playnet\wwiiol\data, verify this
by making sure that `blitzWor.csv` is in C:\playnet\wwiiol\data.


------------------------------------------------------------
5. Populate the Databases
------------------------------------------------------------

Import the "Database\mysqldb.sql" file into your local MySQL instance or
You'll need Python installed, https://www.python.org/downloads/windows/
then run the bootstrap.py script


------------------------------------------------------------
6. Launch the Controller
------------------------------------------------------------

The bootstrap.py script creates a C:\playnet\scripts\~Start-Cluster.bat script
If you wish to start the cluster manually:

- Open a CMD prompt
- Go to the Server Binaries Directory:
    > c:           (if the prompt doesn't have C: in it already)
    > cd \playnet\wwiiol\bin
    > controller
- To terminate: press ctrl-c or close the window.
- Wait for the message saying
  ++ Accepting logins from Auth server: 86c8435 (en) ++
  in the "Map" window
- Log files are generated into the C:\playnet\tmp directory

The first time you launch the Controller, you may need to "Accept" holes in your
Windows firewall. After accepting all the dialogs, restart the
Controller.

ADDITIONAL:

. You can control which server processes the manager runs by editing
  the MANAGE_PROCESSES variable in /playnet/etc/environment.cfg
. You can tell the controller to only run each process once by
  specifying `controller.exe --single-shot`


------------------------------------------------------------
7. Launch the game
------------------------------------------------------------

The bootstrap.py script generates '.bat' and '.tkt' files for launching the client
with different accounts; see the C:\playnet\scripts folder or the "CiaB Accounts.xlsx" file.
