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
#include "QColumnWidget.h"
#include <VariableWidgets/QColumnWidget.moc>
#include "AttributeItem.h"
#include "AttributeView.h"
#include <Controls/QBitmapPreviewDialogImp.h>
#include "VariableWidgets/QColorWidgetImp.h"
#include <AzQtComponents/Components/Widgets/CheckBox.h>

// QT
#include <QEvent>

// EditorCore
#include "Util/Variable.h"
#include "Util/VariablePropertyType.h"

// Undo
#include <Undo/Undo.h>
#include "IEditorParticleUtils.h"
#include <Controls/QToolTipWidget.h>
#include "../Utils.h"

#define STATE_COLLAPSED "collapsed"
#define STATE_UNCOLLAPSED "uncollapsed"

QColumnWidget::QCustomLabel::QCustomLabel(const QString& label, CAttributeItem* parent, bool collapsible)
    : QLabel(label, parent)
    , m_parent(parent)
    , m_tooltip(NULL)
    , m_propertyType(Prop::Description(m_parent->getVar()).m_type)
{
    setMouseTracking(true);

    m_variable = m_parent->getVar();
    if (m_variable)
    {
        auto funcSet = functor(*this, &QColumnWidget::QCustomLabel::onVarChanged);
        m_variable->AddOnSetCallback(funcSet);

        //Check if we are inside a group
        QString path = parent->getAttributePath(false);
        QStringList l = path.split(".");
        int groupDepth = l.count() - 2;

        //Set indent to group depth
        SetCollapsible(collapsible);
    }
    
    m_tooltip = new QToolTipWrapper(this);
}

QColumnWidget::QCustomLabel::~QCustomLabel()
{
    if (m_variable)
    {
        m_variable->RemoveOnSetCallback(functor(*this, &QColumnWidget::QCustomLabel::onVarChanged));
    }
}

void QColumnWidget::QCustomLabel::onVarChanged(IVariable* var)
{
    m_parent->getAttributeView()->UpdateLogicalChildren(var);
    m_parent->getAttributeView()->updateCallbacks(m_parent);
}

void QColumnWidget::QCustomLabel::mouseMoveEvent(QMouseEvent* e)
{
    QWidget::mouseMoveEvent(e);

    IVariable* var = m_parent->getVar();

    // TODO: How to handle custom group tooltips?
}

bool QColumnWidget::QCustomLabel::event(QEvent* e)
{
    IVariable* var = m_parent->getVar();
    switch (e->type())
    {
    case QEvent::ToolTip:
    {
        if (!var)
        {
            return true;
        }
        QHelpEvent* event = (QHelpEvent*)e;
        QPoint ep = event->globalPos();

        QPoint p = mapToGlobal(QPoint(0, 0));
        p.setX(p.x() + indent());
        QFontMetrics fm(font());
        QRect widgetRect = QRect(p, QSize(fm.horizontalAdvance(text()), fm.height()));

        GetIEditor()->GetParticleUtils()->ToolTip_BuildFromConfig(m_tooltip, m_parent->getAttributeView()->GetVariablePath(m_variable), "", QString(m_parent->getVar()->GetDisplayValue()), isEnabled());

        m_tooltip->TryDisplay(ep, widgetRect, QToolTipWidget::ArrowDirection::ARROW_LEFT);

        return true;
    }
    case QEvent::Leave:
    {
        if (m_tooltip->isVisible())
        {
            m_tooltip->hide();
        }

        m_parent->getAttributeView()->setToolTip("");
        break;
    }
    default:
        break;
    }
    return QLabel::event(e);
}

bool QColumnWidget::QCustomLabel::eventFilter(QObject* obj, QEvent* e)
{
    return QLabel::eventFilter(obj, e);
}

