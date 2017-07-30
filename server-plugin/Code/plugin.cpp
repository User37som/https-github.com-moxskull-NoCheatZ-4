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
#include "Systems/Testers/ConVarTester.h"
//#include "Systems/Testers/ShotTester.h"
#include "Systems/Testers/AutoAttackTester.h"
#include "Systems/Testers/SpeedTester.h"
#include "Systems/Testers/ConCommandTester.h"
#include "Systems/Testers/SpamConnectTester.h"
#include "Systems/Testers/SpamChangeNameTester.h"
#include "Systems/Blockers/AntiFlashbangBlocker.h"
#include "Systems/Blockers/AntiSmokeBlocker.h"
#include "Systems/Testers/BadUserCmdTester.h"
#include "Systems/Testers/MouseTester.h"
#include "Systems/Blockers/WallhackBlocker.h"
#include "Systems/Blockers/RadarHackBlocker.h"
#include "Systems/Blockers/BhopBlocker.h"
#include "Systems/BanRequest.h"
#include "Systems/ConfigManager.h"
#include "Systems/Logger.h"
#include "Systems/AutoTVRecord.h"
#include "Misc/EntityProps.h"
#include "Misc/MathCache.h"
#include "Misc/SigHandler.h"
#include "Hooks/DearMetamodSource.h"

#include "Systems/OnTickListener.h"
#include "Systems/TimerListener.h"

static void* __CreatePlugin_interface ()
{
	static KxStackTrace g_KxStackTrace;

	printf ( "__CreatePlugin_interface - HeapMemoryManager::InitPool()\n" );
	HeapMemoryManager::InitPool ();
	printf ( "CNoCheatZPlugin interface created with CSGO callbacks ...\n" );
	return &g_CNoCheatZPlugin;
}

void* CreateInterfaceInternal ( char const *pName, int *pReturnCode )
{
	printf ( "NoCheatZ plugin.cpp : CreateInterfaceInternal - Game engine asking for %s\n", pName );
	/*if( CNoCheatZPlugin::IsCreated () )
	{
		printf ( "NoCheatZ plugin.cpp : CreateInterfaceInternal - ERROR : Plugin already loaded\n" );
		if( pReturnCode ) *pReturnCode = SourceSdk::IFACE_FAILED;
		return nullptr;
	}
	else*/
	{
		if( pReturnCode ) *pReturnCode = SourceSdk::IFACE_OK;
		return __CreatePlugin_interface ();
	}
}

void* SourceSdk::CreateInterface ( char const * pName, int * pReturnCode )
{
	return CreateInterfaceInternal ( pName, pReturnCode );
}

float HOOKFN_INT GetTickInterval(void * const preserve_me)
{
	return ConfigManager::tickinterval;
}

void CNoCheatZPlugin::DestroySingletons ()
{
	g_SourceHookSafety.ProcessRevertAll();
	g_Logger.SetBypassServerConsoleMsg(true);
	HeapMemoryManager::FreePool ();
}

//---------------------------------------------------------------------------------
// Purpose: constructor/destructor
//---------------------------------------------------------------------------------
CNoCheatZPlugin::CNoCheatZPlugin () :
	SourceSdk::IServerPluginCallbacks (),
	Singleton (),
	m_iClientCommandIndex ( 0 ),
	ncz_cmd_ptr ( nullptr )
{
	Tier0::LinkTier0Functions();
}

CNoCheatZPlugin::~CNoCheatZPlugin ()
{
	DestroySingletons ();
}

void HookBasePlayer ( PlayerHandler::iterator ph )
{
	OnGroundHookListener::HookOnGround ( ph );
	PlayerRunCommandHookListener::HookPlayerRunCommand ( ph );
	//TeleportHookListener::HookTeleport(player);
}

void HookEntity ( SourceSdk::edict_t* ent )
{
	SetTransmitHookListener::HookSetTransmit ( ent, true );
}

