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

#include "Preprocessors.h"

#include <iostream>

#include "plugin.h"

#include "Systems/Testers/EyeAnglesTester.h"
#include "Systems/Testers/JumpTester.h"
#include "Systems/Testers/ValidationTester.h"
#include "Systems/Testers/ConVarTester.h"
#include "Systems/Testers/ShotTester.h"
#include "Systems/Testers/SpeedTester.h"
#include "Systems/Testers/ConCommandTester.h"
#include "Systems/Testers/SpamConnectTester.h"
#include "Systems/Testers/SpamChangeNameTester.h"
#include "Systems/Blockers/AntiFlashbangBlocker.h"
#include "Systems/Blockers/AntiSmokeBlocker.h"
#include "Systems/Blockers/BadUserCmdBlocker.h"
#include "Systems/Blockers/WallhackBlocker.h"
#include "Systems/Blockers/RadarHackBlocker.h"
#include "Systems/BanRequest.h"
#include "Systems/ConfigManager.h"
#include "Systems/Logger.h"
#include "Systems/AutoTVRecord.h"
#include "Misc/EntityProps.h"
#include "Misc/MathCache.h"

#include "Systems/OnTickListener.h"
#include "Systems/TimerListener.h"

static void* __CreatePlugin_interface ()
{
	HeapMemoryManager::InitPool ();
	CNoCheatZPlugin::CreateInstance ();
	printf ( "CNoCheatZPlugin interface created with CSGO callbacks ...\n" );
	return CNoCheatZPlugin::GetInstance ();
}

void* CreateInterfaceInternal ( char const *pName, int *pReturnCode )
{
	printf ( "Game engine asking for %s\n", pName );
	if( pReturnCode ) *pReturnCode = SourceSdk::IFACE_OK;
	return __CreatePlugin_interface ();
}

void* SourceSdk::CreateInterface ( char const * pName, int * pReturnCode )
{
	return CreateInterfaceInternal ( pName, pReturnCode );
}

float Plat_FloatTime ()
{
	return ( GlobalTimer::GetInstance ()->GetCurrent () * 0.001f );
}

void CNoCheatZPlugin::CreateSingletons ()
{
	MathCache::CreateInstance ();
	GlobalTimer::CreateInstance ();
	Logger::CreateInstance ();
	ConfigManager::CreateInstance ();
	NczPlayerManager::CreateInstance ();
	BanRequest::CreateInstance ();
	EntityProps::CreateInstance ();
	HookGuard::CreateInstance ();

	AntiFlashbangBlocker::CreateInstance ();
	AntiSmokeBlocker::CreateInstance ();
	BadUserCmdBlocker::CreateInstance ();
	WallhackBlocker::CreateInstance ();
	ConCommandTester::CreateInstance ();
	ConVarTester::CreateInstance ();
	EyeAnglesTester::CreateInstance ();
	JumpTester::CreateInstance ();
	ShotTester::CreateInstance ();
	SpamChangeNameTester::CreateInstance ();
	SpamConnectTester::CreateInstance ();
	SpeedTester::CreateInstance ();
	ValidationTester::CreateInstance ();
	AutoTVRecord::CreateInstance ();
	RadarHackBlocker::CreateInstance ();
}

void CNoCheatZPlugin::DestroySingletons ()
{
	HookGuard::GetInstance ()->UnhookAll ();
	HookGuard::DestroyInstance ();

	RadarHackBlocker::DestroyInstance ();
	AutoTVRecord::DestroyInstance ();
	ValidationTester::DestroyInstance ();
	SpeedTester::DestroyInstance ();
	SpamConnectTester::DestroyInstance ();
	SpamChangeNameTester::DestroyInstance ();
	ShotTester::DestroyInstance ();
	JumpTester::DestroyInstance ();
	EyeAnglesTester::DestroyInstance ();
	ConVarTester::DestroyInstance ();
	ConCommandTester::DestroyInstance ();
	WallhackBlocker::DestroyInstance ();
	BadUserCmdBlocker::DestroyInstance ();
	AntiSmokeBlocker::DestroyInstance ();
	AntiFlashbangBlocker::DestroyInstance ();

	EntityProps::DestroyInstance ();
	BanRequest::DestroyInstance ();
	NczPlayerManager::DestroyInstance ();
	ConfigManager::DestroyInstance ();
	Logger::DestroyInstance ();
	GlobalTimer::DestroyInstance ();
	MathCache::DestroyInstance ();

	HeapMemoryManager::FreePool ();
}

