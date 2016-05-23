# NoCheatZ-4
Source Engine serverside anti-cheat plugin. (CS:S, CS:GO).

___
# Goal

The goal is to make an anti-cheat that can be used with any type of gameservers.


* Because it can be used in competition it must be fast, consistent and safe.

* And because it can be used with others servers it must also be configurable, easy to use and open-source.


Supported games will be [CS:S](http://store.steampowered.com/app/240), [CS:GO](http://store.steampowered.com/app/730), [CS:P](http://cspromod.com/), [Black Mesa](http://store.steampowered.com/app/362890) and maybe more.

Currently [CS:S](http://store.steampowered.com/app/240) is the base of this project.

> * This plugin is not safe (And *never* has been).
> * Features and safety are not representative of what it should be at the end.

___

* **Current project status** : [Fixing](https://github.com/L-EARN/NoCheatZ-4/issues)
* **Last known stable commit** : https://github.com/L-EARN/NoCheatZ-4/commit/b2ba18f4a68a40e4762501c9a048c6906bf042ed

___
# Contributing

___
## Installing a gameserver

See https://developer.valvesoftware.com/wiki/SteamCMD

* It is recommanded, with Windows, to install your server into his own directory.
The Visual Studio project launches a commandfile (to auto-update the server before running the debugger) that expects steamcmd to be installed at C:\steamcmd\steamcmd.exe

* For Linux, please make a new user using adduser. You will also have to copy the files manually to your server after each builds.

```bash
login anonymous
force_install_dir C:\steamcmd\CSS
app_update 232330 validate
```

___
## Cloning the git

su to your srcds user.

```git
git clone git://github.com/L-EARN/NoCheatZ-4.git
cd NoCheatZ-4
git submodule update --init
```

___
## Compiling the project

___
### With Linux

1. su to root.

2. Install your compiler tools :

```sh
dpkg --add-architecture i386
apt-get update
apt-get install build-essential gcc-multilib g++-multilib ia32-libs lib32gcc1 libc6-i386 libc6-dev-i386 autotools-dev autoconf libtool gdb screen
```

3. su to your srcds user.

4. With your console, go to the "server-plugin" directory and type `make debug`

The binary and all other stuff will go in NoCheatZ-4/Builds/[CONFIG]/addons/NoCheatZ

You can copy-paste the entire content of the Builds directory into the root directory of your gameserver.
(e.g. `cp /R ./NoCheatZ-4/Builds/Release/* /home/user/steamcmd/CSS/cstrike` )

5. If you want to run the server using gdb, create a script in steamcmd/CSS with something like this :

```sh
#!/bin/sh

export LD_LIBRARY_PATH=".:bin:$LD_LIBRARY_PATH"
gdb --args ./srcds_linux -console -game cstrike -steam_dir ../ -steamcmd_script ../steamcmd.sh -insecure +map de_dust2 +rcon_password cderfv
```

___
### With Windows

1. Make sure you installed steamcmd at C:\steamcmd\steamcmd.exe

2. Download and extract [libprotobuf src 2.5.0](https://github.com/google/protobuf/archive/v2.5.0.zip) in server-plugin\SourceSdk\Interfaces\Protobuf\

3. Open the solution with Visual Studio 2015

4. Choose your configuration.

5. Click build. 
         
___		 
### With Mac

* You need to setup the makefile to use Clang, and also implement parts of code in the project that are not already translated for OSX.

> I don't eat apples so I can't really do it myself ...

___
## What to do with the code ?

* Whatever you want but first make sure to have your own branch a/o fork.

* Feel free to report using Issues if you see anything you think is not good.
