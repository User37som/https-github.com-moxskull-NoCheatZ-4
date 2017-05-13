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

#include "temp_BaseDetection.h"

/*
	BaseDetection
*/

BaseDetection::BaseDetection ( PlayerHandler::const_iterator player, BaseSystem* tester, uint32_t udid ) :
	m_player ( player ),
	m_tester ( tester ),
	m_udid ( udid )
{

}

BaseDetection::~BaseDetection ()
{

}

PlayerHandler::const_iterator BaseDetection::GetPlayer () const
{
	return m_player;
}

BaseSystem * const BaseDetection::GetTester () const
{
	return m_tester;
}

uint32_t const BaseDetection::GetUDID () const
{
	return m_udid;
}

void BaseDetection::PerformCustomOutput () const
{

}

void BaseDetection::TakeAction ()
{

}
