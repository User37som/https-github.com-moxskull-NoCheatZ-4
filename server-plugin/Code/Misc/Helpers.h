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

#ifndef HELPERS
#define HELPERS


#include <limits> // numeric_limits
#include <ctime> // time_t

#include "Misc/temp_basicstring.h"
#include "Interfaces/InterfacesProxy.h"

namespace SourceSdk
{
	struct edict_t;
}

namespace Helpers
{
	/*
		Standardized reprentation of time in the plugin. Might not be the right place to put it.
	*/
	struct game_tm
	{
		time_t m_systime;
		float m_tickinterval;
		float m_floattime;
		size_t m_tickrate;
		size_t m_tv_tick;

		game_tm ()
		{}

		game_tm ( game_tm const & other )
		{
			memcpy ( this, &other, sizeof ( game_tm ) );
		}

		game_tm ( game_tm && other )
		{
			memcpy ( this, &other, sizeof ( game_tm ) );
			memset ( &other, 0, sizeof ( game_tm ) );
		}

		game_tm& operator=( game_tm const & other )
		{
			if( this != &other )
			{
				memcpy ( this, &other, sizeof ( game_tm ) );
			}

			return *this;
		}

		game_tm& operator=( game_tm && other )
		{
			if( this != &other )
			{
				memcpy ( this, &other, sizeof ( game_tm ) );
				memset ( &other, 0, sizeof ( game_tm ) );
			}

			return *this;
		}

		game_tm operator-( game_tm const & other ) const
		{
			game_tm z ( *this );

			if( other.m_systime > z.m_systime )
			{
				z.m_systime -= other.m_systime;
				if( other.m_floattime > z.m_floattime )
				{
					z.m_floattime -= other.m_floattime;
				}
				else
				{
					z.m_floattime = 0;
				}
				if( other.m_tv_tick > z.m_tv_tick )
				{
					z.m_tv_tick -= other.m_tv_tick;
				}
				else
				{
					z.m_tv_tick = 0;
				}

				return z;
			}
			else
			{
				return other.operator-( z );
			}
		}

		game_tm& operator-=( game_tm const & other )
		{
			if( other.m_systime > m_systime )
			{
				m_systime -= other.m_systime;
				if( other.m_floattime > m_floattime )
				{
					m_floattime -= other.m_floattime;
				}
				else
				{
					m_floattime = 0;
				}
				if( other.m_tv_tick > m_tv_tick )
				{
					m_tv_tick -= other.m_tv_tick;
				}
				else
				{
					m_tv_tick = 0;
				}

				return *this;
			}
			else
			{
				return this->operator=( other.operator-( *this ) );
			}
		}

		void Populate ();

		void WriteXMLOutput ( FILE* ) const
		{

		}

		void WriteLogOutput ( FILE* ) const
		{

		}
	};

	/* Ecrit dans le fichier de log. Doit être remplacé par une classe du même style que BanRequest */
	void writeToLogfile ( const basic_string &p_text );

	/* Retourne la date selon le format */
	const char * getStrDateTime ( const char *p_format, time_t rawtime = time( nullptr ) );

	SourceSdk::edict_t * PEntityOfEntIndex ( const int p_iEntIndex );

	bool isValidEdict ( const SourceSdk::edict_t * const p_entity );

	int IndexOfEdict ( const SourceSdk::edict_t * const p_pEdict );

	SourceSdk::edict_t * edictOfUnknown ( void * unk );

	unsigned int HashString ( char const * );

	uint32_t CRC32 ( void* data, size_t size );

	int GetMaxClients ();

	int GetTickCount ();

	/* Permet d'avoir un format style C dans un conteneur C++ */
	const char * format ( const char *fmt, ... );

	int GetGameTickCount ();

	/* Retourne vrai si la valeur est impaire ... */
	bool isOdd ( const int value );

	/* Retourne vrai si value est un entier ... (140.000, 1587.000 etc) */
	bool IsInt ( float const value );

	/* Envoie un message chat à tous les clients sauf pEntity */
	void noTell ( const SourceSdk::edict_t * const pEntity, const basic_string& msg );

	void chatprintf ( const basic_string& msg );

	/* Envoie un message chat à pEntity */
	void tell ( SourceSdk::edict_t const * const pEntity, const basic_string& message );

	size_t GetUTF8Bytes ( const char* const c );

	bool IsValidUTF8Char ( const char* const c, const size_t bytes );

	bool IsCharSpace ( const char* const c );

	void FadeUser ( SourceSdk::edict_t const * const pEntity, const short time );

	const char* boolToString ( bool const v );

	class CRC32_Digestive
	{
	private:
		uint32_t m_res;

	public:
		CRC32_Digestive () :
			m_res ( std::numeric_limits<uint32_t>::max () )
		{
		}
		~CRC32_Digestive ()
		{
		}

		void Prepare ()
		{
			m_res = std::numeric_limits<uint32_t>::max ();
		}

		void Digest ( void const * data, size_t size );

		uint32_t Final ()
		{
			return m_res ^= std::numeric_limits<uint32_t>::max ();
		}

		uint32_t GetRes () const
		{
			return m_res;
		}
	};

	/*
		This is used to make sure a class has a specialized way to make a hash,
		otherwise we risk hashing pointers inside structs and this is not what we want.
	*/
	class CRC32_Specialize
	{
	public:
		CRC32_Specialize ()
		{}
		virtual ~CRC32_Specialize ()
		{}

		virtual uint32_t Hash_CRC32 () = 0;
	};

	/* C'est la base ... */
	extern SourceSdk::edict_t* m_EdictList;

	/* N'est pas utilisé */
	extern int m_edictCount;

	/* Ne doit pas être utilisé */
	extern int m_clientMax;
};

#endif
