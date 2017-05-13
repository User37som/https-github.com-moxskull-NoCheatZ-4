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

#include "Systems/Testers/Detections/temp_BaseDetection.h"
#include "Systems/OnTickListener.h"
#include "Hooks/PlayerRunCommandHookListener.h"
#include "Players/temp_PlayerDataStruct.h"

/////////////////////////////////////////////////////////////////////////
// SpeedTester
/////////////////////////////////////////////////////////////////////////

typedef struct SpeedHolder : 
	public Helpers::CRC32_Specialize
{
	float ticksLeft;
	size_t detections;
	float lastDetectionTime;
	float previousLatency;
	float lastTest;

	SpeedHolder ()
	{
		if( SourceSdk::InterfacesProxy::_vfptr_GetTickInterval )
			ticksLeft = ceil ( ( 1.0f / SourceSdk::InterfacesProxy::Call_GetTickInterval () ) * 2.0f );
		else
			ticksLeft = 0;
		detections = 0;
		lastDetectionTime = previousLatency = lastTest = 0.0;
	};
	SpeedHolder ( const SpeedHolder& other )
	{
		memset ( this, 0, sizeof ( SpeedHolder ) );
	};

	virtual uint32_t Hash_CRC32 () const
	{
		return 0;
	}
} SpeedHolderT;

class Detection_SpeedHack : public LogDetection<SpeedHolderT>
{
	typedef LogDetection<SpeedHolderT> hClass;
public:
	Detection_SpeedHack ( PlayerHandler::const_iterator player, BaseDynamicSystem * tester, hClass::data_t const * data ) :
		hClass ( player, tester, UniqueDetectionID::SPEEDHACK, data )
	{};
	virtual ~Detection_SpeedHack ()
	{};

	virtual void TakeAction () override final
	{
		m_player->Ban ();
	}

	virtual void WriteXMLOutput ( FILE * const ) const final;

	virtual bool CloneWhenEqual () const final
	{
		return false;
	}

	virtual basic_string GetDetectionLogMessage () const final
	{
		return "SpeedHack";
	};
};

class SpeedTester :
	private BaseDynamicSystem,
	private OnTickListener,
	public PlayerDataStructHandler<SpeedHolderT>,
	private PlayerRunCommandHookListener,
	public Singleton<SpeedTester>
{
	typedef PlayerDataStructHandler<SpeedHolderT> playerdata_class;
	typedef Singleton<SpeedTester> singleton_class;

public:
	SpeedTester ();
	virtual ~SpeedTester () final;

private:
	virtual void Init () override final;

	virtual void Load () override final;

	virtual void Unload () override final;

	virtual bool GotJob () const override final;

	virtual void RT_ProcessOnTick ( float const curtime ) override final;

	virtual PlayerRunCommandRet RT_PlayerRunCommandCallback ( PlayerHandler::const_iterator ph, void * const cmd, void * const old_cmd ) override final;
};

#endif // SPEEDTESTER_H
