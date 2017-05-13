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

#ifndef NCZPLAYERMANAGER
#define NCZPLAYERMANAGER

#include "PlayerHandler_impl.h"

/* Distribue et met à jour l'état des slots du serveur */
class NczPlayerManager :
	public SourceSdk::IGameEventListener002,
	public Singleton<NczPlayerManager>
{
	typedef  Singleton<NczPlayerManager> singleton_class;
	friend PlayerHandler;

private:
	static PlayerHandler FullHandlersList[ MAX_PLAYERS + 1 ];
	int m_max_index;
	
public:
	NczPlayerManager ();
	virtual ~NczPlayerManager ();

	virtual void FireGameEvent ( SourceSdk::IGameEvent* ev );

	/* Force la mise à jour des slots en scannant la mémoire pour EdictList
	   S'inscrit aux événements pour mettre à jour les slots en temps réel */
	void LoadPlayerManager ();
	void OnLevelInit ();

	inline PlayerHandler::const_iterator GetPlayerHandlerByIndex ( int const slot ) const;
	inline PlayerHandler::const_iterator GetPlayerHandlerByUserId ( int const userid ) const;
	PlayerHandler::const_iterator GetPlayerHandlerByBasePlayer ( void* BasePlayer ) const;
	PlayerHandler::const_iterator GetPlayerHandlerBySteamID ( const char * steamid ) const;
	inline PlayerHandler::const_iterator GetPlayerHandlerByEdict ( SourceSdk::edict_t const * const pEdict ) const;
	PlayerHandler::const_iterator GetPlayerHandlerByName ( const char * playerName ) const;

	short GetPlayerCount ( BaseProcessFilter const * const filter ) const;

	void ClientConnect ( SourceSdk::edict_t* pEntity ); // Bots don't call this ...
	void ClientActive ( SourceSdk::edict_t* pEntity ); // ... they call this at first
	void ClientDisconnect ( SourceSdk::edict_t *pEntity );
	void DeclareKickedPlayer ( int const slot );

	void RT_Think ( float const curtime );

	const int GetMaxIndex () const
	{
		return m_max_index;
	}
};

inline PlayerHandler::const_iterator NczPlayerManager::GetPlayerHandlerByIndex ( int const slot ) const
{
	return slot;
}

inline PlayerHandler::const_iterator NczPlayerManager::GetPlayerHandlerByUserId ( int const userid ) const
{
	for( PlayerHandler::const_iterator it ( PlayerHandler::begin () ); it != PlayerHandler::end (); ++it )
	{
		if( it )
		{
			if( it->m_playerinfo )
			{
				if( it->m_playerinfo->GetUserID () == userid )
					return it;
			}
		}
	}
	return PlayerHandler::end ();
}

inline PlayerHandler::const_iterator NczPlayerManager::GetPlayerHandlerByEdict ( SourceSdk::edict_t const * const pEdict ) const
{
	return Helpers::IndexOfEdict ( pEdict );
}

/* Utilisé en interne pour initialiser le tableau, des petites fonctions
  Ajout d'une case supplémentaire à FullHandlersList pour pouvoir quitter proprement la boucle PLAYERS_LOOP_RUNTIME */
#define _PLAYERS_LOOP_INIT { \
		int x = 0; \
		PlayerHandler* ph ( &(FullHandlersList[x])); \
		do{

#define _END_PLAYERS_LOOP_INIT  ++x;ph = &(FullHandlersList[x]);}while(x <= MAX_PLAYERS);}

#endif
