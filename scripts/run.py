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
	"-BOOTKEY",
	"F0",
	"-FD0",
	FDIMG,
	"-DEBUG",
	"-INITCMD","ENA FDCMON",
]+sys.argv[1:])

proc.wait()