//---------------------------------------------------------------------------------
// Purpose: constructor/destructor
//---------------------------------------------------------------------------------
CNoCheatZPlugin::CNoCheatZPlugin () :
	SourceSdk::IServerPluginCallbacks (),
	singleton_class (),
	m_iClientCommandIndex ( 0 ),
	m_bAlreadyLoaded ( false ),
	ncz_cmd_ptr ( nullptr ),
	nocheatz_instance ( nullptr )
{
	CreateSingletons ();
}

CNoCheatZPlugin::~CNoCheatZPlugin ()
{
	DestroySingletons ();
}

void HookBasePlayer ( PlayerHandler::const_iterator ph )
{
	OnGroundHookListener::HookOnGround ( ph );
	PlayerRunCommandHookListener::HookPlayerRunCommand ( ph );
	//TeleportHookListener::HookTeleport(player);
}

void HookEntity ( SourceSdk::edict_t* ent )
{
	SetTransmitHookListener::HookSetTransmit ( ent );
}

//---------------------------------------------------------------------------------
// Purpose: called when the plugin is loaded, load the interface we need from the engine
//---------------------------------------------------------------------------------
bool CNoCheatZPlugin::Load ( SourceSdk::CreateInterfaceFn _interfaceFactory, SourceSdk::CreateInterfaceFn gameServerFactory )
{
	GlobalTimer::GetInstance ()->EnterSection ();

	Logger::GetInstance ()->Msg<MSG_CONSOLE> ( "Loading ..." );

	if( !SourceSdk::InterfacesProxy::Load ( gameServerFactory, _interfaceFactory ) )
	{
		Logger::GetInstance ()->Msg<MSG_ERROR> ( "SourceSdk::InterfacesProxy::Load failed" );
		return false;
	}

	void* pinstance ( SourceSdk::InterfacesProxy::ICvar_FindVar ( "nocheatz_instance" ) );
	if( pinstance )
	{
		if( SourceSdk::InterfacesProxy::ConVar_GetBool ( pinstance ) )
		{
			Logger::GetInstance ()->Msg<MSG_ERROR> ( "CNoCheatZPlugin already loaded" );
			m_bAlreadyLoaded = true;
			return false;
		}
		Assert ( "Error when testing for multiple instances" && 0 );
	}

	if( !ConfigManager::GetInstance ()->LoadConfig () )
	{
		Logger::GetInstance ()->Msg<MSG_ERROR> ( "ConfigManager::LoadConfig failed" );
		return false;
	}

	if( SourceSdk::InterfacesProxy::m_game == SourceSdk::CounterStrikeGlobalOffensive )
	{
		ncz_cmd_ptr = new SourceSdk::ConCommand_csgo ( "ncz", BaseSystem::ncz_cmd_fn, "NoCheatZ", FCVAR_DONTRECORD | 1 << 18 );
		nocheatz_instance = new SourceSdk::ConVar_csgo ( "nocheatz_instance", "0", FCVAR_DONTRECORD | 1 << 18 );

		SourceSdk::ConVar_Register_csgo ( 0 );
	}
	else
	{
		// Fix IServerPluginCallbacks vtable, because CSGO added ClientFullyConnect in the middle ...
		DWORD* vtable ( IFACE_PTR ( this ) );

		int id ( 10 );
		int const max_id ( 19 );
		do
		{
			MoveVirtualFunction ( &vtable[ id + 1 ], &vtable[ id ] );
		}
		while( ++id != max_id );

		ncz_cmd_ptr = new SourceSdk::ConCommand ( "ncz", BaseSystem::ncz_cmd_fn, "NoCheatZ", FCVAR_DONTRECORD );
		nocheatz_instance = new SourceSdk::ConVar ( "nocheatz_instance", "0", FCVAR_DONTRECORD );

		SourceSdk::ConVar_Register ( 0 );
	}

	UserMessageHookListener::HookUserMessage ();

	BaseSystem::InitSystems ();
	BanRequest::GetInstance ()->Init ();

	NczPlayerManager::GetInstance ()->LoadPlayerManager (); // Mark any present player as PLAYER_CONNECTED

	SourceSdk::InterfacesProxy::Call_ServerExecute ();
	SourceSdk::InterfacesProxy::Call_ServerCommand ( "exec nocheatz.cfg\n" );
	SourceSdk::InterfacesProxy::Call_ServerExecute ();

	for( int i ( 1 ); i < MAX_PLAYERS; ++i )
	{
		PlayerHandler::const_iterator ph ( NczPlayerManager::GetInstance ()->GetPlayerHandlerByIndex ( i ) );
		if( ph >= SlotStatus::BOT )
		{
			HookEntity ( ph->GetEdict () );
			WeaponHookListener::HookWeapon ( ph );

			if( ph >= SlotStatus::PLAYER_CONNECTED )
			{
				HookBasePlayer ( ph );
			}
		}
	}
	BaseSystem::ManageSystems ();

	SourceSdk::InterfacesProxy::ConVar_SetValue<bool> ( nocheatz_instance, true );

	Logger::GetInstance ()->Msg<MSG_CHAT> ( "Loaded" );

	return true;
}

