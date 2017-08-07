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

#ifndef NCZ_PREPROCESSORS
#	include "Preprocessors.h"
#endif

#define PLAYERCLASS_PROP "CCSPlayer"

/* OTHERS */

#ifndef EVENT_DEBUG_ID_INIT
#	define EVENT_DEBUG_ID_INIT 42
#endif
#ifndef EVENT_DEBUG_ID_SHUTDOWN
#	define EVENT_DEBUG_ID_SHUTDOWN 13
#endif

#define GET_PLUGIN_COMMAND_INDEX() g_CNoCheatZPlugin.GetCommandIndex()+1
#define PLUGIN_MIN_COMMAND_INDEX   1
#define PLUGIN_MAX_COMMAND_INDEX   64
#define FIRST_PLAYER_ENT_INDEX		1
#define LAST_PLAYER_ENT_INDEX		66

#define FORMAT_STRING_BUFFER_SIZE	32768

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
	size_t const constexpr EyeAnglesTester(0);
	size_t const constexpr MouseTester(EyeAnglesTester + 1);
	size_t const constexpr BadUserCmdTester(MouseTester + 1);
	size_t const constexpr AutoStrafeTester(BadUserCmdTester + 1);
	size_t const constexpr SpeedTester(AutoStrafeTester + 1);
	size_t const constexpr AutoAttackTester(SpeedTester + 1);
	size_t const constexpr AimTester(AutoAttackTester + 1);
	size_t const constexpr JumpTester(AimTester + 1);
	size_t const constexpr BhopBlocker(JumpTester + 1);
	size_t const constexpr AntiFlashbangBlocker(BhopBlocker + 1);
	size_t const constexpr AntiSmokeBlocker(AntiFlashbangBlocker + 1);
	size_t const constexpr WallhackBlocker(AntiSmokeBlocker + 1);
	size_t const constexpr RadarHackBlocker(WallhackBlocker + 1);
	size_t const constexpr ConVarTester(RadarHackBlocker + 1);
	size_t const constexpr ConCommandTester(ConVarTester + 1);
	size_t const constexpr SpamChangenameTester(ConCommandTester + 1);
	size_t const constexpr SpamConnectTester(SpamChangenameTester + 1);
}

#endif // MAGICVALUES_H
