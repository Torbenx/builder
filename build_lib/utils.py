import os
import subprocess
import threading
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
        self.read = None
        self.write = None
        self.cmd = None
        self.ex = None
        self.trace = None
        self.result = 0
        self.children = []
        self.waiting = []
        self.lock = threading.Lock()
        self.active_children_count = 0
        
    def stdout(self):
        self.read, self.write = os.pipe()
        return self.write
        
    def command(self, cmd):
        self.cmd = cmd
        
    def failed(self, ex=None, trace=None):
        self.ex = ex
        self.trace = trace
        self.result = 1
        if self.write: os.close(self.write)
        if self.parent: self.parent.done(self)
        
    def success(self):
        self.result = 2
        if self.write: os.close(self.write)
        if self.parent: self.parent.done(self)
        
    def nothing_to_do(self):
        self.result = 3
        if self.write: os.close(self.write)
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
            
    def get_active_child_count(self):
        with self.lock:
            return self.active_children_count
            
    def print_waiting(self):
        with self.lock:
            for h in self.waiting:
                if h.result==1:
                    print "Failed: "+h.name
                    if h.cmd: print "Cmd="+str(h.cmd)
                    if h.ex: print h.ex
                    if h.trace:
                        for line in traceback.format_tb(h.trace):
                            sys.stdout.write(line)
                elif h.result==2:
                    print "Successfull: "+h.name
                if h.read:
                    while True:
                        buf = os.read(h.read, 1024)
                        if not buf:
                            break
                        sys.stdout.write(buf)
                    os.close(h.read)
            self.waiting = []
            #for h in self.children:
            #    if h.result == 0: print(h.name)
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
