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

#include "CryLegacy_precompiled.h"
#include "FaceEffector.h"
#include "FaceEffectorLibrary.h"

//////////////////////////////////////////////////////////////////////////
float CFacialEffCtrl::Evaluate(float fInput)
{
    switch (m_type)
    {
    case CTRL_LINEAR:
        return fInput * m_fWeight;
    case CTRL_SPLINE:
    {
        float fOut = 0;
        if (m_pSplineInterpolator)
        {
            m_pSplineInterpolator->interpolate(fInput, fOut);
        }
        //fOut=fInput;
        return fOut;
    }
    }
    return 0;
}

//////////////////////////////////////////////////////////////////////////
void CFacialEffCtrl::SetType(ControlType t)
{
    if (t == m_type)
    {
        return;
    }

    if (t == CTRL_SPLINE)
    {
        m_type = t;
        if (!m_pSplineInterpolator)
        {
            m_pSplineInterpolator = new CFacialEffCtrlSplineInterpolator(this);
        }

        m_pSplineInterpolator->clear();
        m_pSplineInterpolator->InsertKeyFloat(-1, -m_fWeight);
        m_pSplineInterpolator->InsertKeyFloat(1, m_fWeight);
    }
    else if (t == CTRL_LINEAR)
    {
        if (m_pSplineInterpolator)
        {
            if (gEnv->IsEditor())
            {
                m_pSplineInterpolator->clear();
            }
            else
            {
                delete m_pSplineInterpolator;
            }
        }
        m_type = t;
    }
};

//////////////////////////////////////////////////////////////////////////
void CFacialEffCtrl::SetConstantWeight(float fWeight)
{
    m_fWeight = fWeight;
    if (m_type == CTRL_SPLINE)
    {
        if (!m_pSplineInterpolator)
        {
            m_pSplineInterpolator = new CFacialEffCtrlSplineInterpolator(this);
        }

        m_pSplineInterpolator->InsertKeyFloat(-1, -fWeight);
        m_pSplineInterpolator->InsertKeyFloat(1, fWeight);
    }
}

//////////////////////////////////////////////////////////////////////////
void CFacialEffCtrl::SetConstantBalance(float fBalance)
{
    m_fBalance = fBalance;
}

//////////////////////////////////////////////////////////////////////////
void CFacialEffCtrlSplineInterpolator::SerializeSpline(XmlNodeRef& node, bool bLoading)
{
    if (bLoading)
    {
        clear();

        const char* attribute = node->getAttr("Keys");
        const char* position = attribute;
        for (;; )
        {
            char* end;

            // Skip to the next digit.
            for (; *position && !(*position >= '0' && *position <= '9') && *position != '.' && *position != '-'; ++position)
            {
                ;
            }
            if (!*position)
            {
                break;
            }

            // Read in the time.
            float time = float(strtod(position, &end));
            if (position == end || !end)
            {
                break;
            }
            position = end;

            // Skip to the next digit.
            for (; *position && !(*position >= '0' && *position <= '9') && *position != '.' && *position != '-'; ++position)
            {
                ;
            }
            if (!*position)
            {
                break;
            }

            // Read in the value.
            float value = float(strtod(position, &end));
            if (position == end || !end)
            {
                break;
            }
            position = end;

            // Insert the key we have created.
            InsertKeyFloat(time, value);
        }
    }
    else
    {
        string keystr;
        string s;
        for (int i = 0; i < num_keys(); i++)
        {
            s.Format("%g:%g,", key(i).time, key(i).value);
            keystr += s;
        }
        node->setAttr("Keys", keystr);
    }
}

