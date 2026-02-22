import subprocess
import os
import sys
import shutil


THISFILE=os.path.realpath(__file__)
THISDIR=os.path.dirname(THISFILE)

def Run(argv):
	os.chdir(THISDIR)

	if True!=os.path.isdir("BUILD"):
		os.mkdir("BUILD")

	proc=subprocess.Popen([
		"Tsugaru_Headless",
		os.path.join(THISDIR,"..","..","CompROM"),
		"-FD0",
		os.path.join(THISDIR,"..","..","buildenv","RUNNERFD.bin"),
		"-BOOTKEY",
		"F0",
		"-SHAREDDIR",
		os.path.join(THISDIR,"..","..","..","HC386ENV"),
		"-SHAREDDIR",
		THISDIR,
		"-DEBUG",
		"-DONTUSEFPU",
		"-CONSCMD",	"D:",
		"-CONSCMD",	"AUTOEXEC",
		"-CONSCMD",	"E:",
		"-CONSCMD","SET PATH=A:\;%PATH%", # Just in case
		"-CONSCMD",	"MAKE",
		"-CONSCMD",	"IF ERRORLEVEL 1 FAIL.EXE",
		"-CONSCMD",	"SUCCESS",
	])
	proc.communicate()
	if 0!=proc.returncode:
		print("Error bulding")
		quit()

	fp=open(os.path.join(THISDIR,"OUTPUT.TXT"),"r")
	for line in fp:
		print(line)
	fp.close()


	# proc=subprocess.Popen(["cl","../../util/makefd.cpp","../../util/dosdisk.cpp","/EHsc"])
	# proc.communicate()
	# if 0!=proc.returncode:
	# 	print("Error bulding makefd.exe")
	# 	quit()
	# subprocess.Popen(["./makefd",
	# 	"-o",		"FDIMG.bin",
	# 	"-i",		"BUILD/SND.EXP",
	# 	"-i",		"CARDINAL.SND",
	# ]).wait()



if __name__=="__main__":
	Run(sys.argv[1:])
