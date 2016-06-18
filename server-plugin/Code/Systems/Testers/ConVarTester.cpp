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

#include <stdio.h>

#include "ConVarTester.h"

#include "Systems/BanRequest.h"
#include "Systems/Logger.h"

/////////////////////////////////////////////////////////////////////////
// ConVarTester
/////////////////////////////////////////////////////////////////////////

ConVarTester::ConVarTester() :
	AsyncNczFilteredPlayersList(),
	BaseSystem("ConVarTester", PLAYER_CONNECTED, PLAYER_CONNECTING, STATUS_EQUAL_OR_BETTER, "Enable - Disable - Verbose - AddRule - ResetRules"),
	OnTickListener(),
	playerdata_class(),
	singleton_class(),
	var_sv_cheats(nullptr)
{
}

ConVarTester::~ConVarTester()
{
	Unload();
}

void ConVarTester::Init()
{
	InitDataStruct();
}

void ConVarTester::ProcessOnTick(float const curtime)
{
	if(m_convars_rules.IsEmpty()) return;

	
	if(SourceSdk::InterfacesProxy::ConVar_GetBool(var_sv_cheats))
	{
		SystemVerbose2("sv_cheats set to 1. Skipping ConVarTest ...");
		return;
	}
	NczPlayer* player = GetNextPlayer();
	if(player)
	{
		ProcessPlayerTest(player);
	}
}

void ConVarTester::ProcessPlayerTest(NczPlayer* player)
{
	if (!IsActive()) return;
	if (m_convars_rules.IsEmpty()) return;

	CurrentConVarRequestT* const req = GetPlayerDataStruct(player);

	if(req->isSent && !req->isReplyed)
	{
		if(Plat_FloatTime() - 30.0 > req->timeStart)
		{
			player->Kick("ConVar request timed out");
		}
		return;
	}

	if(!req->isSent && !req->isReplyed)
	{
		req->ruleset = 0;
		req->answer = "NO ANSWER";
		req->answer_status = "NO STATUS";
	}
	else if(++(req->ruleset) >= m_convars_rules.Size())
	{
		 req->ruleset = 0;
		 req->answer = "NO ANSWER";
		 req->answer_status = "NO STATUS";
	}

	SourceSdk::InterfacesProxy::GetServerPluginHelpers()->StartQueryCvarValue(player->GetEdict(), m_convars_rules[req->ruleset].name);
	req->isSent = true;
	req->isReplyed = false;
	req->timeStart = Plat_FloatTime();
	SystemVerbose1(Helpers::format("Sending %s ConVar request to ent-id %d", m_convars_rules[req->ruleset].name, player->GetIndex()));
}

ConVarInfoT* ConVarTester::FindConvarRuleset(const char * name)
{
	size_t pos = 0;
	size_t const max = m_convars_rules.Size();
	while(pos < max)
	{
		if(Helpers::bStriEq(m_convars_rules[pos].name, name))
		{
			return &m_convars_rules[pos];
		}
		++pos;
	}
	return nullptr;
}

