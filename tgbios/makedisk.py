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
		os.path.join(THISDIR,"..","iosys","build.py")])
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



	proc=subprocess.Popen(["cl","../util/makefd.cpp","../util/dosdisk.cpp","/EHsc"])
	proc.communicate()
	if 0!=proc.returncode:
		print("Error bulding makefd.exe")
		quit()

	proc=subprocess.Popen(["cl","../util/makehd.cpp","../util/dosdisk.cpp","/EHsc"])
	proc.communicate()
	if 0!=proc.returncode:
		print("Error building makehd.exe")
		quit()

	proc=subprocess.Popen(["./makefd",
		"-o",		"FDIMG.bin",
		"-ipl",		"../iosys/FD_IPL.bin",
		"-i",		"../resources/IO.SYS",
		"-i",		"../resources/YSDOS.SYS",
		"-i",		"../resources/YAMAND.COM",
		"-i",		"../resources/FD/CONFIG.SYS",
		"-i",		"../resources/FD/AUTOEXEC.BAT",
		"-i",		"../resources/TGDRV.COM",
		"-i",		"../resources/TEST.EXP",
		"-i",		"../iosys/MINVCPI.SYS",
		"-i",		"../externals/ORICON/ORICON.COM",
		"-i",		"../externals/Free386/free386.com",
		"-i",		"TGBIOS.SYS",
		"-i",		"TGBIOS.BIN",
	])
	proc.communicate()
	if 0!=proc.returncode:
		print("Error building FDIMG.bin")
		quit()

	proc=subprocess.Popen(["./makefd",
		"-o",		"FDIMG_USEROM.bin",
		"-ipl",		"../iosys/FD_IPL.bin",
		"-i",		"../resources/IO.SYS",
		"-i",		"../resources/FD/CONFIG.SYS",
		"-i",		"../resources/FD/AUTOEXEC.BAT",
		"-i",		"../resources/TGDRV.COM",
		"-i",		"../resources/TEST.EXP",
		"-i",		"../iosys/MINVCPI.SYS",
		"-i",		"../externals/ORICON/ORICON.COM",
		"-i",		"../externals/Free386/free386.com",
		"-i",		"TGBIOS.SYS",
		"-i",		"TGBIOS.BIN",
	])
	proc.communicate()
	if 0!=proc.returncode:
		print("Error building FDIMG_USEROM.bin")
		quit()

	proc=subprocess.Popen(["./makefd",
		"-o",		os.path.join(THISDIR,"..","tests","ctest","TESTFD.bin"),
		"-ipl",		"../iosys/FD_IPL.bin",
		"-i",		"../resources/IO.SYS",
		"-i",		"../resources/TESTFD/CONFIG.SYS",
		"-i",		"../resources/TESTFD/AUTOEXEC.BAT",
		"-i",		"../resources/SUCCESS.EXE",
		"-i",		"../resources/FAIL.EXE",
		"-i",		"../resources/TGDRV.COM",
		"-i",		"../iosys/MINVCPI.SYS",
		"-i",		"../externals/ORICON/ORICON.COM",
		"-i",		"../externals/Free386/free386.com",
		"-i",		"TGBIOS.SYS",
		"-i",		"TGBIOS.BIN",
	])
	proc.communicate()
	if 0!=proc.returncode:
		print("Error building TESTFD.bin")
		quit()

	proc=subprocess.Popen(["./makehd",
		"-o",		"HDIMG.h0",
		"-p",		"8", "TSUGARU_OS",
		"-mbr",		"../iosys/HD_MBR.bin",
		"-ipl",		"../iosys/HD_IPL.bin",
		"-i",		"0",	"../resources/IO.SYS",
		"-i",		"0",	"../resources/YSDOS.SYS",
		"-i",		"0",	"../resources/YAMAND.COM",
		"-i",		"0",	"../resources/HD/CONFIG.SYS",
		"-i",		"0",	"../resources/HD/AUTOEXEC.BAT",
		"-i",		"0",	"../resources/TGDRV.COM",
		"-i",		"0",	"../resources/TEST.EXP",
		"-i",		"0",	"../iosys/MINVCPI.SYS",
		"-i",		"0",	"../externals/ORICON/ORICON.COM",
		"-i",		"0",	"../externals/Free386/free386.com",
		"-i",		"0",	"TGBIOS.SYS",
		"-i",		"0",	"TGBIOS.BIN",
	])
	proc.communicate()
	if 0!=proc.returncode:
		print("Error building HDIMG.h0")
		quit()

	CopyToResources("TGBIOS.SYS")
	CopyToResources("TGBIOS.BIN")
	CopyToResources("FDIMG.BIN")
	CopyToResources("FDIMG_USEROM.BIN")
	CopyToResources("HDIMG.h0")


	fp=open(os.path.join(THISDIR,"OUTPUT.TXT"),"r")
	for line in fp:
		print(line)
	fp.close()



if __name__=="__main__":
	Run(sys.argv[1:])
