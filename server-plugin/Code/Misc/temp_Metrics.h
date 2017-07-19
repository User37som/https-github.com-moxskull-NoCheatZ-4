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

#ifndef METRICS_H
#define METRICS_H

#include "../Preprocessors.h"

#include <iostream>
#include <stdint.h> // uint32_t
#include "temp_basicstring.h"
#include "Misc/temp_singleton.h"

#ifdef WIN32
#	include "include_windows_headers.h"
#else
#	include <time.h>
#endif

#include "Helpers.h"

#define METRICS_NAME_SIZE 44

template <typename val_t>
class BaseMetrics : 
	public HeapMemoryManager::OverrideNew<16>
{
protected:
	val_t m_min;
	float m_avg;
	val_t m_max;

	val_t m_current_val;
	val_t m_val_sum;
	uint32_t m_val_count;

	uint32_t m_overload_count;
	val_t m_overload_threshold;
	float m_overload_percent;

	char m_name[METRICS_NAME_SIZE];

public:
	BaseMetrics()
	{
		Reset();
		memset(m_name, '\0', METRICS_NAME_SIZE);
	};

	BaseMetrics(char const * const name, val_t const overload_threshold = 0)
	{
		Reset();
		m_overload_threshold = overload_threshold;
		strncpy(m_name, name, METRICS_NAME_SIZE-1);
	};

	~BaseMetrics()
	{
	};

	char const * const GetName() const
	{
		return m_name;
	}

	/*
		The timer version will start to tick here.
		The counter version will increment by one here.
	*/
	virtual void EnterSection() = 0;

	/*
		The timer version will end to tick here.
		The counter version will reset the count here.

		Compute avg, min and max next.
	*/
	virtual void LeaveSection() = 0;

	virtual void Print() = 0;

	virtual val_t GetCurrent() = 0;

	virtual void Reset()
	{
		memset(&m_min, 0, size_t(&m_name) - size_t(&m_min) );
	}
};

class MetricsCounter : public BaseMetrics<uint32_t>
{
	typedef BaseMetrics<uint32_t> hClass;

public:
	MetricsCounter() : hClass()
	{
	};

	MetricsCounter(char const * const name, uint32_t const overload_threshold = 0) : hClass(name, overload_threshold)
	{
	}

	~MetricsCounter()
	{
	};

	void EnterSection()
	{
		++this->m_current_val;
	};

	void LeaveSection()
	{
		// Compute avg
		++this->m_val_count;
		this->m_val_sum += this->m_current_val;
		this->m_avg = (float)(this->m_val_sum) / (float)(this->m_val_count);

		// min, max, overload

		if ( this->m_current_val > this->m_max )
			this->m_max = this->m_current_val;

		if ( this->m_current_val < this->m_min )
			this->m_min = this->m_current_val;

		if ( this->m_current_val > this->m_overload_threshold )
		{
			++this->m_overload_count;
			this->m_overload_percent = ( (float)(this->m_overload_count) / (float)(this->m_val_count) ) * 100.0f; // m_val_count is always over 0 here, no divide by zero.
			if( this->m_overload_threshold > 0 )
			{
				Print();
			}
		}

		this->m_current_val = 0;
	};

	uint32_t GetCurrent()
	{
		return this->m_current_val;
	}

	void Print()
	{
		std::cout << "----------- Counter " << this->m_name << "-----------\nSamples count : " << this->m_val_count << " ( " << this->m_overload_percent << "% overload with a threshold of " << this->m_overload_threshold << ")\nMin value : " << this->m_min << "\nAverage : " << this->m_avg << "\nLast : " << this->m_current_val << "\n";
	};
};

class MetricsTimer : public BaseMetrics<float>
{
	typedef BaseMetrics<float> hClass;

private:
#ifdef WIN32
	LARGE_INTEGER StartingTime, EndingTime, ElapsedMilliseconds, Frequency;
#else
	struct timespec StartingTime, EndingTime;
	float ElapsedMilliseconds;
#endif

public:
	MetricsTimer() :
		hClass()
	{
#ifdef GNUC
		ElapsedMilliseconds = 0.0f;
#endif
	};