void QColumnWidget::QCustomLabel::SetCollapsible(bool val)
{
    QString path = m_parent->getAttributePath(false);
    QStringList l = path.split(".");
    int groupDepth = l.count() - 2;
    m_collapsible = val;
    //24 is the size of the icon for QCollapseWidget, add to make the first letters of the label align
    setIndent((groupDepth * 16) + ((!m_collapsible) ? 16 : 0));
}

void QColumnWidget::mousePressEvent(QMouseEvent* e)
{
    if (e->button() == Qt::MouseButton::LeftButton)
    {
        setFocus();
    }
    QCopyableWidget::mousePressEvent(e);
}

void QColumnWidget::keyPressEvent(QKeyEvent* event)
{
    QCopyableWidget::keyPressEvent(event);
}


QColumnWidget::QColumnWidget(CAttributeItem* parent, const QString& label, QWidget* widget, bool collapsible, bool fullWidth)
    : QCopyableWidget(parent)
{
    m_parent = parent;
    m_variable = parent->getVar();
    setMouseTracking(true);
    m_topLayout = new QVBoxLayout(this);
    m_containerLayout = new QHBoxLayout();
    m_topLayout->addLayout(m_containerLayout);
    m_leftLayout = new QHBoxLayout();
    m_rightLayout = new QHBoxLayout();
    QGridLayout* gridLayout = new QGridLayout();
    gridLayout->addLayout(m_leftLayout, 0, 0);
    gridLayout->addLayout(m_rightLayout, 0, 1);
    gridLayout->setColumnStretch(0, 1);
    gridLayout->setColumnStretch(1, 1);
    m_containerLayout->addLayout(gridLayout);
    m_containerLayout->setSpacing(0);
    m_containerLayout->setContentsMargins(0, 0, 0, 0);
    m_leftLayout->setSpacing(0);
    m_leftLayout->setContentsMargins(0, 0, 0, 0);
    m_rightLayout->setSpacing(0);
    m_rightLayout->setContentsMargins(0, 0, 0, 0);
    m_topLayout->setSpacing(0);
    m_topLayout->setContentsMargins(0, 0, 0, 0);
    setAttribute(Qt::WA_TranslucentBackground, true);

    lbl = new QCustomLabel(label, parent);

    m_leftLayout->addWidget(lbl, 0, Qt::AlignTop);
    if (widget)
    {
        widget->setParent(this);
        m_rightLayout->addWidget(widget, 0, Qt::AlignTop);
        if (!fullWidth)
        {
            m_rightLayout->addSpacerItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Fixed));
        }
    }
    //if the embedded widget is a color widget imp, we need to something special later
    QColorWidgetImp* colorWidget = qobject_cast<QColorWidgetImp*> (widget);

    setLayout(m_topLayout);
    SetCopyMenuFlags(COPY_MENU_FLAGS(ENABLE_COPY | ENABLE_PASTE | ENABLE_RESET));    
    SetCopyCallback([=]()
        {
            parent->getAttributeView()->copyItem(parent, true);
        });
    SetPasteCallback([=]()
        {
            EBUS_EVENT(EditorLibraryUndoRequestsBus, Suspend, true);
            parent->getAttributeView()->pasteItem(parent);
            EBUS_EVENT(EditorLibraryUndoRequestsBus, Suspend, false);
            emit parent->SignalUndoPoint();
        });
    SetCheckPasteCallback([=]()
        {
            return parent->getAttributeView()->checkPasteItem(parent);
        });
    SetResetCallback([=]()
        {
            EBUS_EVENT(EditorLibraryUndoRequestsBus, Suspend, true);
            parent->ResetValueToDefault();
            EBUS_EVENT(EditorLibraryUndoRequestsBus, Suspend, false);
            emit parent->SignalUndoPoint();
        });
    SetCheckResetCallback([=]()
        {            
            if (colorWidget)
            {
                if (!colorWidget->isDefaultValue())
                    return true;
            }
            return !parent->isDefaultValue();
        });
    if (m_variable)
    {
        auto funcSet = functor(*this, &QColumnWidget::onVarChanged);
        m_variable->AddOnSetCallback(funcSet);
        //force the style udpate in onVarChanged
        m_canReset = !onCheckResetCallback();
        onVarChanged(m_variable);
    }

    if (colorWidget)
    {
        connect(colorWidget, &QColorWidgetImp::SingalSubVarChanged, this, &QColumnWidget::onVarChanged);
    }
}

