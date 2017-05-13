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

#ifndef I18NMANAGER_H
#define I18NMANAGER_H

#include "Misc/temp_singleton.h"
#include "Misc/temp_basicstring.h"

namespace i18n
{
	enum class SentenceID : unsigned int
	{
		PLUGIN_LOADING = 0,
		PLUGIN_LOADED,
		PLUGIN_PAUSE,
		PLUGIN_UNPAUSE,

	};

	struct replace_pair
	{
		basic_string replace_this;
		basic_string replace_by;
	};

	struct replace_table_base
	{
		size_t const m_size;
		replace_pair m_table[ 0 ];
		replace_table_base() : m_size(0)
		{}
	};

	class i18nManager :
		public Singleton<i18nManager>
	{
	private:
		struct Pimpl* m_pimpl;

	public:
		i18nManager ();
		~i18nManager ();

		void Load ();

		void SetServerLanguage ( basic_string const & lang );

		basic_string & GetServerLanguage () const;

		void TranslateText ( basic_string const & lang, ... ) const;
	};
}

#endif // I18NMANAGER_H
