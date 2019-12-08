import os
import subprocess
import threading
import traceback
import sys


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
       
        
class Logger(object):
    
    def __init__(self, parent = None, name = None):
        self.parent = parent
        self.name = name
        self.cmd = None
        self.ex = None
        self.trace = None
        self.result = 0
        self.children = []
        self.waiting = []
        self.lock = threading.Lock()
        self.active_children_count = 0
        self.buffer = ""
        
    def log(self, msg):
        self.buffer += msg+"\n"
        
    def exec_cmd(self, cmd, shell, concat, cwd=None):
        self.command(cmd)
        proc = exec_pipe(cmd, shell, concat, cwd=cwd)
        self.capture_output(proc)
        return proc.returncode
        
    def exec_cmd_final(self, cmd, shell, concat, cwd=None):
        ret = self.exec_cmd(cmd, shell, concat, cwd=cwd)
        if ret==0:
            self.success()
        else:
            self.failed()
        return ret
        
    def capture_output(self, proc):
        while True:
            chunk = proc.stdout.read(1)
            if not chunk:
                break
            self.buffer += chunk
        proc.wait()
        
    def command(self, cmd):
        self.cmd = cmd
        
    def failed(self, ex=None, trace=None):
        self.ex = ex
        self.trace = trace
        self.result = 1
        if self.parent: self.parent.done(self)
        
    def success(self):
        self.result = 2
        if self.parent: self.parent.done(self)
        
    def nothing_to_do(self):
        self.result = 3
        if self.parent: self.parent.done(self)
    
    def child(self, name):
        h = Logger(self, name)
        with self.lock:
            self.active_children_count += 1
            self.children.append(h)
        return h
        
    def done(self, child):
        with self.lock:
            self.active_children_count -= 1
            self.waiting.append(child)
            
    def get_active_children_count(self):
        with self.lock:
            return self.active_children_count


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
