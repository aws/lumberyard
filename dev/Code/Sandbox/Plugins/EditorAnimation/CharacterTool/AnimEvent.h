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

#pragma once

#include <Cry_Math.h>
#include "Serialization/Strings.h"

class XmlNodeRef;
class CAnimEventData;
struct AnimEventInstance;

namespace Serialization {
    class IArchive;
}

namespace CharacterTool
{
    using Serialization::string;

    struct AnimEvent
    {
        float startTime;
        float endTime;
        string type;
        string parameter;
        string boneName;
        string model;
        Vec3 offset;
        Vec3 direction;

        AnimEvent()
            : startTime(0.0f)
            , endTime(0.0f)
            , type("audio_trigger")
            , offset(ZERO)
            , direction(ZERO)
        {
        }

        bool operator<(const AnimEvent& animEvent) const { return startTime < animEvent.startTime; }

        void Serialize(Serialization::IArchive& ar);
        void FromData(const CAnimEventData& eventData);
        void ToData(CAnimEventData* eventData) const;
        void ToInstance(AnimEventInstance* instance) const;
        bool LoadFromXMLNode(const XmlNodeRef& node);
    };
    typedef std::vector<AnimEvent> AnimEvents;

    struct AnimEventPreset
    {
        string name;
        float colorHue;
        AnimEvent event;

        AnimEventPreset()
            : name("Preset")
            , colorHue(0.66f) {}

        void Serialize(Serialization::IArchive& ar);
    };
    typedef std::vector<AnimEventPreset> AnimEventPresets;

    bool IsAudioEventType(const char* audioEventType);
}