//---------------------------------------------------------------------------------
// Purpose: called when the plugin is loaded, load the interface we need from the engine
//---------------------------------------------------------------------------------
bool CNoCheatZPlugin::Load ( SourceSdk::CreateInterfaceFn _interfaceFactory, SourceSdk::CreateInterfaceFn gameServerFactory )
{
	g_Logger.Msg<MSG_CONSOLE> ( "Loading ..." );

	if( !SourceSdk::InterfacesProxy::Load ( gameServerFactory, _interfaceFactory ) )
	{
		g_Logger.Msg<MSG_ERROR> ( "SourceSdk::InterfacesProxy::Load failed" );
		return false;
	}

	if( !g_ConfigManager.LoadConfig () )
	{
		g_Logger.Msg<MSG_ERROR> ( "ConfigManager::LoadConfig failed" );
		return false;
	}

	// replace tickinterval

	switch (SourceSdk::InterfacesProxy::m_servergamedll_version)
	{
	case 5:
	case 6:
		ReplaceVirtualFunctionByFakeVirtual((DWORD)GetTickInterval, &(IFACE_PTR(SourceSdk::InterfacesProxy::m_servergamedll)[9]));
		SourceSdk::InterfacesProxy::_vfptr_GetTickInterval = (SourceSdk::InterfacesProxy::GetTickInterval_t)GetTickInterval;
		break;
	case 9:
	case 10:
		ReplaceVirtualFunctionByFakeVirtual((DWORD)GetTickInterval, &(IFACE_PTR(SourceSdk::InterfacesProxy::m_servergamedll)[10]));
		SourceSdk::InterfacesProxy::_vfptr_GetTickInterval = (SourceSdk::InterfacesProxy::GetTickInterval_t)GetTickInterval;
		break;
	default:
		break;
	};

	g_SourceHookSafety.TryHookMMSourceHook();
	g_QueryCvarProvider.InitCookie();

	if( SourceSdk::InterfacesProxy::m_game == SourceSdk::CounterStrikeGlobalOffensive )
	{
		ncz_cmd_ptr = new SourceSdk::ConCommand_csgo ( "ncz", BaseSystem::ncz_cmd_fn, "NoCheatZ", FCVAR_DONTRECORD | 1 << 18 );

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

		SourceSdk::ConVar_Register ( 0 );
	}

	UserMessageHookListener::HookUserMessage ();

	BaseSystem::InitSystems ();
	g_BanRequest.Init ();

	g_NczPlayerManager.LoadPlayerManager (); // Mark any present player as PLAYER_CONNECTED

	SourceSdk::InterfacesProxy::Call_ServerExecute ();
	SourceSdk::InterfacesProxy::Call_ServerCommand ( "exec nocheatz.cfg\n" );
	SourceSdk::InterfacesProxy::Call_ServerCommand( "exec nocheatz-dev.cfg\n");
	SourceSdk::InterfacesProxy::Call_ServerCommand (Helpers::format("sv_mincmdrate %f\n", g_ConfigManager.tickrate_override));
	SourceSdk::InterfacesProxy::Call_ServerExecute ();

	ProcessFilter::HumanAtLeastConnectedOrBot filter_class;
	for( PlayerHandler::iterator ph ( &filter_class ); ph != PlayerHandler::end(); ph += &filter_class )
	{
		HookEntity ( ph->GetEdict () );
		WeaponHookListener::HookWeapon ( ph );

		if( ph >= SlotStatus::PLAYER_CONNECTED )
		{
			HookBasePlayer ( ph );
		}
	}
	BaseSystem::ManageSystems ();

	g_Logger.Msg<MSG_CHAT> ( "Loaded" );

	return true;
}

