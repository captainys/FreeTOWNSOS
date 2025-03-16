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

	proc=subprocess.Popen(["cl","../util/makefd.cpp","../util/dosdisk.cpp","/EHsc"])
	proc.communicate()
	if 0!=proc.returncode:
		print("Error bulding makefd.exe")
		quit()

	proc=subprocess.Popen(["./makefd",
		"-o",		"BUILDTGBIOS.bin",
		"-ipl",		"../iosys/FD_IPL.bin",
		"-i",		"../resources/IO.SYS",
		"-i",		"../resources/YSDOS.SYS",
		"-i",		"../resources/YAMAND.COM",
		"-i",		"CONFIG.SYS",
		"-i",		"AUTOEXEC.BAT",
		"-i",		"../resources/TGDRV.COM",
		"-i",		"../resources/TEST.EXP",
		"-i",		"../resources/SUCCESS.EXE",
		"-i",		"../resources/FAIL.EXE",
		"-i",		"../resources/TGCONS.SYS",
		"-i",		"../resources/MINVCPI.SYS",
		"-i",		"../externals/Free386/free386.com",
	])
	proc.communicate()
	if 0!=proc.returncode:
		print("Error making disk image.")
		quit()

if __name__=="__main__":
	Run(sys.argv[1:])
