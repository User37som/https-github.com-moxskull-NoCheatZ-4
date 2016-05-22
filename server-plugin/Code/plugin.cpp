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
#include "Systems/AutoTVRecord.h"
#include "Misc/EntityProps.h"

#include "Systems/OnTickListener.h"
#include "Systems/TimerListener.h"

static void* __CreatePlugin_interface()
{ 
	CNoCheatZPlugin::CreateInstance();
	printf("CNoCheatZPlugin interface created with CSGO callbacks ...");
	return CNoCheatZPlugin::GetInstance();
}

void* CreateInterfaceInternal(char const *pName, int *pReturnCode)
{
	printf(Helpers::format("Game engine asking for %s", pName).c_str());
	if(pReturnCode) *pReturnCode = SourceSdk::IFACE_OK;
	return __CreatePlugin_interface();
}

void* SourceSdk::CreateInterface(char const * pName, int * pReturnCode)
{
	return CreateInterfaceInternal(pName, pReturnCode);
}

float Plat_FloatTime()
{
	return (GlobalTimer::GetInstance()->GetCurrent() * 0.001f);
}

void CNoCheatZPlugin::CreateSingletons()
{
	GlobalTimer::CreateInstance();
	Logger::CreateInstance();
	ConfigManager::CreateInstance();
	NczPlayerManager::CreateInstance();
	BanRequest::CreateInstance();
	EntityProps::CreateInstance();
	
	AntiFlashbangBlocker::CreateInstance();
	AntiSmokeBlocker::CreateInstance();
	BadUserCmdBlocker::CreateInstance();
	WallhackBlocker::CreateInstance();
	ConCommandTester::CreateInstance();
	ConVarTester::CreateInstance();
	EyeAnglesTester::CreateInstance();
	JumpTester::CreateInstance();
	ShotTester::CreateInstance();
	SpamChangeNameTester::CreateInstance();
	SpamConnectTester::CreateInstance();
	SpeedTester::CreateInstance();
	ValidationTester::CreateInstance();
	AutoTVRecord::CreateInstance();
}

void CNoCheatZPlugin::DestroySingletons()
{
	AutoTVRecord::DestroyInstance();
	ValidationTester::DestroyInstance();
	SpeedTester::DestroyInstance();
	SpamConnectTester::DestroyInstance();
	SpamChangeNameTester::DestroyInstance();
	ShotTester::DestroyInstance();
	JumpTester::DestroyInstance();
	EyeAnglesTester::DestroyInstance();
	ConVarTester::DestroyInstance();
	ConCommandTester::DestroyInstance();
	WallhackBlocker::DestroyInstance();
	BadUserCmdBlocker::DestroyInstance();
	AntiSmokeBlocker::DestroyInstance();
	AntiFlashbangBlocker::DestroyInstance();

	EntityProps::DestroyInstance();
	BanRequest::DestroyInstance();
	NczPlayerManager::DestroyInstance();
	ConfigManager::DestroyInstance();
	Logger::DestroyInstance();
	GlobalTimer::DestroyInstance();
}



//---------------------------------------------------------------------------------
// Purpose: constructor/destructor
//---------------------------------------------------------------------------------
CNoCheatZPlugin::CNoCheatZPlugin()
{
	m_iClientCommandIndex = 0;
	m_bAlreadyLoaded = false;

	CreateSingletons();
}

