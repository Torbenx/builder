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
        
already_logged = []

def print_rec(logger, prefix):
    global already_logged
    s = ""
    if logger.result != 0 and prefix+str(logger.name) not in already_logged:
        if logger.result == 1:
            print "Failed     : "+prefix+str(logger.name)
            if logger.cmd: print "Cmd="+str(logger.cmd)
            if logger.ex: print str(logger.ex)
            if logger.trace:
                for line in traceback.format_tb(logger.trace):
                    print line
        elif logger.result == 2:
            print "Successfull: "+prefix+str(logger.name)
        sys.stdout.write(logger.buffer)
        sys.stdout.flush()
        already_logged.append(prefix+str(logger.name))
        
    for child in logger.children:
        print_rec(child, prefix+str(logger.name)+"$")
        
def do_print(logger):
    global already_logged
    already_logged = []
    while logger.result == 0:
        print_rec(logger, "")
        time.sleep(0.1)

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
