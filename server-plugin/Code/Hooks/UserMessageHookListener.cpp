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

#include "UserMessageHookListener.h"

#include "Interfaces/InterfacesProxy.h"

UserMessageHookListener::ListenersList_t UserMessageHookListener::m_listeners;
bool UserMessageHookListener::bypass ( false );
SourceSdk::bf_write* UserMessageHookListener::m_buffer ( nullptr );
SourceSdk::IRecipientFilter * UserMessageHookListener::m_filter;
int UserMessageHookListener::m_message_id(0);

HookGuard<UserMessageHookListener> g_HookGuardUserMessageHookListener;

UserMessageHookListener::UserMessageHookListener ()
{
}

UserMessageHookListener::~UserMessageHookListener ()
{
}

void UserMessageHookListener::HookUserMessage ()
{
	if( SourceSdk::InterfacesProxy::m_game == SourceSdk::CounterStrikeGlobalOffensive )
	{
		//HookInfo sendusermessage_info(SourceSdk::InterfacesProxy::m_engineserver, 45, (DWORD)nSendUserMessage);
		//HookGuard::GetInstance()->VirtualTableHook(sendusermessage_info);
	}
	else
	{
		HookInfo usermessagebegin_info ( SourceSdk::InterfacesProxy::m_engineserver, 43, ( DWORD ) RT_nUserMessageBegin );
		g_HookGuardUserMessageHookListener.VirtualTableHook ( usermessagebegin_info, "IVEngineserver::UserMessageBegin" );
		HookInfo messageend_info ( SourceSdk::InterfacesProxy::m_engineserver, 44, ( DWORD ) RT_nMessageEnd );
		g_HookGuardUserMessageHookListener.VirtualTableHook ( messageend_info, "IVEngineserver::MessageEnd" );
	}
}

void UserMessageHookListener::RegisterUserMessageHookListener ( UserMessageHookListener const * const listener )
{
	m_listeners.Add ( listener );
}

void UserMessageHookListener::RemoveUserMessageHookListener ( UserMessageHookListener const * const listener )
{
	m_listeners.Remove ( listener );
}

#ifdef GNUC
void HOOKFN_INT UserMessageHookListener::RT_nSendUserMessage ( SourceSdk::IVEngineServer023csgo const * const thisptr, SourceSdk::IRecipientFilter const & filter, int const message_id, google::protobuf::Message const & buffer )
#else
void HOOKFN_INT UserMessageHookListener::RT_nSendUserMessage ( SourceSdk::IVEngineServer023csgo const * const thisptr, void*, SourceSdk::IRecipientFilter const & filter, int const message_id, google::protobuf::Message const & buffer )
#endif
{
	LoggerAssert ( bypass == false );
	bypass = false;

	ListenersList_t::elem_t* it ( m_listeners.GetFirst () );
	while( it != nullptr )
	{
		bypass |= it->m_value.listener->RT_SendUserMessageCallback ( filter, message_id, buffer );

		it = it->m_next;
	}

	if( !bypass )
	{
		SourceSdk::InterfacesProxy::Call_SendUserMessage ( &const_cast< SourceSdk::IRecipientFilter& >( filter ), message_id, buffer );
	}
}

#ifdef GNUC
SourceSdk::bf_write* HOOKFN_INT UserMessageHookListener::RT_nUserMessageBegin ( SourceSdk::IVEngineServer023 const * const thisptr, SourceSdk::IRecipientFilter const * const filter, int const message_id )
#else
SourceSdk::bf_write* HOOKFN_INT UserMessageHookListener::RT_nUserMessageBegin ( SourceSdk::IVEngineServer023 const * const thisptr, void * const, SourceSdk::IRecipientFilter const * const filter, int const message_id )
#endif
{
	LoggerAssert ( bypass == false );
	bypass = false;

	ListenersList_t::elem_t* it ( m_listeners.GetFirst () );
	while( it != nullptr )
	{
		bypass |= it->m_value.listener->RT_UserMessageBeginCallback ( filter, message_id );

		it = it->m_next;
	}

	if( bypass )
	{
		m_buffer = new SourceSdk::bf_write;
		m_buffer->m_pData = new unsigned long[ 128 ];
		m_buffer->m_nDataBytes = m_buffer->m_nDataBits = m_buffer->m_iCurBit = m_buffer->m_bOverflow = 0;
		m_buffer->m_bAssertOnOverflow = true;
		m_buffer->m_pDebugName = "Intercepted Buffer";
		return m_buffer;
	}
	else
	{
		m_filter = const_cast< SourceSdk::IRecipientFilter* >(filter);
		m_message_id = message_id;
		m_buffer = SourceSdk::InterfacesProxy::Call_UserMessageBegin(m_filter, m_message_id);
		return m_buffer;
	}
}

#ifdef GNUC
void HOOKFN_INT UserMessageHookListener::RT_nMessageEnd ( void * const thisptr )
#else
void HOOKFN_INT UserMessageHookListener::RT_nMessageEnd ( void * const thisptr, void * const )
#endif
{
	if( bypass )
	{
		if( m_buffer )
		{
			if( m_buffer->m_pData ) delete m_buffer->m_pData;
			delete m_buffer;
		}
		m_buffer = nullptr;
		bypass = false;
	}
	else
	{
		ListenersList_t::elem_t* it(m_listeners.GetFirst());
		while (it != nullptr)
		{
			it->m_value.listener->RT_MessageEndCallback(m_filter, m_message_id, m_buffer);

			it = it->m_next;
		}

		SourceSdk::InterfacesProxy::Call_MessageEnd ();
	}
}
