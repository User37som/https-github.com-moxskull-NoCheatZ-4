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
	typedef basic_slist<OnTickListener *> TickListenersListT;

private:
	static TickListenersListT m_tickTestersList;

	SlotStatus m_filter;

public:
	OnTickListener();

	virtual ~OnTickListener(){};

	/* Appelé par le plugin à chaque frame
	   Permet d'appeler les classes filles qui sont à l'écoute */
	static void RT_OnTick(double const & curtime);

protected:
	virtual void RT_ProcessOnTick(double const & curtime) = 0;

	/* Permet de se mettre à l'écoute de l'événement, appelé par Load/Unload des testeurs */
	static void RegisterOnTickListener(OnTickListener const * const listener);

	static void RemoveOnTickListener(OnTickListener const * const listener);
};

#endif // ONTICKLISTENER
