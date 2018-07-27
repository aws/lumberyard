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
            if (m_combo && m_combo->view()->selectionMode() == QAbstractItemView::SingleSelection)
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
        auto ReadInteger = [](QSettings& settings, const QString& name, int& value)
        {
            // only overwrite the value if it's set; otherwise, it'll stay the default
            if (settings.contains(name))
            {
                value = settings.value(name).toInt();
            }
        };
        auto ReadColor = [](QSettings& settings, const QString& name, QColor& color)
        {
            // only overwrite the value if it's set; otherwise, it'll stay the default
            if (settings.contains(name))
            {
                color = QColor(settings.value(name).toString());
            }
        };

        Config config = defaultConfig();

        ReadInteger(settings, "BoxShadowXOffset", config.boxShadowXOffset);
        ReadInteger(settings, "BoxShadowYOffset", config.boxShadowYOffset);
        ReadInteger(settings, "BoxShadowBlurRadius", config.boxShadowBlurRadius);
        ReadColor(settings, "BoxShadowColor", config.boxShadowColor);
        ReadColor(settings, "PlaceHolderTextColor", config.placeHolderTextColor);

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

} // namespace AzQtComponents
