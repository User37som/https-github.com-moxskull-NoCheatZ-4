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
#include "Misc/temp_Throwback.h"

#define TB_MOUSE_MAX_HISTORY 64

struct MouseFrame
{
	float m_pitch_angle;
	float m_yaw_angle;
	int m_buttons;
	SlotStatus m_status;
	short m_x_mouse;
	short m_y_mouse;
	bool m_frozen;

	MouseFrame() :
		m_pitch_angle(0.0f),
		m_yaw_angle(0.0f),
		m_buttons(0),
		m_status(SlotStatus::INVALID),
		m_x_mouse(0),
		m_y_mouse(0),
		m_frozen(false)
	{}

	MouseFrame(float const * angles, short const * mouse, int const * buttons, SlotStatus status, bool frozen) :
		m_pitch_angle(angles[0]),
		m_yaw_angle(angles[1]),
		m_buttons(*buttons),
		m_status(status),
		m_x_mouse(mouse[0]),
		m_y_mouse(mouse[1]),
		m_frozen(frozen)
	{}
};

typedef Throwback<MouseFrame, double, TB_MOUSE_MAX_HISTORY> tb_mouse;

struct MouseDetectInfo
{
	tb_mouse m_frames;
	size_t c;
	float percent_detected;
	size_t min_consecutive_x;
	size_t cur_consecutive_x;
	size_t min_consecutive_y;
	size_t cur_consecutive_y;
	bool m_inverted;
	bool m_pitch_set;
	bool m_pitch_asked;

	MouseDetectInfo() :
		m_frames(),
		c(0),
		percent_detected(0.0f),
		min_consecutive_x(0),
		cur_consecutive_x(0),
		min_consecutive_y(0),
		cur_consecutive_y(0),
		m_inverted(false),
		m_pitch_set(false),
		m_pitch_asked(false)
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
	public PlayerDataStructHandler<MouseDetectInfo>,
	public PlayerRunCommandHookListener,
	public Singleton
{
	typedef PlayerDataStructHandler<MouseDetectInfo> playerdatahandler_class;

public:
	MouseTester();
	virtual ~MouseTester() final;

	virtual void Init() override final;

	virtual void Load() override final;

	virtual void Unload() override final;

	virtual bool GotJob() const override final;

	virtual PlayerRunCommandRet RT_PlayerRunCommandCallback(PlayerHandler::iterator ph, void * const cmd, double const & curtime) override final;

	void ProcessPitchConVar(PlayerHandler::iterator ph, char const * val);

	void FindDetection(PlayerHandler::iterator ph, MouseDetectInfo* graph);
};

extern MouseTester g_MouseTester;

#endif // MOUSETESTER_H
