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

#ifndef CRYINCLUDE_EDITOR_BSTEDITOR_GRAPH_NODES_SELECTIONTREE_BASENODE_H
#define CRYINCLUDE_EDITOR_BSTEDITOR_GRAPH_NODES_SELECTIONTREE_BASENODE_H
#pragma once


#include "HyperGraph/HyperGraphNode.h"

#include "SelectionTree_BaseNode_Painter.h"

#define MBT_LOGGING_NODE "MbtLoggingFunctions"


class CSelectionTreeGraph;

class CSelectionTree_BaseNode
	: public CHyperNode
	, public IEditorNotifyListener
{
public:
	typedef enum
	{
		eSTBS_None = 0,
		eSTBS_Red,
		eSTBS_Yellow,
		eSTBS_Green,
		eSTBS_Blue,

		eSTBS_COUNT,
	} BackgroundState;

	CSelectionTree_BaseNode(  );
	virtual ~CSelectionTree_BaseNode();

	// CHyperNode
	virtual void Init();
	virtual void Done();
	virtual CHyperNode* Clone();
	virtual bool IsEditorSpecialNode() { return true; }
	virtual void SetSelected( bool bSelected );
	virtual void Serialize( XmlNodeRef &node,bool bLoading,CObjectArchive* ar=0 );

	// CSelectionTree_BaseNode interface
	virtual bool LoadFromXml( const XmlNodeRef& xmlNode );
	virtual bool SaveToXml( XmlNodeRef& xmlNode );
	virtual bool AcceptChild( CSelectionTree_BaseNode* pChild ) { return false; }
	virtual bool DefaultEditDialog( bool bCreate = false, bool* pHandled = NULL, string* pUndoDesc = NULL ) { if(pHandled) *pHandled = false; return false; }
	virtual bool OnContextMenuCommandEx( int nCmd, string& undoDesc ) { return false; }

	// visible/read-only properties
	void SetVisible( bool bVisible ) { m_bIsVisible = bVisible; Invalidate( true ); }
	bool IsVisible() const { return m_bIsVisible; }
	
	bool IsReadOnly() const { return m_bReadOnly; }
	void SetReadOnly( bool bReadOnly ) { m_bReadOnly = bReadOnly; }
	void SetTreeReadOnly( bool bReadOnly );
	virtual void Lock();
	virtual void Unlock();

	// leaf translations
	BackgroundState GetBackground() const { return m_Background; }
	void SetBackground(BackgroundState background) { m_Background = background; Invalidate(true); }
	virtual void UpdateTranslations();

	// Tree structure
	void SetParentNode( CSelectionTree_BaseNode* pParent );
	CSelectionTree_BaseNode* GetParentNode();
	const CSelectionTree_BaseNode* GetParentNode() const;

	size_t GetChildNodeCount() const;
	CSelectionTree_BaseNode* GetChildNode( const size_t childIndex );
	const CSelectionTree_BaseNode* GetChildNode( const size_t childIndex ) const;
	void AddChildNode( CSelectionTree_BaseNode* pChild );
	void RemoveChildNode( CSelectionTree_BaseNode* pChild );

	int GetOrder() const { return m_iOrder; }
	bool IsRoot() const { return GetParentNode() == NULL; }

	// auto arrange
	virtual void SetPos( Gdiplus::PointF pos );
	void InvalidateNodePos( bool bForceNotSmooth = false );

	float GetNodeWidth() const { return m_fNodeWidth; }
	void SetNodeWidth( float width ) { m_fNodeWidth = width; }

	float GetNodeHeight() const { return m_fNodeHeight; }
	void SetNodeHeight( float height ) { m_fNodeHeight = height; }

	float GetChildNodesWidth() const;

	float GetTreeWidth() const { return m_fTreeWidth; }
	float GetTreeHeight() const { return m_fTreeHeight; }

	// remove this node and all childs from graph
	void RemoveFromGraph();

	// IEditorNotifyListener
	virtual void OnEditorNotifyEvent( EEditorNotifyEvent event );

	XmlNodeRef GetAttributes() { return m_attributes; }
	void SetAttributes(XmlNodeRef attributes)  {m_attributes = attributes->clone(); }

	virtual void PropertyChanged(XmlNodeRef var);
	virtual void FillPropertyTable(XmlNodeRef var);
	virtual bool ShouldAppendLoggingProps() {return true;}

protected:
	typedef enum 
	{
		ePCT_NoConnectionAllowed,
		ePCT_SingleConnectionAllowed,
		ePCT_MultiConnectionAllowed
	} PortConnectionType;

	void CreateDefaultInPort( const PortConnectionType connectionType );
	void CreateDefaultOutPort( const PortConnectionType connectionType );

	CSelectionTreeGraph* GetTreeGraph();

	bool LoadChildrenFromXml( const XmlNodeRef& xmlNode );
	bool SaveChildrenToXml( XmlNodeRef& xmlNode );
	void ClearChildNodes();

private:
	typedef enum
	{
		ePT_Input,
		ePT_Output
	} PortType;

	void CreateDefaultPort( const CString& portName, const PortType portType, const PortConnectionType connectionType );

	void UpdateChildOrder();

	void RecalculateTreeSize();
	void UpdatePosition( bool bSmooth, float px = 0, float py = 0 );

	bool CanSetPos();

private:
	int m_iOrder;
	float m_fTreeWidth;
	float m_fTreeHeight;
	float m_fNodeWidth;
	float m_fNodeHeight;
	bool m_bIsVisible;
	bool m_bReadOnly;
	BackgroundState m_Background;
	CSelectionTree_BaseNode* m_pParent;
	typedef std::vector< CSelectionTree_BaseNode* > ChildrenList;
	ChildrenList m_children;
	Gdiplus::PointF m_DestPos;
	float m_fLerp;
	bool m_bRegistered;
protected:
	XmlNodeRef m_attributes;

	static CSelectionTree_BaseNode_Painter ms_painter;

};

typedef std::list< CSelectionTree_BaseNode* > TBSTNodeList;

#endif // CRYINCLUDE_EDITOR_BSTEDITOR_GRAPH_NODES_SELECTIONTREE_BASENODE_H
