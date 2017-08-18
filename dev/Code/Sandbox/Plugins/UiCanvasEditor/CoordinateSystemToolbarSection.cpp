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
#include "stdafx.h"

#include "EditorCommon.h"

namespace
{
    void SetCoordinateSystemFromCombobox(EditorWindow* editorWindow, QComboBox* combobox, int newIndex)
    {
        ViewportInteraction::CoordinateSystem s = (ViewportInteraction::CoordinateSystem)combobox->itemData(newIndex).toInt();

        editorWindow->GetViewport()->GetViewportInteraction()->SetCoordinateSystem(s);
    }
} // anonymous namespace.

CoordinateSystemToolbarSection::CoordinateSystemToolbarSection(QToolBar* parent, bool addSeparator)
    : QObject()
    , m_editorWindow(dynamic_cast<EditorWindow*>(parent->parent()))
    , m_combobox(new QComboBox(parent))
    , m_snapCheckbox(new QCheckBox("Snap to grid", parent))
{
    AZ_Assert(m_editorWindow, "Invalid hierarchy of windows.");

    m_combobox->setToolTip(QString("Reference coordinate system (%1)").arg(UICANVASEDITOR_COORDINATE_SYSTEM_CYCLE_SHORTCUT_KEY_SEQUENCE.toString()));
    m_combobox->setMinimumContentsLength(5);

    // Combobox.
    {
        parent->addWidget(m_combobox);

        for (const auto& s : ViewportInteraction::CoordinateSystem())
        {
            m_combobox->addItem(ViewportHelpers::CoordinateSystemToString((int)s), (int)s);
        }

        QObject::connect(m_combobox,
            static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),                 // IMPORTANT: We HAVE to use static_cast<>() to specify which overload of QComboBox::currentIndexChanged() we want.
            [this](int index)
            {
                SetCoordinateSystemFromCombobox(m_editorWindow, m_combobox, index);
            });

        QObject::connect(m_editorWindow,
            SIGNAL(SignalCoordinateSystemCycle()),
            SLOT(HandleCoordinateSystemCycle()));
    }

    // Snap to grid checkbox.
    {
        parent->addSeparator();

        m_snapCheckbox->setToolTip(QString("Toggle snap to grid (%1)").arg(UICANVASEDITOR_SNAP_TO_GRID_TOGGLE_SHORTCUT_KEY_SEQUENCE.toString()));

        // IMPORTANT: The MainToolbar is creater BEFORE the canvas is loaded.
        // The checked state of m_snapCheckbox will be set by
        // SetSnapToGridIsChecked() after the canvas is loaded.

        QObject::connect(m_snapCheckbox,
            &QCheckBox::stateChanged,
            [this](int state)
            {
                EBUS_EVENT_ID(m_editorWindow->GetCanvas(), UiCanvasBus, SetIsSnapEnabled, (state == Qt::Checked));
            });

        parent->addWidget(m_snapCheckbox);

        QObject::connect(m_editorWindow,
            SIGNAL(SignalSnapToGridToggle()),
            SLOT(HandleSnapToGridToggle()));
    }

    if (addSeparator)
    {
        parent->addSeparator();
    }
}

int CoordinateSystemToolbarSection::CycleSelectedItem()
{
    int newIndex = (m_combobox->currentIndex() + 1) % m_combobox->count();

    m_combobox->setCurrentIndex(newIndex);

    return newIndex;
}

void CoordinateSystemToolbarSection::SetIsEnabled(bool enabled)
{
    m_combobox->setEnabled(enabled);
}

void CoordinateSystemToolbarSection::SetCurrentIndex(int index)
{
    AZ_Assert(index < m_combobox->count(), "Invalid index.");
    m_combobox->setCurrentIndex(index);
}

void CoordinateSystemToolbarSection::HandleCoordinateSystemCycle()
{
    SetCoordinateSystemFromCombobox(m_editorWindow, m_combobox, CycleSelectedItem());
}

void CoordinateSystemToolbarSection::HandleSnapToGridToggle()
{
    m_snapCheckbox->toggle();

    EBUS_EVENT_ID(m_editorWindow->GetCanvas(), UiCanvasBus, SetIsSnapEnabled, (m_snapCheckbox->checkState() == Qt::Checked));
}

void CoordinateSystemToolbarSection::SetSnapToGridIsChecked(bool checked)
{
    m_snapCheckbox->setChecked(checked);
}

#include <CoordinateSystemToolbarSection.moc>
