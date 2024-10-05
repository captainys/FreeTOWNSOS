# Free TOWNS OS (TSUGARU OS)
# 互換TOWNS OS (津軽OS)

# Introduction

The goal of this project is to write a copyright-free FM Towns OS to run free games and the re-released games, or why not a brand-new game for FM Towns. without concerns of violating copyrights of the files included in the original Towns OS.

Let's see how far we can go!

このプロジェクトの目標は、著作権フリーなFM Towns OSを実装して、もとのTowns OSの著作権を気にすることなくフリーゲームや、復刻版、さらには新規のFM TOWNS用アプリ開発を可能にすることです。

果たしてどこまでできるかはやってみないとわかりません。



# Usage

とりあえず、お試しに release/FDIMG.bin を津軽から起動できます。release/FDIMG_USEROM.bin を使うと、ディスク上のYSDOS.SYS, YAMAND.COMの代わりにROMドライブのMSDOS.SYS, COMMAND.COMを使います。いずれ実機での動作も想定しているので、その実験用です。

To try it, you can boot from release/FDIMG.bin from Tsugaru.  release/FDIMG_USEROM.bin uses MSDOS.SYS and COMMAND.COM in the ROM drive, instead of YSDOS.SYS and YAMAND.COM on the disk.  I'm going to make it work on the real hardware.  It's a preparation for it.



# Build Instruction

すみません、書きます。

Sorry, I'm going to write it.




# Externals

ORICONはMIYAZAKIさんとYAMAZAKIさんによるコンソールエミュレータです。再配布自由とのことで、ご厚意に甘えて再配布させていただいています。MIYAZAKIさんとYAMAZAKIさんのご厚意とFM TOWNSコミュニティへの貢献に非常に感謝しています。

ORICON was a console emulator and included in this program by courtesy of the developers, Mr. MIYAZAKI and Mr. YAMAZAKI.  I sincerely appreciate their generosity and enormous contribution to the FM TOWNS community.

DOS-Extenderの置き換えには、nabeさんによるFree386を利用させていただいています。Free386はPublic Domain Softwareということなので、nabeさんのご厚意とFM TOWNSコミュニティへの貢献に感謝して再配布させていただいています。

Free386 is a PharLap-compatible DOS-Extender developed by nabe-abk.  Free386 is released as a PDS (Public Domain Software), and is included in this package.  I sincerely appreciate his generosity and enormous contribution to the FM TOWNS community.

YAMAND.COMとYSDOS.SYSは自分のプロジェクトから持ってきてます。https://github.com/captainys/TOWNSROM

YAMAND.COM and YSDOS.SYS are from my project https://github.com/captainys/TOWNSROM .





# History
10/04/2024  Thanks to the great contribution from BCC and Ryu Takegami, VSGP and PANIC BALL 2 and Sky Duel are playable on the TSUGARU OS.

09/06/2024  Can start TGDRV and ORICON.  Support INT AEH, limited functions of INT 90H, INT 93H, and DOS devices.

08/27/2024  Started.
