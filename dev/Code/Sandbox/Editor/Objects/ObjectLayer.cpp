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
#include "GameEngine.h"
#include "HyperGraph/FlowGraphManager.h"
#include "ObjectLayerManager.h"

#define LAYER_ID(x) ((x) >> 16)
#define OBJECT_ID(x) ((x) & 0xFFFF)
#define MAKE_ID(layerId, objectId) (((layerId) << 16) | (objectId))

const char* kExternalLayerPathDelimiter = "/";



//////////////////////////////////////////////////////////////////////////
// class CUndoLayerStates
class CUndoLayerStates
    : public IUndoObject
{
public:
    CUndoLayerStates(CObjectLayer* pLayer)
    {
        m_undoIsVisible = pLayer->IsVisible();
        m_undoIsFrozen = pLayer->IsFrozen();
        m_layerGUID = pLayer->GetGUID();
    }
protected:
    virtual int GetSize() { return sizeof(*this); }
    virtual QString GetDescription() { return "Set Layer State"; }

    virtual void Undo(bool bUndo)
    {
        CObjectLayer* pLayer = GetIEditor()->GetObjectManager()->GetLayersManager()->FindLayer(m_layerGUID);
        if (pLayer)
        {
            if (bUndo)
            {
                m_redoIsVisible = pLayer->IsVisible();
                m_redoIsFrozen = pLayer->IsFrozen();
            }

            pLayer->SetVisible(m_undoIsVisible);
            pLayer->SetFrozen(m_undoIsFrozen);
        }
    }

    virtual void Redo()
    {
        CObjectLayer* pLayer = GetIEditor()->GetObjectManager()->GetLayersManager()->FindLayer(m_layerGUID);
        if (pLayer)
        {
            pLayer->SetVisible(m_redoIsVisible);
            pLayer->SetFrozen(m_redoIsFrozen);
        }
    }

private:
    bool m_undoIsVisible;
    bool m_redoIsVisible;
    bool m_undoIsFrozen;
    bool m_redoIsFrozen;
    GUID m_layerGUID;
};



//////////////////////////////////////////////////////////////////////////
// class CUndoLayerName
class CUndoLayerName
    : public IUndoObject
{
public:
    CUndoLayerName(CObjectLayer* pLayer)
    {
        m_undoName = pLayer->GetName();
        m_layerGUID = pLayer->GetGUID();
    }
protected:
    virtual int GetSize() { return sizeof(*this); }
    virtual QString GetDescription() { return "Change Layer Name"; }

    virtual void Undo(bool bUndo)
    {
        CObjectLayer* pLayer = GetIEditor()->GetObjectManager()->GetLayersManager()->FindLayer(m_layerGUID);
        if (pLayer)
        {
            if (bUndo)
            {
                m_redoName = pLayer->GetName();
            }

            pLayer->SetName(m_undoName, true);
            GetIEditor()->GetObjectManager()->GetLayersManager()->NotifyLayerChange(pLayer);
        }
    }

    virtual void Redo()
    {
        CObjectLayer* pLayer = GetIEditor()->GetObjectManager()->GetLayersManager()->FindLayer(m_layerGUID);
        if (pLayer)
        {
            pLayer->SetName(m_redoName, true);
            GetIEditor()->GetObjectManager()->GetLayersManager()->NotifyLayerChange(pLayer);
        }
    }

private:
    QString m_undoName;
    QString m_redoName;
    GUID m_layerGUID;
};



//////////////////////////////////////////////////////////////////////////
// class CObjectLayer

//////////////////////////////////////////////////////////////////////////
CObjectLayer::CObjectLayer(const GUID* pGUID)
{
    m_hidden = false;
    m_frozen = false;
    m_parent = NULL;
    m_external = true;
    m_exportable = true;
    m_exportLayerPak = true;
    m_defaultLoaded = false;
    m_expanded = false;
    m_havePhysics = true;
    m_isModified = false;
    m_nLayerId = 0;
    m_color = QPalette().color(QPalette::Button);
    m_isDefaultColor = true;
    m_specs = eSpecType_All;
    ZeroStruct(m_parentGUID);

    if (pGUID)
    {
        m_guid = *pGUID;
    }
    else
    {
        BuildGuid();
    }
}

void CObjectLayer::BuildGuid()
{
    m_guid = QUuid::createUuid();
}

//////////////////////////////////////////////////////////////////////////
void CObjectLayer::SerializeBase(XmlNodeRef& node, bool bLoading)
{
    if (bLoading)
    {
        node->getAttr("Name", m_name);
        node->getAttr("GUID", m_guid);
        node->getAttr("Hidden", m_hidden);
        node->getAttr("Frozen", m_frozen);
        node->getAttr("External", m_external);
        node->getAttr("Expanded", m_expanded);
    }
    else // save
    {
        node->setAttr("Name", m_name.toUtf8().data());
        node->setAttr("FullName", GetFullName().toUtf8().data());
        node->setAttr("GUID", m_guid);
        node->setAttr("Hidden", m_hidden);
        node->setAttr("Frozen", m_frozen);
        node->setAttr("Expanded", m_expanded);
        node->setAttr("External", m_external);
    }
}