//---------------------------------------------------------------------------------
// Purpose: called when the plugin is unloaded (turned off)
//---------------------------------------------------------------------------------
void CNoCheatZPlugin::Unload ( void )
{
	BanRequest::GetInstance ()->WriteBansIfNeeded ();

	if( m_bAlreadyLoaded && SourceSdk::InterfacesProxy::GetCvar () )
	{
		void* inst ( SourceSdk::InterfacesProxy::ICvar_FindVar ( "nocheatz_instance" ) );
		if( inst ) SourceSdk::InterfacesProxy::ConVar_SetValue ( inst, false );
	}

	/*PlayerRunCommandHookListener::UnhookPlayerRunCommand();
	OnGroundHookListener::UnhookOnGround();
	//TeleportHookListener::UnhookTeleport();
	SetTransmitHookListener::UnhookSetTransmit();
	WeaponHookListener::UnhookWeapon();
	ConCommandHookListener::UnhookDispatch();*/

	Logger::GetInstance ()->Flush ();

	SourceSdk::ConVar_Unregister ();

	if( SourceSdk::InterfacesProxy::m_game == SourceSdk::CounterStrikeGlobalOffensive )
	{
		if( nocheatz_instance ) delete static_cast< SourceSdk::ConVar_csgo* >( nocheatz_instance );
		if( ncz_cmd_ptr ) delete static_cast< SourceSdk::ConCommand_csgo* >( ncz_cmd_ptr );
	}
	else
	{
		if( nocheatz_instance ) delete static_cast< SourceSdk::ConVar* >( nocheatz_instance );
		if( ncz_cmd_ptr ) delete static_cast< SourceSdk::ConCommand* >( ncz_cmd_ptr );
	}

	DestroySingletons ();
}

//---------------------------------------------------------------------------------
// Purpose: called when the plugin is paused (i.e should stop running but isn't unloaded)
//---------------------------------------------------------------------------------
void CNoCheatZPlugin::Pause ( void )
{
	BanRequest::GetInstance ()->WriteBansIfNeeded ();
	Logger::GetInstance ()->Msg<MSG_CHAT> ( "Plugin pause" );
	Logger::GetInstance ()->Flush ();
	DestroySingletons ();
}

