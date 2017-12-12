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

#ifndef CRYINCLUDE_EDITOR_BSTEDITOR_GRAPH_SELECTIONTREELEAFMAPPING_H
#define CRYINCLUDE_EDITOR_BSTEDITOR_GRAPH_SELECTIONTREELEAFMAPPING_H
#pragma once


#include "Nodes/SelectionTree_BaseNode.h"
#include "Util/IXmlHistoryManager.h"

class CSelectionTree_TreeNode;
class CSelectionTreeGraph;

struct SLeafTranslation
{
	bool operator==(const SLeafTranslation& other) const { return !(*this != other); }
	bool operator!=(const SLeafTranslation& other) const { return Target != other.Target || NodeStack != other.NodeStack;	}
	bool LoadFromXml( const XmlNodeRef& xmlNode );
	bool SaveToXml( const XmlNodeRef& xmlNode );

	bool CheckNode(CSelectionTree_TreeNode* pNode, string& translatedBehavior, CSelectionTree_TreeNode** pEndNode = NULL) const;

	string Target;
	typedef std::vector<string> TNodeNameStack;
	TNodeNameStack NodeStack;

private:
	bool SLeafTranslation::CheckNodeInternal(CSelectionTree_TreeNode* pNode, int index, CSelectionTree_TreeNode** pEndNode) const;
};

class CLeafMappingEditor;

class CLeafMappingManager
	: public IXmlUndoEventHandler
{
public:
	CLeafMappingManager(CSelectionTreeGraph* pGraph);
	~CLeafMappingManager();

	void ClearAll();
	int CreateTranslation();
	void DeleteLeafTranslation(int idx);

	// IXmlUndoEventHandler
	virtual bool SaveToXml( XmlNodeRef& xmlNode );
	virtual bool LoadFromXml( const XmlNodeRef& xmlNode );
	virtual bool ReloadFromXml( const XmlNodeRef& xmlNode );

	bool CheckNode(CSelectionTree_TreeNode* pNode, string& translatedBehavior, int& translationId ) const;
	SLeafTranslation& GetTranslation(int idx);

	CLeafMappingEditor* GetEditor() const { return m_pEditor; }
private:
	CSelectionTreeGraph* m_pGraph;
	CLeafMappingEditor* m_pEditor;

	typedef std::vector<SLeafTranslation> TMappings;
	TMappings m_Mappings;
};


class CLeafMappingEditor
{
public:
	CLeafMappingEditor(CSelectionTreeGraph* pGraph);
	~CLeafMappingEditor();

	void StartEdit(CSelectionTree_TreeNode* pNode);
	void StopEdit(bool abord = false);
	void ChangeEndNode(CSelectionTree_BaseNode* pNode);

	bool IsInEditMode() const;

private:
	void CommitChanges(CSelectionTree_TreeNode* pEndNode);
	void MarkTranslation();
	void ResetMarkings();
	void LockTree();
	void UnlockTree();

private:
	CSelectionTreeGraph* m_pGraph;

	CSelectionTree_TreeNode* m_pMappingEditNode;
	CSelectionTree_TreeNode* m_pMappingEndEditNode;

	SLeafTranslation m_LeafMappingBackup;
	int m_CurrentLeafMappingIdx;
	bool m_bIsCurrentLeafMappingNew;
};


#endif // CRYINCLUDE_EDITOR_BSTEDITOR_GRAPH_SELECTIONTREELEAFMAPPING_H
