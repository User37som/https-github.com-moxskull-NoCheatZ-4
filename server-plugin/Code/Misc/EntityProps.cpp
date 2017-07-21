#include "EntityProps.h"

#include "Console/convar.h"
#include "Containers/utlvector.h"
#include "Systems/ConfigManager.h"

#include "Hooks/Hook.h"

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

EntityProps g_EntityProps;

SourceSdk::datamap_t* GetDataDescMap ( SourceSdk::edict_t* const pEntity )
{
	SourceSdk::CBaseEntity* const baseEnt ( pEntity->GetUnknown ()->GetBaseEntity () );
	const DWORD* pdwInterface ( IFACE_PTR ( baseEnt ) );

	GetDataDescMap_t fn;
	*( DWORD* )&( fn ) = pdwInterface[ g_ConfigManager.vfid_getdatadescmap ];

	return fn ( baseEnt );
}
