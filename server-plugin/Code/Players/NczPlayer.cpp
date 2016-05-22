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

#include "NczPlayerManager.h" // + NczPlayer, needed to invalidate handlers on kick

#include "Maths/Vector.h"
#include "Interfaces/bspflags.h"
#include "Interfaces/IEngineTrace/IEngineTrace.h"

#include "Misc/Helpers.h" // PEntityOfEntIndex, ifaces
#include "Misc/temp_Metrics.h"
#include "Hooks/PlayerRunCommandHookListener.h"

//---------------------------------------------------------------------------------
// NczPlayer
//---------------------------------------------------------------------------------

NczPlayer::NczPlayer(const int index) : cIndex(index), m_time_connected(0.0)
{
	m_edict = Helpers::PEntityOfEntIndex(index);
	m_userid = SourceSdk::InterfacesProxy::Call_GetPlayerUserid(m_edict);
	m_channelinfo = SourceSdk::InterfacesProxy::Call_GetPlayerNetInfo(index);
}

int NczPlayer::GetUserid() const
{
	return m_userid;
}

float NczPlayer::GetTimeConnected() const
{
	return Plat_FloatTime() - m_time_connected;
}

void * NczPlayer::GetPlayerInfo() const
{
	return SourceSdk::InterfacesProxy::Call_GetPlayerInfo(m_edict);
}

const char * NczPlayer::GetName() const
{
	if (Helpers::isValidEdict(m_edict))
	{
		if (GetPlayerInfo())
		{
			return static_cast<SourceSdk::IPlayerInfo*>(GetPlayerInfo())->GetName();
		}
	}
	return "";
}

const char * NczPlayer::GetSteamID() const
{
	return SourceSdk::InterfacesProxy::Call_GetPlayerNetworkIDString(m_edict);
}

const char * NczPlayer::GetIPAddress() const
{
	return GetChannelInfo()->GetAddress();
}

