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

#include <AzQtComponents/Components/Widgets/ComboBox.h>
#include <AzQtComponents/Components/Style.h>
#include <AzQtComponents/Components/ConfigHelpers.h>

#include <QComboBox>
#include <QStyledItemDelegate>
#include <QAbstractItemView>
#include <QGraphicsDropShadowEffect>
#include <QLineEdit>
#include <QSettings>
#include <QDebug>

namespace AzQtComponents
{
    class ComboBoxItemDelegate : public QStyledItemDelegate
    {
    public:
        explicit ComboBoxItemDelegate(QComboBox* combo)
            : QStyledItemDelegate(combo)
            , m_combo(combo)
        {
        }

    protected:
        void initStyleOption(QStyleOptionViewItem* option, const QModelIndex& index) const override
        {
            QStyledItemDelegate::initStyleOption(option, index);

            // The default usage of combobox only allow one selection,
            // then we have to tweak it a bit so it show a check mark in this case.
            if (option && m_combo && m_combo->view()->selectionMode() == QAbstractItemView::SingleSelection)
            {
                option->features |= QStyleOptionViewItem::HasCheckIndicator;
                option->checkState = m_combo->currentIndex() == index.row() ? Qt::Checked : Qt::Unchecked;
            }
        }

    private:
        QPointer<QComboBox> m_combo;
    };

    ComboBox::Config ComboBox::loadConfig(QSettings& settings)
    {
        Config config = defaultConfig();

        ConfigHelpers::read<int>(settings, QStringLiteral("BoxShadowXOffset"), config.boxShadowXOffset);
        ConfigHelpers::read<int>(settings, QStringLiteral("BoxShadowYOffset"), config.boxShadowYOffset);
        ConfigHelpers::read<int>(settings, QStringLiteral("BoxShadowBlurRadius"), config.boxShadowBlurRadius);
        ConfigHelpers::read<QColor>(settings, QStringLiteral("BoxShadowColor"), config.boxShadowColor);
        ConfigHelpers::read<QColor>(settings, QStringLiteral("PlaceHolderTextColor"), config.placeHolderTextColor);
        ConfigHelpers::read<QColor>(settings, QStringLiteral("FramelessTextColor"), config.framelessTextColor);

        return config;
    }

    ComboBox::Config ComboBox::defaultConfig()
    {
        Config config;

        // Style guide require x: 0, y: 1, blur radius: 2, spread radius: 1
        // But QGraphicsDropShadowEffect does not support spread radius, so let
        // customize it a bit differently so it mostly looks like what is asked.
        config.boxShadowXOffset = 0;
        config.boxShadowYOffset = 1;
        config.boxShadowBlurRadius = 4;
        config.boxShadowColor = QColor("#7F000000");
        config.placeHolderTextColor = QColor("#999999");
        config.framelessTextColor = QColor(Qt::white);

        return config;
    }

    bool ComboBox::polish(Style* style, QWidget* widget, const ComboBox::Config& config)
    {
        auto comboBox = qobject_cast<QComboBox*>(widget);

        if (comboBox)
        {
            // The default menu combobox delegate is not stylishable via qss
            comboBox->setItemDelegate(new ComboBoxItemDelegate(comboBox));

            if (QLineEdit* le = comboBox->lineEdit())
            {
                QPalette pal = le->palette();
                pal.setColor(QPalette::PlaceholderText, config.placeHolderTextColor);
                le->setPalette(pal);
            }
            // Allow the popup to have round radius as it's a top level window.
            QWidget* frame = comboBox->view()->parentWidget();
            frame->setWindowFlags(frame->windowFlags() | Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint);
            frame->setAttribute(Qt::WA_TranslucentBackground, true);

            // Qss does not support css box-shadow, let use QGraphicsDropShadowEffect
            // Please note that it does not support spread-radius.
            QGraphicsDropShadowEffect* effect = new QGraphicsDropShadowEffect(frame);
            effect->setXOffset(config.boxShadowXOffset);
            effect->setYOffset(config.boxShadowYOffset);
            effect->setBlurRadius(config.boxShadowBlurRadius);
            effect->setColor(config.boxShadowColor);

            QMargins margins;

            if (effect->xOffset() == 0)
            {
                margins.setLeft(effect->blurRadius());
                margins.setRight(effect->blurRadius());
            }
            else if (effect->xOffset() > 0)
            {
                margins.setLeft(0);
                margins.setRight(effect->xOffset() + effect->blurRadius());
            }
            else if (effect->xOffset() < 0)
            {
                margins.setLeft(-effect->xOffset() + effect->blurRadius());
                margins.setRight(0);
            }

            if (effect->yOffset() == 0)
            {
                margins.setTop(effect->blurRadius());
                margins.setBottom(effect->blurRadius());
            }
            else if (effect->yOffset() > 0)
            {
                margins.setTop(0);
                margins.setBottom(effect->yOffset() + effect->blurRadius());
            }
            else if (effect->yOffset() < 0)
            {
                margins.setTop(-effect->yOffset() + effect->blurRadius());
                margins.setBottom(0);
            }

            frame->setGraphicsEffect(effect);
            frame->setContentsMargins(margins);

            style->repolishOnSettingsChange(comboBox);
        }

        return comboBox;
    }

