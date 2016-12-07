MySQL is an open-source, free data storage / query system (database). The game servers require it to be installed in order to run.

Installation usually takes a couple of minutes. See "Obtaining MySQL" at the end of the document for instructions on how to obtain an installer if you don't already have one.

== Responding to the MySQL Installer

. Accept the license terms, click Next >
. Choose an install type:
-- You can accept "Developer Default" and install a lot of baggage, or:
-- Choose "Server Only",
. Click Next >,
. Click "Execute" at Check Requirements if it is not greyed out,
. Click Next >,
. Click Execute,
. Click Next >,
. Click Next > to enter Product Configuration,
. Click Next > to select the defaults for Type and Networking,
. Choose a root password: feel free to use ciab4ever
. Click Next >, don't add any users,
. Check/uncheck "Start the MySQL Server at Startup Time" as suits you, but you may have to
  manually start MySQL through "services.msc" after reboots if this is unchecked.
. Click Next >,
. Click Next >,
. Click Execute,
. Click Finish,
. Click Next >,

== Obtaining MySQL 

URL: https://dev.mysql.com/downloads/mysql/

 . Under "Generally Available (GA) Releases", "Select Platform" should be
   Microsoft Windows,
 . Click the "Download" button immediately below the "Recommended Download",
   (currently: Windows (x86, 32-bit) MySQL Installer MSI)
 . Scroll down (the pages are confusing) and choose the MSI Installer,
   (there are currently two options, choose the first)
 . Ignore the two Login/Signup buttons click "No thanks, just start my download"
   at the bottom of the page (this always catches *me* out --kfs1)
