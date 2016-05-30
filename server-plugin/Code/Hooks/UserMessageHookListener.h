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

#include "Hook.h"
#include "Players/NczPlayerManager.h"
#include "Preprocessors.h"
#include "Interfaces/InterfacesProxy.h"
#include "Misc/UserMsg.h"

// The function declarations we will call
typedef void (HOOKFN_EXT *SendUserMessage_t)(SourceSdk::IVEngineServer023csgo*, IRecipientFilter &, int, google::protobuf::Message const &);
typedef void (HOOKFN_EXT *UserMessageBegin_t)(SourceSdk::IVEngineServer023* thisptr, bf_write*, IRecipientFilter*, int);
typedef void (HOOKFN_EXT *MessageEnd_t)(void*);

class UserMessageHookListener
{
	typedef HookListenersList<UserMessageHookListener> ListenersList_t;

public:
	UserMessageHookListener();
	virtual ~UserMessageHookListener();

	/*
	Hook and unhook functions.
	FIXME : May not works well with others plugins ...
	*/

	/*
		This must be called after InterfacesProxy::Load so we can have a passthru.
	*/
	static void HookUserMessage();

protected:
	static void RegisterUserMessageHookListener(UserMessageHookListener* listener);
	static void RemoveUserMessageHookListener(UserMessageHookListener* listener);

	virtual bool SendUserMessageCallback(SourceSdk::IRecipientFilter &, int, google::protobuf::Message const &) = 0;
	virtual bool UserMessageBeginCallback(SourceSdk::IRecipientFilter*, int) = 0;

private:
	// Bypass current usermessage and incoming messageend based on what is the answer of listener.
	static bool bypass;
	static SourceSdk::bf_write* m_buffer;

#ifdef GNUC
	static void HOOKFN_INT nSendUserMessage(SourceSdk::IVEngineServer023csgo* thisptr, SourceSdk::IRecipientFilter &, int, google::protobuf::Message const &);
	static SourceSdk::bf_write* HOOKFN_INT nUserMessageBegin(SourceSdk::IVEngineServer023* thisptr, SourceSdk::IRecipientFilter*, int);
	static void HOOKFN_INT nMessageEnd(void* thisptr);
#else
	static void HOOKFN_INT nSendUserMessage(SourceSdk::IVEngineServer023csgo* thisptr, void*, SourceSdk::IRecipientFilter &, int, google::protobuf::Message const &);
	static SourceSdk::bf_write* HOOKFN_INT nUserMessageBegin(SourceSdk::IVEngineServer023* thisptr, void*, SourceSdk::IRecipientFilter*, int);
	static void HOOKFN_INT nMessageEnd(void* thisptr, void*);
#endif

	static ListenersList_t m_listeners;
};

#endif
