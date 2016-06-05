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

#include <limits>
#include <cstring> // memset, memcpy

#include "Preprocessors.h"
#include "NczPlayer.h"
#include "Misc/temp_singleton.h"
#include "Misc/Helpers.h"

#include "SdkPreprocessors.h"
#include "Interfaces/IGameEventManager/IGameEventManager.h"

struct PlayerHandler
{
	SlotStatus status;
	mutable NczPlayer* playerClass;
	float in_tests_time;

	PlayerHandler()
	{
		status = INVALID;
		playerClass = nullptr;
		in_tests_time = std::numeric_limits<float>::max();
	};
	PlayerHandler(const PlayerHandler& other)
	{
		memcpy(this, &other, sizeof(PlayerHandler));
	};
	PlayerHandler& operator=(const PlayerHandler& other)
	{
		memcpy(this, &other, sizeof(PlayerHandler));
		return *this;
	};

	void Reset()
	{
		if (playerClass) delete playerClass;
		playerClass = nullptr;
		status = INVALID;
		in_tests_time = std::numeric_limits<float>::max();
	}
};

class CCSPlayer;

/* Distribue et met à jour l'état des slots du serveur */
class NczPlayerManager :
	public SourceSdk::IGameEventListener002,
	public Singleton<NczPlayerManager>
{
	typedef  Singleton<NczPlayerManager> singleton_class;

public:
	NczPlayerManager();
	virtual ~NczPlayerManager() final;

	/* Force la mise à jour des slots en scannant la mémoire pour EdictList
	   S'inscrit aux événements pour mettre à jour les slots en temps réel */
	void LoadPlayerManager();
	void OnLevelInit();

	inline PlayerHandler const * const GetPlayerHandlerByIndex(int const slot) const;
	inline PlayerHandler const * const GetPlayerHandlerByUserId(int const userid) const;
	PlayerHandler const * const GetPlayerHandlerByBasePlayer(void* BasePlayer) const;
	PlayerHandler const * const GetPlayerHandlerBySteamID(const char * steamid) const;
	inline PlayerHandler const * const GetPlayerHandlerByEdict(SourceSdk::edict_t const * const pEdict) const;
	PlayerHandler const * const GetPlayerHandlerByName(const char * playerName) const;
	
	short GetPlayerCount(SlotStatus filter = INVALID, SlotFilterBehavior strict = STATUS_EQUAL_OR_BETTER) const;

	void ClientConnect(SourceSdk::edict_t* pEntity); // Bots don't call this ...
	void ClientActive(SourceSdk::edict_t* pEntity); // ... they call this at first
	void ClientDisconnect(SourceSdk::edict_t *pEntity);
	void FireGameEvent(SourceSdk::IGameEvent* ev);
	void DeclareKickedPlayer(int const slot);

	void Think();

	const int GetMaxIndex() const {return m_max_index;};

private:
	PlayerHandler FullHandlersList[MAX_PLAYERS+1];
	int m_max_index;
};

inline PlayerHandler const * const NczPlayerManager::GetPlayerHandlerByIndex(int const slot) const
{
	return (PlayerHandler const * const)(&FullHandlersList[slot]);
}

inline PlayerHandler const * const NczPlayerManager::GetPlayerHandlerByUserId(int const userid) const
{
	return GetPlayerHandlerByIndex(Helpers::getIndexFromUserID(userid));
}

inline PlayerHandler const* const NczPlayerManager::GetPlayerHandlerByEdict(SourceSdk::edict_t const * const pEdict) const
{
	return GetPlayerHandlerByIndex(Helpers::IndexOfEdict(pEdict));
}

/* Utilisé en interne pour initialiser le tableau, des petites fonctions
  Ajout d'une case supplémentaire à FullHandlersList pour pouvoir quitter proprement la boucle PLAYERS_LOOP_RUNTIME */
#define _PLAYERS_LOOP_INIT { \
		int x = 0; \
		PlayerHandler* ph = &(FullHandlersList[x]); \
		do{

/* Boucle classique pour les utilisations externes
   Donne la variable ph , x et maxcl dans la boucle */
#define PLAYERS_LOOP_RUNTIME { \
		int x = 1; \
		const int maxcl = NczPlayerManager::GetInstance()->GetMaxIndex(); \
		PlayerHandler const * ph = NczPlayerManager::GetInstance()->GetPlayerHandlerByIndex(x); \
		for(; x <= maxcl; ++x, ph = NczPlayerManager::GetInstance()->GetPlayerHandlerByIndex(x)){if(ph->status == INVALID) continue;

#define END_PLAYERS_LOOP  }}
#define _END_PLAYERS_LOOP_INIT  ++x;ph = &(FullHandlersList[x]);}while(x <= MAX_PLAYERS);}

#endif
