import sys
import threading
import time
sys.dont_write_bytecode = True

import os
import traceback
from build_lib import utils

cwd = os.getcwd()

def choose_target():
    print "a for android, w for windows"
    key = utils.getch()
    global platform
    global arch
    if key == "a":
        platform = "android"
        print "1 for armv7a, 2 for arm64v8a, 3 for x86_32, 4 for x86_64"
        key = utils.getch()
        if key == "1":
            arch = utils.arch_armv7a
        elif key == "2":
            arch = utils.arch_arm64v8a
        elif key == "3":
            arch = utils.arch_x86_32
        elif key == "4":
            arch = utils.arch_x86_64
        else:
            print "invalid arch"
    elif key == "w":
        platform = "windows"
        arch = utils.arch_x86_64
    else:
        print "Invalid target: "+key

def print_rec(logger, prefix):
    s = ""
    if logger.result == 0:
        s += "Running    : "+prefix+str(logger.name)+"\n"
    elif logger.result == 1:
        s += "Failed     : "+prefix+str(logger.name)+"\n"
        if logger.cmd: s += "Cmd="+str(logger.cmd)+"\n"
        if logger.ex: s += str(logger.ex)+"\n"
        if logger.trace:
            for line in traceback.format_tb(logger.trace):
                s += line
    elif logger.result == 2:
        s += "Successfull: "+prefix+str(logger.name)+"\n"
    s += logger.buffer
        
    for child in logger.children:
        s += print_rec(child, prefix+str(logger.name)+"$")
    return s
        
def do_print(logger):
    while logger.result == 0:
        time.sleep(0.5)
        s = print_rec(logger, "")
        utils.clear()
        sys.stdout.write(s)
        sys.stdout.flush()

while (not "platform" in globals()) or not "arch" in globals():
    choose_target()

while True:

    print "Target: "+platform+", "+utils.arch_string(arch)
    print "e to exit, r to rebuild, t to change target"
    while utils.kbhit():
        utils.getch()
    key = utils.getch()
    if key == "e":
        break
    utils.clear()
    rebuild = False
    if key == "r":
        rebuild = True
    if key == "t":
        choose_target()
        continue
    
    
    for module in sys.modules.values():
        if module == sys: continue
        try:
            reload(module)
        except:
            pass
        
    logger = utils.Logger(name="root")
    thread = threading.Thread(target=do_print, args=[logger])
    thread.start()
    try:
        
        f = open("script.conf")
        execfile(f.read())
        f.close()
        logger.success()
            
    except Exception as ex:
        logger.failed()
        thread.join()
        os.chdir(cwd)
        traceback.print_exc()
        print ex
    
    thread.join()
