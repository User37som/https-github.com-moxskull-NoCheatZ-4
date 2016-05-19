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
#include "Systems/BanRequest.h"
#include "Systems/ConfigManager.h"
#include "Systems/Logger.h"

#include "Systems/OnTickListener.h"
#include "Systems/TimerListener.h"

// 
// The plugin is a static singleton that is exported as an interface
//
CNoCheatZPlugin g_NoCheatZPlugin;

static void* __CreatePlugin_interface()
{ 
	DebugMessage("CNoCheatZPlugin interface created ...");
	return (&g_NoCheatZPlugin);
}

static SourceSdk::InterfaceReg __g_CreatePlugin_reg(__CreatePlugin_interface, INTERFACEVERSION_ISERVERPLUGINCALLBACKS);

MetricsTimer g_GlobalTime;

float Plat_FloatTime()
{
	return (g_GlobalTime.GetCurrent() * 0.001f);
}

//---------------------------------------------------------------------------------
// Purpose: constructor/destructor
//---------------------------------------------------------------------------------
CNoCheatZPlugin::CNoCheatZPlugin()
{
	m_iClientCommandIndex = 0;
	m_bAlreadyLoaded = false;
	g_GlobalTime.EnterSection();
}

CNoCheatZPlugin::~CNoCheatZPlugin()
{
}

void HookBasePlayer(NczPlayer* player)
{
	OnGroundHookListener::HookOnGround(player);
	PlayerRunCommandHookListener::HookPlayerRunCommand(player);
	//TeleportHookListener::HookTeleport(player);
}

void HookEntity(SourceSdk::edict_t* ent)
{
	SetTransmitHookListener::HookSetTransmit(ent);
}

//---------------------------------------------------------------------------------
// Purpose: called when the plugin is loaded, load the interface we need from the engine
//---------------------------------------------------------------------------------
bool CNoCheatZPlugin::Load(SourceSdk::CreateInterfaceFn _interfaceFactory, SourceSdk::CreateInterfaceFn gameServerFactory )
{
	ILogger.Msg<MSG_CONSOLE>("Loading ...");

	if (!SourceSdk::InterfacesProxy::Load(gameServerFactory, _interfaceFactory)) return false;

	void* pinstance = SourceSdk::InterfacesProxy::ICvar_FindVar("nocheatz_instance");
	if(pinstance)
	{
		if(SourceSdk::InterfacesProxy::ConVar_GetBool(pinstance))
		{
			ILogger.Msg<MSG_ERROR>("CNoCheatZPlugin already loaded");
			m_bAlreadyLoaded = true;
			return false;
		}
		Assert("Error when testing for multiple instances" && 0);
	}

	if (!g_ConfigManager.LoadConfig()) return false;

	if (SourceSdk::InterfacesProxy::m_game == SourceSdk::CounterStrikeGlobalOffensive)
	{
		ncz_cmd_ptr = new SourceSdk::ConCommand_csgo("ncz", BaseSystem::ncz_cmd_fn, "NoCheatZ", 0);
		nocheatz_instance = new SourceSdk::ConVar_csgo("nocheatz_instance", "0");

		SourceSdk::ConVar_Register_csgo(0);
	}
	else
	{
		// Fix IServerPluginCallbacks vtable, because CSGO added ClientFullyConnect in the middle ...
		DWORD* vtable = IFACE_PTR(this);

		int id = 10;
		int const max_id = 19;
		do
		{
			MoveVirtualFunction(&vtable[id + 1], &vtable[id]);
		} while (++id != max_id);

		ncz_cmd_ptr = new SourceSdk::ConCommand("ncz", BaseSystem::ncz_cmd_fn, "NoCheatZ", 0);
		nocheatz_instance = new SourceSdk::ConVar("nocheatz_instance", "0");

		SourceSdk::ConVar_Register(0);
	}

	BaseSystem::InitSystems();
	g_BanRequest.Init();

	g_NczPlayerManager.LoadPlayerManager(); // Mark any present player as PLAYER_CONNECTED

	SourceSdk::InterfacesProxy::Call_ServerExecute();
	SourceSdk::InterfacesProxy::Call_ServerCommand("exec nocheatz.cfg\n");
	SourceSdk::InterfacesProxy::Call_ServerExecute();

	for(int i = 1; i < MAX_PLAYERS; ++i)
	{
		PlayerHandler* ph = g_NczPlayerManager.GetPlayerHandlerByIndex(i);
		if(ph->status >= BOT)
		{
			HookEntity(ph->playerClass->GetEdict());
			WeaponHookListener::HookWeapon(ph->playerClass);

			if(ph->status >= PLAYER_CONNECTED)
			{
				HookBasePlayer(ph->playerClass);
				g_ValidationTester.ResetPlayerDataStruct(ph->playerClass);
				g_JumpTester.ResetPlayerDataStruct(ph->playerClass);
				g_EyeAnglesTester.ResetPlayerDataStruct(ph->playerClass);
				g_ConVarTester.ResetPlayerDataStruct(ph->playerClass);
				g_ShotTester.ResetPlayerDataStruct(ph->playerClass);
				g_SpeedTester.ResetPlayerDataStruct(ph->playerClass);
				g_ConCommandTester.ResetPlayerDataStruct(ph->playerClass);
				g_AntiFlashbangBlocker.ResetPlayerDataStruct(ph->playerClass);
				g_AntiSmokeBlocker.ResetPlayerDataStruct(ph->playerClass);
				g_BadUserCmdBlocker.ResetPlayerDataStruct(ph->playerClass);
				g_WallhackBlocker.ResetPlayerDataStruct(ph->playerClass);
				g_SpamChangeNameTester.ResetPlayerDataStruct(ph->playerClass);
			}
		}
	}
	BaseSystem::ManageSystems();

	SourceSdk::InterfacesProxy::ConVar_SetValue<bool>(nocheatz_instance, true);

	ILogger.Msg<MSG_CHAT>("Loaded");
	
	return true;
}

