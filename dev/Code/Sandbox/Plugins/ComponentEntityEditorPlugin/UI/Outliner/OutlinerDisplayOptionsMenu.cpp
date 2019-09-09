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

#include "OutlinerDisplayOptionsMenu.h"

#include <QCheckBox>
#include <QLabel>
#include <QIcon>
#include <QRadioButton>

#include <AzCore/std/containers/vector.h>

#include <UI/Outliner/ui_OutlinerDisplayOptionsMenu.h>


namespace
{
    struct SortElementInfo
    {
        SortElementInfo(QFrame* frame, QRadioButton* radioButton, QLabel* iconLabel, const QIcon& icon, int id)
            : m_frame(frame)
            , m_radioButton(radioButton)
            , m_iconLabel(iconLabel)
            , m_icon(icon)
            , m_id(id)
        {
        }

        QFrame* m_frame = nullptr;
        QRadioButton* m_radioButton = nullptr;
        QLabel* m_iconLabel = nullptr;
        QIcon m_icon;
        int m_id;
    };
}

namespace EntityOutliner
{
    DisplayOptionsMenu::DisplayOptionsMenu(QWidget* parent)
        : QMenu(parent)
        , m_ui(new Ui::OutlinerDisplayOptions)
    {
        m_ui->setupUi(this);

        // use the parent's palette to match the existing hover behaviour throughout the outliner
        QWidget* parentWidget = static_cast<QWidget*>(QObject::parent());
        const QString checkboxHoverStyle = QString("QRadioButton::hover { color: %1; }").arg(parentWidget->palette().color(QPalette::Link).name()); 

        SortElementInfo sortElements[] = {
            { m_ui->sortAtoZFrame,      m_ui->sortAtoZRadioButton,       m_ui->sortAtoZIconLabel,        QIcon("://sort_a_to_z.png"),    static_cast<int>(DisplaySortMode::AtoZ) },
            { m_ui->sortManuallyFrame,  m_ui->sortManuallyRadioButton,   m_ui->sortManuallyIconLabel,    QIcon("://sort_manually.png"),  static_cast<int>(DisplaySortMode::Manually) },
            { m_ui->sortZtoAFrame,      m_ui->sortZtoARadioButton,       m_ui->sortZtoAIconLabel,        QIcon("://sort_z_to_a.png"),    static_cast<int>(DisplaySortMode::ZtoA) }
        };

        for (SortElementInfo& sortElem : sortElements)
        {
            // Qt designer has no way of manually setting the button IDs in the group
            m_ui->sortButtonGroup->setId(sortElem.m_radioButton, sortElem.m_id);
            sortElem.m_iconLabel->setPixmap(sortElem.m_icon.pixmap(sortElem.m_iconLabel->size()));
            sortElem.m_radioButton->setStyleSheet(checkboxHoverStyle);
        }

        m_ui->autoScrollCheckbox->setStyleSheet(checkboxHoverStyle);
        m_ui->autoExpandCheckbox->setStyleSheet(checkboxHoverStyle);

        m_ui->sortManuallyRadioButton->setChecked(true);
        m_ui->autoScrollCheckbox->setChecked(true);
        m_ui->autoExpandCheckbox->setChecked(true);

        connect(m_ui->sortButtonGroup, static_cast<void(QButtonGroup::*)(int)>(&QButtonGroup::buttonClicked), this, &DisplayOptionsMenu::OnSortModeSelected);
        connect(m_ui->autoScrollCheckbox, &QCheckBox::stateChanged, this, &DisplayOptionsMenu::OnAutoScrollToggle);
        connect(m_ui->autoExpandCheckbox, &QCheckBox::stateChanged, this, &DisplayOptionsMenu::OnAutoExpandToggle);
    }

    void DisplayOptionsMenu::OnSortModeSelected(int sortMode)
    {
        emit OnSortModeChanged(static_cast<DisplaySortMode>(sortMode));
    }

    void DisplayOptionsMenu::OnAutoScrollToggle(int state)
    {
        emit OnOptionToggled(DisplayOption::AutoScroll, state == Qt::Checked);
    }

    void DisplayOptionsMenu::OnAutoExpandToggle(int state)
    {
        emit OnOptionToggled(DisplayOption::AutoExpand, state == Qt::Checked);
    }
}

#include <UI/Outliner/OutlinerDisplayOptionsMenu.moc>

