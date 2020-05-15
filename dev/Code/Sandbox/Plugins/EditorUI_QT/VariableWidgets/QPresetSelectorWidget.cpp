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
#include "QPresetSelectorWidget.h"

#define ALLOW_DELETION_OF_ALL_LIBRARIES

#ifdef EDITOR_QT_UI_EXPORTS
#include <VariableWidgets/QPresetSelectorWidget.moc>
#endif

#include <QMenu>
#include <QGridLayout>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QComboBox>
#include <QScrollArea>
#include <QScrollBar>
#include <QMessageBox>
#include "QPresetWidget.h"
#include "qfiledialog.h"
#include "qcoreevent.h"
#include "qevent.h"
#include "qapplication.h"
#include "qframe.h"
#include "ContextMenu.h"
#include "IEditorParticleUtils.h"
#include "QAmazonLineEdit.h"
#include "../Utils.h"
#include <qsettings.h>

QPresetSelectorWidget::QPresetSelectorWidget(QWidget* parent /*= 0*/)
    : QWidget(parent)
    , panelFrame(this)
    , panelLayout()
    , libraryLayout()
    , layout()
    , presets(0)
    , libNames(0)
    , libFilePaths(0)
    , panelName(this)
    , panelEditName(this)
    , panelMenuBtn(this)
    , addPresetBtn(this)
    , addPresetBackground(this)
    , addPresetForeground(new QWidget(this))
    , addPresetName(this)
    , m_amCreatingLib(false)
{
    InitPanel();
    viewType = ViewType::GRID_VIEW;
    m_currentLibrary = 0;
    ConnectButtons();
    panelFrame.setFrameShape(QFrame::HLine);

    presetSizes[0].width = presetSizes[0].height = 16;
    presetSizes[1].width = presetSizes[1].height = 32;
    presetSizes[2].width = presetSizes[2].height = 64;

    m_menuFlags = PresetMenuFlag(MAIN_MENU_LIST_OPTIONS | MAIN_MENU_SIZE_OPTIONS | MAIN_MENU_REMOVE_ALL);
    m_layoutType = PresetLayoutType::PRESET_LAYOUT_DEFAULT;

    currentPresetSize = PRESET_MEDIUM;

    scrollArea = new QScrollArea(this);
    scrollBox = new QWidget(this);
    scrollBox->setLayout(&libraryLayout);
    scrollBox->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    m_sizeFix = new QWidget(scrollBox);
    scrollArea->setWidget(scrollBox);
    scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scrollArea->verticalScrollBar()->setStyleSheet(tr("QScrollBar::down-arrow{border-image: url(:/particleQT/buttons/navigate_down_ico.png);}") +
        tr("QScrollBar::add-line:vertical{background:none;}") +
        tr("QScrollBar::sub-line:vertical{background:none;}"));
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->adjustSize();
    scrollBox->setContentsMargins(0, 0, 0, 0);

    m_menu = new QMenu(this);
    swabSizeMenu = new QMenu(this);
    QAction* action;
    swabSizeMenu->setTitle("Preset preview size");

    action = swabSizeMenu->addAction(tr("Small"));
    connect(action, &QAction::triggered, this, [ = ] {
            currentPresetSize = presetSizeEnum::PRESET_SMALL;
            BuildPanelLayout(newLibFlag[m_currentLibrary], libNames[m_currentLibrary]);
            DisplayLibrary(m_currentLibrary);
        });

    action = swabSizeMenu->addAction(tr("Medium"));
    connect(action, &QAction::triggered, this, [ = ] {
            currentPresetSize = presetSizeEnum::PRESET_MEDIUM;
            BuildPanelLayout(newLibFlag[m_currentLibrary], libNames[m_currentLibrary]);
            DisplayLibrary(m_currentLibrary);
        });

    action = swabSizeMenu->addAction(tr("Large"));
    connect(action, &QAction::triggered, this, [ = ] {
            currentPresetSize = presetSizeEnum::PRESET_LARGE;
            BuildPanelLayout(newLibFlag[m_currentLibrary], libNames[m_currentLibrary]);
            DisplayLibrary(m_currentLibrary);
        });

    libSelectMenu = new QMenu(this);
    libSelectMenu->setTitle(tr("Libraries"));
    BuildMainMenu();
    BuildLibSelectMenu();
    presetMenu = new ContextMenu(this);
    BuildPresetMenu();

    panelName.setStyleSheet(tr("QPushButton{ background: none; border: none;}"));
    addPresetBackground.setStyleSheet(tr("background-image:url(:/particleQT/icons/color_btn_bkgn.png); border: none;"));

    setLayout(&layout);
    panelMenuBtn.setMenu(m_menu);
    panelMenuBtn.installEventFilter(this);
    layout.addLayout(&panelLayout);
    layout.addWidget(&panelFrame);
    layout.addWidget(scrollArea);
    //layout.addStretch(5);
    addPresetBtn.installEventFilter(this);
    panelEditName.installEventFilter(this);

    addPresetName.installEventFilter(this);
    QAction* exitRename = addPresetName.addAction(QIcon(":/particleQT/buttons/close_btn.png"), QLineEdit::TrailingPosition);
    connect(exitRename, &QAction::triggered, this, &QPresetSelectorWidget::AddActionCanceled);

    QMargins margins = layout.contentsMargins();
    margins.setLeft(0);
    margins.setRight(0);
    layout.setContentsMargins(margins);
    m_tooltip = new QToolTipWidget(this);
    connect(&panelEditName, &QAmazonLineEdit::textChanged, this, [=](){m_tooltip->hide(); });
    QAction* libSaveAction = panelEditName.addAction(QIcon(":/particleQT/buttons/add_btn.png"), QLineEdit::TrailingPosition);
    connect(libSaveAction, &QAction::triggered, this, &QPresetSelectorWidget::OnPanelSaveButtonClicked);
}

