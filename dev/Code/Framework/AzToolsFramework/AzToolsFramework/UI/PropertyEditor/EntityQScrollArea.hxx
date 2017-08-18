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

#ifndef ENTITY_QSCROLLAREA_HXX
#define ENTITY_QSCROLLAREA_HXX

#include <AzCore/base.h>
#include <AzCore/Component/ComponentBus.h>
#include <AzCore/Memory/SystemAllocator.h>

#include <QtWidgets/QScrollArea>

#pragma once

namespace AZ
{
    namespace Data
    {
        struct AssetId;
    }
    struct Uuid;
}

namespace AzToolsFramework
{
    class EntityQScrollArea
        : public QScrollArea
    {
        Q_OBJECT
    public:
        AZ_CLASS_ALLOCATOR(EntityQScrollArea, AZ::SystemAllocator, 0);

        explicit EntityQScrollArea(QWidget* parent = 0);
        ~EntityQScrollArea() override;

    protected:
        void dragEnterEvent(QDragEnterEvent* event) override;
        void dropEvent(QDropEvent* event) override;
    private:
        void AddComponent(AZ::Data::AssetId assetId, AZ::Uuid componentType) const;
    };
}

#endif
