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

#include <fstream>
#include <iostream>

#include "Logger.h"

#include "Misc/include_windows_headers.h"

#include "Misc/Helpers.h"
#include "Misc/temp_Metrics.h"
#include "Players/NczPlayerManager.h"

void Logger::Push(const basic_string& msg)
{
	m_msg.AddToTail(msg);

	if(NczPlayerManager::GetInstance()->GetPlayerCount(PLAYER_CONNECTED, STATUS_EQUAL_OR_BETTER) == 0)
	{
		// We can flush right now.

		Flush();
	}
}

template <>
void Logger::Msg<MSG_CONSOLE>(const basic_string& msg, int verbose /*= 0*/)
{ 
	std::cout << prolog.c_str() << msg.c_str() << "\n";
#ifdef WIN32
	OutputDebugStringA(prolog.c_str());
	OutputDebugStringA(msg.c_str());
	OutputDebugStringA("\n");
#endif
}

template <>
void Logger::Msg<MSG_CHAT>(const basic_string& msg, int verbose /*= 0*/)
{
	Msg<MSG_CONSOLE>(msg);
	basic_string m(prolog);
	Helpers::chatprintf(m.append(msg).c_str());
}

template <>
void Logger::Msg<MSG_LOG>(const basic_string& msg, int verbose /*= 0*/)
{
	Msg<MSG_CONSOLE>(msg);
	Push(msg);
}

template <>
void Logger::Msg<MSG_LOG_CHAT>(const basic_string& msg, int verbose /*= 0*/)
{
	Msg<MSG_LOG>(msg);
	basic_string m(prolog);
	Helpers::chatprintf(m.append(msg).c_str());
}

template <>
void Logger::Msg<MSG_WARNING>(const basic_string& msg, int verbose /*= 0*/)
{
	basic_string m(prolog);
	m.append("WARNING : ").append(msg).append('\n');
	std::cout << m.c_str();
#ifdef WIN32
	OutputDebugStringA(m.c_str());
#endif
	Push(m);
}

template <>
void Logger::Msg<MSG_ERROR>(const basic_string& msg, int verbose /*= 0*/)
{
	basic_string m(prolog);
	m.append("ERROR : ").append('\n');
	std::cout << m.c_str();
#ifdef WIN32
	OutputDebugStringA(m.c_str());
#endif
	Push(m);
}

template <>
void Logger::Msg<MSG_HINT>(const basic_string& msg, int verbose /*= 0*/)
{
	std::cerr << prolog.c_str() << Plat_FloatTime() << " : " << msg.c_str() << "\n";
}

template <>
void Logger::Msg<MSG_VERBOSE1>(const basic_string& msg, int verbose /*= 0*/)
{
	if (verbose == 1)
	{
		Msg<MSG_CONSOLE>(basic_string("VERBOSE1 : ").append(msg));
	}
}

template <>
void Logger::Msg<MSG_VERBOSE2>(const basic_string& msg, int verbose /*= 0*/)
{
	if (verbose == 2)
	{
		Msg<MSG_CONSOLE>(basic_string("VERBOSE2 : ").append(msg));
	}
}

template <>
void Logger::Msg<MSG_DEBUG>(const basic_string& msg, int verbose /*= 0*/)
{
	if (verbose > 2)
	{
		Msg<MSG_CONSOLE>(basic_string("DEBUG : ").append(msg));
	}
}

void Logger::Flush()
{
	if(m_msg.IsEmpty()) return;

	basic_string path;
	SourceSdk::GetGameDir(path);

	path.append(Helpers::getStrDateTime("/logs/NoCheatZ_4_Logs/NoCheatZ-%d-%b-%Y.log"));
	std::ofstream fichier(path.c_str(), std::ios::out | std::ios::app);
	if(fichier)
	{
		size_t pos = 0;
		size_t const max = m_msg.Size();
		do
		{
			fichier << m_msg[pos].c_str() << std::endl;
		}
		while(++pos != max);
	}
	else 
	{
		basic_string m1(prolog);
		m1.append(Helpers::format("Can't write to logfile at %s ... Please check write access and if the directory exists.\n", path.c_str()));
		Msg<MSG_CONSOLE>(m1);
		SourceSdk::InterfacesProxy::Call_LogPrint(m1.c_str());
		m1 = prolog;
		size_t pos = 0;
		size_t const max = m_msg.Size();
		do
		{
			SourceSdk::InterfacesProxy::Call_LogPrint(m1.append(m_msg[pos]).c_str());
		} while (++pos != max);
	}

	
	m_msg.RemoveAll();
	m_msg.EnsureCapacity(256);
}

void Helpers::writeToLogfile(const basic_string &text)
{
	basic_string msg(Helpers::format("At %f (Server Tick #%d) : %s", Plat_FloatTime(), Helpers::GetTickCount(), text.c_str()));
	Logger::GetInstance()->Push(msg);
}
