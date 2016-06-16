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

NczFilteredPlayersList::NczFilteredPlayersList() : m_next_player(PlayerHandler::end())
{
}

PlayerHandler::const_iterator NczFilteredPlayersList::GetNextPlayer()
{
	// Met à jour le prochain pointeur en interne et retourne celui en cours
	PlayerHandler::const_iterator playerStor = m_next_player;
	if(!playerStor) return PlayerHandler::end(); // Liste vide

	bool again = true;

	++m_next_player;
	do
	{
		if (m_next_player == PlayerHandler::end())
		{
			// second chance
			if (again)
			{
				m_next_player = PlayerHandler::begin();
				again = false;
				continue;
			}
			else
			{
				return PlayerHandler::end();
			}
		}
		++m_next_player;
	} while (m_next_player < PLAYER_CONNECTED);


	if(playerStor == m_next_player) return PlayerHandler::end(); // Fin de liste

	return playerStor;
}

void NczFilteredPlayersList::ResetNextPlayer()
{
	// Remet l'itération à zero
	m_next_player = PlayerHandler::begin();
	for (; m_next_player != PlayerHandler::end(); ++m_next_player)
	{
		if (m_next_player >= PLAYER_CONNECTED)
		{
			break;
		}
	}
}

PlayerHandler::const_iterator AsyncNczFilteredPlayersList::GetNextPlayer()
{
	// On re-vérifie le pointeur actuel
	if(!m_next_player) ResetNextPlayer();
	if(!m_next_player) return PlayerHandler::end();
	if(m_next_player < PLAYER_CONNECTED)
	{
		ResetNextPlayer();
		if(!m_next_player) return PlayerHandler::end();
	}
	if(m_next_player == INVALID) return PlayerHandler::end();

	PlayerHandler::const_iterator playerStor(m_next_player);
	
	int index = m_next_player->GetIndex();
	int loop_count = 0;
	while((!PlayerHandler::const_iterator(++index)) && (++loop_count) != MAX_PLAYERS)
	{
		index %= MAX_PLAYERS;
	}
	return playerStor;
}
