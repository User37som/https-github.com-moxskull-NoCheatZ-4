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
#include <ctime> // time_t
#include <cstdarg> // va
#include <stdio.h>

#include "Helpers.h" // edict, string, vector, iface

#include "Containers/bitbuf.h"
#include "Interfaces/edict.h"

#include "Preprocessors.h"
#include "Misc/MRecipientFilter.h"
#include "Misc/UserMsg.h"

extern unsigned int HashString(char const *);
extern unsigned int HashStringCaseless(char const *);

namespace Helpers
{
	SourceSdk::edict_t * m_EdictList = nullptr;
	SourceSdk::edict_t_csgo * m_EdictList_csgo = nullptr;
	int  m_edictCount = 0;
	int  m_clientMax = 0;

	void toLowerCase(basic_string &str)
	{
		std::locale loc;
		const int length = str.length();
		for(int i=0; i < length; ++i)
		{
			str[i] = std::tolower(str[i], loc);
		}
	}

	void xprintf(const char *fmt, ...)
	{
		va_list		argptr;
		static char		string[4096];

		va_start(argptr, fmt);
		vsnprintf(string, sizeof(string), fmt,argptr);
		va_end (argptr);

		basic_string finalString("[" NCZ_PLUGIN_NAME "] ");
		finalString.append(string);

		SourceSdk::InterfacesProxy::Call_LogPrint(finalString.c_str());

		writeToLogfile(getStrDateTime("%x %X : ").append(finalString));
	}

	basic_string getStrDateTime(const char *format)
	{
		time_t rawtime;
		struct tm * timeinfo;
		char date[256];
		time(&rawtime);
		timeinfo = localtime(&rawtime);
		strftime(date, sizeof(date), format, timeinfo);
		date[255] = '\0';
		return basic_string(date);
	}

	bool bStrEq(const char *sz1, const char *sz2, size_t start_offset, size_t length)
	{
		const size_t len = strlen(sz1);
		if(len != strlen(sz2)) return false;

		for(size_t x = start_offset, c = 0; x < len && c < length; ++x, ++c)
			if(sz1[x] != sz2[x]) return false;

		return true;
	}

	bool bstrneq(char const * s1, char const * s2, size_t len)
	{
		Assert(s1 && s2);

		do
		{
			if(*s1 != *s2) return false;
			--s1;
			--s2;
		}
		while(--len);

		return true;
	}

	bool bStriEq(const char *sz1, const char *sz2, size_t start_offset, size_t length)
	{
		const size_t len = strlen(sz1);
		if(len != strlen(sz2)) return false;

		char t1, t2;
		for(size_t x = start_offset, c = 0; x < len && c < length; ++x, ++c)
		{
			t1 = sz1[x];
			t2 = sz2[x];
			if(sz1[x] <= 'Z' && sz1[x] >= 'A')
				t1 = sz1[x] + 0x20;
			if(sz2[x] <= 'Z' && sz2[x] >= 'A')
				t2 = sz2[x] + 0x20;
			if(t1 != t2) return false;
		}

		return true;
	}

	bool bBytesEq(const char *sz1, const char *sz2, size_t start_offset, size_t length)
	{
		for(size_t x = start_offset, c = 0; c < length; ++x, ++c)
			if(sz1[x] != sz2[c]) return false;

		return true;
	}

	size_t GetUTF8Bytes(const char* const o_c)
	{
		uint8_t c = *(uint8_t *)o_c;
		if (c & (1<<7))
		{
			if (c & (1<<5))
			{
				if (c & (1<<4))
				{
					return 4;
				}
				return 3;
			}
			return 2;
		}
		return 1;
	}

