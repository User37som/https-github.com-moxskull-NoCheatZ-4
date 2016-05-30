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

#ifndef USERMSG_H
#define USERMSG_H

enum eUserMsg
{
	MSG_INVALID = -1,
	Geiger = 0,
	Train = 1,
	HudText = 2,
	SayText = 3,
	SayText2 = 4,
	TextMsg = 5,
	HudMsg = 6,
	ResetHUD = 7,
	GameTitle = 8,
	ItemPickup = 9,
	ShowMenu = 10,
	Shake = 11,
	Fade = 12,
	VGUIMenu = 13,
	Rumble = 14,
	CloseCaption = 15,
	SendAudio = 16,
	RawAudio = 17,
	VoiceMask = 18,
	RequestState = 19,
	BarTime = 20,
	Damage = 21,
	RadioText = 22,
	HintText = 23,
	KeyHintText = 24,
	ReloadEffect = 25,
	PlayerAnimEvent = 26,
	AmmoDenied = 27,
	UpdateRadar = 28,
	KillCam = 29,
	MarkAchievement = 30,
	CallVoteFailed = 31,
	VoteStart = 32,
	VotePass = 33,
	VoteFailed = 34,
	VoteSetup = 35,
	SPHapWeapEvent = 36,
	HapDmg = 37,
	HapPunch = 38,
	HapSetDrag = 39,
	HapSetConst = 40,
	HapMeleeContact = 41,
	PlayerStatsUpdate_DEPRECATED = 42,
	AchievementEvent = 43,
	MatchEndConditions = 44,
	MatchStatsUpdate = 45,
	PlayerStatsUpdate = 46
};

#	include <Interfaces/Protobuf/cstrike15_usermessage_helpers.h>

#endif
