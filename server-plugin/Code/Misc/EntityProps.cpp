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

#include "EntityProps.h"

#include "Interfaces/InterfacesProxy.h" // IFACE_PTR + edict_t
#include "Interfaces/datamap.h"

#include "Systems/ConfigManager.h" // + basic_string

typedef SourceSdk::datamap_t* ( HOOKFN_EXT *GetDataDescMap_t )( SourceSdk::CBaseEntity* );

template <>
basic_string const & EntityProps::PropIdToString<PROP_OWNER> ()
{
	static basic_string const v ( "CBaseEntity.m_hOwnerEntity" );
	return v;
}

template <>
basic_string const & EntityProps::PropIdToString<PROP_VIEW_OFFSET> ()
{
	static basic_string const v ( "CBasePlayer.localdata.m_vecViewOffset[0]" );
	return v;
}

template <>
basic_string const & EntityProps::PropIdToString<PROP_SHADOW_DIRECTION> ()
{
	static basic_string const v ( "CShadowControl.m_shadowDirection" );
	return v;
}

template <>
basic_string const & EntityProps::PropIdToString<PROP_SHADOW_MAX_DIST> ()
{
	static basic_string const v ( "CShadowControl.m_flShadowMaxDist" );
	return v;
}

template <>
basic_string const & EntityProps::PropIdToString<PROP_DISABLE_SHADOW> ()
{
	static basic_string const v ( "CShadowControl.m_bDisableShadows" );
	return v;
}

template <>
basic_string const & EntityProps::PropIdToString<PROP_ABS_VELOCITY> ()
{
	static basic_string const v ( "CBaseEntity.m_vecAbsVelocity" );
	return v;
}

template <>
basic_string const & EntityProps::PropIdToString<PROP_FLASH_MAX_ALPHA> ()
{
	static basic_string const v ( "CCSPlayer.m_flFlashMaxAlpha" );
	return v;
}

template <>
basic_string const & EntityProps::PropIdToString<PROP_FLASH_DURATION> ()
{
	static basic_string const v ( "CCSPlayer.m_flFlashDuration" );
	return v;
}

template <>
basic_string const & EntityProps::PropIdToString<PROP_FLAGS> ()
{
	static basic_string const v ( "CBasePlayer.m_fFlags" );
	return v;
}

template <>
basic_string const & EntityProps::PropIdToString<PROP_PLAYER_SPOTTED> ()
{
	static basic_string const v ( "CCSPlayerResource.m_bPlayerSpotted" );
	return v;
}

template <>
basic_string const & EntityProps::PropIdToString<PROP_BOMB_SPOTTED> ()
{
	static basic_string const v ( "CCSPlayerResource.m_bBombSpotted" );
	return v;
}

template <>
basic_string const & EntityProps::PropIdToString<PROP_OBSERVER_MODE> ()
{
	static basic_string const v ( "CBasePlayer.m_iObserverMode" );
	return v;
}

template <>
basic_string const & EntityProps::PropIdToString<PROP_OBSERVER_TARGET> ()
{
	static basic_string const v ( "CBasePlayer.m_hObserverTarget" );
	return v;
}

template <>
basic_string const & EntityProps::PropIdToString<PROP_LERP_TIME> ()
{
	static basic_string const v ( "CBasePlayer.m_fLerpTime" );
	return v;
}

SourceSdk::datamap_t* GetDataDescMap ( SourceSdk::edict_t* const pEntity )
{
	SourceSdk::CBaseEntity* const baseEnt ( pEntity->GetUnknown ()->GetBaseEntity () );
	const DWORD* pdwInterface ( IFACE_PTR ( baseEnt ) );

	GetDataDescMap_t fn;
	*( DWORD* )&( fn ) = pdwInterface[ ConfigManager::GetInstance ()->vfid_getdatadescmap ];

	return fn ( baseEnt );
}
