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

#include "pch.h"
#include "SceneContent.h"
#include <ICryAnimation.h>
#include <CryExtension/CryCreateClassInstance.h>
#include "Serialization.h"
#include "AnimationContent.h"

namespace CharacterTool {
    MotionParameters::MotionParameters()
    {
        for (size_t i = 0; i < eMotionParamID_COUNT; ++i)
        {
            values[i] = 0.0f;
            enabled[i] = false;
            rangeMin[i] = 0.0f;
            rangeMax[i] = 1.0f;
        }

        values[eMotionParamID_TravelSpeed] = 2.0f;
    }


    void MotionParameters::Serialize(IArchive& ar)
    {
        for (int i = 0; i < eMotionParamID_COUNT; ++i)
        {
            if (enabled[i])
            {
                SMotionParameterDetails details = { 0 };
                gEnv->pCharacterManager->GetMotionParameterDetails(details, EMotionParamID(i));
                ar(Serialization::Range(values[i], rangeMin[i], rangeMax[i]), details.name, details.humanReadableName);
            }
        }
    }

    // ---------------------------------------------------------------------------

    SERIALIZATION_ENUM_BEGIN_NESTED(AimParameters, AimDirection, "Aim Direction")
    SERIALIZATION_ENUM(AimParameters::AIM_CAMERA, "camera", "Camera")
    SERIALIZATION_ENUM(AimParameters::AIM_FORWARD, "forward", "Forward")
    SERIALIZATION_ENUM(AimParameters::AIM_TARGET, "target", "Target")
    SERIALIZATION_ENUM_END()

    static void SerializeSphericalAngle(IArchive& ar, float& radians, float range, const char* name, const char* label)
    {
        float degrees = RAD2DEG(radians);
        float oldValue = degrees;
        ar(Serialization::Range(degrees, -range, range), name, label);
        if (ar.IsInput() && oldValue != degrees)
        {
            radians = DEG2RAD(degrees);
        }
    }

    void AimParameters::Serialize(IArchive& ar)
    {
        ar(direction, "direction", "Direction");
        SerializeSphericalAngle(ar, offsetX, 180.0f, "offsetX", "X Offset");
        SerializeSphericalAngle(ar, offsetY, 90.0f, "offsetY", "Y Offset");
        ar(Range(smoothTime, 0.0f, 1.0f), "smoothTime", "Smooth Time");
        ar(Serialization::LocalToEntity(targetPosition), "target", direction == AIM_TARGET ? "Target Position" : 0);
    }

    // ---------------------------------------------------------------------------

    static const char* MakeStringPersistent(const char* str)
    {
        typedef std::set<string, stl::less_strcmp<string> > StringSet;
        static StringSet persistentStrings;
        StringSet::iterator it = persistentStrings.find(str);
        if (it != persistentStrings.end())
        {
            return it->c_str();
        }
        persistentStrings.insert(str);
        return persistentStrings.find(str)->c_str();
    }

    enum
    {
        FILTER_OVERRIDE_BLEND_SHAPE = 1
    };

    void BlendShapeSkin::Serialize(IArchive& ar)
    {
        for (size_t i = 0; i < params.size(); ++i)
        {
            const char* name = ar.IsEdit() ? MakeStringPersistent(params[i].name.c_str()) : params[i].name.c_str();
            if (!ar.IsEdit() || ar.Filter(FILTER_OVERRIDE_BLEND_SHAPE))
            {
                ar(Range(params[i].weight, 0.0f, 1.0f), name, name);
            }
            else if (ar.OpenBlock(name, name))
            {
                ar.CloseBlock();
            }
        }
    }

    void BlendShapeOptions::Serialize(IArchive& ar)
    {
        ar(overrideWeights, "overrideWeights", "Override Weights");
        if (overrideWeights)
        {
            ar.SetFilter(ar.GetFilter() | FILTER_OVERRIDE_BLEND_SHAPE);
        }
        for (size_t i = 0; i < skins.size(); ++i)
        {
            const char* name = ar.IsEdit() ? MakeStringPersistent(skins[i].name.c_str()) : skins[i].name.c_str();
            ar(skins[i], name, name);
        }
    }

