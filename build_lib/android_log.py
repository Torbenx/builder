import sys
sys.dont_write_bytecode = True
import subprocess
import utils

LOG_USAGE = "r to refresh pid, p to log package, c to clear, s to start, k to kill, e to exit logger"

package = "com.tt.builder"
adb_bin = "C:/Users/Torben/AppData/Local/Android/sdk/platform-tools/adb.exe"

def log_package(package):
    print "Logging "+package+", "+LOG_USAGE
    p = subprocess.Popen([adb_bin, "logcat", "-e", package])
    key = utils.getch()
    p.kill()
    return key
    
def log_pid(pid):
    print "Logging "+str(pid)+", "+LOG_USAGE
    p = subprocess.Popen([adb_bin, "logcat", "--pid="+str(pid)])
    key = utils.getch()
    p.kill()
    return key
    
def find_pid(package):
    p = subprocess.Popen([adb_bin, "shell", "for p in /proc/[0-9]*; do [[ $(<$p/cmdline) = %s ]] && echo ${p##*/}; done" % package], stdout = subprocess.PIPE)
    out,err = p.communicate()
    if out=="":
        return -1
    else:
        return int(out)
        
key = "r"
while True:
    if key == "r":
        pid = find_pid(package)
        if pid == -1:
            print "process not running"
            key = utils.getch()
        else:
            key = log_pid(pid)
    elif key == "p":
        key = log_package(package)
    elif key == "c":
        utils.clear()
        key = utils.getch()
    elif key == "k":
        utils.exec_cmd([adb_bin, "shell", "am force-stop "+package], False, False)
        key = utils.getch()
    elif key == "s":
        utils.exec_cmd([adb_bin, "shell", "am", "start", package+"/"+activity], False, False)
        key = utils.getch()
    elif key == "e":
        break
    else:
        key = utils.getch()