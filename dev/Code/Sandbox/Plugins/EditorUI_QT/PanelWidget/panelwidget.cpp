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
#include "PanelWidget.h"
#include <PanelWidget/ui_PanelWidget.h>

#include <PanelWidget/PanelWidget.moc>

#include "paneltitlebar.h"
#include "../AttributeListView.h"
#include "ActionWidgetColoredCheckbox.h"
#include "IEditorParticleUtils.h"


#include <QDockWidget>
#include <QLabel>
#include <QVBoxLayout>
#include <QSpacerItem>
#include <QScrollArea>
#include "AttributeItem.h"
#include "ContextMenu.h"

#define USE_CUSTOM_CHECKBOXES 1 //Set to 1 to have color coded show/hide context menu checkboxes for panels

#define EXTRA_ROOM_TO_CORRECT_FOR_RESIZE_ISSUES 1024


PanelWidget::PanelWidget(QWidget* parent)
    : QMainWindow(parent)
    , ui(new Ui::PanelWidget)
{
    ui->setupUi(this);
    statusBar()->hide();
    setCentralWidget(nullptr);
    setTabShape(QTabWidget::Triangular);
    setTabPosition(Qt::AllDockWidgetAreas, QTabWidget::South);
    //remove context menu
    setContextMenuPolicy(Qt::NoContextMenu);
    m_correctionWidget = new QDockWidget(this);
    m_correctionWidget->setWindowOpacity(0);
    m_correctionWidget->setTitleBarWidget(nullptr);
    m_correctionWidget->setFeatures(QDockWidget::DockWidgetFeature::NoDockWidgetFeatures);
    m_correctionWidget->setStyleSheet("QDockWidget::title{background: transparent; color: transparent;};");
    m_correctionWidget->setObjectName("PanelCorrectionWidget");
    addDockWidget(Qt::DockWidgetArea::BottomDockWidgetArea, m_correctionWidget, Qt::Orientation::Vertical);
}

PanelWidget::~PanelWidget()
{
    if (ui)
    {
        delete ui;
        ui = nullptr;
    }
}

QWidget* PanelWidget::addItemPanel(const char* name, CAttributeItem* attr_item, bool scrollableArea /*=false*/, bool isCustomPanel /*=false*/, QString isShowInGroup /* = "both"*/)
{
    //Make sure the background color of our contents is different from our parent

    CAttributeListView* dWidget = new CAttributeListView(isCustomPanel, isShowInGroup);
    QScrollArea* dWidgetScrollArea;
    if (scrollableArea)
    {
        dWidgetScrollArea = new QScrollArea();
        dWidgetScrollArea->setFrameShape(QFrame::NoFrame);
        dWidgetScrollArea->setWidget(attr_item);
        dWidgetScrollArea->setMaximumHeight(attr_item->layout()->minimumSize().height());

        dWidgetScrollArea->setWidgetResizable(true);
        attr_item->setSizePolicy(QSizePolicy(QSizePolicy::Ignored, QSizePolicy::Maximum));
        dWidget->setWidget(dWidgetScrollArea);
    }
    else
    {
        //This breaks horizontal resizing when only the left panel is expanded,
        // this fixes the problem where the panels take up all the remaining space.
        //attr_item->setSizePolicy(QSizePolicy(QSizePolicy::Preferred, QSizePolicy::Maximum));
        dWidget->setWidget(attr_item);

        attr_item->setSizePolicy(QSizePolicy(attr_item->sizePolicy().horizontalPolicy(), QSizePolicy::Maximum));
    }

    connect(dWidget, &QDockWidget::topLevelChanged, dWidget, [dWidget](bool isTopLevel)
        {
            dWidget->setWindowOpacity(isTopLevel ? 0.5 : 1.0);
        });

    dWidget->setObjectName(QString("obj_") + name);
    dWidget->setWindowTitle(name);
    ItemPanelTitleBar* bar = new ItemPanelTitleBar(dWidget, attr_item, isCustomPanel);
    bar->setCollapsed(false);
    dWidget->setTitleBarWidget(bar);
    dWidget->setFeatures(isCustomPanel ? (QDockWidget::DockWidgetClosable | QDockWidget::DockWidgetMovable) : QDockWidget::DockWidgetMovable);
    if (isCustomPanel)
    {
        connect(bar, &ItemPanelTitleBar::SignalPanelRenameStart, this, &PanelWidget::PassThroughSignalPanelRename);
        connect(bar, &ItemPanelTitleBar::SignalPanelExportPanel, this, &PanelWidget::PassThroughSignalPanelExport);
        connect(bar, &ItemPanelTitleBar::SignalPanelClosed, this, &PanelWidget::ClosePanel);
        connect(bar, &ItemPanelTitleBar::SignalRemoveAllParams, this, [=](QDockWidget* panel){emit SignalRemoveAllParams(panel); });
        CreateActions(dWidget);
    }
    addDockWidget(m_mainDockWidgetArea, dWidget, Qt::Orientation::Vertical);
    dWidget->show();
    bar->setCollapsed(true);
    m_widgets.push_back(dWidget);
    bar->SetCallbackOnCollapseEvent([=](){ FixSizing(true); });
    return dWidget;
}

