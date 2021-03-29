import os
import subprocess
import threading
import traceback
import sys

def exec_cmd(cmd, shell, concat, cwd=None, stdout=None, stderr=None):
    if stdout and not stderr:
        stderr = subprocess.STDOUT
    if shell and concat:
        cmd = [" ".join(cmd)]
    return subprocess.call(cmd, shell=shell, cwd=cwd, stdout=stdout, stderr=stderr)

def exec_pipe(cmd, shell, concat, cwd=None):
    if shell and concat:
        cmd = [" ".join(cmd)]
    return subprocess.Popen(cmd, shell=shell, cwd=cwd, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)

def join(*paths):
    ret = ""
    for path in paths:
        if path == "": continue
        if ret != "": ret += "/"
        ret += path
    return ret

try:
    import msvcrt

    def is_windows():
        return True

    def getch():
        key = msvcrt.getch()
        return key

    def kbhit():
        return msvcrt.kbhit()

    def clear():
        os.system("cls")

except ImportError:

    def is_windows():
        return False

    def getch():
        return raw_input()

    def kbhit():
        return False

    def clear():
        os.system("clear")
