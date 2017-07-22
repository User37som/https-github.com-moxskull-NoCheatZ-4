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

#ifndef USERMESSAGEHOOKLISTENER_H
#define USERMESSAGEHOOKLISTENER_H

#include "Preprocessors.h"
#include "Hook.h"

namespace SourceSdk
{
	class IVEngineServer023csgo;
	class IVEngineServer023;
	class IRecipientFilter;
	struct bf_write;
}

namespace google
{
	namespace protobuf
	{
		class Message;
	}
}

// The function declarations we will call
typedef void ( HOOKFN_EXT *SendUserMessage_t )( SourceSdk::IVEngineServer023csgo const * const, SourceSdk::IRecipientFilter &, int, google::protobuf::Message const & );
typedef void ( HOOKFN_EXT *UserMessageBegin_t )( SourceSdk::IVEngineServer023 const * const thisptr, SourceSdk::bf_write const * const, SourceSdk::IRecipientFilter const * const, int const );
typedef void ( HOOKFN_EXT *MessageEnd_t )( void* );

class UserMessageHookListener
{
	typedef HookListenersList<UserMessageHookListener> ListenersList_t;

private:
	static ListenersList_t m_listeners;
	// Bypass current usermessage and incoming messageend based on what is the answer of listener.
	static bool bypass;
	static SourceSdk::bf_write* m_buffer;
	static SourceSdk::IRecipientFilter * m_filter;
	static int m_message_id;

public:
	UserMessageHookListener ();
	virtual ~UserMessageHookListener ();

	/*
	Hook and unhook functions.
	FIXME : May not works well with others plugins ...
	*/

	/*
		This must be called after InterfacesProxy::Load so we can have a passthru.
	*/
	static void HookUserMessage ();

protected:
	static void RegisterUserMessageHookListener ( UserMessageHookListener const * const listener );
	static void RemoveUserMessageHookListener ( UserMessageHookListener const * const listener );

	virtual bool RT_SendUserMessageCallback ( SourceSdk::IRecipientFilter const &, int const, google::protobuf::Message const & ) = 0;
	virtual bool RT_UserMessageBeginCallback ( SourceSdk::IRecipientFilter const * const, int const ) = 0;
	virtual void RT_MessageEndCallback(SourceSdk::IRecipientFilter const * const, int const, SourceSdk::bf_write* buffer) = 0;

private:

#ifdef GNUC
	static void HOOKFN_INT RT_nSendUserMessage ( SourceSdk::IVEngineServer023csgo const * const thisptr, SourceSdk::IRecipientFilter const &, int const, google::protobuf::Message const & );
	static SourceSdk::bf_write* HOOKFN_INT RT_nUserMessageBegin ( SourceSdk::IVEngineServer023 const * const thisptr, SourceSdk::IRecipientFilter const * const, int const );
	static void HOOKFN_INT RT_nMessageEnd ( void * const thisptr );
#else
	static void HOOKFN_INT RT_nSendUserMessage ( SourceSdk::IVEngineServer023csgo const * const thisptr, void * const, SourceSdk::IRecipientFilter const &, int const, google::protobuf::Message const & );
	static SourceSdk::bf_write* HOOKFN_INT RT_nUserMessageBegin ( SourceSdk::IVEngineServer023 const * const thisptr, void * const, SourceSdk::IRecipientFilter const * const, int const );
	static void HOOKFN_INT RT_nMessageEnd ( void * const thisptr, void * const );
#endif
};

extern HookGuard<UserMessageHookListener> g_HookGuardUserMessageHookListener;

#endif