//---------------------------------------------------------------------------------
// Purpose: called when the plugin is unloaded (turned off)
//---------------------------------------------------------------------------------
void CNoCheatZPlugin::Unload ( void )
{
	g_BanRequest.WriteBansIfNeeded ();

	/*PlayerRunCommandHookListener::UnhookPlayerRunCommand();
	OnGroundHookListener::UnhookOnGround();
	//TeleportHookListener::UnhookTeleport();
	SetTransmitHookListener::UnhookSetTransmit();
	WeaponHookListener::UnhookWeapon();
	ConCommandHookListener::UnhookDispatch();*/

	g_Logger.Flush ();

	SourceSdk::ConVar_Unregister ();

	if( SourceSdk::InterfacesProxy::m_game == SourceSdk::CounterStrikeGlobalOffensive )
	{
		if( ncz_cmd_ptr ) delete static_cast< SourceSdk::ConCommand_csgo* >( ncz_cmd_ptr );
	}
	else
	{
		if( ncz_cmd_ptr ) delete static_cast< SourceSdk::ConCommand* >( ncz_cmd_ptr );
	}

	DestroySingletons ();
}

//---------------------------------------------------------------------------------
// Purpose: called when the plugin is paused (i.e should stop running but isn't unloaded)
//---------------------------------------------------------------------------------
void CNoCheatZPlugin::Pause ( void )
{
	BaseSystem::UnloadAllSystems ();
	g_BanRequest.WriteBansIfNeeded ();
	g_Logger.Msg<MSG_CHAT> ( "Plugin pause" );
	g_Logger.Flush ();
}

//---------------------------------------------------------------------------------
// Purpose: called when the plugin is unpaused (i.e should start executing again)
//---------------------------------------------------------------------------------
void CNoCheatZPlugin::UnPause ( void )
{
	g_Logger.Msg<MSG_CONSOLE> ( "Unpausing ..." );
	BaseSystem::InitSystems ();
	g_BanRequest.Init ();

	g_NczPlayerManager.LoadPlayerManager (); // Mark any present player as PLAYER_CONNECTED

	SourceSdk::InterfacesProxy::Call_ServerExecute ();
	SourceSdk::InterfacesProxy::Call_ServerCommand ( "exec nocheatz.cfg\n" );
	SourceSdk::InterfacesProxy::Call_ServerExecute ();

	ProcessFilter::HumanAtLeastConnectedOrBot filter_class;
	for( PlayerHandler::iterator ph ( &filter_class ); ph != PlayerHandler::end (); ph += &filter_class )
	{
		HookEntity ( ph->GetEdict () );
		WeaponHookListener::HookWeapon ( ph );

		if( ph >= SlotStatus::PLAYER_CONNECTED )
		{
			HookBasePlayer ( ph );
		}
	}
	BaseSystem::ManageSystems ();

	g_Logger.Msg<MSG_CHAT> ( "Plugin unpaused" );
}

//---------------------------------------------------------------------------------
// Purpose: the name of this plugin, returned in "plugin_print" command
//---------------------------------------------------------------------------------
const char *CNoCheatZPlugin::GetPluginDescription ( void )
{
	return basic_string(NCZ_PLUGIN_NAME).append(" ").append(NCZ_VERSION_GIT).c_str();
}

//---------------------------------------------------------------------------------
// Purpose: called on level start
//---------------------------------------------------------------------------------
void CNoCheatZPlugin::LevelInit ( char const *pMapName )
{	
	g_Logger.OnLevelInit();

	g_Logger.Msg<MSG_LOG> ( Helpers::format ( "PLAYING ON A NEW MAP : %s", pMapName ) );

	g_Logger.Flush ();
	
	g_NczPlayerManager.OnLevelInit ();

	g_BanRequest.OnLevelInit ();
}

//---------------------------------------------------------------------------------
// Purpose: called on level start, when the server is ready to accept client connections
//		edictCount is the number of entities in the level, clientMax is the max client count
//---------------------------------------------------------------------------------
void CNoCheatZPlugin::ServerActivate ( SourceSdk::edict_t *pEdictList, int edictCount, int clientMax )
{
	DebugMessage ( Helpers::format("CNoCheatZPlugin::ServerActivate (pEdictList:%p, edictCount:%d, clientMax:%d)", pEdictList, edictCount, clientMax) );

	Helpers::m_EdictList = pEdictList;
	//Helpers::m_EdictList_csgo = pEdictList;
	//Helpers::m_edictCount = edictCount;
	//Helpers::m_clientMax = clientMax;

	g_SourceHookSafety.TryHookMMSourceHook();

	g_NczPlayerManager.LoadPlayerManager ();

	g_RadarHackBlocker.OnMapStart ();
	g_WallhackBlocker.OnMapStart ();
}

