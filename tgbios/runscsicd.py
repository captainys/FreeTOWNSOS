# CD IPL is mainly for targeting running Tsugaru OS on actual TOWNS.

import subprocess
import os
import sys

THISFILE=os.path.realpath(__file__)
THISDIR=os.path.dirname(THISFILE)

ROMDIR=os.path.join(THISDIR,"..","..","TOWNSEMU_TEST","ROM_MX")
FDIMG=os.path.join(THISDIR,"FDIMG.bin")
HDIMG=os.path.join(THISDIR,"HDIMG.h0")
CDIMG=os.path.join(THISDIR,"CDIMG.ISO")

proc=subprocess.Popen([
	"Tsugaru_CUI",
	ROMDIR,
	"-HIGHFIDELITY",
	"-USEFPU",
	"-MEMSIZE","16",
	"-BOOTKEY",
	"H3",
	"-SCSICD3",
	CDIMG,
	"-FASTSCSI",
	"-DEBUG",
	"-INITCMD","ENA FDCMON",
	# "-INITCMD","BP 31A7:00002893", # Stop in free386.com before call init_dos_malloc
	"-SHAREDDIR",os.path.join(THISDIR,"..","..","TOWNSEMU_TEST","FREEWARE"),
	"-SHAREDDIR",os.path.join(THISDIR,"..","..","TOWNSEMU","testc"),
	"-SHAREDDIR",os.path.join(THISDIR,"..","tests"),
	# "-POWEROFFAT","2B3D:100",
	# "-UNITTEST",

	#"-initcmd","SAVESTATEAT 50:0 00500000.tstate",
	#"-initcmd","SAVESTATEAT B800:0 B8000000.tstate",
	#"-initcmd","SAVESTATEAT B800:262 B8000262.tstate",
	#"-initcmd","SAVESTATEAT B800:2CC B80002CC.tstate",

	"-scale","200",
	"-keyboard","trans",
	"-gameport0","phys0",
]+sys.argv[1:]).wait()