QWidget* PanelWidget::addPanel(const char* name, QWidget* contentWidget, bool scrollableArea /*=false*/)
{
    //Make sure the background color of our contents is different from our parent
    QDockWidget* dWidget = new QDockWidget();
    QScrollArea* dWidgetScrollArea;
    if (scrollableArea)
    {
        dWidgetScrollArea = new QScrollArea();
        dWidgetScrollArea->setFrameShape(QFrame::NoFrame);
        dWidgetScrollArea->setWidget(contentWidget);
        dWidgetScrollArea->setMaximumHeight(contentWidget->layout()->minimumSize().height());

        dWidgetScrollArea->setWidgetResizable(true);
        contentWidget->setSizePolicy(QSizePolicy(QSizePolicy::Ignored, QSizePolicy::Maximum));
        dWidget->setWidget(dWidgetScrollArea);
        this->stackUnder(dWidgetScrollArea);
    }
    else
    {
        //This breaks horizontal resizing when only the left panel is expanded,
        // this fixes the problem where the panels take up all the remaining space.
        //contentWidget->setSizePolicy(QSizePolicy(QSizePolicy::Preferred, QSizePolicy::Maximum));
        dWidget->setWidget(contentWidget);
        this->stackUnder(contentWidget);
    }

    dWidget->setObjectName(QString("obj_") + name);
    dWidget->setWindowTitle(name);
    PanelTitleBar* bar = new PanelTitleBar(dWidget);
    bar->setCollapsed(false);
    dWidget->setTitleBarWidget(bar);
    dWidget->setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetClosable);
    addDockWidget(m_mainDockWidgetArea, dWidget, Qt::Orientation::Vertical);
    dWidget->show();
    bar->setCollapsed(true);

    return dWidget;
}

