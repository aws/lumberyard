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

#include <EMotionFX/Tools/EMotionStudio/EMStudioSDK/Source/DockWidgetPlugin.h>
#include <Editor/Plugins/SkeletonOutliner/SkeletonOutlinerBus.h>


QT_FORWARD_DECLARE_CLASS(QVBoxLayout)

namespace EMotionFX
{
    class Node;
    class ClothJointWidget;

    class ClothJointInspectorPlugin
        : public EMStudio::DockWidgetPlugin
        , private EMotionFX::SkeletonOutlinerNotificationBus::Handler
    {
        Q_OBJECT //AUTOMOC

    public:
        ClothJointInspectorPlugin();
        ~ClothJointInspectorPlugin();

        // EMStudioPlugin overrides
        const char* GetName() const override                { return "Cloth"; }
        uint32 GetClassID() const override                  { return AZ_CRC("ClothJointInspectorPlugin", 0x8efd2bee); }
        bool GetIsClosable() const override                 { return true;  }
        bool GetIsFloatable() const override                { return true;  }
        bool GetIsVertical() const override                 { return false; }
        bool Init() override;
        EMStudioPlugin* Clone() override;

        // SkeletonOutlinerNotificationBus overrides
        void OnContextMenu(QMenu* menu, const QModelIndexList& selectedRowIndices) override;

        void Render(EMStudio::RenderPlugin* renderPlugin, RenderInfo* renderInfo) override;

    public slots:
        void OnAddCollider();
        void OnClearColliders();

        // QDockWidget overrides
        void OnVisibilityChanged(bool visible);

    private:
        ClothJointWidget* m_jointWidget;
    };
} // namespace EMotionFX