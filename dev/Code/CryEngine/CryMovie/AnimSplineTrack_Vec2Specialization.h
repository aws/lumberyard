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

// Description : 'Vec2' explicit specialization of the class template
//               TAnimSplineTrack
// Notice      : Should be included in AnimSplineTrack h only


#ifndef CRYINCLUDE_CRYMOVIE_ANIMSPLINETRACK_VEC2SPECIALIZATION_H
#define CRYINCLUDE_CRYMOVIE_ANIMSPLINETRACK_VEC2SPECIALIZATION_H
#pragma once

template <>
inline TAnimSplineTrack<Vec2>::TAnimSplineTrack()
{
    AllocSpline();
    m_flags = 0;
    m_defaultValue = Vec2(0, 0);
    m_fMinKeyValue = 0.0f;
    m_fMaxKeyValue = 0.0f;
    m_bCustomColorSet = false;
    m_node = nullptr;
    m_trackMultiplier = 1.0f;
}
template <>
inline void TAnimSplineTrack<Vec2>::GetValue(float time, float& value, bool applyMultiplier)
{
    if (GetNumKeys() == 0)
    {
        value = m_defaultValue.y;
    }
    else
    {
        Spline::ValueType tmp;
        m_spline->Interpolate(time, tmp);
        value = tmp[0];
    }
    if (applyMultiplier && m_trackMultiplier != 1.0f)
    {
        value /= m_trackMultiplier;
    }
}
template <>
inline EAnimCurveType TAnimSplineTrack<Vec2>::GetCurveType() { return eAnimCurveType_BezierFloat; }
template <>
inline EAnimValue TAnimSplineTrack<Vec2>::GetValueType() { return eAnimValue_Float; }
template <>
inline void TAnimSplineTrack<Vec2>::SetValue(float time, const float& value, bool bDefault, bool applyMultiplier)
{
    if (!bDefault)
    {
        I2DBezierKey key;
        if (applyMultiplier && m_trackMultiplier != 1.0f)
        {
            key.value = Vec2(time, value * m_trackMultiplier);
        }
        else
        {
            key.value = Vec2(time, value);
        }
        SetKeyAtTime(time, &key);
    }
    else
    {
        if (applyMultiplier && m_trackMultiplier != 1.0f)
        {
            m_defaultValue = Vec2(time, value * m_trackMultiplier);
        }
        else
        {
            m_defaultValue = Vec2(time, value);
        }
    }
}

template <>
inline void TAnimSplineTrack<Vec2>::GetKey(int index, IKey* key) const
{
    assert(index >= 0 && index < GetNumKeys());
    assert(key != 0);
    Spline::key_type& k = m_spline->key(index);
    I2DBezierKey* bezierkey = (I2DBezierKey*)key;
    bezierkey->time = k.time;
    bezierkey->flags = k.flags;

    bezierkey->value = k.value;
}

template <>
inline void TAnimSplineTrack<Vec2>::SetKey(int index, IKey* key)
{
    assert(index >= 0 && index < GetNumKeys());
    assert(key != 0);
    Spline::key_type& k = m_spline->key(index);
    I2DBezierKey* bezierkey = (I2DBezierKey*)key;
    k.time = bezierkey->time;
    k.flags = bezierkey->flags;
    k.value = bezierkey->value;
    UpdateTrackValueRange(k.value.y);
    Invalidate();
}

//! Create key at given time, and return its index.
template <>
inline int TAnimSplineTrack<Vec2>::CreateKey(float time)
{
    float value;

    int nkey = GetNumKeys();

    if (nkey > 0)
    {
        GetValue(time, value);
    }
    else
    {
        value = m_defaultValue.y;
    }

    UpdateTrackValueRange(value);

    Spline::ValueType tmp;
    tmp[0] = value;
    tmp[1] = 0;
    return m_spline->InsertKey(time, tmp);
}

template <>
inline int TAnimSplineTrack<Vec2>::CopyKey(IAnimTrack* pFromTrack, int nFromKey)
{
    // This small time offset is applied to prevent the generation of singular tangents.
    float timeOffset = 0.01f;
    I2DBezierKey key;
    pFromTrack->GetKey(nFromKey, &key);
    float t = key.time + timeOffset;
    int newIndex =  CreateKey(t);
    key.time = key.value.x = t;
    SetKey(newIndex, &key);
    return newIndex;
}