//---------------------------------------------------------------------------------
// Purpose: called when the plugin is unloaded (turned off)
//---------------------------------------------------------------------------------
void CNoCheatZPlugin::Unload( void )
{
	g_BanRequest.WriteBansIfNeeded();

	if(m_bAlreadyLoaded && SourceSdk::InterfacesProxy::GetCvar())
	{
		void* inst = nullptr;
		inst = SourceSdk::InterfacesProxy::ICvar_FindVar("nocheatz_instance");
		if (inst) SourceSdk::InterfacesProxy::ConVar_SetValue(inst, false);
	}

	PlayerRunCommandHookListener::UnhookPlayerRunCommand();
	OnGroundHookListener::UnhookOnGround();
	//TeleportHookListener::UnhookTeleport();
	SetTransmitHookListener::UnhookSetTransmit();
	WeaponHookListener::UnhookWeapon();

	ILogger.Flush();

	SourceSdk::ConVar_Unregister( );

	if (SourceSdk::InterfacesProxy::m_game == SourceSdk::CounterStrikeGlobalOffensive)
	{
		delete static_cast<SourceSdk::ConVar_csgo*>(nocheatz_instance);
		delete static_cast<SourceSdk::ConCommand_csgo*>(ncz_cmd_ptr);
	}
	else
	{
		delete static_cast<SourceSdk::ConVar*>(nocheatz_instance);
		delete static_cast<SourceSdk::ConCommand*>(ncz_cmd_ptr);
	}
}

//---------------------------------------------------------------------------------
// Purpose: called when the plugin is paused (i.e should stop running but isn't unloaded)
//---------------------------------------------------------------------------------
void CNoCheatZPlugin::Pause( void )
{
}

//---------------------------------------------------------------------------------
// Purpose: called when the plugin is unpaused (i.e should start executing again)
//---------------------------------------------------------------------------------
void CNoCheatZPlugin::UnPause( void )
{
}

//---------------------------------------------------------------------------------
// Purpose: the name of this plugin, returned in "plugin_print" command
//---------------------------------------------------------------------------------
const char *CNoCheatZPlugin::GetPluginDescription( void )
{
	return NCZ_PLUGIN_NAME " v" NCZ_VERSION_STR;
}

//---------------------------------------------------------------------------------
// Purpose: called on level start
//---------------------------------------------------------------------------------
void CNoCheatZPlugin::LevelInit( char const *pMapName )
{
	DebugMessage(Helpers::format("CNoCheatZPlugin::LevelInit (%s)", pMapName));

	ILogger.Flush();
	g_NczPlayerManager.OnLevelInit();
}

