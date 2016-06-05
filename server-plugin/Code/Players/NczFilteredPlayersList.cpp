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

#include "NczFilteredPlayersList.h"

#include "Players/NczPlayerManager.h"

NczFilteredPlayersList::NczFilteredPlayersList()
{
	m_next_player = nullptr;
}

NczPlayer * const NczFilteredPlayersList::GetNextPlayer()
{
	// Met à jour le prochain pointeur en interne et retourne celui en cours
	NczPlayer* playerStor = m_next_player->playerClass;
	if(playerStor == nullptr) return nullptr; // Liste vide

	for(int index = playerStor->GetIndex()+1; index < MAX_PLAYERS; ++index)
	{
		m_next_player = NczPlayerManager::GetInstance()->GetPlayerHandlerByIndex(index);
		if(m_next_player->status < PLAYER_CONNECTED) continue;
		break;
	}
	if(playerStor == m_next_player->playerClass) return nullptr; // Fin de liste
	return playerStor;
}

void NczFilteredPlayersList::ResetNextPlayer()
{
	// Remet l'itération à zero
	m_next_player = nullptr;
	PLAYERS_LOOP_RUNTIME
	{
		if(ph->status < PLAYER_CONNECTED) continue;
		m_next_player = ph;
		break;
	}
	END_PLAYERS_LOOP
}

NczPlayer * const AsyncNczFilteredPlayersList::GetNextPlayer()
{
	// On re-vérifie le pointeur actuel
	if(m_next_player == nullptr) ResetNextPlayer();
	if(m_next_player == nullptr) return nullptr;
	if(m_next_player->status < PLAYER_CONNECTED)
	{
		ResetNextPlayer();
		if(m_next_player == nullptr) return nullptr;
	}
	if(m_next_player->status == INVALID) return nullptr;

	NczPlayer* playerStor = m_next_player->playerClass;
	
	int index = m_next_player->playerClass->GetIndex();
	int loop_count = 0;
	while((m_next_player = NczPlayerManager::GetInstance()->GetPlayerHandlerByIndex(++index)) == nullptr && (++loop_count) != MAX_PLAYERS)
	{
		index %= MAX_PLAYERS;
	}
	return playerStor;
}
