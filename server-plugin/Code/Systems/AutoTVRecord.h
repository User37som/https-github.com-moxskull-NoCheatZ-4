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

#ifndef AUTOTVRECORD_H
#define AUTOTVRECORD_H

#include "BaseSystem.h"
#include "OnTickListener.h"
#include "Hooks/ConCommandHookListener.h"
#include "Misc/temp_basicstring.h"
#include "Misc/temp_singleton.h"
#include "TimerListener.h"

/*
	This system will :
		- Spawn a TV (reload the map if TV is not present)
		- Auto record only when at least 1 human player is playing.
*/

class AutoTVRecord :
	private BaseDynamicSystem,
	private TimerListener,
	public Singleton
{
public:
	typedef enum demo_split : size_t
	{
		SPLIT_BY_MAP = 0,
		SPLIT_BY_ROUNDS,
		SPLIT_BY_TIMER_SECONDS,
		SPLIT_BY_DETECTION
	} demo_split_t;

private:
	basic_string m_prefix;
	double m_waitfortv_time;
	int m_minplayers;
	demo_split_t m_splitrule;
	unsigned int m_round_id;
	unsigned int m_max_rounds;
	float m_splittimer_seconds;
	unsigned int m_current_detected_players;
	bool m_spawn_once;

public:

	AutoTVRecord ();
	virtual ~AutoTVRecord () final;

private:
	virtual void Init () override final;

	virtual bool sys_cmd_fn(const SourceSdk::CCommand &args) override final;

	virtual void Load () override final;

	virtual void Unload () override final;

	virtual bool GotJob () const override final;

	virtual void RT_TimerCallback(char const * const timer_name /* Pointer can be invalid past the function */) override final;

public:
	void StartRecord ();

	void StopRecord ();

	inline void SplitRecord();

	void OnRoundStart();

	// How much human players must be in the game before we start recording. 1 or 2 are great values.
	void SetMinPlayers ( int min );

	inline int GetMinPlayers();

	// Prefix in the filename of records
	void SetRecordPrefix ( basic_string const & prefix );

	//	Will try to spawn the TV once and once a client connects (ClientActive)
	void SpawnTV ();

	inline void DeclareDetectedPlayer();

	void OnDetectedPlayerDisconnect();
};

inline void AutoTVRecord::SplitRecord()
{
	StopRecord(); StartRecord();
}

inline void AutoTVRecord::DeclareDetectedPlayer()
{
	++m_current_detected_players;
	if (m_splitrule == demo_split_t::SPLIT_BY_DETECTION)
	{
		StartRecord();
	}
}

inline void AutoTVRecord::OnDetectedPlayerDisconnect()
{
	--m_current_detected_players;
	if (m_splitrule == demo_split_t::SPLIT_BY_DETECTION && m_current_detected_players == 0)
	{
		StopRecord();
	}
}

inline int AutoTVRecord::GetMinPlayers()
{
	return m_minplayers;
}

extern AutoTVRecord g_AutoTVRecord;

class TVWatcher :
	private BaseStaticSystem,
	private ConCommandHookListener,
	public Singleton
{

private:
	basic_string m_demofile;
	size_t m_recordtickcount;
	bool m_recording;

public:
	TVWatcher();
	virtual ~TVWatcher() final;

	virtual void Init() override final;

	virtual bool RT_ConCommandCallback(PlayerHandler::iterator ph, void * const cmd, SourceSdk::CCommand const & args) override final;

	void RecordStarted(SourceSdk::CCommand const & args);

	void RecordEnded();

	void RT_OnTick();

	// What is the current record tick (The tick will be notified in detection logs, so it will be easier to seek detections in demos)
	size_t GetRecordTick() const;

	bool IsRecording() const;

	// Tell if TV is present
	bool IsTVPresent() const;

	// The current filename
	basic_string const & GetRecordFilename() const;

	// Send a chat message to the TV and to all viewers.
	void SendTVChatMessage(basic_string const & msg);
};

extern TVWatcher g_TVWatcher;

#endif
