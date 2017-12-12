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
#include "Volume.h"

#include "../Viewport.h"

//////////////////////////////////////////////////////////////////////////
// CBase implementation.
//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
CVolume::CVolume()
{
    m_box.min = Vec3(-1, -1, 0);
    m_box.max = Vec3(1, 1, 2);

    // Initials.
    mv_width = 4;
    mv_length = 4;
    mv_height = 1;
    mv_viewDistance = 4;

    AddVariable(mv_width, "Width", functor(*this, &CVolume::OnSizeChange));
    AddVariable(mv_length, "Length", functor(*this, &CVolume::OnSizeChange));
    AddVariable(mv_height, "Height", functor(*this, &CVolume::OnSizeChange));
    AddVariable(mv_viewDistance, "ViewDistance");
    AddVariable(mv_shader, "Shader", 0, IVariable::DT_SHADER);
    AddVariable(mv_fogColor, "Color", 0, IVariable::DT_COLOR);
}

//////////////////////////////////////////////////////////////////////////
void CVolume::Done()
{
    CBaseObject::Done();
}

//////////////////////////////////////////////////////////////////////////
bool CVolume::Init(IEditor* ie, CBaseObject* prev, const QString& file)
{
    bool res = CBaseObject::Init(ie, prev, file);
    SetColor(QColor(0, 0, 255));
    if (prev)
    {
        m_sphere = ((CVolume*)prev)->m_sphere;
        m_box = ((CVolume*)prev)->m_box;
    }
    return res;
}

//////////////////////////////////////////////////////////////////////////
void CVolume::GetLocalBounds(AABB& box)
{
    box = m_box;
}

//////////////////////////////////////////////////////////////////////////
bool CVolume::HitTest(HitContext& hc)
{
    Vec3 p;
    AABB box;
    GetBoundBox(box);
    if (Intersect::Ray_AABB(hc.raySrc, hc.rayDir, box, p))
    {
        hc.dist = Vec3(hc.raySrc - p).GetLength();
        hc.object = this;
        return true;
    }
    return false;
}

//////////////////////////////////////////////////////////////////////////
void CVolume::OnSizeChange(IVariable* var)
{
    Vec3 size(0, 0, 0);
    size.x = mv_width;
    size.y = mv_length;
    size.z = mv_height;

    m_box.min = -size / 2;
    m_box.max = size / 2;
    // Make volume position bottom of bounding box.
    m_box.min.z = 0;
    m_box.max.z = size.z;
}

//////////////////////////////////////////////////////////////////////////
void CVolume::Display(DisplayContext& dc)
{
    QColor wireColor, solidColor;
    float alpha = 0.3f;
    if (IsSelected())
    {
        wireColor = dc.GetSelectedColor();
        solidColor = GetColor();
    }
    else
    {
        wireColor = GetColor();
        solidColor = GetColor();
    }

    dc.DepthWriteOff();
    dc.CullOff();

    AABB box;
    const Matrix34& tm = GetWorldTM();
    box.min = tm.TransformPoint(m_box.min);
    box.max = tm.TransformPoint(m_box.max);


    bool bFrozen = IsFrozen();

    if (bFrozen)
    {
        dc.SetFreezeColor();
    }
    //////////////////////////////////////////////////////////////////////////
    if (!bFrozen)
    {
        dc.SetColor(solidColor, alpha);
    }
    //dc.DrawSolidBox( box.min,box.max );

    if (!bFrozen)
    {
        dc.SetColor(wireColor, 1);
    }

    if (IsSelected())
    {
        dc.SetLineWidth(3);
        dc.DrawWireBox(box.min, box.max);
        dc.SetLineWidth(0);
    }
    else
    {
        dc.DrawWireBox(box.min, box.max);
    }
    //////////////////////////////////////////////////////////////////////////

    dc.CullOn();
    dc.DepthWriteOn();


    DrawDefault(dc);
}

//////////////////////////////////////////////////////////////////////////
void CVolume::Serialize(CObjectArchive& ar)
{
    CBaseObject::Serialize(ar);
    XmlNodeRef xmlNode = ar.node;
    if (ar.bLoading)
    {
        xmlNode->getAttr("BoxMin", m_box.min);
        xmlNode->getAttr("BoxMax", m_box.max);
    }
    else
    {
        // Saving.
        xmlNode->setAttr("BoxMin", m_box.min);
        xmlNode->setAttr("BoxMax", m_box.max);
    }
}

//////////////////////////////////////////////////////////////////////////
XmlNodeRef CVolume::Export(const QString& levelPath, XmlNodeRef& xmlNode)
{
    XmlNodeRef objNode = CBaseObject::Export(levelPath, xmlNode);

    // Export position in world space.
    Matrix34 wtm = GetWorldTM();
    AffineParts affineParts;
    affineParts.SpectralDecompose(wtm);
    Vec3 worldPos = affineParts.pos;
    Ang3 worldAngles = RAD2DEG(Ang3::GetAnglesXYZ(Matrix33(affineParts.rot)));

    if (!worldPos.IsEquivalent(Vec3(ZERO), 0))
    {
        objNode->setAttr("Pos", worldPos);
    }

    if (!worldAngles.IsEquivalent(Ang3(ZERO), 0))
    {
        objNode->setAttr("Angles", worldAngles);
    }

    objNode->setAttr("BoxMin", wtm.TransformPoint(m_box.min));
    objNode->setAttr("BoxMax", wtm.TransformPoint(m_box.max));
    return objNode;

    /*
    CString volumeName,volumeParam;

    BBox box;
    GetBoundBox( box );

    volumeName = GetClassDesc()->GetTypeName() + "_" + GetName();
    volumeParam.Format( "Box{ %.2f,%.2f,%.2f,%.2f,%.2f,%.2f } ",box.min.x,box.min.y,box.min.z,box.max.x,box.max.y,box.max.z );

    if (GetParams())
    {
        CString str;
        XmlAttributes attributes = GetParams()->getAttributes();
        for (XmlAttributes::iterator it = attributes.begin(); it != attributes.end(); ++it)
        {
            str.Format( "%s{ %s } ",(const char*)it->key,(const char*)it->value );
            volumeParam += str;
        }
    }
    WritePrivateProfileString( "",volumeName,volumeParam,levelPath+"Volumes.ini" );

    return CBaseObject::Export( levelPath,xmlNode );
    */
}

#include <Objects/Volume.moc>