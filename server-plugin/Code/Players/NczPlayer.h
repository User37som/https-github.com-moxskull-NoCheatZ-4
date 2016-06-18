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

#ifndef NCZPCLASS
#define NCZPCLASS

#include "Interfaces/InterfacesProxy.h"

#include "Misc/temp_Metrics.h"
#include "Misc/Helpers.h"
#include "Misc/temp_singleton.h"
#include "Misc/temp_basicstring.h"

/* Permet de connaître l'état d'un slot du serveur rapidement */
enum SlotStatus
{
	INVALID = 0, // Slot not used
	KICK, // In process of being kicked or banned
	BOT, // A bot ...
	PLAYER_CONNECTING, // Not a bot, not connected
	PLAYER_CONNECTED, // Connected as spectator or dead
	PLAYER_IN_TESTS // Playing the round and shooting people everywhere like a mad nerd :)
};

enum SlotFilterBehavior
{
	STATUS_EQUAL_OR_BETTER = 0,
	STATUS_BETTER,
	STATUS_STRICT
};

class SlotFilter
{
private:
	SlotStatus const m_status;
	SlotStatus const m_load_status;
	SlotFilterBehavior const m_behavior;

	SlotFilter& operator=(SlotFilter const & other) = delete;

protected:
	SlotFilter(SlotFilter const & other) : m_status(other.m_status), m_load_status(other.m_load_status), m_behavior(other.m_behavior)
	{
	}


	SlotFilter(SlotStatus status, SlotStatus load_filter, SlotFilterBehavior behavior) : m_status(status), m_load_status(load_filter), m_behavior(behavior)
	{
	}

	virtual ~SlotFilter() {};

public:
	virtual inline bool CanProcessThisSlot(SlotStatus const player_slot_status) const
	{
		if (player_slot_status < m_status) return false;
		switch (m_behavior)
		{
		case STATUS_BETTER:
			if (player_slot_status == m_status) return false;
		case STATUS_STRICT:
			if (player_slot_status > m_status) return false;
		case STATUS_EQUAL_OR_BETTER:
			return true;
		default:
			return false;
		};
	}

	virtual SlotStatus GetTargetSlotStatus() const
	{
		return m_status;
	}

	virtual SlotStatus GetLoadFilter() const
	{
		return m_load_status;
	}

	virtual SlotFilterBehavior GetFilterBehavior() const
	{
		return m_behavior;
	}
};

enum WpnShotType
{
	HAND,
	AUTO,
	PISTOL
};

class ALIGN16 NczPlayer : private NoCopy
{
private:
	int const m_index;
	int const m_userid;
	SourceSdk::edict_t * const m_edict;
	SourceSdk::INetChannelInfo* m_channelinfo;
	mutable SourceSdk::IPlayerInfo * m_playerinfo;
	float m_time_connected;

public:
	NczPlayer(int const index);
	~NczPlayer(){};

	void* operator new(size_t i)
	{
		return _mm_malloc(i, 16);
	}

	void operator delete(void* p)
	{
		_mm_free(p);
	}

	inline int GetIndex() const ;
	inline int GetUserid() const;
	inline SourceSdk::edict_t * const GetEdict() const;
	inline SourceSdk::IPlayerInfo * const GetPlayerInfo() const;
	inline SourceSdk::INetChannelInfo * const GetChannelInfo() const;
	inline const char * GetName() const;
	inline const char * GetSteamID() const;
	inline const char * GetIPAddress() const;
	WpnShotType const GetWpnShotType() const;
	int const aimingAt(); // Retourne index de la cible présente sur le viseur

	void GetAbsOrigin(SourceSdk::Vector & out);

	void GetRelEyePos(SourceSdk::Vector & out) const;

	void GetAbsEyePos(SourceSdk::Vector & out);

	void GetEyeAngles(SourceSdk::QAngle & out) const;

	inline basic_string const GetReadableIdentity();

	inline float const GetTimeConnected() const;

	inline bool const isValidEdict() const;

	void OnConnect();

	void Kick(const char * msg = "Kicked by NoCheatZ 4");
	void Ban(const char * msg = "Banned by NoCheatZ 4", int minutes = 0);
} ALIGN16_POST;

inline int NczPlayer::GetIndex() const
{
	return m_index;
}

inline int NczPlayer::GetUserid() const
{
	return m_userid;
}

inline SourceSdk::edict_t * const NczPlayer::GetEdict() const
{
	return m_edict;
}

inline SourceSdk::IPlayerInfo * const NczPlayer::GetPlayerInfo() const
{
	if (m_playerinfo == nullptr)
	{
		m_playerinfo = (SourceSdk::IPlayerInfo *)SourceSdk::InterfacesProxy::Call_GetPlayerInfo(m_edict);
	}
	return m_playerinfo;
}

inline SourceSdk::INetChannelInfo * const NczPlayer::GetChannelInfo() const
{
	return m_channelinfo;
}

inline bool const NczPlayer::isValidEdict() const
{
	return Helpers::isValidEdict(m_edict);
}

inline const char * NczPlayer::GetName() const
{
	if (Helpers::isValidEdict(m_edict))
	{
		if (GetPlayerInfo())
		{
			return m_playerinfo->GetName();
		}
	}
	return "";
}

inline const char * NczPlayer::GetSteamID() const
{
	return SourceSdk::InterfacesProxy::Call_GetPlayerNetworkIDString(m_edict);
}

inline const char * NczPlayer::GetIPAddress() const
{
	return GetChannelInfo()->GetAddress();
}

inline float const NczPlayer::GetTimeConnected() const
{
	return Plat_FloatTime() - m_time_connected;
}

inline basic_string const NczPlayer::GetReadableIdentity()
{
	if (SteamGameServer_BSecure())
	{
		return Helpers::format("%s [%s - %s]", this->GetName(), this->GetSteamID(), this->GetIPAddress());
	}
	else
	{
		return Helpers::format("%s [%s]", this->GetName(), this->GetIPAddress());
	}
}

#endif
