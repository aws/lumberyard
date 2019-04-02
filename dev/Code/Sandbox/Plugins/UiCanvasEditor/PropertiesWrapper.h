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

#include <QWidget>

class EditorWindow;
class PropertiesWidget;
class HierarchyWidget;

class PropertiesWrapper
    : public QWidget
{
    Q_OBJECT

public:

    PropertiesWrapper(HierarchyWidget* hierarchy,
        EditorWindow* parent);

    PropertiesWidget* GetProperties();

    void ActiveCanvasChanged();

private:

    PropertiesWidget* m_properties;
    ComponentButton* m_componentButton;
    EditorWindow* m_editorWindow;
    QCheckBox* m_editorOnlyCheckbox = nullptr;
};