void PanelWidget::addShowPanelItemsToMenu(QMenu* menu)
{
    //Counters used to determine if extra menu options for show/hide should be visible
    int visiblePanelCount = 0;
    int panelCount = 0;
    int visibleChangedPanels = 0;
    int invisibleChangedPanels = 0;

    for (auto it : children())
    {
        // check if it's the type we're looking for
        QDockWidget* dw = qobject_cast<QDockWidget*>(it);
        if (dw == nullptr)
        {
            continue;
        }
        if (!(dw->features() & QDockWidget::DockWidgetClosable))
        {
            continue;
        }

        if (dw->isVisible())
        {
            visiblePanelCount++;
        }
        panelCount++;

        PanelTitleBar* titlebar = qobject_cast<PanelTitleBar*>(dw->titleBarWidget());
        bool panelContainsChanges = false;

        ItemPanelTitleBar* itembar = qobject_cast<ItemPanelTitleBar*>(dw->titleBarWidget());

        if (itembar != nullptr)
        {
            panelContainsChanges = !itembar->isDefaultValue();
            if (!itembar->isDefaultValue())
            {
                if (itembar->isVisible())
                {
                    visibleChangedPanels++;
                }
                else
                {
                    invisibleChangedPanels++;
                }
            }
        }

        QAction* action;
#if USE_CUSTOM_CHECKBOXES
        ActionWidgetColoredCheckboxAction* widgetAction = new ActionWidgetColoredCheckboxAction(menu);
        action = widgetAction;
#endif
        if (titlebar != nullptr)
        {
#if USE_CUSTOM_CHECKBOXES
            widgetAction->setCaption(titlebar->parentWidget()->windowTitle());
#else
            action = menu->addAction(titlebar->parentWidget()->windowTitle());
#endif
        }
        else
        {
#if USE_CUSTOM_CHECKBOXES
            widgetAction->setCaption("Unnamed widget");
#else
            action = menu->addAction("Unnamed widget");
#endif
        }
#if USE_CUSTOM_CHECKBOXES
        widgetAction->setColored(panelContainsChanges);
        widgetAction->setChecked(titlebar->isVisible());
#else
        if (panelContainsChanges)
        {
            QFont font = action->font();
            font.setBold(true);
            action->setFont(font);
        }
        action->setCheckable(true);
        action->setChecked(titlebar->isVisible());
#endif

        connect(action, &QAction::triggered, dw, [dw, menu]
            {
                //Set panel visibility
                dw->setVisible(!dw->isVisible());

#if USE_CUSTOM_CHECKBOXES
                //Hide menu
                QWidget* w = menu;
                while (w)
                {
                    if (qobject_cast<QMenu*>(w))
                    {
                        w->hide();
                        w = w->parentWidget();
                    }
                    else
                    {
                        break;
                    }
                }
#endif
            }
            );
        menu->addAction(action);
    }
    menu->addSeparator();


    //Add extra menu options to show/hide multiple panels
    if (visiblePanelCount < panelCount)
    {
        connect(menu->addAction("Show All"), &QAction::triggered, this, [this]()
            {
                for (auto it : children())
                {
                    // check if it's the type we're looking for
                    QDockWidget* dw = qobject_cast<QDockWidget*>(it);
                    if (dw == nullptr)
                    {
                        continue;
                    }

                    //Set panel visibility
                    dw->setVisible(true);
                }
            });
    }

    if (visiblePanelCount > 0)
    {
        connect(menu->addAction("Hide All"), &QAction::triggered, this, [this]()
            {
                for (auto it : children())
                {
                    // check if it's the type we're looking for
                    QDockWidget* dw = qobject_cast<QDockWidget*>(it);
                    if (dw == nullptr)
                    {
                        continue;
                    }

                    //Set panel visibility
                    dw->setVisible(false);
                }
            });
    }

    if (invisibleChangedPanels > 0)
    {
        connect(menu->addAction("Show Changed"), &QAction::triggered, this, [this]()
            {
                for (auto it : children())
                {
                    // check if it's the type we're looking for
                    QDockWidget* dw = qobject_cast<QDockWidget*>(it);
                    if (dw == nullptr)
                    {
                        continue;
                    }

                    ItemPanelTitleBar* itembar = qobject_cast<ItemPanelTitleBar*>(dw->titleBarWidget());

                    if (itembar != nullptr)
                    {
                        if (!itembar->isDefaultValue())
                        {
                            //Set panel visibility
                            dw->setVisible(true);
                        }
                    }
                }
            });
    }
}

QString PanelWidget::saveCollapsedState()
{
    QString str;

    iterateOver([&str](QDockWidget* dw, PanelTitleBar* titlebar)
        {
            // store when it's uncollapsed
            if (titlebar->getCollapsed() == false)
            {
                str += dw->windowTitle() + "||";
            }
        });
    return str;
}

void PanelWidget::loadCollapsedState(const QString& str)
{
    QStringList uncollapsed = str.split("||", QString::SkipEmptyParts);

    // build a set so that we can quickly determine whether to uncollapse something
    QSet<QString> uncollapsedSet;
    for (auto str : uncollapsed)
    {
        uncollapsedSet.insert(str);
    }

    // uncollapse children if needed
    iterateOver([&uncollapsedSet](QDockWidget* dw, PanelTitleBar* titlebar)
        {
            if (uncollapsedSet.contains(dw->windowTitle()))
            {
                titlebar->setCollapsed(false);
            }
            else
            {
                titlebar->setCollapsed(true);
            }
        });
}

