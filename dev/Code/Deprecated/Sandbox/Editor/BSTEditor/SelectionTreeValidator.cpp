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
#include "SelectionTreeValidator.h"
#include "BSTEditor/Graph/Nodes/SelectionTree_TreeNode.h"

#include <ArgumentParser.h> // for easy string splitting

CSelectionTreeValidator::CSelectionTreeValidator(CSelectionTreeManager* pManager)
: m_pManager(pManager)
{
}

void CSelectionTreeValidator::Validate()
{
	m_NewReportedErrors.clear();

	TSelectionTreeInfoList infoList;
	m_pManager->GetInfoList( infoList );

	for (TSelectionTreeInfoList::const_iterator it = infoList.begin(); it != infoList.end(); ++it)
	{
		const SSelectionTreeInfo& info = *it;
		if (info.IsTree)
			ValidateTree(info);
		else
			ValidateReference(info);
	}

	for (TReportedErrors::iterator it = m_ReportedErrors.begin(); it != m_ReportedErrors.end(); ++it)
	{
		m_pManager->RemoveError(*it);
	}

	m_pManager->ReloadErrors();
	m_ReportedErrors = m_NewReportedErrors;
}

void CSelectionTreeValidator::ValidateTree(const SSelectionTreeInfo& info)
{
	TVariableList varList;
	SNode rootNode;

	SSelectionTreeBlockInfo vars;
	bool ok = info.GetBlockById(vars, eSTTI_Variables);
	assert( ok );
	ValidateVarBlock(info, vars, varList);

	SSelectionTreeBlockInfo signalsInfo;
	ok = info.GetBlockById(signalsInfo, eSTTI_Signals);
	assert( ok );
	ValidateSignalBlock(info, signalsInfo, varList);

	SSelectionTreeBlockInfo tree;
	ok = info.GetBlockById(tree, eSTTI_Tree);
	assert( ok );
	ValidateTreeBlock(info, tree, tree.XmlData, rootNode, varList);

	//SSelectionTreeBlockInfo leafmappings;
	//ok = info.GetBlockById(leafmappings, eSTTI_LeafTranslations);
	//assert( ok );
	//ValidateLeafMappingBlock(info, leafmappings, rootNode);

}
void CSelectionTreeValidator::ValidateReference(const SSelectionTreeInfo& info)
{
	TVariableList varList;

	SSelectionTreeBlockInfo vars;
	if (info.GetBlockById(vars, eSTTI_Variables)) 
	{
		ValidateVarBlock(info, vars, varList);
	}

	SSelectionTreeBlockInfo signalsInfo;
	if (info.GetBlockById(signalsInfo, eSTTI_Signals)) 
	{
		ValidateSignalBlock(info, signalsInfo, varList);
	}

	const int treecount = info.GetBlockCountById(eSTTI_Tree);
	for (int i = 0; i < treecount; ++i)
	{
		SNode rootNode;
		SSelectionTreeBlockInfo tree;
		bool ok = info.GetBlockById(tree, eSTTI_Tree, i);
		assert( ok );
		ValidateTreeBlock(info, tree, tree.XmlData, rootNode, varList);
	}
}

void CSelectionTreeValidator::ValidateVarBlock(const SSelectionTreeInfo& info, const SSelectionTreeBlockInfo& block, TVariableList& varlist)
{
	XmlNodeRef xmlNode = block.XmlData;

	// Variables
	bool xmlOk = true;
	for ( int i = 0; i < xmlNode->getChildCount(); ++i )
	{
		XmlNodeRef xmlChild = xmlNode->getChild( i );

		const char* tag = xmlChild->getTag();
		const char* name = xmlChild->getAttr( "name" );
		if ( tag && name )
		{
			if (strcmpi( tag, "Variable" ) == 0 )
			{
				string nameStr = name;
				nameStr.MakeLower();
				if (!stl::push_back_unique(varlist, nameStr))
				{
					ReportError(info, block, string().Format("Duplicated Variable \"%s\"", name), "A Variable with same name exists more than one time in the tree", eSTTI_Variables, eET_Warning);
				}
			}
			else if (strcmpi( tag, "Ref" ) == 0 )
			{
				/*SSelectionTreeBlockInfo refinfo;
				if (m_pManager->GetRefInfoByName(eSTTI_Variables, name, refinfo))
				{
					ValidateVarBlock(info, refinfo, varlist);
				}
				else
				{
					ReportError(info, block, string().Format("Reference \"%s\" does not exist", name), "A Reference could not be found", eSTTI_Variables, eET_Error);
				}*/
			}
			else
			{
				xmlOk = false;
			}
		}
		else
		{
			xmlOk = false;
		}
	}
	if (!xmlOk)
		ReportError(info, block, "Fatal Error: XML error", "The Variable block has invalid items", eSTTI_Variables, eET_Error);
}

