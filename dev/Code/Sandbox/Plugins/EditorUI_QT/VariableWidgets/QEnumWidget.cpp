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
#include "QEnumWidget.h"
#include "BaseVariableWidget.h"
#include "Utils.h"
#include "AttributeItem.h"

#include <Util/Variable.h>
#include <QTimer>
#include "AttributeView.h"
#include "IEditorParticleUtils.h"

QEnumWidget::QEnumWidget(CAttributeItem* parent)
    : CBaseVariableWidget(parent)
    , QComboBox(parent)
    , m_isShown(false)
    , m_lineEditPressed(false)
{
    setFocusPolicy(Qt::StrongFocus);
    installEventFilter(parent);

    QStringList list;

    IVarEnumList* enumList = m_var->GetEnumList();
    if (enumList)
    {
        for (uint i = 0; !enumList->GetItemName(i).isNull(); i++)
        {
            list.push_back(enumList->GetItemName(i));
        }
    }
    else
    {
        list.push_back("INVALID_ENUM_LIST");
    }

    addItems(list);
        
    setEditable(true);

    installEventFilter(this);

    //This sub control (especially the alignment) is un-stylable via QSS global stylesheets, so we have to style it through lineEdit
    lineEdit()->setReadOnly(true); 
    lineEdit()->setStyleSheet("*{background: transparent; border: 0px none red;}");
    lineEdit()->setAlignment(Qt::AlignRight);
    lineEdit()->installEventFilter(this);

    for (int i = 0; i < count(); i++)
    {
        setItemData(i, Qt::AlignRight, Qt::TextAlignmentRole);
    }

    QString selection;
    m_var->Get(selection);
    int selectedIdx = findText(QString(selection));
    if (selectedIdx != -1)
    {
        setCurrentIndex(selectedIdx);
    }

    // connect to particle variable
    connect(this, static_cast<void (QComboBox::*)(const QString&)>(&QComboBox::activated), this, &QEnumWidget::onUserChange);

    m_tooltip = new QToolTipWidget(this);
}

QEnumWidget::~QEnumWidget()
{
}

void QEnumWidget::onVarChanged(IVariable* var)
{
    SelfCallFence(m_ignoreSetCallback);

    QString selection;
    var->Get(selection);
    int selectedIdx = findText(QString(selection));
    if (selectedIdx != -1)
    {
        QSignalBlocker blocker(this);
        setCurrentIndex(selectedIdx);

        //change style sheet for line edit when selection changed
        if (!qobject_cast<QStandardItemModel *>(model())->item(selectedIdx)->isEnabled())
        {
            lineEdit()->setStyleSheet("QLineEdit {background: #ff585858; border: 0px none red; color: #ff808080;}");
        }
        else
        {
            lineEdit()->setStyleSheet("*{background: transparent; border: 0px none red;}");
        }
    }

    m_parent->UILogicUpdateCallback();
}

bool QEnumWidget::event(QEvent* e)
{
    switch (e->type())
    {
    case QEvent::ToolTip:
    {
        QHelpEvent* event = (QHelpEvent*)e;

        GetIEditor()->GetParticleUtils()->ToolTip_BuildFromConfig(m_tooltip, m_parent->getAttributeView()->GetVariablePath(m_var), "ComboBox", QString(m_var->GetDisplayValue()));

        m_tooltip->TryDisplay(event->globalPos(), this, QToolTipWidget::ArrowDirection::ARROW_RIGHT);

        return true;

        break;
    }
    case QEvent::Leave:
    {
        if (m_tooltip)
        {
            m_tooltip->close();
        }
        QCoreApplication::instance()->removeEventFilter(this); //Release filter (Only if we have a tooltip?)
        break;
    }
    default:
        break;
    }
    return QComboBox::event(e);
}

