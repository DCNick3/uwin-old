
import sys
import os.path
import subprocess
import re
#import progressbar

REX = re.compile(r"homm3benchmark: ([0-9.]+) secs elapsed")

uwin_executable = os.path.realpath(sys.argv[1])
homm3_executable = os.path.realpath(sys.argv[2])
workdir = os.path.dirname(uwin_executable)

def run_once():
    output = subprocess.check_output([uwin_executable, "--non-interactive", homm3_executable], cwd=workdir, stderr=subprocess.STDOUT, universal_newlines=True)
    return float(REX.findall(output)[0])
    
times = [ run_once() for x in range(0, 20) ]

print(times)
def stat(a):
    mn = min(a)
    mx = max(a)
    avg = sum(a) / len(a)
    stdev = sum((x - avg) * (x - avg) for x in a) / len(a)
    print("%f %f %f %f" % (mn, mx, avg, stdev))

stat(times)
