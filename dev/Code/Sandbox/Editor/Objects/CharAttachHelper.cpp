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

// Description : Entity character attachment helper object.


#include "StdAfx.h"
#include "CharAttachHelper.h"
#include <ICryAnimation.h>

float CCharacterAttachHelperObject::m_charAttachHelperScale = 1.0f;

//////////////////////////////////////////////////////////////////////////
CCharacterAttachHelperObject::CCharacterAttachHelperObject()
{
    m_entityClass = "CharacterAttachHelper";
}

//////////////////////////////////////////////////////////////////////////
void CCharacterAttachHelperObject::Display(DisplayContext& dc)
{
    CEntityObject::Display(dc);

    dc.SetLineWidth(4.0f);
    float s = 1.0f * GetHelperScale();

    if (m_pEntity)
    {
        Matrix34 tm = m_pEntity->GetWorldTM();

        if (IsSelected())
        {
            dc.SetSelectedColor();
        }
        else
        {
            dc.SetColor(GetColor());
        }

        dc.SetLineWidth(4.0f);
        dc.DrawLine(tm.TransformPoint(Vec3(0, 0, 0)), tm.TransformPoint(Vec3(s, 0, 0)), ColorF(1, 0, 0), ColorF(1, 0, 0));
        dc.DrawLine(tm.TransformPoint(Vec3(0, 0, 0)), tm.TransformPoint(Vec3(0, s, 0)), ColorF(0, 1, 0), ColorF(0, 1, 0));
        dc.DrawLine(tm.TransformPoint(Vec3(0, 0, 0)), tm.TransformPoint(Vec3(0, 0, s)), ColorF(0, 0, 1), ColorF(0, 0, 1));
        dc.SetLineWidth(0);

        dc.SetColor(ColorB(0, 255, 0, 255));
        dc.DrawLine(tm.TransformPoint(Vec3(0, 0, 0)), GetWorldPos());
    }

    const Matrix34& tm = GetWorldTM();

    if (IsSelected())
    {
        dc.SetSelectedColor();
    }
    else
    {
        dc.SetColor(GetColor());
    }

    dc.SetLineWidth(4.0f);
    dc.DrawLine(tm.TransformPoint(Vec3(0, 0, 0)), tm.TransformPoint(Vec3(s, 0, 0)), ColorF(1, 0, 0), ColorF(1, 0, 0));
    dc.DrawLine(tm.TransformPoint(Vec3(0, 0, 0)), tm.TransformPoint(Vec3(0, s, 0)), ColorF(0, 1, 0), ColorF(0, 1, 0));
    dc.DrawLine(tm.TransformPoint(Vec3(0, 0, 0)), tm.TransformPoint(Vec3(0, 0, s)), ColorF(0, 0, 1), ColorF(0, 0, 1));
    dc.SetLineWidth(0);
}

//////////////////////////////////////////////////////////////////////////
void CCharacterAttachHelperObject::SetHelperScale(float scale)
{
    m_charAttachHelperScale = scale;
}

#include <Objects/CharAttachHelper.moc>