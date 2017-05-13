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

#include "Systems/Testers/Detections/temp_BaseDetection.h" // + basic_string + memset/cpy + logger + basesystem + singleton + helpers + cutlvector
#include "Players/temp_PlayerDataStruct.h"
#include "Hooks/PlayerRunCommandHookListener.h"

typedef struct EyeAngle
{
	float value; // Raw value of the angle
	float abs_value; // Abs value so it's easier to test
	float lastDetectionPrintTime;

	EyeAngle ()
	{
		memset ( this, 0, sizeof ( EyeAngle ) );
	};
	EyeAngle ( const EyeAngle& other )
	{
		memcpy ( this, &other, sizeof ( EyeAngle ) );
	};
} EyeAngleT;

typedef struct EyeAngleInfo : 
	public Helpers::CRC32_Specialize
{
	unsigned int ignore_last; // Ignore values potentially not initialized by the engine

	EyeAngleT x;
	EyeAngleT y;
	EyeAngleT z;

	unsigned int detectionsCount;

	EyeAngleInfo ()
	{
		memset ( this, 0, sizeof ( EyeAngleInfo ) );
	};
	EyeAngleInfo ( const EyeAngleInfo& other )
	{
		memcpy ( this, &other, sizeof ( EyeAngleInfo ) );
	};

	virtual uint32_t Hash_CRC32 () const
	{
		Helpers::CRC32_Digestive ctx;
		ctx.Digest ( &( x.abs_value ), sizeof ( float ) );
		ctx.Digest ( &( y.abs_value ), sizeof ( float ) );
		ctx.Digest ( &( z.abs_value ), sizeof ( float ) );
		return ctx.Final ();
	}
} EyeAngleInfoT;

class Base_Detection_EyeAngle : public LogDetection<EyeAngleInfoT>
{
protected:
	typedef LogDetection<EyeAngleInfoT> hClass;
public:
	Base_Detection_EyeAngle ( PlayerHandler::const_iterator player, BaseDynamicSystem * tester, uint32_t udid, hClass::data_t const * data ) :
							  hClass ( player, tester, udid, data )
	{};
	virtual ~Base_Detection_EyeAngle ()
	{};

	virtual void TakeAction () override final;

	virtual void WriteXMLOutput ( FILE * const ) const final;

	virtual bool CloneWhenEqual () const final
	{
		return true;
	}

	virtual basic_string GetDetectionLogMessage () const = 0;
};

class Detection_EyeAngleX : public Base_Detection_EyeAngle
{
public:
	Detection_EyeAngleX ( PlayerHandler::const_iterator player, BaseDynamicSystem * tester, hClass::data_t const * data ) :
		Base_Detection_EyeAngle ( player, tester, UniqueDetectionID::EYEANGLE_X, data )
	{};
	virtual ~Detection_EyeAngleX () final
	{};

	virtual basic_string GetDetectionLogMessage () const final;
};

class Detection_EyeAngleY : public Base_Detection_EyeAngle
{
public:
	Detection_EyeAngleY ( PlayerHandler::const_iterator player, BaseDynamicSystem * tester, hClass::data_t const * data ) :
		Base_Detection_EyeAngle ( player, tester, UniqueDetectionID::EYEANGLE_Y, data )
	{};
	virtual ~Detection_EyeAngleY () final
	{};

	virtual basic_string GetDetectionLogMessage () const final;
};

class Detection_EyeAngleZ : public Base_Detection_EyeAngle
{
public:
	Detection_EyeAngleZ ( PlayerHandler::const_iterator player, BaseDynamicSystem * tester, hClass::data_t const * data ) :
		Base_Detection_EyeAngle ( player, tester, UniqueDetectionID::EYEANGLE_Z, data )
	{};
	virtual ~Detection_EyeAngleZ () final
	{};

	virtual basic_string GetDetectionLogMessage () const final;
};

class EyeAnglesTester :
	public BaseDynamicSystem,
	public SourceSdk::IGameEventListener002,
	public PlayerDataStructHandler<EyeAngleInfoT>,
	public PlayerRunCommandHookListener,
	public Singleton<EyeAnglesTester>
{
	typedef Singleton<EyeAnglesTester> singleton_class;
	typedef PlayerDataStructHandler<EyeAngleInfoT> playerdata_class;

public:
	EyeAnglesTester ();
	virtual ~EyeAnglesTester () final;

private:
	virtual void Init () override final;

	virtual void Load () override final;

	virtual void Unload () override final;

	virtual bool GotJob () const override final;

	virtual void FireGameEvent ( SourceSdk::IGameEvent *ev ) override final;

	virtual PlayerRunCommandRet RT_PlayerRunCommandCallback ( PlayerHandler::const_iterator ph, void * const cmd, void * const old_cmd ) override final;
};

#endif
