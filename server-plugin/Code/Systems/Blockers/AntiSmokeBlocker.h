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

#ifndef ANTISMOKEBLOCKER_H
#define ANTISMOKEBLOCKER_H

#include "Interfaces/IGameEventManager/IGameEventManager.h"
#include "Maths/Vector.h"

#include "Misc/temp_basiclist.h"
#include "Players/temp_PlayerDataStruct.h"
#include "Hooks/SetTransmitHookListener.h"
#include "Systems/BaseSystem.h"
#include "Systems/OnTickListener.h"
#include "Misc/temp_singleton.h"

typedef struct SmokeEntityS
{
	SourceSdk::Vector pos;
	float bang_time;

	SmokeEntityS ()
	{
		memset ( this, 0, sizeof ( SmokeEntityS ) );
	};
	SmokeEntityS ( const SmokeEntityS& other )
	{
		memcpy ( this, &other, sizeof ( SmokeEntityS ) );
	};
	SmokeEntityS ( const SourceSdk::Vector& opos )
	{
		SourceSdk::VectorCopy ( opos, pos );
		bang_time = Plat_FloatTime ();
	};

	bool operator== ( const SmokeEntityS& other )
	{
		if( !VectorEqual ( pos, other.pos ) ) return false;
		if( bang_time != other.bang_time ) return false;
		return true;
	};
} SmokeEntityT;

typedef basic_slist<SmokeEntityT> SmokeListT;

typedef struct ALIGN8 SmokeInfoS
{
	bool is_in_smoke;
	bool can_not_see_this_player[ MAX_PLAYERS ];

	SmokeInfoS ()
	{
		memset ( this, 0, sizeof ( SmokeInfoS ) );
	};
	SmokeInfoS ( const SmokeInfoS& other )
	{
		memcpy ( this, &other, sizeof ( SmokeInfoS ) );
	};
} ALIGN8_POST SmokeInfoT;

class AntiSmokeBlocker :
	private BaseSystem,
	private SourceSdk::IGameEventListener002,
	private OnTickListener,
	public PlayerDataStructHandler<SmokeInfoT>,
	private SetTransmitHookListener,
	public Singleton<AntiSmokeBlocker>
{
	typedef Singleton<AntiSmokeBlocker> singleton_class;
	typedef PlayerDataStructHandler<SmokeInfoT> playerdatahandler_class;

private:
	SmokeListT m_smokes;

public:
	AntiSmokeBlocker ();

	virtual ~AntiSmokeBlocker () final;

private:
	virtual void Init () override final;

	virtual void Load () override final;

	virtual void Unload () override final;

	virtual bool GotJob () const override final;

	virtual void FireGameEvent ( SourceSdk::IGameEvent* ev ) override final;

	virtual void RT_ProcessOnTick ( float const curtime ) override final;

	virtual bool RT_SetTransmitCallback ( PlayerHandler::const_iterator sender, PlayerHandler::const_iterator receiver ) override final;
};

#endif // ANTIFLASHBANGBLOCKER_H
