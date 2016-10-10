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

#ifndef THROWBACK_H
#define THROWBACK_H

#include <limits>
#include <type_traits>

/*
	The purpose of this tool is to keep stored a compile-time specified amount of data, like a stack.

	Every time something is wrote in the variable, the stack is popped.
	This offer us the possibility to revert the change made in a variable and also take back the value he got at a certain moment.
*/

template < typename value_type,
			typename time_type,
			size_t count = 2 
			/* Number of values to store including the current value. Can remember count-1 values back. */
		>
class Throwback_Arithmetic
{
	typedef Throwback_Arithmetic<value_type, time_type, count> this_type;

	static_assert ( count >= 2 , "count is history to keep + current value itself");

public:
	struct inner_type
	{
		value_type v;
		time_type t;
	};
	
private:
	inner_type m_stack[ count ];
	inner_type* m_current;

	inline int GetCurrentIndex ()
	{
		return m_current - m_stack;
	}

	inline inner_type* GetLastPointer ()
	{
		return m_stack + count - 1;
	}

	inline int GetCurrentIndexReverse ()
	{
		return GetLastPointer () - m_current;
	}

	template <size_t distance = 1>
	inner_type * GetForward ()
	{
		static_assert ( distance < count, "distance collides with size" );

		if( m_current + distance > GetLastPointer () )
		{
			return m_stack + ( distance - GetCurrentIndexReverse () );
		}
		else
		{
			return m_current + distance;
		}
	}

	template <size_t distance = 1>
	inner_type * GetBackward ()
	{
		static_assert ( distance < count, "distance collides with size" );

		if( m_current - distance < m_stack )
		{
			return GetLastPointer () - ( distance - GetCurrentIndex () );
		}
		else
		{
			return m_current - distance;
		}
	}

	void GoForward ()
	{
		m_current = GetForward<1> ();
	}

	void GoBackward ()
	{
		m_current = GetBackward<1> ();
	}

public:
	void Reset ()
	{
		inner_type const * const cp_current ( m_current );

		do
		{
			m_current->v = std::numeric_limits<value_type>::max ();
			m_current->t = std::numeric_limits<time_type>::max ();
			GoForward ();
		}
		while( m_current != cp_current );
	}

	Throwback_Arithmetic () :	m_current ( m_stack )
	{
		static_assert ( std::is_arithmetic< value_type>::value, "value_type is not arithmetic" );
		static_assert ( std::is_arithmetic< time_type>::value, "time_type is not arithmetic" );
		static_assert ( std::is_signed< time_type >::value, "time_type can't be negative" );

		Reset ();
	}

	Throwback_Arithmetic ( this_type const & other ) : m_current ( m_stack + ( other.m_current - other.m_stack ) )
	{
		memcpy ( m_stack, other.m_stack, sizeof ( value_type ) * count );
	}

	Throwback_Arithmetic ( value_type const & v ) : Throwback ()
	{
		m_current->v = v;
	}

	~Throwback_Arithmetic ()
	{
	}

	void Revert ()
	{
		inner_type const * const past ( GetBackward<1> () );
		GoForward ();
		*m_current = *past;
	}

	/*value_type operator value_type() const
	{
		return m_current->v;
	}*/

	/*inner_type operator inner_type() const // FIXME : inner_type without copy construct
	{
		return *m_current;
	}*/

	void Store ( value_type value, time_type time )
	{
		GoForward ();
		m_current->v = value;
		m_current->t = time;
	}

	void CopyHistory ( inner_type* dest, size_t& amount, size_t max_amount = count, time_type time_range = std::numeric_limits<time_type>::max (), time_type now = std::numeric_limits<time_type>::max () )
	{
		inner_type const * const cp_current ( m_current );
		amount = 0;

		do
		{
			if( m_current->v == std::numeric_limits<value_type>::max () && m_current->t == std::numeric_limits<time_type>::max () )
			{
				break;
			}
			if( now - m_current->t > time_range )
			{
				break;
			}
			memcpy ( dest, m_current, sizeof ( inner_type ) );
			//*dest = m_current->v;
			++dest;
			++amount;
			GoBackward ();
		}
		while( m_current != cp_current );
	}

	float Average ( value_type range_value_min = std::numeric_limits<value_type>::min(),
					value_type range_value_max = std::numeric_limits<value_type>::max ())
	{
		inner_type const * const cp_current ( m_current );
		value_type sum ( 0 );

		size_t amount ( 0 );

		do
		{
			if( m_current->v == std::numeric_limits<value_type>::max () && m_current->t == std::numeric_limits<time_type>::max () )
			{
				break;
			}
			if( m_current->v >= range_value_min && m_current->v <= range_value_max )
			{
				++amount;
				sum += m_current->v;
			}
			GoBackward ();
		}
		while( m_current != cp_current );

		return (float)sum / (float) amount;
	}

	value_type Min ()
	{
		inner_type const * const cp_current ( m_current );
		value_type min ( m_current->v );

		GoBackward ();
		while( m_current != cp_current )
		{
			if( m_current->v == std::numeric_limits<value_type>::max () && m_current->t == std::numeric_limits<time_type>::max () )
			{
				break;
			}
			if( m_current->v < min )
			{
				min = m_current->v;
			}
			GoBackward ();
		}

		return min;
	}

	value_type Max ()
	{
		inner_type const * const cp_current ( m_current );
		value_type max ( m_current->v );

		GoBackward ();
		while( m_current != cp_current )
		{
			if( m_current->v == std::numeric_limits<value_type>::max () && m_current->t == std::numeric_limits<time_type>::max () )
			{
				break;
			}
			if( m_current->v > max )
			{
				max = m_current->v;
			}
			GoBackward ();
		}

		return max;
	}

	time_type TimeSpan ()
	{
		inner_type const * const cp_current ( m_current );
		inner_type const * min ( m_current );
		time_type const max ( m_current->t );

		GoBackward ();
		while( m_current != cp_current )
		{
			if( m_current->v == std::numeric_limits<value_type>::max () && m_current->t == std::numeric_limits<time_type>::max () )
			{
				break;
			}
			min = m_current;
			GoBackward ();
		}

		return max - min->t;
	}
};

#endif // THROWBACK_H