//---------------------------------------------------------------------------------
// Purpose: called once per server frame, do recurring work here (like checking for timeouts)
//---------------------------------------------------------------------------------
void CNoCheatZPlugin::RT_GameFrame ( bool simulating )
{
	//OnFrameListener::OnFrame();

	if( simulating )
	{
		double const curtime = Tier0::Plat_FloatTime ();
		/**************/
		g_NczPlayerManager.RT_Think ( curtime ); /// ALWAYS FIRST
		/**************/

		g_MathCache.RT_SetCacheExpired ();

		OnTickListener::RT_OnTick ( curtime );

		TimerListener::RT_OnTick ( curtime );

		g_TVWatcher.RT_OnTick ();
	}
}

//---------------------------------------------------------------------------------
// Purpose: called on level end (as the server is shutting down or going to a new map)
//---------------------------------------------------------------------------------
void CNoCheatZPlugin::LevelShutdown ( void ) // !!!!this can get called multiple times per map change
{
	DebugMessage ( "CNoCheatZPlugin::LevelShutdown" );

	g_BanRequest.WriteBansIfNeeded ();
	BaseSystem::UnloadAllSystems ();
	g_TVWatcher.RecordEnded();
	g_Logger.Flush ();
}

//---------------------------------------------------------------------------------
// Purpose: called when a client spawns into a server (i.e as they begin to play)
//---------------------------------------------------------------------------------
void CNoCheatZPlugin::ClientActive ( SourceSdk::edict_t *pEntity )
{
	g_NczPlayerManager.ClientActive ( pEntity );

	PlayerHandler::iterator ph ( g_NczPlayerManager.GetPlayerHandlerByEdict ( pEntity ) );

	DebugMessage ( Helpers::format ( "CNoCheatZPlugin::ClientActive (pEntity:%p -> pEntity->classname:%s -> clientname:%s)", pEntity, pEntity->GetClassName (), ph->GetName() ) );

	if( ph >= SlotStatus::PLAYER_CONNECTED ) HookBasePlayer ( ph );
	if( ph >= SlotStatus::BOT )
	{
		WeaponHookListener::HookWeapon ( ph );
		HookEntity ( ph->GetEdict () );
	}

	ProcessFilter::HumanAtLeastConnected filter_class;
	if( g_NczPlayerManager.GetPlayerCount ( &filter_class ) == 1 ) g_AutoTVRecord.SpawnTV ();
}

//---------------------------------------------------------------------------------
// Purpose: called when a client leaves a server (or is timed out)
//---------------------------------------------------------------------------------
void CNoCheatZPlugin::ClientDisconnect ( SourceSdk::edict_t *pEntity )
{
	DebugMessage ( Helpers::format("CNoCheatZPlugin::ClientDisconnect (pEntity:%p -> pEntity->classname:%s)", pEntity, pEntity->GetClassName () ));

	g_WallhackBlocker.ClientDisconnect ( Helpers::IndexOfEdict ( pEntity ) );
	g_NczPlayerManager.ClientDisconnect ( pEntity );
}

