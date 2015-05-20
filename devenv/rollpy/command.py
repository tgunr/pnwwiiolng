import subprocess

def run(cmd):
    """
    Runs a command and returns the output.

    Example:

        import rollpy.command
        s = rollpy.command.run("echo Banana")
        print(s)
    """
    return subprocess.check_output(cmd)
