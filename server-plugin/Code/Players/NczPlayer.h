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

#include "Interfaces/InterfacesProxy.h" // edict_t, INetChannelInfo, IPlayerInfo

#include "Misc/ClassSpecifications.h" // NoCopy
#include "Misc/HeapMemoryManager.h" // OverrideNew
#include "Misc/Helpers.h" // IsValidEdict
#include "Systems/Testers/Detections/PlayerDetections.h"

extern float Plat_FloatTime ();

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
	PlayerDetections m_detections;

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
	int const aimingAt (); // Retourne index de la cible présente sur le viseur
	
	void GetAbsOrigin ( SourceSdk::Vector & out );

	void GetRelEyePos ( SourceSdk::Vector & out ) const;

	void GetAbsEyePos ( SourceSdk::Vector & out );

	void GetEyeAngles ( SourceSdk::QAngle & out ) const;

	inline float const GetTimeConnected () const;

	inline bool const isValidEdict () const;

	inline PlayerDetections* GetDetectionInfos ();

	void OnConnect ();

	void Kick ( const char * msg = nullptr );
	void Ban ( const char * msg = nullptr, int minutes = 0 );
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

inline PlayerDetections* NczPlayer::GetDetectionInfos ()
{
	return &m_detections;
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

#endif
