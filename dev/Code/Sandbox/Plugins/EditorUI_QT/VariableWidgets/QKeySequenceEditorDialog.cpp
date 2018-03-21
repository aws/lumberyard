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
#include "QKeySequenceEditorDialog.h"
#include <algorithm>
#include "qmenubar.h"
#include "IEditorParticleUtils.h"
#include "qmessagebox.h"
#include <QStyle>
#include <QDesktopWidget>
#include <QApplication>
#include <QSettings>

QKeySequenceEditorDialog::QKeySequenceEditorDialog(QWidget* parent)
{
    formLayout = new QFormLayout(this);
    layout = new QGridLayout(this);
    setWindowTitle("HotKey Configuration");
    scrollArea = new QScrollArea(this);
    scrollWidget = new QWidget(scrollArea);
    scrollWidget->setLayout(formLayout);
    scrollArea->setWidget(scrollWidget);
    scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scrollArea->setWidgetResizable(true);
    scrollArea->adjustSize();
    resetHotkeys();
    BuildLayout();


    scrollArea->setMinimumWidth(scrollWidget->size().width() + style()->pixelMetric(QStyle::PM_ScrollBarExtent));


    //Create menu
    menuButton = new QPushButton(this);
    menuButton->setText(tr("Actions"));
    QMenu* m_menu = new QMenu(this);
    QAction* action = m_menu->addAction(tr("Export"));
    connect(action, &QAction::triggered, this, [&]()
        {
            SetHotKeysFromEditor();
            GetIEditor()->GetParticleUtils()->HotKey_SetKeys(m_hotkeys);
            GetIEditor()->GetParticleUtils()->HotKey_Export();
        });
    action = m_menu->addAction(tr("Import"));
    connect(action, &QAction::triggered, this, [&]()
        {
            if (GetIEditor()->GetParticleUtils()->HotKey_Import())
            {
                while (m_hotkeys.count() > 0)
                {
                    m_hotkeys.takeAt(0);
                }
                m_hotkeys = GetIEditor()->GetParticleUtils()->HotKey_GetKeys();
                qStableSort(m_hotkeys);
                scrollWidget->adjustSize();

                for (QPair<QLabel*, QKeySequenceCaptureWidget*> field : m_fields)
                {
                    for (HotKey key : m_hotkeys)
                    {
                        if (field.second->IsSameField(key.path))
                        {
                            field.second->SetSequence(key.sequence, false);
                            break;
                        }
                        else if (field.second->IsSameCategory(key.path))
                        {
                            if (field.second->IsKeyTaken(key.sequence))
                            {
                                field.second->SetSequence(key.sequence, false);
                            }
                        }
                    }
                }
            }
        });
    action = m_menu->addAction(tr("Reset To Default"));
    connect(action, &QAction::triggered, this, [&]()
        {
            GetIEditor()->GetParticleUtils()->HotKey_BuildDefaults();

            while (m_hotkeys.count() > 0)
            {
                m_hotkeys.takeAt(0);
            }
            m_hotkeys = GetIEditor()->GetParticleUtils()->HotKey_GetKeys();
            qStableSort(m_hotkeys);
            scrollWidget->adjustSize();

            for (QPair<QLabel*, QKeySequenceCaptureWidget*> field : m_fields)
            {
                for (HotKey key : m_hotkeys)
                {
                    if (field.second->IsSameField(key.path))
                    {
                        field.second->SetSequence(key.sequence, true);
                        break;
                    }
                    else if (field.second->IsSameCategory(key.path))
                    {
                        if (field.second->IsKeyTaken(key.sequence))
                        {
                            field.second->SetSequence(key.sequence, true);
                        }
                    }
                }
            }
        });
    menuButton->setMenu(m_menu);

    //create buttons
    okButton = new QPushButton(tr("Ok"), this);
    connect(okButton, &QPushButton::clicked, this, [&]()
        {
            if (ValidateKeys())
            {
                SetHotKeysFromEditor();
                GetIEditor()->GetParticleUtils()->HotKey_SetKeys(m_hotkeys);
                GetIEditor()->GetParticleUtils()->HotKey_SaveCurrent();
                accept();
            }
        });
    cancelButton = new QPushButton(tr("Cancel"), this);
    connect(cancelButton, &QPushButton::clicked, this, [&]()
        {
            reject();
        });

    layout->addWidget(scrollArea, 0, 0, 1, 4);
    layout->addWidget(menuButton, 1, 0, 1, 1);
    layout->setColumnStretch(1, 2);
    layout->setColumnMinimumWidth(1, 61);
    layout->addWidget(okButton, 1, 2, 1, 1);
    layout->addWidget(cancelButton, 1, 3, 1, 1);
    formLayout->setContentsMargins(0, 0, 2, 5);
    setLayout(layout);
    setMinimumHeight(256);
}

QKeySequenceEditorDialog::~QKeySequenceEditorDialog()
{
    GetIEditor()->GetParticleUtils()->HotKey_SetKeys(m_hotkeys);
}

