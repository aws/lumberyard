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

#include <CryExtension/ICryUnknown.h>
#include <Serialization/CryExtension.h>
#include <Serialization/CryStrings.h>

struct AnimEventInstance;
struct ICharacterInstance;
struct SRendParams;
struct SRenderingPassInfo;

// Defines parameters that are used by a custom event type
enum EAnimEventParameters
{
    ANIM_EVENT_USES_BONE = 1 << 0,
    ANIM_EVENT_USES_BONE_INLINE = 1 << 1,
    ANIM_EVENT_USES_OFFSET_AND_DIRECTION = 1 << 2,
    ANIM_EVENT_USES_ALL_PARAMETERS = ANIM_EVENT_USES_BONE | ANIM_EVENT_USES_OFFSET_AND_DIRECTION
};

struct SCustomAnimEventType
{
    const char* name;
    int parameters; // EAnimEventParameters
    const char* description; // used for tooltips in Character Tool
};

// Allows to preview different kinds of animevents within Character Tool
struct IAnimEventPlayer
    : public ICryUnknown
{
    CRYINTERFACE_DECLARE(IAnimEventPlayer, 0x2e2f7475542447f3, 0xb6729edb4a3af495)
    template<class T>
    friend void AZStd::checked_delete(T* x);

    // Can be used to customize parameter type for editing.
    virtual bool Play(ICharacterInstance* character, const AnimEventInstance& animEvent) = 0;
    virtual void Initialize() {}
    // Used when animation is being rewind
    virtual void Reset() {}
    virtual void Update(ICharacterInstance* character, const QuatT& playerPosition, const Matrix34& cameraMatrix) {}
    virtual void Render(const QuatT& playerPosition, SRendParams& params, const SRenderingPassInfo& passInfo) {}
    virtual void EnableAudio(bool enable) {}
    virtual void Serialize(Serialization::IArchive& ar) {}
    // Allows to customize editing of custom parameter of AnimEvent
    virtual const char* SerializeCustomParameter(const char* parameterValue, Serialization::IArchive& ar, int customTypeIndex) { return ""; }
    virtual const SCustomAnimEventType* GetCustomType(int customTypeIndex) const { return 0; }
    virtual int GetCustomTypeCount() const { return 0; }
};

DECLARE_SMART_POINTERS(IAnimEventPlayer);

inline bool Serialize(Serialization::IArchive& ar, IAnimEventPlayerPtr& pointer, const char* name, const char* label)
{
    Serialization::CryExtensionSharedPtr<IAnimEventPlayer, IAnimEventPlayer> serializer(pointer);
    return ar(static_cast<Serialization::IPointer&>(serializer), name, label);
}

