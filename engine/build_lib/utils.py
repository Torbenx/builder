import os
import subprocess


arch_x86_32 = 1
arch_x86_64 = 2
arch_armv7a = 3
arch_arm64v8a = 4

def arch_string(arch):
    if arch==arch_x86_32:
        return "x86_32"
    if arch==arch_x86_64:
        return "x86_64"
    if arch==arch_armv7a:
        return "armv7a"
    if arch==arch_arm64v8a:
        return "arm64v8a"
    raise ValueError("invalid arch")
    
    

def exec_cmd(cmd, shell, concat, cwd=None):
    if shell and concat:
        cmd = [" ".join(cmd)]
    return subprocess.call(cmd, shell=shell, cwd=cwd)



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
        return msvcrt.getch()
        
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