void CFacialEffCtrlSplineInterpolator::Interpolate(float time, ValueType& val)
{
    val[0] = m_pOwner->Evaluate(time);
}
//////////////////////////////////////////////////////////////////////////
void CFacialEffCtrl::Serialize(CFacialEffectorsLibrary* pLibrary, XmlNodeRef& node, bool bLoading)
{
    // MichaelS - Don't serialize m_fBalance - currently this is only used in the preview mode in the facial editor.

    if (bLoading)
    {
        int nType = CTRL_LINEAR;
        m_fWeight = 0;
        node->getAttr("Type", nType);
        node->getAttr("Weight", m_fWeight);
        m_type = (ControlType)nType;
        if (m_type == CTRL_SPLINE)
        {
            SerializeSpline(node, bLoading);
        }
        const char* sEffectorName = node->getAttr("Effector");
        m_pEffector = (CFacialEffector*)pLibrary->Find(sEffectorName);
    }
    else
    {
#ifdef FACE_STORE_ASSET_VALUES
        if (m_type != CTRL_LINEAR)
        {
            node->setAttr("Type", m_type);
        }
        if (m_fWeight != 0)
        {
            node->setAttr("Weight", m_fWeight);
        }
        if (m_type == CTRL_SPLINE)
        {
            SerializeSpline(node, bLoading);
        }
        node->setAttr("Effector", m_pEffector->GetName());
#endif
    }
}

CFacialEffCtrl::CFacialEffCtrl()
{
    m_fWeight = 1.0f;
    m_fBalance = 0.0f;
    m_type = CTRL_LINEAR;
    m_nFlags = 0;
    if (gEnv->IsEditor())
    {
        m_pSplineInterpolator = new CFacialEffCtrlSplineInterpolator(this);
    }
    else
    {
        m_pSplineInterpolator = NULL;
    }
}
//////////////////////////////////////////////////////////////////////////
IFacialEffector* CFacialEffector::GetSubEffector(int nIndex)
{
    IFacialEffCtrl* pCtrl = m_subEffectors[nIndex];
    if (pCtrl)
    {
        return pCtrl->GetEffector();
    }
    return nullptr;
};

//////////////////////////////////////////////////////////////////////////
IFacialEffCtrl* CFacialEffector::GetSubEffCtrl(int nIndex)
{
    return m_subEffectors[nIndex];
}

//////////////////////////////////////////////////////////////////////////
IFacialEffCtrl* CFacialEffector::GetSubEffCtrlByName(const char* effectorName)
{
    int subEffectorCount = (int)m_subEffectors.size();
    for (int i = 0; i < subEffectorCount; ++i)
    {
#ifdef FACE_STORE_ASSET_VALUES
        if (_stricmp(m_subEffectors[i]->GetEffector()->GetName(), effectorName) == 0)
        {
            return m_subEffectors[i];
        }
#endif
    }

    return NULL;
}

//////////////////////////////////////////////////////////////////////////
IFacialEffCtrl* CFacialEffector::AddSubEffector(IFacialEffector* pEffector)
{
    CFacialEffCtrl* pCtrl = new CFacialEffCtrl;
    pCtrl->SetCEffector((CFacialEffector*)pEffector);
    m_subEffectors.push_back(pCtrl);
    return pCtrl;
}

