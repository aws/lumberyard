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

#include <QDockWidget>
#include <QPoint>

namespace AzQtComponents
{
    class TitleBar;

    class AZ_QT_COMPONENTS_API StyledDockWidget
        : public QDockWidget
    {
        Q_OBJECT

    public:
        explicit StyledDockWidget(const QString& name, QWidget* parent = nullptr);
        explicit StyledDockWidget(QWidget* parent = nullptr);
        ~StyledDockWidget();

        static void drawFrame(QPainter& p, QRect rect, bool drawTop = true);
        void createCustomTitleBar();
        TitleBar* customTitleBar();
        bool isSingleFloatingChild();

    Q_SIGNALS:
        void undock();

    protected:
        void closeEvent(QCloseEvent* event) override;
        bool nativeEvent(const QByteArray& eventType, void* message, long* result) override;
        bool event(QEvent* event) override;
        void showEvent(QShowEvent* event) override;
        void paintEvent(QPaintEvent*) override;

    private:
        void fixFramelessFlags();
        void onFloatingChanged(bool floating);
        void init();
        bool m_dropShadowsEnabled = false;
    };
} // namespace AzQtComponents
