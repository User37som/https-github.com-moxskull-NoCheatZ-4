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

#include "Systems/BaseSystem.h"
#include "Hooks/PlayerRunCommandHookListener.h"
#include "Misc/EntityProps.h"
#include "Interfaces\IGameEventManager\IGameEventManager.h"
#include "Players/temp_PlayerDataStruct.h"
#include "Misc/temp_Throwback.h"
#include "Systems/OnTickListener.h"
#include "Events.h"

#define SOLVER1_TB_MAX 32

typedef Throwback<solver_event_enum_type, int, SOLVER1_TB_MAX> tb_playerevent;

struct Solver1PlayerData
{
	tb_playerevent m_tb_events;
	double m_prev_fire_button_change;
	double m_last_target_time;
	SourceSdk::QAngle m_prev_viewangles;
	int m_prev_buttons;
	PlayerHandler::iterator m_last_target;
	
};

class Solver1 :
	public BaseStaticSystem,
	public PlayerRunCommandHookListener,
	public SourceSdk::IGameEventListener002,
	public PlayerDataStructHandler<Solver1PlayerData>,
	public OnTickListener
{
	typedef PlayerDataStructHandler<Solver1PlayerData> playerdata_class;
public:
	Solver1();

	~Solver1();

	virtual void Init() override final;

	void DescribeEvents(solver_event_enum_type pdata, int curtick);

	void DeclareEventForPlayer(SolverEvents ev, PlayerHandler::iterator ph);

	virtual void RT_ProcessOnTick(double const & curtime) override final;

	virtual PlayerRunCommandRet RT_PlayerRunCommandCallback(PlayerHandler::iterator ph, void * const cmd, double const & curtime) override final;

	virtual void FireGameEvent(SourceSdk::IGameEvent* ev) override final;
};

extern Solver1 g_Solver1;