void QPresetSelectorWidget::AddActionCanceled()
{
    addPresetName.clear();
    addPresetName.clearFocus();
}

void QPresetSelectorWidget::RemovePresetFromLibrary(int id, int lib)
{
    while (libraryLayout.count() > 0)
    {
        libraryLayout.takeAt(0);
    }

    for (unsigned int i = 0; i < presets[lib].count(); i++)
    {
        presets[lib][i]->hide();
    }

    presets[lib].remove(id);

    unsavedFlag[m_currentLibrary] = true;
}

int QPresetSelectorWidget::AddNewLibrary(QString name, int index /*= -1*/)
{
    //disable all color buttons and labels
    for (int j = 0; j < libNames.count(); j++)
    {
        for (int k = 0; k < presets[j].count(); k++)
        {
            presets[j][k]->hide();
        }
    }

    //clear layout
    while (libraryLayout.count() > 0)
    {
        libraryLayout.takeAt(0);
    }

    int foundAt = libNames.indexOf(name, 0);
    if (foundAt != -1)
    {
        return foundAt;
    }

    if (index == -1)
    {
        libNames.push_back(name);
        libFilePaths.push_back(QString());
        presets.push_back(QVector<QPresetWidget*>());
        newLibFlag.push_back(true);
        unsavedFlag.push_back(true);
        index = libNames.count() - 1;
    }
    else
    {
        libNames.insert(index, name);
        libFilePaths.insert(index, QString());
        presets.insert(index, QVector<QPresetWidget*>());
        newLibFlag.insert(index, true);
        unsavedFlag.insert(index, true);
    }
    return index;
}

void QPresetSelectorWidget::RemoveLibrary(int id, int gotoLibId /*= 0*/)
{
    if (id == m_currentLibrary)
    {
        while (libraryLayout.count() > 0)
        {
            libraryLayout.takeAt(0);
        }
        for (unsigned int i = 0; i < presets[m_currentLibrary].count(); i++)
        {
            presets[m_currentLibrary][i]->hide();
        }
    }
    if (id < 0 || id >= presets.size())
    {
        return; //nothing to delete
    }

    libFilePaths.remove(id);
    libNames.remove(id);
    newLibFlag.remove(id);
    presets.remove(id);
    m_currentLibrary = gotoLibId;
    if (presets.count() == 0)
    {
        m_currentLibrary = AddNewLibrary(DefaultLibraryName(), m_defaultLibraryId);
        BuildDefaultLibrary();
    }
    BuildPanelLayout(false, libNames[m_currentLibrary]);
    DisplayLibrary(m_currentLibrary);
    BuildLibSelectMenu();
}

void QPresetSelectorWidget::ConnectButtons()
{
    //connect(&panelName, SIGNAL(clicked()), this, SLOT(OnPanelNameClicked()));
    connect(&addPresetBtn, SIGNAL(clicked()), this, SLOT(OnAddPresetClicked()));
}

void QPresetSelectorWidget::DisconnectButtons()
{
    //disconnect(&panelName, SIGNAL(clicked()), this, SLOT(OnPanelNameClicked()));
    connect(&addPresetBtn, SIGNAL(clicked()), this, SLOT(OnAddPresetClicked()));
}

