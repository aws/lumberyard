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

#ifndef CRYINCLUDE_CRYMOVIE_GOTOTRACK_H
#define CRYINCLUDE_CRYMOVIE_GOTOTRACK_H

#pragma once

#include "IMovieSystem.h"
#include "AnimTrack.h"

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
/** Goto track, every key on this track negates boolean value.
*/
class CGotoTrack
    : public TAnimTrack<IDiscreteFloatKey>
{
public:
    CGotoTrack();

    virtual EAnimValue GetValueType() { return eAnimValue_DiscreteFloat; }

    void GetValue(float time, float& value, bool applyMultiplier=false);
    void SetValue(float time, const float& value, bool bDefault = false, bool applyMultiplier = false);

    void SerializeKey(IDiscreteFloatKey& key, XmlNodeRef& keyNode, bool bLoading);
    void GetKeyInfo(int key, const char*& description, float& duration);

    virtual void GetMemoryUsage(ICrySizer* pSizer) const
    {
        pSizer->AddObject(this, sizeof(*this));
    }
protected:
    void SetKeyAtTime(float time, IKey* key);

    float m_DefaultValue;
};

#endif // CRYINCLUDE_CRYMOVIE_GOTOTRACK_H