//---------------------------------------------------------------------------------
// Purpose: called on 
//---------------------------------------------------------------------------------
void CNoCheatZPlugin::ClientPutInServer ( SourceSdk::edict_t *pEntity, char const *playername )
{
	/*
	SlotStatus stat ( g_NczPlayerManager.GetPlayerHandlerByEdict ( pEntity ) );
	*/
	DebugMessage ( Helpers::format ( "CNoCheatZPlugin::ClientPutInServer (pEntity:%p -> pEntity->classname:%s, playername:%s)", pEntity, pEntity->GetClassName (), playername ) );

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
{
	DebugMessage("CNoCheatZPlugin::ClientSettingsChanged");
}

//---------------------------------------------------------------------------------
// Purpose: called when a client joins a server
//---------------------------------------------------------------------------------
SourceSdk::PLUGIN_RESULT CNoCheatZPlugin::ClientConnect ( bool *bAllowConnect, SourceSdk::edict_t *pEntity, const char *pszName, const char *pszAddress, char *reject, int maxrejectlen )
{
#define MAX_CHARS_NAME 32

	g_NczPlayerManager.ClientConnect ( pEntity );

	*bAllowConnect = ! g_BanRequest.IsRejected ( pszAddress );
	if( ! bAllowConnect )
	{
		memcpy ( reject, "Please wait 20 minutes", 23 );

		g_NczPlayerManager.ClientDisconnect ( pEntity );

		g_Logger.Msg<MSG_LOG> ( Helpers::format ( "CNoCheatZPlugin::ClientConnect : Rejected %s with reason %s", pszAddress, reject ) );
		return SourceSdk::PLUGIN_OVERRIDE;
	}

	g_SpamConnectTester.ClientConnect ( bAllowConnect, pEntity, pszName, pszAddress, reject, maxrejectlen );
	g_SpamChangeNameTester.ClientConnect ( bAllowConnect, pEntity, pszName, pszAddress, reject, maxrejectlen );

	DebugMessage ( Helpers::format ( "CNoCheatZPlugin::ClientConnect (bAllowConnect:%s, pEntity:%p -> pEntity->classname:%s, pszName:%s, pszAddress:%s, reject:%s, maxrejectlen:%d", Helpers::boolToString ( *bAllowConnect ), pEntity, pEntity->GetClassName (), pszName, pszAddress, reject, maxrejectlen ) );

	if( !*bAllowConnect )
	{
		if( g_BanRequest.CanKick () )
		{
			g_NczPlayerManager.ClientDisconnect ( pEntity );
			g_Logger.Msg<MSG_LOG> ( Helpers::format ( "CNoCheatZPlugin::ClientConnect : Rejected %s with reason %s", pszAddress, reject ) );
			g_BanRequest.AddReject ( 1200, pszAddress );
			return SourceSdk::PLUGIN_OVERRIDE;
		}
		else
		{
			g_Logger.Msg<MSG_LOG> ( Helpers::format ( "CNoCheatZPlugin::ClientConnect : Would have rejected access to player %s but plugin is set to not kick.", pszAddress ) );
		}
	}

	{
		NczPlayer* player ( *( g_NczPlayerManager.GetPlayerHandlerByEdict ( pEntity ) ) );

		g_JumpTester.ResetPlayerDataStruct ( player );
		g_EyeAnglesTester.ResetPlayerDataStruct ( player );
		g_ConVarTester.ResetPlayerDataStruct ( player );
		g_AutoAttackTester.ResetPlayerDataStruct ( player );
		g_SpeedTester.ResetPlayerDataStruct ( player );
		g_ConCommandTester.ResetPlayerDataStruct ( player );
		g_AntiFlashbangBlocker.ResetPlayerDataStruct ( player );
		g_AntiSmokeBlocker.ResetPlayerDataStruct ( player );
		g_BadUserCmdTester.ResetPlayerDataStruct ( player );
		g_WallhackBlocker.ResetPlayerDataStruct ( player );
		g_SpamChangeNameTester.ResetPlayerDataStruct ( player );
		g_RadarHackBlocker.ResetPlayerDataStruct ( player );
		g_BhopBlocker.ResetPlayerDataStruct ( player );
		g_MouseTester.ResetPlayerDataStruct(player);
	}

	return SourceSdk::PLUGIN_CONTINUE;
}

//---------------------------------------------------------------------------------
// Purpose: called when a client types in a command (only a subset of commands however, not CON_COMMAND's)
//---------------------------------------------------------------------------------
SourceSdk::PLUGIN_RESULT CNoCheatZPlugin::RT_ClientCommand ( SourceSdk::edict_t *pEntity, const SourceSdk::CCommand &args )
{
	if( !pEntity || pEntity->IsFree () )
	{
		return SourceSdk::PLUGIN_CONTINUE;
	}

	PlayerHandler::iterator ph ( g_NczPlayerManager.GetPlayerHandlerByEdict ( pEntity ) );

	/*if( stricmp ( args[ 0 ], "joingame" ) == 0 || stricmp ( args[ 0 ], "jointeam" ) == 0 || stricmp ( args[ 0 ], "joinclass" ) == 0 )
	{
	}*/
	
	if( ph >= SlotStatus::PLAYER_CONNECTED )
	{
		DebugMessage ( Helpers::format ( "CNoCheatZPlugin::ClientCommand (pEntity:%p -> pEntity->classname:%s -> clientname:%s, args:%s)", pEntity, pEntity->GetClassName (), ph->GetName (), args.GetCommandString () ) );
		if( g_ConCommandTester.RT_TestPlayerCommand ( ph, args.GetCommandString () ) )
			return SourceSdk::PLUGIN_STOP;
	}
	else
	{
		//g_Logger.Msg<MSG_WARNING> ( Helpers::format ( "CNoCheatZPlugin::ClientCommand (pEntity:%p -> pEntity->classname:%s -> clientname:%s, args:%s) : Dangerous SlotStatus %s", pEntity, pEntity->GetClassName (), "", args.GetCommandString () , SlotStatusToString( ph.operator SlotStatus() )));
		if( g_ConCommandTester.RT_TestPlayerCommand_Anon ( ph, args.GetCommandString () ) )
			return SourceSdk::PLUGIN_STOP;
	}

	return SourceSdk::PLUGIN_CONTINUE;
}

//---------------------------------------------------------------------------------
// Purpose: called when a client is authenticated
//---------------------------------------------------------------------------------
SourceSdk::PLUGIN_RESULT CNoCheatZPlugin::NetworkIDValidated ( const char *pszUserName, const char *pszNetworkID )
{
	DebugMessage ( Helpers::format ( "CNoCheatZPlugin::NetworkIDValidated (pszUserName:%s, pszNetworkID:%s)", pszUserName, pszNetworkID ) );
	if( !SteamGameServer_BSecure () ) return SourceSdk::PLUGIN_CONTINUE;

	return SourceSdk::PLUGIN_CONTINUE;
}

//---------------------------------------------------------------------------------
// Purpose: called when a cvar value query is finished
//---------------------------------------------------------------------------------
void CNoCheatZPlugin::RT_OnQueryCvarValueFinished ( SourceSdk::QueryCvarCookie_t iCookie, SourceSdk::edict_t *pPlayerEntity, SourceSdk::EQueryCvarValueStatus eStatus, const char *pCvarName, const char *pCvarValue )
{
	PlayerHandler::iterator ph ( g_NczPlayerManager.GetPlayerHandlerByEdict ( pPlayerEntity ) );
	if( !( ph >= SlotStatus::PLAYER_CONNECTING ) )
	{
		DebugMessage ( "RT_OnQueryCvarValueFinished : ConVarTester cannot process callback" );
		return;
	}

	if (stricmp(pCvarName, "m_pitch") == 0)
	{
		g_MouseTester.ProcessPitchConVar(ph, pCvarValue);
	}

	g_ConVarTester.RT_OnQueryCvarValueFinished ( ph, iCookie, eStatus, pCvarName, pCvarValue );
}

void CNoCheatZPlugin::OnEdictAllocated ( SourceSdk::edict_t *edict )
{
	//printf("OnEdictAllocated(%x)\n", edict);
}
void CNoCheatZPlugin::OnEdictFreed ( const SourceSdk::edict_t *edict )
{
	//printf("OnEdictFreed(%x)\n", edict);
}

CNoCheatZPlugin g_CNoCheatZPlugin;