void QPresetSelectorWidget::BuildLibraryLayout(int lib)
{
    //magic number 9 is default content margin 4*9 = 36 these are outer borders and the swabs outer borders
    //magic number 6 is space between each swab
    int scrollBoxWidth = scrollArea->verticalScrollBar()->width();
    presetSizes[currentPresetSize].numPerGridViewRow = qMax(int((width() - scrollBoxWidth + 6) / (presetSizes[currentPresetSize].width + 6)), 1);

    //Check layout flags
    viewType = m_layoutType == PresetLayoutType::PRESET_LAYOUT_FIXED_COLUMN_2 ? ViewType::GRID_VIEW : viewType;

    //determines steps via bool
    int numPerRow = (viewType == ViewType::GRID_VIEW) ? presetSizes[currentPresetSize].numPerGridViewRow : m_layoutType == PresetLayoutType::PRESET_LAYOUT_FIXED_COLUMN_2? 2:1;
    
    bool showName = false;

    //disable all color buttons and labels
    for (int i = 0; i < libNames.count(); i++)
    {
        for (int j = 0; j < presets[i].count(); j++)
        {
            presets[i][j]->hide();
        }
    }
    addPresetForeground->hide();
    addPresetBackground.hide();
    addPresetBtn.hide();
    addPresetName.hide();

    //clear layout
    while (libraryLayout.count() > 0)
    {
        libraryLayout.takeAt(0);
    }

    int lastColumn = 0;
    int lastRow = 0;

    //resize scrollbox to take up full area
    scrollBox->setMinimumHeight(scrollArea->height());

    int maximumWidth = scrollArea->width() - libraryLayout.contentsMargins().right();
    scrollBox->setFixedWidth(maximumWidth);

    if (viewType == ViewType::LIST_VIEW)
    {
        //TODO while in list view have add color section here
        addPresetForeground->show();
        addPresetBackground.show();
        addPresetBtn.show();
        addPresetName.show();
        addPresetBtn.setStyleSheet(tr("QPushButton{background-image:url(:/particleQT/buttons/add_btn.png); background-position: center; background-repeat: no-repeat;}"));
        addPresetName.setText("");
        addPresetForeground->setFixedSize(presetSizes[currentPresetSize].width, presetSizes[currentPresetSize].height);
        addPresetBackground.setFixedSize(presetSizes[currentPresetSize].width, presetSizes[currentPresetSize].height);
        addPresetBackground.stackUnder(addPresetForeground);
        libraryLayout.addWidget(&addPresetBackground, lastRow, 0);
        libraryLayout.addWidget(addPresetForeground, lastRow, 0);
        libraryLayout.addWidget(&addPresetName, lastRow, 1);
        libraryLayout.addWidget(&addPresetBtn, lastRow, 2);
        libraryLayout.setColumnStretch(1, 5);
        addPresetBtn.setFixedSize(16, 16);
        connect(&addPresetName, &QAmazonLineEdit::textChanged, this, [=](){m_tooltip->hide(); });

        lastRow++;
    }
    if (viewType == ViewType::GRID_VIEW)
    {
        addPresetName.hide();
        addPresetBackground.hide();
        addPresetForeground->hide();

        addPresetBtn.setStyleSheet(tr("QPushButton{background-image:url(:/particleQT/buttons/add_btn.png); background-position: center; background-repeat: no-repeat;}"));
        if (m_layoutType == PresetLayoutType::PRESET_LAYOUT_FIXED_COLUMN_2)
        {
            // For fixed 2 column layout, the add button should take both column
            libraryLayout.addWidget(&addPresetBtn, lastRow, lastColumn, 1, numPerRow);
            addPresetBtn.setFixedSize((presetSizes[currentPresetSize].width + 6) * numPerRow, presetSizes[currentPresetSize].height);
            lastColumn += numPerRow;
        }
        else
        {
            libraryLayout.addWidget(&addPresetBtn, lastRow, lastColumn);
            addPresetBtn.setFixedSize(presetSizes[currentPresetSize].width, presetSizes[currentPresetSize].height);
            lastColumn++;
            libraryLayout.setColumnStretch(1, 0);
        }
        addPresetBtn.show();
        maximumWidth = presetSizes[currentPresetSize].width;
    }

    //add color swabs to library
    int numRows = (presets[lib].count()) / numPerRow;
    numRows = m_layoutType == PresetLayoutType::PRESET_LAYOUT_FIXED_COLUMN_2 ? ++numRows : numRows;
    int offset = 0 - lastColumn;
    for (unsigned int r = 0; r <= numRows; r++)
    {
        for (unsigned int c = lastColumn; lastColumn < numPerRow && (offset + c) < (presets[lib].count()); c++)
        {
            if (viewType == ViewType::GRID_VIEW)
            {
                presets[lib][offset + c]->setPresetSize(presetSizes[currentPresetSize].width,
                    presetSizes[currentPresetSize].height);
                presets[lib][offset + c]->ShowGrid();
                libraryLayout.addWidget(presets[lib][offset + c], lastRow, lastColumn, 1, 1);
            }
            else if (viewType == ViewType::LIST_VIEW)
            {
                presets[lib][offset + c]->setPresetSize(presetSizes[currentPresetSize].width,
                    presetSizes[currentPresetSize].height);
                presets[lib][offset + c]->ShowList();
                libraryLayout.addWidget(presets[lib][offset + c], lastRow, lastColumn, 1, 3);
            }
            presets[lib][offset + c]->setMaximumWidth(maximumWidth);
            lastColumn++;
        }
        offset += numPerRow;
        lastRow++;
        lastColumn = 0;
    }

    m_sizeFix->setFixedSize(maximumWidth, 1);
    libraryLayout.addWidget(m_sizeFix, ++lastRow, 0, 1, (viewType == LIST_VIEW) ? 3 : presetSizes[currentPresetSize].numPerGridViewRow);
    libraryLayout.setRowStretch(lastRow, 5);
    libraryLayout.setColumnStretch(qMin(presets[lib].count(), presetSizes[currentPresetSize].numPerGridViewRow), (viewType != LIST_VIEW) ? 5 : 0);
    scrollBox->adjustSize();
    presetIsSelectedForDeletion.clear();
    lastPresetIndexSelected = -1;
    //set horizontal scroll to 0 just in case it manages to automatically scroll
    scrollArea->horizontalScrollBar()->setValue(0);
    for (int i = 0; i < presets[m_currentLibrary].count(); i++)
    {
        presetIsSelectedForDeletion.push_back(false);
    }
}

