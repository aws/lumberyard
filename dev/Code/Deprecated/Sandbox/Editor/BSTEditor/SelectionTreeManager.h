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

#ifndef CRYINCLUDE_EDITOR_BSTEDITOR_SELECTIONTREEMANAGER_H
#define CRYINCLUDE_EDITOR_BSTEDITOR_SELECTIONTREEMANAGER_H
#pragma once


#include "Util/IXmlHistoryManager.h"
#include "LevelIndependentFileMan.h"
#include <ISelectionTreeManager.h>
#include "SelectionTreeErrorReport.h"
#include "Views/SelectionTreeTimestampsView.h"
#include "Views/SelectionTreePropertiesView.h"

class CSelectionTreeModifier;
class CSelectionTreeVariablesView;
class CSelectionTreeTimestampView;
class CSelectionTreeSignalsView;
class CSelectionTreeGraph;
class CSelectionTreeValidator;

// Tree data
enum ESelectionTreeGroupTypeId
{
	eSTGTI_Tree = 0,
};

enum ESelectionTreeTypeId
{
	eSTTI_Undefined = -1,
	eSTTI_All = eSTTI_Undefined,
	eSTTI_NameInfo = 0,
	eSTTI_Variables,
	eSTTI_Signals,
	eSTTI_LeafTranslations,
	eSTTI_TimeStamp,
	eSTTI_Properties,
	eSTTI_Tree,
};

struct SSelectionTreeBlockInfo
{
	SSelectionTreeBlockInfo();

	const char* Name;
	const char* FileName;
	bool IsModified;
	ESelectionTreeTypeId Type;

	XmlNodeRef XmlData;
};

typedef std::vector<SSelectionTreeBlockInfo> TBlockInfoList;

struct SSelectionTreeInfo
{
	SSelectionTreeInfo();

	const char* Name;
	const char* FileName;
	const char* TreeType;
	bool IsModified;
	bool IsLoaded;
	bool IsTree;
	int CurrTreeIndex;
	int GetBlockCount() const {return Blocks.size();}
	bool GetBlockById( SSelectionTreeBlockInfo& out, ESelectionTreeTypeId typeId, int index = 0 ) const;
	int GetBlockCountById( ESelectionTreeTypeId typeId ) const;

	TBlockInfoList Blocks;
};
typedef std::list< SSelectionTreeInfo > TSelectionTreeInfoList;

class CXmlHistoryManager;
struct SXmlHistoryGroup;
struct SXmlHistory;

