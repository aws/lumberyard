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
#include "SelectionTreeManager.h"

#include "Util/XmlHistoryManager.h"

#include "AI/AIManager.h"
#include "AI/AiBehaviorLibrary.h"

#include "SelectionTreeEditor.h"
#include "SelectionTreeModifier.h"
#include "SelectionTreeValidator.h"

#include "Graph/SelectionTreeGraph.h"

#include "Dialogs/SelectionTreeErrorDialog.h"

ICVar* CSelectionTreeManager::CV_bst_debug_ai = NULL;
int CSelectionTreeManager::CV_bst_debug_centernode = 0;
int CSelectionTreeManager::CV_bst_validator_warnlevel = CSelectionTreeManager::eWL_NoWarningsInRef;

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////// Info ////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
SSelectionTreeBlockInfo::SSelectionTreeBlockInfo()
	: Name( NULL )
	, FileName( NULL )
	, IsModified( false )
	, Type( eSTTI_Undefined )
{
}

/////////////////////////////////////////////////////////////////////////////
SSelectionTreeInfo::SSelectionTreeInfo()
	: Name( NULL )
	, FileName( NULL )
	, TreeType( NULL )
	, IsModified( false )
	, IsLoaded( false )
	, IsTree( false )
	, CurrTreeIndex(0)
{
}

/////////////////////////////////////////////////////////////////////////////
bool SSelectionTreeInfo::GetBlockById( SSelectionTreeBlockInfo& out, ESelectionTreeTypeId typeId, int index /*= 0*/ ) const
{
	for ( int i = 0; i < GetBlockCount(); ++i )
	{
		if (Blocks[ i ].Type == typeId)
		{
			if (index == 0)
			{
				out = Blocks[ i ];
				return true;
			}
			--index;
		}
	}
	return false;
}

