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

#ifndef PROCESSFILTER_H
#define PROCESSFILTER_H

typedef enum class SlotStatus : unsigned int
{
	INVALID = 0, // Slot not used
	KICK, // In process of being kicked or banned
	TV, // Is a TV slot
	BOT, // A bot ...
	PLAYER_CONNECTING, // Not a bot, not connected
	PLAYER_CONNECTED, // Connected as spectator or dead
	PLAYER_IN_TESTS, // Playing the round and shooting people everywhere like a mad nerd :)
} SlotStatus_t;

enum SlotFilterBehavior
{
	STATUS_EQUAL_OR_BETTER = 0,
	STATUS_BETTER,
	STATUS_STRICT
};

class BaseProcessFilter
{
public:
	virtual bool CanProcessThisSlot ( SlotStatus const player_slot_status ) const = 0;
};

namespace TemplatedProcessFilter
{
	template <SlotFilterBehavior m_behavior, bool m_process_bots, SlotStatus m_status>
	inline bool CanProcessThisSlot ( SlotStatus const player_slot_status );

	template <SlotStatus_t m_status, SlotFilterBehavior m_behavior>
	struct player_test_spec
	{
		static inline bool PlayerTestComponent ( SlotStatus const player_slot_status );
	};

	template <SlotFilterBehavior m_behavior, SlotStatus m_status, bool m_process_bots>
	struct bot_test_spec
	{
		static inline bool BotTestComponent ( SlotStatus const player_slot_status );
	};

	template <SlotStatus_t m_status>
	struct player_test_spec<m_status, STATUS_EQUAL_OR_BETTER>
	{
		static inline bool PlayerTestComponent ( SlotStatus const player_slot_status )
		{
			return player_slot_status >= m_status;
		}
	};

	template <SlotStatus_t m_status>
	struct player_test_spec<m_status, STATUS_BETTER>
	{
		static inline bool PlayerTestComponent ( SlotStatus const player_slot_status )
		{
			return player_slot_status > m_status;
		}
	};

	template <SlotStatus_t m_status>
	struct player_test_spec<m_status, STATUS_STRICT>
	{
		static inline bool PlayerTestComponent ( SlotStatus const player_slot_status )
		{
			return player_slot_status == m_status;
		}
	};

	template<SlotFilterBehavior m_behavior, SlotStatus_t m_status>
	struct bot_test_spec<m_behavior, m_status, true>
	{
		static inline bool BotTestComponent ( SlotStatus const player_slot_status )
		{
			return player_test_spec<m_status, m_behavior>::PlayerTestComponent ( player_slot_status ) || player_slot_status == SlotStatus::BOT;
		}
	};

	template<SlotFilterBehavior m_behavior, SlotStatus_t m_status>
	struct bot_test_spec<m_behavior, m_status, false>
	{
		static inline bool BotTestComponent ( SlotStatus const player_slot_status )
		{
			return player_test_spec<m_status, m_behavior>::PlayerTestComponent ( player_slot_status ) && player_slot_status != SlotStatus::BOT;
		}
	};

	template <SlotFilterBehavior m_behavior, bool m_process_bots, SlotStatus m_status>
	inline bool CanProcessThisSlot ( SlotStatus const player_slot_status )
	{
		return bot_test_spec<m_behavior, m_status, m_process_bots>::BotTestComponent ( player_slot_status );
	}
}

namespace ProcessFilter
{
	class InTestsOrBot :
		public BaseProcessFilter
	{
		virtual bool CanProcessThisSlot ( SlotStatus const player_slot_status ) const
		{
			return TemplatedProcessFilter::CanProcessThisSlot<STATUS_EQUAL_OR_BETTER, true, SlotStatus::PLAYER_IN_TESTS> ( player_slot_status );
		}
	};

	class InTestsNoBot :
		public BaseProcessFilter
	{
		virtual bool CanProcessThisSlot ( SlotStatus const player_slot_status ) const
		{
			return TemplatedProcessFilter::CanProcessThisSlot<STATUS_EQUAL_OR_BETTER, false, SlotStatus::PLAYER_IN_TESTS> ( player_slot_status );
		}
	};