WpnShotType NczPlayer::GetWpnShotType() const
{
	if (GetPlayerInfo() == nullptr) return HAND;

	char const * wpn_name;
	if (SourceSdk::InterfacesProxy::m_game == SourceSdk::CounterStrikeGlobalOffensive)
	{
		wpn_name = static_cast<SourceSdk::IPlayerInfo_csgo*>(GetPlayerInfo())->GetWeaponName();
	}
	else
	{
		wpn_name = static_cast<SourceSdk::IPlayerInfo*>(GetPlayerInfo())->GetWeaponName();
	}
		
	if(!wpn_name || ! *wpn_name) return HAND;


	switch (SourceSdk::InterfacesProxy::m_game)
	{
	case SourceSdk::CounterStrikeSource:
	{
		wpn_name = static_cast<SourceSdk::IPlayerInfo*>(GetPlayerInfo())->GetWeaponName();
		if (!wpn_name || !*wpn_name) return HAND;
		switch (wpn_name[7])
		{
		case 's': //scout & sg550 & smokegrenade
			switch (wpn_name[8])
			{
			case 'c':
				return PISTOL;
			case 'g':
				return AUTO;

			default: //m
				return HAND;
			};

		case 'a': //awp & ak47
			switch (wpn_name[8])
			{
			case 'w':
				return PISTOL;
			case 'k':
				return AUTO;
			};
		case 'f': //fiveseven & flashbang
			switch (wpn_name[8])
			{
			case 'i':
				return PISTOL;
			default: //l
				return HAND;
			};
		case 'g': //glock & g3sg1
			switch (wpn_name[8])
			{
			case 'l':
				return PISTOL;
			case '3':
				return AUTO;
			};
		case 'p': //p228
		case 'e': //elite
		case 'u': //usp
		case 'd': //deagle
		case 'x': //xm1014
			return PISTOL;
		case 'm': //m3 & m4a1
			switch (wpn_name[8])
			{
			case '3':
				return PISTOL;
			case '4':
				return AUTO;
			};

		default:
			return HAND;
		};
		break;
	}
	case SourceSdk::CounterStrikeGlobalOffensive:
	{
		wpn_name = static_cast<SourceSdk::IPlayerInfo_csgo*>(GetPlayerInfo())->GetWeaponName();
		if (!wpn_name || !*wpn_name) return HAND;
		switch (wpn_name[7])
		{
		case 'a':
			switch (wpn_name[8])
			{
			case 'k':  // weapon_ak47
				return AUTO;
			case 'u':  // weapon_aug
				return AUTO;
			case 'w':  // weapon_awp
				return PISTOL;
			default:
				return HAND;
			};
		case 'b': // weapon_bizon
			return AUTO;
		case 'c':
			switch (wpn_name[8])
			{
			case '4':  // weapon_c4
				return HAND;
			case 'z':  // weapon_cz75a
				return PISTOL;
			default:
				return HAND;
			};
		case 'd':
			switch (wpn_name[9])
			{
			case 'a':  // weapon_deagle
				return PISTOL;
			case 'c':  // weapon_decoy
				return HAND;
			default:
				return HAND;
			};
		case 'e': // weapon_elite
			return PISTOL;
		case 'f':
			switch (wpn_name[8])
			{
			case 'a':  // weapon_famas
				return AUTO;
			case 'i':  // weapon_fiveseven
				return PISTOL;
			case 'l':  // weapon_flashbang
				return HAND;
			default:
				return HAND;
			};
		case 'g':
			switch (wpn_name[8])
			{
			case '3':  // weapon_g3sg1
				return AUTO;
			case 'a': // weapon_galil & weapon_galilar
				return AUTO;
			case 'l': // weapon_glock
				return PISTOL;
			default:
				return HAND;
			};
		case 'h':
			switch (wpn_name[8])
			{
			case 'e':  // weapon_hegrenade
				return HAND;
			case 'k': // weapon_hkp2000
				return PISTOL;
			default:
				return HAND;
			};
		case 'i': // weapon_incgrenade
			return HAND;
		case 'k': // weapon_knife
			return HAND;
		case 'm':
			switch (wpn_name[8])
			{
			case '2': // weapon_m249
				return AUTO;
			case '3': // weapon_m3
				return AUTO;
			case '4': // wweapon_m4a1 & weapon_m4a1_silencer
				return AUTO;
			case 'a':
				switch (wpn_name[9])
				{
				case 'g':  // weapon_mag7
					return PISTOL;
				case 'c':  // weapon_mac10
					return AUTO;
				default:
					return HAND;
				};
			case 'o': // weapon_molotov
				return HAND;
			case 'p':
				switch (wpn_name[9])
				{
				case '5':  // weapon_mp5navy
					return AUTO;
				case '7':  // weapon_mp7
					return AUTO;
				case '9': // weapon_mp9 
					return AUTO;
				default:
					return HAND;
				};
			default:
				return HAND;
			};
		case 'n':
			switch (wpn_name[8])
			{
			case '2': // weapon_nova
				return PISTOL;
			case '3': // weapon_negev
				return AUTO;
			default:
				return HAND;
			};
		case 'p':
			switch (wpn_name[9])
			{
			case '2': // weapon_p228
				return PISTOL;
			case '5': // weapon_p250
				return PISTOL;
			case '0': // weapon_p90 
				return AUTO;
			default:
				return HAND;
			};
		case 'r': // weapon_revolver
			return PISTOL;
		case 's':
			switch (wpn_name[8])
			{
			case 'a': // weapon_sawedoff
				return PISTOL;
			case 'c':
				switch (wpn_name[9])
				{
				case 'a': // weapon_scar17 & weapon_scar20
					return AUTO;
				case 'o': // weapon_scout
					return PISTOL;
				default:
					return HAND;
				};
			case 'g':
				switch (wpn_name[11])
				{
				case '0': // weapon_sg550
					return AUTO;
				case '2': // weapon_sg552
					return AUTO;
				case '6': // weapon_sg556
					return AUTO;
				default:
					return HAND;
				};
			case 's': // weapon_ssg08
				return PISTOL;
			case 'm': // weapon_smokegrenade 
				return HAND;
			default:
				return HAND;
			};
		case 't':
			switch (wpn_name[8])
			{
			case 'a': // weapon_taser
				return HAND;
			case 'e': // weapon_tec9
				return PISTOL;
			case 'm': // weapon_tmp
				return AUTO;
			default:
				return HAND;
			};
		case 'u':
			switch (wpn_name[8])
			{
			case 'm': // weapon_ump45
				return AUTO;
			case 's': // weapon_usp & weapon_usp_silencer
				return PISTOL;
			default:
				return HAND;
			};
		case 'x': // weapon_xm1014
			return AUTO;
		default:
			return HAND;
		};
		break;
	default:
		break;
	}
	};
	return HAND;
}

