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
#include "Misc/temp_basicstring.h"

/*
	This system will :
		- Spawn a TV (reload the map if TV is not present)
		- Auto record only when at least 1 human player is playing.
*/

class AutoTVRecord : 
	public BaseSystem
{
private:
	void Init();
	void Load();
	void Unload();

	basic_string m_demofile;
	int m_tvslot;
	int m_recordstarttick;
	int m_minplayers;
	bool m_recording;

public:
	AutoTVRecord();
	~AutoTVRecord();

	// How much human players must be in the game before we start recording. 1 or 2 are great values.
	void SetMinPlayers(int min);

	// Prefix in the filename of records
	void SetRecordPrefix(basic_string const & directory);

	// What is the current record tick (The tick will be notified in detection logs, so it will be easier to seek the detection)
	int GetRecordTick();

	bool IsRecording();

	// The index of SourceTV
	int GetSlot();

	// The current filename, only the filename.
	basic_string GetRecordFilename();

	// Send a chat message to the TV and to all viewers.
	void SendTVChatMessage(basic_string const & msg);
};

extern AutoTVRecord g_AutoTVRecord;

#endif
