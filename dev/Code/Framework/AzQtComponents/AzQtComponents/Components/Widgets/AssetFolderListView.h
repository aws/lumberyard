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

#include <AzQtComponents/AzQtComponentsAPI.h>
AZ_PUSH_DISABLE_WARNING(4251, "-Wunknown-warning-option") // 4251: 'AzQtComponents::ScrollBar::s_scrollBarWatcher': class 'QPointer<AzQtComponents::ScrollBarWatcher>' needs to have dll-interface to be used by clients of class 'AzQtComponents::ScrollBar'
#include <AzQtComponents/Components/Widgets/TableView.h>
AZ_POP_DISABLE_WARNING

class QAbstractItemModel;

namespace AzQtComponents
{
    class AZ_QT_COMPONENTS_API AssetFolderListView
        : public TableView
    {
        Q_OBJECT
    public:
        explicit AssetFolderListView(QWidget* parent = nullptr);

        void setModel(QAbstractItemModel* model) override;
    };
}