void QPresetSelectorWidget::BuildPanelLayout(bool editable /*= false*/, QString _name /*= QString()*/)
{
    if (_name.compare(QString()) != 0)
    {
        panelName.setText(_name);
    }
    else
    {
        panelName.setText(DefaultLibraryName());
    }

    //clear layout
    while (panelLayout.count() > 0)
    {
        panelLayout.takeAt(0);
    }

    panelName.setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    if (editable)
    {
        panelName.setText(_name);
        panelEditName.setText(panelName.text());
        panelEditName.show();
        panelName.hide();
        panelLayout.addWidget(&panelEditName, 5);
        panelLayout.addStretch(0);
        panelEditName.setFocus();
    }
    else
    {
        panelEditName.hide();
        panelName.show();
        panelLayout.addWidget(&panelName, 1);
    }
    panelLayout.addWidget(&panelMenuBtn);
    BuildMainMenu();
    BuildLibSelectMenu();
    BuildPresetMenu();
}

void QPresetSelectorWidget::SelectPresetLibrary(int i)
{
    AZ_Assert(0 <= i && i < libNames.size(), "Preset library index %d out of range [%d,%d)", i, 0, libNames.size());
    m_currentLibrary = i;
    BuildPanelLayout(m_amCreatingLib, libNames[m_currentLibrary]);
    DisplayLibrary(m_currentLibrary);
    BuildLibSelectMenu();
}

void QPresetSelectorWidget::BuildMainMenu()
{
    m_menu->clear();
    m_menu->setVisible(false);
    QAction* action;
    //////////////////////////////////////////////////////////////////////////
    if (m_menuFlags & PresetMenuFlag::MAIN_MENU_LIST_OPTIONS)
    {
        action = m_menu->addAction("List");
        connect(action, &QAction::triggered, this, [ = ] {
                viewType = ViewType::LIST_VIEW;
                BuildPanelLayout(newLibFlag[m_currentLibrary], (newLibFlag[m_currentLibrary]) ? panelEditName.text() : libNames[m_currentLibrary]);
                DisplayLibrary(m_currentLibrary);
            });
        action = m_menu->addAction("Grid");
        connect(action, &QAction::triggered, this, [ = ] {
                viewType = ViewType::GRID_VIEW;
                BuildPanelLayout(newLibFlag[m_currentLibrary], (newLibFlag[m_currentLibrary]) ? panelEditName.text() : libNames[m_currentLibrary]);
                DisplayLibrary(m_currentLibrary);
            });
        m_menu->addSeparator();
    }
    //////////////////////////////////////////////////////////////////////////
    if (swabSizeMenu != nullptr && (m_menuFlags & PresetMenuFlag::MAIN_MENU_SIZE_OPTIONS))
    {
        m_menu->addMenu(swabSizeMenu);
        m_menu->addSeparator();
    }
    //////////////////////////////////////////////////////////////////////////
    if (!m_amCreatingLib)
    {
        action = m_menu->addAction("Create New Library...");
        connect(action, &QAction::triggered, this, [ = ] {
                m_amCreatingLib = true;
                panelName.setText("");
                BuildPanelLayout(true, tr(""));
                m_currentLibrary = AddNewLibrary("");
                DisplayLibrary(m_currentLibrary);
                BuildMainMenu();
            });
    }
    //////////////////////////////////////////////////////////////////////////
    if (libSelectMenu != nullptr)
    {
        m_menu->addMenu(libSelectMenu);
    }
    //////////////////////////////////////////////////////////////////////////
    action = m_menu->addAction("Load Library...");
    connect(action, &QAction::triggered, this, [ = ] {
            if (newLibFlag[m_currentLibrary])
            {
                m_amCreatingLib = false;
                newLibFlag[m_currentLibrary] = false;
                RemoveLibrary(m_currentLibrary);
                BuildMainMenu();
            }
            LoadPreset(QFileDialog::getOpenFileName(this, "Select preset to load", QString(), tr("Preset Files (*.cpf)")));
        });
    //////////////////////////////////////////////////////////////////////////
    if (!m_amCreatingLib)
    {
        action = m_menu->addAction("Export...");
        connect(action, &QAction::triggered, this, [ = ] {
                if (!newLibFlag[m_currentLibrary])
                {
                    QString location = QFileDialog::getSaveFileName(this, "Select location to save preset", panelName.text(), tr("Preset Files (*.cpf)"));
                    if (location.isEmpty()) //if the user hit cancel
                    {
                        return;
                    }
                    PrepareExport(location);
                    SavePreset(location);
                }
            });
    }
    //////////////////////////////////////////////////////////////////////////
    if (!m_amCreatingLib)
    {
        action = m_menu->addAction("Rename Library");
        connect(action, &QAction::triggered, this, [ = ] {
                BuildPanelLayout(true, panelName.text());
                DisplayLibrary(m_currentLibrary);
                BuildMainMenu();
            });
        if (m_currentLibrary == m_defaultLibraryId)
        {
            action->setDisabled(true);
        }
    }
    //////////////////////////////////////////////////////////////////////////
    action = m_menu->addAction("Remove Library");
    connect(action, &QAction::triggered, this, [ = ] {
            if (m_currentLibrary < libNames.count())
            {
                int answer = QMessageBox::warning(this, tr("Warning"), tr("This will remove library, any unsaved changes will be lost. Would you like to continue?"), QMessageBox::Yes | QMessageBox::No);
                if (answer == QMessageBox::Button::Yes)
                {
                    m_amCreatingLib = false;
                    RemoveLibrary(m_currentLibrary);
                }
            }
        });
    if (m_currentLibrary == m_defaultLibraryId)
    {
        action->setDisabled(true);
    }
    m_menu->addSeparator();
    //////////////////////////////////////////////////////////////////////////
    action = m_menu->addAction("Reset to default");
    if (m_currentLibrary != DEFAULT_PRESET_INDEX)
    {
        connect(action, &QAction::triggered, this, [=] { SelectPresetLibrary(DEFAULT_PRESET_INDEX); });
    }
    else
    {
        connect(action, &QAction::triggered, this, [=] {
            // Reset the default library to its default state
            BuildDefaultLibrary();
            BuildMainMenu();
            SelectPresetLibrary(DEFAULT_PRESET_INDEX);
        });
    }
    m_menu->addSeparator();
    //////////////////////////////////////////////////////////////////////////
    if (m_menuFlags & PresetMenuFlag::MAIN_MENU_REMOVE_ALL)
    {
        action = m_menu->addAction("Remove All Libraries");
        connect(action, &QAction::triggered, this, [&] {
                int answer = QMessageBox::warning(this, tr("Warning"), tr("This will remove ALL libraries!, Continue?"), QMessageBox::Yes | QMessageBox::No);
                if (answer == QMessageBox::Button::Yes)
                {
                    while (libNames.count() > 1)
                    {
                        m_currentLibrary = libNames.count() - 1;
                        m_amCreatingLib = false;
                        RemoveLibrary(m_currentLibrary);
                    }
                    //one more time to get rid of last library
                    m_currentLibrary = 0;
                    m_amCreatingLib = false;
                    RemoveLibrary(m_currentLibrary);
                }
            });
    }
}

