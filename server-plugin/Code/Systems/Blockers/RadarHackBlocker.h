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

	SourceSdk::Vector m_origin;
	SourceSdk::vec_t m_yawangle;
	int m_team;
	float m_next_update;
};

class RadarHackBlocker :
	public BaseSystem,
	public PlayerDataStructHandler<ClientRadarData>,
	public ThinkPostHookListener,
	public UserMessageHookListener,
	public OnTickListener,
	public Singleton<RadarHackBlocker>
{
	typedef Singleton<RadarHackBlocker> singleton_class;
	typedef PlayerDataStructHandler<ClientRadarData> playerdatahandler_class;

public:
	RadarHackBlocker();
	~RadarHackBlocker();

	void OnMapStart();

private:
	void Init();
	void Load();
	void Unload();

	void ThinkPostCallback(SourceSdk::edict_t const * const pent);

	bool SendUserMessageCallback(SourceSdk::IRecipientFilter &, int, google::protobuf::Message const &);
	
	bool UserMessageBeginCallback(SourceSdk::IRecipientFilter*, int);

	void SendApproximativeRadarUpdate(MRecipientFilter & filter, ClientRadarData const * data) const;

	void SendRandomRadarUpdate(MRecipientFilter & filter, ClientRadarData const * data) const;

	void ProcessEntity(SourceSdk::edict_t const * const pent);

	void UpdatePlayerData(NczPlayer* player);

	void ProcessOnTick() {};

	void ProcessPlayerTestOnTick(NczPlayer* player);

	bool* m_players_spotted;
	bool* m_bomb_spotted;
};

#endif // RADARHACKBLOCKER_H
