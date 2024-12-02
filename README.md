# Free TOWNS OS (TSUGARU OS)
# 互換TOWNS OS (津軽OS)

# Introduction

The goal of this project is to write a copyright-free FM Towns OS to run free games and the re-released games, or why not a brand-new game for FM Towns. without concerns of violating copyrights of the files included in the original Towns OS.

Let's see how far we can go!  But, so far so good.  Now Tsugaru OS is capable of running the three probably the most popular free games, Panic Ball 2, VSGP, and Sky Duel.  I've tested Planet Attacker, Tactical Air Wing, and Ground Attack, which all worked ok.  All playable without single file from the original Towns OS.

このプロジェクトの目標は、著作権フリーなFM Towns OSを実装して、もとのTowns OSの著作権を気にすることなくフリーゲームや、復刻版、さらには新規のFM TOWNS用アプリ開発を可能にすることです。

果たしてどこまでできるかはやってみないとわかりません。が、とりあえず、FM TOWNSのフリーソフトの中で、おそらく最も有名だったと思われる、Panic Ball 2, VSGP, Sky Duelの3本はやや音が違う箇所があるものの、プレイ可能になりました。我が、Planet Attacker, Tactical Air Wing, Ground Attackも動作確認できました。



# Usage

とりあえず、お試しに release/FDIMG.bin を津軽から起動できます。release/FDIMG_USEROM.bin を使うと、ディスク上のYSDOS.SYS, YAMAND.COMの代わりにROMドライブのMSDOS.SYS, COMMAND.COMを使います。いずれ実機での動作も想定しているので、その実験用です。

あるいは、release/HDIMG.h0 をSCSI-ID 0にマウントして、他に何もメディアを入れずに津軽を起動しても津軽OSを起動することができます。なお、津軽OSはCMOSのドライブ設定に依存せず、SCSI ID 0のPartition 0から順にドライブレターを割り振っていくので、CMOS設定が無くてもハードディスクから起動できます。

また、CDIMG.ISOをCD-Rに焼くことで、とりあえず実機TOWNSに接続したSCSI CDドライブからの起動も確認できています。

To try it, you can boot from release/FDIMG.bin from Tsugaru.  release/FDIMG_USEROM.bin uses MSDOS.SYS and COMMAND.COM in the ROM drive, instead of YSDOS.SYS and YAMAND.COM on the disk.  I'm going to make it work on the real hardware.  It's a preparation for it.

Or, you can mount release/HDIMG.h0 on SCSI ID 0 and run it from Tsugaru.  Tsugaru OS does not rely on CMOS setting, so the first partition of SCSI ID 0 (or whatever found first) will be assigned to D drive.  It can start without CMOS setup.

I have confirmed that if you burn CDISO.ISO to a CD-R, you can boot from a SCSI CD drive connected to the real TOWNS.



# Build Instruction

## 必要なもの

- Python: ビルドスクリプトを実行するために必要。あんまり好きな言語ではない。どっちかというと嫌い。
- Netwide Assembler: Pathを通して、コマンドからNASMとタイプして実行できるように。
- Visual C++: 2019でビルド確認。Developer Command PromptまたはDeveloper Power Shellから使う。
- FM TOWNSエミュレータ「津軽」実行環境。Tsugaru_CUI.exeをビルドに使うので、パスを通してください。
- High-C Multimedia Kit
- FM TOWNS用386|Assembler Toolkit。なお、DOS用386ASMはFM TOWNSで実行できないらしい。
- Make フリコレ9に収録のもの。

## 準備

NASM, Visual C++のセットアップは多分探せばわかるでしょう。

津軽の実行環境と、TGDRV.COMの使い方がわかってる必要があります。

High-Cと386ASMのセットアップはやや大変かもしれませんが、津軽上のハードディスクイメージにHigh-Cと386ASMを以下のディレクトリにセットアップしてください。
```
D:\HC386
D:\386ASM
```
386ASMは386ASM.EXE, 386LINK.EXEなど実行ファイルを386ASMディレクトリにコピーしておけばOK。次に、フリコレ9のMAKE.EXEを、津軽上で
```
D:\EXE
```
にコピーしてください。

次に、FreeTOWNSOSと同じレベルに、
```
HC386ENV
```
というディレクトリを作って、津軽からTGDRVを使って、津軽上でセットアップした上の3個のディレクトリをコピーしてください。

これで、環境の準備は完了です。

## ビルド

正しく環境がセットアップできていたならば、tgbiosサブディレクトリで makedisk.py を実行すると、全部ビルドして、HDIMG.h0, FDIMG.bin, FDIMG_USEROM.bin の3本のバイナリファイルができるはずです。

だがしかし、上の説明が間違ってる可能性も結構あったりして。

多分Windowsの実行ファイルで完結してる部分、NASM, Visual C++は問題ないと思うけど、津軽上でビルドするTGBIOSがちょっとひっかかるかも。津軽のVMは、make_build_envのAUTOEXEC.BATが想定するディレクトリにHigh-C, 386ASMがあると思ってtgbios下のTASK.BATを実行するので、そのようになってるか確認してください。





Sorry, I'm going to write the English translation.




# Externals