//////////////////////////////////////////////////////////////////////////
void CFacialEffector::RemoveSubEffector(IFacialEffector* pEffector)
{
    int nSize = m_subEffectors.size();
    for (int i = 0; i < nSize; i++)
    {
        if (m_subEffectors[i]->GetCEffector() == pEffector)
        {
            m_subEffectors.erase(m_subEffectors.begin() + i);
            return;
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CFacialEffector::RemoveAllSubEffectors()
{
    m_subEffectors.clear();
}

//////////////////////////////////////////////////////////////////////////
void CFacialEffector::SetIdentifier(CFaceIdentifierHandle ident)
{
    if (m_pLibrary)
    {
        m_pLibrary->RenameEffector(this, ident);
    }
    m_name = ident;
}

//////////////////////////////////////////////////////////////////////////
void CFacialEffector::Serialize(XmlNodeRef& node, bool bLoading)
{
}

void CFacialEffector::GetMemoryUsage(ICrySizer* pSizer) const
{
    pSizer->AddObject(this, sizeof(*this));
    pSizer->AddObject(m_name);
    pSizer->AddObject(m_nFlags);
    pSizer->AddObject(m_nIndexInState);

    SubEffVector::const_iterator it;
    for (it = m_subEffectors.begin(); it != m_subEffectors.end(); ++it)
    {
        (*it)->GetMemoryUsage(pSizer, true);
    }
}
//////////////////////////////////////////////////////////////////////////
// CFacialEffectorAttachment Implementation
//////////////////////////////////////////////////////////////////////////
void CFacialEffectorAttachment::SetParamString(EFacialEffectorParam param, const char* str)
{
    switch (param)
    {
    case EFE_PARAM_BONE_NAME:
#ifdef FACE_STORE_ASSET_VALUES
        m_sAttachment = str;
#endif
        m_attachmentCRC = CCrc32::ComputeLowercase(str);
        break;
    }
}

//////////////////////////////////////////////////////////////////////////
const char* CFacialEffectorAttachment::GetParamString(EFacialEffectorParam param)
{
    switch (param)
    {
    case EFE_PARAM_BONE_NAME:
#ifdef FACE_STORE_ASSET_VALUES
        return m_sAttachment;
#else
        return "Name Stripped";
#endif
        break;
    }
    return CFacialEffector::GetParamString(param);
}

//////////////////////////////////////////////////////////////////////////
void CFacialEffectorAttachment::SetParamVec3(EFacialEffectorParam param, Vec3 vValue)
{
    switch (param)
    {
    case EFE_PARAM_BONE_ROT_AXIS:
#ifdef FACE_STORE_ASSET_VALUES
        m_vAngles = vValue;
#endif
        m_vQuatT.q.SetRotationXYZ(Ang3(vValue));
        break;
    case EFE_PARAM_BONE_POS_AXIS:
#ifdef FACE_STORE_ASSET_VALUES
        m_vOffset = vValue;
#endif
        m_vQuatT.t = vValue;
        break;
    }
}

//////////////////////////////////////////////////////////////////////////
Vec3 CFacialEffectorAttachment::GetParamVec3(EFacialEffectorParam param)
{
#ifdef FACE_STORE_ASSET_VALUES
    switch (param)
    {
    case EFE_PARAM_BONE_ROT_AXIS:
        return m_vAngles;
        break;
    case EFE_PARAM_BONE_POS_AXIS:
        return m_vOffset;
        break;
    }
#else
    CryFatalError("Vec3 Params are not stored on consoles!");
#endif
    return Vec3(0, 0, 0);
}

//////////////////////////////////////////////////////////////////////////
QuatT CFacialEffectorAttachment::GetQuatT()
{
    return m_vQuatT;
}

//////////////////////////////////////////////////////////////////////////
void CFacialEffectorAttachment::Serialize(XmlNodeRef& node, bool bLoading)
{
    if (bLoading)
    {
        string attachment = node->getAttr("Attachment");
        m_attachmentCRC = CCrc32::ComputeLowercase(attachment);

        Vec3 offset, angles;
        node->getAttr("PosOffset", offset);
        node->getAttr("RotOffset", angles);

        m_vQuatT.q.SetRotationXYZ(Ang3(angles));
        m_vQuatT.t = offset;

#ifdef FACE_STORE_ASSET_VALUES
        m_sAttachment = attachment;
        m_vOffset = offset;
        m_vAngles = angles;
#endif
    }
    else
    {
#ifdef FACE_STORE_ASSET_VALUES
        node->setAttr("Attachment", m_sAttachment);
        node->setAttr("PosOffset", m_vOffset);
        node->setAttr("RotOffset", m_vAngles);
#endif
    }
}

void CFacialEffectorAttachment::Set(const CFacialEffectorAttachment* const rhs)
{
#ifdef FACE_STORE_ASSET_VALUES
    m_vAngles = rhs->m_vAngles;
    m_vOffset = rhs->m_vOffset;
    m_sAttachment = rhs->m_sAttachment;
#endif
    m_attachmentCRC = rhs->m_attachmentCRC;
    m_vQuatT = rhs->m_vQuatT;
}

CFacialEffectorAttachment::CFacialEffectorAttachment()
{
#ifdef FACE_STORE_ASSET_VALUES
    m_vAngles.Set(0, 0, 0);
    m_vOffset.Set(0, 0, 0);
#endif
    m_attachmentCRC = 0;
}

