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
	m_filter = SlotStatus::PLAYER_IN_TESTS;
}

void OnTickListener::RegisterOnTickListener(OnTickListener const * const tester)
{
	m_tickTestersList.Add(const_cast<OnTickListener*const>(tester));
}

void OnTickListener::RemoveOnTickListener(OnTickListener const * const tester)
{
	m_tickTestersList.Remove(const_cast<OnTickListener*const>(tester));
}

void OnTickListener::RT_OnTick(double const & curtime)
{
	TickListenersListT::elem_t* it = m_tickTestersList.GetFirst();
	while(it != nullptr)
	{
		it->m_value->RT_ProcessOnTick(curtime);

		it = it->m_next;
	}
}
