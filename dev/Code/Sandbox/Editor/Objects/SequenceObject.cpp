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

#include "EntityObject.h"
#include "Geometry/EdMesh.h"
#include "Material/Material.h"
#include "Material/MaterialManager.h"

#include <IEntitySystem.h>
#include <IEntityRenderState.h>


#include "SequenceObject.h"

#include "TrackView/TrackViewSequenceManager.h"

//////////////////////////////////////////////////////////////////////////
// CSequenceObject implementation.
//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
CSequenceObject::CSequenceObject()
{
    m_sequence = 0;
    m_bValidSettings = false;
    m_sequenceId = 0;
}


//////////////////////////////////////////////////////////////////////////
bool CSequenceObject::Init(IEditor* ie, CBaseObject* prev, const QString& file)
{
    // A SequenceObject cannot be cloned.
    if (prev)
    {
        return false;
    }

    SetColor(QColor(127, 127, 255));

    if (!file.isEmpty())
    {
        SetName(file);
    }

    SetTextureIcon(GetClassDesc()->GetTextureIconId());

    // Must be after SetSequence call.
    bool res = CBaseObject::Init(ie, prev, file);

    return res;
}

//////////////////////////////////////////////////////////////////////////
bool CSequenceObject::CreateGameObject()
{
    if (!m_sequence)
    {
        CTrackViewSequenceManager* pSequenceManager = GetIEditor()->GetSequenceManager();
        m_sequence = pSequenceManager->OnCreateSequenceObject(GetName());
    }

    if (m_sequence)
    {
        m_sequence->SetLegacySequenceObject(this);
    }

    return true;
}

//////////////////////////////////////////////////////////////////////////
void CSequenceObject::Done()
{
    if (m_sequence)
    {
        GetIEditor()->GetSequenceManager()->OnDeleteSequenceEntity(m_sequence->GetSequenceEntityId());
        m_sequence->SetLegacySequenceObject(nullptr);
    }

    m_sequence = nullptr;

    CBaseObject::Done();
}

//////////////////////////////////////////////////////////////////////////
void CSequenceObject::OnNameChanged()
{
    assert(m_sequence);

    if (m_sequence)
    {
        string fullname = m_sequence->GetName();
        CBaseObject::SetName(fullname.c_str());

        OnModified();
    }
}

//////////////////////////////////////////////////////////////////////////
void CSequenceObject::OnModified()
{
    SetLayerModified();
}

//////////////////////////////////////////////////////////////////////////
void CSequenceObject::GetBoundBox(AABB& box)
{
    box.SetTransformedAABB(GetWorldTM(), AABB(Vec3(-1, -1, -1), Vec3(1, 1, 1)));
}

//////////////////////////////////////////////////////////////////////////
void CSequenceObject::GetLocalBounds(AABB& box)
{
    box.min = Vec3(-1, -1, -1);
    box.max = Vec3(1, 1, 1);
}


//////////////////////////////////////////////////////////////////////////
void CSequenceObject::Display(DisplayContext& dc)
{
    /*
    const Matrix34 &wtm = GetWorldTM();
    Vec3 wp = wtm.GetTranslation();

    if (IsSelected())
        dc.SetSelectedColor();
    else if (IsFrozen())
        dc.SetFreezeColor();
    else
        dc.SetColor( GetColor() );

    dc.PushMatrix( wtm );
    BBox box;
    GetLocalBounds(box);
    dc.DrawWireBox( box.min,box.max);
    dc.PopMatrix();
    */

    DrawDefault(dc);
}

//////////////////////////////////////////////////////////////////////////
void CSequenceObject::CZoomScrollSettings::Save(XmlNodeRef node) const
{
    node->setAttr("dopeSheetZoom", dopeSheetZoom);
    node->setAttr("dopeSheetHScroll", dopeSheetHScroll);
    node->setAttr("nodesListVScroll", nodesListVScroll);
    node->setAttr("curveEditorZoom", curveEditorZoom);
    node->setAttr("curveEditorScroll", curveEditorScroll);
}

//////////////////////////////////////////////////////////////////////////
bool CSequenceObject::CZoomScrollSettings::Load(XmlNodeRef node)
{
    if (node->getAttr("dopeSheetZoom", dopeSheetZoom) == false)
    {
        return false;
    }
    if (node->getAttr("dopeSheetHScroll", dopeSheetHScroll) == false)
    {
        return false;
    }
    if (node->getAttr("nodesListVScroll", nodesListVScroll) == false)
    {
        return false;
    }
    if (node->getAttr("curveEditorZoom", curveEditorZoom) == false)
    {
        return false;
    }
    if (node->getAttr("curveEditorScroll", curveEditorScroll) == false)
    {
        return false;
    }

    return true;
}

#include <Objects/SequenceObject.moc>