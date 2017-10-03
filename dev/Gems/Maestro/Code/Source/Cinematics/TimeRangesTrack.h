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

#ifndef CRYINCLUDE_CRYMOVIE_TIMERANGESTRACK_H
#define CRYINCLUDE_CRYMOVIE_TIMERANGESTRACK_H
#pragma once


//forward declarations.
#include "IMovieSystem.h"
#include "AnimTrack.h"

/** CTimeRangesTrack contains keys that represent generic time ranges
*/
class CTimeRangesTrack
    : public TAnimTrack<ITimeRangeKey>
{
public:
    // IAnimTrack.
    void GetKeyInfo(int key, const char*& description, float& duration);
    bool Serialize(XmlNodeRef& xmlNode, bool bLoading, bool bLoadEmptyTracks);
    void SerializeKey(ITimeRangeKey& key, XmlNodeRef& keyNode, bool bLoading);

    //! Gets the duration of an animation key.
    float GetKeyDuration(int key) const;

    virtual void GetMemoryUsage(ICrySizer* pSizer) const
    {
        pSizer->AddObject(this, sizeof(*this));
    }

    int GetActiveKeyIndexForTime(const float time);
};

#endif // CRYINCLUDE_CRYMOVIE_TIMERANGESTRACK_H
