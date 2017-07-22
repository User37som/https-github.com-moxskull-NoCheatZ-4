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

#ifndef THINKPOSTHOOKLISTENER_H
#define THINKPOSTHOOKLISTENER_H

#include "Preprocessors.h"
#include "Hook.h"

// The function declaration we will call
typedef void ( HOOKFN_EXT *PostThink_t )( void * const );

class ThinkPostHookListener
{
	typedef HookListenersList<ThinkPostHookListener> ListenersList_t;

private:
	static ListenersList_t m_listeners;

public:
	ThinkPostHookListener ();
	virtual ~ThinkPostHookListener ();

	/*
	Hook and unhook functions.
	FIXME : May not works well with others plugins ...
	*/

	/*
	Hook function.
	Needs a valid player in game, could be a bot.

	This is a single instance hook. The plugin calls this when there is at least one player in game.
	*/
	static void HookThinkPost ( SourceSdk::edict_t const * const entity );

protected:
	static void RegisterThinkPostHookListener ( ThinkPostHookListener const * const listener );
	static void RemoveThinkPostHookListener ( ThinkPostHookListener const * const listener );

	virtual void RT_ThinkPostCallback ( SourceSdk::edict_t const * const ) = 0;

private:
#ifdef GNUC
	static void HOOKFN_INT RT_nThinkPost ( void * const baseentity );
#else
	static void HOOKFN_INT RT_nThinkPost ( void * const baseentity, void * const );
#endif
};

extern HookGuard<ThinkPostHookListener> g_HookGuardThinkPostHookListener;

#endif // THINKPOSTHOOKLISTENER_H
