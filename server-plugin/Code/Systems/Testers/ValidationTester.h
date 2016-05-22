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

class ValidationTester :
	private BaseSystem,
	private NczFilteredPlayersList,
	private OnTickListener,
	public PlayerDataStructHandler<ValidationInfoT>,
	public Singleton<ValidationTester>
{
private:
	typedef basic_slist<const char *> PendingValidationsT;
	typedef PlayerDataStructHandler<ValidationInfoT> playerdata_class;
	typedef Singleton<ValidationTester> singleton_class;

	PendingValidationsT m_pending_validations;

	void Init();
	void Load();
	void Unload();
	
	void ProcessPlayerTestOnTick(NczPlayer* player);
	void ProcessOnTick();

public:
	ValidationTester();
	~ValidationTester();

	void SetValidated(NczPlayer* player);
	void AddPendingValidation(const char* steamid);
	bool JoinCallback(NczPlayer* const player);
};

#endif // VALIDATIONTESTER_H
