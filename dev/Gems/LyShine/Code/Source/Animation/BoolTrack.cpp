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

#include "LyShine_precompiled.h"
#include "BoolTrack.h"
#include <AzCore/Serialization/SerializeContext.h>
#include "UiAnimSerialize.h"

//////////////////////////////////////////////////////////////////////////
UiBoolTrack::UiBoolTrack()
    : m_bDefaultValue(true)
{
}

//////////////////////////////////////////////////////////////////////////
void UiBoolTrack::GetKeyInfo(int index, const char*& description, float& duration)
{
    description = 0;
    duration = 0;
}

//////////////////////////////////////////////////////////////////////////
void UiBoolTrack::GetValue(float time, bool& value)
{
    value = m_bDefaultValue;

    CheckValid();

    int nkeys = m_keys.size();
    if (nkeys < 1)
    {
        return;
    }

    int key = 0;
    while ((key < nkeys) && (time >= m_keys[key].time))
    {
        key++;
    }

    if (m_bDefaultValue)
    {
        value = !(key & 1); // True if even key.
    }
    else
    {
        value = (key & 1);  // False if even key.
    }
}

//////////////////////////////////////////////////////////////////////////
void UiBoolTrack::SetValue(float time, const bool& value, bool bDefault)
{
    Invalidate();
}

//////////////////////////////////////////////////////////////////////////
void UiBoolTrack::SetDefaultValue(const bool bDefaultValue)
{
    m_bDefaultValue = bDefaultValue;
}

//////////////////////////////////////////////////////////////////////////
template<>
inline void TUiAnimTrack<IBoolKey>::Reflect(AZ::SerializeContext* serializeContext)
{
    serializeContext->Class<TUiAnimTrack<IBoolKey> >()
        ->Version(1)
        ->Field("Flags", &TUiAnimTrack<IBoolKey>::m_flags)
        ->Field("Range", &TUiAnimTrack<IBoolKey>::m_timeRange)
        ->Field("ParamType", &TUiAnimTrack<IBoolKey>::m_nParamType)
        ->Field("ParamData", &TUiAnimTrack<IBoolKey>::m_componentParamData)
        ->Field("Keys", &TUiAnimTrack<IBoolKey>::m_keys);
}

//////////////////////////////////////////////////////////////////////////
void UiBoolTrack::Reflect(AZ::SerializeContext* serializeContext)
{
    TUiAnimTrack<IBoolKey>::Reflect(serializeContext);

    serializeContext->Class<UiBoolTrack, TUiAnimTrack<IBoolKey> >()
        ->Version(1)
        ->SerializerForEmptyClass();
}
