/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
// Original file Copyright Crytek GMBH or its affiliates, used under license.

#include "StdAfx.h"
#include "SelectionTreeErrorReport.h"

//////////////////////////////////////////////////////////////////////////

CSelectionTreeErrorReport::CSelectionTreeErrorReport()
{
}

//////////////////////////////////////////////////////////////////////////
const CSelectionTreeErrorRecord& CSelectionTreeErrorReport::GetError( int i ) const
{
	TErrors::const_iterator it = m_errors.begin();
	for (; it != m_errors.end() && i > 0; ++it, --i);
	assert(it != m_errors.end());
	return it->second.first;
};

//////////////////////////////////////////////////////////////////////////
CSelectionTreeErrorReport::TSelectionTreeErrorId CSelectionTreeErrorReport::AddError( const CSelectionTreeErrorRecord &err )
{
	TSelectionTreeErrorId id;
	if (IsNotInList(err, id))
	{
		id = GetNextFreeId();
		m_errors[id] = std::make_pair(err, 1);
		m_bNeedReload = true;
	}
	else
	{
		m_errors[id].second++;
	}
	return id;
}

//////////////////////////////////////////////////////////////////////////
void CSelectionTreeErrorReport::RemoveError(TSelectionTreeErrorId id)
{
	TErrors::iterator it = m_errors.find(id);
	if (it != m_errors.end())
	{
		int& count = it->second.second;
		count--;
		if (count == 0)
		{
			m_errors.erase(it);
			m_bNeedReload = true;
		}
		return;
	}
	assert( false );
}

//////////////////////////////////////////////////////////////////////////
void CSelectionTreeErrorReport::Clear()
{
	m_errors.clear();
	m_bNeedReload = true;
}

//////////////////////////////////////////////////////////////////////////
bool CSelectionTreeErrorReport::IsNotInList( const CSelectionTreeErrorRecord &err, TSelectionTreeErrorId& id ) const
{
	for (TErrors::const_iterator it = m_errors.begin(); it != m_errors.end(); ++it)
	{
		const CSelectionTreeErrorRecord& existingErr = it->second.first;
		if ( existingErr.module == err.module
			&& existingErr.type == err.type
			&& existingErr.group == err.group
			&& existingErr.error == err.error )
		{
			id = it->first;
			return false;
		}
	}
	return true;
}

CSelectionTreeErrorReport::TSelectionTreeErrorId CSelectionTreeErrorReport::GetNextFreeId() const
{
	TSelectionTreeErrorId res = 0;
	for (TErrors::const_iterator it = m_errors.begin(); it != m_errors.end(); ++it)
	{
		if (it->first != res)
			break;
		res++;
	}
	return res;
}

