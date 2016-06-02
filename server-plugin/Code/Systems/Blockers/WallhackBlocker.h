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

#ifndef WALLHACKBLOCKER_H
#define WALLHACKBLOCKER_H

#include "Interfaces/IGameEventManager/IGameEventManager.h"
#include "Maths/Vector.h"

#include "Players/temp_PlayerDataStruct.h"
#include "Hooks/SetTransmitHookListener.h"
#include "Hooks/WeaponHookListener.h"
#include "Systems/BaseSystem.h"
#include "Systems/OnTickListener.h"
#include "Misc/temp_singleton.h"

enum SpectatorMode {
	OBS_MODE_NONE = 0,
	OBS_MODE_DEATHCAM,
	OBS_MODE_FREEZECAM,
	OBS_MODE_FIXED,
	OBS_MODE_IN_EYE,
	OBS_MODE_CHASE,
	OBS_MODE_ROAMING,
};

struct ClientDataS
{
	SourceSdk::Vector bbox_min;
	SourceSdk::Vector bbox_max;
	SourceSdk::Vector abs_origin;
	SourceSdk::Vector ear_pos;
	SourceSdk::QAngle eye_angles;

	inline ClientDataS()
	{
		//bbox_min = bbox_max = abs_origin = ear_pos = Vector();
	};
	inline ClientDataS(const ClientDataS& other)
	{
		memcpy(this, &other, sizeof(ClientDataS));
	};
};

struct ALIGN4 VisInfo
{
	bool m_visible;
	bool m_valid;

	inline VisInfo()
	{
		m_visible = m_valid = true;
	};
	inline VisInfo(const VisInfo& other)
	{
		m_valid = other.m_valid;
		m_visible = other.m_visible;
	};
	inline VisInfo(bool const visibility)
	{
		m_valid = true;
		m_visible = visibility;
	};
} ALIGN4_POST;

class VisCache
{
private:
	VisInfo m_cache[MAX_PLAYERS][MAX_PLAYERS];

	VisCache(VisCache const & other){};
	VisCache& operator=(VisCache const & other){};

public:
	inline VisCache()
	{
		Invalidate();
	};
	inline ~VisCache()
	{
		Invalidate();
	};

	inline void Invalidate()
	{
		memset(this, 0, sizeof(VisCache));
	};

	inline bool IsValid(NczPlayer const * const pa, NczPlayer const * const pb) const
	{
		Assert(pa && pb);
		Assert(pa != pb);
		return m_cache[pa->GetIndex()][pb->GetIndex()].m_valid;
	};

	inline bool IsVisible(NczPlayer const * const pa, NczPlayer const * const pb) const
	{
		Assert(pa && pb);
		Assert(pa != pb);
		return m_cache[pa->GetIndex()][pb->GetIndex()].m_visible;
	};

	inline void SetVisibility(NczPlayer const * const pa, NczPlayer const * const pb, bool const visibility)
	{
		Assert(pa && pb);
		Assert(pa != pb);
		m_cache[pa->GetIndex()][pb->GetIndex()] = VisInfo(visibility);
	};
};

class WallhackBlocker :
	private BaseSystem,
	public PlayerDataStructHandler<ClientDataS>,
	private SetTransmitHookListener,
	private WeaponHookListener,
	private OnTickListener,
	public Singleton<WallhackBlocker>
{
	typedef Singleton<WallhackBlocker> singleton_class;
	typedef PlayerDataStructHandler<ClientDataS> playerdatahandler_class;

public:
	WallhackBlocker();
	~WallhackBlocker();

	void ClientDisconnect(SourceSdk::edict_t* const client);

private:
	void Init();
	void Load();
	void Unload();

	void ProcessOnTick(float const curtime);
	void ProcessPlayerTestOnTick(NczPlayer* const player, float const curtime){};

	bool SetTransmitCallback(SourceSdk::edict_t* const sender, SourceSdk::edict_t* const receiver);
	bool SetTransmitWeaponCallback(SourceSdk::edict_t* const sender, SourceSdk::edict_t* const receiver);
	void WeaponEquipCallback(NczPlayer* player, SourceSdk::edict_t* const weapon);
	void WeaponDropCallback(NczPlayer* player, SourceSdk::edict_t* const weapon);

	bool IsAbleToSee(NczPlayer* const sender, NczPlayer* const receiver);

private:
	NczPlayer* m_weapon_owner[MAX_EDICTS];
	VisCache m_viscache;

	bool IsInFOV(const SourceSdk::Vector& origin, const SourceSdk::QAngle& dir, const SourceSdk::Vector& target);
	bool IsVisible(const SourceSdk::Vector& origin, const SourceSdk::Vector& target);
	bool IsVisible(const SourceSdk::Vector& origin, const SourceSdk::QAngle& dir, const SourceSdk::Vector& target);
	bool IsVisible(const SourceSdk::Vector& origin, const SourceSdk::Vector& target, const SourceSdk::Vector& mins, const SourceSdk::Vector& maxs, const SourceSdk::vec_t scale = 1.0);

};

#endif // WALLHACKBLOCKER_H