	bool IsValidUTF8Char(const char* const o_c, const size_t bytes)
	{
		const char* str = o_c;

		if (bytes == 2)
		{
			if (*str == 0xC2)
			{
				++str;
				if (*str >= 0x80 && *str <= 0xA0) return false;
				else if (*str == 0xAD) return false;
			}
		}
		else if (bytes == 3)
		{
			if (*str == 0xE0)
			{
				if (*++str == 0xB8)
				{
					if (*++str == 0xB4) return false;
				}
			}
			else if (*str == 0xE1)
			{
				++str;
				if (*str == 0x85)
				{
					++str;
					if (*str == 0x9F || *str == 0xA0) return false;
				}
				else if (*str == 0x8D)
				{
					if (*++str == 0x9F) return false;
				}
				else if (*str == 0xA0)
				{
					++str;
					if (*str >= 0x8B && *str <= 0x8F) return false;
				}
			}
			else if (*str == 0xE2)
			{
				++str;
				if (*str == 0x80)
				{
					++str;
					if (*str >= 0x80 && *str <= 0x8F) return false;
					else if (*str >= 0xA8 && *str <= 0xAF) return false;
				}
				else if (*str == 0x81)
				{
					++str;
					if (*str >= 0x9F && *str <= 0xAF) return false;
				}
			}
			else if (*str == 0xE3)
			{
				++str;
				if (*str == 0x80)
				{
					if (*++str == 0x80) return false;
				}
				else if (*str == 0x85)
				{
					if (*++str == 0xA4) return false;
				}
			}
			else if (*str == 0xEF)
			{
				++str;
				if (*str == 0xBB)
				{
					if (*++str == 0xBF) return false;
				}
				else if (*str == 0xBE)
				{
					if (*++str == 0xA0) return false;
				}
				else if (*str == 0xBB)
				{
					if (*++str >= 0xB9) return false;
				}
			}
		}
	
		return true;
	}

	bool IsCharSpace(const char* const c)
	{
		if(GetUTF8Bytes(c) != 1) return false;
		else return isspace(*c) > 0;
	}

	// At this point, steamid is from a valid human player.
	SourceSdk::edict_t * getEdictFromSteamID(const char *SteamID)
	{
		const int imax = m_clientMax;
		for(int i = 1; i <= imax; i++)
		{
			SourceSdk::edict_t* pEntity = PEntityOfEntIndex(i);
			if(bStrEq(SourceSdk::InterfacesProxy::Call_GetPlayerNetworkIDString(pEntity), SteamID, STEAMID_MAXCHAR))
			{
				return pEntity;
			}
		}
		return nullptr;
	}

	// At this point, steamid if from a valid human player.
	int getIndexFromSteamID(const char *SteamID)
	{
		return IndexOfEdict(getEdictFromSteamID(SteamID));
	}

	SourceSdk::edict_t * PEntityOfEntIndex(const int iEntIndex)
	{
		if (SourceSdk::InterfacesProxy::m_game == SourceSdk::CounterStrikeGlobalOffensive)
		{
			uint8_t* edictlist = reinterpret_cast<uint8_t*>(static_cast<SourceSdk::CGlobalVars_csgo*>(SourceSdk::InterfacesProxy::Call_GetGlobalVars())->pEdicts);

			return (SourceSdk::edict_t *)( sizeof(SourceSdk::edict_t_csgo) * iEntIndex + edictlist );
		}
		else
		{
			return SourceSdk::InterfacesProxy::Call_PEntityOfEntIndex(iEntIndex);
		}
	}

	int IndexOfEdict(const SourceSdk::edict_t *pEdict)
	{
		if (SourceSdk::InterfacesProxy::m_game == SourceSdk::CounterStrikeGlobalOffensive)
		{
			return (int)((SourceSdk::edict_t_csgo*)(pEdict) - (static_cast<SourceSdk::edict_t_csgo*>(static_cast<SourceSdk::CGlobalVars_csgo*>(SourceSdk::InterfacesProxy::Call_GetGlobalVars())->pEdicts)));
		}
		else
		{
			return SourceSdk::InterfacesProxy::Call_IndexOfEdict(pEdict);
		}
	}

	int GetPlayerCount()
	{
		int count = 0;
		for(int index = FIRST_PLAYER_ENT_INDEX; index <= LAST_PLAYER_ENT_INDEX; ++index)
		{
			SourceSdk::edict_t * pEdict = Helpers::PEntityOfEntIndex(index);
			if(!isValidEdict(pEdict)) continue;
			const char * steamid = SourceSdk::InterfacesProxy::Call_GetPlayerNetworkIDString(pEdict);
			if(!steamid) continue;
			if(steamid[0] == 'B') continue;
			++count;
		}
		return count;
	}

