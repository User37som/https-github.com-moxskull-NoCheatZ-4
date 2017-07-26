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

#ifndef MOUSETESTER_H
#define MOUSETESTER_H

#include "Systems/BaseSystem.h"
#include "Players/temp_PlayerDataStruct.h"
#include "Hooks/PlayerRunCommandHookListener.h"
#include "Misc/temp_singleton.h"
#include "Systems/Testers/Detections/temp_BaseDetection.h"

struct MouseInfo
{
	float m_prev_yaw_angle;
	float m_prev_pitch_angle;

	int m_pitch_dir_detect_row;
	int m_yaw_dir_detect_row;

	bool m_prev_set;
	bool m_pitch_set;
	bool m_asked_m_pitch;
	bool m_mouse_pitch_inverted;

	MouseInfo() :
		m_prev_yaw_angle(0.0f),
		m_prev_pitch_angle(0.0f),
		m_pitch_dir_detect_row(0),
		m_yaw_dir_detect_row(0),
		m_prev_set(false),
		m_pitch_set(false),
		m_mouse_pitch_inverted(false)
	{}
};

struct MouseDetectInfo
{
	float m_prev_pitch_angle;
	float m_cur_pitch_angle;
	float m_pitch_delta;
	int m_mouse_pitch_inverted;
	short m_mouse_delta_y;
	float m_prev_yaw_angle;
	float m_cur_yaw_angle;
	float m_yaw_delta;
	short m_mouse_delta_x;
	int m_buttons;

	MouseDetectInfo()
	{}

	MouseDetectInfo(MouseInfo const & other, short const * md, int const * buttons, SourceSdk::QAngle const * cur_angles, float pitch_delta, float yaw_delta) :
		m_prev_pitch_angle(other.m_prev_pitch_angle),
		m_cur_pitch_angle(cur_angles->x),
		m_pitch_delta(pitch_delta),
		m_mouse_pitch_inverted(other.m_mouse_pitch_inverted),
		m_mouse_delta_y(md[1]),
		m_prev_yaw_angle(other.m_prev_yaw_angle),
		m_cur_yaw_angle(cur_angles->y),
		m_yaw_delta(yaw_delta),
		m_mouse_delta_x(md[0]),
		m_buttons(*buttons)
	{}
};

class Detection_MouseMismatch :
	public LogDetection<MouseDetectInfo>
{
	typedef LogDetection<MouseDetectInfo> hClass;

public:
	virtual basic_string GetDetectionLogMessage() override final
	{
		return "aimbot";
	};

	virtual basic_string GetDataDump() override final;
};

class MouseTester :
	public BaseTesterSystem,
	public PlayerDataStructHandler<MouseInfo>,
	public PlayerRunCommandHookListener,
	public Singleton
{
	typedef PlayerDataStructHandler<MouseInfo> playerdatahandler_class;

public:
	MouseTester();
	virtual ~MouseTester() final;

	virtual void Init() override final;

	virtual void Load() override final;

	virtual void Unload() override final;

	virtual bool GotJob() const override final;

	virtual PlayerRunCommandRet RT_PlayerRunCommandCallback(PlayerHandler::iterator ph, void * const cmd, void * const old_cmd) override final;

	void ProcessPitchConVar(PlayerHandler::iterator ph, char const * val);
};

extern MouseTester g_MouseTester;

#endif // MOUSETESTER_H
