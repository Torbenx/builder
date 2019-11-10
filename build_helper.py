import sys
sys.dont_write_bytecode = True

import os
import subprocess
import traceback
import shutil
from engine.build_lib import utils, android, assets

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
        
    try:
        
        f = open("script.conf")
        execfile(f.read())
        f.close()
            
    except Exception as ex:
        os.chdir(cwd)
        traceback.print_exc()
        print ex
