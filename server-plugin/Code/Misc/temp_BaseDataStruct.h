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

#ifndef BASEDATASTRUCT
#define BASEDATASTRUCT

template <typename DataT, int MAX_ELEM>
class BaseDataStructHandler
{
public:
	BaseDataStructHandler ()
	{};
	virtual ~BaseDataStructHandler ()
	{};

	void ResetAll ( const DataT* src )
	{
		if( src )
		{
			for( size_t index ( 0 ); index < MAX_ELEM; ++index )
				m_dataStruct[ index ] = *src;
		}
		else
		{
			for( int x = 0; x < MAX_ELEM; ++x )
			{
				m_dataStruct[ x ] = DataT ();
			}
		}
	};

protected:
	inline DataT& RT_GetDataStruct ( const int elem ) const
	{
		return m_dataStruct[ elem ];
	};

	inline void InitDataStruct ()
	{
		for( int x ( 0 ); x < MAX_ELEM; ++x )
			m_dataStruct[ x ] = DataT ();
	};

	inline void ResetDataStruct ( const int elem )
	{
		m_dataStruct[ elem ] = DataT ();
	};

	DataT m_dataStruct[ MAX_ELEM ];
};

#endif
