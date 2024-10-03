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
		"python",
		os.path.join(THISDIR,"..","src","build.py")])
	proc.communicate()
	if 0!=proc.returncode:
		print("Error bulding IO.SYS")
		quit()


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
		print("Error bulding TGBIOS.BIN")
		quit()

	proc=subprocess.Popen(["cl","../util/dosdisk.cpp","/EHsc"])
	proc.communicate()
	if 0!=proc.returncode:
		print("Error bulding dosdisk.exe")
		quit()

	subprocess.Popen(["./dosdisk",
		"-o",		"FDIMG.bin",
		"-ipl",		"../src/FD_IPL.bin",
		"-i",		"../resources/IO.SYS",
		"-i",		"../resources/YSDOS.SYS",
		"-i",		"../resources/YAMAND.COM",
		"-i",		"CONFIG.SYS",
		"-i",		"../resources/AUTOEXEC.BAT",
		"-i",		"../resources/TGDRV.COM",
		"-i",		"../resources/TEST.EXP",
		"-i",		"../src/MINVCPI.SYS",
		"-i",		"../externals/ORICON/ORICON.COM",
		"-i",		"../externals/Free386/free386.com",
		"-i",		"TGBIOS.SYS",
		"-i",		"TGBIOS.BIN",
	]).wait()

	subprocess.Popen(["./dosdisk",
		"-o",		"FDIMG_USEROM.bin",
		"-ipl",		"../src/FD_IPL.bin",
		"-i",		"../resources/IO.SYS",
		"-i",		"CONFIG.SYS",
		"-i",		"../resources/AUTOEXEC.BAT",
		"-i",		"../resources/TGDRV.COM",
		"-i",		"../resources/TEST.EXP",
		"-i",		"../src/MINVCPI.SYS",
		"-i",		"../externals/ORICON/ORICON.COM",
		"-i",		"../externals/Free386/free386.com",
		"-i",		"TGBIOS.SYS",
		"-i",		"TGBIOS.BIN",
	]).wait()


if __name__=="__main__":
	Run(sys.argv[1:])
