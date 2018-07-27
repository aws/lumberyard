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
#include <QImage>
#include <QColor>

#include <AzQtComponents/AzQtComponentsAPI.h>

class QSettings;
class QToolButton;

namespace AzQtComponents
{

    class Style;
    class ScreenGrabber;
    class MouseHider;

    class AZ_QT_COMPONENTS_API Eyedropper
        : public QWidget
    {
        Q_OBJECT

    public:
        struct Config
        {
            int contextSizeInPixels;
            int zoomFactor;
        };

        explicit Eyedropper(QWidget* parent, QToolButton* button);
        ~Eyedropper() override;

        bool eventFilter(QObject* target, QEvent* event) override;


        /*!
        * Loads the configuration data from a settings object.
        */
        static Config loadConfig(QSettings& settings);

        /*!
        * Returns default configuration data.
        */
        static Config defaultConfig();

        static bool polish(Style* style, QWidget* widget, const Config& config);

    Q_SIGNALS:
        void colorHovered(QColor color) const;
        void colorSelected(QColor color) const;

    protected:
        void showEvent(QShowEvent *event) override;
        void paintEvent(QPaintEvent* event) override;

    private:
        void setContextAndZoom(int contextSize, int zoomFactor);

        void handleMouseMove();
        void handleKeyPress(QKeyEvent* event);

        void release(bool selected);

        QToolButton* m_button;

        int m_contextSize;
        int m_sampleSize;
        int m_zoomFactor;
        int m_size;
        int m_centerOffset;

        QImage m_sample;
        QColor m_color;

        QScopedPointer<ScreenGrabber> m_grabber;
        QScopedPointer<MouseHider> m_mouseHider;
    };

} // namespace AzQtComponents

