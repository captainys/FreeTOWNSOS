import subprocess
import os
import sys
import shutil


THISFILE=os.path.realpath(__file__)
THISDIR=os.path.dirname(THISFILE)

def CopyToResources(filename):
	shutil.copyfile(filename,os.path.join("..","resources",filename))

def Run(argv):
	os.chdir(THISDIR)

	proc=subprocess.Popen([
		"Tsugaru_CUI",
		os.path.join(THISDIR,"..","CompROM"),
		"-FD0",
		os.path.join(THISDIR,"FDIMG.bin"),
		"-BOOTKEY",
		"F0",
		"-SHAREDDIR",
		os.path.join(THISDIR,"..","tgbiostest"),
		"-SHAREDDIR",
		THISDIR,
		"-DEBUG",
		"-USEFPU",
	])
	proc.communicate()
	if 0!=proc.returncode:
		print("Error bulding TGBIOS.BIN")
		quit()

if __name__=="__main__":
	Run(sys.argv[1:])
