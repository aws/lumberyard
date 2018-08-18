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
#pragma once

#include <AzQtComponents/AzQtComponentsAPI.h>

#include <QWidget>

class QLabel;

namespace AzQtComponents
{
    // Renders the property names with an ellipsis (...) when they are too long.
    // In that scenario, the tooltip is modified to include the full name as well
    // as the description.
    // e.g.
    // Description "Some stuff that explains it!"
    // Displayed: "A Really Long Nam..."
    // Tooltip:
    // A Really Long Name
    // Some stuff that explains it!
    class AZ_QT_COMPONENTS_API ElidingLabel : public QWidget
    {
        Q_OBJECT
    public:
        explicit ElidingLabel(QWidget* parent = nullptr);

        void SetText(const QString& text) { setText(text); }
        void setText(const QString& text);

        const QString& Text() const { return text(); }
        const QString& text() const { return m_text; }

        void SetDescription(const QString& description) { setDescription(description); }
        void setDescription(const QString& description);

        const QString& Description() const { return description(); }
        const QString& description() const { return m_description; }

        void SetElideMode(Qt::TextElideMode mode) { setElideMode(mode); }
        void setElideMode(Qt::TextElideMode mode);

        void RefreshStyle() { refreshStyle(); }
        void refreshStyle();

        QSize minimumSizeHint() const override;

    protected:
        void paintEvent(QPaintEvent* event) override;
        void resizeEvent(QResizeEvent* event) override;

    private:
        void elide();

        QString m_text;
        QString m_elidedText;
        QString m_description;

        Qt::TextElideMode m_elideMode;

        QLabel* m_label;
    };
} // namespace AzQtComponents
