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

enum WpnShotType
{
	HAND,
	AUTO,
	PISTOL
};

class NczPlayerManager;

class alignas(16) NczPlayer :
	public HeapMemoryManager::OverrideNew<16>,
	private NoCopy
{
	friend NczPlayerManager;

private:
	int const m_index;
	SourceSdk::edict_t * const m_edict;
	SourceSdk::INetChannelInfo* m_channelinfo;
	SourceSdk::IPlayerInfo * m_playerinfo;
	float m_time_connected;

public:
	NczPlayer ( int const index );
	~NczPlayer ()
	{};

	inline int GetIndex () const;
	inline SourceSdk::edict_t * const GetEdict () const;
	inline SourceSdk::IPlayerInfo * const GetPlayerInfo () const;
	inline SourceSdk::INetChannelInfo * const GetChannelInfo () const;
	inline const char * GetName () const;
	inline const char * GetSteamID () const;
	inline const char * GetIPAddress () const;
	WpnShotType const GetWpnShotType () const;
	int const aimingAt (); // Retourne index de la cible pr�sente sur le viseur

	void GetAbsOrigin ( SourceSdk::Vector & out );

	void GetRelEyePos ( SourceSdk::Vector & out ) const;

	void GetAbsEyePos ( SourceSdk::Vector & out );

	void GetEyeAngles ( SourceSdk::QAngle & out ) const;

	inline basic_string const GetReadableIdentity ();

	inline float const GetTimeConnected () const;

	inline bool const isValidEdict () const;

	void OnConnect ();

	void Kick ( const char * msg = "Kicked by NoCheatZ 4" );
	void Ban ( const char * msg = "Banned by NoCheatZ 4", int minutes = 0 );
};

inline int NczPlayer::GetIndex () const
{
	return m_index;
}

inline SourceSdk::edict_t * const NczPlayer::GetEdict () const
{
	return m_edict;
}

inline SourceSdk::IPlayerInfo * const NczPlayer::GetPlayerInfo () const
{
	return m_playerinfo;
}

inline SourceSdk::INetChannelInfo * const NczPlayer::GetChannelInfo () const
{
	return m_channelinfo;
}

inline bool const NczPlayer::isValidEdict () const
{
	return Helpers::isValidEdict ( m_edict );
}

inline const char * NczPlayer::GetName () const
{
	if( Helpers::isValidEdict ( m_edict ) )
	{
		if( GetPlayerInfo () )
		{
			return m_playerinfo->GetName ();
		}
	}
	return "";
}

inline const char * NczPlayer::GetSteamID () const
{
	return SourceSdk::InterfacesProxy::Call_GetPlayerNetworkIDString ( m_edict );
}

inline const char * NczPlayer::GetIPAddress () const
{
	return GetChannelInfo ()->GetAddress ();
}

inline float const NczPlayer::GetTimeConnected () const
{
	return Plat_FloatTime () - m_time_connected;
}

inline basic_string const NczPlayer::GetReadableIdentity ()
{
	if( SteamGameServer_BSecure () )
	{
		return Helpers::format ( "%s [%s - %s]", this->GetName (), this->GetSteamID (), this->GetIPAddress () );
	}
	else
	{
		return Helpers::format ( "%s [%s]", this->GetName (), this->GetIPAddress () );
	}
}

#endif