void CSelectionTreeValidator::ValidateSignalBlock(const SSelectionTreeInfo& info, const SSelectionTreeBlockInfo& block, const TVariableList& varlist)
{
	XmlNodeRef xmlNode = block.XmlData;

	// Signals
	bool xmlOk = true;
	for ( int i = 0; i < xmlNode->getChildCount(); ++i )
	{
		XmlNodeRef xmlChild = xmlNode->getChild( i );

		const char* tag = xmlChild->getTag();
		const char* name = xmlChild->getAttr( "name" );
		if ( tag && name )
		{
			if (strcmpi( tag, "Signal" ) == 0 )
			{
				const char* var = xmlChild->getAttr( "variable" );
				if (var)
				{
					string tmp = var;
					tmp.MakeLower();
					if (!stl::find(varlist, tmp))
					{
						ReportError(info, block, string().Format("Signal Variable \"%s\" does not exist (Signal: \"%s\")", var, name), "A Variable that is controlled by a signal does not exist", eSTTI_Signals, info.IsTree ? eET_Error : eET_Warning);
					}
				}
				else
				{
					xmlOk = false;
				}
			}
			else if (strcmpi( tag, "Ref" ) == 0 )
			{
				SSelectionTreeBlockInfo refinfo;
				/*if (m_pManager->GetRefInfoByName(eSTTI_Signals, name, refinfo))
				{
					ValidateSignalBlock(info, refinfo, varlist);
				}
				else
				{
					ReportError(info, block, string().Format("Reference \"%s\" does not exist", name), "A Reference could not be found", eSTTI_Signals, eET_Error);
				}*/
			}
			else
			{
				xmlOk = false;
			}
		}
		else
		{
			xmlOk = false;
		}
	}
	if (!xmlOk)
		ReportError(info, block, "Fatal Error: XML error", "The Signal block has invalid items", eSTTI_Signals, eET_Error);
}

void CSelectionTreeValidator::ValidateTreeBlock(const SSelectionTreeInfo& info, const SSelectionTreeBlockInfo& block, XmlNodeRef xmlNode, SNode& node, const TVariableList& varlist)
{
	// tree
	bool xmlOk = true;
	const char* tag = xmlNode->getTag();
	const char* name = xmlNode->getAttr( "name" );
	if ( tag && name )
	{
		if ( strcmpi(tag, "Ref") == 0)
		{
			/*SSelectionTreeBlockInfo refinfo;
			if (m_pManager->GetRefInfoByName(eSTTI_Tree, name, refinfo))
			{
				ValidateTreeBlock(info, refinfo, refinfo.XmlData, node, varlist);
			}
			else
			{
				ReportError(info, block, string().Format("Reference \"%s\" does not exist", name), "A Reference could not be found", eSTTI_Tree, eET_Error);
			}*/
		}
		else
		{
			//CSelectionTree_TreeNode::ESelectionTreeNodeSubType subType = CSelectionTree_TreeNode::GetSubTypeEnum( tag );
			if ( name && name[0] )
			{
				//if (subType != CSelectionTree_TreeNode::eSTNST_Invalid )
				{
					const char* condition = xmlNode->getAttr( "condition" );
					if (condition && condition[0])
						ValidateCondition( info, block, condition, varlist, name );
					if ( strcmpi(name, INVALID_HELPER_NODE) == 0)
					{
						ReportError(info, block, "Invalid Selection Tree", "The Selection Tree is invalid, this can happen if the tree or a referenced tree has more than one root nodes, or a condition node without a child!", eSTTI_Tree, eET_Error);
					}
					else
					{
						node.Name = name;
						node.Name.MakeLower();
						node.Children.resize(xmlNode->getChildCount());
						for (int i = 0; i < xmlNode->getChildCount(); ++i)
						{
							ValidateTreeBlock(info, block, xmlNode->getChild(i), node.Children[i], varlist);
						}
					}
				}
			}
			else if (strcmp("Root", xmlNode->getTag()) == 0) // temp solution to ignore the rootnode from throwing errors [michiel]
			{
			}
			else
			{
				xmlOk = false;
			}
		}
	}
	else
	{
		xmlOk = false;
	}
	if (!xmlOk)
		ReportError(info, block, "Fatal Error: XML error", "The Tree block has invalid items", eSTTI_Tree, eET_Error);
}

void CSelectionTreeValidator::ValidateLeafMappingBlock(const SSelectionTreeInfo& info, const SSelectionTreeBlockInfo& block, const SNode& rootnode)
{
	XmlNodeRef xmlNode = block.XmlData;

	// leaf mappings
	bool xmlOk = true;
	for ( int i = 0; i < xmlNode->getChildCount(); ++i )
	{
		XmlNodeRef xmlChild = xmlNode->getChild( i );
		const char* tag = xmlChild->getTag();
		const char* node = xmlChild->getAttr( "node" );
		const char* target = xmlChild->getAttr( "target" );
		if ( tag && node && target )
		{
			if ( strcmpi(tag, "Map") == 0)
			{
				string nodeStr = node;
				nodeStr.MakeLower();

				SArgumentParser args;
				args.SetDelimiter(":");
				args.SetArguments(nodeStr.c_str());
				std::vector<string> path;
				for (int i = args.GetArgCount()-1; i >= 0; --i)
				{
					string node;
					args.GetArg(i, node);
					path.push_back(node);
				}

				if (!ValidateNodePath(rootnode, path))
				{
					ReportError(info, block, string().Format("Source Node \"%s\" for LeafMapping does not exist", args.GetAsString()), "The Source Node that is used to by a LeafMapping does not exsist", eSTTI_LeafTranslations, eET_Warning);
				}
			}
		}
		else
		{
			xmlOk = false;
		}
	}
	if (!xmlOk)
		ReportError(info, block, "Fatal Error: XML error", "The LeafMapping block has invalid items", eSTTI_LeafTranslations, eET_Error);
}

