import argparse
import shutil
import sys

def cptree(argv):
    parser = argparse.ArgumentParser(prog = 'shutil.copytree')
    parser.add_argument('-r', '--recursive', action = 'store_true')
    parser.add_argument('--src', type = str)
    parser.add_argument('--dst', type = str)
    args = parser.parse_args(argv)

    if args.recursive:
        shutil.copytree(args.src, args.dst)
    else:
        shutil.copyfile(args.src, args.dst)

def rmtree(argv):
    parser = argparse.ArgumentParser(prog = 'shutil.rmtree')
    parser.add_argument('--paths', nargs = '*', type = str)
    args = parser.parse_args(argv)

    for path in args.paths:
        shutil.rmtree(path)

if __name__ == '__main__':
    """Command line utils

    The CI pipeline is executed in bash on linux/mac and in PowerShell on
    Windows. This mini-script handles a couple of commands that differ in the
    two shells.
    """
    parser = argparse.ArgumentParser(prog = 'shutil')
    parser.add_argument('cmd', choices = ['copy', 'remove'])
    args = parser.parse_args(sys.argv[1:2])

    if args.cmd == 'copy':   cptree(sys.argv[2:])
    if args.cmd == 'remove': rmtree(sys.argv[2:])
