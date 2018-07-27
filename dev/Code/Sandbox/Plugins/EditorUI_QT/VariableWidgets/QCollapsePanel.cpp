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
#include "QCollapsePanel.h"
#include <VariableWidgets/ui_QCollapsePanel.h>
#include <QtWidgets/QLabel>
#include <QMouseEvent>

#ifdef EDITOR_QT_UI_EXPORTS
    #include "AttributeItem.h"
    #include "AttributeView.h"
    #include <VariableWidgets/QCollapsePanel.moc>

    #define STATE_COLLAPSED "collapsed"
    #define STATE_UNCOLLAPSED "uncollapsed"
#endif
#include "QCopyModalDialog.h"
#include "Undo/Undo.h"
#include "../ContextMenu.h"

QCollapsePanel::QCollapsePanel(QWidget* parent, CAttributeItem* attributeItem)
    : QCopyableWidget(parent)
    , ui(new Ui::QCollapsePanel)
    , m_attributeItem(attributeItem)
    , m_optionalCollapseAction(nullptr)
    , m_menu(new ContextMenu(this))
{
    ui->setupUi(this);

    #ifdef EDITOR_QT_UI_EXPORTS
    if (m_attributeItem)
    {
        const QString path = m_attributeItem->getAttributePath(true);
        setCollapsed(m_attributeItem->getAttributeView()->getValue(path, STATE_UNCOLLAPSED) == STATE_COLLAPSED);

        setObjectName(QString(m_attributeItem->getVarName()));

        // Create and add action which collapses and uncollapses the panel
        // It is important to set nullptr as parent, to avoid double delete
        QAction* action = new QAction(objectName(), nullptr);
        action->setCheckable(true);
        action->setChecked(!isCollapsed());
        action->setUserData(0, this);
        action->connect(action, &QAction::triggered, this, [this, action](bool state)
            {
                setCollapsed(!isCollapsed());
                action->setChecked(!isCollapsed());
            });

        m_optionalCollapseAction = action;

        m_attributeItem->getAttributeView()->addToCollapseContextMenu(action);
    }
    #else
    setCollapsed(false);
    #endif

    setContextMenuPolicy(Qt::CustomContextMenu);
    SetCopyMenuFlags(QCopyableWidget::COPY_MENU_FLAGS(QCopyableWidget::COPY_MENU_FLAGS::ENABLE_COPY | QCopyableWidget::COPY_MENU_FLAGS::ENABLE_RESET | QCopyableWidget::COPY_MENU_FLAGS::ENABLE_PASTE));
    SetCopyCallback([=]()
        {
            m_attributeItem->getAttributeView()->copyItem(m_attributeItem, true);
        });
    SetPasteCallback([=]()
        {
            EBUS_EVENT(EditorLibraryUndoRequestsBus, Suspend, true);
            m_attributeItem->getAttributeView()->pasteItem(m_attributeItem);
            EBUS_EVENT(EditorLibraryUndoRequestsBus, Suspend, false);
            emit m_attributeItem->SignalUndoPoint();
        });
    SetCheckPasteCallback([=]()
        {
            return m_attributeItem->getAttributeView()->checkPasteItem(m_attributeItem);
        });
    SetResetCallback([=]()
        {
            EBUS_EVENT(EditorLibraryUndoRequestsBus, Suspend, true);
            m_attributeItem->ResetValueToDefault();
            EBUS_EVENT(EditorLibraryUndoRequestsBus, Suspend, false);
            emit m_attributeItem->SignalUndoPoint();
        });
}

QCollapsePanel::~QCollapsePanel()
{
    delete ui;
}

void QCollapsePanel::addWidget(QCopyableWidget* widget)
{
    ui->containerLayout->addWidget(widget);
}

void QCollapsePanel::setTitle(const QString& title)
{
    ui->CaptionButton->setText(title);
}

QVBoxLayout* QCollapsePanel::getChildLayout()
{
    return ui->containerLayout;
}

void QCollapsePanel::setCollapsed(bool state)
{
    QIcon icon;
    if (state)
    {
        ui->containerWidget->setHidden(true);
        icon.addFile(QStringLiteral("Editor/UI/Icons/QCollapsePanel/collapsed.png"), QSize(), QIcon::Normal, QIcon::Off);
    }
    else
    {
        ui->containerWidget->setHidden(false);
        icon.addFile(QStringLiteral("Editor/UI/Icons/QCollapsePanel/open.png"), QSize(), QIcon::Normal, QIcon::Off);
    }

    #ifdef EDITOR_QT_UI_EXPORTS
    // Save state
    if (m_attributeItem)
    {
        const QString path = m_attributeItem->getAttributePath(true);
        m_attributeItem->getAttributeView()->setValue(path, state ? STATE_COLLAPSED : STATE_UNCOLLAPSED);

        if (m_optionalCollapseAction)
        {
            m_optionalCollapseAction->setChecked(!state);
        }
    }
    #endif

    ui->CaptionButton->setIcon(icon);
}

bool QCollapsePanel::isCollapsed() const
{
    return ui->containerWidget->isHidden();
}

void QCollapsePanel::on_CaptionButton_clicked()
{
    const bool collapsed = !ui->containerWidget->isHidden();
    setCollapsed(collapsed);
}

void QCollapsePanel::mousePressEvent(QMouseEvent* e)
{
    const QRect& rect = ui->CaptionButton->rect();
    if (m_attributeItem && e->button() == Qt::MouseButton::RightButton && rect.intersects(QRect(e->pos(), QSize(1, 1))))
    {
        //m_attributeItem->getAttributeView()->openCollapseMenu(mapToGlobal(e->pos()));
        LaunchMenu(e->globalPos());
    }
}
