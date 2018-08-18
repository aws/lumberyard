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
#include <AzQtComponents/Components/Widgets/Card.h>
#include <AzQtComponents/Components/Widgets/ColorPicker/Palette.h>

class QUndoStack;

namespace AzQtComponents
{
    namespace Internal
    {
        class ColorController;
    }

    class CardHeader;
    class PaletteView;

    class AZ_QT_COMPONENTS_API PaletteCard
        : public QWidget
    {
        Q_OBJECT
        Q_PROPERTY(QString title READ title WRITE setTitle NOTIFY titleChanged);
        Q_PROPERTY(bool modified READ modified WRITE setModified NOTIFY modifiedChanged);
        Q_PROPERTY(QSharedPointer<Palette> palette READ palette);

    public:
        explicit PaletteCard(QSharedPointer<Palette> palette, Internal::ColorController* controller, QUndoStack* undoStack, QWidget* parent = nullptr);
        ~PaletteCard() override;

        QString title() const;
        void setTitle(const QString& title);

        bool modified() const;
        void setModified(bool modified);

        QSharedPointer<Palette> palette() const;

        void setSwatchSize(const QSize& size);
        void setGammaEnabled(bool enabled);
        void setGamma(qreal gamma);

    Q_SIGNALS:
        void titleChanged(const QString& title);
        void modifiedChanged(bool modified);
        void removeClicked();
        void saveClicked();
        void colorSelected(const AZ::Color& color);

    private Q_SLOTS:
        void handleModelChanged();

    private:
        void updateHeader();

        QString m_title;
        bool m_modified;
        QSharedPointer<Palette> m_palette;

        CardHeader* m_header;
        PaletteView* m_paletteView;
    };
} // namespace AzQtComponents
