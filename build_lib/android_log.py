import sys
sys.dont_write_bytecode = True
import subprocess
import utils

LOG_USAGE = "Usage: r to refresh pid, p to log package, c to clear, s to start, k to kill, e to exit logger"

adb_bin = sys.argv[1]
package = sys.argv[2]
activity = sys.argv[3]

def log_package(package):
    print("Logging "+package)
    p = subprocess.Popen([adb_bin, "logcat", "-e", package])
    key = utils.getch()
    p.kill()
    return key

def log_pid(pid):
    print("Logging "+str(pid))
    p = subprocess.Popen([adb_bin, "logcat", "--pid="+str(pid)])
    key = utils.getch()
    p.kill()
    return key

def find_pid(package):
    p = subprocess.Popen([adb_bin, "shell", "for p in /proc/[0-9]*; do [[ $(<$p/cmdline) = %s ]] && echo ${p##*/}; done" % package], stdout = subprocess.PIPE)
    out,err = p.communicate()
    if out==b'':
        return -1
    else:
        return int(out)



key = b"R"
while True:
    if key == b"r":
        pid = find_pid(package)
        if pid == -1:
            print("process not running")
            key = utils.getch()
        else:
            key = log_pid(pid)
    elif key == b"R":
        pid = -1
        tries = 10
        while pid == -1 and tries > 0:
            pid = find_pid(package)
            tries -= 1
        if pid != -1:
            key = log_pid(pid)
        else:
            print("process not running")
            key = utils.getch()
    elif key == b"p":
        key = log_package(package)
    elif key == b"c":
        utils.clear()
        key = utils.getch()
    elif key == b"k":
        utils.exec_cmd([adb_bin, "shell", "am force-stop "+package], False, False)
        key = utils.getch()
    elif key == b"s":
        utils.exec_cmd([adb_bin, "shell", "am", "start", package+"/"+activity], False, False)
        key = utils.getch()
    elif key == b"S":
        utils.exec_cmd([adb_bin, "shell", "am", "start", package+"/"+activity], False, False)
        key = b"R"
    elif key == b"i":
        print("package: "+package)
        print("activity: "+activity)
        print("adb location: "+adb_bin)
        print(LOG_USAGE)
        key = utils.getch()
    elif key == b"e":
        break
    else:
        print(LOG_USAGE)
        key = utils.getch()