	int GetMaxClients()
	{
		if (SourceSdk::InterfacesProxy::m_game == SourceSdk::CounterStrikeGlobalOffensive)
		{
			return static_cast<SourceSdk::CGlobalVars_csgo*>(SourceSdk::InterfacesProxy::Call_GetGlobalVars())->maxClients;
		}
		else
		{
			return static_cast<SourceSdk::CGlobalVars*>(SourceSdk::InterfacesProxy::Call_GetGlobalVars())->maxClients;
		}
	}

	int GetTickCount()
	{
		if (SourceSdk::InterfacesProxy::m_game == SourceSdk::CounterStrikeGlobalOffensive)
		{
			return static_cast<SourceSdk::CGlobalVars_csgo*>(SourceSdk::InterfacesProxy::Call_GetGlobalVars())->tickcount;
		}
		else
		{
			return static_cast<SourceSdk::CGlobalVars*>(SourceSdk::InterfacesProxy::Call_GetGlobalVars())->tickcount;
		}
	}

	basic_string format(const char *fmt, ...)
	{
		va_list		argptr;
		static char		string[FORMAT_STRING_BUFFER_SIZE];

		va_start(argptr, fmt);
		vsnprintf(string, sizeof(string), fmt, argptr);
		va_end (argptr);

		return tostring(string);
	}

	const char* boolToString(bool v)
	{
		if(v) return "Yes";
		return "No";
	}

	template<typename T>
	basic_string tostring(const T & toConvert)
	{
		std::locale loc;
		std::stringstream convertion;
		convertion << toConvert;
		return basic_string(convertion.str().c_str());
	}

	bool isOdd(const int value)
	{
		return (value & 1);
	}

	int getIndexFromUserID(const int userid)
	{
		int const ent_count = GetMaxClients();
		int i = 1;
		do
		{
			if(SourceSdk::InterfacesProxy::Call_GetPlayerUserid(PEntityOfEntIndex(i)) == userid)
			{
				return i;
			}
		} while(++i <= ent_count);
		return -1;
	}

	bool isValidEdict(const SourceSdk::edict_t * const p_entity)
	{
		return p_entity != nullptr && !p_entity->IsFree();
	}

	bool IsInt(const float value)
	{
		float n;
		if(std::modf(value, &n) == 0.0f) return true;
		return false;
	}

	void FadeUser(SourceSdk::edict_t* const pEntity, const short time)
	{
		MRecipientFilter filter;
		filter.AddRecipient(IndexOfEdict(pEntity));

		if (SourceSdk::InterfacesProxy::m_game == SourceSdk::CounterStrikeGlobalOffensive)
		{
			CCSUsrMsg_Fade* pBuffer = (CCSUsrMsg_Fade *)g_Cstrike15UsermessageHelpers.GetPrototype(CS_UM_Fade)->New();
			pBuffer->set_duration((time > 0) ? time : 50);
			pBuffer->set_hold_time((time > 0) ? 1000 : 0);
			pBuffer->set_flags(0x11);
			CMsgRGBA* clr = new CMsgRGBA();
			clr->set_r(0xFF);
			clr->set_g(0xFF);
			clr->set_b(0xFF);
			clr->set_a(0xFF);
			pBuffer->set_allocated_clr(clr);
			SourceSdk::InterfacesProxy::Call_SendUserMessage(&filter, CS_UM_Fade, *pBuffer);
			SourceSdk::InterfacesProxy::Call_MessageEnd();
			delete pBuffer;
			//delete clr; It is already deleted by the dctor of CCSUsrMsg_Fade
		}
		else
		{
			SourceSdk::bf_write *pBuffer = SourceSdk::InterfacesProxy::Call_UserMessageBegin(&filter, /*eUserMsg::Fade*/ 12);
			SourceSdk::BfWriteShort(pBuffer, (time > 0) ? time : 50);
			SourceSdk::BfWriteShort(pBuffer, (time > 0) ? 1000 : 0);
			SourceSdk::BfWriteShort(pBuffer, 0x11);   // FFADE_IN FFADE_PURGE
			SourceSdk::BfWriteShort(pBuffer, 0xFFFF); // r g
			SourceSdk::BfWriteShort(pBuffer, 0xFFFF); // b a

			SourceSdk::InterfacesProxy::Call_MessageEnd();
		}
	}
}

