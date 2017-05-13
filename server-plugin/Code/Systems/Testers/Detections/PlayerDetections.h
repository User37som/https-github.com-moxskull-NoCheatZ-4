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

#ifndef PLAYERDETECTIONS_H
#define PLAYERDETECTIONS_H

#include "Containers/utlvector.h" // CUtlVector

#include "Misc/ForwardedCommonDefinitions.h"
#include "Misc/Helpers.h" // game_tm
#include "Misc/temp_basicstring.h"

class PlayerDetections
{
public:

	struct rt_detection_elem
	{
		Helpers::game_tm m_time;
		BaseDetection* m_detection;

		rt_detection_elem ();

		rt_detection_elem ( Helpers::game_tm& time, BaseDetection* detection );

		bool operator==( rt_detection_elem const & other );
	};

	typedef CUtlVector<rt_detection_elem> rt_detection_list;

	struct rt_detection_list_by_udid
	{
		uint32_t m_udid;
		rt_detection_list m_list;

		rt_detection_list_by_udid ();

		rt_detection_list_by_udid ( uint32_t udid );

		bool operator==( rt_detection_list_by_udid const & other ) const;
	};

	typedef CUtlVector<rt_detection_list_by_udid> rt_detection_dict;

private:
	size_t m_playerindex;
	rt_detection_dict m_dict;

public:

	PlayerDetections ( size_t playerindex );

	~PlayerDetections ();

	/*
	Add a detection in dictionnary
	*/
	void AddDetection ( BaseDetection * detection, Helpers::game_tm& time );

	int GetTotalDetectionsCount () const;

	/*
	Get kick or ban message relative to detection(s) we currently have
	*/
	basic_string GetActionMessage () const;

	static int _cdecl detection_list_compare ( rt_detection_elem const * a, rt_detection_elem const * b );

	/*
	Write down every detections and free them
	*/
	void FlushDetections ();
};

#endif // PLAYERDETECTIONS_H
