#include "Solver1.h"

#include "Interfaces/InterfacesProxy.h"
#include "Systems/Logger.h"
#include "Systems/Blockers/WallhackBlocker.h"
#include "Misc/MathCache.h"

Solver1::Solver1() :
	BaseStaticSystem("Solver1"),
	PlayerRunCommandHookListener(),
	SourceSdk::IGameEventListener002(),
	playerdata_class(),
	OnTickListener()
{
}

Solver1::~Solver1()
{
}

void Solver1::Init()
{
	InitDataStruct();

	PlayerRunCommandHookListener::RegisterPlayerRunCommandHookListener(this);
	SourceSdk::InterfacesProxy::GetGameEventManager()->AddListener(this, "player_hurt", true);
	OnTickListener::RegisterOnTickListener(this);
}

void Solver1::DescribeEvents(solver_event_enum_type pdata, int curtick)
{
	if (pdata)
	{
		basic_string msg(Helpers::format("%d 0x%I64X ", curtick, pdata));

		if (pdata & SolverEvents::SHOOT)
		{
			msg.append("SHOOT,");
		}
		if (pdata & SolverEvents::HIT_TARGET)
		{
			msg.append("HIT_TARGET,");
		}
		if (pdata & SolverEvents::THRU_WALL)
		{
			msg.append("THRU_WALL,");
		}
		if (pdata & SolverEvents::PERFECT_RECOIL)
		{
			msg.append("PERFECT_RECOIL,");
		}
		if (pdata & SolverEvents::ALMOST_PERFECT_RECOIL)
		{
			msg.append("ALMOST_PERFECT_RECOIL,");
		}
		if (pdata & SolverEvents::TARGET_SWITCH)
		{
			msg.append("TARGET_SWITCH,");
		}
		if (pdata & SolverEvents::DETECT_MOUSEMISMATCH)
		{
			msg.append("MOUSEMISMATCH,");
		}
		if (pdata & SolverEvents::DETECT_ANGLESNAP)
		{
			msg.append("ANGLESNAP,");
		}
		if (pdata & SolverEvents::DETECT_ANGLEOUTOFBOUND)
		{
			msg.append("ANGLEOUTOFBOUND,");
		}
		if (pdata & SolverEvents::SHORT_REACT_TIME)
		{
			msg.append("SHORT_REACT_TIME,");
		}
		if (pdata & SolverEvents::AIM_STICK)
		{
			msg.append("AIM_STICK,");
		}
		if (pdata & SolverEvents::DETECT_RAPIDFIRE)
		{
			msg.append("RAPIDFIRE,");
		}
		if (pdata & SolverEvents::DETECT_STRAFE)
		{
			msg.append("STRAFE,");
		}
		if (pdata & SolverEvents::DETECT_PERFECT_BHOP)
		{
			msg.append("PERFECT_BHOP,");
		}
		if (pdata & SolverEvents::DETECT_SCRIPT_BHOP)
		{
			msg.append("SCRIPT_BHOP,");
		}
		if (pdata & SolverEvents::DETECT_TAMPERING_USERCMD)
		{
			msg.append("TAMPERING_USERCMD,");
		}

		g_Logger.Msg<MSG_CHAT>(msg.c_str());
	}
}

void Solver1::DeclareEventForPlayer(SolverEvents ev, PlayerHandler::iterator ph)
{
	auto pdata(GetPlayerDataStructByIndex(ph.GetIndex()));

	pdata->m_tb_events.ModCurrent()->v |= ev;
}

void Solver1::RT_ProcessOnTick(double const & curtime)
{
	ProcessFilter::InTestsNoBot filter_class;
	int curtick ( Helpers::GetGameTickCount());
	for (PlayerHandler::iterator ph(&filter_class); ph != PlayerHandler::end(); ph += &filter_class)
	{
		auto pdata(GetPlayerDataStructByIndex(ph.GetIndex()));

		if(pdata->m_tb_events.GetCurrent()->v == 0) // nothing happened at all last tick, just reuse this data space for another tick
		{ 
			pdata->m_tb_events.ModCurrent()->t = curtick;
		}
		else // save this and go forward
		{
			DescribeEvents(pdata->m_tb_events.GetCurrent()->v, pdata->m_tb_events.GetCurrent()->t);
			pdata->m_tb_events.Store(0, curtick);
		}
	}
}