//---------------------------------------------------------------------------------
// Purpose: called on level start, when the server is ready to accept client connections
//		edictCount is the number of entities in the level, clientMax is the max client count
//---------------------------------------------------------------------------------
void CNoCheatZPlugin::ServerActivate(SourceSdk::edict_t *pEdictList, int edictCount, int clientMax )
{
	DebugMessage("CNoCheatZPlugin::ServerActivate");

	Helpers::m_EdictList = pEdictList;
	Helpers::m_EdictList_csgo = pEdictList;
	//Helpers::m_edictCount = edictCount;
	//Helpers::m_clientMax = clientMax;

	g_NczPlayerManager.LoadPlayerManager();
}

//---------------------------------------------------------------------------------
// Purpose: called once per server frame, do recurring work here (like checking for timeouts)
//---------------------------------------------------------------------------------
void CNoCheatZPlugin::GameFrame( bool simulating )
{
	//OnFrameListener::OnFrame();

	if(simulating)
	{
		/**************/
		g_NczPlayerManager.Think(); /// ALWAYS FIRST
		/**************/

		OnTickListener::OnTick();

		TimerListener::OnTick();
	}
}

//---------------------------------------------------------------------------------
// Purpose: called on level end (as the server is shutting down or going to a new map)
//---------------------------------------------------------------------------------
void CNoCheatZPlugin::LevelShutdown( void ) // !!!!this can get called multiple times per map change
{
	DebugMessage("CNoCheatZPlugin::LevelShutdown");

	g_BanRequest.WriteBansIfNeeded();
	BaseSystem::UnloadAllSystems();
	ILogger.Flush();
}

//---------------------------------------------------------------------------------
// Purpose: called when a client spawns into a server (i.e as they begin to play)
//---------------------------------------------------------------------------------
void CNoCheatZPlugin::ClientActive(SourceSdk::edict_t *pEntity )
{
	DebugMessage(Helpers::format("CNoCheatZPlugin::ClientActive (%X -> %s)", pEntity, pEntity->GetClassName()));

	g_NczPlayerManager.ClientActive(pEntity);

	PlayerHandler* ph = g_NczPlayerManager.GetPlayerHandlerByEdict(pEntity);
	if(ph->status >= PLAYER_CONNECTED) HookBasePlayer(ph->playerClass);
	if(ph->status >= BOT)
	{
		WeaponHookListener::HookWeapon(ph->playerClass);
		HookEntity(ph->playerClass->GetEdict());
	}
}

//---------------------------------------------------------------------------------
// Purpose: called when a client leaves a server (or is timed out)
//---------------------------------------------------------------------------------
void CNoCheatZPlugin::ClientDisconnect(SourceSdk::edict_t *pEntity )
{
	g_WallhackBlocker.ClientDisconnect(pEntity);
	g_NczPlayerManager.ClientDisconnect(pEntity);
}

//---------------------------------------------------------------------------------
// Purpose: called on 
//---------------------------------------------------------------------------------
void CNoCheatZPlugin::ClientPutInServer(SourceSdk::edict_t *pEntity, char const *playername )
{
}

//---------------------------------------------------------------------------------
// Purpose: called on level start
//---------------------------------------------------------------------------------
void CNoCheatZPlugin::SetCommandClient( int index )
{
	m_iClientCommandIndex = index;
}

//---------------------------------------------------------------------------------
// Purpose: called on level start
//---------------------------------------------------------------------------------
void CNoCheatZPlugin::ClientSettingsChanged(SourceSdk::edict_t *pEdict )
{
}