//---------------------------------------------------------------------------------
// Purpose: called when the plugin is unpaused (i.e should start executing again)
//---------------------------------------------------------------------------------
void CNoCheatZPlugin::UnPause ( void )
{
	CreateSingletons ();

	GlobalTimer::GetInstance ()->EnterSection ();

	Logger::GetInstance ()->Msg<MSG_CONSOLE> ( "Unpausing ..." );
	BaseSystem::InitSystems ();
	BanRequest::GetInstance ()->Init ();

	NczPlayerManager::GetInstance ()->LoadPlayerManager (); // Mark any present player as PLAYER_CONNECTED

	SourceSdk::InterfacesProxy::Call_ServerExecute ();
	SourceSdk::InterfacesProxy::Call_ServerCommand ( "exec nocheatz.cfg\n" );
	SourceSdk::InterfacesProxy::Call_ServerExecute ();

	for( int i ( 1 ); i < MAX_PLAYERS; ++i )
	{
		PlayerHandler::const_iterator ph ( NczPlayerManager::GetInstance ()->GetPlayerHandlerByIndex ( i ) );
		if( ph >= SlotStatus::BOT )
		{
			HookEntity ( ph->GetEdict () );
			WeaponHookListener::HookWeapon ( ph );

			if( ph >= SlotStatus::PLAYER_CONNECTED )
			{
				HookBasePlayer ( ph );
			}
		}
	}
	BaseSystem::ManageSystems ();

	Logger::GetInstance ()->Msg<MSG_CHAT> ( "Plugin unpaused" );
}

//---------------------------------------------------------------------------------
// Purpose: the name of this plugin, returned in "plugin_print" command
//---------------------------------------------------------------------------------
const char *CNoCheatZPlugin::GetPluginDescription ( void )
{
	return NCZ_PLUGIN_NAME " " NCZ_VERSION_STR;
}

//---------------------------------------------------------------------------------
// Purpose: called on level start
//---------------------------------------------------------------------------------
void CNoCheatZPlugin::LevelInit ( char const *pMapName )
{
	Logger::GetInstance ()->Msg<MSG_LOG> ( Helpers::format ( "PLAYING ON A NEW MAP : %s", pMapName ) );

	Logger::GetInstance ()->Flush ();
	NczPlayerManager::GetInstance ()->OnLevelInit ();

	BanRequest::GetInstance ()->OnLevelInit ();
}

//---------------------------------------------------------------------------------
// Purpose: called on level start, when the server is ready to accept client connections
//		edictCount is the number of entities in the level, clientMax is the max client count
//---------------------------------------------------------------------------------
void CNoCheatZPlugin::ServerActivate ( SourceSdk::edict_t *pEdictList, int edictCount, int clientMax )
{
	DebugMessage ( "CNoCheatZPlugin::ServerActivate" );

	Helpers::m_EdictList = pEdictList;
	//Helpers::m_EdictList_csgo = pEdictList;
	//Helpers::m_edictCount = edictCount;
	//Helpers::m_clientMax = clientMax;

	NczPlayerManager::GetInstance ()->LoadPlayerManager ();

	RadarHackBlocker::GetInstance ()->OnMapStart ();
	WallhackBlocker::GetInstance ()->OnMapStart ();
}

//---------------------------------------------------------------------------------
// Purpose: called once per server frame, do recurring work here (like checking for timeouts)
//---------------------------------------------------------------------------------
void CNoCheatZPlugin::RT_GameFrame ( bool simulating )
{
	//OnFrameListener::OnFrame();

	//HookGuard::GetInstance()->GuardHooks();

	if( simulating )
	{
		float const curtime = Plat_FloatTime ();
		/**************/
		NczPlayerManager::GetInstance ()->RT_Think ( curtime ); /// ALWAYS FIRST
		/**************/

		MathCache::GetInstance ()->RT_SetCacheExpired ();

		OnTickListener::RT_OnTick ( curtime );

		TimerListener::RT_OnTick ( curtime );

		AutoTVRecord::GetInstance ()->RT_OnTick ();
	}
}

//---------------------------------------------------------------------------------
// Purpose: called on level end (as the server is shutting down or going to a new map)
//---------------------------------------------------------------------------------
void CNoCheatZPlugin::LevelShutdown ( void ) // !!!!this can get called multiple times per map change
{
	DebugMessage ( "CNoCheatZPlugin::LevelShutdown" );

	BanRequest::GetInstance ()->WriteBansIfNeeded ();
	BaseSystem::UnloadAllSystems ();
	Logger::GetInstance ()->Flush ();
}