void QPresetSelectorWidget::BuildLibSelectMenu()
{
    libSelectMenu->clear();

    for (int i = 0; i < libNames.count(); i++)
    {
        //removes empty named libraries that are left over when swapping around and out of create library
        if (libNames[i].isEmpty() && !m_amCreatingLib)
        {
            RemoveLibrary(i);
            i--;
            m_currentLibrary = i;
            continue;
        }
        QAction* action = libSelectMenu->addAction(libNames[i]);
        connect(action, &QAction::triggered, this, [ = ] {
                if (newLibFlag[m_currentLibrary] && i != m_currentLibrary)
                {
                    m_amCreatingLib = false;
                    RemoveLibrary(m_currentLibrary, i);
                }
                else
                {
                    SelectPresetLibrary(i);
                }
            });
    }
}

void QPresetSelectorWidget::InitPanel()
{
    panelEditName.setFrame(false);
    panelEditName.hide();

    panelMenuBtn.setProperty("iconButton", true);
    panelMenuBtn.setObjectName("btnMenu");
    panelMenuBtn.setFixedSize(27, 16);
    panelMenuBtn.setMinimumSize(27, 16);
    panelMenuBtn.setMaximumSize(27, 16);
}

bool QPresetSelectorWidget::eventFilter(QObject* obj, QEvent* event)
{
    int lpis = lastPresetIndexSelected;
    QWidget* objWidget = static_cast<QWidget*>(obj);
    QRect widgetRect = QRect(mapToGlobal(QPoint(0, 0)), QSize(0, 0));
    QToolTipWidget::ArrowDirection arrowDir = QToolTipWidget::ArrowDirection::ARROW_RIGHT;

    if (event->type() == QEvent::ToolTip)
    {
        QHelpEvent* e = static_cast<QHelpEvent*>(event);
        QPoint ep = e->globalPos();

        if (objWidget == &addPresetBtn)
        {
            GetIEditor()->GetParticleUtils()->ToolTip_BuildFromConfig(m_tooltip, "Preset Library", "Add Preset");
            widgetRect.setTopLeft(mapToGlobal(addPresetBtn.pos()));
            arrowDir = QToolTipWidget::ArrowDirection::ARROW_LEFT;
            widgetRect.setSize(addPresetBtn.size());
        }
        if (objWidget == &panelMenuBtn)
        {
            GetIEditor()->GetParticleUtils()->ToolTip_BuildFromConfig(m_tooltip, "Preset Library", "Menu");
            widgetRect.setTopLeft(mapToGlobal(panelMenuBtn.pos()));
            arrowDir = QToolTipWidget::ArrowDirection::ARROW_LEFT;
            widgetRect.setSize(panelMenuBtn.size());
        }
        if (objWidget == &panelEditName)
        {
            GetIEditor()->GetParticleUtils()->ToolTip_BuildFromConfig(m_tooltip, "Preset Library", "New library name edit");
            widgetRect.setTopLeft(mapToGlobal(panelEditName.pos()));
            arrowDir = QToolTipWidget::ArrowDirection::ARROW_LEFT;
            widgetRect.setSize(panelEditName.size());
        }
        if (objWidget == &addPresetName)
        {
            GetIEditor()->GetParticleUtils()->ToolTip_BuildFromConfig(m_tooltip, "Preset Library", "New preset name edit");
            widgetRect.setTopLeft(mapToGlobal(addPresetName.pos()));
            arrowDir = QToolTipWidget::ArrowDirection::ARROW_LEFT;
            widgetRect.setSize(addPresetName.size());
        }

        m_tooltip->TryDisplay(ep, widgetRect, arrowDir);

        return true;
    }
    if (obj == static_cast<QObject*>(&panelMenuBtn))
    {
        if (event->type() == QEvent::Leave)
        {
            if (m_tooltip->isVisible())
            {
                m_tooltip->hide();
            }
        }
    }
    if (obj == (QObject*)&panelEditName)
    {
        if (event->type() == QEvent::KeyPress)
        {
            QKeyEvent* e = (QKeyEvent*)event;
            if (e->key() == Qt::Key_Enter || e->key() == Qt::Key_Return)
            {
                OnPanelSaveButtonClicked();
                e->ignore();
            }
        }
        if (event->type() == QEvent::Leave)
        {
            if (m_tooltip->isVisible())
            {
                m_tooltip->hide();
            }
        }
    }
    if (obj == (QObject*)&addPresetName)
    {
        if (event->type() == QEvent::KeyPress)
        {
            QKeyEvent* e = (QKeyEvent*)event;
            if (e->key() == Qt::Key_Enter || e->key() == Qt::Key_Return)
            {
                OnAddPresetClicked();
                e->ignore();
            }
        }
        if (event->type() == QEvent::Leave)
        {
            if (m_tooltip->isVisible())
            {
                m_tooltip->hide();
            }
        }
    }
    if (presets.count() <= 0)
    {
        return QWidget::eventFilter(obj, event);
    }
    for (int i = 0; i < presets[m_currentLibrary].count(); i++)
    {
        if (obj == (QObject*)presets[m_currentLibrary][i])
        {
            if (event->type() == QEvent::MouseButtonRelease)
            {
                QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);
                if (mouseEvent->button() == Qt::LeftButton)
                {
                    if (QApplication::keyboardModifiers() == Qt::ControlModifier)
                    {
                        OnPresetClicked(i, false, presetIsSelectedForDeletion[i]);
                    }
                    else if (QApplication::keyboardModifiers() == Qt::ShiftModifier)
                    {
                        if (lpis >= i && lpis < presets[m_currentLibrary].count())
                        {
                            OnPresetClicked(lpis, true);
                            for (int j = lpis; j >= i; j--)
                            {
                                OnPresetClicked(j, false);
                                lastPresetIndexSelected = lpis;
                            }
                        }
                        else if (lpis < i && lpis > -1)
                        {
                            OnPresetClicked(lpis, true);
                            for (int j = lpis; j <= i; j++)
                            {
                                OnPresetClicked(j, false);
                                lastPresetIndexSelected = lpis;
                            }
                        }
                    }
                    else
                    {
                        OnPresetClicked(i, true);
                    }
                }
                if (mouseEvent->button() == Qt::RightButton)
                {
                    if (!presetIsSelectedForDeletion[i])
                    {
                        OnPresetClicked(i, true);
                    }
                    OnPresetRightClicked(i);
                }
            }
        }
    }

    return QWidget::eventFilter(obj, event);
}

