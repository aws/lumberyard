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

#include <Editor/ObjectEditor.h>
#include <AzToolsFramework/UI/PropertyEditor/ReflectedPropertyEditor.hxx>
#include <QVBoxLayout>


namespace EMotionFX
{
    const int ObjectEditor::m_propertyLabelWidth = 160;

    ObjectEditor::ObjectEditor(AZ::SerializeContext* serializeContext, QWidget* parent)
        : QFrame(parent)
        , m_propertyEditor(nullptr)
    {
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);

        m_propertyEditor = aznew AzToolsFramework::ReflectedPropertyEditor(this);
        m_propertyEditor->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);
        m_propertyEditor->setObjectName("PropertyEditor");
        m_propertyEditor->Setup(serializeContext, nullptr, false/*enableScrollbars*/, m_propertyLabelWidth);

        QVBoxLayout* mainLayout = new QVBoxLayout(this);
        mainLayout->setSizeConstraint(QLayout::SetMinimumSize);
        mainLayout->setMargin(0);
        mainLayout->setContentsMargins(0, 0, 0, 0);
        mainLayout->addWidget(m_propertyEditor);
        setLayout(mainLayout);
    }


    ObjectEditor::~ObjectEditor()
    {
    }


    void ObjectEditor::AddInstance(void* object, const AZ::TypeId& objectTypeId)
    {
        m_propertyEditor->AddInstance(object, objectTypeId);
        m_propertyEditor->InvalidateAll();
    }


    void ObjectEditor::ClearInstances(bool invalidateImmediately)
    {
        m_propertyEditor->ClearInstances();
        if (invalidateImmediately)
        {
            m_propertyEditor->InvalidateAll();
        }
    }
} // namespace EMotionFX

#include <Source/Editor/ObjectEditor.moc>