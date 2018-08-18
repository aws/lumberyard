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
#include "paneltitlebar.h"
#include "ContextMenu.h"
#include <PanelWidget/ui_PanelTitleBar.h>
#include <PanelWidget/paneltitlebar.moc>
#include <IEditor.h>
#include <IEditorParticleUtils.h>

#include <QDockWidget>
#include <QMouseEvent>
#include <QLineEdit>
#include "AttributeItem.h"
#include "AttributeView.h"
#include <Undo/Undo.h>
#include "Util/Variable.h"

PanelTitleBar::PanelTitleBar(QWidget* parent)
    : QCopyableWidget(parent)
    , ui(new Ui::PanelTitleBar)
{
    ui->setupUi(this);

    QDockWidget* dw = qobject_cast<QDockWidget*>(parentWidget());
    ui->label->setText(dw->windowTitle());
    ui->label->installEventFilter(this);

    ui->renameCheckButton->setObjectName("PanelTitleBarComfirmRenameBtn");
    storedContents = new QWidget(this);
    //do not apply a size policy to collapsed widget,
    //it will cause the horizontal separator to disappear on the collapsed panels
    //also leave the maximum height at 1, it's a workaround to get separator showing
    //while panel is collapsed. potential style issues, set it to transparent
    storedContents->setMaximumHeight(1);
    storedContents->setWindowOpacity(0.0f);
    setFixedHeight(24);
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);

    // hide panel whenever btnClose is clicked
    ui->btnClose->setObjectName("PanelTitleBarCloseBtn");
    connect(ui->btnClose, &QPushButton::clicked, this, &PanelTitleBar::closePanel);
    EndRename();

    // This makes sure that when this panel is docked as a tab the collapsed state
    //  is set to all other panels in the tab bar.
    connect(dw, &QDockWidget::visibilityChanged, this, &PanelTitleBar::MatchCollapsedStateToOtherTabs);

    // start collapsed
    toggleCollapsed();
    m_collapsed = true;

    // enable background color fill
    setAutoFillBackground(true);

    updateColor();
}

PanelTitleBar::~PanelTitleBar()
{
    delete ui;
}

bool PanelTitleBar::eventFilter(QObject* obj, QEvent* ev)
{
    if (onEventFilter(obj, ev))
    {
        return QObject::eventFilter(obj, ev);
    }

    if (obj == ui->label)
    {
        if (ev->type() == QEvent::MouseButtonRelease)
        {
            QMouseEvent* mev = (QMouseEvent*)ev;
            if (mev->button() == Qt::LeftButton)
            {
                toggleCollapsed();
                repaintAll();
            }
        }
    }

    return QObject::eventFilter(obj, ev);
}

void PanelTitleBar::setCollapsed(bool vCollapsed)
{
    if (m_collapsed != vCollapsed)
    {
        toggleCollapsed();
    }
}

void PanelTitleBar::setAllTabsCollapsed(bool collapsed)
{
    //Set collpased state of all dock widgets tabbed in the same tab bar
    QDockWidget* panel = qobject_cast<QDockWidget*>(parentWidget());
    PanelWidget* panelWidget = panel ? qobject_cast<PanelWidget*>(panel->parentWidget()) : nullptr;
    bool wasTabified = false;
    if (panelWidget)
    {
        QList<QDockWidget*> list = panelWidget->tabifiedDockWidgets(panel);
        for (QDockWidget* d : list)
        {
            PanelTitleBar* t = qobject_cast<PanelTitleBar*>(d->titleBarWidget());

            if (t)
            {
                t->setCollapsed(collapsed);
            }
        }
        wasTabified = list.size() > 0;
    }
    if (!wasTabified)
    {
        //This dock widget is not tabified
        setCollapsed(collapsed);
    }
}

void PanelTitleBar::toggleCollapsed()
{
    // swap stored widget with original widget when the label is left clicked.
    // this will unhide/hide the contents.

    QDockWidget* dw = qobject_cast<QDockWidget*>(parentWidget());
    QWidget* current = dw->widget();

    //hide the about to be stored widget
    dw->widget()->hide();
    dw->setWidget(storedContents);
    //show the newly presented widget
    dw->widget()->show();

    storedContents = current;

    m_collapsed = !m_collapsed;
    setAllTabsCollapsed(m_collapsed);
    if (m_callbackOnCollapse)
    {
        m_callbackOnCollapse();
    }
}

void PanelTitleBar::repaintAll()
{
    QWidget* curr = this;
    while (curr)
    {
        curr->repaint();
        curr = curr->parentWidget();
    }
}

