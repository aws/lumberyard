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

#include <MCore/Source/StandardHeaders.h>
#include <EMotionFX/Source/AnimGraphTransitionCondition.h>
#include "../StandardPluginsConfig.h"
#include <QDialog>

QT_FORWARD_DECLARE_CLASS(QListWidget)
QT_FORWARD_DECLARE_CLASS(QListWidgetItem)


namespace EMStudio
{
    // forward declarations
    class AnimGraphPlugin;

    class ConditionSelectDialog
        : public QDialog
    {
        Q_OBJECT
        MCORE_MEMORYOBJECTCATEGORY(ConditionSelectDialog, MCore::MCORE_DEFAULT_ALIGNMENT, MEMCATEGORY_STANDARDPLUGINS_ANIMGRAPH);

    public:
        ConditionSelectDialog(QWidget* parent);
        ~ConditionSelectDialog();

        AZ::TypeId GetSelectedConditionType() const { return m_selectedTypeId; }

    protected slots:
        void OnCreateButton();
        void OnItemDoubleClicked(QListWidgetItem* item);

    private:
        QListWidget*              m_listBox;
        AZStd::vector<AZ::TypeId> m_typeIds;
        AZ::TypeId                m_selectedTypeId;
    };
} // namespace EMStudio