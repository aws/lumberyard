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

#include <vector>
#include <QObject>
#include "FeatureTest.h"
#include "PlaybackLayers.h"

#include <IAnimEventPlayer.h>

namespace CharacterTool {
    using std::vector;

    struct IFeatureTest;

    struct BlendShapeParameter
    {
        string name;
        float weight;
        bool operator<(const BlendShapeParameter& rhs) const{ return name < rhs.name; }

        BlendShapeParameter()
            : weight(1.0f)
        {
        }
    };
    typedef vector<BlendShapeParameter> BlendShapeParameters;

    struct BlendShapeSkin
    {
        string name;
        BlendShapeParameters params;

        void Serialize(IArchive& ar);
        bool operator<(const BlendShapeSkin& rhs) const{ return name < rhs.name; }
    };
    typedef vector<BlendShapeSkin> BlendShapeSkins;

    struct BlendShapeOptions
    {
        bool overrideWeights;
        BlendShapeSkins skins;

        BlendShapeOptions()
            : overrideWeights(false)
        {
        }

        void Serialize(IArchive& ar);
    };




    struct SceneContent
        : public QObject
    {
        Q_OBJECT
    public:

        string characterPath;
        PlaybackLayers layers;
        int aimLayer;
        int lookLayer;
        BlendShapeOptions blendShapeOptions;
        IAnimEventPlayerPtr animEventPlayer;

        bool runFeatureTest;
        _smart_ptr<IFeatureTest> featureTest;

        std::vector<char> lastLayersContent;
        std::vector<char> lastContent;

        SceneContent();
        void Serialize(Serialization::IArchive& ar);
        bool CheckIfPlaybackLayersChanged(bool continuous);
        void CharacterChanged();
        void PlaybackLayersChanged(bool continuous);
        void LayerActivated();
        void MotionParametersChanged(bool continuousChange);
        AimParameters& GetAimParameters();
        AimParameters& GetLookParameters();
        MotionParameters& GetMotionParameters();
    signals:
        void SignalAnimEventPlayerTypeChanged();
        void SignalCharacterChanged();
        void SignalPlaybackLayersChanged(bool continuous);
        void SignalLayerActivated();
        void SignalBlendShapeOptionsChanged();
        void SignalNewLayerActivated();

        void SignalChanged(bool continuous);
    };
}