//---------------------------------------------------------------------------------
// Purpose: called when a client joins a server
//---------------------------------------------------------------------------------
SourceSdk::PLUGIN_RESULT CNoCheatZPlugin::ClientConnect( bool *bAllowConnect, SourceSdk::edict_t *pEntity, const char *pszName, const char *pszAddress, char *reject, int maxrejectlen )
{
#define MAX_CHARS_NAME 32
	DebugMessage("CNoCheatZPlugin::ClientConnect");

	g_NczPlayerManager.ClientConnect(pEntity);
	NczPlayer* player = g_NczPlayerManager.GetPlayerHandlerByEdict(pEntity)->playerClass;

	g_SpamConnectTester.ClientConnect(bAllowConnect, pEntity, pszName, pszAddress, reject, maxrejectlen);
	g_SpamChangeNameTester.ClientConnect(bAllowConnect, pEntity, pszName, pszAddress, reject, maxrejectlen);
	if(!*bAllowConnect)
	{
		g_NczPlayerManager.ClientDisconnect(pEntity);
		return SourceSdk::PLUGIN_STOP;
	}
	
	g_ValidationTester.ResetPlayerDataStruct(player);
	g_JumpTester.ResetPlayerDataStruct(player);
	g_EyeAnglesTester.ResetPlayerDataStruct(player);
	g_ConVarTester.ResetPlayerDataStruct(player);
	g_ShotTester.ResetPlayerDataStruct(player);
	g_SpeedTester.ResetPlayerDataStruct(player);
	g_ConCommandTester.ResetPlayerDataStruct(player);
	g_AntiFlashbangBlocker.ResetPlayerDataStruct(player);
	g_AntiSmokeBlocker.ResetPlayerDataStruct(player);
	g_BadUserCmdBlocker.ResetPlayerDataStruct(player);
	g_WallhackBlocker.ResetPlayerDataStruct(player);
	g_SpamChangeNameTester.ResetPlayerDataStruct(player);

	return SourceSdk::PLUGIN_CONTINUE;
}

//---------------------------------------------------------------------------------
// Purpose: called when a client types in a command (only a subset of commands however, not CON_COMMAND's)
//---------------------------------------------------------------------------------
SourceSdk::PLUGIN_RESULT CNoCheatZPlugin::ClientCommand(SourceSdk::edict_t *pEntity, const SourceSdk::CCommand &args )
{
	DebugMessage(Helpers::format("CNoCheatZPlugin::ClientCommand(%s)", args.GetCommandString()));
	
	if ( !pEntity || pEntity->IsFree() ) 
	{
		return SourceSdk::PLUGIN_CONTINUE;
	}

	PlayerHandler* ph = g_NczPlayerManager.GetPlayerHandlerByEdict(pEntity);
	if(ph->status >= PLAYER_CONNECTED)
	{
		if(g_ConCommandTester.TestPlayerCommand(ph->playerClass, args.GetCommandString()))
			return SourceSdk::PLUGIN_STOP;
		if(stricmp(args[0], "joingame") == 0 || stricmp(args[0], "jointeam") == 0 || stricmp(args[0], "joinclass") == 0)
		{
			if(g_ValidationTester.JoinCallback(ph->playerClass))
				return SourceSdk::PLUGIN_STOP;
		}
	}
	return SourceSdk::PLUGIN_CONTINUE;
}

//---------------------------------------------------------------------------------
// Purpose: called when a client is authenticated
//---------------------------------------------------------------------------------
SourceSdk::PLUGIN_RESULT CNoCheatZPlugin::NetworkIDValidated( const char *pszUserName, const char *pszNetworkID )
{
	if(!SteamGameServer_BSecure()) return SourceSdk::PLUGIN_CONTINUE;

	NczPlayer* player = g_NczPlayerManager.GetPlayerHandlerBySteamID(pszNetworkID)->playerClass;
	
	if(player) // Sometimes NetworkIDValidated gets called before CServerPlugin::ClientConnect
		g_ValidationTester.SetValidated(player);
	else
		g_ValidationTester.AddPendingValidation(pszNetworkID);
	
	return SourceSdk::PLUGIN_CONTINUE;
}

//---------------------------------------------------------------------------------
// Purpose: called when a cvar value query is finished
//---------------------------------------------------------------------------------
void CNoCheatZPlugin::OnQueryCvarValueFinished(SourceSdk::QueryCvarCookie_t iCookie, SourceSdk::edict_t *pPlayerEntity, SourceSdk::EQueryCvarValueStatus eStatus, const char *pCvarName, const char *pCvarValue )
{
	PlayerHandler* ph = g_NczPlayerManager.GetPlayerHandlerByEdict(pPlayerEntity);
	if(!g_ConVarTester.CanProcessThisSlot(ph->status)) return;

	g_ConVarTester.OnQueryCvarValueFinished(ph->playerClass, eStatus, pCvarName, pCvarValue);
}

void CNoCheatZPlugin::OnEdictAllocated(SourceSdk::edict_t *edict )
{
	//printf("OnEdictAllocated(%x)\n", edict);
}
void CNoCheatZPlugin::OnEdictFreed( const SourceSdk::edict_t *edict  )
{
	//printf("OnEdictFreed(%x)\n", edict);
}
