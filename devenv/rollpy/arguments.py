import argparse
import platform
import rollpy.command
import subprocess

class CommandLineError(Exception):
    """ Raised to notify the user of problems with command line options. """

    def __init__(self, error):
        self.error = error

    def __str__(self):
        return "\nERROR: " + self.error


def get_cmdline_arguments():
    """
    Returns an argparse object with the command line options.

    This includes performing {box} and {prov} substitution on boxUri.

    Example:

        from __future__ import print_function
        import rollpy.arguments
        argv = rollpy.arguments.get_cmdline_arguments()
        print("Box: {} ({}) with {} cpus and {}Mb of memory.".format(
            argv.boxName, argv.boxUri, argv.cpus, argv.memMb))
    """

    ap = argparse.ArgumentParser(
        add_help=True, description=(
            "Installs a WWIIOL virtual dev environment using 'Vagrant' to roll "
            "a virtual machine.\n"
            "\n"
            "This will include downloading a vagrant 'box' for the required "
            "Linux distro if required, configuring and provisioning the virtual "
            "machine."))

    ap.add_argument(
        "--debug", "-w",
        action='count', default=0,
        help="Enable debug output")
    ap.add_argument(
        "--quiet", "-q",
        action='store_true', default=False,
        help="Disable 'INFO:' output")
    ap.add_argument(
        "--provider",
        type=str, default="virtualbox", dest="provider",
        help="Specify which Vagrant provider (virtualbox, vmware, etc) to use.")
    ap.add_argument(
        "--box-name",
        type=str, default="vivid", dest="boxName",
        help="Specify the friendly name for the distro box.")
    ap.add_argument(
        "--box-uri",
        type=str, dest="boxUri",
        default="https://cloud-images.ubuntu.com"
                 "/{prov}/{box}/current/"
                  "{box}-server-cloudimg-amd64-{prov}-disk1.box",
        help="Specify the URI for the Vagrant box (distro image). "
             "{box} will be replaced with the box name, "
             "{prov} will be replaced with the provider name.")
    ap.add_argument(
        "--cpus",
        type=int, default=4,
        help="Specify the number of virtual CPUs to provide the devenv")
    ap.add_argument(
        "--mem-mb",
        type=int, default=1024, dest="memMb",
        help="Specify the size the virtual machine's memory in Mb")
    ap.add_argument(
        "--vagrant",
        type=str, default="vagrant",
        help="Specify the 'vagrant' executable to use (default: vagrant)")

    argv = ap.parse_args()

    argv.boxUri = argv.boxUri.format(box=argv.boxName, prov=argv.provider)

    try:
        rollpy.command.run([argv.vagrant, "--version"])
    except (OSError, subprocess.CalledProcessError) as e:
        raise CommandLineError(
            "Could not find command '{}' ({}), is Vagrant installed?\n"
            "Consider running: "
                "sudo apt-get install -qy vagrant virtualbox virtualbox-dkms"
            .format(argv.vagrant, str(e)))

    return argv
