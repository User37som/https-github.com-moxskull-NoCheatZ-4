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

#include <locale> // tolower, locale
#include <sstream> // stringstream
#include <cmath> // modf
#include <cstdarg> // va
#include <stdio.h>

#include "Helpers.h" // edict, string, vector, iface

#include "Containers/bitbuf.h"
#include "Interfaces/edict.h"
#include "Interfaces/InterfacesProxy.h"

#include "Preprocessors.h"
#include "Misc/MRecipientFilter.h"
#include "Misc/UserMsg.h"
#include "Systems/Logger.h" // SpewAssert

namespace Helpers
{
	SourceSdk::edict_t * m_EdictList ( nullptr );
	SourceSdk::edict_t_csgo * m_EdictList_csgo ( nullptr );
	int  m_edictCount ( 0 );
	int  m_clientMax ( 0 );

	const char * getStrDateTime ( const char *format )
	{
		time_t rawtime;
		struct tm * timeinfo;
		static char date[ 256 ];
		time ( &rawtime );
		timeinfo = localtime ( &rawtime );
		strftime ( date, 256, format, timeinfo );
		return date;
	}

	unsigned int g_nRandomValues[ 256 ] =
	{
		238,	164,	191,	168,	115,	 16,	142,	 11,	213,	214,	 57,	151,	248,	252,	 26,	198,
		13,	105,	102,	 25,	 43,	 42,	227,	107,	210,	251,	 86,	 66,	 83,	193,	126,	108,
		131,	  3,	 64,	186,	192,	 81,	 37,	158,	 39,	244,	 14,	254,	 75,	 30,	  2,	 88,
		172,	176,	255,	 69,	  0,	 45,	116,	139,	 23,	 65,	183,	148,	 33,	 46,	203,	 20,
		143,	205,	 60,	197,	118,	  9,	171,	 51,	233,	135,	220,	 49,	 71,	184,	 82,	109,
		36,	161,	169,	150,	 63,	 96,	173,	125,	113,	 67,	224,	 78,	232,	215,	 35,	219,
		79,	181,	 41,	229,	149,	153,	111,	217,	 21,	 72,	120,	163,	133,	 40,	122,	140,
		208,	231,	211,	200,	160,	182,	104,	110,	178,	237,	 15,	101,	 27,	 50,	 24,	189,
		177,	130,	187,	 92,	253,	136,	100,	212,	 19,	174,	 70,	 22,	170,	206,	162,	 74,
		247,	  5,	 47,	 32,	179,	117,	132,	195,	124,	123,	245,	128,	236,	223,	 12,	 84,
		54,	218,	146,	228,	157,	 94,	106,	 31,	 17,	 29,	194,	 34,	 56,	134,	239,	246,
		241,	216,	127,	 98,	  7,	204,	154,	152,	209,	188,	 48,	 61,	 87,	 97,	225,	 85,
		90,	167,	155,	112,	145,	114,	141,	 93,	250,	  4,	201,	156,	 38,	 89,	226,	196,
		1,	235,	 44,	180,	159,	121,	119,	166,	190,	144,	 10,	 91,	 76,	230,	221,	 80,
		207,	 55,	 58,	 53,	175,	  8,	  6,	 52,	 68,	242,	 18,	222,	103,	249,	147,	129,
		138,	243,	 28,	185,	 62,	 59,	240,	202,	234,	 99,	 77,	 73,	199,	137,	 95,	165,
	};

	unsigned int HashString ( const char *pszKey )
	{
		const uint8_t *k = ( const uint8_t * ) pszKey;
		unsigned int even = 0,
			odd = 0,
			n;

		while( ( n = *k++ ) != 0 )
		{
			even = g_nRandomValues[ odd ^ n ];
			if( ( n = *k++ ) != 0 )
				odd = g_nRandomValues[ even ^ n ];
			else
				break;
		}

		return ( even << 8 ) | odd;
	}

	size_t GetUTF8Bytes ( const char* const o_c )
	{
		uint8_t c ( *( uint8_t * ) o_c );
		if( c & ( 1 << 7 ) )
		{
			if( c & ( 1 << 5 ) )
			{
				if( c & ( 1 << 4 ) )
				{
					return 4;
				}
				return 3;
			}
			return 2;
		}
		return 1;
	}

	int GetGameTickCount ()
	{
		if( SourceSdk::InterfacesProxy::m_game == SourceSdk::CounterStrikeGlobalOffensive )
		{
			return static_cast< SourceSdk::CGlobalVars_csgo* >( SourceSdk::InterfacesProxy::Call_GetGlobalVars () )->tickcount;
		}
		else
		{
			return static_cast< SourceSdk::CGlobalVars* >( SourceSdk::InterfacesProxy::Call_GetGlobalVars () )->tickcount;
		}
	}

