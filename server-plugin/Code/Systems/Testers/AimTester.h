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

#ifndef AIMTESTER_H
#define AIMTESTER_H

/*
	This one is tough ...

	We will try to detect "human-like" aimbots automatically.

	- Need to implement "aiming good" probability model
		- Detect accuracy of spray pattern over visible target and over invisible target (basically tells wether the current shot is near the last one over that target; more the pattern tells that bulllet was supposed to be far the last one, more the probability of cheating is true if that bullet is near the last one ... always over that target)
			- Have the weapon recoil values
			- Properly detect all visibles targets (move visibility data struct from antiwallhackblocker to a persistent system)
			- Detect the current target and sray transfers
				- Have a relative reference 3dpoint to that target (like a bone)
					- Capture body impact
				- Have a distance to that target
				- Have the velocity of that target
				- Have the number of shots fired
					
	- Detect inconsistent aim speed towards a target
		- That happens when the crosshair enters the aimbot radio of a target.
		- Also very fast and multiple aim direction changes within a short period.

	Will use the throwback data struct, registering everything for 1 second.
		- Implement .bmp graphical print of a detection's data.

*/

#include "Systems/BaseSystem.h"
#include "Hooks/PlayerRunCommandHookListener.h"
#include "Players/temp_PlayerDataStruct.h"
#include "Interfaces/IGameEventManager/IGameEventManager.h"
#include "Systems/Testers/Detections/temp_BaseDetection.h"
#include "Misc/temp_singleton.h"
#include "Misc/temp_Throwback.h"

/*
	This struct holds informations about the data we get each tick about a player
	Note : some of the values can be delayed by one tick, take care of this when analyzing.
*/
struct AimTickInfo
{
	SourceSdk::QAngle m_eyes; // eye angles of the player
	SourceSdk::QAngle m_view_punch; // view punch (from previous tick because when we copy the data the shot isn't done yet)
	SourceSdk::Vector m_target_pos; // last target hit
	SourceSdk::QAngle m_target_angle; // angles of the target
	SourceSdk::Vector m_target_rel_ref_point; // relative point of reference, pseudo-bone
	bool m_got_target; // do we have any alive target the player did shoot at, if not consider the target pos as a bullet impact
	bool m_target_visible; // is that target visible
	bool m_primary_shoot; // is the player shooting
};

typedef Throwback<AimTickInfo, int, 256> data_throwback_t;

class AimTester :
	public BaseTesterSystem,
	public SourceSdk::IGameEventListener002,
	public PlayerDataStructHandler<data_throwback_t>,
	public PlayerRunCommandHookListener,
	public Singleton
{
	typedef PlayerDataStructHandler<data_throwback_t> playerdata_class;

public:
	AimTester();
	virtual ~AimTester() final;

private:
	virtual void Init() override final;

	virtual void Load() override final;

	virtual void Unload() override final;

	virtual bool GotJob() const override final;

	virtual void FireGameEvent(SourceSdk::IGameEvent *ev) override final;

	virtual PlayerRunCommandRet RT_PlayerRunCommandCallback(PlayerHandler::iterator ph, void * const cmd, double const & curtime) override final;
};

#endif // AIMTESTER_H
