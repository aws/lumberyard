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

class QComboBox;

namespace AzQtComponents
{
    class TagContainer;

    class AZ_QT_COMPONENTS_API TagWidget
        : public QWidget
    {
        Q_OBJECT
    public:
        explicit TagWidget(const QString& text, QWidget* parent = nullptr);
        void setText(const QString&);
        QString text() const;
        QSize sizeHint() const;

    signals:
        void deleteClicked();

    protected:
        void paintEvent(QPaintEvent* ev) override;
        void mousePressEvent(QMouseEvent* ev) override;
        void mouseReleaseEvent(QMouseEvent* ev) override;

    private:
        QRect deleteButtonRect() const;
        int delegeButtonRightOffset() const;
        QRect textRect() const;
        QString m_text;
        QPixmap m_deleteButton;
        bool m_deleteButtonPressed = false;
    };

    class AZ_QT_COMPONENTS_API TagSelector
        : public QWidget
    {
        Q_OBJECT
    public:
        explicit TagSelector(QWidget* parent = nullptr);
        QComboBox* combo() const;

    public Q_SLOTS:

        void selectTag(int comboIndex);

    private:
        QComboBox* const m_combo;
        TagContainer* const m_tagContainer;
    };
} // namespace AzQtComponents
