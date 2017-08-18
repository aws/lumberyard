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

#ifndef CRYINCLUDE_CRYMOVIE_MUSICTRACK_H
#define CRYINCLUDE_CRYMOVIE_MUSICTRACK_H
#pragma once

//forward declarations.
#include "IMovieSystem.h"
#include "AnimTrack.h"
#include "AnimKey.h"

/** MusicTrack contains music keys, when time reach event key, it applies changes to the music-system...
*/
class CMusicTrack
    : public TAnimTrack<IMusicKey>
{
public:
    //////////////////////////////////////////////////////////////////////////
    // Overrides of IAnimTrack.
    //////////////////////////////////////////////////////////////////////////
    void GetKeyInfo(int key, const char*& description, float& duration);
    void SerializeKey(IMusicKey& key, XmlNodeRef& keyNode, bool bLoading);

    //! Check if track is masked
    virtual bool IsMasked(const uint32 mask) const { return (mask & eTrackMask_MaskMusic) != 0; }

    virtual void GetMemoryUsage(ICrySizer* pSizer) const
    {
        pSizer->AddObject(this, sizeof(*this));
    }

    bool UsesMute() const override { return true; }
};

#endif // CRYINCLUDE_CRYMOVIE_MUSICTRACK_H
