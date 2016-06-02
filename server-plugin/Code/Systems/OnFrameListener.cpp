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

#include "OnFrameListener.h"

/////////////////////////////////////////////////////////////////////////
// BaseFramedTester
/////////////////////////////////////////////////////////////////////////

OnFrameListener::FrameListenersListT OnFrameListener::m_framedTestersList;

OnFrameListener::OnFrameListener()
{
	m_filter = PLAYER_IN_TESTS;
}

void OnFrameListener::ProcessTestsOnFrame()
{
	PLAYERS_LOOP_RUNTIME_UNROLL(x)
	{
		if(x_ph->status >= GetFilter())
		{
			ProcessPlayerTestOnFrame(x_ph->playerClass);
		}
	}
	END_PLAYERS_LOOP_UNROLL(x)
}

void OnFrameListener::RegisterOnFrameListener(OnFrameListener* tester)
{
	m_framedTestersList.Add(tester);
}

void OnFrameListener::RemoveOnFrameListener(OnFrameListener* tester)
{
	m_framedTestersList.Remove(tester);
}

void OnFrameListener::OnFrame()
{
	FrameListenersListT::elem_t* it = m_framedTestersList.GetFirst();
	while(it != nullptr)
	{
		it->m_value->ProcessTestsOnFrame();
		it->m_value->ProcessOnFrame();

		it = it->m_next;
	}
}
