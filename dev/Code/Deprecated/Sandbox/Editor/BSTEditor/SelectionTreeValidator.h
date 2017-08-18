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

#ifndef CRYINCLUDE_EDITOR_BSTEDITOR_SELECTIONTREEVALIDATOR_H
#define CRYINCLUDE_EDITOR_BSTEDITOR_SELECTIONTREEVALIDATOR_H
#pragma once


#include "SelectionTreeManager.h"

class CSelectionTreeValidator
{
public:
	CSelectionTreeValidator(CSelectionTreeManager* pManager);

	void Validate();
private:
	enum EErrorType
	{
		eET_Error,
		eET_Warning,
		eET_Comment,
	};

	void ValidateTree(const SSelectionTreeInfo& info);
	void ValidateReference(const SSelectionTreeInfo& info);

	typedef std::list<string> TVariableList;
	struct SNode
	{
		string Name;
		std::vector<SNode> Children;
	};

	void ValidateVarBlock(const SSelectionTreeInfo& info, const SSelectionTreeBlockInfo& block, TVariableList& varlist);
	void ValidateSignalBlock(const SSelectionTreeInfo& info, const SSelectionTreeBlockInfo& block, const TVariableList& varlist);
	void ValidateTreeBlock(const SSelectionTreeInfo& info, const SSelectionTreeBlockInfo& block, XmlNodeRef xmlNode, SNode& node, const TVariableList& varlist);
	void ValidateLeafMappingBlock(const SSelectionTreeInfo& info, const SSelectionTreeBlockInfo& block, const SNode& rootnode);

	void ValidateCondition(const SSelectionTreeInfo& info, const SSelectionTreeBlockInfo& block, const string& conditionStr, const TVariableList& varlist, const char* nodeName);
	bool ValidateNodePath(const SNode& node, std::vector<string> path, bool firstNodeFound = false);

	void ReportError( const SSelectionTreeInfo &info, const SSelectionTreeBlockInfo& block, const string& err, const string& desc, ESelectionTreeTypeId blockType, EErrorType errType );

private:
	CSelectionTreeManager* m_pManager;

	typedef std::list<CSelectionTreeErrorReport::TSelectionTreeErrorId> TReportedErrors;
	TReportedErrors m_ReportedErrors;
	TReportedErrors m_NewReportedErrors;
};

#endif // CRYINCLUDE_EDITOR_BSTEDITOR_SELECTIONTREEVALIDATOR_H
