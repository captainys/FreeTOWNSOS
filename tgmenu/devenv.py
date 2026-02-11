import subprocess
import os

THISFILE=os.path.realpath(__file__)
THISDIR=os.path.dirname(THISFILE)


subprocess.Popen([
	"Tsugaru_Headless",
	os.path.join(THISDIR,"..","..","FreeTOWNSOS","CompROM"),
	"-cd",
	os.path.join(THISDIR,"..","..","FreeTOWNSOS","resources","CDIMG.ISO"),
	"-tgdrv",
	os.path.join(THISDIR,"..","..","HC386ENV"),
	"-tgdrv",
	THISDIR
]).wait()