template <>
inline bool TAnimSplineTrack<Vec2>::Serialize(XmlNodeRef& xmlNode, bool bLoading, bool bLoadEmptyTracks)
{
    if (bLoading)
    {
        int num = xmlNode->getChildCount();

        int flags = m_flags;
        xmlNode->getAttr("Flags", flags);
        xmlNode->getAttr("defaultValue", m_defaultValue);
        SetFlags(flags);
        xmlNode->getAttr("HasCustomColor", m_bCustomColorSet);
        if (m_bCustomColorSet)
        {
            unsigned int abgr;
            xmlNode->getAttr("CustomColor", abgr);
            m_customColor = ColorB(abgr);
        }

        SetNumKeys(num);
        for (int i = 0; i < num; i++)
        {
            I2DBezierKey key; // Must be inside loop.

            XmlNodeRef keyNode = xmlNode->getChild(i);
            if (!keyNode->getAttr("time", key.time))
            {
                CryLog("[CRYMOVIE:TAnimSplineTrack<Vec2>::Serialize]Ill formed legacy track:missing time information.");
                return false;
            }
            if (!keyNode->getAttr("value", key.value))
            {
                CryLog("[CRYMOVIE:TAnimSplineTrack<Vec2>::Serialize]Ill formed legacy track:missing value information.");
                return false;
            }
            //assert(key.time == key.value.x);

            keyNode->getAttr("flags", key.flags);

            SetKey(i, &key);

            // In-/Out-tangent
            if (!keyNode->getAttr("ds", m_spline->key(i).ds))
            {
                CryLog("[CRYMOVIE:TAnimSplineTrack<Vec2>::Serialize]Ill formed legacy track:missing ds spline information.");
                return false;
            }

            if (!keyNode->getAttr("dd", m_spline->key(i).dd))
            {
                CryLog("[CRYMOVIE:TAnimSplineTrack<Vec2>::Serialize]Ill formed legacy track:dd spline information.");
                return false;
            }
            // now that tangents are loaded, compute the relative angle and size for later unified Tangent manipulations
            m_spline->key(i).ComputeThetaAndScale();
        }

        if ((!num) && (!bLoadEmptyTracks))
        {
            return false;
        }
    }
    else
    {
        int num = GetNumKeys();
        xmlNode->setAttr("Flags", GetFlags());
        xmlNode->setAttr("defaultValue", m_defaultValue);
        xmlNode->setAttr("HasCustomColor", m_bCustomColorSet);
        if (m_bCustomColorSet)
        {
            xmlNode->setAttr("CustomColor", m_customColor.pack_abgr8888());
        }
        I2DBezierKey key;
        for (int i = 0; i < num; i++)
        {
            GetKey(i, &key);
            XmlNodeRef keyNode = xmlNode->newChild("Key");
            assert(key.time == key.value.x);
            keyNode->setAttr("time", key.time);
            keyNode->setAttr("value", key.value);

            int flags = key.flags;
            // Just save the in/out/unify mask part. Others are for editing convenience.
            flags &= (SPLINE_KEY_TANGENT_IN_MASK | SPLINE_KEY_TANGENT_OUT_MASK | SPLINE_KEY_TANGENT_UNIFY_MASK);
            if (flags != 0)
            {
                keyNode->setAttr("flags", flags);
            }

            // We also have to save in-/out-tangents, because TCB infos are not used for custom tangent keys.
            keyNode->setAttr("ds", m_spline->key(i).ds);
            keyNode->setAttr("dd", m_spline->key(i).dd);
        }
    }
    return true;
}

template <>
inline bool TAnimSplineTrack<Vec2>::SerializeSelection(XmlNodeRef& xmlNode, bool bLoading, bool bCopySelected, float fTimeOffset)
{
    if (bLoading)
    {
        int numCur = GetNumKeys();
        int num = xmlNode->getChildCount();

        int type;
        xmlNode->getAttr("TrackType", type);

        if (type != GetCurveType())
        {
            return false;
        }

        SetNumKeys(num + numCur);
        for (int i = 0; i < num; i++)
        {
            I2DBezierKey key; // Must be inside loop.

            XmlNodeRef keyNode = xmlNode->getChild(i);
            keyNode->getAttr("time", key.time);
            keyNode->getAttr("value", key.value);
            assert(key.time == key.value.x);
            key.time += fTimeOffset;
            key.value.x += fTimeOffset;

            keyNode->getAttr("flags", key.flags);

            SetKey(i + numCur, &key);

            if (bCopySelected)
            {
                SelectKey(i + numCur, true);
            }

            // In-/Out-tangent
            keyNode->getAttr("ds", m_spline->key(i + numCur).ds);
            keyNode->getAttr("dd", m_spline->key(i + numCur).dd);
        }
        SortKeys();
    }
    else
    {
        int num = GetNumKeys();
        xmlNode->setAttr("TrackType", GetCurveType());

        I2DBezierKey key;
        for (int i = 0; i < num; i++)
        {
            GetKey(i, &key);
            assert(key.time == key.value.x);

            if (!bCopySelected || IsKeySelected(i))
            {
                XmlNodeRef keyNode = xmlNode->newChild("Key");
                keyNode->setAttr("time", key.time);
                keyNode->setAttr("value", key.value);

                int flags = key.flags;
                // Just save the in/out mask part. Others are for editing convenience.
                flags &= (SPLINE_KEY_TANGENT_IN_MASK | SPLINE_KEY_TANGENT_OUT_MASK);
                if (flags != 0)
                {
                    keyNode->setAttr("flags", flags);
                }

                // We also have to save in-/out-tangents, because TCB infos are not used for custom tangent keys.
                keyNode->setAttr("ds", m_spline->key(i).ds);
                keyNode->setAttr("dd", m_spline->key(i).dd);
            }
        }
    }
    return true;
}

//////////////////////////////////////////////////////////////////////////
template<>
inline void TAnimSplineTrack<Vec2>::GetKeyInfo(int index, const char*& description, float& duration)
{
    duration = 0;

    static char str[64];
    description = str;
    assert(index >= 0 && index < GetNumKeys());
    Spline::key_type& k = m_spline->key(index);
    sprintf_s(str, "%.2f", k.value.y);
}

#endif // CRYINCLUDE_CRYMOVIE_ANIMSPLINETRACK_VEC2SPECIALIZATION_H
