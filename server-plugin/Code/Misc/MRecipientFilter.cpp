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

#include "MRecipientFilter.h"

#include "Helpers.h" // edict, PEntityOfEntIndex

MRecipientFilter::MRecipientFilter(void)
{
}

MRecipientFilter::~MRecipientFilter(void)
{
}

int MRecipientFilter::GetRecipientCount() const
{
	return m_Recipients.Count();
}

int MRecipientFilter::GetRecipientIndex(int slot) const
{
    if ( slot < 0 || slot >= GetRecipientCount() )
        return -1;

    return m_Recipients[ slot ];
}

bool MRecipientFilter::IsInitMessage() const
{
    return false;
}

bool MRecipientFilter::IsReliable() const
{
    return false;
}

void MRecipientFilter::AddAllPlayers(int maxClients)
{
    m_Recipients.RemoveAll();
    for ( int i = 1; i <= maxClients; i++ )
    {
		SourceSdk::edict_t *pPlayer = Helpers::PEntityOfEntIndex(i);
        if ( !pPlayer || pPlayer->IsFree())
            continue;
        m_Recipients.AddToTail(i);
    }
}

void MRecipientFilter::AddRecipient( int iPlayer )
{
    if ( m_Recipients.Find( iPlayer ) != m_Recipients.InvalidIndex() )
        return;

    m_Recipients.AddToTail( iPlayer );
}