bool ConVarTester::sys_cmd_fn ( const SourceSdk::CCommand &args )
{
	if(Helpers::bStriEq("AddRule", args.Arg(2))) 
		// example : ncz ConVarTester AddRule sv_cheats 0 NO_VALUE
	{
		if(args.ArgC() == 6)
		{
			ConVarRuleT rule;

			if(Helpers::bStriEq("NO_VALUE", args.Arg(5))) rule = NO_VALUE;
			else if(Helpers::bStriEq("SAME", args.Arg(5))) rule = SAME;
			else if(Helpers::bStriEq("SAME_FLOAT", args.Arg(5))) rule = SAME_FLOAT;
			else if(Helpers::bStriEq("SAME_AS_SERVER", args.Arg(5))) rule = SAME_AS_SERVER;
			else if(Helpers::bStriEq("SAME_FLOAT_AS_SERVER", args.Arg(5))) rule = SAME_FLOAT_AS_SERVER;
			else
			{
				printf("Arg %s not found.\n", args.Arg(5));
				return true;
			}

			if(Helpers::bStriEq("sv_", args.Arg(3), 0U, 3)) rule = SAME_AS_SERVER;
			else rule = SAME;

			basic_string value = args.Arg(4);

			if(value.find('.') != basic_string::npos)
			{
				if(rule == SAME_AS_SERVER) rule = SAME_FLOAT_AS_SERVER;
				else rule = SAME_FLOAT;
			}
			else
			{
				if(rule != SAME_AS_SERVER) rule = SAME_FLOAT_AS_SERVER;
				else rule = SAME_FLOAT;
			}

			AddConvarRuleset(args.Arg(3), args.Arg(4), rule, false);
			printf("Added convar test rule.\n");
			return true;
		}
		else
		{
			printf("ncz ConVarTester AddRule [ConVar] [value] [TestType]\nTestType : NO_VALUE - SAME - SAME_FLOAT - SAME_AS_SERVER - SAME_FLOAT_AS_SERVER\n");
			return true;
		}
	}
	if(Helpers::bStriEq("ResetRules", args.Arg(2)))
		// example : ncz ConVarTester ResetRules
	{
		Unload();
		Load();
		return true;
	}
	return false;
}

void ConVarTester::AddConvarRuleset(const char * name, const char * value, ConVarRuleT rule, bool safe)
{
	if(rule == SAME_AS_SERVER || rule == SAME_FLOAT_AS_SERVER)
	{
		void * sv_cvar = SourceSdk::InterfacesProxy::ICvar_FindVar(name);
		if(sv_cvar)
		{
			m_convars_rules.AddToTail(ConVarInfo(name, SourceSdk::InterfacesProxy::ConVar_GetString(sv_cvar), rule, safe, sv_cvar));
		}
		else
		{
			Logger::GetInstance()->Msg<MSG_ERROR>(Helpers::format("ConVarTester : Failed to link the server convar %s", name));
		}
	}
	else
	{
		m_convars_rules.AddToTail(ConVarInfo(name, value, rule, safe));
	}
}

