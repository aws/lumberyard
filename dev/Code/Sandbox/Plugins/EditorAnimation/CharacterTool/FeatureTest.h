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
#include <Cry_Geo.h>
#include <Cry_Camera.h>
#include <smartptr.h>

struct SRenderContext;
struct AnimEventInstance;

namespace Serialization {
    class IArchive;
}

namespace CharacterTool
{
    class CharacterDocument;
    using Serialization::IArchive;

    struct IFeatureTest
        : public _i_reference_target_t
    {
        virtual void Update(CharacterDocument* document) {}
        // Tell if regular Character Tool update should be skipped
        virtual bool OverrideUpdate() const { return false; }
        virtual bool OverrideCameraPosition(CCamera* camera) { return false; }

        virtual void Render(const SRenderContext& x, CharacterDocument* document) {}
        virtual bool AnimEvent(const AnimEventInstance& event, CharacterDocument* document) { return false; }
        virtual void Serialize(IArchive& ar) {}
    };
}
