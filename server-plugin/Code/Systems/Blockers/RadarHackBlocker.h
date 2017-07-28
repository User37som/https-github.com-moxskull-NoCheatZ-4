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

#ifndef RADARHACKBLOCKER_H
#define RADARHACKBLOCKER_H

#include "Interfaces/IGameEventManager/IGameEventManager.h"
#include "Containers/utlvector.h"
#include "Maths/Vector.h"

#include "Players/temp_PlayerDataStruct.h"
#include "Systems/BaseSystem.h"
#include "Systems/OnTickListener.h"
#include "Misc/temp_singleton.h"
#include "Hooks/ThinkPostHookListener.h"
#include "Misc/MRecipientFilter.h"
#include "Hooks/UserMessageHookListener.h"

enum
{
	TEAM_NONE = 0,
	TEAM_SPECTATOR,
	TEAM_1,
	TEAM_2
};

struct ClientRadarData
{
	bool m_last_spotted_status;
	int m_origin_index;

	int m_team;
	//float m_next_update;

	ClientRadarData ()
	{
		memset ( this, 0, sizeof ( ClientRadarData ) );
	}
	ClientRadarData ( int x ) :
		m_last_spotted_status(false),
		m_origin_index(x),
		m_team(0)
	{
	}

	ClientRadarData ( ClientRadarData const & other )
	{
		memcpy ( this, &other, sizeof ( ClientRadarData ) );
	}

	ClientRadarData& operator=( ClientRadarData const & other )
	{
		memcpy ( this, &other, sizeof ( ClientRadarData ) );
		return *this;
	}
};

class RadarHackBlocker :
	public BaseBlockerSystem,
	public OnTickListener,
	public PlayerDataStructHandler<ClientRadarData>,
	public Singleton,
	public ThinkPostHookListener,
	public UserMessageHookListener
{
	typedef PlayerDataStructHandler<ClientRadarData> playerdatahandler_class;

private:
	bool* m_players_spotted;

	bool* m_bomb_spotted;

	void* m_cvar_forcecamera;

	double m_next_process;

public:
	RadarHackBlocker ();
	virtual ~RadarHackBlocker () final;

private:
	virtual void Init () override final;

	virtual void Load () override final;

	virtual void Unload () override final;

	virtual bool GotJob () const override final;

	virtual void RT_ProcessOnTick (double const & curtime ) override final;

	virtual void RT_ThinkPostCallback ( SourceSdk::edict_t const * const pent ) override final;

	virtual bool RT_SendUserMessageCallback ( SourceSdk::IRecipientFilter const &, int const, google::protobuf::Message const & ) override final;

	virtual bool RT_UserMessageBeginCallback ( SourceSdk::IRecipientFilter const * const, int const ) override final;

	virtual void RT_MessageEndCallback(SourceSdk::IRecipientFilter const * const, int const, SourceSdk::bf_write* buffer) override final;

public:
	void OnMapStart ();

private:
	void RT_SendApproximativeRadarUpdate ( MRecipientFilter & filter, ClientRadarData const * data ) const;

	void RT_SendRandomRadarUpdate ( MRecipientFilter & filter, ClientRadarData const * data ) const;

	void RT_ProcessEntity ( SourceSdk::edict_t const * const pent );

	void RT_UpdatePlayerData ( NczPlayer* player );
};

extern RadarHackBlocker g_RadarHackBlocker;

#endif // RADARHACKBLOCKER_H
