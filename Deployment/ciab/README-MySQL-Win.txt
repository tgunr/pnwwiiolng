[[ This section needs work, I already have MySQL installed, I'll update it
   when I rebuild my machine next week and have a clean install to work on. ]]

MySQL is an open-source, free data storage / query system (database). The
game servers require it to be installed in order to run.

It is strongly recommended that you go with the default install options as
much as possible, except where specified below.

Installation usually takes a couple of minutes.

	URL: https://dev.mysql.com/downloads/mysql/

 . Under "Generally Available (GA) Releases", "Select Platform" should be
   Microsoft Windows,
 . Click the "Download" button immediately below the "Recommended Download",
   (currently: Windows (x86, 32-bit) MySQL Installer MSI)
 . Scroll down (the pages are confusing) and choose the MSI Installer,
   (there are currently two options, choose the first)
 . Ignore the two Login/Signup buttons click "No thanks, just start my download"
   at the bottom of the page (this always catches *me* out --kfs1)

Launch the installer,

. Check "MySQL Server" and "MySQL Utilities",
. Connectors, documentation, samples and examples are optional and can be
  unchecked,
. Configuration:
 - Config Type is Development Machine,
 - TCP/IP port 3306 (default),
 - Uncheck "open firewall port for network access'"
 - Uncheck "named pipe",
 - Uncheck "shared memory",
 - Root password: your choice, recommend "ciab4ever"
 - Check configure MySQL server as a windows service,
 - Leave name as-is,
 - Check/uncheck "Start the MySQL server at system startup" as suits you
 - Standard System Account is fine unless you want to create a local windows
   account specifically for MySQL,
 - Plugins/Extensions: leave unchecked,

If you chose *not* to have MySQL server auto-start, you will need to manually
start it. To do this:

 . Press <Windows>+<R> or Start and type "services.msc",
 . Find the "MySQL" service,
 . Right click,
 . Select "Start".

You'll need to do this any time you want to use CiaB and have rebooted.
