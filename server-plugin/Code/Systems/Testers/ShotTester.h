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

#ifndef MUTE_INCLUDES_IN_HEADERS

#	include "Systems/BaseSystem.h"
#	include "Hooks/PlayerRunCommandHookListener.h"
#	include "Players/temp_PlayerDataStruct.h"
#	include "Systems/Testers/Detections/temp_BaseDetection.h"
#	include "Misc/temp_singleton.h"

#endif

#define SHORT_TIME 0.04 // sec

typedef struct ShotStatHandler
{
	size_t n;
	float avg_time;
	float ratio;

	ShotStatHandler ()
	{
		memset ( this, 0, sizeof ( ShotStatHandler ) );
	};
	ShotStatHandler ( const ShotStatHandler& other )
	{
		memcpy ( this, &other, sizeof ( ShotStatHandler ) );
	};
} ShotStatHandlerT;

void TriggerStat ( ShotStatHandlerT* handler );
void OutputStat ( ShotStatHandlerT* handler );

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

	ShotStats ()
	{
		memset ( this, 0, sizeof ( ShotStats ) );
	};
	ShotStats ( const ShotStats& other )
	{
		memcpy ( this, &other, sizeof ( ShotStats ) );
	};
} ShotStatsT;

class ShotTester :
	public BaseDynamicSystem,
	public PlayerDataStructHandler<ShotStatsT>,
	public PlayerRunCommandHookListener,
	public Singleton<ShotTester>
{
	typedef PlayerDataStructHandler<ShotStatsT> playerdata_class;
	typedef Singleton<ShotTester> singleton_class;

public:
	ShotTester ();

	virtual ~ShotTester () final;

	virtual void Init () override final;

	virtual void Load () override final;

	virtual void Unload () override final;

	virtual bool GotJob () const override final;

	virtual PlayerRunCommandRet RT_PlayerRunCommandCallback ( PlayerHandler::const_iterator ph, void * const pCmd, void * const old_cmd ) override final;
};

class ShotDetection : public LogDetection<ShotStats>
{
	typedef LogDetection<ShotStats> hClass;
public:
	ShotDetection () : hClass ()
	{};
	virtual ~ShotDetection () override
	{};

	virtual basic_string GetDataDump () override final;
	virtual basic_string GetDetectionLogMessage () override
	{
		return "ShotDetection";
	};
};

class Detection_TriggerBot : public ShotDetection
{
public:
	Detection_TriggerBot () : ShotDetection ()
	{};
	virtual ~Detection_TriggerBot () override final
	{};

	virtual basic_string GetDetectionLogMessage () override final
	{
		return "TriggerBot";
	};
};

class Detection_AutoPistol : public ShotDetection
{
public:
	Detection_AutoPistol () : ShotDetection ()
	{};
	virtual ~Detection_AutoPistol () override final
	{};

	virtual basic_string GetDetectionLogMessage () override final
	{
		return "AutoPistol";
	};
};

#endif
