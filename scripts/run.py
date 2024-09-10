import subprocess
import os
import sys

THISFILE=os.path.realpath(__file__)
THISDIR=os.path.dirname(THISFILE)

ROMDIR=os.path.join(THISDIR,"..","CompROM")
SRCDIR=os.path.join(THISDIR,"..","src")
FDIMG=os.path.join(SRCDIR,"FDIMG.bin")

proc=subprocess.Popen([
	"Tsugaru_CUI",
	ROMDIR,
	"-HIGHFIDELITY",
	"-USEFPU",
	"-MEMSIZE","16",
	"-BOOTKEY",
	"F0",
	"-FD0",
	FDIMG,
	"-FD1",
	os.path.join(THISDIR,"..","..","TOWNSEMU_TEST","DISKIMG","TBIOSV1.1L30.d77"),
	"-DEBUG",
	"-INITCMD","ENA FDCMON",
	# "-INITCMD","BP 31A7:00002893", # Stop in free386.com before call init_dos_malloc
	"-SHAREDDIR",os.path.join(THISDIR,"..","tgdrv"),
	# "-POWEROFFAT","2B3D:100",
	# "-UNITTEST",

	#"-initcmd","SAVESTATEAT 50:0 00500000.tstate",
	#"-initcmd","SAVESTATEAT B800:0 B8000000.tstate",
	#"-initcmd","SAVESTATEAT B800:262 B8000262.tstate",
	#"-initcmd","SAVESTATEAT B800:2CC B80002CC.tstate",

]+sys.argv[1:]).wait()