QString PanelWidget::saveAdvancedState()
{
    QString str;

    iterateOver([&str](QDockWidget* dw, PanelTitleBar* titlebar)
        {
            // store when advanced options are visible
            ItemPanelTitleBar* itemTitleBar = qobject_cast<ItemPanelTitleBar*>(titlebar);
            if (itemTitleBar)
            {
                if (itemTitleBar->isAdvancedVisible())
                {
                    str += dw->windowTitle() + "||";
                }
            }
        });
    return str;
}

void PanelWidget::loadAdvancedState(const QString& str)
{
#ifdef _ENABLE_ADVANCED_BASIC_BUTTONS_
    QStringList advanced = str.split("||", QString::SkipEmptyParts);

    // build a set so that we can quickly determine whether a panel has advanced options visible
    QSet<QString> unadvancedSet;
    for (auto str : advanced)
    {
        unadvancedSet.insert(str);
    }

    // enable/disable advanced options on children
    iterateOver([this, &str, &unadvancedSet](QDockWidget* dw, PanelTitleBar* titlebar)
        {
            ItemPanelTitleBar* itemTitleBar = qobject_cast<ItemPanelTitleBar*>(titlebar);
            if (itemTitleBar)
            {
                if (unadvancedSet.contains(dw->windowTitle()))
                {
                    itemTitleBar->showAdvanced(true);
                }
                else
                {
                    itemTitleBar->showAdvanced(false);
                }
            }
        });
#else
    iterateOver([](QDockWidget* dw, PanelTitleBar* titlebar)
        {
            ItemPanelTitleBar* itemTitleBar = qobject_cast<ItemPanelTitleBar*>(titlebar);
            itemTitleBar->showAdvanced(true);
        });
#endif
}

void PanelWidget::finalizePanels()
{
    m_defaultSettings = saveAll();
}

void PanelWidget::iterateOver(std::function<void(QDockWidget*, PanelTitleBar*)> iteratorReceiver)
{
    for (auto it : m_widgets)
    {
        // check if it's the type we're looking for
        QDockWidget* dw = qobject_cast<QDockWidget*>(it);
        if (dw == nullptr)
        {
            continue;
        }
        PanelTitleBar* titlebar = qobject_cast<PanelTitleBar*>(dw->titleBarWidget());
        if (titlebar == nullptr)
        {
            continue;
        }

        iteratorReceiver(dw, titlebar);
    }
}

bool PanelWidget::isEmpty()
{
    bool empty = true;
    iterateOver([&empty](QDockWidget*, PanelTitleBar*){empty = false; }); //Overkill
    return empty;
}

void PanelWidget::ClearAllPanels()
{
    if (!isEmpty())
    {
        iterateOver([=](QDockWidget* widget, PanelTitleBar* titlebar)
            {
                ClosePanel(widget);
            });

        while (m_widgets.count())
        {
            delete m_widgets.takeFirst();
        }

        m_widgets.clear();
    }
}

void PanelWidget::ClearCustomPanels()
{
    if (!isEmpty())
    {
        iterateOver([=](QDockWidget* widget, PanelTitleBar* titlebar)
            {
                CAttributeListView* listView = static_cast<CAttributeListView*>(widget);
                if (listView->isCustomPanel())
                {
                    ClosePanel(widget);
                    for (int i = 0; i < m_widgets.count(); i++)
                    {
                        if (m_widgets[i] == widget)
                        {
                            m_widgets.remove(i);
                            break;
                        }
                    }
                }
            });
    }
}


void PanelWidget::ResetGeometry()
{
    loadAll(m_defaultSettings);
}

static unsigned int gSaveMagicValue = 0x9340245;

