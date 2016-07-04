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

#ifndef CONVARTESTER_H
#define CONVARTESTER_H

#include "Misc/temp_basicstring.h"

#include "Misc/temp_basiclist.h"
#include "Systems/BaseSystem.h"
#include "Systems/Testers/Detections/temp_BaseDetection.h" // + helpers, ifaces, preprocessors, playermanager
#include "Players/temp_PlayerDataStruct.h"
#include "Systems/OnTickListener.h"
#include "Misc/temp_singleton.h"

/////////////////////////////////////////////////////////////////////////
// ConVarTester
/////////////////////////////////////////////////////////////////////////

typedef enum class ConVarRule : unsigned int
{
	SAME = 0,
	SAME_FLOAT,
	SAME_AS_SERVER,
	SAME_FLOAT_AS_SERVER,
	NO_VALUE
} ConVarRuleT;

typedef struct ConVarInfo
{
	char name[ 64 ];
	char value[ 64 ];
	ConVarRuleT rule;
	bool safe;
	void* sv_var;

	ConVarInfo ()
	{
		memset ( this, 0, sizeof ( ConVarInfo ) );
	};
	ConVarInfo ( const char* pname, const char* pvalue, ConVarRuleT prule, bool psafe, void* sv = nullptr )
	{
		strncpy ( name, pname, 64 );
		strncpy ( value, pvalue, 64 );
		rule = prule;
		safe = psafe;
		sv_var = sv;
	};
	ConVarInfo ( const ConVarInfo& other )
	{
		memcpy ( this, &other, sizeof ( ConVarInfo ) );
	};
} ConVarInfoT;

typedef CUtlVector<ConVarInfoT> ConVarRulesListT;

typedef struct CurrentConVarRequest
{
	bool isSent;
	bool isReplyed;
	float timeStart;
	int ruleset;
	basic_string answer;
	basic_string answer_status;

	CurrentConVarRequest ()
	{
		isSent = false; isReplyed = false; timeStart = 0.0; ruleset = 0;
	};
	CurrentConVarRequest ( const CurrentConVarRequest& other )
	{
		isSent = other.isSent; isReplyed = other.isReplyed; timeStart = other.timeStart; ruleset = other.ruleset; answer = other.answer; answer_status = other.answer_status;
	};
} CurrentConVarRequestT;

class ConVarTester;

class Detection_ConVar : public LogDetection<CurrentConVarRequestT>
{
	friend ConVarTester;
	typedef LogDetection<CurrentConVarRequestT> hClass;
public:
	Detection_ConVar () : hClass ()
	{};
	~Detection_ConVar ()
	{};

	virtual basic_string GetDataDump ();
	virtual basic_string GetDetectionLogMessage ();
};

class ConVarTester :
	public BaseSystem,
	public OnTickListener,
	public PlayerDataStructHandler<CurrentConVarRequestT>,
	public Singleton<ConVarTester>
{
	typedef Singleton<ConVarTester> singleton_class;
	typedef PlayerDataStructHandler<CurrentConVarRequestT> playerdata_class;

	friend Detection_ConVar;

private:
	ConVarRulesListT m_convars_rules;

	void* var_sv_cheats;

public:
	ConVarTester ();
	virtual ~ConVarTester () final;

private:
	virtual void Init () override final;

	virtual void Load () override final;

	virtual void Unload () override final;

	virtual bool GotJob () const override final;

	virtual bool sys_cmd_fn ( SourceSdk::CCommand const &args ) override final;

	/* Nouvelle version de la fonction qui va faire en sorte de ne tester qu'un seul joueur par frame */
	virtual void RT_ProcessOnTick ( float const curtime ) override final;

public:
	void RT_OnQueryCvarValueFinished ( PlayerHandler::const_iterator ph, SourceSdk::EQueryCvarValueStatus eStatus, const char *pCvarName, const char *pCvarValue );

private:
	void RT_ProcessPlayerTest ( PlayerHandler::const_iterator ph, float const curtime );

	void AddConvarRuleset ( const char * name, const char * value, ConVarRuleT rule, bool safe = true );

	ConVarInfoT* RT_FindConvarRuleset ( const char * name );

	PlayerHandler::const_iterator m_current_player;
};

#endif // CONVARTESTER_H
