import os
import subprocess
import threading
import traceback
import sys


arch_x86_32 = 1
arch_x86_64 = 2
arch_armv7a = 3
arch_arm64v8a = 4
    
def copy_assets(source_dir, target_dir, rebuild=False):
    any_copy = False
    if not os.path.exists(target_dir):
        os.makedirs(target_dir)
    if not os.path.exists(source_dir):
    	return False
    for item in os.listdir(source_dir):
        source_file = utils.join(source_dir, item)
        target_file = utils.join(target_dir, item)
        if os.path.isfile(source_file):
            if (not os.path.exists(target_file)) or os.path.getmtime(target_file) < os.path.getmtime(source_file):
                print "Copying "+source_file
                any_copy = True
                shutil.copyfile(source_file, target_file)
        else:
            if copy_assets(utils.join(source_dir, item), utils.join(target_dir, item), rebuild=rebuild):
                any_copy = True
    return any_copy

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