QByteArray PanelWidget::saveAll()
{
    QByteArray combined;
    combined.append((char*)&gSaveMagicValue, sizeof(gSaveMagicValue));

    QByteArray geometry = saveState(0);
    int geometryLength = geometry.size();
    combined.append((char*)&geometryLength, sizeof(int));
    combined.append(geometry);

    QString collapsedState = saveCollapsedState();
    QByteArray collapsedStateBytes = collapsedState.toUtf8();
    int collapsedStateLength = collapsedStateBytes.size();
    combined.append((char*)&collapsedStateLength, sizeof(int));
    combined.append(collapsedStateBytes);

    QString advancedState = saveAdvancedState();
    QByteArray advancedStateBytes = advancedState.toUtf8();
    int advancedStateLength = advancedStateBytes.size();
    combined.append((char*)&advancedStateLength, sizeof(int));
    combined.append(advancedStateBytes);

    return combined;
}

void PanelWidget::loadAll(const QByteArray& vdata)
{
    int offset = 0;

    unsigned int& magicValue = *(unsigned int*)(vdata.data() + offset);
    if (magicValue != gSaveMagicValue)  // check if save still valid
    {
        QMessageLogger logger;
        logger.warning("PanelWidget::loadAll vdata not valid");
        return;
    }
    offset += sizeof(gSaveMagicValue);

    int& geometryLength = *(int*)(vdata.data() + offset);
    QByteArray geometry = vdata.mid(offset + sizeof(int), geometryLength);
    restoreState(geometry, 0);
    offset += sizeof(int) + geometryLength;

    int& collapsedStateLength = *(int*)(vdata.data() + offset);
    QByteArray collapsedStateBytes = vdata.mid(offset + sizeof(int), collapsedStateLength);
    QString collapsedState = QString::fromUtf8(collapsedStateBytes);
    loadCollapsedState(collapsedState);
    offset += sizeof(int) + collapsedStateLength;

    int& advancedStateLength = *(int*)(vdata.data() + offset);
    QByteArray advancedStateBytes = vdata.mid(offset + sizeof(int), advancedStateLength);
    QString advancedState = QString::fromUtf8(advancedStateBytes);
    loadAdvancedState(advancedState);
    offset += sizeof(int) + collapsedStateLength;
}

void PanelWidget::paintEvent(QPaintEvent*)
{
    QStyleOption opt;
    opt.init(this);
    QPainter p(this);
    style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);
}

void PanelWidget::FixSizing(bool includeRoomForExpansion /*= false*/)
{
    //magic number is to provide "growing room" for the panels,
    //this fixes the bug where dragging panels that are side by side to one above the other could
    //fail due to running out of room.
    int roomForExpansion = EXTRA_ROOM_TO_CORRECT_FOR_RESIZE_ISSUES;
    int expectedHeight = GetTotalPanelHeight(roomForExpansion);
    if (!includeRoomForExpansion)
    {
        roomForExpansion = 0;
    }
    setMinimumHeight(expectedHeight + roomForExpansion);
}

int PanelWidget::GetTotalPanelHeight(int& largestHeight)
{
    int total = 0;
    int current = 0;
    for (QDockWidget* widget : m_widgets)
    {
        if (widget != nullptr)
        {
            current = widget->height();
            total += current;
            if (current > largestHeight)
            {
                largestHeight = current;
            }
        }
    }
    return total;
}