    // ---------------------------------------------------------------------------

    SceneContent::SceneContent()
        : runFeatureTest(false)
        , aimLayer(-1)
        , lookLayer(-1)
    {
        ::CryCreateClassInstance<IAnimEventPlayer>("AnimEventPlayer_CharacterTool", animEventPlayer);
    }

    void SceneContent::Serialize(Serialization::IArchive& ar)
    {
        string oldCharacterPath = characterPath;
        ar(CharacterPath(characterPath), "character", "<Character");
        if (ar.IsInput() && oldCharacterPath != characterPath)
        {
            SignalCharacterChanged();
        }

        bool activeLayerSetToEmpty = false;
        layers.Serialize(&activeLayerSetToEmpty, ar);
        if (ar.IsInput() && activeLayerSetToEmpty)
        {
            SignalNewLayerActivated();
        }

        if (!blendShapeOptions.skins.empty())
        {
            ar(blendShapeOptions, "blendShapeOptions", "Blend Shapes");
        }
        else if (ar.OpenBlock("noBlendShapes", "No Blend Shapes"))
        {
            ar.CloseBlock();
        }

        ar(*animEventPlayer, "animEventPlayer", "AnimEvent Players");
        if (ar.IsInput())
        {
            SignalAnimEventPlayerTypeChanged();
        }

        if (ar.OpenBlock("featureTest", "+Run Feature Test"))
        {
            ar(runFeatureTest, "runFeatureTest", "^^");
            ar(featureTest, "featureTest", runFeatureTest ? "^" : 0);
            ar.CloseBlock();
        }
    }

    static bool SerializedContentChanged(std::vector<char>* buffer, const Serialization::SStruct& obj, int filter)
    {
        MemoryOArchive oa;
        oa.SetFilter(filter);
        oa(obj);

        if (oa.length() != buffer->size() || memcmp(oa.buffer(), buffer->data(), buffer->size()) != 0)
        {
            buffer->assign(oa.buffer(), oa.buffer() + oa.length());
            return true;
        }
        return false;
    }

    bool SceneContent::CheckIfPlaybackLayersChanged(bool continuous)
    {
        if (SerializedContentChanged(&lastLayersContent, Serialization::SStruct(layers), PlaybackLayer::SERIALIZE_LAYERS_ONLY))
        {
            PlaybackLayersChanged(continuous);
            return true;
        }
        return false;
    }

    void SceneContent::CharacterChanged()
    {
        SignalCharacterChanged();
    }

    void SceneContent::PlaybackLayersChanged(bool continuous)
    {
        aimLayer = -1;
        lookLayer = -1;
        for (size_t i = 0; i < layers.layers.size(); ++i)
        {
            const PlaybackLayer& layer = layers.layers[i];
            if (!layer.enabled)
            {
                continue;
            }
            if (layer.type == AnimationContent::AIMPOSE)
            {
                aimLayer = layer.layerId;
            }
            if (layer.type == AnimationContent::LOOKPOSE)
            {
                lookLayer = layer.layerId;
            }
        }

        SignalPlaybackLayersChanged(continuous);
    }

    void SceneContent::LayerActivated()
    {
        SignalLayerActivated();
    }

    AimParameters& SceneContent::GetAimParameters()
    {
        if (aimLayer >= 0 && aimLayer < layers.layers.size())
        {
            return layers.layers[aimLayer].aim;
        }
        static AimParameters defaultResult;
        return defaultResult;
    }

    AimParameters& SceneContent::GetLookParameters()
    {
        if (lookLayer >= 0 && lookLayer < layers.layers.size())
        {
            return layers.layers[lookLayer].aim;
        }
        static AimParameters defaultResult;
        return defaultResult;
    }

    MotionParameters& SceneContent::GetMotionParameters()
    {
        if (!layers.layers.empty())
        {
            return layers.layers[0].motionParameters;
        }
        static MotionParameters defaultResult;
        return defaultResult;
    }
}

#include <CharacterTool/SceneContent.moc>