//---------------------------------------------------------------------------------
// Purpose: called when a client spawns into a server (i.e as they begin to play)
//---------------------------------------------------------------------------------
void CNoCheatZPlugin::ClientActive ( SourceSdk::edict_t *pEntity )
{
	DebugMessage ( Helpers::format ( "CNoCheatZPlugin::ClientActive (%X -> %s)", pEntity, pEntity->GetClassName () ) );

	NczPlayerManager::GetInstance ()->ClientActive ( pEntity );

	PlayerHandler::const_iterator ph ( NczPlayerManager::GetInstance ()->GetPlayerHandlerByEdict ( pEntity ) );
	if( ph >= SlotStatus::PLAYER_CONNECTED ) HookBasePlayer ( ph );
	if( ph >= SlotStatus::BOT )
	{
		WeaponHookListener::HookWeapon ( ph );
		HookEntity ( ph->GetEdict () );
	}

	ProcessFilter::HumanAtLeastConnected filter_class;
	if( NczPlayerManager::GetInstance ()->GetPlayerCount ( &filter_class ) == 1 ) AutoTVRecord::GetInstance ()->SpawnTV ();
}

//---------------------------------------------------------------------------------
// Purpose: called when a client leaves a server (or is timed out)
//---------------------------------------------------------------------------------
void CNoCheatZPlugin::ClientDisconnect ( SourceSdk::edict_t *pEntity )
{
	DebugMessage ( "CNoCheatZPlugin::ClientDisconnect" );

	WallhackBlocker::GetInstance ()->ClientDisconnect ( Helpers::IndexOfEdict ( pEntity ) );
	NczPlayerManager::GetInstance ()->ClientDisconnect ( pEntity );
}

//---------------------------------------------------------------------------------
// Purpose: called on 
//---------------------------------------------------------------------------------
void CNoCheatZPlugin::ClientPutInServer ( SourceSdk::edict_t *pEntity, char const *playername )
{
	SlotStatus stat ( NczPlayerManager::GetInstance ()->GetPlayerHandlerByEdict ( pEntity ) );
	if( stat == SlotStatus::INVALID )
	{
		ValidationTester::GetInstance ()->ResetPlayerDataStruct ( pEntity );
	}

	DebugMessage ( Helpers::format ( "CNoCheatZPlugin::ClientPutInServer (%s -> %s) (Was already connected: %s)", pEntity->GetClassName (), playername, Helpers::boolToString ( stat != SlotStatus::INVALID ) ) );
}

//---------------------------------------------------------------------------------
// Purpose: called on level start
//---------------------------------------------------------------------------------
void CNoCheatZPlugin::SetCommandClient ( int index )
{
	m_iClientCommandIndex = index;
}

//---------------------------------------------------------------------------------
// Purpose: called on level start
//---------------------------------------------------------------------------------
void CNoCheatZPlugin::ClientSettingsChanged ( SourceSdk::edict_t *pEdict )
{}

//---------------------------------------------------------------------------------
// Purpose: called when a client joins a server
//---------------------------------------------------------------------------------
SourceSdk::PLUGIN_RESULT CNoCheatZPlugin::ClientConnect ( bool *bAllowConnect, SourceSdk::edict_t *pEntity, const char *pszName, const char *pszAddress, char *reject, int maxrejectlen )
{
#define MAX_CHARS_NAME 32

	NczPlayerManager::GetInstance ()->ClientConnect ( pEntity );
	NczPlayer* player ( NczPlayerManager::GetInstance ()->GetPlayerHandlerByEdict ( pEntity ) );

	SpamConnectTester::GetInstance ()->ClientConnect ( bAllowConnect, pEntity, pszName, pszAddress, reject, maxrejectlen );
	SpamChangeNameTester::GetInstance ()->ClientConnect ( bAllowConnect, pEntity, pszName, pszAddress, reject, maxrejectlen );

	DebugMessage ( Helpers::format ( "CNoCheatZPlugin::ClientConnect (AllowConnect: %s, %X -> %s, %s, %s, %s", Helpers::boolToString ( *bAllowConnect ), pEntity, pEntity->GetClassName (), pszName, pszAddress, reject ) );

	if( !*bAllowConnect )
	{
		NczPlayerManager::GetInstance ()->ClientDisconnect ( pEntity );
		return SourceSdk::PLUGIN_STOP;
	}

	JumpTester::GetInstance ()->ResetPlayerDataStruct ( player );
	EyeAnglesTester::GetInstance ()->ResetPlayerDataStruct ( player );
	ConVarTester::GetInstance ()->ResetPlayerDataStruct ( player );
	ShotTester::GetInstance ()->ResetPlayerDataStruct ( player );
	SpeedTester::GetInstance ()->ResetPlayerDataStruct ( player );
	ConCommandTester::GetInstance ()->ResetPlayerDataStruct ( player );
	AntiFlashbangBlocker::GetInstance ()->ResetPlayerDataStruct ( player );
	AntiSmokeBlocker::GetInstance ()->ResetPlayerDataStruct ( player );
	BadUserCmdBlocker::GetInstance ()->ResetPlayerDataStruct ( player );
	WallhackBlocker::GetInstance ()->ResetPlayerDataStruct ( player );
	SpamChangeNameTester::GetInstance ()->ResetPlayerDataStruct ( player );
	RadarHackBlocker::GetInstance ()->ResetPlayerDataStruct ( player );

	return SourceSdk::PLUGIN_CONTINUE;
}

