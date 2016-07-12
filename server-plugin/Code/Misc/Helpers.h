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

#include "Misc/temp_basicstring.h"

namespace SourceSdk
{
	struct edict_t;
}

namespace Helpers
{
	/* Ecrit dans le fichier de log. Doit être remplacé par une classe du même style que BanRequest */
	void writeToLogfile ( const basic_string &p_text );

	/* Retourne la date selon le format */
	const char * getStrDateTime ( const char *p_format );

	SourceSdk::edict_t * PEntityOfEntIndex ( const int p_iEntIndex );

	bool isValidEdict ( const SourceSdk::edict_t * const p_entity );

	int IndexOfEdict ( const SourceSdk::edict_t * const p_pEdict );

	SourceSdk::edict_t * edictOfUnknown ( void * unk );

	int GetMaxClients ();

	int GetTickCount ();

	/* Permet d'avoir un format style C dans un conteneur C++ */
	const char * format ( const char *fmt, ... );

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

	/* C'est la base ... */
	extern SourceSdk::edict_t* m_EdictList;

	/* N'est pas utilisé */
	extern int m_edictCount;

	/* Ne doit pas être utilisé */
	extern int m_clientMax;
};

#endif