	MetricsTimer(char const * const name, float const overload_threshold = 0) : hClass(name, overload_threshold)
	{
#ifdef GNUC
		ElapsedMilliseconds = 0.0f;
#endif
	}

	~MetricsTimer()
	{
	};

	void EnterSection()
	{
#ifdef WIN32
		QueryPerformanceFrequency(&Frequency); 
		QueryPerformanceCounter(&StartingTime);
#else
		clock_gettime(CLOCK_MONOTONIC, &StartingTime);
#endif
	};

	float GetCurrent()
	{
#ifdef WIN32
		QueryPerformanceCounter(&EndingTime);
		ElapsedMilliseconds.QuadPart = EndingTime.QuadPart - StartingTime.QuadPart;
		ElapsedMilliseconds.QuadPart *= 1000;
		ElapsedMilliseconds.QuadPart /= Frequency.QuadPart;

		return (float)(ElapsedMilliseconds.QuadPart);
#else
		clock_gettime(CLOCK_MONOTONIC, &EndingTime);
		ElapsedMilliseconds = (float)(EndingTime.tv_sec - StartingTime.tv_sec) * 1000.0f + (float)(EndingTime.tv_nsec - StartingTime.tv_nsec) * 0.000000001f;
		return ElapsedMilliseconds;
#endif
	}

	void LeaveSection()
	{
		this->m_current_val = GetCurrent();

		// Compute avg
		++this->m_val_count;
		this->m_val_sum += this->m_current_val;
		this->m_avg = (float)(this->m_val_sum) / (float)(this->m_val_count);

		// min, max, overload

		if ( this->m_current_val > this->m_max )
			this->m_max = this->m_current_val;

		if ( this->m_current_val < this->m_min )
			this->m_min = this->m_current_val;

		if ( this->m_current_val > this->m_overload_threshold )
		{
			++this->m_overload_count;
			this->m_overload_percent = ( (float)(this->m_overload_count) / (float)(this->m_val_count) ) * 100.0f; // m_val_count is always over 0 here, no divide by zero.
			if( this->m_overload_threshold > 0.0 )
			{
				Print();
#ifdef DEBUG
				//DBG_BREAK();
#endif
			}
		}
	};

	void Print()
	{
		std::cout << "----------- Timer " << this->m_name << " -----------\nSamples count : " << this->m_val_count << " ( " << this->m_overload_percent << "% overload with a threshold of " << this->m_overload_threshold << " ms)\nTotal time spent : " << this->m_val_sum << " ms\nMin time : " << this->m_min << " ms\nMax time : " << this->m_max << " ms\nAverage : " << this->m_avg << " ms\nLast : " << this->m_current_val << " ms\n";
	};
};

#ifdef NCZ_USE_METRICS

typedef std::list<MetricsCounter> counter_list_t;
typedef std::list<MetricsTimer> metrics_timer_list_t;

template <typename nothing = int>
class MetricsContainer
{
private:
	counter_list_t m_counters;
	metrics_timer_list_t m_timers;

public:
	MetricsContainer(){}
	~MetricsContainer(){}

	void AddCounter(char const * const name, uint32_t const overload_threshold = 0)
	{
		m_counters.push_back(MetricsCounter(name, overload_threshold));
	}

	void AddTimer(char const * const name, float const overload_threshold = 0)
	{
		m_timers.push_back(MetricsTimer(name, overload_threshold));
	}

	void EnterSection(char const * const name)
	{
		metrics_timer_list_t::iterator it_timer = m_timers.begin();
		while(it_timer != m_timers.end())
		{
			if(strcmp(name, it_timer->GetName()) == 0)
			{
				it_timer->EnterSection();
				return;
			}
			++it_timer;
		}

		counter_list_t::iterator it_counter = m_counters.begin();
		while(it_counter != m_counters.end())
		{
			if(strcmp(name, it_counter->GetName()) == 0)
			{
				it_counter->EnterSection();
				return;
			}
			++it_counter;
		}
	}

