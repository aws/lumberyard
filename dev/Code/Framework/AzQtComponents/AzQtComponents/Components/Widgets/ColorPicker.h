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
#include <AzQtComponents/Components/StyledDialog.h>
#include <AzQtComponents/Components/Widgets/ColorPicker/ColorGrid.h>
#include <AzQtComponents/Components/Widgets/ColorPicker/Palette.h>
#include <QColor>
#include <QSharedPointer>
#include <QHash>

class QAction;
class QActionGroup;
class QFrame;
class QMenu;
class QScrollArea;
class QSettings;
class QToolButton;
class QGridLayout;
class QHBoxLayout;
class QSettings;
class QDialogButtonBox;
class QUndoStack;

namespace AzQtComponents
{
    class GradientSlider;
    class HSLSliders;
    class HSVSliders;
    class RGBSliders;
    class ColorPreview;
    class ColorWarning;
    class ColorRGBAEdit;
    class ColorHexEdit;
    class PaletteModel;
    class PaletteView;
    class PaletteCardCollection;
    class GammaEdit;
    class Eyedropper;
    class Style;
    class PaletteCard;

    namespace Internal
    {
        class ColorController;
    }

    /**
     * Color Picker Dialog
     *
     * Use this under a number of different configurations to pick a color and manipulate palettes.
     *
     */
    class AZ_QT_COMPONENTS_API ColorPicker
        : public StyledDialog
    {
        Q_OBJECT //AUTOMOC

        Q_PROPERTY(AZ::Color selectedColor READ selectedColor WRITE setSelectedColor NOTIFY selectedColorChanged USER true)
        Q_PROPERTY(AZ::Color currentColor READ currentColor WRITE setCurrentColor NOTIFY currentColorChanged USER true)

    public:
        enum class Configuration
        {
            // will definitely come up with smarter names for this before we go live with this...
            A,
            B,
            C
        };

        struct ColorGridConfig
        {
            QSize minimumSize;
        };

        struct Config
        {
            int padding;
            int spacing;
            int maximumContentHeight;
            ColorGridConfig colorGrid;
        };

        /*!
         * Loads the color picker config data from a settings object.
         */
        static Config loadConfig(QSettings& settings);

        /*!
         * Returns default color picker config data.
         */
        static Config defaultConfig();

        explicit ColorPicker(Configuration configuration, QWidget* parent = nullptr);
        ~ColorPicker() override;

        AZ::Color currentColor() const;
        AZ::Color selectedColor() const;

        void importPalettesFromFolder(const QString& path);

        static AZ::Color getColor(Configuration configuration, const AZ::Color& initial, const QString& title, const QStringList& palettePaths = QStringList(), QWidget* parent = nullptr);

    Q_SIGNALS:
        void selectedColorChanged(const AZ::Color& color);
        void currentColorChanged(const AZ::Color& color);

    public Q_SLOTS:
        void setSelectedColor(const AZ::Color& color);
        void setCurrentColor(const AZ::Color& color);

    private Q_SLOTS:
        void importPalette();
        void newPalette();
        void removePaletteCardRequested(QSharedPointer<PaletteCard> card);
        void savePalette(QSharedPointer<PaletteCard> palette);

    protected:
        void closeEvent(QCloseEvent* e) override;
        bool eventFilter(QObject* o, QEvent* e) override;

    private:
        struct ColorLibrary
        {
            QString fileName;
            QSharedPointer<Palette> palette;
        };

        class CurrentColorChangedCommand;
        class PaletteAddedCommand;
        class PaletteRemovedCommand;

        friend class Style;
        friend class CurrentColorChangedCommand;
        friend class PaletteAddedCommand;
        friend class PaletteRemovedCommand;

        static bool polish(Style* style, QWidget* widget, const Config& config);

        void setConfiguration(Configuration configuration);
        void applyConfigurationA(); 
        void applyConfigurationB();
        void applyConfigurationC();
        void polish(const Config& config);

        void setColorGridMode(ColorGrid::Mode mode);
        void initContextMenu(Configuration configuration);

        void readSettings();
        void writeSettings() const;

        bool importPaletteFromFile(const QString& fileName);
        int colorLibraryIndex(const Palette* palette) const;

        void addPalette(QSharedPointer<Palette> palette, const QString& fileName, const QString& title);
        void addPaletteCard(QSharedPointer<PaletteCard> card, ColorLibrary colorLibrary);
        void removePaletteCard(QSharedPointer<PaletteCard> card);

        void beginDynamicColorChange();
        void endDynamicColorChange();

        Configuration m_configuration;
        Config m_config;
        Internal::ColorController* m_currentColorController;
        AZ::Color m_selectedColor;

        Palette m_quickPalette;

        QHash<QSharedPointer<PaletteCard>, ColorLibrary> m_colorLibraries;

        QUndoStack* m_undoStack;

        QScrollArea* m_scrollArea;
        QWidget* m_containerWidget;
        
        QGridLayout* m_hsvPickerLayout;
        QHBoxLayout* m_rgbHexLayout;
        QHBoxLayout* m_quickPaletteLayout;

        GradientSlider* m_alphaSlider;
        ColorGrid* m_colorGrid;
        GradientSlider* m_hueSlider;
        GradientSlider* m_valueSlider;
        QToolButton* m_eyedropperButton;
        QToolButton* m_toggleHueGridButton;
        ColorPreview* m_preview;
        ColorWarning* m_warning;
        ColorRGBAEdit* m_rgbaEdit;
        ColorHexEdit* m_hexEdit;
        QFrame* m_slidersSeparator;
        HSLSliders* m_hslSliders;
        HSVSliders* m_hsvSliders;
        RGBSliders* m_rgbSliders;
        QFrame* m_quickPaletteSeparator;
        PaletteModel* m_paletteModel;
        PaletteView* m_paletteView;
        QFrame* m_paletteCardSeparator;
        PaletteCardCollection* m_paletteCardCollection;
        QFrame* m_gammaEditSeparator;
        GammaEdit* m_gammaEdit;
        QMenu* m_menu;
        Eyedropper* m_eyedropper;
        QAction* m_showLinearValuesAction;
        QAction* m_showHexValueAction;
        QAction* m_showHSLSlidersAction;
        QAction* m_showHSVSlidersAction;
        QAction* m_showRGBSlidersAction;
        QActionGroup* m_swatchSizeGroup;
        QAction* m_showGammaAction;

        bool m_dynamicColorChange = false;
        AZ::Color m_previousColor;
        QAction* m_undoAction;
        QAction* m_redoAction;

        QDialogButtonBox* m_dialogButtonBox;
    };
} // namespace AzQtComponents
