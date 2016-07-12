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

#ifndef _MRECIPIENT_FILTER_H
#define _MRECIPIENT_FILTER_H

#include "Interfaces/irecipientfilter.h"
#include "Containers/utlvector.h"

class MRecipientFilter : public SourceSdk::IRecipientFilter
{
public:
	MRecipientFilter ( void );
	virtual ~MRecipientFilter ( void ) final;

	virtual bool IsReliable ( void ) const final;
	virtual bool IsInitMessage ( void ) const final;

	virtual int GetRecipientCount ( void ) const final;
	virtual int GetRecipientIndex ( int slot ) const final;

	MRecipientFilter ( MRecipientFilter const & other );
	MRecipientFilter& operator=( MRecipientFilter const & other );

	void SetReliable ( bool reliable );
	void SetInitMessage ( bool init );

	void AddAllPlayers ( int maxClients );
	void AddTeam ( int teamid );
	void AddAllPlayersExcludeTeam ( int teamid );
	void AddRecipient ( int iPlayer );
	void RemoveRecipient ( int iPlayer );
	void RemoveAll ();

private:
	bool m_bReliable;
	bool m_bInitMessage;
	CUtlVector< int > m_Recipients;
};

#endif
