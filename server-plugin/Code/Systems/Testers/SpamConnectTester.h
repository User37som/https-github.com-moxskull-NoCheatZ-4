
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

#ifndef SPAMCONNECTTESTER_H
#define SPAMCONNECTTESTER_H

#include "Misc/temp_basicstring.h"

#include <ctime>

#include "Misc/temp_basiclist.h"
#include "Preprocessors.h"
#include "Systems/BaseSystem.h"
#include "Misc/temp_singleton.h"
#include "Misc/Helpers.h"

struct ConnectInfo
{
	unsigned int ip_addr;
	time_t next_connect_time;
	int count;

	ConnectInfo ()
	{
		count = 0;
		next_connect_time = time(nullptr);
	};
	ConnectInfo ( const char* addr )
	{
		count = 0;
		next_connect_time = 0;
		ip_addr = Helpers::HashString(addr);
	};
	ConnectInfo ( const ConnectInfo& other )
	{
		ip_addr = other.ip_addr;
		count = other.count;
		next_connect_time = other.next_connect_time;
	};
	void OnConnect ()
	{
		next_connect_time = time ( nullptr ) + 30;
	};

	bool operator== ( const ConnectInfo& other ) const
	{
		return ip_addr == other.ip_addr;
	};
};

class SpamConnectTester :
	private BaseDynamicSystem,
	public Singleton
{
private:
	typedef CUtlVector<ConnectInfo> ConnectListT;
	ConnectListT m_connect_list;

public:
	SpamConnectTester ();
	virtual ~SpamConnectTester () final;

private:
	virtual void Init () override final;

	virtual void Load () override final;

	virtual void Unload () override final;

	virtual bool GotJob () const override final;

public:
	void ClientConnect ( bool *bAllowConnect, SourceSdk::edict_t const * const pEntity, const char *pszName, const char *pszAddress, char *reject, int maxrejectlen );
};

extern SpamConnectTester g_SpamConnectTester;

#endif
