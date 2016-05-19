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

#ifndef NCZFILTEREDPLAYERLIST
#define NCZFILTEREDPLAYERLIST

#include "NczPlayerManager.h"

/*
	Gives a list to Systems.
	May be used to iterate over all players in the filter specifyed
	in one frame.
*/
class NczFilteredPlayersList
{
public:
	NczFilteredPlayersList();
	virtual ~NczFilteredPlayersList(){};

	virtual NczPlayer* GetNextPlayer(); 
	virtual void ResetNextPlayer();

protected:
	PlayerHandler* m_nextPlayer;
};

/*
	Gives a list to Systems.
	May be used to iterate over all players in the filter specifyed
	in multiple frames.
	NOT THREAD SAFE.
*/
class AsyncNczFilteredPlayersList : public NczFilteredPlayersList
{
public:
	AsyncNczFilteredPlayersList() : NczFilteredPlayersList() {};
	virtual ~AsyncNczFilteredPlayersList(){};

	// Returns null only if no players are available
	virtual NczPlayer* GetNextPlayer();
};

#endif
