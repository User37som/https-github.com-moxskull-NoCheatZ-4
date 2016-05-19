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

#include "Misc/temp_basicstring.h"
#include <vector>
#include <limits> // numeric_limits

#include "SdkPreprocessors.h"
#include "Interfaces/edict.h"
#include "Interfaces/InterfacesProxy.h"

#include "Preprocessors.h"

namespace Helpers
{
	/* Convertis une chaine de caractères en caractères minuscules */
	void toLowerCase(basic_string &p_str);

	/* Envoie un message */
	void xprintf(const char *p_fmt, ...);

	/* Vérifie que 2 chaines de caractères sont strictement identiques
	   Possibilité de se servir de sz1 comme un buffer à l'aide de start_offset et length */
	bool bStrEq(const char *sz1, const char *sz2, size_t start_offset = 0, size_t length = std::numeric_limits<size_t>::max());

	bool bstrneq(char const * s1, char const * s2, size_t len);

	/* Même utilité que bStrEq, sz1 est un buffer obligatoirement */
	bool bBytesEq(const char *sz1, const char *sz2, size_t start_offset, size_t length);

	/* Même utilité que bStrEq, n'est pas sensible à la casse */
	bool bStriEq(const char *sz1, const char *sz2, size_t start_offset = 0, size_t length = std::numeric_limits<size_t>::max());

	/* Ecrit dans le fichier de log. Doit être remplacé par une classe du même style que BanRequest */
	void writeToLogfile(const basic_string &p_text);

	/* Retourne la date selon le format */
	basic_string getStrDateTime(const char *p_format);


	SourceSdk::edict_t * getEdictFromSteamID(const char *p_SteamID);
	int getIndexFromSteamID(const char *SteamID);
	SourceSdk::edict_t * PEntityOfEntIndex(const int p_iEntIndex);
	int getIndexFromUserID(const int p_userid);

	bool isValidEdict(const SourceSdk::edict_t * const p_entity);

	int IndexOfEdict(const SourceSdk::edict_t *p_pEdict);
	

	int GetPlayerCount();

	int GetMaxClients();

	int GetTickCount();

	/* Permet d'avoir un format style C dans un conteneur C++ */
	basic_string format(const char *fmt, ...);
	
	/* Retourne vrai si la valeur est impaire ... */
	bool isOdd(const int value);

	/* Conversion des chars vers basic_string */
	template<typename T>
	basic_string tostring(const T & p_toConvert);

	/* Retourne vrai si value est un entier ... (140.000, 1587.000 etc) */
	bool IsInt(float value);

	/* Envoie un message chat à tous les clients sauf pEntity */
	void noTell(const SourceSdk::edict_t *pEntity, const basic_string& msg);

	void chatprintf(const basic_string& msg);

	/* Envoie un message chat à pEntity */
	void tell(SourceSdk::edict_t *pEntity, const basic_string& message);

	size_t GetUTF8Bytes(const char* const c);

	bool IsValidUTF8Char(const char* const c, const size_t bytes);

	bool IsCharSpace(const char* const c);

	void FadeUser(SourceSdk::edict_t* const pEntity, const short time);

	const char* boolToString(bool v);

	/* C'est la base ... */
	extern SourceSdk::edict_t_csgo* m_EdictList_csgo;
	extern SourceSdk::edict_t* m_EdictList;

	/* N'est pas utilisé */
	extern int m_edictCount;

	/* Ne doit pas être utilisé */
	extern int m_clientMax;
};

#endif
