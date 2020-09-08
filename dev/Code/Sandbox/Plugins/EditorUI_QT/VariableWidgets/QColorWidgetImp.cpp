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
#include "QColorWidgetImp.h"
#include "../UIUtils.h"
#include "Utils.h"
#include "AttributeItem.h"

#include <Util/Variable.h>
#include "AttributeView.h"
#include <VariableWidgets/ui_QColorWidget.h>
#include "../ContextMenu.h"
#include "IEditorParticleUtils.h"
#include "UIFactory.h"
#include <VariableWidgets/QColumnWidget.h>
#include <AzQtComponents/Components/Widgets/ColorPicker.h>
#include <AzQtComponents/Utilities/Conversions.h>

#include <ParticleParams.h>

#ifdef EDITOR_QT_UI_EXPORTS
#include <VariableWidgets/QColorWidgetImp.moc>
#endif

QColorWidgetImp::QColorWidgetImp(CAttributeItem* parent)
    : QColorWidget(parent)
    , CBaseVariableWidget(parent)
    , m_ignoreSetVar(false)
    , m_menu(new ContextMenu(this))
    , m_VarEnableVar1(nullptr)
    , m_Var1(nullptr)
    , m_AlphaVar(nullptr)
    , m_AlphaVar1(nullptr)
    , m_AlphaVarEnableVar1(nullptr)
    , m_colorPicker(nullptr)
{
    QString colorString;
    parent->getVar()->Get(colorString);

    auto relations = parent->GetRelations();
    TrySetupSecondColor(relations["randomColor"], relations["randomColorEnable"]);
    TrySetupAlpha(relations["alpha"]);
    TrySetupSecondAlpha(relations["randomAlpha"], relations["randomAlphaEnable"]);
    
    buildMenu();

    m_tooltip = new QToolTipWidget(this);

    //Code depends on alpha being initialized first.
    if (m_AlphaVar)
    {
        setVar(m_AlphaVar);
        m_defaultAlpha[0] = GetAlphaValue(0);
    }
    if (m_AlphaVar1)
    {
        setVar(m_AlphaVar1);
        m_defaultAlpha[1] = GetAlphaValue(1);
    }
    if (m_VarEnableVar1)
    {
        setVar(m_VarEnableVar1);
    }
    if (m_AlphaVarEnableVar1)
    {
        setVar(m_AlphaVarEnableVar1);
    }
    //Color last.
    if (m_Var1)
    {
        setVar(m_Var1);
        m_defaultColor[1] = GetColor(1);
    }
    m_defaultColor[0] = GetColor(0);

    m_color[0] = m_defaultColor[0];
    m_color[1] = m_defaultColor[1];

    //Special handling for "Reset to default" since this widget includes more than one variables
    connect(parent, &CAttributeItem::SignalResetToDefault, this, &QColorWidgetImp::ResetToDefault);
}

QColorWidgetImp::~QColorWidgetImp()
{
}

bool QColorWidgetImp::TrySetupSecondColor(QString targetName, QString targetEnableName)
{
    auto funcSet = functor(*this, &CBaseVariableWidget::onVarChanged);
    if (targetName.size() && targetEnableName.size())
    {
        m_VarEnableVar1 = m_parent->getAttributeView()->GetVarFromPath(targetEnableName);
        m_Var1 = m_parent->getAttributeView()->GetVarFromPath(targetName);
        if (m_VarEnableVar1 == nullptr || m_Var1 == nullptr)
        {
            m_VarEnableVar1 = m_Var1 = nullptr;
            return false;
        }

        m_Var1->AddOnSetCallback(funcSet);
        addOnDestructionEvent([funcSet, this](CBaseVariableWidget* self) { m_Var1->RemoveOnSetCallback(funcSet); });

        m_VarEnableVar1->AddOnSetCallback(funcSet);
        addOnDestructionEvent([funcSet, this](CBaseVariableWidget* self) { m_VarEnableVar1->RemoveOnSetCallback(funcSet); });

        if (QString(m_VarEnableVar1->GetDisplayValue()) == "true")
        {
            ui->colorButton1->show();
        }
        return true;
    }
    return false;
}

bool QColorWidgetImp::TrySetupAlpha(QString targetName)
{
    auto funcSet = functor(*this, &CBaseVariableWidget::onVarChanged);
    if (targetName.size())
    {
        m_AlphaVar = m_parent->getAttributeView()->GetVarFromPath(targetName);
        if (m_AlphaVar == nullptr)
        {
            return false;
        }

        m_AlphaVar->AddOnSetCallback(funcSet);
        addOnDestructionEvent([funcSet, this](CBaseVariableWidget* self) { m_AlphaVar->RemoveOnSetCallback(funcSet); });

        ui->multiplierBox->show();

        return true;
    }
    return false;
}

