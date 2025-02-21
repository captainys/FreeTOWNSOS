import os
import sys
import shutil
import subprocess

THISFILE=os.path.realpath(__file__)
THISDIR=os.path.dirname(THISFILE)


def Copy(file):
	shutil.copyfile(
		os.path.join(THISDIR,"..","resources",file),
		os.path.join(THISDIR,"..","release",file))

def Run(argv):
	files=[
		"IO.SYS",
		"FAKENSDD.SYS",
		"MINVCPI.SYS",
		"SYSXXXX0.COM",
		"TGBIOS.SYS",
		"TGBIOS.BIN",
		"FD_IPL.BIN",
		"FDIMG.BIN",
		"RUNNERFD.BIN",
		"FDIMG_USEROM.BIN",
		"HDIMG.h0",
		"CDIMG.ISO",
	]
	for file in files:
		Copy(file)
		subprocess.Popen([
			"git","add","-f",os.path.join(os.path.join(THISDIR,"..","release",file))]).wait()


if __name__=="__main__":
	Run(sys.argv[1:])
