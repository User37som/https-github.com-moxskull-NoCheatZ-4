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

#ifndef SVCLIENTSPROVIDER
#define SVCLIENTSPROVIDER

/*
	Provides a pointer to sv.Cients array of the server.
	CGameClients also have a pointer to CGameServer that we can get as long as we have the correct offsets (= correct definitions of the class)

	We find the offset in this function : virtual QueryCvarCookie_t VEngineServer::StartQueryCvarValue( edict_t *pPlayerEntity, const char *pCvarName )
	because this function is easy to find with the string "StartQueryCvarValue: not a client"
	and also because there is low risk the signature changes overtime.
	engine module.
*/

#include "Hooks/SigScan.h"

class CBaseClient;

/*
	Once we get the address of m_Clients, we can discover the address of CBaseServer simply by substracting the offset of m_Clients using the following struct.
*/

struct CGameServer_se2007
{
	void ** vtable;
	server_state_t	m_State;		// some actions are only valid during load
	int				m_Socket;		// network socket 
	int				m_nTickCount;	// current server tick
	char			m_szMapname[64];		// map name without path and extension
	char			m_szSkyname[64];		// skybox name
	char			m_Password[32];		// server password
	CRC32_t			worldmapCRC;      // For detecting that client has a hacked local copy of map, the client will be dropped if this occurs.
	CRC32_t			clientDllCRC; // The dll that this server is expecting clients to be using.
	void *m_StringTables;	// CNetworkStringTableContainer newtork string table container
	void *m_pInstanceBaselineTable; // INetworkStringTable *
	void *m_pLightStyleTable;// INetworkStringTable *
	void *m_pUserInfoTable;// INetworkStringTable *
	void *m_pServerStartupTable;// INetworkStringTable *
	void *m_pDownloadableFileTable;// INetworkStringTable *
	bf_write			m_Signon;
	CUtlMemory<byte>	m_SignonBuffer;
	int			serverclasses;		// number of unique server classes
	int			serverclassbits;	// log2 of serverclasses
	int			m_nUserid;			// increases by one with every new client
	int			m_nMaxclients;         // Current max #
	int			m_nSpawnCount;			// Number of servers spawned since start,
	float		m_flTickInterval;		// time for 1 tick in seconds
	CUtlVector<CBaseClient*>	m_Clients;		// array of up to [maxclients] client slots.
	bool		m_bIsDedicated;
	CUtlVector<challenge_t> m_ServerQueryChallenges; // prevent spoofed IP's from server queries/connecting
	float		m_fCPUPercent;
	float		m_fStartTime;
	float		m_fLastCPUCheckTime;
	bool		m_bRestartOnLevelChange;
	bool		m_bMasterServerRulesDirty;
	double		m_flLastMasterServerUpdateTime;
	bool		m_bLoadgame;			// handle connections specially
	char		m_szStartspot[64];
	int			num_edicts;
	int			max_edicts;
	edict_t		*edicts;			// Can array index now, edict_t is fixed
	void *edictchangeinfo; // IChangeInfoAccessor * HACK to allow backward compat since we can't change edict_t layout
	int			m_nMaxClientsLimit;    // Max allowed on server.
	bool		allowsignonwrites;
	bool	    dll_initialized;    // Have we loaded the game dll.
	bool		m_bIsLevelMainMenuBackground;	// true if the level running only as the background to the main menu
	CUtlVector<CEventInfo*>	m_TempEntities;		// temp entities
	bf_write			m_FullSendTables;
	CUtlMemory<byte>	m_FullSendTablesBuffer;
	/*CPrecacheItem	model_precache[ MAX_MODELS ];
	CPrecacheItem	generic_precache[ MAX_GENERIC ];
	CPrecacheItem	sound_precache[ MAX_SOUNDS ];
	CPrecacheItem	decal_precache[ MAX_BASE_DECALS ];
	INetworkStringTable *m_pModelPrecacheTable;	
	INetworkStringTable *m_pSoundPrecacheTable;
	INetworkStringTable *m_pGenericPrecacheTable;
	INetworkStringTable *m_pDecalPrecacheTable;
	CPureServerWhitelist *m_pPureServerWhitelist;*/
}

#endif // SVCLIENTSPROVIDER
