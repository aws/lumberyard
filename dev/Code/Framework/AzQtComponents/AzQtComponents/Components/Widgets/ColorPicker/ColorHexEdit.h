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

class QLineEdit;

namespace AzQtComponents
{
    class AZ_QT_COMPONENTS_API ColorHexEdit
        : public QWidget
    {
        Q_OBJECT
        Q_PROPERTY(qreal red READ red WRITE setRed NOTIFY redChanged);
        Q_PROPERTY(qreal green READ green WRITE setGreen NOTIFY greenChanged);
        Q_PROPERTY(qreal blue READ blue WRITE setBlue NOTIFY blueChanged);

    public:
        explicit ColorHexEdit(QWidget* parent = nullptr);
        ~ColorHexEdit() override;

        qreal red() const;
        qreal green() const;
        qreal blue() const;

        bool eventFilter(QObject* watched, QEvent* event) override;

    Q_SIGNALS:
        void redChanged(qreal red);
        void greenChanged(qreal green);
        void blueChanged(qreal blue);
        void valueChangeBegan();
        void valueChangeEnded();

    public Q_SLOTS:
        void setRed(qreal red);
        void setGreen(qreal green);
        void setBlue(qreal blue);

    private Q_SLOTS:
        void textChanged(const QString& text);

    private:
        void initEditValue();

        qreal m_red;
        qreal m_green;
        qreal m_blue;

        QLineEdit* m_edit;

        bool m_valueChanging = false;
    };
} // namespace AzQtComponents
