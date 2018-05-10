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

#pragma once

#include "../StandardPluginsConfig.h"
#include <AzCore/std/containers/vector.h>
#include <EMotionFX/Source/MorphTarget.h>
#include <EMotionFX/Source/MorphSetupInstance.h>
#include <EMotionFX/Source/ActorInstance.h>
#include <MysticQt/Source/IntSlider.h>
#include "MorphTargetEditWindow.h"
#include <QWidget>
#include <QCheckBox>


namespace EMStudio
{
    class MorphTargetGroupWidget
        : public QWidget
    {
        Q_OBJECT
        MCORE_MEMORYOBJECTCATEGORY(MorphTargetGroupWidget, MCore::MCORE_DEFAULT_ALIGNMENT, MEMCATEGORY_STANDARDPLUGINS);

    public:
        MorphTargetGroupWidget(const char* name, EMotionFX::ActorInstance* actorInstance, const AZStd::vector<EMotionFX::MorphTarget*>& morphTargets, const AZStd::vector<EMotionFX::MorphSetupInstance::MorphTarget*>& morphTargetInstances, QWidget* parent);
        ~MorphTargetGroupWidget();

        void UpdateInterface();
        void UpdateMorphTarget(const char* name);

    public slots:
        void SetManualModeForAll(int value);
        void ManualModeClicked();
        void SliderWeightMoved(float value);
        void SliderWeightReleased(float value);
        void EditClicked();
        void ResetAll();

    private:
        struct MorphTarget
        {
            EMotionFX::MorphTarget*                     mMorphTarget;
            EMotionFX::MorphSetupInstance::MorphTarget* mMorphTargetInstance;
            QCheckBox*                                  mManualMode;
            MysticQt::FloatSlider*                      mSliderWeight;
            float                                       mOldWeight;
        };

    private:
        AZStd::string               mName;
        EMotionFX::ActorInstance*   mActorInstance;
        QCheckBox*                  mSelectAll;
        AZStd::vector<MorphTarget>  mMorphTargets;
        MorphTargetEditWindow*      mEditWindow;
    };
} // namespace EMStudio