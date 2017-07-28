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
	double bang_time;

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
		bang_time = Tier0::Plat_FloatTime ();
	};

	bool operator== ( const SmokeEntityS& other )
	{
		if( !VectorEqual ( pos, other.pos ) ) return false;
		if( bang_time != other.bang_time ) return false;
		return true;
	};
} SmokeEntityT;

typedef basic_slist<SmokeEntityT> SmokeListT;

typedef struct SmokeInfoS
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
} SmokeInfoT;

class AntiSmokeBlocker :
	private BaseBlockerSystem,
	private SourceSdk::IGameEventListener002,
	private OnTickListener,
	public PlayerDataStructHandler<SmokeInfoT>,
	private SetTransmitHookListener,
	public Singleton
{
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

	virtual void RT_ProcessOnTick (double const & curtime ) override final;

	virtual bool RT_SetTransmitCallback ( PlayerHandler::iterator sender, PlayerHandler::iterator receiver ) override final;
};

extern AntiSmokeBlocker g_AntiSmokeBlocker;

#endif // ANTIFLASHBANGBLOCKER_H