void PanelTitleBar::updateColor()
{
    if (ui->pushButton->text() == "A")
    {
        QPalette pal = ui->pushButton->palette();
        pal.setColor(QPalette::ButtonText, QColor(0x1b, 0x74, 0xce, 255));
        ui->pushButton->setPalette(pal);
    }
    else if (ui->pushButton->text() == "B")
    {
        QPalette pal = ui->pushButton->palette();
        pal.setColor(QPalette::ButtonText, QColor(0xd6, 0x8a, 0x1a, 255));
        ui->pushButton->setPalette(pal);
    }
}

void PanelTitleBar::MatchCollapsedStateToOtherTabs()
{
    QDockWidget* panel = qobject_cast<QDockWidget*>(parentWidget());
    PanelWidget* panelWidget = panel ? qobject_cast<PanelWidget*>(panel->parentWidget()) : nullptr;
    if (panelWidget)
    {
        //Find the first panel that is not this
        // and match it's collapse state
        QList<QDockWidget*> list = panelWidget->tabifiedDockWidgets(panel);
        for (QDockWidget* d : list)
        {
            if (d != panel)
            {
                PanelTitleBar* titleBar = qobject_cast<PanelTitleBar*>(d->titleBarWidget());
                if (titleBar)
                {
                    setCollapsed(titleBar->getCollapsed());
                    break;
                }
            }
        }
    }
}

void PanelTitleBar::on_pushButton_clicked()
{
    if (ui->pushButton->text() == "B")
    {
        ui->pushButton->setText("A");
    }
    else if (ui->pushButton->text() == "A")
    {
        ui->pushButton->setText("B");
    }
    updateColor();
    toggleAdvanced();
}

void PanelTitleBar::paintEvent(QPaintEvent*)
{
    QStyleOption opt;
    opt.init(this);
    QPainter p(this);
    style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);
}

QSize PanelTitleBar::sizeHint() const
{
    return QSize(QWidget::sizeHint().width(), 24);
}
QSize PanelTitleBar::minimumSizeHint() const
{
    return QSize(QWidget::minimumSizeHint().width(), 24);
}

void PanelTitleBar::SetCallbackOnCollapseEvent(std::function<void()> callback)
{
    m_callbackOnCollapse = callback;
}

void PanelTitleBar::StartRename()
{
    ui->nameEditor->blockSignals(true);
    ui->label->hide();
    ui->btnClose->hide();
    ui->nameEditor->setReadOnly(false);
    ui->nameEditor->setText(ui->label->text());
    ui->nameEditor->show();
    ui->renameCheckButton->show();
    ui->nameEditor->setFocus();
    ui->nameEditor->blockSignals(false);
}

void PanelTitleBar::EndRename()
{
    ui->nameEditor->blockSignals(true);
    ui->nameEditor->setReadOnly(true);
    ui->nameEditor->hide();
    ui->renameCheckButton->hide();
    ui->label->show();
    ui->btnClose->show();
    ui->nameEditor->blockSignals(false);
}

void PanelTitleBar::closePanel()
{
    emit SignalPanelClosed((QDockWidget*)parentWidget());
}

ItemPanelTitleBar::ItemPanelTitleBar(QWidget* parent, class CAttributeItem* _item, bool isCustomPanel)
    : PanelTitleBar(parent)
    , item(_item)
    , m_advancedVisible(false) //Note that this should be initialized to match the actual state.
                               //This is currently done from loadAdvancedState.
{
    updateLabel();
    m_variable = item->getVar();

    // hook variable changes
    item->addCallback(functor(*this, &ItemPanelTitleBar::onVarChanged));

    ui->btnClose->setVisible(isCustomPanel);
    m_isCustom = isCustomPanel;


    QAction* exitRename = ui->nameEditor->addAction(QIcon(":/particleQT/buttons/close_btn.png"), QLineEdit::TrailingPosition);
    connect(exitRename, &QAction::triggered, this, &ItemPanelTitleBar::EndRename);
    connect(ui->renameCheckButton, &QPushButton::clicked, this, &ItemPanelTitleBar::RenameRequest);
    connect(ui->nameEditor, &QLineEdit::editingFinished, this, &ItemPanelTitleBar::RenameRequest);

    ui->renameCheckButton->setIcon(QIcon(":/particleQT/icons/check.png"));

    SetCopyMenuFlags(QCopyableWidget::COPY_MENU_FLAGS(QCopyableWidget::COPY_MENU_FLAGS::ENABLE_COPY | QCopyableWidget::COPY_MENU_FLAGS::ENABLE_RESET | QCopyableWidget::COPY_MENU_FLAGS::ENABLE_PASTE));
    SetCopyCallback([=]()
        {
            item->getAttributeView()->copyItem(item, true);
        });
    SetPasteCallback([=]()
        {
            EBUS_EVENT(EditorLibraryUndoRequestsBus, Suspend, true);
            item->getAttributeView()->pasteItem(item);
            EBUS_EVENT(EditorLibraryUndoRequestsBus, Suspend, false);
            emit item->SignalUndoPoint();
        });
    SetCheckPasteCallback([=]()
        {
            return item->getAttributeView()->checkPasteItem(item);
        });
    SetResetCallback([=]()
        {
            EBUS_EVENT(EditorLibraryUndoRequestsBus, Suspend, true);
            item->ResetValueToDefault();
            EBUS_EVENT(EditorLibraryUndoRequestsBus, Suspend, false);
            emit item->SignalUndoPoint();
        });

#ifdef _ENABLE_ADVANCED_BASIC_BUTTONS_
    ui->pushButton->setVisible(!isCustomPanel && item->hasAdvancedChildren());
#else
    ui->pushButton->setVisible(false);
#endif
}

