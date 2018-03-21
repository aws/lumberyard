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
#include <QtGlobal>

#include <AzQtComponents/Components/Style.h>
#include <AzQtComponents/Components/Widgets/PushButton.h>
#include <AzQtComponents/Components/Widgets/CheckBox.h>
#include <AzQtComponents/Components/Widgets/RadioButton.h>
#include <AzQtComponents/Components/Widgets/ProgressBar.h>
#include <AzQtComponents/Components/Widgets/Slider.h>
#include <AzQtComponents/Components/TitleBarOverdrawHandler.h>

#include <QObject>
#include <QApplication>
#include <QPushButton>
#include <QToolButton>
#include <QCheckBox>
#include <QRadioButton>
#include <QFile>
#include <QSettings>
#include <QFileSystemWatcher>
#include <QStyleOptionToolButton>
#include <QPainter>
#include <QProgressBar>



namespace AzQtComponents
{

// Private data structure
struct Style::Data
{
    QPalette palette;
    PushButton::Config pushButtonConfig;
    RadioButton::Config radioButtonConfig;
    CheckBox::Config checkBoxConfig;
    ProgressBar::Config progressBarConfig;
    Slider::Config sliderConfig;
    QFileSystemWatcher watcher;
};

// Local template function to load config data from .ini files
template <typename ConfigType, typename WidgetType>
void loadConfig(Style* style, QFileSystemWatcher* watcher, ConfigType* config, const QString& path)
{
    QString fullPath = QStringLiteral("AzQtComponentWidgets:%1").arg(path);
    if (QFile::exists(fullPath))
    {
        // add to the file watcher
        watcher->addPath(fullPath);

        // connect the relead slot()
        QObject::connect(watcher, &QFileSystemWatcher::fileChanged, style, [style, fullPath, config](const QString& changedPath) {
            if (changedPath == fullPath)
            {
                QSettings settings(fullPath, QSettings::IniFormat);
                *config = WidgetType::loadConfig(settings);

                Q_EMIT style->settingsReloaded();
            }
        });

        QSettings settings(fullPath, QSettings::IniFormat);
        *config = WidgetType::loadConfig(settings);
    }
    else
    {
        *config = WidgetType::defaultConfig();
    }
}

Style::Style(QStyle* style)
    : QProxyStyle(style)
    , m_data(new Style::Data)
{
    // set up the push button settings watcher
    loadConfig<PushButton::Config, PushButton>(this, &m_data->watcher, &m_data->pushButtonConfig, "PushButtonConfig.ini");
    loadConfig<RadioButton::Config, RadioButton>(this, &m_data->watcher, &m_data->radioButtonConfig, "RadioButtonConfig.ini");
    loadConfig<CheckBox::Config, CheckBox>(this, &m_data->watcher, &m_data->checkBoxConfig, "CheckBoxConfig.ini");
    loadConfig<ProgressBar::Config, ProgressBar>(this, &m_data->watcher, &m_data->progressBarConfig, "ProgressBarConfig.ini");
    loadConfig<Slider::Config, Slider>(this, &m_data->watcher, &m_data->sliderConfig, "SliderConfig.ini");
}

Style::~Style()
{
}

QSize Style::sizeFromContents(QStyle::ContentsType type, const QStyleOption* option, const QSize& size, const QWidget* widget) const
{
     switch (type)
     {
         case QStyle::CT_PushButton:
             if (qobject_cast<const QPushButton*>(widget) || qobject_cast<const QToolButton*>(widget))
             {
                 return PushButton::sizeFromContents(this, type, option, size, widget, m_data->pushButtonConfig);
             }
             break;

         case QStyle::CT_CheckBox:
             if (qobject_cast<const QCheckBox*>(widget))
             {
                 return CheckBox::sizeFromContents(this, type, option, size, widget, m_data->checkBoxConfig);
             }
             break;

         case QStyle::CT_RadioButton:
             if (qobject_cast<const QRadioButton*>(widget))
             {
                 return RadioButton::sizeFromContents(this, type, option, size, widget, m_data->radioButtonConfig);
             }
             break;

         case QStyle::CT_ProgressBar:
             if (qobject_cast<const QProgressBar*>(widget))
             {
                 return ProgressBar::sizeFromContents(this, type, option, size, widget, m_data->progressBarConfig);
             }
             break;
     }

    return QProxyStyle::sizeFromContents(type, option, size, widget);
}

void Style::drawControl(QStyle::ControlElement element, const QStyleOption* option, QPainter* painter, const QWidget* widget) const
{
    switch (element)
    {
        case CE_PushButtonBevel:
        {
            if (qobject_cast<const QPushButton*>(widget))
            {
                if (PushButton::drawPushButtonBevel(this, option, painter, widget, m_data->pushButtonConfig))
                {
                    return;
                }
            }
        }
        break;

        case CE_CheckBox:
        {
            if (qobject_cast<const QCheckBox*>(widget))
            {
                if (CheckBox::drawCheckBox(this, option, painter, widget, m_data->checkBoxConfig))
                {
                    return;
                }
            }
        }
        break;

        case CE_CheckBoxLabel:
        {
            if (qobject_cast<const QCheckBox*>(widget))
            {
                if (CheckBox::drawCheckBoxLabel(this, option, painter, widget, m_data->checkBoxConfig))
                {
                    return;
                }
            }
        }
        break;

        case CE_RadioButton:
        {
            if (qobject_cast<const QRadioButton*>(widget))
            {
                if (RadioButton::drawRadioButton(this, option, painter, widget, m_data->radioButtonConfig))
                {
                    return;
                }
            }
        }
        break;

        case CE_RadioButtonLabel:
        {
            if (qobject_cast<const QRadioButton*>(widget))
            {
                if (RadioButton::drawRadioButtonLabel(this, option, painter, widget, m_data->radioButtonConfig))
                {
                    return;
                }
            }
        }
        break;
    }

    return QProxyStyle::drawControl(element, option, painter, widget);
}

void Style::drawPrimitive(QStyle::PrimitiveElement element, const QStyleOption* option, QPainter* painter, const QWidget* widget) const
{
    switch (element)
    {
        case PE_FrameFocusRect:
        {
            if (qobject_cast<const QPushButton*>(widget) || qobject_cast<const QToolButton*>(widget))
            {
                if (PushButton::drawPushButtonFocusRect(this, option, painter, widget, m_data->pushButtonConfig))
                {
                    return;
                }
            }
        }
        break;

        case PE_PanelButtonTool:
        {
            if (PushButton::drawPushButtonBevel(this, option, painter, widget, m_data->pushButtonConfig))
            {
                return;
            }
        }
        break;

        case PE_IndicatorArrowDown:
        {
            if (PushButton::drawIndicatorArrow(this, option, painter, widget, m_data->pushButtonConfig))
            {
                return;
            }
        }
        break;
    }

    return QProxyStyle::drawPrimitive(element, option, painter, widget);
}

void Style::drawComplexControl(QStyle::ComplexControl element, const QStyleOptionComplex* option, QPainter* painter, const QWidget* widget) const
{
    switch (element)
    {
        case CC_ToolButton:
            if (PushButton::drawToolButton(this, option, painter, widget, m_data->pushButtonConfig))
            {
                return;
            }
        break;

        case CC_Slider:
            if (Slider::drawSlider(this, option, painter, widget, m_data->sliderConfig))
            {
                return;
            }
            break;
    }

    return QProxyStyle::drawComplexControl(element, option, painter, widget);
}

int Style::pixelMetric(QStyle::PixelMetric metric, const QStyleOption* option,
    const QWidget* widget) const
{
    switch (metric)
    {
    case QStyle::PM_ButtonMargin:
        return PushButton::buttonMargin(this, option, widget, m_data->pushButtonConfig);

    case QStyle::PM_LayoutLeftMargin:
    case QStyle::PM_LayoutTopMargin:
    case QStyle::PM_LayoutRightMargin:
    case QStyle::PM_LayoutBottomMargin:
        return 5;

    case QStyle::PM_LayoutHorizontalSpacing:
    case QStyle::PM_LayoutVerticalSpacing:
        return 3;

    case QStyle::PM_HeaderDefaultSectionSizeVertical:
        return 24;

    case QStyle::PM_DefaultFrameWidth:
    {
        if (auto button = qobject_cast<const QToolButton*>(widget))
        {
            if (button->popupMode() == QToolButton::MenuButtonPopup)
            {
                return 0;
            }
        }
        
        break;
    }

    case QStyle::PM_ButtonIconSize:
        return 24;
        break;

    case QStyle::PM_ToolBarFrameWidth:
        // There's a bug in .css, changing right padding also changes top-padding
        return 0;
        break;

    case QStyle::PM_ToolBarItemSpacing:
        return 5;
        break;

    case QStyle::PM_DockWidgetSeparatorExtent:
        return 4;
        break;

    case QStyle::PM_ToolBarIconSize:
        return 16;
        break;

    default:
        break;
    }

    return QProxyStyle::pixelMetric(metric, option, widget);
}

void Style::polish(QWidget* widget)
{
    TitleBarOverdrawHandler::getInstance()->polish(widget);

    PushButton::polish(this, widget, m_data->pushButtonConfig);
    CheckBox::polish(this, widget, m_data->checkBoxConfig);
    RadioButton::polish(this, widget, m_data->radioButtonConfig);
    Slider::polish(this, widget, m_data->sliderConfig);

    QProxyStyle::polish(widget);
}

QPalette Style::standardPalette() const
{
    return m_data->palette;
}

bool Style::hasClass(const QWidget* button, const QString& className) const
{
    QVariant buttonClassVariant = button->property("class");
    if (buttonClassVariant.isNull())
    {
        return false;
    }

    QString classText = buttonClassVariant.toString();
    QStringList classList = classText.split(QRegularExpression("\\s+"));
    return classList.contains(className, Qt::CaseInsensitive);
}

void Style::addClass(QWidget* button, const QString& className)
{
    QVariant buttonClassVariant = button->property("class");
    if (buttonClassVariant.isNull())
    {
        button->setProperty("class", className);
    }
    else
    {
        QString classText = buttonClassVariant.toString();
        classText += QStringLiteral(" %1").arg(className);
        button->setProperty("class", classText);
    }

    button->style()->unpolish(button);
    button->style()->polish(button);
}

void Style::polish(QApplication* application)
{
    Q_UNUSED(application);

    const QString g_linkColorValue = QStringLiteral("#4285F4");
    m_data->palette = application->palette();
    m_data->palette.setColor(QPalette::Link, QColor(g_linkColorValue));

    application->setPalette(m_data->palette);

    QProxyStyle::polish(application);
}


#include <Components/Style.moc>
} // namespace AzQtComponents
