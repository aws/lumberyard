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
#include "EditorUI_QT_Precompiled.h"
#include "QKeySequenceEditorDialog.h"
#include <algorithm>
#include "qmenubar.h"
#include "IEditorParticleUtils.h"
#include "qmessagebox.h"

#include <QApplication>
#include <QLineEdit>
#include <QScreen>
#include <QSettings>
#include <QStyle>

KeyWidgetItem::KeyWidgetItem(QWidget* parent)
    : QWidget(parent)
{
    m_gridLayout = new QGridLayout(this);
    m_gridLayout->setColumnStretch(0, 2);
    m_gridLayout->setColumnStretch(1, 3);
    m_label = new QLabel();
    m_gridLayout->addWidget(m_label, 0, 0, Qt::AlignRight);
}

KeyWidgetItem::~KeyWidgetItem()
{
}

void KeyWidgetItem::SetCapture(QKeySequenceCaptureWidget* capture)
{
    m_captureWidget = capture;
    m_gridLayout->addWidget(m_captureWidget, 0, 1);
}

QKeySequenceEditorDialog::QKeySequenceEditorDialog(QWidget* parent)
{
    setWindowTitle("HotKey Configuration");
    layout = new QGridLayout(this);
    m_treeWidget = new QTreeWidget(this);
    m_treeWidget->setColumnCount(1);
    m_treeWidget->setHeaderHidden(true);
    m_treeWidget->setAlternatingRowColors(true);
    m_treeWidget->setProperty("HotkeyLabel", "Standard");

    resetHotkeys();
    BuildLayout();

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
                std::stable_sort(m_hotkeys.begin(), m_hotkeys.end(), std::less<HotKey>());
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
            std::stable_sort(m_hotkeys.begin(), m_hotkeys.end(), std::less<HotKey>());
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
    okButton->setAutoDefault(true);
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

    layout->addWidget(m_treeWidget, 0, 0, 1, 4);
    layout->addWidget(menuButton, 1, 0, 1, 1);
    layout->setColumnStretch(1, 2);
    layout->setColumnMinimumWidth(1, 61);
    layout->addWidget(okButton, 1, 2, 1, 1);
    layout->addWidget(cancelButton, 1, 3, 1, 1);
    setLayout(layout);
    setMinimumSize(500, 400);
}

QKeySequenceEditorDialog::~QKeySequenceEditorDialog()
{
    GetIEditor()->GetParticleUtils()->HotKey_SetKeys(m_hotkeys);
}

void QKeySequenceEditorDialog::BuildLayout()
{
    m_treeWidget->clear();
    for (unsigned int i = 0; i < m_fields.count(); i++)
    {
        SAFE_DELETE(m_fields[i].first);
        SAFE_DELETE(m_fields[i].second);
    }
    while (m_fields.count() > 0)
    {
        m_fields.takeAt(0);
    }
    std::stable_sort(m_hotkeys.begin(), m_hotkeys.end(), std::less<HotKey>());
    QString lastCategory = "";
    QTreeWidgetItem* parent;
    for (HotKey key : m_hotkeys)
    {
        if (key.path.split('.').first() != lastCategory)
        {
            QStringList strings;
            lastCategory = key.path.split('.').first();
            strings << lastCategory;
            parent = new QTreeWidgetItem(strings);
            m_treeWidget->addTopLevelItem(parent);
        }
        m_fields.push_back(QPair<QLabel*, QKeySequenceCaptureWidget*>(
                new QLabel("   " + key.path.split('.').back(), nullptr),
                new QKeySequenceCaptureWidget(nullptr, key.path, key.sequence.toString())));

        m_fields.back().second->SetCallbackOnFieldChanged([&](QKeySequenceCaptureWidget* field) -> bool
            {
                return this->onFieldUpdate(field);
            });
        QTreeWidgetItem* treeWidgetItem = new QTreeWidgetItem;
        KeyWidgetItem* widgetItem = new KeyWidgetItem();
        parent->addChild(treeWidgetItem);
        widgetItem->SetLabel(key.path.split('.').back());
        widgetItem->SetCapture(m_fields.back().second);
        m_treeWidget->setItemWidget(treeWidgetItem, 0, widgetItem);
    }
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
    std::stable_sort(m_hotkeys.begin(), m_hotkeys.end(), std::less<HotKey>());

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
    QRect desktop = QApplication::primaryScreen()->geometry();

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

#include <VariableWidgets/QKeySequenceEditorDialog.moc>