void ItemPanelTitleBar::onVarChanged()
{
    updateLabel();
    updateColor();
}

void ItemPanelTitleBar::updateLabel()
{
    QDockWidget* dw = qobject_cast<QDockWidget*>(parentWidget());
    if (isDefaultValue() == false)
    {
        QPalette palette = item->palette();
        palette.setColor(ui->label->foregroundRole(), QColor(255, 128, 0, 255));
        ui->label->setPalette(palette);
    }
    else
    {
        QPalette palette = item->palette();
        palette.setColor(ui->label->foregroundRole(), QColor(255, 255, 255, 255));
        ui->label->setPalette(palette);
    }
}

ItemPanelTitleBar::~ItemPanelTitleBar()
{
    // no need to unregister callback, title bar will be destroyed after CAttributeItem destruction.
}

bool ItemPanelTitleBar::isDefaultValue()
{
    return item->isDefaultValue();
}

void ItemPanelTitleBar::updateColor()
{
    if (item->isDefaultValue())
    {
        //Set text color to default
        QPalette pal = ui->pushButton->palette();
        pal.setColor(QPalette::ButtonText, QColor(0xCC, 0xCC, 0xCC, 255));
        ui->pushButton->setPalette(pal);
    }
    else
    {
        PanelTitleBar::updateColor();
    }
}

void ItemPanelTitleBar::toggleAdvanced()
{
    showAdvanced(ui->pushButton->text().compare("A") == 0);
}
void ItemPanelTitleBar::showAdvanced(bool show)
{
    ui->pushButton->setText(show ? "A" : "B");
    m_advancedVisible = show;
    updateColor();
    item->showAdvanced(show);
}

QWidget* ItemPanelTitleBar::getStoredWidget()
{
    return storedContents;
}

bool ItemPanelTitleBar::onEventFilter(QObject* obj, QEvent* ev)
{
    if (obj == ui->label)
    {
        if (ev->type() == QEvent::MouseButtonRelease)
        {
            QMouseEvent* mev = (QMouseEvent*)ev;
            if (mev->button() == Qt::RightButton)
            {
                LaunchMenu(mev->globalPos());
            }
        }
    }
    return false;
}

void ItemPanelTitleBar::LaunchMenu(QPoint pos)
{
    ContextMenu menu(this);

    //RebuildMenu();
    if (m_isCustom)
    {
        BuildCustomMenu(&menu);
    }
    else
    {
        BuildMenu(&menu);
    }
    menu.setFocus();
    menu.exec(pos);
}

void ItemPanelTitleBar::RenameRequest()
{
    ui->nameEditor->blockSignals(true);
    emit SignalPanelRenameStart((QDockWidget*)parentWidget(), ui->label->text(), ui->nameEditor->text());
    ui->nameEditor->blockSignals(false);
}

void ItemPanelTitleBar::RenamePanelName(QString name)
{
    ui->label->setText(name);
    EndRename();
}

void ItemPanelTitleBar::BuildCustomMenu(QMenu *menu)
{
    // just rebuild menu by clearing
    menu->clear();

    menu->addAction("Rename", this, &ItemPanelTitleBar::StartRename);
    menu->addAction("Remove all", this, [this]
        {
            emit SignalRemoveAllParams((QDockWidget*)parentWidget());
        });

    menu->addAction("Export", this, [this]
        {
            QString fileName = QFileDialog::getSaveFileName(this, tr("Export Panel"), QString(), tr("Panel file (*.custom_attribute)"));
            if (!fileName.isEmpty())
            {
                emit SignalPanelExportPanel((QDockWidget*)parentWidget(), fileName);
            }
        }, GetIEditor()->GetParticleUtils()->HotKey_GetShortcut("Attributes.Export Panel"));

    menu->addAction("Close", this, &ItemPanelTitleBar::closePanel);
}
