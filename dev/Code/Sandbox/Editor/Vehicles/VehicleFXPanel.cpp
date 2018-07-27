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
#include "VehicleFXPanel.h"

#include "VehicleData.h"
#include "VehicleEditorDialog.h"
#include "VehiclePrototype.h"

#include <QBoxLayout>

CVehicleFXPanel::CVehicleFXPanel(CVehicleEditorDialog* pDialog)
    : QWidget(pDialog)
    , m_pVehicle(0)
    , m_propsCtrl(this)
{
    m_propsCtrl.Setup();
    assert(pDialog); // dialog is needed
    m_pDialog = pDialog;

    setLayout(new QHBoxLayout);
    layout()->addWidget(&m_propsCtrl);
}

CVehicleFXPanel::~CVehicleFXPanel()
{
}


//////////////////////////////////////////////////////////////////////////
void CVehicleFXPanel::ExpandProps(ReflectedPropertyItem* pItem, bool expand /*=true*/)
{
    // expand all children and their children
    for (int i = 0; i < pItem->GetChildCount(); ++i)
    {
        ReflectedPropertyItem* pChild = pItem->GetChild(i);
        m_propsCtrl.Expand(pChild, true);

        for (int j = 0; j < pChild->GetChildCount(); ++j)
        {
            m_propsCtrl.Expand(pChild->GetChild(j), true);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CVehicleFXPanel::AddCategory(IVariable* pVar)
{
    if (!pVar)
    {
        return;
    }

    CVarBlock* block = new CVarBlock;
    block->AddVariable(pVar);
    ReflectedPropertyItem* pItem = m_propsCtrl.AddVarBlock(block);
    ExpandProps(pItem);
}

//////////////////////////////////////////////////////////////////////////
void CVehicleFXPanel::UpdateVehiclePrototype(CVehiclePrototype* pProt)
{
    assert(pProt);
    m_pVehicle = pProt;
    m_propsCtrl.RemoveAllItems();
    m_propsCtrl.SetPreSelChangeCallback(functor(*m_pDialog, &CVehicleEditorDialog::OnPropsSelChanged));

    // get data from root var
    IVariablePtr pRoot = pProt->GetVariable();

    // add Particles table
    if (IVariable* pVar = GetChildVar(pRoot, "Particles"))
    {
        CVehicleData::FillDefaults(pVar, "Particles");

        for (int i = 0; i < pVar->GetNumVariables(); ++i)
        {
            AddCategory(pVar->GetVariable(i));
        }
    }
}

#include <Vehicles/VehicleFXPanel.moc>