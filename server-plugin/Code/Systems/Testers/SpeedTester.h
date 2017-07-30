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

#ifndef SPEEDTESTER_H
#define SPEEDTESTER_H

#include <cmath>

#include "Interfaces/InterfacesProxy.h"

#include "Systems/Testers/Detections/temp_BaseDetection.h"
#include "Systems/BaseSystem.h"
#include "Systems/OnTickListener.h"
#include "Hooks/PlayerRunCommandHookListener.h"
#include "Players/temp_PlayerDataStruct.h"
#include "Misc/temp_singleton.h"

/////////////////////////////////////////////////////////////////////////
// SpeedTester
/////////////////////////////////////////////////////////////////////////

typedef struct SpeedHolder
{
	float ticksLeft;
	size_t detections;
	double lastDetectionTime;
	float previousLatency;
	double lastTest;

	SpeedHolder ()
	{
		if( SourceSdk::InterfacesProxy::_vfptr_GetTickInterval )
			ticksLeft = ceil ( ( 1.0f / SourceSdk::InterfacesProxy::Call_GetTickInterval () ) * 2.0f );
		else
			ticksLeft = 0;
		detections = 0;
		lastDetectionTime = lastTest = 0.0;
		previousLatency = 0.0f;
	};
	SpeedHolder ( const SpeedHolder& other )
	{
		memset ( this, 0, sizeof ( SpeedHolder ) );
	};
} SpeedHolderT;

class Detection_SpeedHack : public LogDetection<SpeedHolderT>
{
	typedef LogDetection<SpeedHolderT> hClass;
public:
	Detection_SpeedHack () : hClass ()
	{};
	virtual ~Detection_SpeedHack () final
	{};

	virtual basic_string GetDataDump () final;
	virtual basic_string GetDetectionLogMessage () final
	{
		return "SpeedHack";
	};
};

class SpeedTester :
	private BaseTesterSystem,
	private OnTickListener,
	public PlayerDataStructHandler<SpeedHolderT>,
	private PlayerRunCommandHookListener,
	public Singleton
{
	typedef PlayerDataStructHandler<SpeedHolderT> playerdata_class;

public:
	SpeedTester ();
	virtual ~SpeedTester () final;

private:
	virtual void Init () override final;

	virtual void Load () override final;

	virtual void Unload () override final;

	virtual bool GotJob () const override final;

	virtual void RT_ProcessOnTick (double const & curtime ) override final;

	virtual PlayerRunCommandRet RT_PlayerRunCommandCallback ( PlayerHandler::iterator ph, void * const cmd, double const & curtime) override final;
};

extern SpeedTester g_SpeedTester;

#endif // SPEEDTESTER_H