	class HumanAtLeastConnected :
		public BaseProcessFilter
	{
		virtual bool CanProcessThisSlot ( SlotStatus const player_slot_status ) const
		{
			return TemplatedProcessFilter::CanProcessThisSlot<STATUS_EQUAL_OR_BETTER, false, SlotStatus::PLAYER_CONNECTED> ( player_slot_status );
		}
	};

	class HumanAtLeastConnecting :
		public BaseProcessFilter
	{
		virtual bool CanProcessThisSlot ( SlotStatus const player_slot_status ) const
		{
			return TemplatedProcessFilter::CanProcessThisSlot<STATUS_EQUAL_OR_BETTER, false, SlotStatus::PLAYER_CONNECTING> ( player_slot_status );
		}
	};

	class HumanOnlyConnected :
		public BaseProcessFilter
	{
		virtual bool CanProcessThisSlot ( SlotStatus const player_slot_status ) const
		{
			return TemplatedProcessFilter::CanProcessThisSlot<STATUS_STRICT, false, SlotStatus::PLAYER_CONNECTED> ( player_slot_status );
		}
	};

	class HumanOnlyConnecting :
		public BaseProcessFilter
	{
		virtual bool CanProcessThisSlot(SlotStatus const player_slot_status) const
		{
			return TemplatedProcessFilter::CanProcessThisSlot<STATUS_STRICT, false, SlotStatus::PLAYER_CONNECTING>(player_slot_status);
		}
	};

	class HumanOnlyKick :
		public BaseProcessFilter
	{
		virtual bool CanProcessThisSlot(SlotStatus const player_slot_status) const
		{
			return TemplatedProcessFilter::CanProcessThisSlot<STATUS_STRICT, false, SlotStatus::KICK>(player_slot_status);
		}
	};

	class HumanAtLeastConnectedOrBot :
		public BaseProcessFilter
	{
		virtual bool CanProcessThisSlot ( SlotStatus const player_slot_status ) const
		{
			return TemplatedProcessFilter::CanProcessThisSlot<STATUS_EQUAL_OR_BETTER, true, SlotStatus::PLAYER_CONNECTED> ( player_slot_status );
		}
	};

	/*class NoFilter :
		public BaseProcessFilter
	{
		virtual bool CanProcessThisSlot ( SlotStatus const player_slot_status ) const
		{
			return true;
		}
	};*/

	class AnyClient :
		public BaseProcessFilter
	{
		virtual bool CanProcessThisSlot ( SlotStatus const player_slot_status ) const
		{
			return TemplatedProcessFilter::CanProcessThisSlot<STATUS_BETTER, true, SlotStatus::INVALID> ( player_slot_status );
		}
	};

	class TVOnly :
		public BaseProcessFilter
	{
		virtual bool CanProcessThisSlot ( SlotStatus const player_slot_status ) const
		{
			return player_slot_status == SlotStatus::TV;
		}
	};

	class BOTOnly :
		public BaseProcessFilter
	{
		virtual bool CanProcessThisSlot ( SlotStatus const player_slot_status ) const
		{
			return player_slot_status == SlotStatus::BOT;
		}
	};

	class FakeClientOnly :
		public BaseProcessFilter
	{
		virtual bool CanProcessThisSlot ( SlotStatus const player_slot_status ) const
		{
			return player_slot_status == SlotStatus::BOT || player_slot_status == SlotStatus::TV;
		}
	};
}

inline const char * const SlotStatusToString ( SlotStatus_t status )
{
	switch( status )
	{
		case SlotStatus::INVALID:
			return "INVALID";
		case SlotStatus::KICK:
			return "KICK";
		case SlotStatus::TV:
			return "TV";
		case SlotStatus::BOT:
			return "BOT";
		case SlotStatus::PLAYER_CONNECTING:
			return "PLAYER_CONNECTING";
		case SlotStatus::PLAYER_CONNECTED:
			return "PLAYER_CONNECTED";
		case SlotStatus::PLAYER_IN_TESTS:
			return "PLAYER_IN_TESTS";
		default:
			Assert ( 0 && "Undefined SloStatus in SlotStatusToString" );
			return "ERROR";
	}
}

#endif // PROCESSFILTER_H
