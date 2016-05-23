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

#ifndef ShotTester_H
#define ShotTester_H

#include "Systems/BaseSystem.h"
#include "Hooks/PlayerRunCommandHookListener.h"
#include "Players/temp_PlayerDataStruct.h"
#include "Systems/Testers/Detections/temp_BaseDetection.h"
#include "Misc/temp_singleton.h"

#define SHORT_TIME 0.04 // sec

typedef struct ShotStatHandler
{
	size_t n;
	float avg_time;
	float ratio;

	ShotStatHandler()
	{
		n = 0;
		avg_time = ratio = 0.0;
	};
	ShotStatHandler(const ShotStatHandler& other)
	{
		n = other.n;
		avg_time = other.avg_time;
		ratio = other.ratio;
	};
} ShotStatHandlerT;

void TriggerStat(ShotStatHandlerT* handler);
void OutputStat(ShotStatHandlerT* handler);

typedef struct ShotStats
{
	float up_time; // Heure o� le bouton est appuy�
	float down_time; // Heure o� le bouton est rel�ch�
	ShotStatHandlerT clicks; // Nombre de click total
	ShotStatHandlerT short_clicks; // Nombre de click trop courts :
	ShotStatHandlerT with_hand;    // - D�sarm�
	ShotStatHandlerT with_pistol;  // - Avec un pistolet ou un fusil � pompe
	ShotStatHandlerT with_auto;    // - Avec une arme automatique
	ShotStatHandlerT on_target;    // - En visant un adversaire
	size_t row; // D�tections cons�cutives
	float last_detection; // Heure de la derni�re d�tection

	ShotStats()
	{
		up_time = down_time = last_detection = 0;
		row = 0;
		clicks = short_clicks = with_hand = with_pistol = with_auto = on_target = ShotStatHandler();
	};
	ShotStats(const ShotStats& other)
	{
		up_time = other.up_time;
		down_time = other.down_time;
		clicks = other.clicks;
		short_clicks = other.short_clicks;
		with_hand = other.with_hand;
		with_pistol = other.with_pistol;
		with_auto = other.with_auto;
		on_target = other.on_target;
		row = other.row;
		last_detection = other.last_detection;
	};
} ShotStatsT;

class ShotTester :
	public BaseSystem,
	public PlayerRunCommandHookListener,
	public PlayerDataStructHandler<ShotStatsT>,
	public Singleton<ShotTester>
{
	typedef PlayerDataStructHandler<ShotStatsT> playerdata_class;
	typedef Singleton<ShotTester> singleton_class;

public:
	ShotTester();
	~ShotTester();

	void Init();
	void Load();
	void Unload();
	SlotStatus GetFilter();
	PlayerRunCommandRet PlayerRunCommandCallback(NczPlayer* player, void* pCmd, void* old_cmd);
};

class ShotDetection : public LogDetection<ShotStats>
{
	typedef LogDetection<ShotStats> hClass;
public:
	ShotDetection() : hClass() {};
	~ShotDetection() {};

	virtual basic_string GetDataDump();
	virtual basic_string GetDetectionLogMessage()
	{
		return "ShotDetection";
	};
};

class Detection_TriggerBot : public ShotDetection
{
public:
	Detection_TriggerBot() : ShotDetection() {};
	~Detection_TriggerBot() {};

	virtual basic_string GetDetectionLogMessage()
	{
		return "TriggerBot";
	};
};

class Detection_AutoPistol : public ShotDetection
{
public:
	Detection_AutoPistol() : ShotDetection() {};
	~Detection_AutoPistol() {};

	virtual basic_string GetDetectionLogMessage()
	{
		return "AutoPistol";
	};
};

#endif
