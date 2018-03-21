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

// Description : Implementation of CPropertyCtrlExt.


#include "stdafx.h"
#include "PropertyCtrlExt.h"

#include "Objects/EntityObject.h"
#include "Objects/SelectionGroup.h"
#include "VehicleData.h"
#include "VehicleXMLHelper.h"

#include <QMouseEvent>

// CPropertyCtrlExt

enum PropertyCtrlExContextual
{
    PCEC_USER_CANCELLED = 0,
    PCEC_ADD_ARRAY_ELEMENT,
    PCEC_REMOVE_ARRAY_ELEMENT,
    PCEC_GET_EFFECT,
    PCEC_REMOVE_OPTIONAL_VAR,
    PCEC_CREATE_OPTIONAL_VAR_FIRST,
    PCEC_CREATE_OPTIONAL_VAR_LAST = PCEC_CREATE_OPTIONAL_VAR_FIRST + 100,
};


CPropertyCtrlExt::CPropertyCtrlExt(QWidget* widget)
    : ReflectedPropertyControl(widget)
    , m_pVehicleVar(NULL)
{
    m_preSelChangeFunc = 0;
}


void CPropertyCtrlExt::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() != Qt::RightButton)
    {
        ReflectedPropertyControl::mouseReleaseEvent(event);
        return;
    }
    ReflectedPropertyItem* pItem = nullptr;
    if (GetSelectedItems().size() == 1)
    {
        pItem = GetSelectedItem();
    }
    else
    {
        pItem = GetRootItem();
        if (pItem != nullptr)
        {
            if (0 < pItem->GetChildCount())
            {
                // The root item doesn't have a variable!
                pItem = pItem->GetChild(0);
            }
        }
    }

    if (pItem == NULL)
    {
        return;
    }

    IVariable* pVar = pItem->GetVariable();
    if (pVar == NULL)
    {
        return;
    }

    QMenu menu;

    if (pVar->GetDataType() == IVariable::DT_EXTARRAY)
    {
        if (static_cast<int>(pVar->GetUserData().type()) == QMetaType::VoidStar)
        {
            menu.addAction(tr("Add '%1'").arg(static_cast< IVariable* >(pVar->GetUserData().value<void *>())->GetName()))->setData(PCEC_ADD_ARRAY_ELEMENT);
        }
    }
    else if (pItem->GetParent() && pItem->GetParent()->GetVariable()
             && pItem->GetParent()->GetVariable()->GetDataType() == IVariable::DT_EXTARRAY)
    {
        // if clicked on child of extendable array element
        menu.addAction(tr("Delete '%1'").arg(pVar->GetName()))->setData(PCEC_REMOVE_ARRAY_ELEMENT);
    }
    else if (QString::compare(pVar->GetName(), "effect", Qt::CaseInsensitive) == 0)
    {
        menu.addAction(tr("Get effect from selection"))->setData(PCEC_GET_EFFECT);
    }

    XmlNodeRef nodeDefinition = VehicleXml::GetXmlNodeDefinitionByVariable(CVehicleData::GetXMLDef(), m_pVehicleVar, pVar);
    if (!nodeDefinition)
    {
        return;
    }

    if (VehicleXml::IsOptionalNode(nodeDefinition))
    {
        if (!VehicleXml::IsArrayElementNode(nodeDefinition, pVar->GetName().toUtf8().data()))
        {
            menu.addAction(tr("Delete '%1'").arg(pVar->GetName()))->setData(PCEC_REMOVE_OPTIONAL_VAR);
        }
    }

    if (!menu.actions().isEmpty())
    {
        // TODO: only add separator if will add elements after that...
        menu.addSeparator();
    }

    std::vector< XmlNodeRef > childDefinitions;
    if (pVar->GetDataType() != IVariable::DT_EXTARRAY)
    {
        VehicleXml::GetXmlNodeChildDefinitionsByVariable(CVehicleData::GetXMLDef(), m_pVehicleVar, pVar, childDefinitions);
        for (int i = 0; i < childDefinitions.size(); ++i)
        {
            XmlNodeRef childProperty = childDefinitions[ i ];
            if (VehicleXml::IsOptionalNode(childProperty))
            {
                QString propertyName = VehicleXml::GetNodeName(childProperty);
                IVariable* pChildVar = GetChildVar(pVar, propertyName.toUtf8().data());
                bool alreadyHasChildProperty = (pChildVar != NULL);
                int contextOptionId = PCEC_CREATE_OPTIONAL_VAR_FIRST + i;

                bool allowCreation = true;
                allowCreation &= !alreadyHasChildProperty;
                allowCreation &= (contextOptionId <= PCEC_CREATE_OPTIONAL_VAR_LAST);
                allowCreation &= !VehicleXml::IsDeprecatedNode(childProperty);

                if (allowCreation)
                {
                    menu.addAction(tr("Add '%1'").arg(propertyName))->setData(contextOptionId);
                }
            }
        }
    }


    QAction* action = menu.exec(QCursor::pos());
    int res = action ? action->data().toInt() : -1;
    switch (res)
    {
    case PCEC_ADD_ARRAY_ELEMENT:
        OnAddChild();
        break;
    case PCEC_REMOVE_ARRAY_ELEMENT:
        OnDeleteChild(pItem);
        break;
    case PCEC_GET_EFFECT:
        OnGetEffect(pItem);
        break;
    case PCEC_REMOVE_OPTIONAL_VAR:
        OnDeleteChild(pItem);
        break;
    }

    if (PCEC_CREATE_OPTIONAL_VAR_FIRST <= res && res < PCEC_CREATE_OPTIONAL_VAR_FIRST + childDefinitions.size() && res <= PCEC_CREATE_OPTIONAL_VAR_LAST)
    {
        IVariablePtr pItemVar = pItem->GetVariable();
        int index = res - PCEC_CREATE_OPTIONAL_VAR_FIRST;

        XmlNodeRef node = childDefinitions[ index ];
        IVariablePtr pChildVar = VehicleXml::CreateDefaultVar(CVehicleData::GetXMLDef(), node, VehicleXml::GetNodeName(node));

        if (pChildVar)
        {
            pItemVar->AddVariable(pChildVar);
            // Make sure the controls are completely rebuilt if we've created a variable.
            ReloadItem(pItem);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CPropertyCtrlExt::ReloadItem(ReflectedPropertyItem* pItem)
{
    IVariablePtr pItemVar = pItem->GetVariable();
    if (!pItemVar)
    {
        return;
    }

    CVarBlockPtr pVarBlock(new CVarBlock());
    for (int i = 0; i < pItemVar->GetNumVariables(); ++i)
    {
        pVarBlock->AddVariable(pItemVar->GetVariable(i));
    }

    ReplaceVarBlock(pItemVar, pVarBlock);
}


//////////////////////////////////////////////////////////////////////////
void CPropertyCtrlExt::OnAddChild()
{
    if (!GetSelectedItems().size() == 1)
    {
        return;
    }

    ReflectedPropertyItem* pItem = GetSelectedItem();
    IVariable* pVar = pItem->GetVariable();
    IVariable* pClone = 0;

    if (static_cast<int>(pVar->GetUserData().type()) == QMetaType::VoidStar)
    {
        pClone = static_cast<IVariable*>(pVar->GetUserData().value<void *>())->Clone(true);

        pVar->AddVariable(pClone);

        ReflectedPropertyItemPtr pNewItem = new ReflectedPropertyItem(this, pItem);
        pNewItem->SetVariable(pClone);

        RebuildCtrl();
    }
}

//////////////////////////////////////////////////////////////////////////
void CPropertyCtrlExt::OnDeleteChild(ReflectedPropertyItem* pItem)
{
    // remove item and variable
    ReflectedPropertyItem* pParent = pItem->GetParent();
    IVariable* pVar = pItem->GetVariable();

    pParent->RemoveChild(pItem);
    pParent->GetVariable()->DeleteVariable(pVar);

    ClearSelection();
}



//////////////////////////////////////////////////////////////////////////
void CPropertyCtrlExt::SelectItem(ReflectedPropertyItem* item)
{
    // call PreSelect callback
    if (m_preSelChangeFunc)
    {
        m_preSelChangeFunc(item);
    }

    ReflectedPropertyControl::SelectItem(item);
}


//////////////////////////////////////////////////////////////////////////
void CPropertyCtrlExt::OnItemChange(ReflectedPropertyItem* item, bool deferCallbacks)
{
    ReflectedPropertyControl::OnItemChange(item, deferCallbacks);

    // handle class-like items that need to insert/remove subitems
    if (item->GetType() == ePropertySelection && item->GetName() == "class")
    {
        IVariable* pVar = item->GetVariable();

        QString classname;
        pVar->Get(classname);

        // get parent var/item of class item
        ReflectedPropertyItem* pClassItem = item;
        if (!pClassItem)
        {
            return;
        }

        ReflectedPropertyItem* pParentItem = pClassItem->GetParent();

        assert(pParentItem);
        if (!pParentItem)
        {
            return;
        }

        if (IVariablePtr pMainVar = pParentItem->GetVariable())
        {
            // first remove child vars that belong to other classes
            if (IVarEnumList* pEnumList = pVar->GetEnumList())
            {
                ReflectedPropertyItemPtr pSubItem = 0;

                for (uint i = 0; !pEnumList->GetItemName(i).isNull(); i++)
                {
                    QString sEnumName = pEnumList->GetItemName(i);
                    // if sub variables for enum classes are found
                    // delete their property items and variables
                    if (IVariable* pSubVar = GetChildVar(pMainVar, sEnumName.toUtf8().data()))
                    {
                        pSubItem = pParentItem->findItem(pSubVar);
                        if (pSubItem)
                        {
                            pParentItem->RemoveChild(pSubItem);
                            pMainVar->DeleteVariable(pSubVar);
                        }
                    }
                }

                // now check if we have extra properties for that part class
                if (IVariable* pSubProps = CreateDefaultVar(classname.toUtf8().data(), true))
                {
                    // add new PropertyItem
                    pSubItem = new ReflectedPropertyItem(this, pParentItem);
                    pSubItem->SetVariable(pSubProps);

                    pMainVar->AddVariable(pSubProps);

                    Expand(pSubItem, true);
                }
                RebuildCtrl();
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CPropertyCtrlExt::OnGetEffect(ReflectedPropertyItem* pItem)
{
    CSelectionGroup* pSel = GetIEditor()->GetSelection();
    for (int i = 0; i < pSel->GetCount(); ++i)
    {
        CBaseObject* pObject = pSel->GetObject(i);
        if (qobject_cast<CEntityObject*>(pObject))
        {
            CEntityObject* pEntity = (CEntityObject*)pObject;
            if (pEntity->GetProperties())
            {
                IVariable* pVar = pEntity->GetProperties()->FindVariable("ParticleEffect");
                if (pVar)
                {
                    QString effect;
                    pVar->Get(effect);

                    pItem->SetValue(effect);

                    break;
                }
            }
        }
    }
}

#include <Vehicles/PropertyCtrlExt.moc>