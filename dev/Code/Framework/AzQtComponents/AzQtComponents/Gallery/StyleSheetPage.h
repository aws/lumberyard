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

#include <QWidget>
#include <QScopedPointer>

namespace Ui
{
    class StyleSheetPage;
}

namespace Example
{
    /* Example::Widget is used to demonstrate StyleHelpers::repolishWhenPropertyChanges.
     *
     * StyleHelpers::repolishWhenPropertyChanges is used to ensure that style sheet rules that
     * depend on the property of a widget are correctly updated when that property changes.
     *
     * See also: ExampleWidget.qss
     */
    class Widget : public QWidget
    {
        Q_OBJECT
        Q_PROPERTY(bool drawSimple READ drawSimple WRITE setDrawSimple NOTIFY drawSimpleChanged)
    public:
        explicit Widget(QWidget* parent = nullptr);
        ~Widget() override;

        bool drawSimple() const { return m_drawSimple; }

    public Q_SLOTS:
        void setDrawSimple(bool drawSimple);

    Q_SIGNALS:
        void drawSimpleChanged(bool drawSimple);

    private:
        bool m_drawSimple;
    };
}

class StyleSheetPage : public QWidget
{
    Q_OBJECT

public:
    explicit StyleSheetPage(QWidget* parent = nullptr);
    ~StyleSheetPage() override;

private:
    QScopedPointer<Ui::StyleSheetPage> ui;
};