void QPresetSelectorWidget::OnPresetClicked(int preset, bool overwriteSelection, bool deselect /*= false*/)
{
    if (preset >= presets[m_currentLibrary].count() || preset < 0)
    {
        return;
    }

    onValueChanged(preset); //calls any callbacks child may need

    if (overwriteSelection)
    {
        for (int c = 0; c < presets[m_currentLibrary].count(); c++)
        {
            presetIsSelectedForDeletion[c] = false;
            StylePreset(presets[m_currentLibrary][c], false);
        }
    }

    if (deselect)
    {
        presetIsSelectedForDeletion[preset] = false;
        StylePreset(presets[m_currentLibrary][preset], false);
        return;
    }
    presetIsSelectedForDeletion[preset] = true;
    lastPresetIndexSelected = preset;
    StylePreset(presets[m_currentLibrary][preset], true);
}

void QPresetSelectorWidget::OnAddPresetClicked()
{
    if (addPresetName.text().isEmpty())
    {
        AddPresetToLibrary(CreateSelectedWidget(), m_currentLibrary);
    }
    else
    {
        AddPresetToLibrary(CreateSelectedWidget(), m_currentLibrary, addPresetName.text());
    }

    DisplayLibrary(m_currentLibrary);

    addPresetBtn.clearFocus();
}