	bool IsArgTrue(char const * v)
	{
		return (*v == '1' || stricmp(v, "yes") == 0 || stricmp(v, "enable") == 0 || stricmp(v, "true") == 0 || stricmp(v, "on") == 0);
	}

	bool IsArgFalse(char const * v)
	{
		return (*v == '0' || stricmp(v, "no") == 0 || stricmp(v, "disable") == 0 || stricmp(v, "false") == 0 || stricmp(v, "off") == 0);
	}

	bool IsValidUTF8Char ( const char* const o_c, const size_t bytes )
	{
		const char* str ( o_c );

		if( bytes == 2 )
		{
			if( *str == 0xC2 )
			{
				++str;
				if( *str >= 0x80 && *str <= 0xA0 ) return false;
				else if( *str == 0xAD ) return false;
			}
		}
		else if( bytes == 3 )
		{
			if( *str == 0xE0 )
			{
				if( *++str == 0xB8 )
				{
					if( *++str == 0xB4 ) return false;
				}
			}
			else if( *str == 0xE1 )
			{
				++str;
				if( *str == 0x85 )
				{
					++str;
					if( *str == 0x9F || *str == 0xA0 ) return false;
				}
				else if( *str == 0x8D )
				{
					if( *++str == 0x9F ) return false;
				}
				else if( *str == 0xA0 )
				{
					++str;
					if( *str >= 0x8B && *str <= 0x8F ) return false;
				}
			}
			else if( *str == 0xE2 )
			{
				++str;
				if( *str == 0x80 )
				{
					++str;
					if( *str >= 0x80 && *str <= 0x8F ) return false;
					else if( *str >= 0xA8 && *str <= 0xAF ) return false;
				}
				else if( *str == 0x81 )
				{
					++str;
					if( *str >= 0x9F && *str <= 0xAF ) return false;
				}
			}
			else if( *str == 0xE3 )
			{
				++str;
				if( *str == 0x80 )
				{
					if( *++str == 0x80 ) return false;
				}
				else if( *str == 0x85 )
				{
					if( *++str == 0xA4 ) return false;
				}
			}
			else if( *str == 0xEF )
			{
				++str;
				if( *str == 0xBB )
				{
					if( *++str >= 0xB9 ) return false;
				}
				else if( *str == 0xBE )
				{
					if( *++str == 0xA0 ) return false;
				}
			}
		}

		return true;
	}

	bool IsCharSpace ( const char* const c )
	{
		if( GetUTF8Bytes ( c ) != 1 ) return false;
		else return isspace ( (unsigned char)(*c) ) > 0;
	}

	SourceSdk::edict_t * edictOfUnknown ( void * unk )
	{
		if( SourceSdk::InterfacesProxy::m_game == SourceSdk::CounterStrikeGlobalOffensive )
		{
			SourceSdk::edict_t_csgo * cur ( PEntityOfEntIndex ( 0 ) );
			SourceSdk::edict_t_csgo * const max_cur ( cur + MAX_EDICTS );
			do
			{
				if( cur->m_pUnk == unk )
					return ( SourceSdk::edict_t * )cur;
			}
			while( ++cur <= max_cur );

			//LoggerAssert ( "Helpers::edictOfUnknown failed" && 0 ); // https://github.com/L-EARN/NoCheatZ-4/issues/131#issuecomment-306068870
			return nullptr;
		}
		else
		{
			SourceSdk::edict_t * cur ( PEntityOfEntIndex ( 0 ) );
			SourceSdk::edict_t * const max_cur ( cur + MAX_EDICTS );
			do
			{
				if( cur->m_pUnk == unk )
					return cur;
			}
			while( ++cur <= max_cur );

			//LoggerAssert ( "Helpers::edictOfUnknown failed" && 0 );
			return nullptr;
		}
	}

	SourceSdk::edict_t * PEntityOfEntIndex ( const int iEntIndex )
	{
		if( SourceSdk::InterfacesProxy::m_game == SourceSdk::CounterStrikeGlobalOffensive )
		{
			uint8_t* edictlist ( reinterpret_cast< uint8_t* >( static_cast< SourceSdk::CGlobalVars_csgo* >( SourceSdk::InterfacesProxy::Call_GetGlobalVars () )->pEdicts ) );

			return ( SourceSdk::edict_t * )( sizeof ( SourceSdk::edict_t_csgo ) * iEntIndex + edictlist );
		}
		else
		{
			return SourceSdk::InterfacesProxy::Call_PEntityOfEntIndex ( iEntIndex );
		}
	}