CNoCheatZPlugin::~CNoCheatZPlugin()
{
	DestroySingletons();
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
	GlobalTimer::GetInstance()->EnterSection();

	Logger::CreateInstance();
	Logger::GetInstance()->Msg<MSG_CONSOLE>("Loading ...");

	if (!SourceSdk::InterfacesProxy::Load(gameServerFactory, _interfaceFactory)) return false;

	void* pinstance = SourceSdk::InterfacesProxy::ICvar_FindVar("nocheatz_instance");
	if(pinstance)
	{
		if(SourceSdk::InterfacesProxy::ConVar_GetBool(pinstance))
		{
			Logger::GetInstance()->Msg<MSG_ERROR>("CNoCheatZPlugin already loaded");
			m_bAlreadyLoaded = true;
			return false;
		}
		Assert("Error when testing for multiple instances" && 0);
	}

	ConfigManager::CreateInstance();
	if (!ConfigManager::GetInstance()->LoadConfig()) return false;

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

	EntityProps::CreateInstance();

	BaseSystem::InitSystems();
	BanRequest::CreateInstance();
	BanRequest::GetInstance()->Init();

	NczPlayerManager::CreateInstance();
	NczPlayerManager::GetInstance()->LoadPlayerManager(); // Mark any present player as PLAYER_CONNECTED

	SourceSdk::InterfacesProxy::Call_ServerExecute();
	SourceSdk::InterfacesProxy::Call_ServerCommand("exec nocheatz.cfg\n");
	SourceSdk::InterfacesProxy::Call_ServerExecute();

	for(int i = 1; i < MAX_PLAYERS; ++i)
	{
		PlayerHandler* ph = NczPlayerManager::GetInstance()->GetPlayerHandlerByIndex(i);
		if(ph->status >= BOT)
		{
			HookEntity(ph->playerClass->GetEdict());
			WeaponHookListener::HookWeapon(ph->playerClass);

			if(ph->status >= PLAYER_CONNECTED)
			{
				HookBasePlayer(ph->playerClass);
				ValidationTester::GetInstance()->ResetPlayerDataStruct(ph->playerClass);
				JumpTester::GetInstance()->ResetPlayerDataStruct(ph->playerClass);
				EyeAnglesTester::GetInstance()->ResetPlayerDataStruct(ph->playerClass);
				ConVarTester::GetInstance()->ResetPlayerDataStruct(ph->playerClass);
				ShotTester::GetInstance()->ResetPlayerDataStruct(ph->playerClass);
				SpeedTester::GetInstance()->ResetPlayerDataStruct(ph->playerClass);
				ConCommandTester::GetInstance()->ResetPlayerDataStruct(ph->playerClass);
				AntiFlashbangBlocker::GetInstance()->ResetPlayerDataStruct(ph->playerClass);
				AntiSmokeBlocker::GetInstance()->ResetPlayerDataStruct(ph->playerClass);
				BadUserCmdBlocker::GetInstance()->ResetPlayerDataStruct(ph->playerClass);
				WallhackBlocker::GetInstance()->ResetPlayerDataStruct(ph->playerClass);
				SpamChangeNameTester::GetInstance()->ResetPlayerDataStruct(ph->playerClass);
			}
		}
	}
	BaseSystem::ManageSystems();

	SourceSdk::InterfacesProxy::ConVar_SetValue<bool>(nocheatz_instance, true);

	Logger::GetInstance()->Msg<MSG_CHAT>("Loaded");
	
	return true;
}

//---------------------------------------------------------------------------------
// Purpose: called when the plugin is unloaded (turned off)
//---------------------------------------------------------------------------------
void CNoCheatZPlugin::Unload( void )
{
	BanRequest::GetInstance()->WriteBansIfNeeded();

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

	Logger::GetInstance()->Flush();

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

	DestroySingletons();
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
	Logger::GetInstance()->Msg<MSG_LOG>(Helpers::format("PLAYING ON A NEW MAP : %s", pMapName));

	Logger::GetInstance()->Flush();
	NczPlayerManager::GetInstance()->OnLevelInit();
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

	NczPlayerManager::GetInstance()->LoadPlayerManager();
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
		NczPlayerManager::GetInstance()->Think(); /// ALWAYS FIRST
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

	BanRequest::GetInstance()->WriteBansIfNeeded();
	BaseSystem::UnloadAllSystems();
	Logger::GetInstance()->Flush();
}

//---------------------------------------------------------------------------------
// Purpose: called when a client spawns into a server (i.e as they begin to play)
//---------------------------------------------------------------------------------
void CNoCheatZPlugin::ClientActive(SourceSdk::edict_t *pEntity )
{
	DebugMessage(Helpers::format("CNoCheatZPlugin::ClientActive (%X -> %s)", pEntity, pEntity->GetClassName()));

	NczPlayerManager::GetInstance()->ClientActive(pEntity);

	PlayerHandler* ph = NczPlayerManager::GetInstance()->GetPlayerHandlerByEdict(pEntity);
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
	DebugMessage("CNoCheatZPlugin::ClientDisconnect");

	WallhackBlocker::GetInstance()->ClientDisconnect(pEntity);
	NczPlayerManager::GetInstance()->ClientDisconnect(pEntity);
}

