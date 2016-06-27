
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

#include "Misc/temp_basiclist.h"
#include "Preprocessors.h"
#include "Systems/BaseSystem.h"
#include "Misc/temp_singleton.h"

struct ConnectInfo
{
	basic_string ip_addr;
	float next_connect_time;
	int count;

	ConnectInfo ()
	{
		count = 0;
		next_connect_time = Plat_FloatTime ();
	};
	ConnectInfo ( const char* addr )
	{
		ip_addr = addr;
	};
	ConnectInfo ( const ConnectInfo& other )
	{
		ip_addr = other.ip_addr;
		count = other.count;
		next_connect_time = other.next_connect_time;
	};
	void OnConnect ()
	{
		next_connect_time = Plat_FloatTime () + 10.0f;
	};

	bool operator== ( const ConnectInfo& other ) const
	{
		return ( strcmp ( ip_addr.c_str (), other.ip_addr.c_str () ) == 0 );
	};
};

class SpamConnectTester :
	private BaseSystem,
	public Singleton<SpamConnectTester>
{
	typedef Singleton<SpamConnectTester> singleton_class;

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

#endif