    bool ComboBox::unpolish(Style* style, QWidget* widget, const ComboBox::Config& config)
    {
        Q_UNUSED(style);
        Q_UNUSED(config);

        auto comboBox = qobject_cast<QComboBox*>(widget);

        if (comboBox)
        {
            QWidget* frame = comboBox->view()->parentWidget();
            frame->setWindowFlags(frame->windowFlags() & ~(Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint));
            frame->setAttribute(Qt::WA_TranslucentBackground, false);

            frame->setContentsMargins(QMargins());
            delete frame->graphicsEffect();
        }

        return comboBox;
    }

    QSize ComboBox::sizeFromContents(const Style* style, QStyle::ContentsType type, const QStyleOption* option, const QSize& size, const QWidget* widget, const ComboBox::Config& config)
    {
        Q_UNUSED(config);

        QSize comboBoxSize = style->QProxyStyle::sizeFromContents(type, option, size, widget);

        auto opt = qstyleoption_cast<const QStyleOptionComboBox*>(option);
        if (opt && opt->frame)
        {
            return comboBoxSize;
        }

        const int indicatorWidth = style->proxy()->pixelMetric(QStyle::PM_IndicatorWidth, option, widget);
        const int margin = style->proxy()->pixelMetric(QStyle::PM_FocusFrameHMargin, option, widget) + 2;
        comboBoxSize.setWidth(comboBoxSize.width() + indicatorWidth + 3 * margin);
        return comboBoxSize;
    }

    bool ComboBox::drawComboBox(const Style* style, const QStyleOptionComplex* option, QPainter* painter, const QWidget* widget, const Config& config)
    {
        Q_UNUSED(config);

        auto opt = qstyleoption_cast<const QStyleOptionComboBox*>(option);
        if (!opt || opt->frame)
        {
            return false;
        }

        // flat combo box
        style->drawPrimitive(QStyle::PE_IndicatorArrowDown, option, painter, widget);
        return true;
    }

    bool ComboBox::drawComboBoxLabel(const Style* style, const QStyleOption* option, QPainter* painter, const QWidget* widget, const Config& config)
    {
        auto opt = qstyleoption_cast<const QStyleOptionComboBox*>(option);
        if (!opt || opt->frame)
        {
            return false;
        }

        QStyleOptionComboBox cb(*opt);
        cb.palette.setColor(QPalette::Text, config.framelessTextColor);
        const QRect editRect = style->proxy()->subControlRect(QStyle::CC_ComboBox, &cb, QStyle::SC_ComboBoxEditField, widget);
        style->proxy()->drawItemText(painter, editRect.adjusted(1, 0, -1, 0), Qt::AlignLeft | Qt::AlignVCenter, cb.palette, cb.state & QStyle::State_Enabled, cb.currentText, QPalette::Text);
        return true;
    }

    bool ComboBox::drawIndicatorArrow(const Style* style, const QStyleOption* option, QPainter* painter, const QWidget* widget, const Config& config)
    {
        auto opt = qstyleoption_cast<const QStyleOptionComboBox*>(option);
        if (!opt || opt->frame)
        {
            return false;
        }

        QStyleOptionComboBox cb(*opt);
        const QRect downArrowRect = style->proxy()->subControlRect(QStyle::CC_ComboBox, &cb, QStyle::SC_ComboBoxArrow, widget);
        cb.rect = downArrowRect;
        cb.palette.setColor(QPalette::ButtonText, config.framelessTextColor);
        style->QCommonStyle::drawPrimitive(QStyle::PE_IndicatorArrowDown, &cb, painter, widget);
        return true;
    }

} // namespace AzQtComponents