void PanelWidget::AddCustomPanel(QDockWidget* dockPanel, bool addToTop /*= true*/)
{
    CRY_ASSERT(dockPanel);

    if (addToTop && m_widgets.count() > 0)
    {
        // Find Top widget by position y
        QDockWidget* topWidget = nullptr;
        for (unsigned int i = 0; i < m_widgets.count(); i++)
        {
            if (m_widgets[i]->pos().y() < 0)
            {
                continue;
            }
            if (!m_widgets[i]->isVisible())
            {
                continue;
            }
            if (!topWidget)
            {
                topWidget = m_widgets[i];
            }
            else
            {
                if (m_widgets[i]->pos().y() <= topWidget->pos().y())
                {
                    topWidget = m_widgets[i];
                }
            }
        }
        if (topWidget)
        {
            topWidget->show();
            if (!tabifiedDockWidgets(topWidget).isEmpty())
            {
                tabifyDockWidget(topWidget, dockPanel);
            }
            else
            {
                // Insert Top Widget
                splitDockWidget(topWidget, dockPanel, Qt::Orientation::Vertical);
                splitDockWidget(dockPanel, topWidget, Qt::Orientation::Vertical);
            }
        }
        else
        {
            addDockWidget(m_mainDockWidgetArea, dockPanel, Qt::Orientation::Vertical);
        }
    }
    else
    {
        addDockWidget(m_mainDockWidgetArea, dockPanel, Qt::Orientation::Vertical);
    }
    dockPanel->setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetClosable);

    ItemPanelTitleBar* bar = static_cast<ItemPanelTitleBar*>(dockPanel->titleBarWidget());
    if (bar)
    {
        connect(bar, &ItemPanelTitleBar::SignalPanelRenameStart, this, &PanelWidget::PassThroughSignalPanelRename);
        connect(bar, &ItemPanelTitleBar::SignalPanelExportPanel, this, &PanelWidget::PassThroughSignalPanelExport);
        connect(bar, &ItemPanelTitleBar::SignalPanelClosed, this, &PanelWidget::ClosePanel);
        connect(bar, &ItemPanelTitleBar::SignalRemoveAllParams, this, [=](QDockWidget* panel){emit SignalRemoveAllParams(panel); });
    }
    CreateActions(dockPanel);

    m_widgets.push_back(dockPanel);
    m_widgets.back()->installEventFilter(this);
    dockPanel->show();
    dockPanel->raise();
}

void PanelWidget::ClosePanel(QDockWidget* panel)
{
    emit SignalRemovePanel(panel);
    RemovePanel(panel);
}

void PanelWidget::RemovePanel(QDockWidget* dockPanel)
{
    if (!tabifiedDockWidgets(dockPanel).isEmpty())
    {
        dockPanel->setFloating(true);
        addDockWidget(Qt::DockWidgetArea::BottomDockWidgetArea, dockPanel, Qt::Orientation::Vertical);
        dockPanel->setFloating(false);
    }
    dockPanel->hide();
    removeDockWidget(dockPanel);
    dockPanel->removeEventFilter(this);
    for (unsigned int i = 0; i < m_widgets.count(); i++)
    {
        if (m_widgets[i] == dockPanel)
        {
            m_widgets.removeAt(i);
            return;
        }
    }
}

bool PanelWidget::eventFilter(QObject* obj, QEvent* e)
{
    if (e->type() == QEvent::Resize)
    {
        QResizeEvent* evnt = static_cast<QResizeEvent*>(e);
        if (evnt)
        {
            for (unsigned int i = 0; i < m_widgets.count(); i++)
            {
                if (obj == static_cast<QObject*>(m_widgets[i]))
                {
                    QSize objSize = evnt->size();
                    if (objSize.height() >  m_correctionWidget->size().height())
                    {
                        m_correctionWidget->resize(QSize(m_correctionWidget->width(), objSize.height()));
                    }
                }
            }
        }
    }
    return QMainWindow::eventFilter(obj, e);
}

void PanelWidget::CreateActions(QDockWidget* dwidget)
{
    CRY_ASSERT(GetIEditor());
    CRY_ASSERT(GetIEditor()->GetParticleUtils());
    IEditorParticleUtils* utils = GetIEditor()->GetParticleUtils();

    QAction* action = new QAction("Export...", this);
    action->setShortcut(utils->HotKey_GetShortcut("Attributes.Export Panel"));
    action->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    connect(action, &QAction::triggered, this, [=]()
        {
            QString fileName = QFileDialog::getSaveFileName(this, tr("Export Panel"), QString(), tr("Panel file (*.custom_attribute)"));
            if (fileName.size() > 0)
            {
                emit SignalExportPanel(dwidget, fileName);
            }
        });
    dwidget->addAction(action);
}

void PanelWidget::PassThroughSignalPanelRename(QDockWidget* panel, QString origName, QString newName)
{
    emit SignalRenamePanel(panel, origName, newName);
}

void PanelWidget::PassThroughSignalPanelExport(QDockWidget* panel, QString name)
{
    emit SignalExportPanel(panel, name);
}
