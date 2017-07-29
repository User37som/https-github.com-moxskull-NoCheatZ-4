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
#include "Misc/QueryCvarProvider.h"

/////////////////////////////////////////////////////////////////////////
// ConVarTester
/////////////////////////////////////////////////////////////////////////

typedef enum class ConVarRule : unsigned int
{
	SAME = 0,
	SAME_FLOAT,
	SAME_AS_SERVER,
	SAME_FLOAT_AS_SERVER,
	LOWER,
	HIGHER,
	//RANGE,
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

enum class ConVarRequestStatus : unsigned int
{
	NOT_PROCESSING = 0,
	SENT,
	REPLYED
};

typedef struct CurrentConVarRequest
{
	ConVarRequestStatus status;
	double timeStart;
	double timeEnd;
	int ruleset;
	SourceSdk::QueryCvarCookie_t cookie;
	int attempts;
	basic_string answer;
	basic_string answer_status;

	CurrentConVarRequest () : 
		status ( ConVarRequestStatus::NOT_PROCESSING ),
		timeStart (0.0f),
		timeEnd (0.0f),
		ruleset (0),
		cookie (0),
		attempts (0),
		answer (),
		answer_status ()
	{
	};

	CurrentConVarRequest ( const CurrentConVarRequest& other )
	{
		status = other.status; timeStart = other.timeStart; timeEnd = other.timeEnd;  ruleset = other.ruleset; answer = other.answer; answer_status = other.answer_status;
	};

	void PrepareNextRequest (ConVarRulesListT const & rules)
	{
		if( ++ ruleset >= rules.Size () )
		{
			ruleset = 0;
		}
		answer = "NO ANSWER";
		answer_status = "NO STATUS";
		status = ConVarRequestStatus::NOT_PROCESSING;
		attempts = 0;
	}

	inline void SendCurrentRequest(PlayerHandler::iterator ph, double const curtime, ConVarRulesListT const & rules);
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

class Detection_ConVarRequestTimedOut : public Detection_ConVar
{
	friend ConVarTester;
	typedef Detection_ConVar hClass;

public:
	Detection_ConVarRequestTimedOut() : hClass()
	{};
	~Detection_ConVarRequestTimedOut()
	{};

	virtual basic_string GetDetectionLogMessage();
};

class Detection_IllegalConVar : public Detection_ConVar
{
	friend ConVarTester;
	typedef Detection_ConVar hClass;

public:
	Detection_IllegalConVar() : hClass()
	{};
	~Detection_IllegalConVar()
	{};

	virtual basic_string GetDetectionLogMessage();
};

class ConVarTester :
	public BaseTesterSystem,
	public OnTickListener,
	public PlayerDataStructHandler<CurrentConVarRequestT>,
	public Singleton
{
	typedef PlayerDataStructHandler<CurrentConVarRequestT> playerdata_class;

	friend Detection_ConVar;
	friend Detection_ConVarRequestTimedOut;
	friend Detection_IllegalConVar;

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

	virtual bool sys_cmd_fn ( SourceSdk::CCommand const &args );

	/* Nouvelle version de la fonction qui va faire en sorte de ne tester qu'un seul joueur par frame */
	virtual void RT_ProcessOnTick (double const & curtime ) override final;

public:
	void RT_OnQueryCvarValueFinished ( PlayerHandler::iterator ph, SourceSdk::QueryCvarCookie_t cookie, SourceSdk::EQueryCvarValueStatus eStatus, const char *pCvarName, const char *pCvarValue );

private:
	void RT_ProcessPlayerTest ( PlayerHandler::iterator ph, double const & curtime );

	void AddConvarRuleset ( const char * name, const char * value, ConVarRuleT rule, bool safe = true );

	void LoadDefaultRules ();

	ConVarInfoT* RT_FindConvarRuleset ( const char * name );

	PlayerHandler::iterator m_current_player;
};

extern ConVarTester g_ConVarTester;

static bool err_log_once = true;

inline void CurrentConVarRequest::SendCurrentRequest(PlayerHandler::iterator ph, double const curtime, ConVarRulesListT const & rules)
{
	SourceSdk::edict_t* pedict(ph->GetEdict());
	char const * var(rules[ruleset].name);

	cookie = g_QueryCvarProvider.StartQueryCvarValue(pedict, var);
	
	if (cookie == InvalidQueryCvarCookie)
	{
		status = ConVarRequestStatus::NOT_PROCESSING;
		g_Logger.Msg<MSG_ERROR>("ConVarTester : StartQueryCvarValue returned InvalidQueryCvarCookie");
	}
	else if(cookie < InvalidQueryCvarCookie && err_log_once)
	{
		g_Logger.Msg<MSG_ERROR>("ConVarTester : StartQueryCvarValue returned negative cookie (Please report issue)");
		err_log_once = false;
		status = ConVarRequestStatus::NOT_PROCESSING;
	}
	else
	{
		status = ConVarRequestStatus::SENT;
		timeStart = curtime;
		timeEnd = timeStart + 10.0;
	}
	
}

#endif // CONVARTESTER_H
