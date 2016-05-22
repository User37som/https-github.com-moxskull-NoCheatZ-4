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

/* HOOKS */

/* https://www.sourcemodplugins.org/vtableoffsets

	CBasePlayer::NetworkStateChanged_m_hGroundEntity(void*)
	CCSPlayer::PlayerRunCommand(CUserCmd*, IMoveHelper*)
	CBaseCombatCharacter::SetTransmit(CCheckTransmitInfo*, bool)
	CBaseFlex::Teleport(Vector const*, QAngle const*, Vector const*)
	CCSPlayer::Weapon_Equip(CBaseCombatWeapon*)
	CBasePlayer::Weapon_Drop(CBaseCombatWeapon*, Vector const*, Vector const*)
	CCSPlayer::GetDataDescMap()
*/

#ifdef NCZ_CSS
#	ifdef GNUC
#		define DEFAULT_DISPATCH_OFFSET 13
#		define DEFAULT_GROUND_OFFSET "179"
#		define DEFAULT_RUNCOMMAND_OFFSET "420"
#		define DEFAULT_SETTRANSMIT_OFFSET "21"
#		define DEFAULT_TELEPORT_OFFSET "109"
#		define DEFAULT_WEAPONEQUIP_OFFSET "262"
#		define DEFAULT_WEAPONDROP_OFFSET "264"
#		define DEFAULT_GETDATADESCMAP_OFFSET "12"
#	else
#		define DEFAULT_DISPATCH_OFFSET 12
#		define DEFAULT_GROUND_OFFSET "177"
#		define DEFAULT_RUNCOMMAND_OFFSET "419"
#		define DEFAULT_SETTRANSMIT_OFFSET "20"
#		define DEFAULT_TELEPORT_OFFSET "108"
#		define DEFAULT_WEAPONEQUIP_OFFSET "261"
#		define DEFAULT_WEAPONDROP_OFFSET "263"
#		define DEFAULT_GETDATADESCMAP_OFFSET "11"
#	endif
#endif
#ifdef NCZ_CSGO
#	ifdef GNUC
#		define DEFAULT_DISPATCH_OFFSET 15
#		define DEFAULT_GROUND_OFFSET "176"
#		define DEFAULT_RUNCOMMAND_OFFSET "469"
#		define DEFAULT_SETTRANSMIT_OFFSET "23"
#		define DEFAULT_TELEPORT_OFFSET "114"
#		define DEFAULT_WEAPONEQUIP_OFFSET "282"
#		define DEFAULT_WEAPONDROP_OFFSET "284"
#		define DEFAULT_GETDATADESCMAP_OFFSET "12"
#	else
#		define DEFAULT_DISPATCH_OFFSET 14
#		define DEFAULT_GROUND_OFFSET "176"
#		define DEFAULT_RUNCOMMAND_OFFSET "468"
#		define DEFAULT_SETTRANSMIT_OFFSET "22"
#		define DEFAULT_TELEPORT_OFFSET "113"
#		define DEFAULT_WEAPONEQUIP_OFFSET "281"
#		define DEFAULT_WEAPONDROP_OFFSET "283"
#		define DEFAULT_GETDATADESCMAP_OFFSET "11"
#	endif
#endif

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

#endif // MAGICVALUES_H
