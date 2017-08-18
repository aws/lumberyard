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

// Description : Definitions for various modifiers to target tracks


#ifndef CRYINCLUDE_CRYAISYSTEM_TARGETSELECTION_TARGETTRACKMODIFIERS_H
#define CRYINCLUDE_CRYAISYSTEM_TARGETSELECTION_TARGETTRACKMODIFIERS_H
#pragma once

#include "TargetTrackCommon.h"

struct SStimulusInvocation;

struct ITargetTrackModifier
{
    virtual ~ITargetTrackModifier()
    {
    }

    virtual uint32 GetUniqueId() const = 0;

    // Returns if this modifier matches the given xml tag
    virtual bool IsMatchingTag(const char* szTag) const = 0;
    virtual char const* GetTag() const = 0;

    // Returns the modifier value
    virtual float GetModValue(const CTargetTrack* pTrack, TargetTrackHelpers::EAIEventStimulusType stimulusType,
        const Vec3& vPos, const TargetTrackHelpers::SEnvelopeData& envelopeData,
        const TargetTrackHelpers::STargetTrackModifierConfig& modConfig) const = 0;
};

class CTargetTrackDistanceModifier
    : public ITargetTrackModifier
{
public:
    CTargetTrackDistanceModifier();
    virtual ~CTargetTrackDistanceModifier();

    enum
    {
        UNIQUE_ID = 1
    };
    virtual uint32 GetUniqueId() const { return UNIQUE_ID; }

    virtual bool IsMatchingTag(const char* szTag) const;
    virtual char const* GetTag() const;
    virtual float GetModValue(const CTargetTrack* pTrack, TargetTrackHelpers::EAIEventStimulusType stimulusType,
        const Vec3& vPos, const TargetTrackHelpers::SEnvelopeData& envelopeData,
        const TargetTrackHelpers::STargetTrackModifierConfig& modConfig) const;
};

class CTargetTrackHostileModifier
    : public ITargetTrackModifier
{
public:
    CTargetTrackHostileModifier();
    virtual ~CTargetTrackHostileModifier();

    enum
    {
        UNIQUE_ID = 2
    };
    virtual uint32 GetUniqueId() const { return UNIQUE_ID; }

    virtual bool IsMatchingTag(const char* szTag) const;
    virtual char const* GetTag() const;
    virtual float GetModValue(const CTargetTrack* pTrack, TargetTrackHelpers::EAIEventStimulusType stimulusType,
        const Vec3& vPos, const TargetTrackHelpers::SEnvelopeData& envelopeData,
        const TargetTrackHelpers::STargetTrackModifierConfig& modConfig) const;
};

class CTargetTrackClassThreatModifier
    : public ITargetTrackModifier
{
public:
    CTargetTrackClassThreatModifier();
    virtual ~CTargetTrackClassThreatModifier();

    enum
    {
        UNIQUE_ID = 3
    };
    virtual uint32 GetUniqueId() const { return UNIQUE_ID; }

    virtual bool IsMatchingTag(const char* szTag) const;
    virtual char const* GetTag() const;
    virtual float GetModValue(const CTargetTrack* pTrack, TargetTrackHelpers::EAIEventStimulusType stimulusType,
        const Vec3& vPos, const TargetTrackHelpers::SEnvelopeData& envelopeData,
        const TargetTrackHelpers::STargetTrackModifierConfig& modConfig) const;
};

class CTargetTrackDistanceIgnoreModifier
    : public ITargetTrackModifier
{
public:
    CTargetTrackDistanceIgnoreModifier();
    virtual ~CTargetTrackDistanceIgnoreModifier();

    enum
    {
        UNIQUE_ID = 4
    };
    virtual uint32 GetUniqueId() const { return UNIQUE_ID; }

    virtual bool IsMatchingTag(const char* szTag) const;
    virtual char const* GetTag() const;
    virtual float GetModValue(const CTargetTrack* pTrack, TargetTrackHelpers::EAIEventStimulusType stimulusType,
        const Vec3& vPos, const TargetTrackHelpers::SEnvelopeData& envelopeData,
        const TargetTrackHelpers::STargetTrackModifierConfig& modConfig) const;
};

class CTargetTrackPlayerModifier
    : public ITargetTrackModifier
{
public:
    enum
    {
        UNIQUE_ID = 5
    };
    virtual uint32 GetUniqueId() const { return UNIQUE_ID; }

    virtual bool IsMatchingTag(const char* szTag) const;
    virtual char const* GetTag() const;
    virtual float GetModValue(const CTargetTrack* pTrack, TargetTrackHelpers::EAIEventStimulusType stimulusType,
        const Vec3& vPos, const TargetTrackHelpers::SEnvelopeData& envelopeData,
        const TargetTrackHelpers::STargetTrackModifierConfig& modConfig) const;
};

#endif // CRYINCLUDE_CRYAISYSTEM_TARGETSELECTION_TARGETTRACKMODIFIERS_H
