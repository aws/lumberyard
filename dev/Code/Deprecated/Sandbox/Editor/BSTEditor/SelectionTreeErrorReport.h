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

#ifndef CRYINCLUDE_EDITOR_BSTEDITOR_SELECTIONTREEERRORREPORT_H
#define CRYINCLUDE_EDITOR_BSTEDITOR_SELECTIONTREEERRORREPORT_H
#pragma once


class CSelectionTreeErrorRecord
{
public:
	enum ESeverity
	{
		ESEVERITY_UNDEFINED,
		ESEVERITY_ERROR,
		ESEVERITY_WARNING,
		ESEVERITY_COMMENT,
	};

	enum ETreeModule
	{
		EMODULE_UNDEFINED,
		EMODULE_TREE,
		EMODULE_VARIABLES,
		EMODULE_SIGNALS,
		EMODULE_LEAFTRANSLATIONS,
	};

	enum EType
	{
		ETYPE_TREE,
		ETYPE_REFERENCE
	};


	ESeverity severity;
	ETreeModule module;
	EType type;
	CString group;
	CString error;
	CString file;
	CString description;

	CSelectionTreeErrorRecord(ESeverity _severity, ETreeModule _module, EType _type, const char* _group,
		const char* _error, const char* _description, const char* _file)
		: severity(_severity)
		, module(_module)
		, type( _type )
		, group(_group)
		, error(_error)
		, description(_description)
		, file(_file)
	{
	}

	CSelectionTreeErrorRecord()
	{
		severity = ESEVERITY_UNDEFINED;
		module = EMODULE_UNDEFINED;
		group = "UNDEFINED";
	}
};

class CSelectionTreeErrorReport
{
public:
	typedef uint32 TSelectionTreeErrorId;
	static const TSelectionTreeErrorId InvalidErrorId = (TSelectionTreeErrorId) -1;

	CSelectionTreeErrorReport();

	TSelectionTreeErrorId AddError( const CSelectionTreeErrorRecord &err );
	void RemoveError(TSelectionTreeErrorId id);

	bool NeedReload() const {return m_bNeedReload; }
	void SetReloaded() { m_bNeedReload = false; }

	int GetErrorCount() const { return m_errors.size(); };
	const CSelectionTreeErrorRecord& GetError( int i ) const;

	bool IsEmpty() const { return GetErrorCount() == 0; }
	void Clear();

private:
	bool IsNotInList( const CSelectionTreeErrorRecord &err, TSelectionTreeErrorId& id ) const;
	TSelectionTreeErrorId GetNextFreeId() const;

private:
	typedef std::map<TSelectionTreeErrorId, std::pair<CSelectionTreeErrorRecord, int>> TErrors;
	TErrors m_errors;
	bool m_bNeedReload;
};

#endif // CRYINCLUDE_EDITOR_BSTEDITOR_SELECTIONTREEERRORREPORT_H