ORICONはMIYAZAKIさんとYAMAZAKIさんによるコンソールエミュレータです。再配布自由とのことで、ご厚意に甘えて再配布させていただいています。MIYAZAKIさんとYAMAZAKIさんのご厚意とFM TOWNSコミュニティへの貢献に非常に感謝しています。

ORICON was a console emulator and included in this program by courtesy of the developers, Mr. MIYAZAKI and Mr. YAMAZAKI.  I sincerely appreciate their generosity and enormous contribution to the FM TOWNS community.

DOS-Extenderの置き換えには、nabeさんによるFree386を利用させていただいています。Free386はPublic Domain Softwareということなので、nabeさんのご厚意とFM TOWNSコミュニティへの貢献に感謝して再配布させていただいています。

Free386 is a PharLap-compatible DOS-Extender developed by nabe-abk.  Free386 is released as a PDS (Public Domain Software), and is included in this package.  I sincerely appreciate his generosity and enormous contribution to the FM TOWNS community.

YAMAND.COMとYSDOS.SYSは自分のプロジェクトから持ってきてます。https://github.com/captainys/TOWNSROM

YAMAND.COM and YSDOS.SYS are from my project https://github.com/captainys/TOWNSROM .



# TOWNS_CD

TOWNS OSは、MSCDEXのハードウェアレイアドライバとして、TOWNS_CDというドライバを使います。津軽OSも同じ名前を踏襲しています。津軽OSのTOWNS_CDは、内蔵CDドライブからSCSI CDドライブへのリダイレクション機能をサポートしています。これは、従来YSSCSICD.SYSによって実現していた機能と同等です。

拡張機能として、TOWNS_CDに文字を書き込むことで、リダイレクションをコントロールできます。例えば、
```
ECHO E > TOWNS_CD
```
と書くことで、SCSIデバイスを6番から0番まで順にスキャンして、最初に見つけたCDドライブを内蔵ドライブとして扱います。もしも、複数のCDドライブを接続している状態で、小さな番号のSCSI IDを優先したい場合は、次のように書きます。
```
ECHO e > TOWNS_CD
```
また、リダイレクション対象SCSI IDを指定して有効化したいときは、
```
ECHO 4 > TOWNS_CD
```
のように書くことで、SCSI ID 4番のCDにリダイレクトすることができます。

その他、
```
ECHO P > TOWNS_CD
```
でリダイレクション一時停止(Dでも可)、
```
ECHO R > TOWNS_CD
```
で再開できます。なお、大文字・小文字は区別します。

TOWNS OS uses a DOS driver called TOWNS_CD as a hardware layer for MSCDEX.  Tsugaru OS has its own version of TOWNS_CD.  This driver has an additional functionality of redirecting internal CD to external SCSI CD drive.  This functionality is same as the functionality of YSSCSICD.SYS.

In addition to YSSCSICD.SYS, Tsugaru OS's TOWNS_CD can enable or disable redirection by writing a letter to TOWNS_CD.  For example, by typing a command:
```
ECHO E > TOWNS_CD
```
You can enable the redirection.  TOWNS_CD driver will scan SCSI ID from 6 to 0 in the descending order and use the first CD drive found.  If you have multiple SCSI CD drives connected, and if you want to use the smallest SCSI ID, type:
```
ECHO e > TOWNS_CD
```
Or, if you want to enable the redirection for a specific SCSI ID, type:
```
ECHO 4 > TOWNS_CD
```
then TOWNS_CD will redirect CD BIOS to SCSI ID 4.

Or,
```
ECHO P > TOWNS_CD
```
to pause the redirection (D also works), and
```
ECHO R > TOWNS_CD
```
for resuming the redirection.




# FORRBIOS.NSD

TOWNS OS V2.1 added a new device-driver mechanism called NSDD, which stands for (probably) Native System Device Driver.  The driver is a straight-forward extension of MS-DOS device driver, it has entry point for Strategy and Interrupt.

This mechanism is built on top of COCO memory-management, which is largely undocumented and in the dark.

However, the programs compiled by F-BASIC 386 Compiler, and High-C Multimedia Kit require one of such NSDDs called FORRBIOS.

Tsugaru OS includes a driver that fakes COCO and FORRBIOS.

nabe@abk did a very detailed research on COCO memory-management API functions, and is available from:

https://github.com/nabe-abk/free386/blob/main/doc-ja/dosext/coco_nsd.txt

FAKENSDD.SYS, included in the Tsugaru OS implements only minimum functions of COCO.

FAKENSDD.SYS also loads a minimum version of FORRBIOS.  FORRBIOS is not really using Strategy and Interrupt, but it expands the real-mode device drivers.

For example, CD-ROM BIOS INT 93H AH=54H, Read TOC, takes DS:DI as the data buffer pointer.  However, if the same INT is issued in the protected mode, the data buffer pointer is DS:EDI.





# History
10/04/2024  Thanks to the great contribution from BCC and Ryu Takegami, VSGP and PANIC BALL 2 and Sky Duel are playable on the TSUGARU OS.

09/06/2024  Can start TGDRV and ORICON.  Support INT AEH, limited functions of INT 90H, INT 93H, and DOS devices.

08/27/2024  Started.