class CSelectionTreeManager
	: public IXmlHistoryEventListener
	, public ILevelIndependentFileModule
	, public ISelectionTreeObserver
{
public:
	CSelectionTreeManager();
	~CSelectionTreeManager();

	// Init
	void Init();

	// History
	IXmlHistoryManager* GetHistory();

	// load / save
	void LoadFromFolder( const char* folder );
	bool SaveAll();

	// info
	void GetInfoList( TSelectionTreeInfoList& infoList ) const;
	bool GetTreeInfoByName(SSelectionTreeInfo& out, const char* name) const;
	bool GetCurrentTreeInfo(SSelectionTreeInfo& out, int* pIndex = NULL) const;

	// change views
	void DisplayTree( const char* name );
	void UnloadViews();

	// add/delete/rename tree
	struct STreeDefinition
	{
		const char* name;
		const char* filename;
	};

	bool AddNewTree( const STreeDefinition& definition );
	bool DeleteTree( const char* name );
	bool EditTree( const char* name, const STreeDefinition& definition );

	// add/delete/rename refs
	struct SRefGroupDefinition
	{
		const char* name;
		const char* filename;
		const char* varsname;
		const char* signalsname;
		const char* treename;
		bool vars;
		bool tree;
	};
		
	// Reset all data
	void Reset();

	CSelectionTreeModifier* GetModifier() const { return m_pModifier; }
	// IXmlHistoryEventListener
	virtual void OnEvent( EHistoryEventType event, void* pData = NULL );
	// ~IXmlHistoryEventListener

	// ILevelIndependentFileModule
	virtual bool PromptChanges();
	// ~ILevelIndependentFileModule

	void SetIngame( bool inGame );
	// ISelectionTreeObserver
	virtual void SetSelectionTree(const char* name, const STreeNodeInfo& rootNode);
	virtual void DumpVars(const TVariableStateMap& vars);
	virtual void StartEval();
	virtual void EvalNode(uint16 nodeId);
	virtual void EvalNodeCondition(uint16 nodeId, bool condition);
	virtual void EvalStateCondition(uint16 nodeId, bool condition);
	virtual void StopEval(uint16 nodeId);
	// ~ISelectionTreeObserver

	// CVars
	static ICVar* CV_bst_debug_ai;
	static int CV_bst_debug_centernode;

	enum EWarnLevel
	{
		eWL_All = 0,
		eWL_NoWarningsInRef,
		eWL_NoErrorsInRef,
		eWL_OnlyErrors,
		eWL_OnlyErrorsInTree,
	};
	static int CV_bst_validator_warnlevel;

	static void	OnBSTDebugAIVariableChanged(ICVar*pCVar);
	static void	OnBSTValidatorWarnVariableChanged(ICVar*pCVar);

	// Views
	void SetVarView(CSelectionTreeVariablesView* pVarView) { m_pVarView = pVarView; }
	void SetSigView(CSelectionTreeSignalsView* pSigView) { m_pSigView = pSigView; }
	void SetTimestampView(CSelectionTreeTimestampsView* pTimestampView) { m_pTimestampView = pTimestampView; }
	void SetPropertiesView(CSelectionTreePropertiesView* pPropertiesView) { m_pPropertiesView = pPropertiesView; }
	void SetTreeGraph(CSelectionTreeGraph* pGraph) { m_pGraph = pGraph; }

	CSelectionTreeVariablesView* GetVarView() const { return m_pVarView; };
	CSelectionTreeSignalsView* GetSigView() const { return m_pSigView; };
	CSelectionTreePropertiesView* GetPropertiesView() const { return m_pPropertiesView; };
	CSelectionTreeTimestampsView* GetTimestampView() const { return m_pTimestampView; };
	CSelectionTreeGraph* GetTreeGraph() const { return m_pGraph; };

	// Error Report
	CSelectionTreeErrorReport::TSelectionTreeErrorId ReportError(const CSelectionTreeErrorRecord& error, bool displayErrorDialog = false);
	void RemoveError(CSelectionTreeErrorReport::TSelectionTreeErrorId id);
	void ReloadErrors();
	void ClearErrors();
	void DisplayErrors();

private:
	SXmlHistoryGroup* GetSelectionTree( const char* name, string* displayName = NULL ) const;
	
	void LoadFromFolderInt( const char* folder );
	void LoadFromFileNode( const XmlNodeRef& node, const char* filename );
	void LoadTreeXml( const XmlNodeRef& tree, const char* filename );
	void UpdateFileList(bool reset = false);

	bool ReadGroupInfo( SSelectionTreeInfo& info, const SXmlHistoryGroup* pGroup ) const;
	bool ReadTreeGroupInfo( SSelectionTreeInfo& info, const SXmlHistoryGroup* pGroup ) const;
	bool LoadTreeInfoNode( XmlNodeRef& outNode, const XmlNodeRef& inNode, const char* filename );
	bool ReadBlockInfo( SSelectionTreeInfo& info, ESelectionTreeTypeId type, const SXmlHistory* pData, const XmlNodeRef& nameInfoNode ) const;
	bool LoadBlockInfoNode( XmlNodeRef& outNode, const XmlNodeRef& inNode, int blockindex, const char* filename );
	bool LoadBlockGroupInfoNode( XmlNodeRef& outNode, const XmlNodeRef& inNode );

public:
	ESelectionTreeTypeId GetBlockType( const XmlNodeRef& xmlNode ) const;
	ESelectionTreeTypeId GetBlockTypeByStr( const char* name ) const;
	const char* GetBlockStrByType(ESelectionTreeTypeId type) const;

private:
	CXmlHistoryManager* m_pXmlHistoryMan;
	CSelectionTreeModifier* m_pModifier;
	CSelectionTreeVariablesView* m_pVarView;
	CSelectionTreeSignalsView* m_pSigView;
	CSelectionTreeTimestampsView* m_pTimestampView;
	CSelectionTreePropertiesView* m_pPropertiesView;
	CSelectionTreeGraph* m_pGraph;
	CSelectionTreeErrorReport* m_pErrorReport;
	CSelectionTreeValidator* m_pValiator;
	bool m_bIsLoading;

	typedef std::vector< SXmlHistoryGroup* > TXmlGroupList;
	TXmlGroupList m_XmlGroupList;

	typedef std::pair< string, XmlNodeRef > TXmlNodeSet;
	typedef std::list< TXmlNodeSet >TXmlNodeList;
	TXmlNodeList m_BlocksToResolve;

	typedef std::map< string, int > TFileCountList;
	TFileCountList m_FileList;
};

template<class T>
class CMakeNameUnique
{
public:
	typedef bool (T::*TValidateNameFct) ( const string& name );

	CMakeNameUnique( T* pThis, TValidateNameFct fct ) : m_pThis( pThis), m_Fct(fct) {}

	void MakeNameUnique( string& name )
	{
		if ( m_pThis && m_Fct )
		{
			MakeNameUniqueInt( name, 0 );
		}
	}

private:
	T* m_pThis;
	TValidateNameFct m_Fct;

	void MakeNameUniqueInt( string& name, int appendix )
	{
		string uniqueName = name;
		if ( appendix > 0 )
		{
			uniqueName.Format( "%s_%i", name, appendix );
		}

		if ( ( m_pThis->*m_Fct )( uniqueName ) )
		{
			name = uniqueName;
		}
		else
		{
			MakeNameUniqueInt( name, appendix + 1 );
		}
	}
};

#endif // CRYINCLUDE_EDITOR_BSTEDITOR_SELECTIONTREEMANAGER_H