bool QColorWidgetImp::TrySetupSecondAlpha(QString targetName, QString targetEnableName)
{
    auto funcSet = functor(*this, &CBaseVariableWidget::onVarChanged);
    if (targetName.size() && targetEnableName.size())
    {
        m_AlphaVarEnableVar1 = m_parent->getAttributeView()->GetVarFromPath(targetEnableName);
        m_AlphaVar1 = m_parent->getAttributeView()->GetVarFromPath(targetName);
        if (m_AlphaVarEnableVar1 == nullptr || m_AlphaVar1 == nullptr)
        {
            m_AlphaVarEnableVar1 = m_AlphaVar1 = nullptr;
            return false;
        }

        m_AlphaVar1->AddOnSetCallback(funcSet);
        addOnDestructionEvent([funcSet, this](CBaseVariableWidget* self) { m_AlphaVar1->RemoveOnSetCallback(funcSet); });

        m_AlphaVarEnableVar1->AddOnSetCallback(funcSet);
        addOnDestructionEvent([funcSet, this](CBaseVariableWidget* self) { m_AlphaVarEnableVar1->RemoveOnSetCallback(funcSet); });

        if (QString(m_AlphaVarEnableVar1->GetDisplayValue()) == "true")
        {
            ui->multiplierBox1->show();
        }
        return true;
    }
    return false;
}

void QColorWidgetImp::setColor(const QColor& color, int index)
{
    SelfCallFence(m_ignoreSetVar);
    // Pass override function to internal local function
    setInternalColor(color, index, true);
    emit SingalSubVarChanged(index == 0 ? m_var : m_Var1);
}

void QColorWidgetImp::setAlpha(float alpha, int index)
{
    SelfCallFence(m_ignoreSetVar);
    // Pass override function to internal local function
    setInternalAlpha(alpha, index, true);
    emit SingalSubVarChanged(index == 0?m_AlphaVar:m_AlphaVar1);
}


void QColorWidgetImp::setInternalColor(const QColor& color, int index, bool enableUndo /*=true*/)
{
    QColorWidget::setColor(color, index);
    QString value = UIUtils::ColorToString(color).toStdString().c_str();

    if (index == 0)
    {
        m_var->Set(value);
    }
    else if (index == 1)
    {
        m_Var1->Set(value);
    }
}

void QColorWidgetImp::setInternalAlpha(float alpha, int index, bool enableUndo /*=true*/)
{
    QColorWidget::setAlpha(alpha, index);
    //update color widget display
    QColorWidget::setColor(m_color[index], index);


    if (index == 0)
    {
        m_AlphaVar->Set(alpha);
    }
    else if (index == 1)
    {
        m_AlphaVar1->Set(alpha);
    }

}

float QColorWidgetImp::GetAlphaValue(int index)
{
    float alpha = 1;
    if (index == 0 && m_AlphaVar)
    {
        m_AlphaVar->Get(alpha);
    }
    
    if (index == 1 && m_AlphaVar1)
    {
        m_AlphaVar1->Get(alpha);
    }
    return alpha;
}

QColor QColorWidgetImp::GetColor(int index)
{
    QColor color;
    if (index == 0 && m_var)
    {
        QString colorValue;
        m_var->Get(colorValue);
        color = UIUtils::StringToColor(QString(colorValue));
    }

    if (index == 1 && m_Var1)
    {
        QString colorValue;
        m_Var1->Get(colorValue);
        color = UIUtils::StringToColor(QString(colorValue));
    }
    return color;
}