void QPresetSelectorWidget::OnPanelNameClicked()
{
    if (scrollArea->isVisible())
    {
        scrollArea->hide();
    }
    else
    {
        scrollArea->show();
    }
    panelName.clearFocus();
}

void QPresetSelectorWidget::OnPanelSaveButtonClicked()
{
    QString whiteSpaceCheck = panelEditName.text();

    whiteSpaceCheck.remove(" ");
    if (whiteSpaceCheck.length() == 0)
    {
        //create a new library name
        panelEditName.setText(GetUniqueLibraryName("Library"));
    }
    else
    {
        for (QString name : libNames)
        {
            if (name == libNames[m_currentLibrary]) // if the new name is its original name, continue;
            {
                continue;
            }
            if (name == panelEditName.text())
            {
                QMessageBox dlg(QMessageBox::NoIcon, "Warning", "Library name in use, please use another name.", QMessageBox::Button::Ok);
                dlg.setWindowFlags(Qt::WindowStaysOnTopHint);
                dlg.exec();
                return;
            }
        }
    }

    libNames[m_currentLibrary] = panelEditName.text();
    BuildPanelLayout(false, panelEditName.text());
    DisplayLibrary(m_currentLibrary);
    m_amCreatingLib = false;
    BuildMainMenu();
    BuildLibSelectMenu();

    newLibFlag[m_currentLibrary] = false;
    DisplayLibrary(m_currentLibrary);
    SaveDefaultPreset(m_currentLibrary);
}

void QPresetSelectorWidget::OnPresetRightClicked(int i)
{
    BuildPresetMenu();
    presetMenu->actions().constFirst()->setData(i);
    presetMenu->exec(QCursor::pos());
}


QPresetSelectorWidget::~QPresetSelectorWidget()
{
    scrollArea->close();
    panelMenuBtn.close();
    panelMenuBtn.setMenu(nullptr);
    m_menu->clear();
    while (panelLayout.count() > 0)
    {
        panelLayout.takeAt(0);
    }
    while (libraryLayout.count() > 0)
    {
        libraryLayout.takeAt(0);
    }
    while (layout.count() > 0)
    {
        layout.takeAt(0);
    }

    for (int i = 0; i < libNames.count(); i++)
    {
        for (int j = 0; j < presets[i].count(); j++)
        {
            presets[i].remove(j);
            presets[i].takeAt(i);
        }
        newLibFlag.remove(i);
        presets.remove(i);
        if (presets.count() > 0)
        {
            presets.takeAt(i);
        }
        libNames.remove(i);
        libFilePaths.remove(i);
    }
    SAFE_DELETE(m_tooltip);
    SAFE_DELETE(m_menu);
    SAFE_DELETE(libSelectMenu);
    SAFE_DELETE(presetMenu);
}

void QPresetSelectorWidget::SaveOnClose()
{
    if (m_amCreatingLib)
    {
        m_amCreatingLib = false;
        RemoveLibrary(m_currentLibrary);
    }
    StoreSessionPresets();
}

void QPresetSelectorWidget::BuildPresetMenu()
{
    presetMenu->clear();
    QAction* action = presetMenu->addAction(tr("Remove"));
    connect(action, &QAction::triggered, this, [ = ] {
            for (int i = presets[m_currentLibrary].count() - 1; i > -1; i--)
            {
                if (presetIsSelectedForDeletion[i])
                {
                    RemovePresetFromLibrary(i, m_currentLibrary);
                }
            }
            DisplayLibrary(m_currentLibrary);
        });
}

void QPresetSelectorWidget::showEvent(QShowEvent* event)
{
    DisplayLibrary(m_currentLibrary);
    QWidget::showEvent(event);
}

void QPresetSelectorWidget::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    DisplayLibrary(m_currentLibrary);
}

QString QPresetSelectorWidget::GetUniqueLibraryName(QString name)
{
    static unsigned int version = 0;
    for (auto lib : libNames)
    {
        int lastIndex = lib.lastIndexOf('_');
        QString comparedString = (lastIndex > 0 && lastIndex < lib.length()) ? lib.left(lastIndex) : lib;
        if (comparedString.compare(name, Qt::CaseInsensitive) == 0)
        {
            version++;
            return name.append("_%1").arg(version, 2, 10, QChar('0'));
        }
    }
    return name;
}


void QPresetSelectorWidget::DisplayLibrary(int libId)
{
    //The first pass figures out how many items can be included and calculates their size
    //the second pass fixes their sizes and positions and displays them
    BuildLibraryLayout(libId);
    BuildLibraryLayout(libId);
}

QString QPresetSelectorWidget::DefaultLibraryName()
{
    return tr("Default Library");
}

void QPresetSelectorWidget::keyPressEvent(QKeyEvent* e)
{
    if (m_amCreatingLib)
    {
        if (e->key() == Qt::Key_Escape)
        {
            m_amCreatingLib = false;
            RemoveLibrary(m_currentLibrary);
        }
    }
}