QColumnWidget::~QColumnWidget()
{
    if (m_variable)
    {
        m_variable->RemoveOnSetCallback(functor(*this, &QColumnWidget::onVarChanged));
    }
}

void test()
{
}

void QColumnWidget::SetAsCollapsible(CAttributeItem* item)
{
    m_attributeItem = item;
    m_collapseButton = new QCheckBox(this);
    AzQtComponents::CheckBox::applyExpanderStyle(m_collapseButton);
    m_leftLayout->insertWidget(0, m_collapseButton);
    connect(m_collapseButton, &QCheckBox::clicked, this, &QColumnWidget::CollapseButton_clicked);
    m_widgetChildren = new QWidget(this);
    m_layoutChildren = new QVBoxLayout(m_widgetChildren);
    m_layoutChildren->setSpacing(0);
    m_layoutChildren->setSizeConstraint(QLayout::SetDefaultConstraint);
    m_layoutChildren->setContentsMargins(0, 6, 0, 0);

    m_topLayout->addWidget(m_widgetChildren, 0, Qt::AlignTop);

#ifdef EDITOR_QT_UI_EXPORTS
    if (m_attributeItem)
    {
        const QString path = m_attributeItem->getAttributePath(true);
        SetCollapsed(m_attributeItem->getAttributeView()->getValue(path, STATE_COLLAPSED) == STATE_COLLAPSED);
    }
#else
    setCollapsed(true);
#endif
    m_isCollapsible = true;
}


void QColumnWidget::onVarChanged(IVariable* var)
{
    bool canReset = onCheckResetCallback();
    if (m_canReset == canReset)
    {
        return;
    }
    m_canReset = canReset;
    setProperty("VariableAjusted", canReset);
    //Expensive style update
    RecursiveStyleUpdate(this);
}

void QColumnWidget::RecursiveStyleUpdate(QWidget* root)
{
    if (qobject_cast<CAttributeItem*>(root) || root == nullptr)
    {
        return;
    }
    root->style()->unpolish(root);
    root->style()->polish(root);
    root->update();

    for (int i = 0; i < root->children().count(); i++)
    {
        RecursiveStyleUpdate(qobject_cast<QWidget*>(root->children().at(i)));
    }
}

void QColumnWidget::paintEvent(QPaintEvent*)
{
    QStyleOption opt;
    opt.init(this);
    QPainter p(this);
    style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);
}

void QColumnWidget::SetCollapsible(bool val)
{
    lbl->SetCollapsible(val);
}

void QColumnWidget::SetCollapsed(bool state)
{
    QIcon icon;
    if (state)
    {
        m_widgetChildren->setHidden(true);
    }
    else
    {
        m_widgetChildren->setHidden(false);
    }

    // make sure the parent widget knows that we have changed the geometry.

    m_collapseButton->setProperty("collapsed", state);
    m_collapseButton->style()->unpolish(m_collapseButton);
    m_collapseButton->style()->polish(m_collapseButton);
    m_collapseButton->update();
}

void QColumnWidget::CollapseButton_clicked()
{
    // Toggle

    const bool collapsed = !m_widgetChildren->isHidden();

    SetCollapsed(collapsed);

#ifdef EDITOR_QT_UI_EXPORTS
    // Save state
    if (m_attributeItem)
    {
        const QString path = m_attributeItem->getAttributePath(true);
        m_attributeItem->getAttributeView()->setValue(path, collapsed ? STATE_COLLAPSED : STATE_UNCOLLAPSED);
    }
#endif
}