void QColorWidgetImp::onSelectColor(int index)
{
    float originAlpha = GetAlphaValue(index);

    //disable alpha editing when color picker dialog is showing
    ui->multiplierBox->setDisabled(true);
    ui->multiplierBox1->setDisabled(true);

    AzQtComponents::ColorPicker* dlg = m_colorPicker;
    if (!dlg) {
        dlg = new AzQtComponents::ColorPicker(AzQtComponents::ColorPicker::Configuration::RGBA);
        dlg->setWindowTitle("Color Picker");
        m_colorPicker = dlg;
    }
    else {
        disconnect(dlg, &AzQtComponents::ColorPicker::currentColorChanged, nullptr, nullptr);
        disconnect(dlg, &AzQtComponents::ColorPicker::finished, nullptr, nullptr);
    }

    // Connect the color dialog and Color to undo
    connect(dlg, &AzQtComponents::ColorPicker::currentColorChanged, this, [=]()
        {
            const QColor currentColor = AzQtComponents::toQColor(dlg->currentColor());
            setInternalColor(currentColor, index, false);
            //Only update alpha var if the alpha selected is less than 1 or the alpha input is less than 1
            float curAlpha = GetAlphaValue(index);
            if (currentColor.alphaF() < 1 || curAlpha <= 1)
            {
                setInternalAlpha(currentColor.alphaF(), index, false);
            }
        });
    connect(dlg, &AzQtComponents::ColorPicker::finished, this, [=](bool accept)
        {
            QColor oldValue = m_OriginColor;
            QColor currentValue = m_color[0];
            float curAlpha = GetAlphaValue(index);
            if (index == 1)
            {
                oldValue = m_OriginColor1;
                currentValue = m_color[1];
            }
            //Reset original color
            setInternalColor(oldValue, index, false);
            setInternalAlpha(originAlpha, index, false);
            if (accept) // if accept the new color
            {
                //Set value to new value and enable record undo
                setInternalColor(currentValue, index, true);
                if (currentValue.alphaF() < 1 || curAlpha <= 1)
                {
                    setInternalAlpha(currentValue.alphaF(), index, true);
                }
                emit m_parent->SignalUndoPoint();
            }

            dlg->deleteLater();
            m_colorPicker = nullptr;

            //enable the alpha editing
            ui->multiplierBox->setDisabled(false);
            ui->multiplierBox1->setDisabled(false);
        });

    dlg->setCurrentColor(AzQtComponents::fromQColor(m_color[index]));
    dlg->setSelectedColor(AzQtComponents::fromQColor(m_color[index]));

    //Store color for undoing
    m_OriginColor = m_color[0];
    m_OriginColor1 = m_color[1];

    // Use show() instead of exec() for the dialog. exec() will block other processes.
    dlg->show();
}

void QColorWidgetImp::setVar(IVariable* var)
{
    if (m_ignoreSetVar)
    {
        return;
    }

    if (var == m_VarEnableVar1)
    {
        bool visible = QString(m_VarEnableVar1->GetDisplayValue()) == "true";
        ui->colorButton1->setVisible(visible);
        buildMenu();
    }
    else if (var == m_AlphaVarEnableVar1)
    {
        bool visible = QString(m_AlphaVarEnableVar1->GetDisplayValue()) == "true";
        ui->multiplierBox1->setVisible(visible);
        buildMenu();
    }
    else if (var == m_AlphaVar)
    {
        float alpha;
        var->Get(alpha);
        SetMultiplierBox(alpha, 0);
        m_defaultAlpha[0] = alpha;
    }
    else if (var == m_AlphaVar1)
    {
        float alpha;
        var->Get(alpha);
        SetMultiplierBox(alpha, 1);
        m_defaultAlpha[1] = alpha;
    }
    else if (var == m_var)
    {
        QString colorValue;
        var->Get(colorValue);
        QColor color = UIUtils::StringToColor(QString(colorValue));
        QColorWidget::setColor(color, 0);
    }
    else if (var == m_Var1)
    {
        QString colorValue;
        var->Get(colorValue);
        QColor color = UIUtils::StringToColor(QString(colorValue));
        QColorWidget::setColor(color, 1);
    }

    emit SingalSubVarChanged(var);
}

void QColorWidgetImp::onVarChanged(IVariable* var)
{
    setVar(var);
}

void QColorWidgetImp::mousePressEvent(QMouseEvent* e)
{
    if (e->button() == Qt::RightButton)
    {
        m_menu->exec(e->globalPos());
    }
}

