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
#include "Hooks/PlayerRunCommandHookListener.h"
#include "Systems/BaseSystem.h"
#include "Systems/OnTickListener.h"
#include "Misc/temp_singleton.h"

enum SpectatorMode
{
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
	int cmd_tickcount;

	inline ClientDataS ()
	{
		//bbox_min = bbox_max = abs_origin = ear_pos = Vector();
	};
	inline ClientDataS ( const ClientDataS& other )
	{
		memcpy ( this, &other, sizeof ( ClientDataS ) );
	};
};

struct VisInfo
{
	bool m_visible;
	bool m_valid;

	inline VisInfo () :
		m_visible ( true ),
		m_valid ( true )
	{};
	inline VisInfo ( const VisInfo& other ) :
		m_visible(other.m_visible),
		m_valid(other.m_valid)
	{
	};
	inline VisInfo ( bool const visibility )
	{
		m_valid = true;
		m_visible = visibility;
	};
};

class VisCache :
	protected NoCopy,
	protected NoMove
{
private:
	VisInfo m_cache[ MAX_PLAYERS ][ MAX_PLAYERS ];

public:
	inline VisCache ()
	{
		Invalidate ();
	};
	inline ~VisCache ()
	{
		Invalidate ();
	};

	inline void Invalidate ()
	{
		memset ( this, 0, sizeof ( VisCache ) );
	};

	inline bool IsValid ( int pa, int pb ) const
	{
		return m_cache[ pa ][ pb ].m_valid;
	};

	inline bool IsVisible ( int pa, int pb ) const
	{
		return m_cache[ pa][ pb ].m_visible;
	};

	inline void SetVisibility ( int pa, int pb, bool const visibility )
	{
		m_cache[ pa][ pb ] = VisInfo ( visibility );
	};

	inline bool SetVisibility_GetNotVisible(int pa, int pb, bool const visibility)
	{
		m_cache[pa][pb] = VisInfo(visibility);
		return !visibility;
	}
};

class WallhackBlocker :
	private BaseBlockerSystem,
	private OnTickListener,
	public PlayerDataStructHandler<ClientDataS>,
	private PlayerRunCommandHookListener,
	private SetTransmitHookListener,
	public Singleton,
	private WeaponHookListener
{
	typedef PlayerDataStructHandler<ClientDataS> playerdatahandler_class;

private:
	NczPlayer* m_weapon_owner[ MAX_EDICTS ];
	VisCache m_viscache;

	bool* m_disable_shadows;
	SourceSdk::Vector* m_shadow_direction;
	float* m_shadow_maxdist;

	bool m_ffamode;

public:
	WallhackBlocker ();
	virtual ~WallhackBlocker () final;

private:
	virtual void Init () override final;

	virtual void Load () override final;

	virtual void Unload () override final;

	virtual bool GotJob () const override final;

	virtual void RT_ProcessOnTick (double const & curtime ) override final;

	virtual PlayerRunCommandRet RT_PlayerRunCommandCallback(PlayerHandler::iterator ph, void * const cmd, double const & curtime) override final;

	virtual bool RT_SetTransmitCallback ( PlayerHandler::iterator sender, PlayerHandler::iterator receiver ) override final;

	virtual bool RT_SetTransmitWeaponCallback ( SourceSdk::edict_t const * const sender, PlayerHandler::iterator receiver ) override final;

	virtual void RT_WeaponEquipCallback ( PlayerHandler::iterator ph, SourceSdk::edict_t const * const weapon ) override final;

	virtual void RT_WeaponDropCallback ( PlayerHandler::iterator ph, SourceSdk::edict_t const * const weapon ) override final;

	virtual bool sys_cmd_fn(const SourceSdk::CCommand & args);

public:
	void ClientDisconnect ( PlayerHandler::iterator client );

	void OnMapStart ();

	bool GetFFAMode() const
	{
		return m_ffamode;
	}

private:
	bool RT_IsAbleToSee ( PlayerHandler::iterator sender, PlayerHandler::iterator receiver );

	bool RT_IsInFOV ( const SourceSdk::Vector& origin, const SourceSdk::QAngle& dir, const SourceSdk::Vector& target );

	bool RT_IsVisible ( const SourceSdk::Vector& origin, const SourceSdk::Vector& target );

	bool RT_IsVisible ( const SourceSdk::Vector& origin, const SourceSdk::QAngle& dir, const SourceSdk::Vector& target );

	bool RT_IsVisible ( const SourceSdk::Vector& origin, const SourceSdk::Vector& target, const SourceSdk::Vector& mins, const SourceSdk::Vector& maxs, const SourceSdk::vec_t scale = 1.0 );

};

extern WallhackBlocker g_WallhackBlocker;

#endif // WALLHACKBLOCKER_H
