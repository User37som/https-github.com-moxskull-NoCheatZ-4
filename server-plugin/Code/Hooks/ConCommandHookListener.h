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

#ifndef CONCOMMANDHOOKLISTENER
#define CONCOMMANDHOOKLISTENER

#include "Console/convar.h"
#include "Console/icvar.h"
#include "Containers/utlvector.h"

#include "Misc/temp_basiclist.h"
#include "Players/NczPlayerManager.h"
#include "Hook.h"
#include "Preprocessors.h"

// The function declaration we will call
typedef void (HOOKFN_EXT *Dispatch_t)(void* cmd, SourceSdk::CCommand const & args );

class ConCommandHookListener;

typedef HookList<void> HookedCommandListT;
typedef HookListenersList<ConCommandHookListener> ConCommandListenersListT;
typedef CUtlVector<void*> MyCommandsListT;

/*
	The base class used by a listener to hook some ConCommands.
*/
class ConCommandHookListener
{
public:
	ConCommandHookListener();
	virtual ~ConCommandHookListener();

	static void UnhookDispatch();

protected:
	/*
		Called by the child to register a ConCommand to be hooked.
		If the ConCommand is not already hooked, this function will call HookDispatch.
		It is safe to register multiple listeners for the same ConCommand,
		as well as calling this function with the same parameter multiple times.
	*/
	static void RegisterConCommandHookListener(ConCommandHookListener* listener);

	/*
		Each ConCommand the listener registered will be reviewed to see if it can be unhooked,
		then it will be passed to UnhookDispatch for extra-check before actually unhooking, if possible.

		This function will clear the requested list of commands the listener needs.
		After that the listener is removed.
	*/
	static void RemoveConCommandHookListener(ConCommandHookListener* listener);
	
	/*
		The listener will receive callbacks for all registered ConCommands in this function.
		It must be defined by the child.

		Returning true will bypass the original function.
	*/
	virtual bool ConCommandCallback(NczPlayer* player, void* cmd, const SourceSdk::CCommand & args) = 0;

	MyCommandsListT m_mycommands;

private:
	/*
		Hook and unhook functions.
		FIXME : May not works well with others plugins ...

		This is called automatically by the class
	*/
	static void HookDispatch(void* cmd);

	/*
		Our version of the ConCommand::Dispatch.
	*/
#ifdef GNUC
	static void HOOKFN_INT nDispatch(void* cmd, SourceSdk::CCommand const &command );
#else
	static void HOOKFN_INT nDispatch(void* cmd, void*, SourceSdk::CCommand const &command );
#endif
	
	static ConCommandListenersListT m_listeners;
	static HookedCommandListT m_hooked_commands;
};

#endif // CONCOMMANDHOOKLISTENER