bool QColorWidgetImp::event(QEvent* e)
{
    IVariable* var = m_parent->getVar();
    QToolTipWidget::ArrowDirection arrowDir = QToolTipWidget::ArrowDirection::ARROW_RIGHT;
    switch (e->type())
    {
    case QEvent::ToolTip:
    {
        //determine which color is being hit
        QHelpEvent* event = (QHelpEvent*)e;
        QPoint ep = event->globalPos();
        QRect colorRect = QRect(mapToGlobal(m_background0->pos()), m_background0->size());
        QRect altColorRect = QRect(mapToGlobal(m_background1->pos()), m_background1->size());
        QRect alphaRect = GetAlphaOneTooltipRect();
        QRect altAlphaRect = GetAlphaTwoTooltipRect();
        //Color 1


        if (IsPointInRect(ep, colorRect))
        {
            QString colorString = CreateToolTipString(m_color[0]);
            CreateTooltipForRect(colorRect, "Color", colorString);
        }
        else if (IsPointInRect(ep, alphaRect))
        {
            CreateTooltipForRect(alphaRect, "Alpha");
        }
        else if (IsPointInRect(ep, altColorRect))
        {
            QString colorString = CreateToolTipString(m_color[1]);
            CreateTooltipForRect(altColorRect, "Color", colorString);
        }
        else if (IsPointInRect(ep, altAlphaRect))
        {
            CreateTooltipForRect(altAlphaRect, "Alpha");
        }
        else
        {
            m_tooltip->hide();
        }
        e->accept();
        break;
    }
    case QEvent::Leave:
    {
        if (m_tooltip->isVisible())
        {
            m_tooltip->hide();
        }
        QCoreApplication::instance()->removeEventFilter(this); //Release filter (Only if we have a tooltip?)

        m_parent->getAttributeView()->setToolTip("");
        break;
    }
    default:
        break;
    }
    return QColorWidget::event(e);
}

void QColorWidgetImp::buildMenu()
{
    QAction* action;
    m_menu->clear();
    if (m_VarEnableVar1 != nullptr)
    {
        bool randomActive = false;
        m_VarEnableVar1->Get(randomActive);
        if (randomActive)
        {
            action = m_menu->addAction("Single color");//launch random dlg
        }
        else
        {
            action = m_menu->addAction("Random between two colors");//launch random dlg
        }
        connect(action, &QAction::triggered, this, [&]()
            {
                m_VarEnableVar1->Set(!(QString(m_VarEnableVar1->GetDisplayValue()) == "true"));
                if (m_AlphaVarEnableVar1 != nullptr)
                {
                    m_AlphaVarEnableVar1->Set((QString(m_VarEnableVar1->GetDisplayValue()) == "true"));
                }
                buildMenu();
            });
    }
    action = m_menu->addAction("Reset");//launch export dlg
    connect(action, &QAction::triggered, this, [&]()
        {
            SetMultiplierBox(m_defaultAlpha[0], 0);
            SetMultiplierBox(m_defaultAlpha[1], 1);
            setColor(m_defaultColor[0], 0);
            setColor(m_defaultColor[1], 1);
        });
}

void QColorWidgetImp::CreateTooltipForRect(QRect widgetRect, const QString& option /*= ""*/, const QString& extraDataForTooltip /*= ""*/)
{
    GetIEditor()->GetParticleUtils()->ToolTip_BuildFromConfig(m_tooltip, m_parent->getAttributeView()->GetVariablePath(m_var), option, extraDataForTooltip);
    m_tooltip->Display(widgetRect, QToolTipWidget::ArrowDirection::ARROW_RIGHT);
}

QString QColorWidgetImp::CreateToolTipString(const QColor& color)
{
    return QString("R: %1\tG: %2\tB: %3\tA: %4").arg(color.red()).arg(color.green()).arg(color.blue()).arg(int(color.alphaF() * 100.0f));
}

void QColorWidgetImp::ResetToDefault()
{
    if (m_Var1)
    {
        m_Var1->ResetToDefault();
    }
    if (m_AlphaVar)
    {
        m_AlphaVar->ResetToDefault();
    }
    if (m_AlphaVar1)
    {
        m_AlphaVar1->ResetToDefault();
    }
    if (m_VarEnableVar1)
    {
        m_VarEnableVar1->ResetToDefault();
    }
    if (m_AlphaVarEnableVar1)
    {
        m_AlphaVarEnableVar1->ResetToDefault();
    }
}

bool QColorWidgetImp::isDefaultValue()
{
    if (m_Var1)
    {
        if (!m_Var1->HasDefaultValue())
            return false;
    }
    if (m_AlphaVar)
    {
        if (!m_AlphaVar->HasDefaultValue())
            return false;
    }
    if (m_AlphaVar1)
    {
        if (!m_AlphaVar1->HasDefaultValue())
            return false;
    }
    if (m_VarEnableVar1)
    {
        if (!m_VarEnableVar1->HasDefaultValue())
            return false;
    }
    if (m_AlphaVarEnableVar1)
    {
        if (!m_AlphaVarEnableVar1->HasDefaultValue())
            return false;
    }
    return m_var->HasDefaultValue();
}
