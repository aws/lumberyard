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

#include <QLabel>

namespace AzQtComponents
{
        /**  Extends the QLabel widget to automatically truncate (elide) the label text
    *    with ellipsis (...) if it doesn't fit within the width of the widget.

    *    The tooltip for the label is also set to include the full label text
    *    as well as an optional description, for example:
    *      Truncated Display:  "A long label that..."
    *      Tooltip:  "A long label that didn't fit
    *                 Here is where the optional description will show"
    */
    class AZ_QT_COMPONENTS_API ElidingLabel
        : public QLabel
    {
        Q_OBJECT
    public:
        explicit ElidingLabel(const QString& text, QWidget* parent = nullptr);
        explicit ElidingLabel(QWidget* parent = nullptr) : ElidingLabel("", parent) {}

        void SetText(const QString& text) { setText(text); }
        void setText(const QString& text);

        void SetFilter(const QString& filter) { setFilter(filter); }
        void setFilter(const QString& filter);

        const QString& Text() const { return text(); }
        const QString& text() const { return m_text; }
        const QString& ElidedText() { return elidedText(); }
        const QString& elidedText() { return m_elidedText; }

        void SetDescription(const QString& description) { setDescription(description); }
        void setDescription(const QString& description);

        const QString& Description() const { return description(); }
        const QString& description() const { return m_description; }

        void SetElideMode(Qt::TextElideMode mode) { setElideMode(mode); }
        void setElideMode(Qt::TextElideMode mode);

        void RefreshStyle() { refreshStyle(); }
        void refreshStyle();

        void setObjectName(const QString &name);
        QSize minimumSizeHint() const override;
        QSize sizeHint() const override;

    protected:
        void resizeEvent(QResizeEvent* event) override;
        void paintEvent(QPaintEvent* event) override;

        QString m_filterString;
        QRegExp m_filterRegex;

    private:
        void elide();
        QRect TextRect();

        const QColor backgroundColor{ "#707070" };

        QString m_text;
        QString m_elidedText;
        QString m_description;

        Qt::TextElideMode m_elideMode;
        QLabel* m_metricsLabel;
    };

} // namespace AzQtComponents
