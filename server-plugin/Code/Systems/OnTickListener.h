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

#ifndef ONTICKLISTENER
#define ONTICKLISTENER

#include "Misc/temp_basiclist.h"
#include "Players/NczPlayerManager.h"

/////////////////////////////////////////////////////////////////////////
// BaseFramedTester
/////////////////////////////////////////////////////////////////////////

class OnTickListener
{
public:
	OnTickListener();
	virtual ~OnTickListener(){};

	/* Appelé par le plugin à chaque frame
	   Permet d'appeler les classes filles qui sont à l'écoute */
	static void OnTick();

	/* Filtre du testeur, peut/doit être redéfinit dans la classe fille */
	virtual SlotStatus GetFilter() const {return m_filter;};

protected:
	/* Appelé par OnFrame, sert à iétrer entre les différents joueurs en fonction du filtre */
	virtual void ProcessTestsOnTick();

	/* Appelé par ProcessTestsOnFrame() lorsque le joueur correspond au filtre */
	virtual void ProcessPlayerTestOnTick(NczPlayer* player) = 0;
	virtual void ProcessOnTick() = 0;

	/* Permet de se mettre à l'écoute de l'événement, appelé par Load/Unload des testeurs */
	static void RegisterOnTickListener(OnTickListener* listener);
	static void RemoveOnTickListener(OnTickListener* listener);

	SlotStatus m_filter;

private:
	typedef basic_slist<OnTickListener*> TickListenersListT;
	static TickListenersListT m_tickTestersList;
};

#endif // ONTICKLISTENER
