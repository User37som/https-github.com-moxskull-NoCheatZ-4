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
}

void OnFrameListener::ProcessTestsOnFrame()
{
	for (PlayerHandler::const_iterator it = PlayerHandler::begin(); it != PlayerHandler::end(); ++it)
	{
		if(it >= GetFilter())
		{
			ProcessPlayerTestOnFrame(it);
		}
	}
}

void OnFrameListener::RegisterOnFrameListener(OnFrameListener const * const tester)
{
	m_framedTestersList.Add(const_cast<OnFrameListener*const>(tester));
}

void OnFrameListener::RemoveOnFrameListener(OnFrameListener const * const tester)
{
	m_framedTestersList.Remove(const_cast<OnFrameListener*const>(tester));
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