	int IndexOfEdict ( const SourceSdk::edict_t *pEdict )
	{
		if( SourceSdk::InterfacesProxy::m_game == SourceSdk::CounterStrikeGlobalOffensive )
		{
			return ( int ) ( ( SourceSdk::edict_t_csgo* )( pEdict ) - ( static_cast< SourceSdk::edict_t_csgo* >( static_cast< SourceSdk::CGlobalVars_csgo* >( SourceSdk::InterfacesProxy::Call_GetGlobalVars () )->pEdicts ) ) );
		}
		else
		{
			return SourceSdk::InterfacesProxy::Call_IndexOfEdict ( pEdict );
		}
	}

	int GetMaxClients ()
	{
		if( SourceSdk::InterfacesProxy::m_game == SourceSdk::CounterStrikeGlobalOffensive )
		{
			return static_cast< SourceSdk::CGlobalVars_csgo* >( SourceSdk::InterfacesProxy::Call_GetGlobalVars () )->maxClients;
		}
		else
		{
			return static_cast< SourceSdk::CGlobalVars* >( SourceSdk::InterfacesProxy::Call_GetGlobalVars () )->maxClients;
		}
	}

	int GetTickCount ()
	{
		if( SourceSdk::InterfacesProxy::m_game == SourceSdk::CounterStrikeGlobalOffensive )
		{
			return static_cast< SourceSdk::CGlobalVars_csgo* >( SourceSdk::InterfacesProxy::Call_GetGlobalVars () )->tickcount;
		}
		else
		{
			return static_cast< SourceSdk::CGlobalVars* >( SourceSdk::InterfacesProxy::Call_GetGlobalVars () )->tickcount;
		}
	}

	const char * format ( const char *fmt, ... )
	{
		va_list		argptr;
		static char		string[ FORMAT_STRING_BUFFER_SIZE ] = { '\0' } ;

		va_start ( argptr, fmt );
		vsnprintf ( string, FORMAT_STRING_BUFFER_SIZE, fmt, argptr );
		va_end ( argptr );

		return string;
	}

	const char* boolToString ( bool v )
	{
		if( v ) return "Yes";
		return "No";
	}

	template<typename T>
	basic_string tostring ( const T & toConvert )
	{
		//std::locale loc;
		std::stringstream convertion;
		convertion << toConvert;
		return basic_string ( convertion.str ().c_str () );
	}

	bool isOdd ( const int value )
	{
		return ( value & 1 );
	}

	bool isValidEdict ( const SourceSdk::edict_t * const p_entity )
	{
		return p_entity != nullptr && !p_entity->IsFree ();
	}

	bool IsInt ( const float value )
	{
		float n;
		if( std::modf ( value, &n ) == 0.0f ) return true;
		return false;
	}

	void FadeUser ( SourceSdk::edict_t const * const pEntity, const short time )
	{
		MRecipientFilter filter;
		filter.AddRecipient ( IndexOfEdict ( pEntity ) );

		if( SourceSdk::InterfacesProxy::m_game == SourceSdk::CounterStrikeGlobalOffensive )
		{
			CCSUsrMsg_Fade* pBuffer ( ( CCSUsrMsg_Fade * ) g_Cstrike15UsermessageHelpers.GetPrototype ( CS_UM_Fade )->New () );

			if( time > 0 )
			{
				pBuffer->set_duration ( time / 2 );
				pBuffer->set_hold_time ( time / 4 );
				pBuffer->set_flags ( 0x0011 );// FFADE_PURGE | FFADE_IN
			}
			else
			{
				pBuffer->set_duration ( 10 );
				pBuffer->set_hold_time ( 0 );
				pBuffer->set_flags ( 0x0011 ); // FFADE_PURGE | FFADE_IN
			}
			CMsgRGBA* clr ( new CMsgRGBA () );
#ifdef DEBUG // Have a different color in debug mode
			clr->set_r ( 0xFF );
			clr->set_g ( 0x00 );
			clr->set_b ( 0x00 );
#else
			clr->set_r ( 0xFF );
			clr->set_g ( 0xFF );
			clr->set_b ( 0xFF );
#endif
			clr->set_a ( 0xFF );
			pBuffer->set_allocated_clr ( clr );
			SourceSdk::InterfacesProxy::Call_SendUserMessage ( &filter, CS_UM_Fade, *pBuffer );
			delete pBuffer;
			//delete clr; It is already deleted by the dctor of CCSUsrMsg_Fade
		}
		else
		{
			SourceSdk::bf_write *pBuffer ( SourceSdk::InterfacesProxy::Call_UserMessageBegin ( &filter, /*eUserMsg::Fade*/ 12 ) );
			SourceSdk::BfWriteShort ( pBuffer, ( time > 0 ) ? time : 50 );
			SourceSdk::BfWriteShort ( pBuffer, ( time > 0 ) ? 1000 : 0 );
			SourceSdk::BfWriteShort ( pBuffer, 0x11 );   // FFADE_IN FFADE_PURGE
			SourceSdk::BfWriteShort ( pBuffer, 0xFFFF ); // r g
			SourceSdk::BfWriteShort ( pBuffer, 0xFFFF ); // b a

			SourceSdk::InterfacesProxy::Call_MessageEnd ();
		}
	}
}

