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

#ifndef VALIDATIONTESTER_H
#define VALIDATIONTESTER_H

#include "Misc/temp_basiclist.h"
#include "Systems/BaseSystem.h"
#include "Players/NczFilteredPlayersList.h"
#include "Systems/OnTickListener.h"
#include "Players/temp_PlayerDataStruct.h"
#include "Misc/temp_singleton.h"
#include "Interfaces/IGameEventManager/IGameEventManager.h"

/////////////////////////////////////////////////////////////////////////
// ValidationTester
/////////////////////////////////////////////////////////////////////////

typedef struct ValidationInfo
{
	bool b;

	ValidationInfo()
	{
		b = false;
	};
	ValidationInfo(const ValidationInfo& other)
	{
		b = other.b;
	};
} ValidationInfoT;

typedef struct ValidatedInfo
{
	char m_steamid[64];
	char m_ipaddress[64];

	ValidatedInfo()
	{
		memset(this, 0, sizeof(ValidatedInfo));
	}

	ValidatedInfo(char const * steamid)
	{
		strncpy(m_steamid, steamid, 64);
	}

	ValidatedInfo(char const * steamid, char const * ip)
	{
		strncpy(m_steamid, steamid, 64);
		strncpy(m_ipaddress, ip, 64);
	}

	ValidatedInfo(ValidatedInfo const & other)
	{
		memcpy(m_steamid, other.m_steamid, 64);
		memcpy(m_ipaddress, other.m_ipaddress, 64);
	}

	ValidatedInfo & operator=(ValidatedInfo const & other)
	{
		memcpy(m_steamid, other.m_steamid, 64);
		memcpy(m_ipaddress, other.m_ipaddress, 64);
		return *this;
	}

	bool operator==(ValidatedInfo const & other) const
	{
		return strcmp(m_steamid, other.m_steamid) == 0;
	}
} ValidatedInfoT;

class ValidationTester :
	private BaseSystem,
	private SourceSdk::IGameEventListener002,
	private NczFilteredPlayersList,
	private OnTickListener,
	public PlayerDataStructHandler<ValidationInfoT>,
	public Singleton<ValidationTester>
{
	typedef basic_slist<const char *> PendingValidationsT;
	typedef basic_slist<ValidatedInfoT> ValidatedIdsT;
	typedef PlayerDataStructHandler<ValidationInfoT> playerdata_class;
	typedef Singleton<ValidationTester> singleton_class;

private:
	PendingValidationsT m_pending_validations;
	ValidatedIdsT m_validated_ids;

public:
	ValidationTester();
	virtual ~ValidationTester() final;

private:
	virtual void Init() override final;

	virtual void Load() override final;

	virtual void Unload() override final;
	
	virtual void ProcessPlayerTestOnTick(NczPlayer * const player, float const curtime) override final;

	virtual void ProcessOnTick(float const curtime) override final;

	virtual void FireGameEvent(SourceSdk::IGameEvent* ev) override final;

	void SetValidated(NczPlayer const * const player);

public:
	void AddPendingValidation(const char *pszUserName, const char* steamid);

private:
	bool WasPreviouslyValidated(NczPlayer const * const player);

public:
	bool JoinCallback(NczPlayer const * const player);
};

#endif // VALIDATIONTESTER_H