bool QEnumWidget::eventFilter(QObject* obj, QEvent* event)
{
    if (isEnabled())
    {        
        //handle the mouse press/release if the mouse is click on the lineEdit
        if (event->type() == QEvent::MouseButtonRelease || event->type() == QEvent::MouseButtonPress)
        {
            bool isClickLineEdit = false;
            QMouseEvent* mouseEvt = static_cast<QMouseEvent*>(event);
            QRect rect = lineEdit()->geometry();
            QPoint scnPos = QPoint(static_cast<int>(mouseEvt->screenPos().x()), static_cast<int>(mouseEvt->screenPos().y()));
            QPoint mousePos = lineEdit()->mapFromGlobal(scnPos);

            if (mouseEvt->button() == Qt::LeftButton && rect.contains(mousePos))
            {
                isClickLineEdit = true;
            }

            if (mouseEvt->button() == Qt::LeftButton && event->type() == QEvent::MouseButtonRelease)
            {
                m_lineEditPressed = false;
            }

            if (isClickLineEdit)
            {
                //Note: when the popup menu is not showing, the lineEdit object recieves the mouse button press event. Otherwise the ComboBoxPrivateContainer
                // will recieve the press event instead of lineEdit. 
                if (event->type() == QEvent::MouseButtonPress)
                {
                    if (!m_lineEditPressed)
                    {
                        //Use m_lineEditPressed to make sure the press event only be handled once. 
                        //This is because if we call hidePopup for ComboBoxPrivateContainer's mouse press event,
                        //the LineEdit will receive the mouse press event in the same frame which always open popup menu after close it.
                        m_lineEditPressed = true;

                        if (!IsPopupShowing())
                        {
                            showPopup();
                        }
                        else
                        {   //hide popup when popup is showing
                            hidePopup();
                        }
                    }
                }

                //Disable press and release handle for children objects.
                return true;
            }  
        }
        else if (event->type() == QEvent::Wheel)
        {
            // We used to clear the focus in onUserChange(), but the wheel behavior was still is odd (you can only 
            // scroll one space up/down and then focus is cleared). So we just prevent all wheel events.
            return true;
        }
    }

    return QComboBox::eventFilter(obj, event);
}

bool QEnumWidget::IsPopupShowing()
{   
    //Note: this function won't return true in the same frame after called showPopup. 
    QStyleOptionComboBox opt;
    initStyleOption(&opt);
    return (opt.state & QStyle::State_On);
}

void QEnumWidget::onUserChange(const QString& text)
{
    //if the option wasn't changed, return
    QString str = m_var->GetDisplayValue();
    if (text.compare(str, Qt::CaseInsensitive) == 0)
    {
        //nothing needs to happen
    }
    //only apply change if the the change is accepted
    else if (!m_parent->getAttributeView()->ValidateUserChangeEnum(m_var, text))
    {
        //reset selection from m_var
        QString selection;
        m_var->Get(selection);
        int selectedIdx = findText(QString(selection));
        if (selectedIdx != -1)
        {
            QSignalBlocker blocker(this);
            setCurrentIndex(selectedIdx);
        }
    }
    else if (!m_parent->getAttributeView()->HandleUserChangedEnum(m_var, text))
    {
        m_var->SetDisplayValue(text.toUtf8().data()); 
        emit m_parent->SignalEnumChanged();
        emit m_parent->SignalUndoPoint();
    }
}

void QEnumWidget::showPopup()
{
    QComboBox::showPopup();    

    //we need add event filter to QComboBoxPrivateContainer so we can block the mouse left button release on lineEdit when popup menu is showing
    //the instance only was created after the popup menu first shown. Since there is no way to find out which QObject is QComboBoxPrivateContainer, 
    //we add event filter to all direct children (actually four) here. 
    if (!m_isShown)
    {
        m_isShown = true;

        QObjectList objs = children();
        for (int i = 0; i< objs.size(); i++)
        {
            QObject* obj = objs[i];
            obj->installEventFilter(this);
        }
    }
}

void QEnumWidget::SetItemEnable(int itemIdx, bool enable)
{
    qobject_cast<QStandardItemModel *>(model())->item(itemIdx)->setEnabled(enable);
}