void QKeySequenceEditorDialog::BuildLayout()
{
    while (formLayout->count() > 0)
    {
        formLayout->takeAt(0);
    }
    for (unsigned int i = 0; i < m_fields.count(); i++)
    {
        SAFE_DELETE(m_fields[i].first);
        SAFE_DELETE(m_fields[i].second);
    }
    while (m_fields.count() > 0)
    {
        m_fields.takeAt(0);
    }
    qStableSort(m_hotkeys);
    scrollWidget->setLayout(formLayout);
    QString lastCategory = "";
    for (HotKey key : m_hotkeys)
    {
        if (key.path.split('.').first() != lastCategory)
        {
            QLabel* catLabel = new QLabel(this);
            catLabel->setText(key.path.split('.').first());
            catLabel->setProperty("HotkeyLabel", "Category");
            formLayout->addRow(catLabel);
            lastCategory = catLabel->text();
        }
        m_fields.push_back(QPair<QLabel*, QKeySequenceCaptureWidget*>(
                new QLabel("   " + key.path.split('.').back(), this),
                new QKeySequenceCaptureWidget(this, key.path, key.sequence.toString())));

        m_fields.back().second->SetCallbackOnFieldChanged([&](QKeySequenceCaptureWidget* field) -> bool
            {
                return this->onFieldUpdate(field);
            });
        formLayout->addRow(m_fields.back().first, m_fields.back().second);
    }
    scrollWidget->adjustSize();
    scrollArea->adjustSize();
}

void QKeySequenceEditorDialog::SetHotKeysFromEditor()
{
    for (QPair<QLabel*, QKeySequenceCaptureWidget*> field : m_fields)
    {
        for (unsigned int i = 0; i < m_hotkeys.count(); i++)
        {
            if (field.second->IsSameField(m_hotkeys[i].path))
            {
                m_hotkeys[i].SetSequence(field.second->GetSequence());
            }
        }
    }
}

bool QKeySequenceEditorDialog::ValidateKeys()
{
    QString invalidItems = "";
    for (QPair<QLabel*, QKeySequenceCaptureWidget*> field : m_fields)
    {
        QString extractedSeq = field.second->GetSequence().toString(QKeySequence::PortableText);
        if (extractedSeq.length() != 0)
        {
            //Strip out these items.
            extractedSeq.remove("Ctrl+");
            extractedSeq.remove("Shift+");
            extractedSeq.remove("Alt+");
            extractedSeq.remove("Meta+");
            if (extractedSeq.length() == 0)
            {
                invalidItems += field.second->GetPath() + "\n";
            }
        }
    }

    if (invalidItems.length() > 0)
    {
        QMessageBox dlg(QMessageBox::Critical, "Attempted to use an invalid hotkey", "The Hotkey is invalid for: \n" + invalidItems + " Please correct and try again.", QMessageBox::Ok, this);
        dlg.exec();
        return false;
    }

    return true;
}

bool QKeySequenceEditorDialog::onFieldUpdate(QKeySequenceCaptureWidget* field)
{
    if (field->GetSequence().isEmpty())
    {
        return true;
    }
    for (unsigned int i = 0; i < m_fields.count(); i++)
    {
        if (field->IsSameField(m_fields[i].second->GetPath()))
        {
            continue;
        }
        QKeySequence other = m_fields[i].second->GetSequence();
        if (field->IsKeyTaken(other))
        {
            QMessageBox dlg(QMessageBox::NoIcon, "Attempted to write duplicate hotkey", "This key is already used by: " + m_hotkeys[i].path + "\nKeep Changes? (old shortcut will be overridden)",
                QMessageBox::Button::Apply | QMessageBox::Abort, this);
            if (dlg.exec() == QMessageBox::Button::Apply)
            {
                m_fields[i].second->SetSequence(QKeySequence(), false);
            }
            else
            {
                field->SetSequence(QKeySequence(), false);
                return false;
            }
        }
    }
    return ValidateKeys();
}

void QKeySequenceEditorDialog::resetHotkeys()
{
    GetIEditor()->GetParticleUtils()->HotKey_LoadExisting();
    m_hotkeys = GetIEditor()->GetParticleUtils()->HotKey_GetKeys();
    while (m_hotkeys.count() > 0)
    {
        m_hotkeys.takeAt(0);
    }
    m_hotkeys = GetIEditor()->GetParticleUtils()->HotKey_GetKeys();
    qStableSort(m_hotkeys);
    scrollWidget->adjustSize();

    for (QPair<QLabel*, QKeySequenceCaptureWidget*> field : m_fields)
    {
        for (HotKey key : m_hotkeys)
        {
            if (field.second->IsSameField(key.path))
            {
                field.second->SetSequence(key.sequence, false);
                break;
            }
            else if (field.second->IsSameCategory(key.path))
            {
                if (field.second->IsKeyTaken(key.sequence))
                {
                    field.second->SetSequence(key.sequence, false);
                }
            }
        }
    }
    LoadSessionState();
}

void QKeySequenceEditorDialog::StoreSessionState()
{
    QSettings settings("Amazon", "Lumberyard");
    QString group = "Hotkey Editor/";
    QPoint _pos = this->frameGeometry().topLeft();
    QSize _size = size();

    settings.beginGroup(group);
    settings.remove("");
    settings.sync();

    settings.setValue("pos", _pos);
    settings.setValue("size", _size);

    settings.endGroup();
    settings.sync();
}

void QKeySequenceEditorDialog::LoadSessionState()
{
    QSettings settings("Amazon", "Lumberyard");
    QString group = "Hotkey Editor/";
    settings.beginGroup(group);
    QRect desktop = QApplication::desktop()->availableGeometry();

    //start with a decent size, or last saved size
    resize(settings.value("size", QSize(width(), height())).toSize());
    //start in a central location, or last saved pos
    move(settings.value("pos", QPoint((desktop.width() - width()) / 2, (desktop.height() - height()) / 2)).toPoint());

    settings.endGroup();
    settings.sync();
}

void QKeySequenceEditorDialog::hideEvent(QHideEvent* e)
{
    StoreSessionState();
    QDialog::hideEvent(e);
}
