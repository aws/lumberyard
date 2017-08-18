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
#include <Cry_Color.h>

namespace Serialization {
    class IArchive;
    struct SStruct;
}

struct SkeletonAlias;
bool Serialize(Serialization::IArchive& ar, SkeletonAlias& value, const char* name, const char* label);

enum
{
    SERIALIZE_STATE = 1 << 0,
    SERIALIZE_LAYOUT = 1 << 1,
};

#include <Serialization/STL.h>
#include <Serialization/Math.h>
#include <Serialization/Decorators/Slider.h>
#include <Serialization/Color.h>
#include <Serialization/Decorators/Resources.h>
#include <Serialization/Decorators/ResourceFilePath.h>
#include <Serialization/Decorators/ResourceFolderPath.h>
#include <Serialization/Decorators/JointName.h>
#include <Serialization/Decorators/BitFlags.h>
#include <Serialization/Decorators/Slider.h>
#include <Serialization/Decorators/Range.h>
#include <Serialization/Decorators/TagList.h>
#include <Serialization/Decorators/INavigationProvider.h>
#include <Serialization/Decorators/LocalFrame.h>
#include <Serialization/Decorators/ToggleButton.h>
#include <Serialization/SmartPtr.h>
#include <Serialization/ClassFactory.h>
#include <Serialization/Enum.h>
#include <Serialization/IArchive.h>

// from ../EditorCommon/Serialization
#include "Serialization/BinArchive.h"
#include "Serialization/JSONIArchive.h"
#include "Serialization/JSONOArchive.h"
#include "Serialization/Qt.h"

namespace CharacterTool
{
    using Serialization::IArchive;
    using Serialization::AttachmentName;
    using Serialization::ResourceFilePath;
    using Serialization::MaterialPath;
    using Serialization::SkeletonPath;
    using Serialization::SkeletonParamsPath;
    using Serialization::AnimationAlias;
    using Serialization::AnimationPath;
    using Serialization::AnimationPathWithId;
    using Serialization::CharacterPath;
    using Serialization::CharacterPhysicsPath;
    using Serialization::CharacterRigPath;
    using Serialization::ResourceFolderPath;
    using Serialization::JointName;
    using Serialization::BitFlags;
    using Serialization::Decorators::Slider;
    using Serialization::Decorators::Range;
    using Serialization::LocalToJoint;
    using Serialization::ToggleButton;

#if 0 // useful for debugging
    typedef Serialization::JSONOArchive MemoryOArchive;
    typedef Serialization::JSONIArchive MemoryIArchive;
#else
    typedef Serialization::BinOArchive MemoryOArchive;
    typedef Serialization::BinIArchive MemoryIArchive;
#endif

    void SerializeToMemory(std::vector<char>* buffer, const Serialization::SStruct& obj);
    void SerializeToMemory(DynArray<char>* buffer, const Serialization::SStruct& obj);
    void SerializeFromMemory(const Serialization::SStruct& outObj, const std::vector<char>& buffer);
    void SerializeFromMemory(const Serialization::SStruct& outObj, const DynArray<char>& buffer);
}