void ConVarTester::OnQueryCvarValueFinished(NczPlayer* player, SourceSdk::EQueryCvarValueStatus eStatus, const char *pCvarName, const char *pCvarValue)
{
	if(!IsActive()) return;
	SystemVerbose1(Helpers::format("Received %s ConVar reply for ent-id %d", pCvarName, player->GetIndex()));

	if(SourceSdk::InterfacesProxy::ConVar_GetBool(var_sv_cheats))
	{
		SystemVerbose2("sv_cheats set to 1. Skipping ...");
		return;
	}

	ConVarInfoT* ruleset = FindConvarRuleset(pCvarName);
	if(!ruleset)
	{
unexpected:
		SystemVerbose2("Unexpected reply. Skipping ...");
		return;
	}

	CurrentConVarRequest* const req = GetPlayerDataStruct(player);

	if(!req->isSent) goto unexpected;
	if(!Helpers::bStriEq(m_convars_rules[req->ruleset].name, ruleset->name)) goto unexpected;
	req->isReplyed = true;
	req->answer = pCvarValue;

	switch(eStatus)
	{
	case SourceSdk::eQueryCvarValueStatus_ValueIntact:
		{
			req->answer_status = "ValueIntact";
			SystemVerbose2("Received eQueryCvarValueStatus_ValueIntact");
			if(ruleset->rule == NO_VALUE)
			{
				SystemVerbose2("Was expecting eQueryCvarValueStatus_CvarNotFound");
				Detection_ConVar pDetection = Detection_ConVar();
				pDetection.PrepareDetectionData(req);
				pDetection.PrepareDetectionLog(player, this);
				pDetection.Log();
				BanRequest::GetInstance()->AddAsyncBan(player, 0, "Banned by NoCheatZ 4");
			}
			else if(ruleset->rule == SAME)
			{
				if(Helpers::bStrEq(ruleset->value, pCvarValue))
				{
					SystemVerbose2(Helpers::format("Value %s is correct\n", pCvarValue));
				}
				else
				{
					SystemVerbose2(Helpers::format("Value %s is NOT correct\n", pCvarValue));
					Detection_ConVar pDetection = Detection_ConVar();
					pDetection.PrepareDetectionData(req);
					pDetection.PrepareDetectionLog(player, this);
					pDetection.Log();
					BanRequest::GetInstance()->AddAsyncBan(player, 0, "Banned by NoCheatZ 4");
				}
			}
			else if(ruleset->rule == SAME_AS_SERVER)
			{
				if(Helpers::bStrEq(SourceSdk::InterfacesProxy::ConVar_GetString(ruleset->sv_var), pCvarValue))
				{
					SystemVerbose2(Helpers::format("Value %s is correct\n", pCvarValue));
				}
				else
				{
					SystemVerbose2(Helpers::format("Value %s is NOT correct\n", pCvarValue));
					Detection_ConVar pDetection = Detection_ConVar();
					pDetection.PrepareDetectionData(req);
					pDetection.PrepareDetectionLog(player, this);
					pDetection.Log();
					BanRequest::GetInstance()->AddAsyncBan(player, 0, "Banned by NoCheatZ 4");
				}
			}
			else if(ruleset->rule == SAME_FLOAT)
			{
				float fcval = (float)atof(pCvarValue);
				float fsval = (float)atof(ruleset->value);
				if(fcval == fsval)
				{
					SystemVerbose2(Helpers::format("Value %s is correct\n", pCvarValue));
				}
				else
				{
					SystemVerbose2(Helpers::format("Value %s is NOT correct\n", pCvarValue));
					Detection_ConVar pDetection = Detection_ConVar();
					pDetection.PrepareDetectionData(req);
					pDetection.PrepareDetectionLog(player, this);
					pDetection.Log();
					BanRequest::GetInstance()->AddAsyncBan(player, 0, "Banned by NoCheatZ 4");
				}
			}
			else if(ruleset->rule == SAME_FLOAT_AS_SERVER)
			{
				float fcval = (float)atof(pCvarValue);
				float fsval = (float)atof(SourceSdk::InterfacesProxy::ConVar_GetString(ruleset->sv_var));
				if(fcval == fsval)
				{
					SystemVerbose2(Helpers::format("Value %s is correct\n", pCvarValue));
				}
				else
				{
					SystemVerbose2(Helpers::format("Value %s is NOT correct\n", pCvarValue));
					Detection_ConVar pDetection = Detection_ConVar();
					pDetection.PrepareDetectionData(req);
					pDetection.PrepareDetectionLog(player, this);
					pDetection.Log();
					BanRequest::GetInstance()->AddAsyncBan(player, 0, "Banned by NoCheatZ 4");
				}
			}
			break;
		}

	case SourceSdk::eQueryCvarValueStatus_CvarNotFound:
		{
			req->answer_status = "CvarNotFound";
			req->answer = "NO VALUE";
			SystemVerbose2("eQueryCvarValueStatus_CvarNotFound");
			if(ruleset->rule != NO_VALUE)
			{
				SystemVerbose2("Was expecting eQueryCvarValueStatus_ValueIntact");
				Detection_ConVar pDetection = Detection_ConVar();
				pDetection.PrepareDetectionData(req);
				pDetection.PrepareDetectionLog(player, this);
				pDetection.Log();
				BanRequest::GetInstance()->AddAsyncBan(player, 0, "Banned by NoCheatZ 4");
			}
			break;
		}

	case SourceSdk::eQueryCvarValueStatus_NotACvar:
		{
			req->answer_status = "NotACvar";
			req->answer = "CONCOMMAND";
			SystemVerbose2("eQueryCvarValueStatus_NotACvar");
			goto unexpected2;
		}

	case SourceSdk::eQueryCvarValueStatus_CvarProtected:
		{
			req->answer_status = "CvarProtected";
			req->answer = "NO VALUE";
			SystemVerbose2("eQueryCvarValueStatus_CvarProtected");
			goto unexpected2;
		}

	default:
		{
			req->answer_status = "NO STATUS";
			SystemVerbose2("eQueryCvarValueStatus_UnknownStatus");
			goto unexpected2;
		}
	}
	
	req->isSent = false;
	return;
unexpected2:
	SystemVerbose2("Was expecting eQueryCvarValueStatus_ValueIntact");
	Detection_ConVar pDetection = Detection_ConVar();
	pDetection.PrepareDetectionData(req);
	pDetection.PrepareDetectionLog(player, this);
	pDetection.Log();
	BanRequest::GetInstance()->AddAsyncBan(player, 0, "Banned by NoCheatZ 4");
}

