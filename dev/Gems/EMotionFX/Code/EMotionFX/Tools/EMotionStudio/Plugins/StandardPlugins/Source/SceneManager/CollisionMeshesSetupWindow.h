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
#include <MCore/Source/StandardHeaders.h>
#include <EMotionFX/CommandSystem/Source/SelectionCommands.h>
#include "CollisionMeshesNodeHierarchyWidget.h"
#include <QDialog>


namespace EMStudio
{
    class CollisionMeshesSetupWindow
        : public QDialog
    {
        Q_OBJECT
                 MCORE_MEMORYOBJECTCATEGORY(CollisionMeshesSetupWindow, MCore::MCORE_DEFAULT_ALIGNMENT, MEMCATEGORY_STANDARDPLUGINS)

    public:
        CollisionMeshesSetupWindow(QWidget* parent);
        virtual ~CollisionMeshesSetupWindow();

        void Update(uint32 actorInstanceID)             { mHierarchyWidget->Update(actorInstanceID); }

    public slots:
        void OnAccept();

    private:
        CollisionMeshesNodeHierarchyWidget* mHierarchyWidget;
    };
} // namespace EMStudio