bool QPresetSelectorWidget::LoadSessionPresets()
{
    QSettings settings("Amazon", "Lumberyard");
    bool success = false;

    settings.beginGroup(SettingsBaseGroup()); //start of all libraries
    QStringList libraries = settings.childGroups();
    if (libraries.count() <= 0)
    {
        return false;
    }
    for (QString lib : libraries)
    {
        if (LoadSessionPresetFromName(&settings, lib))
        {
            //something loaded
            success = true;
        }
    }
    settings.endGroup(); //end of all libraries
    if (success)
    {
        BuildLibSelectMenu();
        m_currentLibrary = 0;
        DisplayLibrary(m_currentLibrary);
        BuildPanelLayout(false, libNames[m_currentLibrary]);
    }
    return success;
}

void QPresetSelectorWidget::StoreSessionPresets()
{
    QSettings settings("Amazon", "Lumberyard");
    int digitsToDisplay = 3;
    int numericalBase = 10;
    QChar fillerCharacter = '0';

    settings.beginGroup(SettingsBaseGroup()); //start of all libraries
    settings.remove("");
    settings.sync();
    for (unsigned int lib = 0; lib < presets.count(); lib++)
    {
        if (libNames[lib].length() == 0)
        {
            continue;
        }
        //assigning a path of 000-999 to preserve library order, and prevent cases where duplicate names would create conflict
        settings.beginGroup(QString("%1").arg(lib, digitsToDisplay, numericalBase, fillerCharacter)); // start of library
        settings.setValue("path", libFilePaths[lib]);
        settings.setValue("name", libNames[lib]);
        for (unsigned int preset = 0; preset < presets[lib].count(); preset++)
        {
            SavePresetToSession(&settings, lib, preset);
        }
        settings.endGroup();//end of library
    }
    settings.endGroup(); //end of all libraries
}

void QPresetSelectorWidget::LoadDefaultPreset(int libId)
{
    //if the libID = 0 then load the actual default library
    if (libId == m_defaultLibraryId)
    {
        return BuildDefaultLibrary();
    }

    QSettings settings("Amazon", "Lumberyard");

    settings.beginGroup(SettingsBaseGroup()); //start of all libraries
    QStringList libraries = settings.childGroups();
    for (QString lib : libraries)
    {
        //name is not stored in group, to compare we need to get the value for it
        QString name = settings.value(lib + "/name", "").toString();
        if (name.compare(libNames[libId], Qt::CaseInsensitive) == 0)
        {
            //remove presets from library
            while (presets[libId].size() > 0)
            {
                RemovePresetFromLibrary(0, libId);
            }
            //then load the new presets
            LoadSessionPresetFromName(&settings, lib);
            settings.endGroup(); //end of all libraries
            return;
        }
    }
    settings.endGroup(); //end of all libraries
}

void QPresetSelectorWidget::PrepareExport(QString filepath)
{
    QStringList libstrlist = filepath.split('.');
    QString libName = "";
    if (libstrlist.count() > 0)
    {
        libName = libstrlist[qMax(libstrlist.count() - 2, 0)]; //second to last item or last item if only one item
    }
    if (libName.length() > 0)
    {
        libName = libName.split('/').back();
    }
    if (m_currentLibrary == m_defaultLibraryId)
    {
        //create a new library, copy the default to the new lib, and then reset default library
        int newlib = AddNewLibrary(libName);
        libFilePaths[newlib] = filepath;
        presets[newlib] = presets[m_currentLibrary];
        m_currentLibrary = newlib;
        DisplayLibrary(m_currentLibrary);
        BuildPanelLayout(false, libNames[m_currentLibrary]);
        SaveDefaultPreset(m_currentLibrary);
    }
    else
    {
        libNames[m_currentLibrary] = libName;
    }
}

void QPresetSelectorWidget::SaveDefaultPreset(int libId)
{
    if (libNames[libId].length() == 0)
    {
        //library is not named, can't save
        return;
    }

    QSettings settings("Amazon", "Lumberyard");
    QString libString = "-"; //special identifier for no saved presets
    settings.beginGroup(SettingsBaseGroup()); //start of all libraries
    //find if the preset exists, if so overwrite it if not add a new one
    QStringList libraries = settings.childGroups();
    for (QString lib : libraries)
    {
        if (settings.value(lib + "/name", "").toString().compare(libNames[libId]) == 0)
        {
            libString = lib;
            break;
        }
    }

    if (libString == "-")
    {
        int digitsToDisplay = 3;
        int numericalBase = 10;
        QChar fillerCharacter = '0';
        //assigning a path of 000-999 to preserve library order, and prevent cases where duplicate names would create conflict
        libString = QString("%1").arg(libraries.count(), digitsToDisplay, numericalBase, fillerCharacter);
    }

    settings.beginGroup(libString); // start of library
    settings.remove("");
    settings.sync();

    settings.setValue("path", libFilePaths[libId]);
    settings.setValue("name", libNames[libId]);
    for (unsigned int preset = 0; preset < presets[libId].count(); preset++)
    {
        SavePresetToSession(&settings, libId, preset);
    }
    settings.endGroup();//end of library
    settings.endGroup(); //end of all libraries
}
