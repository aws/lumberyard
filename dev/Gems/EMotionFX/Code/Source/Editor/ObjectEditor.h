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


#include <AzCore/RTTI/TypeInfo.h>
#include <QFrame>
#include <QWidget>


// Forward declarations
namespace AZ { class SerializeContext; }
namespace AzToolsFramework
{
    class ReflectedPropertyEditor;
    class IPropertyEditorNotify;
}

namespace EMotionFX
{
    class ObjectEditor
        : public QFrame
    {
        Q_OBJECT

    public:
        ObjectEditor(AZ::SerializeContext* serializeContext, QWidget* parent = nullptr);
        ObjectEditor(AZ::SerializeContext* serializeContext, AzToolsFramework::IPropertyEditorNotify* notify, QWidget* parent = nullptr);
        ~ObjectEditor();

        void AddInstance(void* object, const AZ::TypeId& objectTypeId);
        void ClearInstances(bool invalidateImmediately);

        void InvalidateAll();
        void InvalidateValues();

        void* GetObject() { return m_object; }

    private:
        void*                                       m_object;
        AzToolsFramework::ReflectedPropertyEditor*  m_propertyEditor;
        static const int                            m_propertyLabelWidth;
    };
} // namespace EMotionFX
