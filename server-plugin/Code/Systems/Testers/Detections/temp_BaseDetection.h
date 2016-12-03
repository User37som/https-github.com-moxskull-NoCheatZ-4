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

#ifndef BASEDETECTION
#define BASEDETECTION

#include "Misc/ForwardedCommonDefinitions.h"
#include "Misc/HeapMemoryManager.h" // OverrideNew
#include "Misc/temp_basicstring.h" // basic_string
#include "Players/PlayerHandler_impl.h"

/*
	Detections must store relevant data about what behavior was detected as wrong against a particular player.
	
	Once a player triggers a detection within a tester, the data is copied onto the detection class and remains in memory until the player disconnects.
	This means each players have it's own list of detections. Having multiple detections is important to see if the detection was false or not
	and also because this class could be used later on for machine learning situations.
	
	In order to prevent memory overflow issues, we want to implement a way to compress the detection list.
		-> Limiting the maximum number of same detections type and removing detections that got the same content, possibly have a sub list to repeat it.
			-> Providing a static unique id to each detection type (basically we have one detection type per tester) and also providing a way to hash content of detections.

	Format of the dictionnary of detections a player store is :
		-> Unique Detection Type ID
			-> Time
			-> Pointer to BaseDetection ( could be the same as an earlier detection )

	Each detection type must also implement procedures to be printed to chat, console and logs, be timed with the current TV record, if any, otherwise current system time.

	The dictionnary must provide a procedure to write down a simple XML file. A single detection can also provide a way to write additionnal data.

	When the plugin decides to write down the data ( When the player disconnects with detections ):
		The dictionnary is translated to a list sorted by Time rather than UDID.

		The XML file is opened as SteamID-SystemTime.xml.
		It walks all the detections of the list, call BaseDetection->WriteXMLOutput(File) and writes the result in the XML file,
		then call BaseDetection->PerformCustomOutput() if the detection has any custom file to write.
		After that, the XML file is closed, notified in logs and finally the dictionnary is freed.
*/

class BaseDetection :
	public HeapMemoryManager::OverrideNew<16>
{
protected:
	PlayerHandler::const_iterator m_player;
	BaseSystem * const m_tester;
	uint32_t const m_udid;

public:
	BaseDetection ( PlayerHandler::const_iterator player, BaseSystem* tester, uint32_t udid );

	virtual ~BaseDetection ();

	/*
		Returns the player that triggered the detection.
	*/
	virtual PlayerHandler::const_iterator GetPlayer () const;

	/*
		Returns a pointer to the tester responsible of the detection.
	*/
	virtual BaseSystem * const GetTester () const;

	/*
		Returns the unique detection type id, not the same as content hash.
	*/
	virtual uint32_t const GetUDID () const;

	virtual void PerformCustomOutput () const;

	/*
		Returns the hash of the current data.
	*/
	virtual uint32_t GetDataHash () const = 0;

	/*
		If true, the detection will be cloned in dictionnary if we already have another detection with the same hash.
		Otherwise it will not be accounted as an additionnal detection and will be ignored.
	*/
	virtual bool CloneWhenEqual () const = 0;

	/*
		Write the data struct of detection as XML
	*/
	virtual void WriteXMLOutput (FILE * const ) const = 0;

	/*
		A short indication of the detection type.
	*/
	virtual basic_string GetDetectionLogMessage () const = 0;

	/*
		A detection might make a delayed ban, or instant ban or kick ... or nothing.
	*/
	virtual void TakeAction ();
};

template <typename playerDataStructT>
class LogDetection :
	public BaseDetection
{
protected:
	playerDataStructT const m_dataStruct;

private:
	uint32_t const m_datahash;

public:
	typedef playerDataStructT data_t;

	LogDetection ( PlayerHandler::const_iterator player, BaseDynamicSystem* tester, uint32_t udid, playerDataStructT const * data ) :
		BaseDetection ( player, tester, udid ),
		m_dataStruct ( *data ),
		m_datahash ( m_dataStruct.Hash_CRC32 () )
	{
		static_assert ( std::is_copy_constructible<playerDataStructT>::value, "playerDataStructT must be copy constructible" );
		static_assert ( !std::is_trivially_copy_constructible<playerDataStructT>::value, "You must implement copy construction to playerDataStructT" );
		static_assert( std::is_base_of<playerDataStructT, Helpers::CRC32_Specialize>::value, "playerDataStructT must be derived from Helpers::CRC32_Specialize and Hash_CRC32 must be implemented for each playerDataStructT" )

		Log ();

		Helpers::game_tm tm;
		tm.Populate ();

		this->m_player->GetDetectionInfos ()->AddDetection ( this, tm );

		this->TakeAction ();
	}

	virtual ~LogDetection ()
	{

	}

	virtual uint32_t GetDataHash () const
	{
		return m_datahash;
	}

	void Log ()
	{
		basic_string msg ( Helpers::format ( "%s triggered a detection : %s is using a %s.", this->m_tester->GetName (), this->m_player->GetName (), this->GetDetectionLogMessage ().c_str () ) );
		Helpers::writeToLogfile ( msg );
		char const * text2 ( Helpers::format ( "[" NCZ_PLUGIN_NAME "] %s\0", msg.c_str () ) );
		SourceSdk::InterfacesProxy::Call_LogPrint ( text2 );

		switch( Logger::GetInstance ()->GetDCFilter () )
		{
			case ALL:
				Helpers::chatprintf ( text2 );
				break;

			case DEFAULT:
				Helpers::noTell ( this->m_player->GetEdict (), text2 );
				break;

			case TVONLY:
				AutoTVRecord::GetInstance ()->SendTVChatMessage ( text2 );

			default:
				break;
		}
	}

	playerDataStructT const * GetDataStruct () const
	{
		return ( playerDataStructT const * ) ( &( this->m_dataStruct ) );
	}
};

#define TriggerDetectionExt( detection_classname, player_iterator, data_ptr, sysptr ) new detection_classname ( player_iterator, sysptr, data_ptr )
#define TriggerDetection( detection_classname, player_iterator, data_ptr ) TriggerDetectionExt ( detection_classname, player_iterator, data_ptr, this  )

#endif
