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

#ifndef BHOPBLOCKER_H
#define BHOPBLOCKER_H

#include "Systems/BaseSystem.h"
#include "Players/temp_PlayerDataStruct.h"
#include "Hooks/PlayerRunCommandHookListener.h"
#include "Systems/OnTickListener.h"
#include "Misc/temp_singleton.h"

struct JmpInfo
{
	int jmp_up_tick;
	bool has_been_blocked;

	JmpInfo ()
	{
		jmp_up_tick = 0;
		has_been_blocked = false;
	};
	JmpInfo ( const JmpInfo& other )
	{
		jmp_up_tick = other.jmp_up_tick;
		has_been_blocked = other.has_been_blocked;
	};
};

class BhopBlocker :
	public BaseDynamicSystem,
	public PlayerDataStructHandler<JmpInfo>,
	public PlayerRunCommandHookListener,
	public Singleton<BhopBlocker>
{
	typedef Singleton<BhopBlocker> singleton_class;
	typedef PlayerDataStructHandler<JmpInfo> playerdatahandler_class;

public:
	BhopBlocker ();
	virtual ~BhopBlocker () final;

	virtual void Init () override final;

	virtual void Load () override final;

	virtual void Unload () override final;

	virtual bool GotJob () const override final;

	virtual PlayerRunCommandRet RT_PlayerRunCommandCallback ( PlayerHandler::const_iterator ph, void * const cmd, void * const old_cmd ) override final;
};

#endif // BHOPBLOCKER_H
