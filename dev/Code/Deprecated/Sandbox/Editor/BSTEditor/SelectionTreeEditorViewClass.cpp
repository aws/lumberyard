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

#include "stdafx.h"
#include "BSTEditor/SelectionTreeEditor.h"

class CSelectionTreeEditorViewClass : public CViewPaneClass
{
	//////////////////////////////////////////////////////////////////////////
	// IClassDesc
	//////////////////////////////////////////////////////////////////////////
	virtual ESystemClassID SystemClassID() { return ESYSTEM_CLASS_VIEWPANE; }
	virtual REFGUID ClassID()
	{
		// {83794077-181D-4aff-A353-C565E75D0A21}
		static const GUID guid = { 0x83794077, 0x181d, 0x4aff, { 0xa3, 0x53, 0xc5, 0x65, 0xe7, 0x5d, 0xa, 0x21 } };
		return guid;
	}
	virtual const char* ClassName() { return "Modular BehaviorTree Editor"; }
	virtual const char* Category() { return "Modular BehaviorTree Editor"; }
	//////////////////////////////////////////////////////////////////////////

	virtual CRuntimeClass* GetRuntimeClass() { return RUNTIME_CLASS( CSelectionTreeEditor ); }
	virtual unsigned int GetPaneTitleID() const { return IDS_MODULAR_BEHAVIORTREE_EDITOR_TITLE; }
	virtual EDockingDirection GetDockingDirection() { return DOCK_FLOAT; }
	virtual CRect GetPaneRect() { return CRect( 200, 200, 800, 700 ); }
	virtual CSize GetMinSize() { return CSize(1000,800); }
	virtual bool SinglePane() { return false; }
	virtual bool WantIdleUpdate() { return true; }
};

#ifdef USING_SELECTION_TREE_EDITOR
REGISTER_CLASS_DESC( CSelectionTreeEditorViewClass )
#endif // USING_SELECTION_TREE_EDITOR