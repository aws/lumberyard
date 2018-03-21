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
#include "VehicleMovementPanel.h"

#include "VehicleData.h"
#include "VehicleEditorDialog.h"
#include "VehiclePrototype.h"

#include <QBoxLayout>

CVehicleMovementPanel::CVehicleMovementPanel(CVehicleEditorDialog* pDialog)
    : QWidget(pDialog)
    , m_pVehicle(0)
    , m_propsCtrl(this)
{
    m_propsCtrl.Setup();
    assert(pDialog); // dialog is needed
    m_pDialog = pDialog;

    m_pMovementBlock = new CVarBlock;
    m_pPhysicsBlock = new CVarBlock;
    m_pTypeBlock = new CVarBlock;

    setLayout(new QHBoxLayout);
    layout()->addWidget(&m_propsCtrl);
    m_propsCtrl.ExpandAll();
}

CVehicleMovementPanel::~CVehicleMovementPanel()
{
}

void CVehicleMovementPanel::ExpandProps(ReflectedPropertyItem* pItem, bool expand /*=true*/)
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


void CVehicleMovementPanel::OnMovementTypeChange(IVariable* pVar)
{
    QMetaObject::invokeMethod(this, "DoOnMovementTypeChange", Qt::QueuedConnection, Q_ARG(IVariable*, pVar));
}

void CVehicleMovementPanel::DoOnMovementTypeChange(IVariable* pVar)
{
    // ask if user is sure? :)

    m_pMovementBlock->DeleteAllVariables();

    // load defaults
    const IVariable* pDefaults = CVehicleData::GetDefaultVar();
    IVariable* pRoot = m_pVehicle->GetVariable();

    if (IVariable* pParams = GetChildVar(pDefaults, "MovementParams"))
    {
        QString type;
        pVar->Get(type);
        Log("looking up defaults for movement type %s", type.toUtf8().data());
        if (IVariable* pType = GetChildVar(pParams, type.toUtf8().data()))
        {
            IVariable* pNewType = pType->Clone(true);

            // fetch property item holding the type var
            IVariable* pMoveParams = GetChildVar(pRoot, "MovementParams");
            ReflectedPropertyItemPtr pTypeItem = 0;
            if (pMoveParams && pMoveParams->GetNumVariables() > 0)
            {
                // delete old var/item
                IVariable* pOldType = pMoveParams->GetVariable(0);
                pTypeItem = m_propsCtrl.GetRootItem()->findItem(pMoveParams)->findItem(pOldType);
                assert(pTypeItem != NULL);

                pTypeItem->RemoveAllChildren();
                pMoveParams->DeleteVariable(pOldType);
            }
            else
            {
                pTypeItem = new ReflectedPropertyItem(&m_propsCtrl, m_propsCtrl.GetRootItem()->findItem(pMoveParams));
            }
            pTypeItem->SetVariable(pNewType);
            if (pMoveParams)
            {
                pMoveParams->AddVariable(pNewType);
            }

            // expand "Wheeled" subtable if present
            if (IVariable* pWheeledVar = GetChildVar(pNewType, "Wheeled"))
            {
                m_propsCtrl.Expand(pTypeItem->findItem(pWheeledVar), true);
            }
        }
    }

    m_propsCtrl.RebuildCtrl();
}

void CVehicleMovementPanel::UpdateVehiclePrototype(CVehiclePrototype* pProt)
{
    assert(pProt);
    m_pVehicle = pProt;
    m_propsCtrl.RemoveAllItems();
    m_propsCtrl.SetPreSelChangeCallback(functor(*m_pDialog, &CVehicleEditorDialog::OnPropsSelChanged));

    m_pTypeBlock->DeleteAllVariables();
    m_pMovementBlock->DeleteAllVariables();
    m_pPhysicsBlock->DeleteAllVariables();

    // get data from root var
    IVariable* pRoot = pProt->GetVariable();

    // add enum for selecting movement type
    CVariableEnum<QString>* pTypeVar = new CVariableEnum<QString>();
    pTypeVar->SetName("Type");

    {
        // fetch available movement types
        const XmlNodeRef& def = CVehicleData::GetXMLDef();
        XmlNodeRef node;
        for (int i = 0; i < def->getChildCount(); ++i)
        {
            node = def->getChild(i);
            if (0 == strcmp("MovementParams", node->getAttr("name")))
            {
                XmlNodeRef moveNode;
                for (int j = 0; j < node->getChildCount(); j++)
                {
                    QString name = node->getChild(j)->getAttr("name");
                    pTypeVar->AddEnumItem(name, name);
                }
                break;
            }
        }

        m_pTypeBlock->AddVariable(pTypeVar, NULL);

        ReflectedPropertyItem* pItem = m_propsCtrl.AddVarBlock(m_pTypeBlock);
        ExpandProps(pItem);
    }

    // add movementParams
    if (IVariable* pVar = GetChildVar(pRoot, "MovementParams"))
    {
        m_pMovementBlock->AddVariable(pVar, "MovementParams");
        ReflectedPropertyItem* pItem = m_propsCtrl.AddVarBlock(m_pMovementBlock);
        ExpandProps(pItem);

        // set type enum to actual type
        if (pVar->GetNumVariables() > 0)
        {
            QString sType = pVar->GetVariable(0)->GetName();
            pTypeVar->Set(sType);

            // expand "Wheeled" subtable if present
            if (IVariable* pWheeledVar = GetChildVar(pVar->GetVariable(0), "Wheeled"))
            {
                m_propsCtrl.Expand(pItem->findItem(pWheeledVar), true);
            }
        }
    }

    if (IVariable* pVar = GetChildVar(pRoot, "Physics"))
    {
        m_pPhysicsBlock->AddVariable(pVar, NULL);
        ReflectedPropertyItem* pItem = m_propsCtrl.AddVarBlock(m_pPhysicsBlock);
        ExpandProps(pItem);
    }

    m_pTypeBlock->AddOnSetCallback(functor(*this, &CVehicleMovementPanel::OnMovementTypeChange));

    m_propsCtrl.RebuildCtrl();
}

#include <Vehicles/VehicleMovementPanel.moc>