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

#ifndef SPAMCHANGENAMETESTER_H
#define SPAMCHANGENAMETESTER_H

#include "Preprocessors.h"
#include "Players/temp_PlayerDataStruct.h"
#include "Systems/BaseSystem.h"
#include "Interfaces/IGameEventManager/IGameEventManager.h"
#include "Systems/OnTickListener.h"
#include "Misc/temp_singleton.h"

struct ChangeNameInfo
{
	double next_namechange_test;
	size_t namechange_count;

	ChangeNameInfo ()
	{
		next_namechange_test = 0;
		namechange_count = 0;
	};
	ChangeNameInfo ( const ChangeNameInfo& other )
	{
		next_namechange_test = other.next_namechange_test;
		namechange_count = other.namechange_count;
	};
};

class SpamChangeNameTester :
	public BaseDynamicSystem,
	public SourceSdk::IGameEventListener002,
	public OnTickListener,
	public PlayerDataStructHandler<ChangeNameInfo>,
	public Singleton
{
	typedef PlayerDataStructHandler<ChangeNameInfo> playerdata_class;

public:
	SpamChangeNameTester ();

	virtual ~SpamChangeNameTester () final;

	virtual void Init () override final;

	virtual void Load () override final;

	virtual void Unload () override final;

	virtual bool GotJob () const override final;

	virtual void FireGameEvent ( SourceSdk::IGameEvent* ev ) override final;

	virtual void RT_ProcessOnTick (double const & curtime ) override final;

	void ClientConnect ( bool *bAllowConnect, SourceSdk::edict_t const * const pEntity, const char *pszName, const char *pszAddress, char *reject, int maxrejectlen );
};

extern SpamChangeNameTester g_SpamChangeNameTester;

#endif
