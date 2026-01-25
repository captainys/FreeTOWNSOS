import subprocess
import os

THISFILE=os.path.realpath(__file__)
THISDIR=os.path.dirname(THISFILE)

os.chdir(os.path.join(THISDIR,"tgbios"))
subprocess.Popen(["python","makedisk.py"]).wait()
