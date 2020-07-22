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
#include <AzQtComponents/Components/Widgets/LogicalTabOrderingWidget.h>
#include <QColor>
#include <QSharedPointer>
#include <QHash>
#include <QVector>
#include <QScopedPointer>

class QAction;
class QActionGroup;
class QMenu;
class QScrollArea;
class QSettings;
class QToolButton;
class QGridLayout;
class QHBoxLayout;
class QSettings;
class QDialogButtonBox;
class QUndoStack;
class QCheckBox;

namespace AzQtComponents
{
    class DoubleSpinBox;
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
    class Eyedropper;
    class Style;
    class PaletteCard;
    class QuickPaletteCard;
    class ColorValidator;
    class GammaEdit;

    namespace Internal
    {
        class ColorController;
        struct ColorLibrarySettings
        {
            bool expanded;
        };
    }

    /**
     * Color Picker Dialog
     *
     * Use this under a number of different configurations to pick a color and manipulate palettes.
     *
     */
    AZ_PUSH_DISABLE_WARNING(4251, "-Wunknown-warning-option") // 4251: 'AzQtComponents::LogicalTabOrderingWidget<AzQtComponents::StyledDialog>::m_entries': class 'QMap<QObject *,AzQtComponents::LogicalTabOrderingInternal::TabKeyEntry>' needs to have dll-interface to be used by clients of class 'AzQtComponents::LogicalTabOrderingWidget<AzQtComponents::StyledDialog>'
    class AZ_QT_COMPONENTS_API ColorPicker
        : public LogicalTabOrderingWidget<StyledDialog>
    {
        Q_OBJECT //AUTOMOC

        Q_PROPERTY(AZ::Color selectedColor READ selectedColor WRITE setSelectedColor NOTIFY selectedColorChanged USER true)
        Q_PROPERTY(AZ::Color currentColor READ currentColor WRITE setCurrentColor NOTIFY currentColorChanged USER true)

    public:
        enum class Configuration
        {
            RGBA,
            RGB,
            HueSaturation // Simplified mode for picking lighting related values
        };

        struct ColorGridConfig
        {
            QSize minimumSize;
        };

        struct DialogButtonsConfig
        {
            int topPadding;
        };

        struct Config
        {
            int padding;
            int spacing;
            int maximumContentHeight;
            ColorGridConfig colorGrid;
            DialogButtonsConfig dialogButtons;
        };

        /*!
         * Loads the color picker config data from a settings object.
         */
        static Config loadConfig(QSettings& settings);

        /*!
         * Returns default color picker config data.
         */
        static Config defaultConfig();

        explicit ColorPicker(Configuration configuration, const QString& context = QString(), QWidget* parent = nullptr);
        ~ColorPicker() override;

        AZ::Color currentColor() const;
        AZ::Color selectedColor() const;

        void importPalettesFromFolder(const QString& path);

        static AZ::Color getColor(Configuration configuration, const AZ::Color& initial, const QString& title, const QString& context = QString(), const QStringList& palettePaths = QStringList(), QWidget* parent = nullptr);

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

    protected:
        bool eventFilter(QObject* o, QEvent* e) override;
        void hideEvent(QHideEvent* event) override;
        void done(int result) override;

        void contextMenuEvent(QContextMenuEvent* e) override;

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
        friend class EditorProxyStyle;
        friend class CurrentColorChangedCommand;
        friend class PaletteAddedCommand;
        friend class PaletteRemovedCommand;

        static bool polish(Style* style, QWidget* widget, const Config& config);

        void warnColorAdjusted(const QString& message);

        void setConfiguration(Configuration configuration);
        void applyConfigurationRGBA(); 
        void applyConfigurationRGB();
        void applyConfigurationHueSaturation();
        void polish(const Config& config);
        void refreshCardMargins();

        void setColorGridMode(ColorGrid::Mode mode);
        void initContextMenu(Configuration configuration);

        void readSettings();
        void writeSettings() const;

        void savePalette(QSharedPointer<PaletteCard> palette, bool queryFileName);
        bool saveColorLibrary(ColorLibrary& colorLibrary, bool queryFileName);

        bool importPaletteFromFile(const QString& fileName, const Internal::ColorLibrarySettings& settings);
        int colorLibraryIndex(const Palette* palette) const;

        void addPalette(QSharedPointer<Palette> palette, const QString& fileName, const QString& title, const Internal::ColorLibrarySettings& settings);
        void addPaletteCard(QSharedPointer<PaletteCard> card, ColorLibrary colorLibrary);
        void removePaletteCard(QSharedPointer<PaletteCard> card);
        bool saveChangedPalettes();

        void beginDynamicColorChange();
        void endDynamicColorChange();

        void initializeValidation(ColorValidator* validator);
        void showPreviewContextMenu(const QPoint& p, const AZ::Color& selectedColor);

        void swatchSizeActionToggled(bool checked, int newSize);
        void setQuickPaletteVisibility(bool show);

        void paletteContextMenuRequested(QSharedPointer<PaletteCard> paletteCard, const QPoint& point);
        void quickPaletteContextMenuRequested(const QPoint& point);

        QWidget* makeSeparator(QWidget* parent);
        QWidget* makePaddedSeparator(QWidget* parent);

        Configuration m_configuration;
        QString m_context;
        Config m_config;
        Internal::ColorController* m_currentColorController = nullptr;
        AZ::Color m_selectedColor;
        
        QSharedPointer<Palette> m_quickPalette;
        QuickPaletteCard* m_quickPaletteCard;

        QHash<QSharedPointer<PaletteCard>, ColorLibrary> m_colorLibraries;

        QUndoStack* m_undoStack = nullptr;

        QScrollArea* m_scrollArea = nullptr;
        QWidget* m_containerWidget = nullptr;
        
        QGridLayout* m_hsvPickerLayout = nullptr;
        QHBoxLayout* m_rgbHexLayout = nullptr;
        QHBoxLayout* m_quickPaletteLayout = nullptr;

        GradientSlider* m_alphaSlider = nullptr;
        ColorGrid* m_colorGrid = nullptr;
        GradientSlider* m_hueSlider = nullptr;
        GradientSlider* m_valueSlider = nullptr;
        QToolButton* m_eyedropperButton = nullptr;
        QToolButton* m_toggleHueGridButton = nullptr;
        ColorPreview* m_preview = nullptr;
        ColorWarning* m_warning = nullptr;
        ColorRGBAEdit* m_rgbaEdit = nullptr;
        ColorHexEdit* m_hexEdit = nullptr;
        QWidget* m_hslSlidersSeparator = nullptr;
        HSLSliders* m_hslSliders = nullptr;
        QWidget* m_hsvSlidersSeparator = nullptr;
        HSVSliders* m_hsvSliders = nullptr;
        QWidget* m_rgbSlidersSeparator = nullptr;
        RGBSliders* m_rgbSliders = nullptr;
        QWidget* m_quickPaletteSeparator = nullptr;
        QWidget* m_paletteCardSeparator = nullptr;
        PaletteCardCollection* m_paletteCardCollection = nullptr;
        QMenu* m_menu = nullptr;
        Eyedropper* m_eyedropper = nullptr;
        QAction* m_showLinearValuesAction = nullptr;
        QAction* m_showHexValueAction = nullptr;
        QAction* m_showHSLSlidersAction = nullptr;
        QAction* m_showHSVSlidersAction = nullptr;
        QAction* m_showRGBSlidersAction = nullptr;
        QActionGroup* m_swatchSizeGroup = nullptr;

        bool m_dynamicColorChange = false;
        AZ::Color m_previousColor;
        QAction* m_undoAction = nullptr;
        QAction* m_redoAction = nullptr;

        QAction* m_importPaletteAction = nullptr;
        QAction* m_newPaletteAction = nullptr;
        QAction* m_toggleQuickPaletteAction = nullptr;
        QWidget* m_gammaSeparator = nullptr;
        GammaEdit* m_gammaEdit = nullptr;
        QDialogButtonBox* m_dialogButtonBox = nullptr;
        qreal m_defaultVForHsMode = 0.0f;
        qreal m_defaultLForHsMode = 0.0f;

        QString m_lastSaveDirectory;
        QVector<QWidget*> m_separators;
    };
    AZ_POP_DISABLE_WARNING

} // namespace AzQtComponents
