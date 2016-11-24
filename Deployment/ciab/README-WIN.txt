Windows Cluster-in-a-Box setup:

1. Install MySQL Server,
2. Create playnet directory,
3. Get binaries,
4. Get ww2mods data,
5. Populate the database,
6. Launch controller

======================================================================
== Detailed steps:
======================================================================

------------------------------------------------------------
1. Install MySQL Server
------------------------------------------------------------

[[ This section needs work, I already have MySQL installed, I'll update it
   when I rebuild my machine next week and have a clean install to work on. ]]

https://dev.mysql.com/downloads/mysql/

. Under "Generally Available (GA) Releases",
. "Select Platform" should be Microsoft Windows,
. Click the "Download" button immediately below the "Recommended Download",
  (currently: Windows (x86, 32-bit) MySQL Installer MSI)
. Scroll down (the pages are confusing) and choose the MSI Installer,
  (there are currently two options, choose the first)
. Ignore the two Login/Signup buttons, click "No thanks, just start my download"
. Launch the installer,
. Ensure "MySQL Server",
. Ensure "MySQL Utilities" or "MySQL Fabric & MySQL Utilities" is checked,
. Connectors, documentation, samples and examples are optional and can be
  unchecked,
. Configuration:
 . Config Type is Development Machine,
 . TCP/IP port 3306,
 . Uncheck "open firewall port for network access'"
 . Uncheck "named pipe",
 . Uncheck "shared memory",
 . Root password: your choice, recommend "ciab4ever"
 . Check configure MySQL server as a windows service,
 . Leave name as-is,
 . Check/uncheck "Start the MySQL server at system startup" as suits you:
   - I leave it unchecked and manually start MySQL when I'm testing,
     You do this by clicking Start and typing "services",
 . Standard System Account is fine unless you want to create a local windows
   account specifically for MySQL,
 . Plugins/Extensions: leave unchecked,

** MAKE SURE YOU MAKE A NOTE OF THE ROOT PASSWORD **


------------------------------------------------------------
2. Create playnet directory
------------------------------------------------------------

Under Windows the default location for playnet data and binaries is the
C:\Playnet folder. This will need to be created along with:

  C:\playnet
    C:\playnet\bin
    C:\playnet\etc
    C:\playnet\wwiiol
      c:\playnet\wwiiol\bin
      c:\playnet\wwiiol\bin\data
      c:\playnet\wwiiol\lua
      c:\playnet\wwiiol\web
        c:\playnet\wwiiol\web\debug
    c:\playnet\tmp

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
. Run the populate-db.py script,

* MAKE A NOTE OF THE TICKET FILE.


------------------------------------------------------------
6. Launch controller
------------------------------------------------------------

. Open a DOS prompt,
. Go to the binaries directory:
    > c:
    > cd \playnet\wwiiol\bin
    > controller
. To terminate: press ctrl-c or close the window.


------------------------------------------------------------
7. Launch the game
------------------------------------------------------------

[[ This needs refinement and probably generation of a default client .bat file ]]

. Copy the TICKET FILE to the directory containing your WW2.exe client,
. Run the client with the following command line:
   ww2.exe -ticket FILE -window 1 -width 800 -height 640 -datadir DATA -data 1
  - Replace 'FILE' with your ticket filename, e.g. kfs1.tkt,
  - Replace 'DATA' with the location of your client DATA directory,
  - If you are using archives, you can remove '-datadir' and '-data' options,
  - You can use '-log 0' to disable logging and speed up the client,
  - You can use '-nowor' and '-noinf' to improve start up times at the cost
    of spawn times.