void ConVarTester::Load()
{
	for (PlayerHandler::const_iterator it = PlayerHandler::begin(); it != PlayerHandler::end(); ++it)
	{
		if (it)
			ResetPlayerDataStruct(*it);
	}

	AddConvarRuleset("developer", "0", SAME);
	AddConvarRuleset("sv_cheats", "0", SAME_AS_SERVER);
	AddConvarRuleset("sv_accelerate", "0", SAME_AS_SERVER);
	AddConvarRuleset("sv_showimpacts", "0", SAME_AS_SERVER);
	AddConvarRuleset("sv_showlagcompensation", "0", SAME_AS_SERVER);
	AddConvarRuleset("host_framerate", "0", SAME_FLOAT_AS_SERVER);
	AddConvarRuleset("host_timescale", "0", SAME_FLOAT_AS_SERVER);
	AddConvarRuleset("r_visualizetraces", "0", SAME_AS_SERVER);
	AddConvarRuleset("mat_normalmaps", "0", SAME_AS_SERVER);
	AddConvarRuleset("mp_playerid", "0", SAME_AS_SERVER);
	AddConvarRuleset("mp_forcecamera", "1", SAME_AS_SERVER);
	AddConvarRuleset("net_fakeloss", "0", SAME);
	AddConvarRuleset("net_fakelag", "0", SAME);
	AddConvarRuleset("net_fakejitter", "0", SAME);
	AddConvarRuleset("r_drawothermodels", "1", SAME);
	AddConvarRuleset("r_shadowwireframe", "0", SAME);
	AddConvarRuleset("r_avglight", "1", SAME);
	AddConvarRuleset("r_novis", "0", SAME);
	AddConvarRuleset("r_drawparticles", "1", SAME);
	AddConvarRuleset("r_drawopaqueworld", "1", SAME);
	AddConvarRuleset("r_drawtranslucentworld", "1", SAME);
	AddConvarRuleset("r_drawmodelstatsoverlay", "0", SAME);
	AddConvarRuleset("r_skybox", "1", SAME);
	AddConvarRuleset("r_aspectratio", "0", SAME);
	AddConvarRuleset("r_drawskybox", "1", SAME);
	AddConvarRuleset("r_showenvcubemap", "0", SAME);
	AddConvarRuleset("r_drawlights", "0", SAME);
	AddConvarRuleset("r_drawrenderboxes", "0", SAME);
	AddConvarRuleset("mat_wireframe", "0", SAME);
	AddConvarRuleset("mat_drawwater", "1", SAME);
	AddConvarRuleset("mat_loadtextures", "1", SAME);
	AddConvarRuleset("mat_showlowresimage", "0", SAME);
	AddConvarRuleset("mat_fillrate", "0", SAME);
	AddConvarRuleset("mat_proxy", "0", SAME);
	AddConvarRuleset("mem_force_flush", "0", SAME);
	AddConvarRuleset("fog_enable", "1", SAME);
	AddConvarRuleset("cl_pitchup", "89", SAME);
	AddConvarRuleset("cl_pitchdown", "89", SAME);
	if (SourceSdk::InterfacesProxy::m_game == SourceSdk::CounterStrikeGlobalOffensive)
	{
		AddConvarRuleset("cl_bobcycle", "0.98", SAME_FLOAT);
	}
	else
	{
		AddConvarRuleset("cl_bobcycle", "0.8", SAME_FLOAT);
	}
	AddConvarRuleset("cl_leveloverviewmarker", "0", SAME);
	AddConvarRuleset("snd_visualize", "0", SAME);
	AddConvarRuleset("snd_show", "0", SAME);
	AddConvarRuleset("openscript", "", NO_VALUE);
	AddConvarRuleset("openscript_version", "", NO_VALUE);
	AddConvarRuleset("ms_sv_cheats", "", NO_VALUE);
	AddConvarRuleset("ms_r_drawothermodels", "", NO_VALUE);
	AddConvarRuleset("ms_chat", "", NO_VALUE);
	AddConvarRuleset("ms_aimbot", "", NO_VALUE);
	AddConvarRuleset("wallhack", "", NO_VALUE);
	AddConvarRuleset("cheat_chat", "", NO_VALUE);
	AddConvarRuleset("cheat_chams", "", NO_VALUE);
	AddConvarRuleset("cheat_dlight", "", NO_VALUE);
	AddConvarRuleset("SmAdminTakeover", "", NO_VALUE);
	AddConvarRuleset("ManiAdminTakeover", "", NO_VALUE);
	AddConvarRuleset("ManiAdminHacker", "", NO_VALUE);
	AddConvarRuleset("byp_svc", "", NO_VALUE);
	AddConvarRuleset("byp_speed_hts", "", NO_VALUE);
	AddConvarRuleset("byp_speed_hfr", "", NO_VALUE);
	AddConvarRuleset("byp_render_rdom", "", NO_VALUE);
	AddConvarRuleset("byp_render_mwf", "", NO_VALUE);
	AddConvarRuleset("byp_render_rdp", "", NO_VALUE);
	AddConvarRuleset("byp_fake_lag", "", NO_VALUE);
	AddConvarRuleset("byp_fake_loss", "", NO_VALUE);

	var_sv_cheats = SourceSdk::InterfacesProxy::ICvar_FindVar("sv_cheats");

	OnTickListener::RegisterOnTickListener(this);
}

