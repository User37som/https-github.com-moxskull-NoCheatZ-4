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
#include "Systems/AutoTVRecord.h"

namespace Helpers
{
	SourceSdk::edict_t * m_EdictList ( nullptr );
	SourceSdk::edict_t_csgo * m_EdictList_csgo ( nullptr );
	int  m_edictCount ( 0 );
	int  m_clientMax ( 0 );

	const char * getStrDateTime ( const char *format, time_t rawtime )
	{
		struct tm * timeinfo;
		static char date[ 256 ];
		time ( &rawtime );
		timeinfo = localtime ( &rawtime );
		strftime ( date, 256, format, timeinfo );
		return date;
	}

	void game_tm::Populate ()
	{
		m_systime = time ( nullptr );
		m_tickinterval = SourceSdk::InterfacesProxy::Call_GetTickInterval ();
		m_floattime = Plat_FloatTime ();

		__assume( m_tickinterval > 0.0f && m_tickinterval <= 1.0f );
		m_tickrate = ( size_t ) floorf ( 1.0f / m_tickinterval );

		if( AutoTVRecord::IsCreated () )
		{
			AutoTVRecord const * const tvi ( AutoTVRecord::GetInstance () );
			if( tvi->IsRecording () )
			{
				m_tv_tick = tvi->GetRecordTick ();
			}
			else
			{
				m_tv_tick = 0;
			}
		}
		else
		{
			m_tv_tick = 0;
		}
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

	uint32_t CRC32_Table[ 256 ] = {
		0x00000000L, 0x77073096L, 0xEE0E612CL,
		0x990951BAL, 0x076DC419L, 0x706AF48FL,
		0xE963A535L, 0x9E6495A3L, 0x0EDB8832L,
		0x79DCB8A4L, 0xE0D5E91EL, 0x97D2D988L,
		0x09B64C2BL, 0x7EB17CBDL, 0xE7B82D07L,
		0x90BF1D91L, 0x1DB71064L, 0x6AB020F2L,
		0xF3B97148L, 0x84BE41DEL, 0x1ADAD47DL,
		0x6DDDE4EBL, 0xF4D4B551L, 0x83D385C7L,
		0x136C9856L, 0x646BA8C0L, 0xFD62F97AL,
		0x8A65C9ECL, 0x14015C4FL, 0x63066CD9L,
		0xFA0F3D63L, 0x8D080DF5L, 0x3B6E20C8L,
		0x4C69105EL, 0xD56041E4L, 0xA2677172L,
		0x3C03E4D1L, 0x4B04D447L, 0xD20D85FDL,
		0xA50AB56BL, 0x35B5A8FAL, 0x42B2986CL,
		0xDBBBC9D6L, 0xACBCF940L, 0x32D86CE3L,
		0x45DF5C75L, 0xDCD60DCFL, 0xABD13D59L,
		0x26D930ACL, 0x51DE003AL, 0xC8D75180L,
		0xBFD06116L, 0x21B4F4B5L, 0x56B3C423L,
		0xCFBA9599L, 0xB8BDA50FL, 0x2802B89EL,
		0x5F058808L, 0xC60CD9B2L, 0xB10BE924L,
		0x2F6F7C87L, 0x58684C11L, 0xC1611DABL,
		0xB6662D3DL, 0x76DC4190L, 0x01DB7106L,
		0x98D220BCL, 0xEFD5102AL, 0x71B18589L,
		0x06B6B51FL, 0x9FBFE4A5L, 0xE8B8D433L,
		0x7807C9A2L, 0x0F00F934L, 0x9609A88EL,
		0xE10E9818L, 0x7F6A0DBBL, 0x086D3D2DL,
		0x91646C97L, 0xE6635C01L, 0x6B6B51F4L,
		0x1C6C6162L, 0x856530D8L, 0xF262004EL,
		0x6C0695EDL, 0x1B01A57BL, 0x8208F4C1L,
		0xF50FC457L, 0x65B0D9C6L, 0x12B7E950L,
		0x8BBEB8EAL, 0xFCB9887CL, 0x62DD1DDFL,
		0x15DA2D49L, 0x8CD37CF3L, 0xFBD44C65L,
		0x4DB26158L, 0x3AB551CEL, 0xA3BC0074L,
		0xD4BB30E2L, 0x4ADFA541L, 0x3DD895D7L,
		0xA4D1C46DL, 0xD3D6F4FBL, 0x4369E96AL,
		0x346ED9FCL, 0xAD678846L, 0xDA60B8D0L,
		0x44042D73L, 0x33031DE5L, 0xAA0A4C5FL,
		0xDD0D7CC9L, 0x5005713CL, 0x270241AAL,
		0xBE0B1010L, 0xC90C2086L, 0x5768B525L,
		0x206F85B3L, 0xB966D409L, 0xCE61E49FL,
		0x5EDEF90EL, 0x29D9C998L, 0xB0D09822L,
		0xC7D7A8B4L, 0x59B33D17L, 0x2EB40D81L,
		0xB7BD5C3BL, 0xC0BA6CADL, 0xEDB88320L,
		0x9ABFB3B6L, 0x03B6E20CL, 0x74B1D29AL,
		0xEAD54739L, 0x9DD277AFL, 0x04DB2615L,
		0x73DC1683L, 0xE3630B12L, 0x94643B84L,
		0x0D6D6A3EL, 0x7A6A5AA8L, 0xE40ECF0BL,
		0x9309FF9DL, 0x0A00AE27L, 0x7D079EB1L,
		0xF00F9344L, 0x8708A3D2L, 0x1E01F268L,
		0x6906C2FEL, 0xF762575DL, 0x806567CBL,
		0x196C3671L, 0x6E6B06E7L, 0xFED41B76L,
		0x89D32BE0L, 0x10DA7A5AL, 0x67DD4ACCL,
		0xF9B9DF6FL, 0x8EBEEFF9L, 0x17B7BE43L,
		0x60B08ED5L, 0xD6D6A3E8L, 0xA1D1937EL,
		0x38D8C2C4L, 0x4FDFF252L, 0xD1BB67F1L,
		0xA6BC5767L, 0x3FB506DDL, 0x48B2364BL,
		0xD80D2BDAL, 0xAF0A1B4CL, 0x36034AF6L,
		0x41047A60L, 0xDF60EFC3L, 0xA867DF55L,
		0x316E8EEFL, 0x4669BE79L, 0xCB61B38CL,
		0xBC66831AL, 0x256FD2A0L, 0x5268E236L,
		0xCC0C7795L, 0xBB0B4703L, 0x220216B9L,
		0x5505262FL, 0xC5BA3BBEL, 0xB2BD0B28L,
		0x2BB45A92L, 0x5CB36A04L, 0xC2D7FFA7L,
		0xB5D0CF31L, 0x2CD99E8BL, 0x5BDEAE1DL,
		0x9B64C2B0L, 0xEC63F226L, 0x756AA39CL,
		0x026D930AL, 0x9C0906A9L, 0xEB0E363FL,
		0x72076785L, 0x05005713L, 0x95BF4A82L,
		0xE2B87A14L, 0x7BB12BAEL, 0x0CB61B38L,
		0x92D28E9BL, 0xE5D5BE0DL, 0x7CDCEFB7L,
		0x0BDBDF21L, 0x86D3D2D4L, 0xF1D4E242L,
		0x68DDB3F8L, 0x1FDA836EL, 0x81BE16CDL,
		0xF6B9265BL, 0x6FB077E1L, 0x18B74777L,
		0x88085AE6L, 0xFF0F6A70L, 0x66063BCAL,
		0x11010B5CL, 0x8F659EFFL, 0xF862AE69L,
		0x616BFFD3L, 0x166CCF45L, 0xA00AE278L,
		0xD70DD2EEL, 0x4E048354L, 0x3903B3C2L,
		0xA7672661L, 0xD06016F7L, 0x4969474DL,
		0x3E6E77DBL, 0xAED16A4AL, 0xD9D65ADCL,
		0x40DF0B66L, 0x37D83BF0L, 0xA9BCAE53L,
		0xDEBB9EC5L, 0x47B2CF7FL, 0x30B5FFE9L,
		0xBDBDF21CL, 0xCABAC28AL, 0x53B39330L,
		0x24B4A3A6L, 0xBAD03605L, 0xCDD70693L,
		0x54DE5729L, 0x23D967BFL, 0xB3667A2EL,
		0xC4614AB8L, 0x5D681B02L, 0x2A6F2B94L,
		0xB40BBE37L, 0xC30C8EA1L, 0x5A05DF1BL,
		0x2D02EF8DL };

	uint32_t CRC32 ( void *pData, size_t size )
	{
		uint32_t res ( std::numeric_limits<uint32_t>::max() );
		uint8_t const *pszData ( reinterpret_cast<uint8_t const *>( pData ) );

		for( size_t i ( 0 ); i < size; ++i )
		{
			res = ( ( res >> 8 ) & 0x00FFFFFF ) ^ CRC32_Table[ ( res ^ *pszData++ ) & std::numeric_limits<uint8_t>::max () ];
		}

		return ( res ^ std::numeric_limits<uint32_t>::max () );
	}

	void CRC32_Digestive::Digest ( void const * data, size_t size )
	{
		uint8_t const *pszData ( reinterpret_cast<uint8_t const *>( data ) );

		for( ; size; --size, ++pszData )
		{
			m_res = ( ( m_res >> 8 ) & 0x00FFFFFF ) ^ CRC32_Table[ ( m_res ^ *pszData ) & std::numeric_limits<uint8_t>::max () ];
		}
	}

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
			SourceSdk::edict_t_csgo * cur ( PEntityOfEntIndex ( 65 ) );
			SourceSdk::edict_t_csgo * const max_cur ( cur + MAX_EDICTS - 65 );
			do
			{
				if( cur->m_pUnk == unk )
					return ( SourceSdk::edict_t * )cur;
			}
			while( ++cur <= max_cur );

			LoggerAssert ( "Helpers::edictOfUnknown failed" && 0 );
			return nullptr;
		}
		else
		{
			SourceSdk::edict_t * cur ( PEntityOfEntIndex ( 65 ) );
			SourceSdk::edict_t * const max_cur ( cur + MAX_EDICTS - 65 );
			do
			{
				if( cur->m_pUnk == unk )
					return cur;
			}
			while( ++cur <= max_cur );

			LoggerAssert ( "Helpers::edictOfUnknown failed" && 0 );
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
