# NoCheatZ-4 [![Build Status Linux](https://travis-ci.org/TortoiseOrHare/NoCheatZ-4.svg?branch=master)](https://travis-ci.org/TortoiseOrHare/NoCheatZ-4) [![Build Status Windows](https://ci.appveyor.com/api/projects/status/89j3kmcdpwo5fy03?svg=true)](https://ci.appveyor.com/project/TortoiseOrHare/nocheatz-4)
Source Engine serversided anti-cheat plugin. (CS:S, CS:GO).

# Table of content

1. [Introduction](#Introduction)
2. [Goal](#Goal)
3. [Current Project Status](#Status)
4. [Features](#Features)
    1. [Systems](#Systems)
        1. [Detection Systems](#detection-systems)
        2. [Blocking Systems](#blocking-systems)
        3. [Other Systems](#other-systems)
    2. [Available Commands](#command)
5. [Contributing](#Contributing)
    1. [Installing a Game Server](#Installation)
    2. [Cloning the repository](#Cloning)
    3. [Compiling the project](#Compiling)
        1. [With Linux](#Linux)
        2. [With Windows](#Windows)
        3. [With Mac](#Mac)
    4. [What to do with the code ?](#what-to-do)

<a name="Introduction"></a>
# Notes about anti-cheat systems in general
 
> * Be aware that no anti-cheat in this world can detect every cheating for this game.
* Also be aware that there is no such a thing as an anti-cheat system that detects each and every cheating method.
  
### So far there are 4 kinds of anti-cheats :

* Client side anti-cheats
* Server side anti-cheats
* Both client and server side anti-cheats
* **Yourself**

### Let me explain the differences ...

* Some anti-cheat systems can detects injected programs and illegal memory patchs, such as VAC. However those anti-cheat systems can be bypassed because they run on the **clients computer** - morever, requiring the install of yet another anti-cheat **will decrease the activity of your server** because clients will have to lose time downloading unwanted programs to play there. In addition, sometimes that anti cheat may not be compatible.
Client side anti cheat systems also suffers from the fact they are **reading -private- data** on client computers and can access -anything-.

* The **server sided anti cheats** are more concerned about the data the client is sending to the server. But this can also be exploited on client side.
On the other hand, **the server can block for sure the data** it sends to the clients while client side anti-cheats can always be bypassed for those blocking systems.
The avantage is the server side anti cheats are less subjects to bypassing (If the admin is of confidence) because they are more protected. But they can't access the entire computer of the clients and, because of that, **are less effective** because they must run more heuristic tests that returns more false detections than client sided anti cheats.

* Anti-cheat systems that runs on **both client and server side** just merge all the drawbacks and advantages.

* And there is you. If you read this page, this should be because you think you have a lot of cheating in your server. So you know (or at least think to know) when someone seems to be cheating and **you need a solution to automatically ban cheaters**.
  
But if there was something to tell you "*Hey, there's something suspicious right now ...*" when you're viewing a demo or "*Hey, I banned this cheater. Look at this if you don't trust me*", this would help a lot, wouldn't it ?
 
#### NoCheatZ 4 is here to help.
* It's not an anti-cheat that will detect everything - *don't get the wrong idea because such anti-cheat does not exist and will never* - but it will at least **help you** detect cheats and at most **ban** confirmed **cheaters**, **sparing you not only plenty of time but also money**.

<a name="Goal"></a> 
# Goal
___
The goal is to make an anti-cheat that can be used with any type of gameservers.


* Because it can be used in competition it must be fast, consistent and safe.

* And because it can be used with others servers it must also be configurable, easy to use and open-source.


Supported games will be [CS:S](http://store.steampowered.com/app/240), [CS:GO](http://store.steampowered.com/app/730), [CS:P](http://cspromod.com/), [Black Mesa](http://store.steampowered.com/app/362890) and maybe more.

Currently [CS:GO](http://store.steampowered.com/app/730) is the base of this project.

<a name="Status"></a> 
# Current Project Status
___

* **Status** : [Killer humanized aimbot detection incoming ... if you're wise](https://github.com/L-EARN/NoCheatZ-4/tree/master)

<a name="Features"></a> 
# Features
___

<a name="Systems"></a> 
## Systems
___

<a name="detection-systems"></a> 
### Detection Systems
___

* ConCommandTester : Test and block player commands for common exploits on Source Engine servers such as rcon password stealing.
* ConVarTester : Test and detect players using old ConVar bypassers. **Can link the value to the server if the ConVar exists on the server**
* EyeAnglesTester : Provides good detection against badly coded aimbots and spinhacks.
* JumpTester : Detects bunnyhop programs and scripts (**Is able to make the difference**)
* AutoAttackTester : Detects robotic shot button spam.
* SpamChangeNameTester : Detects when a players changes his name too often (_Is now fixed internally by the Source Engine_)
* SpamConnectTester : Detects when a client is flooding the connection (Trying to connect and disconnect too much)
* SpeedTester : Detect SpeedHacks using CUserCmd and tickrate ratio.
* MouseTester : Detect almost any type of injected aimbots

<a name="blocking-systems"></a> 
### Blocking Systems
___

* AntiFlashbangBlocker : Send a extra white screen and stop transmit to fully blind players.
* AntiSmokeBlocker : Players inside a smoke cannot see others. **Players can't see thru smokes even when r_drawparticles is 0**.
* WallhackBlocker : Player visibility is tested against the walls.
* RadarHackBlocker : Block aimbots and ESPs that use the radar positions.
* BhopBlocker : Rejects jumps that are sent when the player lands on world for a short period of time.

<a name="other-systems"></a> 
### Other Systems

* AutoTVRecord : Automatically spawn the SourceTV. Automatically record the game when at least one human player is in game (*Rather than record when the server is empty ...*). **Detection logs also store the tick of the current record for easier reviewing**.
* BanRequest : Some bans are delayed by 20 seconds to try to detect more things.
* ConfigManager : Store informations about the different games the plugin can be used for ([config.ini](https://github.com/L-EARN/NoCheatZ-4/blob/master/server-plugin/Res/config.ini))
* Logger : Outputs NoCheatZ logs into `logs/NoCheatZ_4_Logs/`

<a name="command"></a> 
## Available commands
___

See https://github.com/L-EARN/NoCheatZ-4/blob/master/server-plugin/Res/nocheatz.cfg

<a name="Contributing"></a> 
# Contributing
___

<a name="Installation"></a> 
## Installing a Game Server
___

* Even though you're not forced to install a local game-server to build the project, it's more convenient to test your build directly.
However, running the game-server and the client game on the same computer will introduce a lot of lags if not using a very performant computer.
It is recommended to host the server on another computer (Either LAN or remote). A remote is preferred because it matches "production" context.

See https://developer.valvesoftware.com/wiki/SteamCMD

* It is recommanded, with Windows, to install your server into his own directory.
The Visual Studio project launches a commandfile (to auto-update the server before running the debugger) expecting steamcmd to be installed at E:\steamcmd\steamcmd.exe
You can disable this script in the post-build event options of the project. You can also edit it for your own configuration.

```bash
login anonymous
force_install_dir E:\steamcmd\CSS
app_update 232330 validate
```

```bash
login anonymous
force_install_dir E:\steamcmd\CSGO
app_update 740 validate
```

<a name="Cloning"></a> 
## Cloning the repository
___

`su` to your srcds user.

```git
git clone git://github.com/L-EARN/NoCheatZ-4.git
cd NoCheatZ-4
git submodule update --init
```

<a name="Compiling"></a> 
## Compiling the project
___

<a name="Linux"></a> 
### With Linux
___

1. `su` to root.

2. Install your compiler tools :

```sh
dpkg --add-architecture i386
apt-get update
apt-get install git g++ g++-multilib build-essential ia32-libs lib32gcc1 libc6-i386 libc6-dev-i386 autotools-dev autoconf libtool gdb screen
```

If your version of g++ is higher than g++ 4.8.5, then you must install g++-4.8 instead :

```sh
apt-get install g++-4.8 g++-4.8-multilib
```

3. `su` to your srcds user.

4. With your console, go to the `server-plugin` directory and type `make debug CXX=g++-4.8` or `make release CXX=g++-4.8`

The binary and all other stuff will go in `NoCheatZ-4/Builds/[CONFIG]/addons/NoCheatZ`

You can copy-paste the entire content of the `Builds` directory into the root directory of your gameserver.
(e.g. `cp /R ./NoCheatZ-4/Builds/Release/* /home/user/steamcmd/CSGO/csgo` )

5. If you want to run the server using gdb, create a script in `steamcmd/CSGO` with something like this :

```sh
#!/bin/sh

export LD_LIBRARY_PATH=".:bin:$LD_LIBRARY_PATH"
gdb --args ./srcds_linux -console -condebug -condump -usercon -game csgo -steam_dir ../ -steamcmd_script ../steamcmd.sh -insecure +net_public_adr xxx.xxx.xxx.xxx +ip xxx.xxx.xxx.xxx +game_type 0 +game_mode 1 +map de_inferno +rcon_password cderfv
```

For availables game-modes : https://developer.valvesoftware.com/wiki/Counter-Strike:_Global_Offensive_Dedicated_Servers#Starting_the_Server

<a name="Windows"></a> 
### With Windows
___

1. Make sure you installed steamcmd at `E:\steamcmd\steamcmd.exe` if you want a localhost game-server

2. Download and extract [libprotobuf src 2.5.0](https://github.com/google/protobuf/archive/v2.5.0.zip) in server-plugin\SourceSdk\Interfaces\Protobuf\

3. Download and extract [googletest src 1.5.0](https://github.com/google/googletest/archive/release-1.5.0.zip) in server-plugin\SourceSdk\Interfaces\Protobuf\protobuf-2.5.0\

4. Rename the extracted folder of googletest into `gtest`

3. Open the solution with [Visual Studio 2017](https://www.visualstudio.com/downloads/)  (Visual Studio Community 2017 is free and just requires to setup a Microsoft Account)

4. Choose your configuration.

5. Click build. 
         
<a name="Mac"></a> 
### With Mac
___

* You need to setup the makefile to use Clang, and also implement parts of code in the project that are not already translated for OSX.

> I don't eat apples so I can't really do it myself ...

<a name="what-to-do"></a> 
## What to do with the code ?
___

* Whatever you want but first make sure to have your own branch a/o fork.

* Feel free to report using [Issues](https://github.com/L-EARN/NoCheatZ-4/issues) if you see anything you think is not good, for a feature request or just some help.
