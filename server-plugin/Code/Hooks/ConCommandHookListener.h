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

#include "Preprocessors.h"
#include "Hook.h"

// The function declaration we will call
typedef void ( HOOKFN_EXT *Dispatch_t )( void * const cmd, SourceSdk::CCommand const & args );

class NczPlayer;

/*
	The base class used by a listener to hook some ConCommands.
*/
class ConCommandHookListener
{
	typedef HookListenersList<ConCommandHookListener> ConCommandListenersListT;
	typedef CUtlVector<void*> MyCommandsListT;

private:
	static ConCommandListenersListT m_listeners;

protected:
	MyCommandsListT m_mycommands;

public:
	ConCommandHookListener ();
	virtual ~ConCommandHookListener ();

protected:
	/*
	The listener will receive callbacks for all registered ConCommands in this function.
	It must be defined by the child.

	Returning true will bypass the original function.
	*/
	virtual bool RT_ConCommandCallback ( PlayerHandler::iterator ph, void * const cmd, SourceSdk::CCommand const & args ) = 0;

	/*
		Called by the child to register a ConCommand to be hooked.
		If the ConCommand is not already hooked, this function will call HookDispatch.
		It is safe to register multiple listeners for the same ConCommand,
		as well as calling this function with the same parameter multiple times.
	*/

	static void RegisterConCommandHookListener ( ConCommandHookListener const * const listener );

	/*
		Each ConCommand the listener registered will be reviewed to see if it can be unhooked,
		then it will be passed to UnhookDispatch for extra-check before actually unhooking, if possible.

		This function will clear the requested list of commands the listener needs.
		After that the listener is removed.
	*/
	static void RemoveConCommandHookListener ( ConCommandHookListener * const listener );

private:
	/*
		Hook and unhook functions.
		FIXME : May not works well with others plugins ...

		This is called automatically by the class
	*/
	static void HookDispatch ( void * const cmd );

	/*
		Our version of the ConCommand::Dispatch.
	*/
#ifdef GNUC
	static void HOOKFN_INT RT_nDispatch ( void * const cmd, SourceSdk::CCommand const &command );
#else
	static void HOOKFN_INT RT_nDispatch ( void * const cmd, void*, SourceSdk::CCommand const &command );
#endif
};

extern HookGuard<ConCommandHookListener> g_HookGuardConCommandHookListener;


#endif // CONCOMMANDHOOKLISTENER
