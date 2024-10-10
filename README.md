# Free TOWNS OS (TSUGARU OS)
# 互換TOWNS OS (津軽OS)

# Introduction

The goal of this project is to write a copyright-free FM Towns OS to run free games and the re-released games, or why not a brand-new game for FM Towns. without concerns of violating copyrights of the files included in the original Towns OS.

Let's see how far we can go!  But, so far so good.  Now Tsugaru OS is capable of running the three probably the most popular free games, Panic Ball 2, VSGP, and Sky Duel.  All playable without single file from the original Towns OS.

このプロジェクトの目標は、著作権フリーなFM Towns OSを実装して、もとのTowns OSの著作権を気にすることなくフリーゲームや、復刻版、さらには新規のFM TOWNS用アプリ開発を可能にすることです。

果たしてどこまでできるかはやってみないとわかりません。が、とりあえず、FM TOWNSのフリーソフトの中で、おそらく最も有名だったと思われる、Panic Ball 2, VSGP, Sky Duelの3本はやや音が違う箇所があるものの、プレイ可能になりました。



# Usage

とりあえず、お試しに release/FDIMG.bin を津軽から起動できます。release/FDIMG_USEROM.bin を使うと、ディスク上のYSDOS.SYS, YAMAND.COMの代わりにROMドライブのMSDOS.SYS, COMMAND.COMを使います。いずれ実機での動作も想定しているので、その実験用です。

To try it, you can boot from release/FDIMG.bin from Tsugaru.  release/FDIMG_USEROM.bin uses MSDOS.SYS and COMMAND.COM in the ROM drive, instead of YSDOS.SYS and YAMAND.COM on the disk.  I'm going to make it work on the real hardware.  It's a preparation for it.



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





# History
10/04/2024  Thanks to the great contribution from BCC and Ryu Takegami, VSGP and PANIC BALL 2 and Sky Duel are playable on the TSUGARU OS.

09/06/2024  Can start TGDRV and ORICON.  Support INT AEH, limited functions of INT 90H, INT 93H, and DOS devices.

08/27/2024  Started.
