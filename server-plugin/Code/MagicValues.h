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

#ifndef MAGICVALUES_H
#define MAGICVALUES_H

#ifndef MUTE_INCLUDES_IN_HEADERS

#	include <stdint.h>

#	ifndef NCZ_PREPROCESSORS
#		include "Preprocessors.h"
#	endif

#endif

#define PLAYERCLASS_PROP "CCSPlayer"

/* OTHERS */

#ifndef EVENT_DEBUG_ID_INIT
#	define EVENT_DEBUG_ID_INIT 42
#endif
#ifndef EVENT_DEBUG_ID_SHUTDOWN
#	define EVENT_DEBUG_ID_SHUTDOWN 13
#endif

#define GET_PLUGIN_COMMAND_INDEX() CNoCheatZPlugin::GetInstance()->GetCommandIndex()+1
#define PLUGIN_MIN_COMMAND_INDEX   1
#define PLUGIN_MAX_COMMAND_INDEX   64
#define FIRST_PLAYER_ENT_INDEX		1
#define LAST_PLAYER_ENT_INDEX		66

#define FORMAT_STRING_BUFFER_SIZE	4096

#define STEAMID_MAXCHAR 32

#define MAX_PLAYERS 66

#define ANTIFLOOD_LOGGING_TIME 4.5f

/*
	When the logging system takes more than this size (bytes) then force it to flush.
	The better is to have a low value that will not make the server hangs
	but also a high value that will not make it lag in case there is a high amount
	of messages to be logged per second.
*/
#define LOGGER_FORCE_FLUSH_MAX_MEMORY 65536

namespace SystemPriority
{
	namespace UserCmdHookListener
	{
		/*
			Some HookListener classes can sort listeners by priority.
			This is useful here for instance when we want BadUserCmdBlocker to sanitize data before testers can actually test data.
			We also basically want to block after doing tests otherwise tests would be bypassed if the blocker decides to inert.

			Now you understand that lower priority values are run before higher priority values.
		*/
		size_t const constexpr BadUserCmdBlocker ( 0 );
		size_t const constexpr SpeedTester ( BadUserCmdBlocker + 1 );
		size_t const constexpr EyeAnglesTester ( SpeedTester + 1 );
		size_t const constexpr AutoAttackTester ( EyeAnglesTester + 1 );
		size_t const constexpr ShotTester ( AutoAttackTester + 1 );
		size_t const constexpr JumpTester ( ShotTester + 1 );
		size_t const constexpr BhopBlocker ( JumpTester + 1 );
	}
}

namespace UniqueDetectionID
{
	/*
		We attribute a unique detection id for each childs of BaseDetection ( LogDetection ) just for faster sorting and memory compression of detections at runtime.
		See the comments in temp_BaseDetection.h
	*/
	uint32_t const constexpr AUTOATTACK ( 0 );
	uint32_t const constexpr CMD_FLOOD ( 1 );
	uint32_t const constexpr CMD_VIOLATION ( 2 );
	uint32_t const constexpr CONVAR_TIMEOUT (4 );
	uint32_t const constexpr CONVAR_WRONGVALUE ( 8 );
	uint32_t const constexpr CONVAR_ILLEGAL ( 16 );
	uint32_t const constexpr EYEANGLE_X ( 32 );
	uint32_t const constexpr EYEANGLE_Y ( 64 );
	uint32_t const constexpr EYEANGLE_Z ( 128 );
	uint32_t const constexpr BUNNYHOP_JUMPMACRO ( 256 );
	uint32_t const constexpr BUNNYHOP_PERFECT ( 512 );
	uint32_t const constexpr SPEEDHACK ( 1024 );
}


#endif // MAGICVALUES_H
