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

	if True!=os.path.isdir("BUILD"):
		os.mkdir("BUILD")

	proc=subprocess.Popen([
		"Tsugaru_CUI",
		os.path.join(THISDIR,"..","CompROM"),
		"-FD0",
		os.path.join(THISDIR,"..","make_build_env","BUILDTGBIOS.bin"),
		"-BOOTKEY",
		"F0",
		"-SHAREDDIR",
		os.path.join(THISDIR,"..","..","HC386ENV"),
		"-SHAREDDIR",
		THISDIR,
		"-DEBUG",
		"-UNITTEST",
		"-USEFPU",
	])
	proc.communicate()
	if 0!=proc.returncode:
		print("Error bulding")
		quit()


	proc=subprocess.Popen(["cl","../util/dosdisk.cpp","/EHsc"])
	proc.communicate()
	if 0!=proc.returncode:
		print("Error bulding dosdisk.exe")
		quit()


	subprocess.Popen(["./dosdisk",
		"-o",		"FDIMG.bin",
		"-i",		"BUILD/SND.EXP",
		"-i",		"CARDINAL.SND",
	]).wait()



if __name__=="__main__":
	Run(sys.argv[1:])