void CSelectionTreeValidator::ValidateCondition(const SSelectionTreeInfo& info, const SSelectionTreeBlockInfo& block, const string& conditionStr, const TVariableList& varlist, const char* nodeName)
{
	SArgumentParser args;
	args.SetDelimiter(" ");
	args.SetArguments(conditionStr.c_str());

	std::list<string> vars;
	for (int i = 0; i < args.GetArgCount(); ++i)
	{
		string part;
		args.GetArg(i, part);
		
		const char* cStr = part.c_str();
		if (cStr[0] == '!') cStr++;

		if ( strcmpi(cStr, "or") == 0
			|| strcmpi(cStr, "and") == 0
			|| strcmpi(cStr, "xor") == 0
			|| strcmpi(cStr, "true") == 0
			|| strcmpi(cStr, "false") == 0
			|| strcmpi(cStr, INVALID_HELPER_NODE) == 0)
			continue;

		string tmp = cStr;
		tmp.MakeLower();
		stl::push_back_unique(vars, tmp);
	}

	for (std::list<string>::iterator it = vars.begin(); it != vars.end(); ++it)
	{
		if (!stl::find(varlist, *it))
		{
			if (info.IsTree)
				ReportError(info, block, string().Format("Condition Varaible \"%s\" checked in Node \"%s\" does not exist", *it, nodeName), "The Variable that is checked in the condition does not belong to this Tree", eSTTI_Tree, eET_Error);
			else
				ReportError(info, block, string().Format("Condition Varaible \"%s\" checked in Node \"%s\" does not exist", *it, nodeName), "The Variable that is checked in the condition does not belong to this Tree", eSTTI_Tree, eET_Warning);
		}
	}
}

bool CSelectionTreeValidator::ValidateNodePath(const SNode& node, std::vector<string> path, bool firstNodeFound)
{
	if (node.Name == *path.rbegin())
	{
		path.pop_back();
		if (path.size() == 0) 
			return true;
		firstNodeFound = true;
	}
	else if (firstNodeFound)
	{
		return false;
	}

	for (std::vector<SNode>::const_iterator it = node.Children.begin(); it != node.Children.end(); ++it)
	{
		if (ValidateNodePath(*it, path, firstNodeFound))
			return true;
	}

	return false;
}

void CSelectionTreeValidator::ReportError( const SSelectionTreeInfo &info, const SSelectionTreeBlockInfo& block, const string& err, const string& desc, ESelectionTreeTypeId blockType, EErrorType errType )
{
	switch (m_pManager->CV_bst_validator_warnlevel)
	{
	case CSelectionTreeManager::eWL_OnlyErrors:
		if (errType != eET_Error) return;
		break;
	case CSelectionTreeManager::eWL_NoWarningsInRef:
		if (errType != eET_Error && !info.IsTree) return;
		break;
	case CSelectionTreeManager::eWL_OnlyErrorsInTree:
		if (errType != eET_Error) return;
	case CSelectionTreeManager::eWL_NoErrorsInRef:
		if (!info.IsTree) return;
		break;
	}

	CSelectionTreeErrorRecord::ESeverity severity;
	switch(errType)
	{
	case eET_Error: severity = CSelectionTreeErrorRecord::ESEVERITY_ERROR; break;
	case eET_Warning: severity = CSelectionTreeErrorRecord::ESEVERITY_WARNING; break;
	case eET_Comment: severity = CSelectionTreeErrorRecord::ESEVERITY_COMMENT; break;
	}

	CSelectionTreeErrorRecord::ETreeModule module;
	switch(blockType)
	{
	case eSTTI_Tree: module = CSelectionTreeErrorRecord::EMODULE_TREE; break;
	case eSTTI_Signals: module = CSelectionTreeErrorRecord::EMODULE_SIGNALS; break;
	case eSTTI_Variables: module = CSelectionTreeErrorRecord::EMODULE_VARIABLES; break;
	case eSTTI_LeafTranslations: module = CSelectionTreeErrorRecord::EMODULE_LEAFTRANSLATIONS; break;
	}

	CSelectionTreeErrorReport::TSelectionTreeErrorId id = m_pManager->ReportError( CSelectionTreeErrorRecord(severity, module, 
		info.IsTree ? CSelectionTreeErrorRecord::ETYPE_TREE : CSelectionTreeErrorRecord::ETYPE_REFERENCE,
		info.IsTree ? info.Name : block.Name,
		err.c_str(), desc.c_str(), 
		info.IsTree ? info.FileName : block.FileName),
		false );
	m_NewReportedErrors.push_back(id);
}