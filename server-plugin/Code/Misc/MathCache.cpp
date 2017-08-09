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

#include "MathCache.h"

#include "EntityProps.h"
#include "Interfaces/InterfacesProxy.h"
#include "Helpers.h"
#include "Players/NczPlayerManager.h"
#include "Hooks/PlayerRunCommandHookListener.h"

MathCache::MathCache ()
{
	RT_SetCacheExpired ();
}

MathCache::~MathCache ()
{
	RT_SetCacheExpired ();
}

MathInfo& MathCache::RT_GetCachedMaths ( int const player_index, bool const force_update /* = false */ )
{
	LoggerAssert ( player_index > 0 && player_index <= 66 );

	CacheInfo& item ( m_cache[ player_index ] );
	MathInfo& info ( item.m_info );
	if( ( item.m_is_not_expired == false ) || force_update )
	{
		PlayerHandler::iterator ph ( player_index );

		LoggerAssert ( ph > SlotStatus::INVALID );

		SourceSdk::IPlayerInfo * const playerinfo ( ph->GetPlayerInfo () );

		if( playerinfo )
		{

			SourceSdk::VectorCopy ( playerinfo->GetAbsOrigin (), info.m_abs_origin );
			ph->GetAbsEyePos ( info.m_eyepos );
			if( playerinfo->IsFakeClient () )
			{
				SourceSdk::VectorCopy ( playerinfo->GetAbsAngles (), info.m_eyeangles );
			}
			else
			{
				ph->GetEyeAngles ( info.m_eyeangles );
			}
			SourceSdk::VectorCopy ( *g_EntityProps.GetPropValue<SourceSdk::Vector, PROP_ABS_VELOCITY> ( ph->GetEdict (), true ), info.m_velocity );
			SourceSdk::VectorCopy(info.m_velocity, info.m_abs_velocity);
			SourceSdk::VectorMultiply ( info.m_abs_velocity, 0.01f );
			SourceSdk::VectorAbs ( info.m_abs_velocity );
			SourceSdk::VectorCopy ( playerinfo->GetPlayerMins (), info.m_mins );
			SourceSdk::VectorCopy ( playerinfo->GetPlayerMaxs (), info.m_maxs );
		}
		else
		{
			DebugMessage ( "Encountered null playerinfo in MathCache" );
		}
	}
	return info;
}

MathCache g_MathCache;
