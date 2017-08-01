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

#include "SpamChangeNameTester.h"

#include <stdio.h>

#include "Interfaces/InterfacesProxy.h"

#include "Players/NczPlayerManager.h"
#include "Misc/Helpers.h"
#include "Systems/BanRequest.h"

SpamChangeNameTester::SpamChangeNameTester () :
	BaseDynamicSystem ( "SpamChangeNameTester" ),
	SourceSdk::IGameEventListener002 (),
	OnTickListener (),
	playerdata_class (),
	Singleton ()
{}

SpamChangeNameTester::~SpamChangeNameTester ()
{
	Unload ();
}

void SpamChangeNameTester::Init ()
{
	InitDataStruct ();
}

void SpamChangeNameTester::Load ()
{
	for( PlayerHandler::iterator it ( PlayerHandler::begin () ); it != PlayerHandler::end (); ++it )
	{
		ResetPlayerDataStructByIndex ( it.GetIndex () );
	}

	SourceSdk::InterfacesProxy::GetGameEventManager ()->AddListener ( this, "player_changename", true );
	OnTickListener::RegisterOnTickListener ( this );
}

void SpamChangeNameTester::Unload ()
{
	OnTickListener::RemoveOnTickListener ( this );
	SourceSdk::InterfacesProxy::GetGameEventManager ()->RemoveListener ( this );
}

bool SpamChangeNameTester::GotJob () const
{
	// Create a filter
	ProcessFilter::HumanAtLeastConnecting const filter_class;
	// Initiate the iterator at the first match in the filter
	PlayerHandler::iterator it ( &filter_class );
	// Return if we have job to do or not ...
	return it != PlayerHandler::end ();
}

bool IsNameValid ( const char* const o_name )
{
	const char* name ( o_name );

	const size_t len ( strlen ( name ) );

	if( len < 1 || *name == '&' || Helpers::IsCharSpace ( name ) || Helpers::IsCharSpace ( name + len - 1 ) )
		return false;

	do
	{
		const size_t bytes ( Helpers::GetUTF8Bytes ( name ) );

		if( bytes > 1 )
		{
			if( !Helpers::IsValidUTF8Char ( name, bytes ) )
				return false;

			name += bytes - 1;
			continue;
		}

		if( *name < 32 || *name == '%' || *name == 0x7F )
			return false;
	}
	while( ( size_t ) ( ++name - o_name ) < len );

	return true;
}

void SpamChangeNameTester::FireGameEvent ( SourceSdk::IGameEvent* ev )
{
	PlayerHandler::iterator ph ( g_NczPlayerManager.GetPlayerHandlerByUserId ( ev->GetInt ( "userid", 0 ) ) );

	if( ph < SlotStatus::PLAYER_CONNECTED ) return;

	ChangeNameInfo* const pInfo ( GetPlayerDataStruct ( *ph ) );

	++( pInfo->namechange_count );
}

void SpamChangeNameTester::RT_ProcessOnTick (double const & curtime )
{
	ProcessFilter::HumanAtLeastConnected filter_class;
	for( PlayerHandler::iterator ph ( &filter_class ); ph != PlayerHandler::end (); ph+=&filter_class )
	{
		ChangeNameInfo* const pInfo ( GetPlayerDataStruct ( *ph ) );

		{
			if (pInfo->namechange_count >= 5)
			{
				if (pInfo->next_namechange_test < curtime)
				{
					g_BanRequest.KickNow(*ph, "Kicked for namechange spamming");
				}
				else
				{
					ResetPlayerDataStruct(*ph);
					pInfo->next_namechange_test = curtime + 20.0;
				}
			}
		}
	}
}

void SpamChangeNameTester::ClientConnect ( bool *bAllowConnect, SourceSdk::edict_t const * const pEntity, const char *pszName, const char *pszAddress, char *reject, int maxrejectlen )
{
	if( !*bAllowConnect || !IsActive() ) return;

	if( !IsNameValid ( pszName ) )
	{
		*bAllowConnect = false;
		strncpy ( reject, "Your name is not valid", 23 );
	}
}

SpamChangeNameTester g_SpamChangeNameTester;
