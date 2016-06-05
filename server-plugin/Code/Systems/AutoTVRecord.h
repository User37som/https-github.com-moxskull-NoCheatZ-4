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

/*
	This system will :
		- Spawn a TV (reload the map if TV is not present)
		- Auto record only when at least 1 human player is playing.
*/

class AutoTVRecord : 
	private BaseSystem,
	private ConCommandHookListener,
	public Singleton<AutoTVRecord>
{
	typedef Singleton<AutoTVRecord> singleton_class;

private:
	basic_string m_demofile;
	basic_string m_prefix;
	size_t m_recordtickcount;
	float m_waitfortv_time;
	int m_tvslot;
	int m_minplayers;
	bool m_recording;
	bool m_expectedtvconfigchange;

public:
	AutoTVRecord();
	virtual ~AutoTVRecord() final;

private:
	virtual void Init() override final;

	virtual void Load() override final;

	virtual void Unload() override final;

	virtual bool ConCommandCallback(NczPlayer * const player, void * const cmd, SourceSdk::CCommand const & args) override final;

	void StartRecord();

	void StopRecord();

public:
	void OnTick();

	// How much human players must be in the game before we start recording. 1 or 2 are great values.
	void SetMinPlayers(int min);

	// Prefix in the filename of records
	void SetRecordPrefix(basic_string const & prefix);

	// What is the current record tick (The tick will be notified in detection logs, so it will be easier to seek detections in demos)
	size_t GetRecordTick() const;

	bool IsRecording() const;

	// The index of SourceTV
	int GetSlot();

	void SpawnTV();

	// The current filename, only the filename.
	basic_string const & GetRecordFilename() const;

	// Send a chat message to the TV and to all viewers.
	void SendTVChatMessage(basic_string const & msg);
};

#endif