PlayerRunCommandRet Solver1::RT_PlayerRunCommandCallback(PlayerHandler::iterator ph, void * const cmd, double const & curtime)
{
	int buttons(0);
	auto pdata(GetPlayerDataStructByIndex(ph.GetIndex()));

	if (SourceSdk::InterfacesProxy::m_game == SourceSdk::CounterStrikeGlobalOffensive)
	{
		buttons = ((SourceSdk::CUserCmd_csgo *)cmd)->buttons;
	}
	else
	{
		buttons = ((SourceSdk::CUserCmd *)cmd)->buttons;
	}

	if (buttons & IN_ATTACK)
	{
		DeclareEventForPlayer(SolverEvents::SHOOT, ph);

		if (!pdata->m_prev_buttons & IN_ATTACK)
		{
			if (curtime - pdata->m_prev_fire_button_change < 0.055)
			{
				DeclareEventForPlayer(SolverEvents::DETECT_RAPIDFIRE, ph);
			}

			pdata->m_prev_fire_button_change = curtime;
		}
	}
	else if (pdata->m_prev_buttons & IN_ATTACK)
	{
		if (curtime - pdata->m_prev_fire_button_change < 0.055)
		{
			DeclareEventForPlayer(SolverEvents::DETECT_RAPIDFIRE, ph);
		}

		pdata->m_prev_fire_button_change = curtime;
	}

	/*
	// test aim_stick
	if (pdata->m_last_target >= SlotStatus::PLAYER_IN_TESTS || pdata->m_last_target == SlotStatus::BOT)
	{
		// determine angle delta needed

		SourceSdk::Vector last_target_last_pos;
		SourceSdk::Vector last_target_cur_pos;
		SourceSdk::Vector ph_last_pos;
		SourceSdk::Vector ph_cur_pos;
		SourceSdk::QAngle ph_aimpunch;

	}*/

	// set prev

	pdata->m_prev_buttons = buttons;


	return PlayerRunCommandRet::CONTINUE;
}

void Solver1::FireGameEvent(SourceSdk::IGameEvent * ev)
{
	PlayerHandler::iterator ph_victim(g_NczPlayerManager.GetPlayerHandlerByUserId(ev->GetInt("userid")));
	PlayerHandler::iterator ph_attacker(g_NczPlayerManager.GetPlayerHandlerByUserId(ev->GetInt("attacker")));

	if (ph_victim != ph_attacker && ph_attacker != SlotStatus::INVALID)
	{
		DeclareEventForPlayer(SolverEvents::HIT_TARGET, ph_attacker);

		// test if shot was thru walls or using fast reaction

		if (!g_WallhackBlocker.m_viscache.IsVisible(ph_attacker.GetIndex(), ph_victim.GetIndex()))
		{
			DeclareEventForPlayer(SolverEvents::THRU_WALL, ph_attacker);
		}
		else if (Tier0::Plat_FloatTime() - g_WallhackBlocker.m_viscache.GetVisibleTime(ph_attacker.GetIndex(), ph_victim.GetIndex()) < 0.1)
		{
			DeclareEventForPlayer(SolverEvents::SHORT_REACT_TIME, ph_attacker);
		}

		char const * ev_name(ev->GetName() + 7);

		if (*ev_name == 'h') // player_hurt
		{
			auto attacker_pdata(GetPlayerDataStructByIndex(ph_attacker.GetIndex()));
			PlayerHandler::iterator & last_attacker_target(attacker_pdata->m_last_target);

			if (ph_victim != last_attacker_target && Tier0::Plat_FloatTime() - attacker_pdata->m_last_target_time < 0.3)
			{
				DeclareEventForPlayer(SolverEvents::TARGET_SWITCH, ph_attacker);
			}

			attacker_pdata->m_last_target_time = Tier0::Plat_FloatTime();
			last_attacker_target = ph_victim;
		}

		if (*ev_name == 'd') // player_death
		{
			auto attacker_pdata(GetPlayerDataStructByIndex(ph_attacker.GetIndex()));
			PlayerHandler::iterator & last_attacker_target(attacker_pdata->m_last_target);

			if (ph_victim != last_attacker_target && Tier0::Plat_FloatTime() - attacker_pdata->m_last_target_time < 0.3)
			{
				DeclareEventForPlayer(SolverEvents::TARGET_SWITCH, ph_attacker);
			}

			attacker_pdata->m_last_target_time = Tier0::Plat_FloatTime();
			last_attacker_target = ph_victim;
		}
	}
}

Solver1 g_Solver1;
