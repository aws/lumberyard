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
#include "SelectionTreeModifier.h"

#include "SelectionTreeManager.h"

#include "CustomMessageBox.h"
#include "Dialogs/AddNewTreeDialog.h"
#include "Dialogs/AddNewBlockGroupDialog.h"
#include "Dialogs/DialogCommon.h"

#include "Views/SelectionTreeVariablesView.h"
#include "Views/SelectionTreeSignalsView.h"

CSelectionTreeModifier::CSelectionTreeModifier(CSelectionTreeManager* pManager)
: m_pManager(pManager)
{

}

CSelectionTreeModifier::~CSelectionTreeModifier()
{

}

void CSelectionTreeModifier::ShowErrorMsg(const char* title, const char* textFormat, ...)
{
	va_list args;	
	va_start( args, textFormat );
	char tmp[5096];
	vsnprintf_s( tmp, 5096, 5095, textFormat, args );
	va_end( args );

	CString error( tmp );
	CCustomMessageBox::Show( error, title, "OK" );
}

CSelectionTreeModifier::EMsgBoxResult CSelectionTreeModifier::ShowMsgBox(const char* title, const char* userButton1, const char* userButton2, const char* textFormat, ...)
{
	va_list args;	
	va_start( args, textFormat );
	char tmp[5096];
	vsnprintf_s( tmp, 5096, 5095, textFormat, args );
	va_end( args );

	CString msg( tmp );
	int idx = CCustomMessageBox::Show( msg, title, "Cancel" /*idx 1*/, userButton2 /*idx 2*/, userButton1 /*idx 3*/);
	switch (idx)
	{
	case 1: return eMBR_Cancel;
	case 2: return eMBR_UserBtn2;
	case 3: return eMBR_UserBtn1;
	}
	return eMBR_Cancel;
}

void CSelectionTreeModifier::CreateNewTree()
{
	CAddNewTreeDialog newDlg;
	newDlg.Init( "New Selection Tree", "NewTree" );
	if(newDlg.DoModal() == IDOK)
	{
		CSelectionTreeManager::STreeDefinition def;
		def.name = newDlg.GetName().c_str();
		string newFile = string().Format("Scripts\\AI\\BehaviorTrees\\%s.xml", def.name);
		def.filename = newFile.c_str();
		if (strlen(def.name) == 0)
		{
			CString error;
			error.Format( _T("Tree \"%s\" could not created, invalid name!" ), def.name );
			CCustomMessageBox::Show( error,_T("Error: Faild to create new SelectionTree!"), "OK" );
			return;
		}
		if (!m_pManager->AddNewTree( def ))
		{
			CString error;
			error.Format( _T("Tree \"%s\" could not created, tree with same name alredy exist!" ), def.name );
			CCustomMessageBox::Show( error,_T("Error: Faild to create new SelectionTree!"), "OK" );
		}
	}
}

void CSelectionTreeModifier::EditTree(const char* tree)
{
	CAddNewTreeDialog newDlg;

	SSelectionTreeInfo info;
	if (m_pManager->GetTreeInfoByName(info, tree))
	{
		newDlg.Init( "Edit Selection Tree", info.Name);
		if(newDlg.DoModal() == IDOK)
		{
			CSelectionTreeManager::STreeDefinition def;
			def.name = newDlg.GetName().c_str();
			string newFile = string().Format("Scripts\\AI\\BehaviorTrees\\%s.xml", def.name);
			def.filename = newFile.c_str();
			if (strlen(def.name) == 0)
			{
				CString error;
				error.Format( _T("Tree \"%s\" could not edited, invalid name!" ), def.name );
				CCustomMessageBox::Show( error,_T("Error: Faild to edit new SelectionTree!"), "OK" );
				return;
			}
			if (!m_pManager->EditTree( info.Name, def ))
			{
				CString error;
				error.Format( _T("Tree \"%s\" could not be renamed, tree with same name alredy exist!" ), def.name );
				CCustomMessageBox::Show( error,_T("Error: Faild to rename SelectionTree!"), "OK" );
			}
		}
		return;
	}
	assert(false);
}