void Helpers::tell ( SourceSdk::edict_t const * const pEntity, const basic_string& message )
{
	SourceSdk::IPlayerInfo * const player ( static_cast< SourceSdk::IPlayerInfo * >( SourceSdk::InterfacesProxy::Call_GetPlayerInfo ( const_cast< SourceSdk::edict_t*const >( pEntity ) ) ) );
	if( player )
	{
		if( player->IsConnected () )
		{
			const int ent_id ( Helpers::IndexOfEdict ( pEntity ) );
			MRecipientFilter filter;
			filter.AddRecipient ( ent_id );

			if( SourceSdk::InterfacesProxy::m_game == SourceSdk::CounterStrikeGlobalOffensive )
			{
				CCSUsrMsg_SayText* pBuffer ( ( CCSUsrMsg_SayText * ) g_Cstrike15UsermessageHelpers.GetPrototype ( CS_UM_SayText )->New () );
				pBuffer->set_ent_idx ( ent_id );
				pBuffer->set_text ( message.c_str () );
				pBuffer->set_chat ( true );
				SourceSdk::InterfacesProxy::Call_SendUserMessage ( &filter, CS_UM_SayText, *pBuffer );
				delete pBuffer;
			}
			else
			{
				SourceSdk::bf_write *pBuffer ( SourceSdk::InterfacesProxy::Call_UserMessageBegin ( &filter, 3 ) );
				SourceSdk::BfWriteByte ( pBuffer, ent_id );
				SourceSdk::BfWriteString ( pBuffer, message.c_str () );
				SourceSdk::InterfacesProxy::Call_MessageEnd ();
			}
		}
	}
}

void Helpers::noTell ( const SourceSdk::edict_t *pEntity, const basic_string& msg )
{
	int maxclients;
	if( SourceSdk::InterfacesProxy::m_game == SourceSdk::CounterStrikeGlobalOffensive )
	{
		maxclients = static_cast< SourceSdk::CGlobalVars_csgo* >( SourceSdk::InterfacesProxy::Call_GetGlobalVars () )->maxClients;
	}
	else
	{
		maxclients = static_cast< SourceSdk::CGlobalVars* >( SourceSdk::InterfacesProxy::Call_GetGlobalVars () )->maxClients;
	}
	for( int i = 1; i <= maxclients; i++ )
	{
		SourceSdk::edict_t * ent_id ( Helpers::PEntityOfEntIndex ( i ) );
		if( ent_id == pEntity ) continue;

		SourceSdk::IPlayerInfo * const player ( static_cast< SourceSdk::IPlayerInfo * >( SourceSdk::InterfacesProxy::Call_GetPlayerInfo ( ent_id ) ) );

		if( player )
		{
			if( player->IsConnected () )
			{
				Helpers::tell ( ent_id, msg );
			}
		}
	}
}

void Helpers::chatprintf ( const basic_string& msg )
{
	int maxclients;
	if( SourceSdk::InterfacesProxy::m_game == SourceSdk::CounterStrikeGlobalOffensive )
	{
		maxclients = static_cast< SourceSdk::CGlobalVars_csgo* >( SourceSdk::InterfacesProxy::Call_GetGlobalVars () )->maxClients;
	}
	else
	{
		maxclients = static_cast< SourceSdk::CGlobalVars* >( SourceSdk::InterfacesProxy::Call_GetGlobalVars () )->maxClients;
	}
	for( int i ( 1 ); i <= maxclients; i++ )
	{
		SourceSdk::edict_t * ent_id ( Helpers::PEntityOfEntIndex ( i ) );
		SourceSdk::IPlayerInfo * const player ( static_cast< SourceSdk::IPlayerInfo * >( SourceSdk::InterfacesProxy::Call_GetPlayerInfo ( ent_id ) ) );

		if( player )
		{
			if( player->IsConnected () )
			{
				Helpers::tell ( ent_id, msg );
			}
		}
	}
}