//---------------------------------------------------------------------------------
// Purpose: called when a client types in a command (only a subset of commands however, not CON_COMMAND's)
//---------------------------------------------------------------------------------
SourceSdk::PLUGIN_RESULT CNoCheatZPlugin::RT_ClientCommand ( SourceSdk::edict_t *pEntity, const SourceSdk::CCommand &args )
{
	DebugMessage ( Helpers::format ( "CNoCheatZPlugin::ClientCommand(%X -> %s, %s)", pEntity, pEntity->GetClassName (), args.GetCommandString () ) );

	if( !pEntity || pEntity->IsFree () )
	{
		return SourceSdk::PLUGIN_CONTINUE;
	}

	PlayerHandler::const_iterator ph ( NczPlayerManager::GetInstance ()->GetPlayerHandlerByEdict ( pEntity ) );
	if( ph >= SlotStatus::PLAYER_CONNECTED )
	{
		if( ConCommandTester::GetInstance ()->RT_TestPlayerCommand ( ph, args.GetCommandString () ) )
			return SourceSdk::PLUGIN_STOP;
		if( stricmp ( args[ 0 ], "joingame" ) == 0 || stricmp ( args[ 0 ], "jointeam" ) == 0 || stricmp ( args[ 0 ], "joinclass" ) == 0 )
		{
			if( ValidationTester::GetInstance ()->JoinCallback ( ph ) )
				return SourceSdk::PLUGIN_STOP;
		}
	}
	return SourceSdk::PLUGIN_CONTINUE;
}

//---------------------------------------------------------------------------------
// Purpose: called when a client is authenticated
//---------------------------------------------------------------------------------
SourceSdk::PLUGIN_RESULT CNoCheatZPlugin::NetworkIDValidated ( const char *pszUserName, const char *pszNetworkID )
{
	if( !SteamGameServer_BSecure () ) return SourceSdk::PLUGIN_CONTINUE;

	ValidationTester::GetInstance ()->AddPendingValidation ( pszUserName, pszNetworkID );

	return SourceSdk::PLUGIN_CONTINUE;
}

//---------------------------------------------------------------------------------
// Purpose: called when a cvar value query is finished
//---------------------------------------------------------------------------------
void CNoCheatZPlugin::RT_OnQueryCvarValueFinished ( SourceSdk::QueryCvarCookie_t iCookie, SourceSdk::edict_t *pPlayerEntity, SourceSdk::EQueryCvarValueStatus eStatus, const char *pCvarName, const char *pCvarValue )
{
	PlayerHandler::const_iterator ph ( NczPlayerManager::GetInstance ()->GetPlayerHandlerByEdict ( pPlayerEntity ) );
	if( !( ph >= SlotStatus::PLAYER_CONNECTING ) )
	{
		DebugMessage ( "RT_OnQueryCvarValueFinished : ConVarTester cannot process callback" );
		return;
	}

	ConVarTester::GetInstance ()->RT_OnQueryCvarValueFinished ( ph, eStatus, pCvarName, pCvarValue );
}

void CNoCheatZPlugin::OnEdictAllocated ( SourceSdk::edict_t *edict )
{
	//printf("OnEdictAllocated(%x)\n", edict);
}
void CNoCheatZPlugin::OnEdictFreed ( const SourceSdk::edict_t *edict )
{
	//printf("OnEdictFreed(%x)\n", edict);
}
