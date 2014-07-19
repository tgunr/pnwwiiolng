Linux Vagrant Setup
-------------------

Vagrant provides a means of efficiently deploying complete, custom, consistent development environments.

By combining a standard baseline image ("box"), a configuration file ("Vagrantfile") and a setup
script ("VAGRANT_PROVISION.sh"), you can quickly go from zero to a fully functional virtual dev box
within two minutes.

By modifying the Vagrantfile you can use the same procedure to create test or even production servers,
and know that they will all be consistent.

Because there are Vagrant box images for Linux, MacOS and Windows, it makes it possible for a single
developer to work on all three OSes without having to reboot.

MOST IMPORTANTLY: Vagrant uses the virtual-machine's support for shared directories so that the source
repository you are working out of is shared with the virtual machine. This allows the developer to
do work with the files on their workstation but build in the virtual machine.

This saves you the overhead of dedicated build machines while avoiding the headaches of having developers
doing builds on volatile workstation environments.



GETTING STARTED:

Run the "BOOTSTRAP.sh" (Linux/Mac) or "BOOTSTRAP.bat" script.



FIRST BUILD:

Issue the command

  host$ vagrant ssh

to log into the guest machine. You may need to issue 'vagrant up' if you have rebooted.

On the guest machine

  # The source repository you installed from is visible as /playnet/wwiiolng
  vagrant$ cd /playnet/wwiiolng

  # Ready the build folder (/playnet/wwiiolng/build64)
  vagrant$ ./linux_start_here.sh

  # Compile
  vagrant$ cd build64
  vagrant$ make -j9

I recommend that you create a symlink on the host machine:

  ln -s /playnet/wwiiolng `pwd -P`/../..



COMMANDS:

   vagrant up
     Start the virtual machine

   vagrant ssh
     SSH into the virtual machine

   vagrant down
     Stop the virtual machine

   vagrant destroy
     Delete the virtual machine

