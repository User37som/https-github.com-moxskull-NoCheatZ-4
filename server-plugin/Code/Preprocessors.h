/*
	Copyright 2012 - Le Padellec Sylvain

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at
	   http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*/


/*
	This file makes it easier to cross-compile between all the games and all the os they use.
	Because I use Makefile with Linux and VisualStudio Compiler with Windows, I think it is easier to have only one place to make sure everything works everywhere for anything ...
	It may be included everywhere.
*/

#ifndef NCZ_PREPROCESSORS
#define NCZ_PREPROCESSORS

#include "version.h"

#ifdef GNUC
#	include <cstdlib>
#endif

#ifdef GNUC
#	define __assume(cond) if (!(cond)) __builtin_unreachable()
#	define __unreachable() __builtin_unreachable()
#else
#	define __unreachable() __assume(0)
#endif

#define SafePtrDeref(x) (x != 0 ? *x : 0)

#define NCZ_PLUGIN_NAME			"NoCheatZ" // Name of the plugin

#define IN_ATTACK (1 << 0)
#define IN_ATTACK2	(1 << 11)
#define IN_JUMP   (1 << 1)

/*
	Use for backward compatibility.
	EP1 (Older version of Source Engine) = Counter-Strike : Promod
*/
#ifdef NCZ_EP1
#	define GET_ARGV(a) engine->Cmd_Argv(a)
#	define GET_ARGS engine->Cmd_Args()
#	define GET_ARGC engine->Cmd_Argc()
#	define FINDCOMMAND(a) g_pCVar->GetCommands()->FindCommand(a)
#	define IsFlagSet IsBitSet
#else
#	define GET_ARGV(a) args.Arg(a)
#	define GET_ARGC args.ArgC()
#	define GET_ARGS args.GetCommandString()
#	define FINDCOMMAND(a) g_pCVar->FindCommand(a)
#endif

/*
	gcc is f*cking strict ...
*/
#ifdef GNUC
#	include <cstring>
#	define strcpy_s(a, b, c) strncpy(a, c, b)
#ifndef nullptr
#	define nullptr 0
#endif
typedef unsigned long DWORD;
#endif

#ifdef WIN32
#	include <tchar.h>
#endif

#undef GetClassName

/*
	Conventionnal rules to set local variables to static (To avoid pushing temporary local variables to stack and saving some iops) :
	I'm a setting this rule just to make the code easier to port into multi threads.
	You must avoid putting static everywhere though.

	MT -> Is multi thread safe even if static, or is set to atomic.
	ST -> Only safe in single thread mode (Server side code is single thread).
	R -> Is a read only variable
	W -> Is a read/write variable.
*/

/*
#define MT_R_STATIC static const  // Must be atomic, currently it isn't
#define ST_R_STATIC static const
#define MT_W_STATIC static // Must be atomic, currently it isn't
#define ST_W_STATIC static
*/
#define MT_R_STATIC
#define ST_R_STATIC
#define MT_W_STATIC
#define ST_W_STATIC

#ifndef MAGICVALUES_H
#	include "MagicValues.h"
#endif

#endif

