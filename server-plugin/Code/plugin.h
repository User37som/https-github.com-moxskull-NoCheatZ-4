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

#ifndef CNOCHEATZ_PLUGIN
#define CNOCHEATZ_PLUGIN

#include "SdkPreprocessors.h"
#include "Interfaces/iserverplugin.h"

#include "Preprocessors.h"
#include "Misc/temp_Metrics.h"

class CNoCheatZPlugin: public SourceSdk::IServerPluginCallbacks
{
public:
	// IServerPluginCallbacks methods
	virtual bool			Load(SourceSdk::CreateInterfaceFn interfaceFactory, SourceSdk::CreateInterfaceFn gameServerFactory );
	virtual void			Unload( void );
	virtual void			Pause( void );
	virtual void			UnPause( void );
	virtual const char     *GetPluginDescription( void );      
	virtual void			LevelInit( char const *pMapName );
	virtual void			ServerActivate(SourceSdk::edict_t *pEdictList, int edictCount, int clientMax );
	virtual void			GameFrame( bool simulating );
	virtual void			LevelShutdown( void );
	virtual void			ClientActive(SourceSdk::edict_t *pEntity );
	virtual void            ClientFullyConnect(SourceSdk::edict_t *) {};
	virtual void			ClientDisconnect(SourceSdk::edict_t *pEntity );
	virtual void			ClientPutInServer(SourceSdk::edict_t *pEntity, char const *playername );
	virtual void			SetCommandClient( int index );
	virtual void			ClientSettingsChanged(SourceSdk::edict_t *pEdict );
	virtual SourceSdk::PLUGIN_RESULT	ClientConnect( bool *bAllowConnect, SourceSdk::edict_t *pEntity, const char *pszName, const char *pszAddress, char *reject, int maxrejectlen );
	virtual SourceSdk::PLUGIN_RESULT	ClientCommand(SourceSdk::edict_t *pEntity, const SourceSdk::CCommand &args );
	virtual SourceSdk::PLUGIN_RESULT	NetworkIDValidated( const char *pszUserName, const char *pszNetworkID );
	virtual void			OnQueryCvarValueFinished(SourceSdk::QueryCvarCookie_t iCookie, SourceSdk::edict_t *pPlayerEntity, SourceSdk::EQueryCvarValueStatus eStatus, const char *pCvarName, const char *pCvarValue );

	// added with version 3 of the interface.
	virtual void			OnEdictAllocated(SourceSdk::edict_t *edict );
	virtual void			OnEdictFreed( const SourceSdk::edict_t *edict  );

	int GetCommandIndex() const { return m_iClientCommandIndex; }

	CNoCheatZPlugin();
	~CNoCheatZPlugin();

private:
	CNoCheatZPlugin(const CNoCheatZPlugin & toCopy);
	CNoCheatZPlugin & operator =(const CNoCheatZPlugin & toCopy);
	int m_iClientCommandIndex;
	bool m_bAlreadyLoaded;

public:
	void * ncz_cmd_ptr;
	void * nocheatz_instance;
};

extern CNoCheatZPlugin g_NoCheatZPlugin;

#endif