//---------------------------------------------------------------------------------
// Purpose: called on 
//---------------------------------------------------------------------------------
void CNoCheatZPlugin::ClientPutInServer(SourceSdk::edict_t *pEntity, char const *playername )
{
	SlotStatus stat = NczPlayerManager::GetInstance()->GetPlayerHandlerByEdict(pEntity)->status;
	if (stat == INVALID)
	{
		ValidationTester::GetInstance()->ResetPlayerDataStruct(pEntity);
	}

	DebugMessage(Helpers::format("CNoCheatZPlugin::ClientPutInServer (%s -> %s) (Was already connected: %s)", pEntity->GetClassName(), playername, Helpers::boolToString(stat != INVALID)));
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

	NczPlayerManager::GetInstance()->ClientConnect(pEntity);
	NczPlayer* player = NczPlayerManager::GetInstance()->GetPlayerHandlerByEdict(pEntity)->playerClass;

	g_SpamConnectTester.ClientConnect(bAllowConnect, pEntity, pszName, pszAddress, reject, maxrejectlen);
	SpamChangeNameTester::GetInstance()->ClientConnect(bAllowConnect, pEntity, pszName, pszAddress, reject, maxrejectlen);

	DebugMessage(Helpers::format("CNoCheatZPlugin::ClientConnect (AllowConnect: %s, %X -> %s, %s, %s, %s", Helpers::boolToString(*bAllowConnect), pEntity, pEntity->GetClassName(), pszName, pszAddress, reject));

	if (!*bAllowConnect)
	{
		NczPlayerManager::GetInstance()->ClientDisconnect(pEntity);
		return SourceSdk::PLUGIN_STOP;
	}

	JumpTester::GetInstance()->ResetPlayerDataStruct(player);
	EyeAnglesTester::GetInstance()->ResetPlayerDataStruct(player);
	ConVarTester::GetInstance()->ResetPlayerDataStruct(player);
	ShotTester::GetInstance()->ResetPlayerDataStruct(player);
	SpeedTester::GetInstance()->ResetPlayerDataStruct(player);
	ConCommandTester::GetInstance()->ResetPlayerDataStruct(player);
	AntiFlashbangBlocker::GetInstance()->ResetPlayerDataStruct(player);
	AntiSmokeBlocker::GetInstance()->ResetPlayerDataStruct(player);
	BadUserCmdBlocker::GetInstance()->ResetPlayerDataStruct(player);
	WallhackBlocker::GetInstance()->ResetPlayerDataStruct(player);
	SpamChangeNameTester::GetInstance()->ResetPlayerDataStruct(player);

	return SourceSdk::PLUGIN_CONTINUE;
}

//---------------------------------------------------------------------------------
// Purpose: called when a client types in a command (only a subset of commands however, not CON_COMMAND's)
//---------------------------------------------------------------------------------
SourceSdk::PLUGIN_RESULT CNoCheatZPlugin::ClientCommand(SourceSdk::edict_t *pEntity, const SourceSdk::CCommand &args )
{
	DebugMessage(Helpers::format("CNoCheatZPlugin::ClientCommand(%X -> %s, %s)", pEntity, pEntity->GetClassName(), args.GetCommandString()));
	
	if ( !pEntity || pEntity->IsFree() ) 
	{
		return SourceSdk::PLUGIN_CONTINUE;
	}

	PlayerHandler* ph = NczPlayerManager::GetInstance()->GetPlayerHandlerByEdict(pEntity);
	if(ph->status >= PLAYER_CONNECTED)
	{
		if(ConCommandTester::GetInstance()->TestPlayerCommand(ph->playerClass, args.GetCommandString()))
			return SourceSdk::PLUGIN_STOP;
		if(stricmp(args[0], "joingame") == 0 || stricmp(args[0], "jointeam") == 0 || stricmp(args[0], "joinclass") == 0)
		{
			if(ValidationTester::GetInstance()->JoinCallback(ph->playerClass))
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

	NczPlayer* player = NczPlayerManager::GetInstance()->GetPlayerHandlerBySteamID(pszNetworkID)->playerClass;
	
	if(player) // Sometimes NetworkIDValidated gets called before CServerPlugin::ClientConnect
		ValidationTester::GetInstance()->SetValidated(player);
	else
		ValidationTester::GetInstance()->AddPendingValidation(pszNetworkID);
	
	return SourceSdk::PLUGIN_CONTINUE;
}

//---------------------------------------------------------------------------------
// Purpose: called when a cvar value query is finished
//---------------------------------------------------------------------------------
void CNoCheatZPlugin::OnQueryCvarValueFinished(SourceSdk::QueryCvarCookie_t iCookie, SourceSdk::edict_t *pPlayerEntity, SourceSdk::EQueryCvarValueStatus eStatus, const char *pCvarName, const char *pCvarValue )
{
	PlayerHandler* ph = NczPlayerManager::GetInstance()->GetPlayerHandlerByEdict(pPlayerEntity);
	if(!ConVarTester::GetInstance()->CanProcessThisSlot(ph->status)) return;

	ConVarTester::GetInstance()->OnQueryCvarValueFinished(ph->playerClass, eStatus, pCvarName, pCvarValue);
}

void CNoCheatZPlugin::OnEdictAllocated(SourceSdk::edict_t *edict )
{
	//printf("OnEdictAllocated(%x)\n", edict);
}
void CNoCheatZPlugin::OnEdictFreed( const SourceSdk::edict_t *edict  )
{
	//printf("OnEdictFreed(%x)\n", edict);
}