	void LeaveSection(char const * const name)
	{
		metrics_timer_list_t::iterator it_timer = m_timers.begin();
		while(it_timer != m_timers.end())
		{
			if(strcmp(name, it_timer->GetName()) == 0)
			{
				it_timer->LeaveSection();
				return;
			}
			++it_timer;
		}

		counter_list_t::iterator it_counter = m_counters.begin();
		while(it_counter != m_counters.end())
		{
			if(strcmp(name, it_counter->GetName()) == 0)
			{
				it_counter->LeaveSection();
				return;
			}
			++it_counter;
		}
	}

	void Reset(char const * const name)
	{
		metrics_timer_list_t::iterator it_timer = m_timers.begin();
		while(it_timer != m_timers.end())
		{
			if(stricmp(name, it_timer->GetName()) == 0)
			{
				it_timer->Reset();
				return;
			}
			++it_timer;
		}

		counter_list_t::iterator it_counter = m_counters.begin();
		while(it_counter != m_counters.end())
		{
			if(stricmp(name, it_counter->GetName()) == 0)
			{
				it_counter->Reset();
				return;
			}
			++it_counter;
		}
	}

	void ResetAll()
	{
		metrics_timer_list_t::iterator it_timer = m_timers.begin();
		while(it_timer != m_timers.end())
		{
			it_timer->Reset();
			++it_timer;
		}

		counter_list_t::iterator it_counter = m_counters.begin();
		while(it_counter != m_counters.end())
		{
			it_counter->Reset();
			++it_counter;
		}
	}

	void Print(char const * const name)
	{
		metrics_timer_list_t::iterator it_timer = m_timers.begin();
		while(it_timer != m_timers.end())
		{
			if(stricmp(name, it_timer->GetName()) == 0)
			{
				it_timer->Print();
				return;
			}
			++it_timer;
		}

		counter_list_t::iterator it_counter = m_counters.begin();
		while(it_counter != m_counters.end())
		{
			if(stricmp(name, it_counter->GetName()) == 0)
			{
				it_counter->Print();
				return;
			}
			++it_counter;
		}
	}

	void PrintAll()
	{
		metrics_timer_list_t::iterator it_timer = m_timers.begin();
		while(it_timer != m_timers.end())
		{
			it_timer->Print();
			++it_timer;
		}

		counter_list_t::iterator it_counter = m_counters.begin();
		while(it_counter != m_counters.end())
		{
			it_counter->Print();
			++it_counter;
		}
	}
};

#define METRICS_ENTER_SECTION(x) m_metrics.EnterSection(x)
#define METRICS_LEAVE_SECTION(x) m_metrics.LeaveSection(x)
#define METRICS_ADD_TIMER(x, y) m_metrics.AddTimer(x, y)
#define METRICS_ADD_COUNTER(x, y) m_metrics.AddCounter(x, y)

class RAIIMetrics
{
private:
	char const * const m_name;
	MetricsContainer * const m_metrics;

public:
	RAIIMetrics ( char const * const name, MetricsContainer * const metrics ) : m_name ( name ), m_metrics ( metrics )
	{
		m_metrics->EnterSection ( m_name );
	}

	~RAIIMetrics ()
	{
		m_metrics->LeaveSection ( m_name );
	}
};

#else // ! NCZ_USE_METRICS

#define METRICS_ENTER_SECTION(x)
#define METRICS_LEAVE_SECTION(x)
#define METRICS_ADD_TIMER(x, y)
#define METRICS_ADD_COUNTER(x, y)

#endif // NCZ_USE_METRICS

typedef Singleton<MetricsTimer> GlobalTimer;

extern float Plat_FloatTime();

#endif // METRICS_H


