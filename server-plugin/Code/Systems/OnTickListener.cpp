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

#include "OnTickListener.h"
#include "plugin.h"

/////////////////////////////////////////////////////////////////////////
// BaseFramedTester
/////////////////////////////////////////////////////////////////////////

OnTickListener::TickListenersListT OnTickListener::m_tickTestersList;

OnTickListener::OnTickListener()
{
	m_filter = PLAYER_IN_TESTS;
}

void OnTickListener::ProcessTestsOnTick(float const curtime)
{
	PLAYERS_LOOP_RUNTIME_UNROLL(x)
	{
		if(x_ph->status >= GetFilter())
		{
			ProcessPlayerTestOnTick(x_ph->playerClass, curtime);
		}
	}
	END_PLAYERS_LOOP_UNROLL(x)
}

void OnTickListener::RegisterOnTickListener(OnTickListener* tester)
{
	m_tickTestersList.Add(tester);
}

void OnTickListener::RemoveOnTickListener(OnTickListener* tester)
{
	m_tickTestersList.Remove(tester);
}

void OnTickListener::OnTick()
{
	float const curtime = Plat_FloatTime();
	TickListenersListT::elem_t* it = m_tickTestersList.GetFirst();
	while(it != nullptr)
	{
		it->m_value->ProcessOnTick(curtime);
		it->m_value->ProcessTestsOnTick(curtime);

		it = it->m_next;
	}
}
