#! /bin/env python

from __future__ import print_function

import rollpy.arguments
import shutil

def debug(argv, *args, **kwargs):
    if argv.debug:
        print("#", *args, **kwargs)

def note(argv, *args, **kwargs):
    if not argv.quiet:
        print("INFO:", *args, **kwargs)


def main():
    argv = rollpy.arguments.get_cmdline_arguments()
    debug(argv, "boxName:", argv.boxName)
    debug(argv, "boxUri:", argv.boxUri)
    debug(argv, "boxCpus:", argv.cpus)
    debug(argv, "boxMem:", argv.memMb)
    debug(argv, "provider:", argv.provider)
    debug(argv, "vagrant:", argv.vagrant)

    note(argv, "Generating Vagrantfile")
    with open("Vagrantfile", "w") as fh:
        print(fh, """
### AUTOMATICALLY GENERATED VAGRANTFILE

Vagrant.configure(VAGRANTFILE_API_VERSION) do |config|

  # From {BOXURI}
  config.vm.box = '{BOXNAME}'

  config.vm.provision :shell, path: 'vagrant_provision.sh'

  config.vm.synced_folder '{SRCDIR}', '/playnet/wwiiolng'

  config.ssh.forward_agent = true

  config.vm.provider 'virtualbox' do |vb|

  #  # Don't boot with headless mode
  #  vb.gui = true

    vb.customize [ 'modifyvm', :id, '--memory', '{BOXMEMMB}' ]

    vb.customize [ 'modifyvm', :id, '--ioapic', 'on' ]
    vb.customize [ 'modifyvm', :id, '--cpus', {BOXCPUS} ]

  end

end""".format(
        BOXURI=argv.boxUri,
        BOXNAME=argv.boxName,
        SRCDIR=srcDir,
        BOXMEMMB=argv.memMb,
        BOXCPUS=argv.cpus))

if __name__ == "__main__":
    main()