void ConVarTester::Unload()
{
	OnTickListener::RemoveOnTickListener(this);
	m_convars_rules.RemoveAll();
}

const char* ConvertRule(ConVarRule rule)
{
	switch(rule)
	{
	case SAME:
		return "SAME";
	case SAME_FLOAT:
		return "SAME_FLOAT";
	case SAME_AS_SERVER:
		return "SAME_AS_SERVER";
	case SAME_FLOAT_AS_SERVER:
		return "SAME_FLOAT_AS_SERVER";
	case NO_VALUE:
		return "NO_VALUE";
	default:
		return "SAME";
	};
}

basic_string Detection_ConVar::GetDataDump()
{
	return Helpers::format(":::: CurrentConVarRequest {\n:::::::: Is Request Sent : %s,\n:::::::: Is Request Answered : %s,\n:::::::: Request Sent At : %f,\n:::::::: ConVarInfo {\n:::::::::::: ConVar Name : %s,\n:::::::::::: Expected Value : %s,\n:::::::::::: Got Value : %s,\n:::::::::::: Answer Status : %s,\n:::::::::::: Comparison Rule : %s\n::::::::}\n::::}",
							Helpers::boolToString(GetDataStruct()->isSent),
							Helpers::boolToString(GetDataStruct()->isReplyed),
							GetDataStruct()->timeStart,
							ConVarTester::GetInstance()->m_convars_rules[GetDataStruct()->ruleset].name,
							ConVarTester::GetInstance()->m_convars_rules[GetDataStruct()->ruleset].value,
							GetDataStruct()->answer.c_str(),
							GetDataStruct()->answer_status.c_str(),
							ConvertRule(ConVarTester::GetInstance()->m_convars_rules[GetDataStruct()->ruleset].rule));
}

basic_string Detection_ConVar::GetDetectionLogMessage()
{
	return Helpers::format("%s ConVar Bypasser", ConVarTester::GetInstance()->m_convars_rules[GetDataStruct()->ruleset].name);
}

