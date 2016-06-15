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

#ifndef BANREQUEST_H
#define BANREQUEST_H

#include "Misc/temp_basiclist.h"
#include "Misc/temp_singleton.h"
#include "Preprocessors.h"
#include "Players/NczPlayer.h"
#include "Systems/TimerListener.h"

typedef struct PlayerBanRequest
{
	int ban_time;
	int userid;
	float request_time;
	const char * kick_message;
	char player_name[24];
	char steamid[24];
	char ip[24];
	char identity[64];

	PlayerBanRequest()
	{
		ban_time=userid=0;
		request_time=0.0;
		kick_message=nullptr;
		int x = 0;
		do {player_name[x]=steamid[x]=ip[x]=identity[x]='\0';} while(++x < 24);
		do {identity[x]='\0';} while(++x < 64); 
	};
	PlayerBanRequest(int id)
	{
		userid = id;
	};
	PlayerBanRequest(const PlayerBanRequest& other)
	{
		ban_time = other.ban_time;
		userid = other.userid;
		request_time = other.request_time;
		kick_message = other.kick_message;
		int x = 0;
		do
		{
			player_name[x]=other.player_name[x];
			steamid[x]=other.steamid[x];
			ip[x]=other.ip[x];
			identity[x]=other.identity[x];
		} while(++x < 23);
		player_name[x]=steamid[x]=ip[x]='\0';
		do
		{
			identity[x]=other.identity[x];
		}
		while(++x < 63); 
		identity[x] = '\0';
	};

	bool operator== (const PlayerBanRequest& other) const
	{
		return (userid == other.userid);
	};
} PlayerBanRequestT;

typedef basic_slist<PlayerBanRequestT> BanRequestListT;

class BanRequest :
	public Singleton<BanRequest>,
	public TimerListener
{
	typedef Singleton<BanRequest> singleton_class;

private:
	float m_wait_time;
	bool m_do_writeid;

	void * cmd_gb_ban;
	void * cmd_sm_ban;

	BanRequestListT m_requests;

	void BanInternal(int ban_time, char const * steam_id, int userid, char const * kick_message, char const * ip);

public:
	BanRequest();
	virtual ~BanRequest() override final;

	void Init();

	void OnLevelInit();

	void WriteBansIfNeeded();

	void SetWaitTime(float wait_time);

	void AddAsyncBan(NczPlayer * const player, int ban_time, const char * kick_message);

	void BanNow(NczPlayer * const player, int ban_time, const char * kick_message);

	void TimerCallback(char const * const timer_name);
};

#endif
