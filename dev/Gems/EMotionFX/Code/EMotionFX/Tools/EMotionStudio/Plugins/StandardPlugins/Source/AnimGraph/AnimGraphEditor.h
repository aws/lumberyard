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

#include <EMotionFX/Source/AnimGraphObject.h>
#include <Editor/AnimGraphEditorBus.h>
#include <QWidget>


QT_FORWARD_DECLARE_CLASS(QComboBox)
QT_FORWARD_DECLARE_CLASS(QPushButton)

namespace AzToolsFramework
{
    class ReflectedPropertyEditor;
}

namespace EMotionFX
{
    class AnimGraph;

    class AnimGraphEditor
        : public QWidget
        , public EMotionFX::AnimGraphEditorRequestBus::Handler
        , public EMotionFX::AnimGraphEditorNotificationBus::Handler
    {
        Q_OBJECT

    public:
        AnimGraphEditor(EMotionFX::AnimGraph* animGraph, AZ::SerializeContext* serializeContext, QWidget* parent);
        virtual ~AnimGraphEditor();

        // AnimGraphEditorRequests
        EMotionFX::MotionSet* GetSelectedMotionSet() override;

        // AnimGraphEditorNotifications
        void UpdateMotionSetComboBox() override;

    private slots:
        void OnMotionSetChanged(int index);

    private:
        AZ::Outcome<uint32> GetMotionSetIndex(int comboBoxIndex) const;

        AzToolsFramework::ReflectedPropertyEditor*  m_propertyEditor;
        static const int                            m_propertyLabelWidth;
        QComboBox*                                  m_motionSetComboBox;
        static QString                              m_lastMotionSetText;

    };
} // namespace EMotionFX