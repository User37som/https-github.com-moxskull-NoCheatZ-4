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
#include "Systems/OnTickListener.h"
#include "Players/temp_PlayerDataStruct.h"
#include "Hooks/PlayerRunCommandHookListener.h"
#include "Hooks/OnGroundHookListener.h"

struct JmpInfo
{
	int on_ground_tick;
	bool has_been_blocked;

	JmpInfo ()
	{
		on_ground_tick = 0;
		has_been_blocked = false;
	};
	JmpInfo ( const JmpInfo& other )
	{
		on_ground_tick = other.on_ground_tick;
		has_been_blocked = other.has_been_blocked;
	};
};

class BhopBlocker :
	public BaseDynamicSystem,
	public PlayerDataStructHandler<JmpInfo>,
	public PlayerRunCommandHookListener,
	public OnGroundHookListener,
	public Singleton<BhopBlocker>
{
	typedef Singleton<BhopBlocker> singleton_class;
	typedef PlayerDataStructHandler<JmpInfo> playerdatahandler_class;

private:
	void * convar_sv_enablebunnyhopping;
	void * convar_sv_autobunnyhopping;

public:
	BhopBlocker ();
	virtual ~BhopBlocker () final;

	virtual void Init () override final;

	virtual void Load () override final;

	virtual void Unload () override final;

	virtual bool GotJob () const override final;

	virtual PlayerRunCommandRet RT_PlayerRunCommandCallback ( PlayerHandler::const_iterator ph, void * const cmd, void * const old_cmd ) override final;

	virtual void RT_m_hGroundEntityStateChangedCallback ( PlayerHandler::const_iterator ph, bool const new_isOnGround ) override final;
};

#endif // BHOPBLOCKER_H