int NczPlayer::aimingAt()
{
	SourceSdk::CTraceFilterWorldAndPropsOnly filter;

	SourceSdk::edict_t* edict = GetEdict();
	if(!edict) return -1;

	SourceSdk::Vector eyePos;
	SourceSdk::InterfacesProxy::Call_ClientEarPosition(edict, &eyePos);

	SourceSdk::Vector vEnd;
	AngleVectors(PlayerRunCommandHookListener::GetLastUserCmd(this)->viewangles, &vEnd);
	VectorMultiply(vEnd, 8192.0f);
	VectorAdd(eyePos, vEnd);

	SourceSdk::edict_t* target;
	bool allsolid;
	if (SourceSdk::InterfacesProxy::m_game == SourceSdk::CounterStrikeGlobalOffensive)
	{
		static SourceSdk::CGameTrace_csgo trace;
		static SourceSdk::Ray_t_csgo ray;
		ray.Init(eyePos, vEnd);
		SourceSdk::InterfacesProxy::Call_TraceRay((void*)&ray, MASK_VISIBLE | CONTENTS_HITBOX, &filter, (void*)&trace);
		target = SourceSdk::InterfacesProxy::Call_BaseEntityToEdict(trace.m_pEnt);
		allsolid = trace.allsolid;
	}
	else
	{
		static SourceSdk::CGameTrace trace;
		static SourceSdk::Ray_t ray;
		ray.Init(eyePos, vEnd);
		SourceSdk::InterfacesProxy::Call_TraceRay((void*)&ray, MASK_VISIBLE | CONTENTS_HITBOX, &filter, (void*)&trace);
		target = SourceSdk::InterfacesProxy::Call_BaseEntityToEdict(trace.m_pEnt);
		allsolid = trace.allsolid;
	}
	
	if ( target && !Helpers::IndexOfEdict(target) == 0 && !allsolid)
	{
		if(!Helpers::isValidEdict(target)) return -1;
#undef GetClassName
		if(strcmp(target->GetClassName(), "player") == 0)
		{
			SourceSdk::IPlayerInfo* const targetinfo = static_cast<SourceSdk::IPlayerInfo*>(SourceSdk::InterfacesProxy::Call_GetPlayerInfo(target));
			if(targetinfo)
			{
				const int ta = targetinfo->GetTeamIndex();
				const int tb = static_cast<SourceSdk::IPlayerInfo*>(SourceSdk::InterfacesProxy::Call_GetPlayerInfo(edict))->GetTeamIndex();
				if( ta != tb )
				{
					if( targetinfo->IsPlayer() && !targetinfo->IsHLTV() && !targetinfo->IsObserver() )
					{
						return Helpers::IndexOfEdict(target);
					}
				} 
			}
		}
	}
	return -1;
}

SourceSdk::INetChannelInfo* NczPlayer::GetChannelInfo() const
{
	return m_channelinfo;
}

SourceSdk::edict_t * NczPlayer::GetEdict() const
{
	return m_edict;
}

bool NczPlayer::isValidEdict()
{
	return Helpers::isValidEdict(m_edict);
}

void NczPlayer::OnConnect()
{
	m_time_connected = Plat_FloatTime();
}

basic_string NczPlayer::GetReadableIdentity()
{
	if(SteamGameServer_BSecure())
	{
		return Helpers::format("%s [%s - %s]", this->GetName(), this->GetSteamID(), this->GetIPAddress());
	}
	else
	{
		return Helpers::format("%s [%s]", this->GetName(), this->GetIPAddress());
	}
}

void NczPlayer::Kick(const char * msg)
{
	NczPlayerManager::GetInstance()->GetPlayerHandlerByIndex(cIndex)->status = KICK;
	Helpers::writeToLogfile(Helpers::format(
				"Kicked %s with reason : %s\n", this->GetReadableIdentity().c_str(), msg));

	SourceSdk::InterfacesProxy::Call_ServerCommand(
				Helpers::format("kickid %d [NoCheatZ 4] %s\n", this->GetUserid(), msg).c_str()
				);
}

void NczPlayer::Ban(const char * msg, int minutes)
{
	Helpers::writeToLogfile(Helpers::format(
				"Banned %s with reason : %s\n", this->GetReadableIdentity().c_str(), msg));
	{
		if(SteamGameServer_BSecure())
		{
			SourceSdk::InterfacesProxy::Call_ServerCommand(Helpers::format("banid %d %s\n", minutes, GetSteamID()).c_str());
			SourceSdk::InterfacesProxy::Call_ServerCommand("writeid\n");
		}
		Kick(msg);
		if(!Helpers::bStrEq("127.0.0.1", GetIPAddress()))
		{
			SourceSdk::InterfacesProxy::Call_ServerCommand(Helpers::format("addip 1440 \"%s\"\n", GetIPAddress()).c_str());
			SourceSdk::InterfacesProxy::Call_ServerCommand("writeip\n");
		}
	}
}
