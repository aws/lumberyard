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

namespace Serialization {
    class IArchive;
}

struct SkeletonAlias;

bool Serialize(Serialization::IArchive& ar, SkeletonAlias& value, const char* name, const char* label);

#include <Serialization/STL.h>
#include <Serialization/Decorators/Range.h>
#include <Serialization/Decorators/OutputFilePath.h>
using Serialization::OutputFilePath;
#include <Serialization/Decorators/ResourceFilePath.h>
using Serialization::ResourceFilePath;
#include <Serialization/Decorators/JointName.h>
using Serialization::JointName;

#include <Serialization/IArchive.h>

#include <Serialization/SmartPtr.h>
#include <Serialization/STLImpl.h>
#include <Serialization/SmartPtrImpl.h>
#include <Serialization/Decorators/SliderImpl.h>
#include <Serialization/Decorators/OutputFilePathImpl.h>
#include <Serialization/Decorators/ResourceFilePathImpl.h>
#include <Serialization/Decorators/JointNameImpl.h>

#include <Serialization/ClassFactory.h>
#include <Serialization/Enum.h>
using Serialization::IArchive;