//////////////////////////////////////////////////////////////////////////
void CObjectLayer::Serialize(XmlNodeRef& node, bool bLoading)
{
    if (bLoading)
    {
        m_specs = eSpecType_All;
        // Loading.
        node->getAttr("Name", m_name);
        node->getAttr("GUID", m_guid);
        node->getAttr("Exportable", m_exportable);
        node->getAttr("ExportLayerPak", m_exportLayerPak);
        node->getAttr("DefaultLoaded", m_defaultLoaded);
        node->getAttr("HavePhysics", m_havePhysics);
        node->getAttr("Specs", m_specs);

        m_isDefaultColor = false;
        node->getAttr("IsDefaultColor", m_isDefaultColor);

        m_color = QPalette().color(QPalette::Button);
        if (!m_isDefaultColor)
        {
            node->getAttr("Color", m_color);
        }
        SetColor(m_color); // Update default state

        ZeroStruct(m_parentGUID);
        node->getAttr("ParentGUID", m_parentGUID);

        GetIEditor()->GetObjectManager()->InvalidateVisibleList();
    }
    else
    {
        // Saving.
        node->setAttr("Name", m_name.toUtf8().data());
        node->setAttr("GUID", m_guid);

        node->setAttr("Exportable", m_exportable);
        node->setAttr("ExportLayerPak", m_exportLayerPak);
        node->setAttr("DefaultLoaded", m_defaultLoaded);
        node->setAttr("HavePhysics", m_havePhysics);
        node->setAttr("IsDefaultColor", m_isDefaultColor);
        if (m_specs != eSpecType_All)
        {
            node->setAttr("Specs", m_specs);
        }
        if (!m_isDefaultColor)
        {
            node->setAttr("Color", m_color);
        }

        GUID parentGUID = m_parentGUID;
        if (!GuidUtil::IsEmpty(parentGUID))
        {
            node->setAttr("ParentGUID", parentGUID);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
int CObjectLayer::GetObjectCount() const
{
    CBaseObjectsArray objects;
    GetIEditor()->GetObjectManager()->GetObjects(objects, this);
    return int(objects.size());
}

//////////////////////////////////////////////////////////////////////////
void CObjectLayer::SetName(const QString& name, bool IsUpdateDepends)
{
    if (m_name == name)
    {
        return;
    }

    if (CUndo::IsRecording())
    {
        CUndo::Record(new CUndoLayerName(this));
    }

    QString oldName = m_name;
    m_name = name;

    if (IsUpdateDepends)
    {
        GetIEditor()->GetFlowGraphManager()->UpdateLayerName(oldName, name);
    }
}

//////////////////////////////////////////////////////////////////////////
void CObjectLayer::AddChild(CObjectLayer* pLayer)
{
    assert(pLayer);
    if (IsChildOf(pLayer))
    {
        return;
    }
    if (pLayer->GetParent())
    {
        pLayer->GetParent()->RemoveChild(pLayer);
    }
    stl::push_back_unique(m_childLayers, pLayer);
    pLayer->m_parent = this;
    pLayer->m_parentGUID = GetGUID();
    GetIEditor()->GetObjectManager()->InvalidateVisibleList();

    // Notify layer manager on layer modification.
    GetIEditor()->GetObjectManager()->GetLayersManager()->NotifyLayerChange(this);
}

//////////////////////////////////////////////////////////////////////////
void CObjectLayer::RemoveChild(CObjectLayer* pLayer)
{
    assert(pLayer);
    pLayer->m_parent = 0;
    ZeroStruct(pLayer->m_parentGUID);
    stl::find_and_erase(m_childLayers, pLayer);
    GetIEditor()->GetObjectManager()->InvalidateVisibleList();

    // Notify layer manager on layer modification.
    GetIEditor()->GetObjectManager()->GetLayersManager()->NotifyLayerChange(this);
}

//////////////////////////////////////////////////////////////////////////
CObjectLayer* CObjectLayer::GetChild(int index) const
{
    assert(index >= 0 && index < m_childLayers.size());
    return m_childLayers[index];
}

//////////////////////////////////////////////////////////////////////////
CObjectLayer* CObjectLayer::FindChildLayer(REFGUID guid)
{
    if (m_guid == guid)
    {
        return this;
    }

    for (int i = 0; i < GetChildCount(); i++)
    {
        CObjectLayer* pLayer = GetChild(i)->FindChildLayer(guid);
        if (pLayer)
        {
            return pLayer;
        }
    }
    return 0;
}

//////////////////////////////////////////////////////////////////////////
bool CObjectLayer::IsChildOf(CObjectLayer* pParent)
{
    if (m_parent == pParent)
    {
        return true;
    }
    if (m_parent)
    {
        return m_parent->IsChildOf(pParent);
    }
    return false;
}

//////////////////////////////////////////////////////////////////////////
void CObjectLayer::SetVisible(bool b, bool bRecursive)
{
    if (m_hidden != !b)
    {
        if (CUndo::IsRecording())
        {
            CUndo::Record(new CUndoLayerStates(this));
        }

        m_hidden = !b;

        // Notify layer manager on layer modification.
        GetIEditor()->GetObjectManager()->GetLayersManager()->NotifyLayerChange(this);
        GetIEditor()->GetObjectManager()->InvalidateVisibleList();
    }

    if (bRecursive)
    {
        for (int i = 0; i < GetChildCount(); i++)
        {
            GetChild(i)->SetVisible(b, bRecursive);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CObjectLayer::SetFrozen(bool b, bool bRecursive)
{
    if (m_frozen != b)
    {
        if (CUndo::IsRecording())
        {
            CUndo::Record(new CUndoLayerStates(this));
        }

        m_frozen = b;

        // Notify layer manager on layer modification.
        GetIEditor()->GetObjectManager()->GetLayersManager()->NotifyLayerChange(this);
        GetIEditor()->GetObjectManager()->InvalidateVisibleList();
    }
    if (bRecursive)
    {
        for (int i = 0; i < GetChildCount(); i++)
        {
            GetChild(i)->SetFrozen(b, bRecursive);
        }
    }
    GetIEditor()->GetFlowGraphManager()->SendNotifyEvent(EHG_GRAPH_UPDATE_FROZEN);
}

//////////////////////////////////////////////////////////////////////////
void CObjectLayer::Expand(bool bExpand)
{
    m_expanded = bExpand;
}

//////////////////////////////////////////////////////////////////////////
bool CObjectLayer::IsParentExternal() const
{
    if (m_parent)
    {
        if (m_parent->IsExternal())
        {
            return true;
        }
        return m_parent->IsParentExternal();
    }
    return false;
}

//////////////////////////////////////////////////////////////////////////
void CObjectLayer::SetModified(bool isModified)
{
    CObjectLayerManager* pLayerManager = GetIEditor()->GetObjectManager()->GetLayersManager();

    if (pLayerManager->CanModifyLayers() == false)
    {
        return;
    }

    if (m_isModified == isModified)
    {
        return;
    }
    m_isModified = isModified;

    pLayerManager->NotifyLayerChange(this);
    GetIEditor()->Notify(eNotify_OnInvalidateControls);
}

//////////////////////////////////////////////////////////////////////////
QString CObjectLayer::GetExternalLayerPath()
{
    CObjectLayerManager* pLayerManager = GetIEditor()->GetObjectManager()->GetLayersManager();
    return Path::AddSlash(GetIEditor()->GetGameEngine()->GetLevelPath()) + pLayerManager->GetLayersPath() + GetFullName() + LAYER_FILE_EXTENSION;
}

//////////////////////////////////////////////////////////////////////////
QColor CObjectLayer::GetColor() const
{
    if (m_isDefaultColor)
    {
        return QPalette().color(QPalette::Button);
    }
    return m_color;
}

//////////////////////////////////////////////////////////////////////////
bool CObjectLayer::IsDefaultColor() const
{
    return m_isDefaultColor;
}

//////////////////////////////////////////////////////////////////////////
void CObjectLayer::SetColor(const QColor& color)
{
    m_color = color;
    m_isDefaultColor = (color == QPalette().color(QPalette::Button));
}

//////////////////////////////////////////////////////////////////////////
QString CObjectLayer::GetFullName() const
{
    QString fullName;
    GetFullNameRecursively(this, fullName);
    return fullName;
}

//////////////////////////////////////////////////////////////////////////
void CObjectLayer::GetFullNameRecursively(const CObjectLayer* const pObjectLayer, QString& name) const
{
    if (!pObjectLayer)
    {
        return;
    }

    CObjectLayer* parent = pObjectLayer->GetParent();
    if (parent)
    {
        GetFullNameRecursively(parent, name);
        name = name + kExternalLayerPathDelimiter + pObjectLayer->GetName();
    }
    else
    {
        name = name + pObjectLayer->GetName();
    }
}

//////////////////////////////////////////////////////////////////////////
bool CObjectLayer::IsExpanded() const
{
    return m_expanded;
}

//////////////////////////////////////////////////////////////////////////
void CObjectLayer::SetLayerID(uint16 nLayerId)
{
    m_nLayerId = nLayerId;
}

//////////////////////////////////////////////////////////////////////////
uint16 CObjectLayer::GetLayerID() const
{
    return m_nLayerId;
}