/////////////////////////////////////////////////////////////////////////////
int SSelectionTreeInfo::GetBlockCountById( ESelectionTreeTypeId typeId ) const
{
	int res = 0;
	for ( int i = 0; i < GetBlockCount(); ++i )
	{
		if (Blocks[ i ].Type == typeId) res++;
	}
	return res;
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////// CSelectionTreeManager ///////////////////////////
/////////////////////////////////////////////////////////////////////////////
CSelectionTreeManager::CSelectionTreeManager()
: m_pXmlHistoryMan( new CXmlHistoryManager() )
, m_pVarView( NULL )
, m_pSigView( NULL )
, m_pTimestampView( NULL )
, m_pGraph( NULL )
, m_pErrorReport( new CSelectionTreeErrorReport() )
, m_bIsLoading( false )
{
	m_pModifier = new CSelectionTreeModifier( this );
	m_pValiator = new CSelectionTreeValidator( this );
	m_pXmlHistoryMan->RegisterEventListener(this);
	GetIEditor()->GetLevelIndependentFileMan()->RegisterModule(this);
}

/////////////////////////////////////////////////////////////////////////////
CSelectionTreeManager::~CSelectionTreeManager()
{
	GetIEditor()->GetLevelIndependentFileMan()->UnregisterModule(this);
	CSelectionTreeErrorDialog::Close();

	delete m_pValiator;
	delete m_pErrorReport;
	delete m_pXmlHistoryMan;
	delete m_pModifier;
}

/////////////////////////////////////////////////////////////////////////////
void CSelectionTreeManager::Init()
{
	CV_bst_debug_ai  = REGISTER_STRING("bst_debug_ai", "", VF_NULL, "Name of AI to display SelectionTree");
	REGISTER_CVAR2("bst_debug_centernode", &CV_bst_debug_centernode, CV_bst_debug_centernode, VF_NULL, "Centers the current selected node in debug mode");
	REGISTER_CVAR2("bst_validator_warnlevel", &CV_bst_validator_warnlevel, CV_bst_validator_warnlevel, VF_NULL, "Level of warings for BST"
		"\n0: Dispay all Errors and Warnings"
		"\n1: Hide all Warnings from References"
		"\n2: Hide all Warnings and Errors from References"
		"\n3: Show only Errors"
		"\n4: Show only Errors from SelectionTrees")->SetOnChangeCallback(&CSelectionTreeManager::OnBSTValidatorWarnVariableChanged);
}

/////////////////////////////////////////////////////////////////////////////
void CSelectionTreeManager::Reset()
{
	m_XmlGroupList.clear();
	m_BlocksToResolve.clear();
	m_pXmlHistoryMan->DeleteAll();
}

/////////////////////////////////////////////////////////////////////////////
void CSelectionTreeManager::OnEvent( EHistoryEventType event, void* pData )
{
	SXmlHistoryGroup* pGroup = (SXmlHistoryGroup*) pData;
	switch (event)
	{
	case eHET_HistoryGroupAdded:
		if (!stl::push_back_unique(m_XmlGroupList, pGroup))
			assert( false );
		break;
	case eHET_HistoryGroupRemoved:
		if (!stl::find_and_erase(m_XmlGroupList, pGroup ))
			assert( false );
		break;
	default:
		if (CSelectionTreeEditor::GetInstance())
		{
			CSelectionTreeEditor::GetInstance()->SetTreeName(CString(MAKEINTRESOURCE(IDS_NO_TREE_LOADED_TITLE)));
			CSelectionTreeEditor::GetInstance()->SetVariablesName(CString(MAKEINTRESOURCE(IDS_NO_VARIABLES_LOADED_TITLE)));
			CSelectionTreeEditor::GetInstance()->SetSignalsName(CString(MAKEINTRESOURCE(IDS_NO_SIGNALS_LOADED_TITLE)));
			SSelectionTreeInfo info;
			int blockTreeIndex = 0;
			if (GetCurrentTreeInfo(info, &blockTreeIndex))
			{
				if (info.IsTree)
				{
					CSelectionTreeEditor::GetInstance()->SetTreeName( info.Name );
					CSelectionTreeEditor::GetInstance()->SetVariablesName(CString(MAKEINTRESOURCE(IDS_TREE_VARIABLES_TITLE)));
					CSelectionTreeEditor::GetInstance()->SetSignalsName(CString(MAKEINTRESOURCE(IDS_TREE_SIGNALS_TITLE)));
				}
				else
				{
					SSelectionTreeBlockInfo block;
					if (info.GetBlockById(block, eSTTI_Tree, blockTreeIndex))
						CSelectionTreeEditor::GetInstance()->SetTreeName( block.Name );
					else
						CSelectionTreeEditor::GetInstance()->SetTreeName(CString(MAKEINTRESOURCE(IDS_NO_TREE_IN_BLOCK_TITLE)));

					if (info.GetBlockById(block, eSTTI_Variables))
						CSelectionTreeEditor::GetInstance()->SetVariablesName( block.Name );
					else
						CSelectionTreeEditor::GetInstance()->SetVariablesName(CString(MAKEINTRESOURCE(IDS_NO_VARIABLES_IN_BLOCK_TITLE)));

					if (info.GetBlockById(block, eSTTI_Signals))
						CSelectionTreeEditor::GetInstance()->SetSignalsName( block.Name );
					else
						CSelectionTreeEditor::GetInstance()->SetSignalsName(CString(MAKEINTRESOURCE(IDS_NO_SIGNALS_IN_BLOCK_TITLE)));

					if (info.GetBlockById(block, eSTTI_TimeStamp))
						CSelectionTreeEditor::GetInstance()->SetTimestampsName( block.Name );
					else
						CSelectionTreeEditor::GetInstance()->SetSignalsName(CString(MAKEINTRESOURCE(IDS_NO_TIMESTAMPS_IN_BLOCK_TITLE)));
				}
			}
		}
		break;
	}
	if (!m_bIsLoading)
	{
		m_pValiator->Validate();
	}
}

bool CSelectionTreeManager::PromptChanges()
{
	TFileCountList oldFileCountlist = m_FileList;
	UpdateFileList();

	bool isModified = oldFileCountlist != m_FileList;
	TSelectionTreeInfoList infoList;
	GetInfoList(infoList);
	for (TSelectionTreeInfoList::iterator it = infoList.begin(); it != infoList.end(); ++it)
	{
		if (it->IsModified)
		{
			isModified = true;
			break;
		}
	}
	m_FileList = oldFileCountlist;


	if (isModified)
	{
		CSelectionTreeModifier::EMsgBoxResult res = m_pModifier->ShowMsgBox("Behaviour Selection Tree(s) not saved!", "Save", "Don't save",
			"Some BST Files are modified!\nDo you want to save your changes?");

		if (res == CSelectionTreeModifier::eMBR_UserBtn1)
		{
			return SaveAll();
		}
		else if (res == CSelectionTreeModifier::eMBR_Cancel)
		{
			return false;
		}
	}
	return true;
}

/////////////////////////////////////////////////////////////////////////////
IXmlHistoryManager* CSelectionTreeManager::GetHistory()
{
	return m_pXmlHistoryMan;
}

/////////////////////////////////////////////////////////////////////////////
////////////////////////////// object access ////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

void CSelectionTreeManager::GetInfoList( TSelectionTreeInfoList& infoList ) const
{
	for ( TXmlGroupList::const_iterator it = m_XmlGroupList.begin(); it != m_XmlGroupList.end(); ++it )
	{
		SSelectionTreeInfo info;
		if (ReadGroupInfo( info, *it ))
			infoList.push_back( info );
	}
}

/////////////////////////////////////////////////////////////////////////////
bool CSelectionTreeManager::GetTreeInfoByName(SSelectionTreeInfo& out, const char* name) const
{
	SXmlHistoryGroup* pGroup = GetSelectionTree(name);
	if (pGroup)
	{
		return ReadTreeGroupInfo(out, pGroup);
	}
	return false;
}

/////////////////////////////////////////////////////////////////////////////
bool CSelectionTreeManager::GetCurrentTreeInfo(SSelectionTreeInfo& out, int* pIndex/* = NULL*/) const
{
	TGroupIndexMap userIndex;
	const SXmlHistoryGroup* pCurrGroup = m_pXmlHistoryMan->GetActiveGroup(userIndex);
	if ( pCurrGroup )
	{
		if (pIndex) *pIndex = userIndex[eSTTI_Tree];
		return ReadGroupInfo( out, pCurrGroup );
	}
	return false;
}

/////////////////////////////////////////////////////////////////////////////
void CSelectionTreeManager::DisplayTree( const char* name )
{
	string displayName;
	SXmlHistoryGroup* pGroup = GetSelectionTree( name, &displayName );
	m_pXmlHistoryMan->SetActiveGroup( pGroup, displayName.c_str() );
}

/////////////////////////////////////////////////////////////////////////////
void CSelectionTreeManager::UnloadViews()
{
	m_pXmlHistoryMan->SetActiveGroup( NULL, "" );
}

/////////////////////////////////////////////////////////////////////////////
bool CSelectionTreeManager::AddNewTree( const STreeDefinition& definition )
{
	if ( GetSelectionTree( definition.name ) != NULL ) return false;

	XmlNodeRef infoNode = gEnv->pSystem->CreateXmlNode("Info");
	infoNode->setAttr( "name", definition.name );
	infoNode->setAttr( "filename", definition.filename );

	SXmlHistoryGroup* pTreeGroup = m_pXmlHistoryMan->CreateXmlGroup( eSTGTI_Tree );
	pTreeGroup->CreateXmlHistory( eSTTI_NameInfo, infoNode );

	XmlNodeRef varNode = gEnv->pSystem->CreateXmlNode("Variables");
	XmlNodeRef sigNode = gEnv->pSystem->CreateXmlNode("SignalVariables");
	XmlNodeRef timestampNode = gEnv->pSystem->CreateXmlNode("Timestamps");
	XmlNodeRef treeNode = gEnv->pSystem->CreateXmlNode("Root");
	//treeNode->setAttr( "name", "Root" );
	pTreeGroup->CreateXmlHistory( eSTTI_Variables, varNode );
	pTreeGroup->CreateXmlHistory( eSTTI_Signals, sigNode );
	pTreeGroup->CreateXmlHistory( eSTTI_TimeStamp, timestampNode );
	pTreeGroup->CreateXmlHistory( eSTTI_Tree, treeNode );

	m_pXmlHistoryMan->AddXmlGroup( pTreeGroup, string().Format("Added %s (SelectionTree)", definition.name) );
	m_pXmlHistoryMan->SetActiveGroup( pTreeGroup, definition.name );
	return true;
}

/////////////////////////////////////////////////////////////////////////////
bool CSelectionTreeManager::DeleteTree( const char* name )
{
	SXmlHistoryGroup* pTreeGroup =  GetSelectionTree( name );
	if (pTreeGroup)
	{
		m_pXmlHistoryMan->RemoveXmlGroup( pTreeGroup, string().Format("Deleted %s (SelectionTree)", name) );
		return true;
	}
	return false;
}

/////////////////////////////////////////////////////////////////////////////
bool CSelectionTreeManager::EditTree( const char* name, const STreeDefinition& definition )
{
	SXmlHistoryGroup* pTreeGroup =  GetSelectionTree( name );
	if ( pTreeGroup && (GetSelectionTree( definition.name ) == NULL || GetSelectionTree( definition.name ) == pTreeGroup) )
	{
		m_pXmlHistoryMan->PrepareForNextVersion();
		SXmlHistory* pNameInfo = pTreeGroup->GetHistoryByTypeId(eSTTI_NameInfo);
		assert(pNameInfo);
		if (!pNameInfo) return false;
		XmlNodeRef xmlNode = gEnv->pSystem->CreateXmlNode("Info");
		xmlNode->setAttr( "name", definition.name );
		xmlNode->setAttr( "filename", definition.filename );
		m_pXmlHistoryMan->RecordNextVersion( pNameInfo, xmlNode, string().Format("Modified Tree %s", definition.name).c_str() );
		return true;
	}
	return false;
}

/////////////////////////////////////////////////////////////////////////////
//////////////////////////// internal access ////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
SXmlHistoryGroup* CSelectionTreeManager::GetSelectionTree( const char* name, string* displayName ) const
{
	for ( TXmlGroupList::const_iterator it = m_XmlGroupList.begin(); it != m_XmlGroupList.end(); ++it )
	{
		SSelectionTreeInfo info;
		if (ReadTreeGroupInfo( info, *it ))
		{
			if ( strcmpi( info.Name, name ) == 0 )
			{
				if ( displayName )
					*displayName = info.Name;
				return *it;
			}
		}
	}
	return NULL;
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////// Loading - Saving ////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
void CSelectionTreeManager::LoadFromFolder( const char* folder )
{
	if (!PromptChanges()) return;

	m_bIsLoading = true;
	m_pXmlHistoryMan->SetExclusiveListener(this);
	Reset();
	LoadFromFolderInt( folder );
	m_pXmlHistoryMan->SetExclusiveListener(NULL);
	m_bIsLoading = false;
	m_pXmlHistoryMan->ClearHistory(true);
	UpdateFileList(true);
}

/////////////////////////////////////////////////////////////////////////////
bool CSelectionTreeManager::SaveAll()
{
	if (!m_pErrorReport->IsEmpty())
		CSelectionTreeErrorDialog::Open(m_pErrorReport);

	TFileCountList oldFileCountlist = m_FileList;
	UpdateFileList();

	typedef std::list< XmlNodeRef > TXmlNodeList;
	typedef std::map< string, std::pair<TXmlNodeList, bool> > TFileList;
	TFileList trees;
	TFileList blocks;

	TSelectionTreeInfoList infoList;
	GetInfoList( infoList );

	for ( TSelectionTreeInfoList::const_iterator it = infoList.begin(); it != infoList.end(); ++it )
	{
		const SSelectionTreeInfo& info = *it;

		if ( info.IsTree )
		{
			XmlNodeRef treeNode = gEnv->pSystem->CreateXmlNode( "BehaviorTree" );
			for ( int i = 0; i < info.GetBlockCount(); ++i )
				treeNode->addChild( info.Blocks[ i ].XmlData );

			trees[ info.FileName ].first.push_back( treeNode );
			trees[ info.FileName ].second |= info.IsModified || m_FileList[info.FileName] != oldFileCountlist[info.FileName];
		}
		else // Block group
		{
			for ( int i = 0; i < info.GetBlockCount(); ++i )
			{
				const SSelectionTreeBlockInfo& blockInfo = info.Blocks[ i ];

				XmlNodeRef blockNode = NULL;
				if ( blockInfo.Type == eSTTI_Tree )
				{
					blockNode = gEnv->pSystem->CreateXmlNode( "Block" );
					blockNode->addChild( blockInfo.XmlData );
				}
				else
				{
					blockNode = blockInfo.XmlData;
				}
				blockNode->setTag( "Block" );
				blockNode->setAttr( "name", blockInfo.Name );
				blockNode->setAttr( "group", info.Name );
				if (blockNode->getChildCount() > 0)
				{
					blocks[ blockInfo.FileName ].first.push_back( blockNode );
				}
				else
				{
					m_pModifier->ShowErrorMsg( "Error: Faild to save file(s)!",
						"Block %s can't be saved! The Block is empty.\n A reference block needs at least one element to determine the block type!", blockInfo.Name );
					return false;
				}
				blocks[ blockInfo.FileName ].second |= info.Blocks[ i ].IsModified || m_FileList[info.Blocks[ i ].FileName] != oldFileCountlist[info.Blocks[ i ].FileName];
			}
		}
	}

	std::set< string > m_NotSavedFiles;
	for ( TFileList::iterator file = trees.begin(); file != trees.end(); ++file )
	{
		if (!file->second.second) continue;

		XmlNodeRef root = NULL;

		for ( TXmlNodeList::iterator tree = file->second.first.begin(); tree != file->second.first.end(); ++tree )
		{
			if(!root)
				root = *tree;
			else
				root->addChild( *tree );
		}
		if (!XmlHelpers::SaveXmlNode(GetIEditor()->GetFileUtil(), root, file->first.c_str()))
		{
			m_NotSavedFiles.insert(file->first);
		}
	}

	for ( TFileList::iterator file = blocks.begin(); file != blocks.end(); ++file )
	{
		if (!file->second.second) continue;

		XmlNodeRef root = gEnv->pSystem->CreateXmlNode( "Blocks" );
		for ( TXmlNodeList::iterator block = file->second.first.begin(); block != file->second.first.end(); ++block )
		{
			root->addChild( *block );
		}
		if (!XmlHelpers::SaveXmlNode(GetIEditor()->GetFileUtil(), root, file->first.c_str()))
		{
			m_NotSavedFiles.insert(file->first);
		}
	}

	std::set< string > m_NotDeletedFiles;
	for (TFileCountList::iterator it = m_FileList.begin(); it != m_FileList.end(); ++it)
	{
		if (it->second == 0)
		{
			bool ok = false;
			if (CFileUtil::FileExists(it->first.c_str()))
				ok = CFileUtil::DeleteFile(Path::GamePathToFullPath(it->first.c_str()));
			if (!ok)
			{
				m_NotDeletedFiles.insert(it->first);
			}
		}
	}

	if ( m_NotSavedFiles.size() == 0 && m_NotDeletedFiles.size() == 0 )
	{
		m_pXmlHistoryMan->FlagHistoryAsSaved();
		UpdateFileList(true);
		gEnv->pAISystem->GetSelectionTreeManager()->Reload();
		return true;
	}

	if (m_NotSavedFiles.size() > 0)
	{
		string filesNotSafed;
		for (std::set< string >::iterator it = m_NotSavedFiles.begin(); it != m_NotSavedFiles.end(); ++it)
		{
			filesNotSafed += *it;
			filesNotSafed += "\n";
		}
		m_pModifier->ShowErrorMsg("Error: Faild to save file(s)!", 
			"Following files where not safed:\n%sMaybe in pak or write protected?", filesNotSafed.c_str());
	}

	if (m_NotDeletedFiles.size() > 0)
	{
		string filesNotDeleted;
		for (std::set< string >::iterator it = m_NotDeletedFiles.begin(); it != m_NotDeletedFiles.end(); ++it)
		{
			filesNotDeleted += *it;
			filesNotDeleted += "\n";
		}
		m_pModifier->ShowErrorMsg("Error: Faild to delete file(s)!", 
			"Following files where not deleted:\n%sMaybe in pak or write protected?", filesNotDeleted.c_str() );
	}

	return false;
}

/////////////////////////////////////////////////////////////////////////////
void CSelectionTreeManager::LoadFromFolderInt( const char* folder )
{
	assert( folder );
	if ( !folder ) return;

	ICryPak * pCryPak = gEnv->pCryPak;
	_finddata_t fd;
	char filename[_MAX_PATH];

	string search = string( folder ) + "*";
	intptr_t handle = pCryPak->FindFirst( search.c_str(), &fd );

	if ( handle != -1 )
	{
		int res = 0;
		do 
		{
			if ( strcmpi( fd.name, "." ) == 0 
				|| strcmpi( fd.name, ".." ) == 0 )
			{
				res = pCryPak->FindNext( handle, &fd );
				continue;
			}

			cry_strcpy( filename, folder );
			cry_strcat( filename, "/" );
			cry_strcat( filename, fd.name );

			if (fd.attrib & _A_SUBDIR) // load directories.
			{
				cry_strcat( filename, "/" );
				LoadFromFolderInt( filename );
			}
			else
			{
				XmlNodeRef xmlNode = GetISystem()->LoadXmlFromFile( filename );
				if ( xmlNode )
				{
					LoadFromFileNode( xmlNode, filename );
				}
			}

			res = pCryPak->FindNext( handle, &fd );
		}
		while ( res >= 0 );
		pCryPak->FindClose( handle );
	}
}

/////////////////////////////////////////////////////////////////////////////
void CSelectionTreeManager::LoadFromFileNode( const XmlNodeRef& node, const char* filename )
{
	assert( node );
	if ( !node ) return;

	//for ( int i = 0; i < node->getChildCount(); ++i )
	//{
		const char* tag = node->getTag();

		if ( strcmpi( tag, "BehaviorTree") == 0 )
		{
			if ( GetSelectionTree( filename ) == NULL )
				LoadTreeXml( node, filename );
			else
				gEnv->pLog->LogError( "[BST] Failed to load Tree \"%s\" from file \"%s\" (Already exits with same name)", filename, filename );
		}
	//}
}

/////////////////////////////////////////////////////////////////////////////
void CSelectionTreeManager::LoadTreeXml( const XmlNodeRef& tree, const char* filename )
{
	XmlNodeRef infoNode = gEnv->pSystem->CreateXmlNode();
	if ( LoadTreeInfoNode( infoNode, tree, filename ) )
	{
		SXmlHistoryGroup* pTreeGroup = m_pXmlHistoryMan->CreateXmlGroup( eSTGTI_Tree );
		pTreeGroup->CreateXmlHistory( eSTTI_NameInfo, infoNode );
		
		for ( int i = 0; i < tree->getChildCount(); ++i )
		{
			XmlNodeRef child = tree->getChild( i );
			const char* tag = child->getTag();

			if ( strcmpi( tag, "Variables") == 0 && pTreeGroup->GetHistoryByTypeId( eSTTI_Variables ) == NULL )
				pTreeGroup->CreateXmlHistory( eSTTI_Variables, child );

			else if ( strcmpi( tag, "SignalVariables") == 0 && pTreeGroup->GetHistoryByTypeId( eSTTI_Signals ) == NULL )
				pTreeGroup->CreateXmlHistory( eSTTI_Signals, child );

			else if (  strcmpi( tag, "Timestamps") == 0 && pTreeGroup->GetHistoryCountByTypeId( eSTTI_TimeStamp ) == 0 )
				pTreeGroup->CreateXmlHistory( eSTTI_TimeStamp, child );
			
			else if ( strcmpi( tag, "Root") == 0 && pTreeGroup->GetHistoryByTypeId( eSTTI_Tree ) == NULL )
				pTreeGroup->CreateXmlHistory( eSTTI_Tree, child );
		}

		if ( pTreeGroup->GetHistoryCountByTypeId( eSTTI_Variables ) == 0 )
			pTreeGroup->CreateXmlHistory( eSTTI_Variables, gEnv->pSystem->CreateXmlNode( "Variables" ) );

		if ( pTreeGroup->GetHistoryCountByTypeId( eSTTI_Signals ) == 0 )
			pTreeGroup->CreateXmlHistory( eSTTI_Signals, gEnv->pSystem->CreateXmlNode( "SignalVariables" ) );

		if ( pTreeGroup->GetHistoryCountByTypeId( eSTTI_TimeStamp ) == 0 )
			pTreeGroup->CreateXmlHistory( eSTTI_TimeStamp, gEnv->pSystem->CreateXmlNode( "Timestamps" ) );

		if ( pTreeGroup->GetHistoryCountByTypeId( eSTTI_Tree ) == 0 )
			pTreeGroup->CreateXmlHistory( eSTTI_Tree, gEnv->pSystem->CreateXmlNode( "Root" ) );
		
		m_pXmlHistoryMan->AddXmlGroup( pTreeGroup );
		CryLogAlways( "[BST] Load Tree \"%s\" from file \"%s\"", infoNode->getAttr("name"), filename );
	}
	else
	{
		gEnv->pLog->LogError( "[BST] Failed to load Tree from file \"%s\" (Invalid Tree)", filename );
	}
}

/////////////////////////////////////////////////////////////////////////////
void CSelectionTreeManager::UpdateFileList(bool reset /*= false*/)
{
	if (reset)
		m_FileList.clear();

	for (TFileCountList::iterator it = m_FileList.begin(); it != m_FileList.end(); ++it)
		it->second = 0;

	TSelectionTreeInfoList list;
	GetInfoList(list);
	for (TSelectionTreeInfoList::iterator it = list.begin(); it != list.end(); ++it)
	{
		if (it->IsTree)
			m_FileList[ it->FileName ]++;
		else
		{
			for (int i = 0; i < it->GetBlockCount(); ++i)
			{
				m_FileList[ it->Blocks[i].FileName ]++;
			}
		}
	}
}

/////////////////////////////////////////////////////////////////////////////
////////////////////////////// Conversation /////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
bool CSelectionTreeManager::ReadGroupInfo( SSelectionTreeInfo& info, const SXmlHistoryGroup* pGroup ) const
{
	if ( pGroup->GetTypeId() == eSTGTI_Tree )
		return ReadTreeGroupInfo( info, pGroup );
	
	return false;
}

bool CSelectionTreeManager::ReadTreeGroupInfo( SSelectionTreeInfo& info, const SXmlHistoryGroup* pGroup ) const
{	
	if ( pGroup->GetHistoryCount() < 4 ) return false;

	const SXmlHistory* pNameData				= pGroup->GetHistoryByTypeId( eSTTI_NameInfo );
	const SXmlHistory* pTreeData				= pGroup->GetHistoryByTypeId( eSTTI_Tree );
	const SXmlHistory* pVarData					= pGroup->GetHistoryByTypeId( eSTTI_Variables );
	const SXmlHistory* pSigData					= pGroup->GetHistoryByTypeId( eSTTI_Signals );
	const SXmlHistory* pTimestampData		= pGroup->GetHistoryByTypeId( eSTTI_TimeStamp );
	assert( pNameData && pTreeData && pVarData && pSigData && pTimestampData );

	info.Name = pNameData->GetCurrentVersion()->getAttr("name");
	info.FileName = pNameData->GetCurrentVersion()->getAttr("filename");
	info.TreeType = pNameData->GetCurrentVersion()->getAttr("type");
	info.IsLoaded = m_pXmlHistoryMan->GetActiveGroup() == pGroup;
	info.CurrTreeIndex = 0;
	info.IsModified =  pNameData->IsModified()
		|| pTreeData->IsModified()
		|| pVarData->IsModified()
		|| pSigData->IsModified()
		|| pTimestampData->IsModified();
	info.IsTree = true;

	bool ok = true;
	ok &= ReadBlockInfo( info, eSTTI_Variables, pVarData, pNameData->GetCurrentVersion() );
	ok &= ReadBlockInfo( info, eSTTI_Signals, pSigData, pNameData->GetCurrentVersion() );
	ok &= ReadBlockInfo( info, eSTTI_TimeStamp, pTimestampData, pNameData->GetCurrentVersion() );
	ok &= ReadBlockInfo( info, eSTTI_Tree, pTreeData, pNameData->GetCurrentVersion() );
	return ok;
}

/////////////////////////////////////////////////////////////////////////////
bool CSelectionTreeManager::ReadBlockInfo( SSelectionTreeInfo& info, ESelectionTreeTypeId type, const SXmlHistory* pData, const XmlNodeRef& nameInfoNode ) const
{
	assert(nameInfoNode);
	if  ( pData && nameInfoNode )
	{
		SSelectionTreeBlockInfo blockInfo;
		blockInfo.Type = type;
		blockInfo.Name = nameInfoNode->getAttr("name");
		blockInfo.FileName = nameInfoNode->getAttr("filename");
		blockInfo.IsModified = pData->IsModified();
		blockInfo.XmlData = pData->GetCurrentVersion();
		info.Blocks.push_back(blockInfo);
		return true;
	}
	return false;
}

/////////////////////////////////////////////////////////////////////////////
bool CSelectionTreeManager::LoadTreeInfoNode( XmlNodeRef& outNode, const XmlNodeRef& inNode, const char* filename )
{
	if ( inNode && outNode )
	{		
			outNode->setTag( "Info" );
			outNode->setAttr( "name", Path::GetFileName(filename) );
			outNode->setAttr( "type", "BehaviorSelectionTree" );
			outNode->setAttr( "filename", filename );
			return true;
	}
	return false;
}

/////////////////////////////////////////////////////////////////////////////
bool CSelectionTreeManager::LoadBlockInfoNode( XmlNodeRef& outNode, const XmlNodeRef& inNode, int blockindex, const char* filename )
{
	if ( LoadBlockGroupInfoNode( outNode, inNode ) )
	{	
		const char* name = inNode->getAttr( "name" );
		outNode->setTag( string().Format( "info_%i", blockindex ).c_str() );
		outNode->setAttr( "name", name );
		outNode->setAttr( "filename", filename );
		return true;
	}
	return false;
}

/////////////////////////////////////////////////////////////////////////////
bool CSelectionTreeManager::LoadBlockGroupInfoNode( XmlNodeRef& outNode, const XmlNodeRef& inNode )
{
	if ( inNode && outNode )
	{
		const char* name = inNode->getAttr( "name" );
		const char* group = inNode->getAttr( "group" );
		if ( name[0] )
		{
			outNode->setTag( "Info" );
			outNode->setAttr( "name", group[0] ? group : name );
			return true;
		}
	}
	return false;
}

/////////////////////////////////////////////////////////////////////////////
ESelectionTreeTypeId CSelectionTreeManager::GetBlockType( const XmlNodeRef& xmlNode ) const
{
	const char* tag = xmlNode->getTag();
	if ( strcmpi( tag, "Block" ) == 0 )
	{
		for ( int i = 0; i < xmlNode->getChildCount(); ++i )
		{
			tag = xmlNode->getChild( i )->getTag();
			ESelectionTreeTypeId type = GetBlockTypeByStr( tag );
			if (type != eSTTI_Undefined) return type;
		}
	}
	return eSTTI_Undefined;
}

ESelectionTreeTypeId CSelectionTreeManager::GetBlockTypeByStr( const char* name ) const
{
	if ( strcmpi( name, "Variable" ) == 0 ) return eSTTI_Variables;
	if ( strcmpi( name, "Signal" ) == 0 ) return eSTTI_Signals;
	if ( strcmpi( name, "Priority" ) == 0 ) return eSTTI_Tree;
	if ( strcmpi( name, "Timestamp" ) == 0 ) return eSTTI_TimeStamp;
	if ( strcmpi( name, "StateMachine" ) == 0 ) return eSTTI_Tree;
	if ( strcmpi( name, "Sequence" ) == 0 ) return eSTTI_Tree;
	return eSTTI_Undefined;
}

const char* CSelectionTreeManager::GetBlockStrByType(ESelectionTreeTypeId type) const
{
	switch(type)
	{
	case eSTTI_Variables: return "Variables";
	case eSTTI_Signals: return "Signals";
	case eSTTI_Tree: return "Tree";
	case eSTTI_TimeStamp: return "Timestamp";
	}
	return "UNDEFINED";
}

void CSelectionTreeManager::OnBSTDebugAIVariableChanged(ICVar*pCVar)
{
	GetIEditor()->GetSelectionTreeManager()->SetIngame(true);
}

void CSelectionTreeManager::OnBSTValidatorWarnVariableChanged(ICVar*pCVar)
{
	GetIEditor()->GetSelectionTreeManager()->m_pValiator->Validate();
}

void CSelectionTreeManager::SetIngame( bool inGame )
{
	ISelectionTreeDebugger* pDebugger = gEnv->pAISystem->GetSelectionTreeManager()->GetDebugger();
	if (!pDebugger) return;

	pDebugger->SetObserver(NULL,"");
	if (m_pGraph) m_pGraph->UnmarkNodes();
	if (m_pVarView) m_pVarView->SetDebugging(false);

	if (CV_bst_debug_ai)
	{
		CV_bst_debug_ai->SetOnChangeCallback(NULL);
		if (inGame)
		{
			CV_bst_debug_ai->SetOnChangeCallback(&CSelectionTreeManager::OnBSTDebugAIVariableChanged);
			const char* ai = CV_bst_debug_ai->GetString();
			if (strlen(ai) > 0)
			{
				pDebugger->SetObserver(this, ai);
			}
		}
	}
}

void CSelectionTreeManager::SetSelectionTree(const char* name, const STreeNodeInfo& rootNode)
{
	SXmlHistoryGroup* pGroup = GetSelectionTree(name);
	if (pGroup)
	{
		bool bFitToView = false;
		if (pGroup != m_pXmlHistoryMan->GetActiveGroup())
		{
			TGroupIndexMap userIndex;
			m_pXmlHistoryMan->SetActiveGroup(pGroup, name, userIndex, true);
			bFitToView = true;
		}
		if (m_pVarView) m_pVarView->SetDebugging(true);
		if (m_pGraph && m_pGraph->InitDebugTree(rootNode))
		{
			if (bFitToView)
				m_pGraph->GetGraphView()->ResetView();
			return;
		}
	}

	gEnv->pLog->LogWarning("[BST] Failed to debug SelectionTree %s, Tree is not loaded or out of date!", name);
	ISelectionTreeDebugger* pDebugger = gEnv->pAISystem->GetSelectionTreeManager()->GetDebugger();
	if (pDebugger) pDebugger->SetObserver(NULL,"");
	if (m_pGraph) m_pGraph->UnmarkNodes();
}

void CSelectionTreeManager::DumpVars(const TVariableStateMap& vars)
{
	if (m_pVarView)
	{
		for (TVariableStateMap::const_iterator it = vars.begin(); it != vars.end(); ++it)
		{
			m_pVarView->SetVarVal(  it->first.c_str(), it->second );
		}
	}
}

void CSelectionTreeManager::StartEval()
{
	if (m_pGraph)
	{
		m_pGraph->StartEval();
	}
}

void CSelectionTreeManager::EvalNode(uint16 nodeId)
{
	if (m_pGraph)
	{
		m_pGraph->EvalNode(nodeId);
	}
}

void CSelectionTreeManager::EvalNodeCondition(uint16 nodeId, bool condition)
{
	if (m_pGraph)
	{
		m_pGraph->EvalNodeCondition(nodeId, condition);
	}
}

void CSelectionTreeManager::EvalStateCondition(uint16 nodeId, bool condition)
{
	if (m_pGraph)
	{
		m_pGraph->EvalStateCondition(nodeId, condition);
	}
}

void CSelectionTreeManager::StopEval(uint16 nodeId)
{
	if (m_pGraph)
	{
		m_pGraph->StopEval(nodeId);
	}
}


CSelectionTreeErrorReport::TSelectionTreeErrorId CSelectionTreeManager::ReportError(const CSelectionTreeErrorRecord& error, bool displayErrorDialog )
{
	CSelectionTreeErrorReport::TSelectionTreeErrorId id = m_pErrorReport->AddError(error);
	if (displayErrorDialog)
		CSelectionTreeErrorDialog::Open(m_pErrorReport);
	return id;
}

void CSelectionTreeManager::RemoveError(CSelectionTreeErrorReport::TSelectionTreeErrorId id)
{
	m_pErrorReport->RemoveError(id);
}

void CSelectionTreeManager::ReloadErrors()
{
	if (m_pErrorReport->NeedReload())
		CSelectionTreeErrorDialog::Reload();
}

void CSelectionTreeManager::ClearErrors()
{
	m_pErrorReport->Clear();
}

void CSelectionTreeManager::DisplayErrors()
{
	CSelectionTreeErrorDialog::Open(m_pErrorReport);
}

