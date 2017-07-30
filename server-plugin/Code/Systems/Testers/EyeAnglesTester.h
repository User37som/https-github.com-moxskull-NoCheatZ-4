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

#ifndef EYEANGLESTESTER
#define EYEANGLESTESTER

#include "Systems/BaseSystem.h"
#include "Hooks/PlayerRunCommandHookListener.h"
#include "Hooks/UserMessageHookListener.h"
#include "Players/temp_PlayerDataStruct.h"
#include "Interfaces/IGameEventManager/IGameEventManager.h"
#include "Systems/Testers/Detections/temp_BaseDetection.h"
#include "Misc/temp_singleton.h"

typedef struct EyeAngle
{
	float value; // Raw value of the angle
	float abs_value; // Abs value so it's easier to test

	double lastDetectionPrintTime;
	unsigned int detectionsCount;

	EyeAngle ()
	{
		memset ( this, 0, sizeof ( EyeAngle ) );
	};
	EyeAngle ( const EyeAngle& other )
	{
		memcpy ( this, &other, sizeof ( EyeAngle ) );
	};
} EyeAngleT;

typedef struct EyeAngleInfo
{
	EyeAngleT past_x;
	EyeAngleT past_y;
	EyeAngleT past_z;
	EyeAngleT x;
	EyeAngleT y;
	EyeAngleT z;

	EyeAngleInfo ()
	{
		memset ( this, 0, sizeof ( EyeAngleInfo ) );
	};
	EyeAngleInfo ( const EyeAngleInfo& other )
	{
		memcpy ( this, &other, sizeof ( EyeAngleInfo ) );
	};
} EyeAngleInfoT;

class Detection_EyeAngle : public LogDetection<EyeAngleInfoT>
{
	typedef LogDetection<EyeAngleInfoT> hClass;
public:
	Detection_EyeAngle () : hClass ()
	{};
	virtual ~Detection_EyeAngle ()
	{};

	virtual basic_string GetDataDump () final;
};

class Detection_EyeAngleX : public Detection_EyeAngle
{
public:
	Detection_EyeAngleX ()
	{};
	virtual ~Detection_EyeAngleX () final
	{};

	virtual basic_string GetDetectionLogMessage () final;
};

class Detection_EyeAngleY : public Detection_EyeAngle
{
public:
	Detection_EyeAngleY ()
	{};
	virtual ~Detection_EyeAngleY () final
	{};

	virtual basic_string GetDetectionLogMessage () final;
};

class Detection_EyeAngleZ : public Detection_EyeAngle
{
public:
	Detection_EyeAngleZ ()
	{};
	virtual ~Detection_EyeAngleZ () final
	{};

	virtual basic_string GetDetectionLogMessage () final;
};

class EyeAnglesTester :
	public BaseTesterSystem,
	public PlayerDataStructHandler<EyeAngleInfoT>,
	public PlayerRunCommandHookListener,
	public UserMessageHookListener,
	public Singleton
{
	typedef PlayerDataStructHandler<EyeAngleInfoT> playerdata_class;

public:
	EyeAnglesTester ();
	virtual ~EyeAnglesTester () final;

private:
	virtual void Init () override final;

	virtual void Load () override final;

	virtual void Unload () override final;

	virtual bool GotJob () const override final;

	virtual PlayerRunCommandRet RT_PlayerRunCommandCallback ( PlayerHandler::iterator ph, void * const cmd, double const & curtime ) override final;

	virtual bool RT_SendUserMessageCallback(SourceSdk::IRecipientFilter const &, int const, google::protobuf::Message const &);

	virtual bool RT_UserMessageBeginCallback(SourceSdk::IRecipientFilter const * const, int const);

	virtual void RT_MessageEndCallback(SourceSdk::IRecipientFilter const * const, int const, SourceSdk::bf_write* buffer);
};

extern EyeAnglesTester g_EyeAnglesTester;

#endif