void Helpers::tell(SourceSdk::edict_t *pEntity, const basic_string& message)
{
	SourceSdk::IPlayerInfo * const player = static_cast<SourceSdk::IPlayerInfo *>(SourceSdk::InterfacesProxy::Call_GetPlayerInfo(pEntity));
	if (player)
	{
		if (player->IsConnected())
		{
			const int ent_id = Helpers::IndexOfEdict(pEntity);
			MRecipientFilter filter;
			filter.AddRecipient(ent_id);

			if (SourceSdk::InterfacesProxy::m_game == SourceSdk::CounterStrikeGlobalOffensive)
			{
				CCSUsrMsg_SayText* pBuffer = (CCSUsrMsg_SayText *)g_Cstrike15UsermessageHelpers.GetPrototype(CS_UM_SayText)->New();
				pBuffer->set_ent_idx(ent_id);
				pBuffer->set_text(message.c_str());
				pBuffer->set_chat(true);
				SourceSdk::InterfacesProxy::Call_SendUserMessage(&filter, CS_UM_SayText, *pBuffer);
				SourceSdk::InterfacesProxy::Call_MessageEnd();
				delete pBuffer;
			}
			else
			{
				SourceSdk::bf_write *pBuffer = SourceSdk::InterfacesProxy::Call_UserMessageBegin(&filter, 3);
				SourceSdk::BfWriteByte(pBuffer, ent_id);
				SourceSdk::BfWriteString(pBuffer, message.c_str());
				SourceSdk::InterfacesProxy::Call_MessageEnd();
			}
		}
	}
}

void Helpers::noTell(const SourceSdk::edict_t *pEntity, const basic_string& msg)
{
	int maxclients;
	if (SourceSdk::InterfacesProxy::m_game == SourceSdk::CounterStrikeGlobalOffensive)
	{
		maxclients = static_cast<SourceSdk::CGlobalVars_csgo*>(SourceSdk::InterfacesProxy::Call_GetGlobalVars())->maxClients;
	}
	else
	{
		maxclients = static_cast<SourceSdk::CGlobalVars*>(SourceSdk::InterfacesProxy::Call_GetGlobalVars())->maxClients;
	}
	for (int i=1; i <= maxclients; i++)
	{
		SourceSdk::edict_t * ent_id = Helpers::PEntityOfEntIndex(i);
		if(ent_id == pEntity) continue;

		SourceSdk::IPlayerInfo * const player = static_cast<SourceSdk::IPlayerInfo *>(SourceSdk::InterfacesProxy::Call_GetPlayerInfo(ent_id));

		if (player)
		{
			if (player->IsConnected())
			{
				Helpers::tell(ent_id, msg);
			}
		}
	}
}

void Helpers::chatprintf(const basic_string& msg)
{
	int maxclients;
	if (SourceSdk::InterfacesProxy::m_game == SourceSdk::CounterStrikeGlobalOffensive)
	{
		maxclients = static_cast<SourceSdk::CGlobalVars_csgo*>(SourceSdk::InterfacesProxy::Call_GetGlobalVars())->maxClients;
	}
	else
	{
		maxclients = static_cast<SourceSdk::CGlobalVars*>(SourceSdk::InterfacesProxy::Call_GetGlobalVars())->maxClients;
	}
	for (int i=1; i <= maxclients; i++)
	{
		SourceSdk::edict_t * ent_id = Helpers::PEntityOfEntIndex(i);
		SourceSdk::IPlayerInfo * const player = static_cast<SourceSdk::IPlayerInfo *>(SourceSdk::InterfacesProxy::Call_GetPlayerInfo(ent_id));

		if (player)
		{
			if (player->IsConnected())
			{
				Helpers::tell(ent_id, msg);
			}
		}
	}
}
