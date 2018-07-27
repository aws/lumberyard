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
#include <AzQtComponents/Components/Widgets/ColorPicker.h>

#include <AzQtComponents/Components/Widgets/GradientSlider.h>
#include <AzQtComponents/Components/Widgets/ColorPicker/ColorController.h>
#include <AzQtComponents/Components/Widgets/ColorPicker/ColorComponentSliders.h>
#include <AzQtComponents/Components/Widgets/ColorPicker/ColorPreview.h>
#include <AzQtComponents/Components/Widgets/ColorPicker/ColorRGBAEdit.h>
#include <AzQtComponents/Components/Widgets/ColorPicker/ColorHexEdit.h>
#include <AzQtComponents/Components/Widgets/ColorPicker/ColorGrid.h>
#include <AzQtComponents/Components/Widgets/ColorPicker/PaletteView.h>
#include <AzQtComponents/Components/Widgets/ColorPicker/PaletteCard.h>
#include <AzQtComponents/Components/Widgets/ColorPicker/PaletteCardCollection.h>
#include <AzQtComponents/Components/Widgets/ColorPicker/GammaEdit.h>
#include <AzQtComponents/Components/Widgets/ColorPicker/ColorWarning.h>
#include <AzQtComponents/Components/Widgets/Eyedropper.h>
#include <AzQtComponents/Components/Style.h>
#include <AzQtComponents/Utilities/Conversions.h>
#include <AzQtComponents/Utilities/ColorUtilities.h>

#include <QEvent>
#include <QResizeEvent>
#include <QFileDialog>
#include <QMessageBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QDialogButtonBox>
#include <QMenu>
#include <QScrollArea>
#include <QScrollBar>
#include <QSettings>
#include <QToolButton>
#include <QDirIterator>
#include <QUndoStack>

// settings keys
static const char* g_colorPickerSection = "ColorPicker";
static const char* g_showLinearValuesKey = "LinearValues";
static const char* g_showHexValuesKey = "HexValues";
static const char* g_showHSLSlidersKey = "HSLSliders";
static const char* g_showHSVSlidersKey = "HSVSliders";
static const char* g_showRGBSlidersKey = "RGBSliders";
static const char* g_showHSSlidersKey = "HSSliders";
static const char* g_quickPaletteKey = "QuickPalette";
static const char* g_colorLibrariesKey = "ColorLibraries";
static const char* g_swatchSizeKey = "SwatchSize";
static const char* g_showGammaKey = "Gamma";
static const char* g_gammaCheckedKey = "GammaChecked";
static const char* g_gammaValueKey = "GammaValue";

namespace AzQtComponents
{

    namespace
    {
        const QString SEPARATOR_CLASS = QStringLiteral("HorizontalSeparator");
        static const AZ::Color INVALID_COLOR = AZ::Color{ -1.0f, -1.0f, -1.0f, -1.0f };

        void RemoveAllWidgets(QLayout* layout)
        {
            for (int i = 0; i < layout->count(); ++i)
            {
                layout->removeWidget(layout->itemAt(i)->widget());
            }
        }

        QFrame* MakeSeparator(QWidget* parent)
        {
            auto separator = new QFrame(parent);
            separator->setFrameStyle(QFrame::StyledPanel);
            separator->setFrameShadow(QFrame::Plain);
            separator->setFrameShape(QFrame::HLine);
            Style::addClass(separator, SEPARATOR_CLASS);
            return separator;
        }

        const char* ConfigurationName(ColorPicker::Configuration configuration)
        {
            switch (configuration)
            {
            case ColorPicker::Configuration::A:
                return "ConfigurationA";
            case ColorPicker::Configuration::B:
                return "ConfigurationB";
            case ColorPicker::Configuration::C:
                return "ConfigurationC";
            }

            Q_UNREACHABLE();
        }

        void ReadColorGridConfig(QSettings& settings, const QString& name, ColorPicker::ColorGridConfig& colorGrid)
        {
            settings.beginGroup(name);
            colorGrid.minimumSize = settings.value("MinimumSize", colorGrid.minimumSize).toSize();
            settings.endGroup();
        }
    } // namespace
    
    class ColorPicker::CurrentColorChangedCommand : public QUndoCommand
    {
    public:
        CurrentColorChangedCommand(ColorPicker* picker, const AZ::Color& previousColor, const AZ::Color& newColor, QUndoCommand* parent = nullptr);
        ~CurrentColorChangedCommand() override;

        void undo() override;
        void redo() override;

    private:
        ColorPicker* m_picker;
        const AZ::Color m_previousColor;
        const AZ::Color m_newColor;
    };

    ColorPicker::CurrentColorChangedCommand::CurrentColorChangedCommand(ColorPicker* picker, const AZ::Color& previousColor, const AZ::Color& newColor, QUndoCommand* parent)
        : QUndoCommand(parent)
        , m_picker(picker)
        , m_previousColor(previousColor)
        , m_newColor(newColor)
    {
    }

    ColorPicker::CurrentColorChangedCommand::~CurrentColorChangedCommand()
    {
    }

    void ColorPicker::CurrentColorChangedCommand::undo()
    {
        m_picker->m_previousColor = m_previousColor;
        m_picker->m_currentColorController->setColor(m_previousColor);
    }

    void ColorPicker::CurrentColorChangedCommand::redo()
    {
        m_picker->m_previousColor = m_newColor;
        m_picker->m_currentColorController->setColor(m_newColor);
    }

    class ColorPicker::PaletteAddedCommand : public QUndoCommand
    {
    public:
        PaletteAddedCommand(ColorPicker* picker, QSharedPointer<PaletteCard> card, ColorLibrary colorLibrary, QUndoCommand* parent = nullptr);
        ~PaletteAddedCommand() override;

        void undo() override;
        void redo() override;

    private:
        ColorPicker* m_picker;
        QSharedPointer<PaletteCard> m_card;
        ColorLibrary m_colorLibrary;
    };

    ColorPicker::PaletteAddedCommand::PaletteAddedCommand(ColorPicker* picker, QSharedPointer<PaletteCard> card, ColorLibrary colorLibrary, QUndoCommand* parent)
        : QUndoCommand(parent)
        , m_picker(picker)
        , m_card(card)
        , m_colorLibrary(colorLibrary)
    {
    }

    ColorPicker::PaletteAddedCommand::~PaletteAddedCommand()
    {
    }

    void ColorPicker::PaletteAddedCommand::undo()
    {
        m_picker->removePaletteCard(m_card);
    }

    void ColorPicker::PaletteAddedCommand::redo()
    {
        m_picker->addPaletteCard(m_card, m_colorLibrary);
    }

    class ColorPicker::PaletteRemovedCommand : public QUndoCommand
    {
    public:
        PaletteRemovedCommand(ColorPicker* picker, QSharedPointer<PaletteCard> card, ColorLibrary colorLibrary, QUndoCommand* parent = nullptr);
        ~PaletteRemovedCommand() override;

        void undo() override;
        void redo() override;

    private:
        ColorPicker* m_picker;
        QSharedPointer<PaletteCard> m_card;
        ColorLibrary m_colorLibrary;
    };

    ColorPicker::PaletteRemovedCommand::PaletteRemovedCommand(ColorPicker* picker, QSharedPointer<PaletteCard> card, ColorLibrary colorLibrary, QUndoCommand* parent)
        : QUndoCommand(parent)
        , m_picker(picker)
        , m_card(card)
        , m_colorLibrary(colorLibrary)
    {
    }

    ColorPicker::PaletteRemovedCommand::~PaletteRemovedCommand()
    {
    }

    void ColorPicker::PaletteRemovedCommand::undo()
    {
        m_picker->addPaletteCard(m_card, m_colorLibrary);
    }

    void ColorPicker::PaletteRemovedCommand::redo()
    {
        m_picker->removePaletteCard(m_card);
    }

ColorPicker::Config ColorPicker::loadConfig(QSettings& settings)
{
    Config config = defaultConfig();
    config.padding = settings.value(QStringLiteral("Padding"), config.padding).toInt();
    config.spacing = settings.value(QStringLiteral("Spacing"), config.spacing).toInt();
    config.maximumContentHeight = settings.value(QStringLiteral("MaximumContentHeight"), config.maximumContentHeight).toInt();
    ReadColorGridConfig(settings, "ColorGrid", config.colorGrid);
    return config;
}

ColorPicker::Config ColorPicker::defaultConfig()
{
    return ColorPicker::Config{
        16,  // Padding
        8,   // Spacing;
        492, // MaximumContentHeight
        {    // ColorGrid
            {194, 150} // ColorGrid/MinimumSize
        }
    };
}

bool ColorPicker::polish(Style* style, QWidget* widget, const Config& config)
{
    auto colorPicker = qobject_cast<ColorPicker*>(widget);
    if (!colorPicker)
    {
        return false;
    }

    colorPicker->polish(config);
    style->repolishOnSettingsChange(colorPicker);
    return true;
}

void ColorPicker::polish(const ColorPicker::Config& config)
{
    m_config = config;

    // Outer layout
    layout()->setSpacing(m_config.spacing);
    layout()->setContentsMargins({ 0, m_config.padding, 0, m_config.padding });

    // Color grid, preview, eyedropper
    m_colorGrid->setMinimumSize(m_config.colorGrid.minimumSize);

    m_hsvPickerLayout->setContentsMargins({ m_config.padding, 0, m_config.padding, 0 });

    m_rgbaEdit->setHorizontalSpacing(m_config.spacing);
    m_rgbHexLayout->setContentsMargins({ m_config.padding, m_config.spacing, m_config.padding, 0 });
    m_rgbHexLayout->setSpacing(m_config.spacing);

    m_quickPaletteLayout->setContentsMargins(QMargins{ m_config.padding, 0, m_config.padding, 0 });
    m_quickPaletteLayout->addWidget(m_paletteView);

    m_paletteCardCollection->setContentsMargins(QMargins{ 0, 0, m_config.padding, 0 });
    m_gammaEdit->setContentsMargins(QMargins{ m_config.padding, 0, m_config.padding, 0 });

    // Special case as the artboards show a 12px padding above the buttons
    m_dialogButtonBox->setContentsMargins(QMargins{ m_config.padding, 4, m_config.padding, 0 });

    m_hslSliders->layout()->setContentsMargins(QMargins{ m_config.padding, 0, m_config.padding, 0 });
    m_hslSliders->layout()->setSpacing(m_config.spacing);
    m_hsvSliders->layout()->setContentsMargins(QMargins{ m_config.padding, 0, m_config.padding, 0 });
    m_hsvSliders->layout()->setSpacing(m_config.spacing);
    m_rgbSliders->layout()->setContentsMargins(QMargins{ m_config.padding, 0, m_config.padding, 0 });
    m_rgbSliders->layout()->setSpacing(m_config.spacing);
}

ColorPicker::ColorPicker(ColorPicker::Configuration configuration, QWidget* parent)
    : StyledDialog(parent)
    , m_configuration(configuration)
    , m_undoStack(new QUndoStack(this))
    , m_previousColor(INVALID_COLOR)
{
    qRegisterMetaTypeStreamOperators<Palette>("AzQtComponents::Palette");

    m_currentColorController = new Internal::ColorController(this);
    connect(m_currentColorController, &Internal::ColorController::colorChanged, this, &ColorPicker::currentColorChanged);
    connect(m_currentColorController, &Internal::ColorController::colorChanged, this, [this]() {
        if (!m_dynamicColorChange)
        {
            endDynamicColorChange();
        }
    });

    auto mainLayout = new QVBoxLayout(this);
    mainLayout->setSizeConstraint(QLayout::SetFixedSize);

    m_scrollArea = new QScrollArea(this);
    mainLayout->addWidget(m_scrollArea);

    m_scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    m_containerWidget = new QWidget(this);
    m_containerWidget->setObjectName(QStringLiteral("Container"));
    m_containerWidget->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Expanding);
    m_containerWidget->installEventFilter(this);
    m_scrollArea->setWidget(m_containerWidget);

    auto containerLayout = new QVBoxLayout(m_containerWidget);
    containerLayout->setSizeConstraint(QLayout::SetFixedSize);
    containerLayout->setContentsMargins(0, 0, 0, 0);

    // alpha slider + color grid + lightness slider + color preview

    m_hsvPickerLayout = new QGridLayout();
    containerLayout->addLayout(m_hsvPickerLayout);

    // alpha slider

    m_alphaSlider = new GradientSlider(Qt::Vertical, this);
    m_alphaSlider->setColorFunction([this](qreal position) {
        auto color = ToQColor(m_currentColorController->color());
        color.setAlphaF(position);
        return color;
    });
    m_alphaSlider->setMinimum(0);
    m_alphaSlider->setMaximum(255);

    connect(m_alphaSlider, &QSlider::valueChanged, this, [this](int alpha) {
        m_currentColorController->setAlpha(static_cast<qreal>(alpha)/255.0);
    });
    connect(m_currentColorController, &Internal::ColorController::colorChanged, m_alphaSlider, &GradientSlider::updateGradient);
    connect(m_currentColorController, &Internal::ColorController::alphaChanged, this, [this](qreal alpha) {
        const QSignalBlocker b(m_alphaSlider);
        m_alphaSlider->setValue(qRound(alpha*255.0));
    });

    // color grid

    m_colorGrid = new ColorGrid(this);

    connect(m_currentColorController, &Internal::ColorController::hsvHueChanged, m_colorGrid, &ColorGrid::setHue);
    connect(m_currentColorController, &Internal::ColorController::hsvSaturationChanged, m_colorGrid, &ColorGrid::setSaturation);
    connect(m_currentColorController, &Internal::ColorController::valueChanged, m_colorGrid, &ColorGrid::setValue);
    connect(m_colorGrid, &ColorGrid::hueChanged, m_currentColorController, &Internal::ColorController::setHsvHue);
    connect(m_colorGrid, &ColorGrid::saturationChanged, m_currentColorController, &Internal::ColorController::setHsvSaturation);
    connect(m_colorGrid, &ColorGrid::valueChanged, m_currentColorController, &Internal::ColorController::setValue);
    connect(m_colorGrid, &ColorGrid::gridPressed, this, &ColorPicker::beginDynamicColorChange);
    connect(m_colorGrid, &ColorGrid::gridReleased, this, &ColorPicker::endDynamicColorChange);

    // hue slider

    m_hueSlider = new GradientSlider(Qt::Vertical, this);
    m_hueSlider->setColorFunction([](qreal position) {
        return QColor::fromHsvF(position, 1.0, 1.0);
    });
    m_hueSlider->setMinimum(0);
    m_hueSlider->setMaximum(360);

    connect(m_hueSlider, &QSlider::valueChanged, this, [this](int value) {
        m_currentColorController->setHsvHue(static_cast<qreal>(value)/360.0);
    });
    connect(m_hueSlider, &GradientSlider::sliderPressed, this, &ColorPicker::beginDynamicColorChange);
    connect(m_hueSlider, &GradientSlider::sliderReleased, this, &ColorPicker::endDynamicColorChange);
    connect(m_currentColorController, &Internal::ColorController::hsvHueChanged, this, [this](qreal hue) {
        const QSignalBlocker b(m_hueSlider);
        m_hueSlider->setValue(qRound(hue*360.0));
    });

    // value slider

    m_valueSlider = new GradientSlider(Qt::Vertical, this);
    m_valueSlider->setColorFunction([this](qreal position) {
        return QColor::fromHsvF(m_currentColorController->hsvHue(), m_currentColorController->hsvSaturation(), position);
    });
    m_valueSlider->setMinimum(0);
    m_valueSlider->setMaximum(255);

    connect(m_valueSlider, &QSlider::valueChanged, this, [this](int value) {
        m_currentColorController->setValue(static_cast<qreal>(value)/255.0);
    });
    connect(m_valueSlider, &GradientSlider::sliderPressed, this, &ColorPicker::beginDynamicColorChange);
    connect(m_valueSlider, &GradientSlider::sliderReleased, this, &ColorPicker::endDynamicColorChange);
    connect(m_currentColorController, &Internal::ColorController::hsvHueChanged, m_valueSlider, &GradientSlider::updateGradient);
    connect(m_currentColorController, &Internal::ColorController::hsvSaturationChanged, m_valueSlider, &GradientSlider::updateGradient);
    connect(m_currentColorController, &Internal::ColorController::valueChanged, this, [this](qreal value) {
        const QSignalBlocker b(m_valueSlider);
        m_valueSlider->setValue(qRound(value*255.0));
    });

    // eyedropper button

    m_eyedropperButton = new QToolButton(this);
    QIcon eyedropperIcon;
    eyedropperIcon.addPixmap(QPixmap(":/ColorPickerDialog/ColorGrid/eyedropper-normal.png"), QIcon::Normal);
    m_eyedropperButton->setIcon(eyedropperIcon);
    m_hsvPickerLayout->addWidget(m_eyedropperButton, 1, 0);
    m_eyedropper = new Eyedropper(this, m_eyedropperButton);
    connect(m_eyedropper, &Eyedropper::colorSelected, this, [this](const QColor& color) { setCurrentColor(FromQColor(color)); });
    connect(m_eyedropperButton, &QToolButton::pressed, m_eyedropper, &Eyedropper::show);

    // preview

    m_preview = new ColorPreview(this);
    connect(this, &ColorPicker::currentColorChanged, m_preview, &ColorPreview::setCurrentColor);
    connect(this, &ColorPicker::selectedColorChanged, m_preview, &ColorPreview::setSelectedColor);

    // toggle hue/saturation and saturation/value color grid button

    m_toggleHueGridButton = new QToolButton(this);
    QIcon toggleHueGridIcon;
    toggleHueGridIcon.addPixmap(QPixmap(":/ColorPickerDialog/ColorGrid/toggle-normal-on.png"), QIcon::Normal, QIcon::On);
    m_toggleHueGridButton->setIcon(toggleHueGridIcon);
    m_toggleHueGridButton->setCheckable(true);
    m_toggleHueGridButton->setChecked(true);
    m_hsvPickerLayout->addWidget(m_toggleHueGridButton, 1, 2);

    connect(m_toggleHueGridButton, &QAbstractButton::toggled, this, [this](bool checked) {
        setColorGridMode(checked ? ColorGrid::Mode::SaturationValue : ColorGrid::Mode::HueSaturation);
    });

    m_warning = new ColorWarning(ColorWarning::Mode::Warning, {}, tr("Selected color is the closest available"), this);
    connect(m_currentColorController, &Internal::ColorController::colorChanged, m_warning, [this](const AZ::Color& qt) { m_warning->setColor(qt); });

    // RGBA edit

    m_rgbHexLayout = new QHBoxLayout();
    containerLayout->addLayout(m_rgbHexLayout);

    m_rgbHexLayout->addStretch(1);

    m_rgbaEdit = new ColorRGBAEdit(this);
    m_rgbHexLayout->addWidget(m_rgbaEdit);

    connect(m_currentColorController, &Internal::ColorController::redChanged, m_rgbaEdit, &ColorRGBAEdit::setRed);
    connect(m_currentColorController, &Internal::ColorController::greenChanged, m_rgbaEdit, &ColorRGBAEdit::setGreen);
    connect(m_currentColorController, &Internal::ColorController::blueChanged, m_rgbaEdit, &ColorRGBAEdit::setBlue);
    connect(m_currentColorController, &Internal::ColorController::alphaChanged, m_rgbaEdit, &ColorRGBAEdit::setAlpha);
    connect(m_rgbaEdit, &ColorRGBAEdit::redChanged, m_currentColorController, &Internal::ColorController::setRed);
    connect(m_rgbaEdit, &ColorRGBAEdit::greenChanged, m_currentColorController, &Internal::ColorController::setGreen);
    connect(m_rgbaEdit, &ColorRGBAEdit::blueChanged, m_currentColorController, &Internal::ColorController::setBlue);
    connect(m_rgbaEdit, &ColorRGBAEdit::alphaChanged, m_currentColorController, &Internal::ColorController::setAlpha);
    connect(m_rgbaEdit, &ColorRGBAEdit::valueChangeBegan, this, &ColorPicker::beginDynamicColorChange);
    connect(m_rgbaEdit, &ColorRGBAEdit::valueChangeEnded, this, &ColorPicker::endDynamicColorChange);

    // hex

    m_hexEdit = new ColorHexEdit(this);
    m_rgbHexLayout->addWidget(m_hexEdit);

    m_rgbHexLayout->addStretch(1);

    connect(m_currentColorController, &Internal::ColorController::redChanged, m_hexEdit, &ColorHexEdit::setRed);
    connect(m_currentColorController, &Internal::ColorController::greenChanged, m_hexEdit, &ColorHexEdit::setGreen);
    connect(m_currentColorController, &Internal::ColorController::blueChanged, m_hexEdit, &ColorHexEdit::setBlue);
    connect(m_hexEdit, &ColorHexEdit::redChanged, m_currentColorController, &Internal::ColorController::setRed);
    connect(m_hexEdit, &ColorHexEdit::greenChanged, m_currentColorController, &Internal::ColorController::setGreen);
    connect(m_hexEdit, &ColorHexEdit::blueChanged, m_currentColorController, &Internal::ColorController::setBlue);
    connect(m_hexEdit, &ColorHexEdit::valueChangeBegan, this, &ColorPicker::beginDynamicColorChange);
    connect(m_hexEdit, &ColorHexEdit::valueChangeEnded, this, &ColorPicker::endDynamicColorChange);

    // sliders separator

    m_slidersSeparator = MakeSeparator(this);
    containerLayout->addWidget(m_slidersSeparator);

    // HSL sliders

    m_hslSliders = new HSLSliders(this);
    containerLayout->addWidget(m_hslSliders);

    connect(m_currentColorController, &Internal::ColorController::hslHueChanged, m_hslSliders, &HSLSliders::setHue);
    connect(m_currentColorController, &Internal::ColorController::hslSaturationChanged, m_hslSliders, &HSLSliders::setSaturation);
    connect(m_currentColorController, &Internal::ColorController::lightnessChanged, m_hslSliders, &HSLSliders::setLightness);
    connect(m_hslSliders, &HSLSliders::hueChanged, m_currentColorController, &Internal::ColorController::setHslHue);
    connect(m_hslSliders, &HSLSliders::saturationChanged, m_currentColorController, &Internal::ColorController::setHslSaturation);
    connect(m_hslSliders, &HSLSliders::lightnessChanged, m_currentColorController, &Internal::ColorController::setLightness);
    connect(m_hslSliders, &HSLSliders::valueChangeBegan, this, &ColorPicker::beginDynamicColorChange);
    connect(m_hslSliders, &HSLSliders::valueChangeEnded, this, &ColorPicker::endDynamicColorChange);

    // HSV sliders

    m_hsvSliders = new HSVSliders(this);
    m_hsvSliders->hide();
    containerLayout->addWidget(m_hsvSliders);

    connect(m_currentColorController, &Internal::ColorController::hsvHueChanged, m_hsvSliders, &HSVSliders::setHue);
    connect(m_currentColorController, &Internal::ColorController::hsvSaturationChanged, m_hsvSliders, &HSVSliders::setSaturation);
    connect(m_currentColorController, &Internal::ColorController::valueChanged, m_hsvSliders, &HSVSliders::setValue);
    connect(m_hsvSliders, &HSVSliders::hueChanged, m_currentColorController, &Internal::ColorController::setHsvHue);
    connect(m_hsvSliders, &HSVSliders::saturationChanged, m_currentColorController, &Internal::ColorController::setHsvSaturation);
    connect(m_hsvSliders, &HSVSliders::valueChanged, m_currentColorController, &Internal::ColorController::setValue);
    connect(m_hsvSliders, &HSVSliders::valueChangeBegan, this, &ColorPicker::beginDynamicColorChange);
    connect(m_hsvSliders, &HSVSliders::valueChangeEnded, this, &ColorPicker::endDynamicColorChange);

    // RGB sliders

    m_rgbSliders = new RGBSliders(this);
    m_rgbSliders->hide();
    containerLayout->addWidget(m_rgbSliders);

    connect(m_currentColorController, &Internal::ColorController::redChanged, m_rgbSliders, &RGBSliders::setRed);
    connect(m_currentColorController, &Internal::ColorController::greenChanged, m_rgbSliders, &RGBSliders::setGreen);
    connect(m_currentColorController, &Internal::ColorController::blueChanged, m_rgbSliders, &RGBSliders::setBlue);
    connect(m_rgbSliders, &RGBSliders::redChanged, m_currentColorController, &Internal::ColorController::setRed);
    connect(m_rgbSliders, &RGBSliders::greenChanged, m_currentColorController, &Internal::ColorController::setGreen);
    connect(m_rgbSliders, &RGBSliders::blueChanged, m_currentColorController, &Internal::ColorController::setBlue);
    connect(m_rgbSliders, &RGBSliders::valueChangeBegan, this, &ColorPicker::beginDynamicColorChange);
    connect(m_rgbSliders, &RGBSliders::valueChangeEnded, this, &ColorPicker::endDynamicColorChange);

    // quick palette

    m_quickPaletteSeparator = MakeSeparator(this);
    containerLayout->addWidget(m_quickPaletteSeparator);


    m_paletteView = new PaletteView(&m_quickPalette, m_currentColorController, m_undoStack, this);

    m_paletteView->addDefaultRGBColors();
    connect(m_paletteView, &PaletteView::selectedColorsChanged, this, [this](const QVector<AZ::Color>& selectedColors) {
        if (selectedColors.size() == 1)
        {
            m_currentColorController->setColor(selectedColors[0]);
        }
    });
    connect(m_preview, &ColorPreview::colorSelected, m_paletteView, &PaletteView::tryAddColor);

    m_quickPaletteLayout = new QHBoxLayout();
    containerLayout->addLayout(m_quickPaletteLayout);

    // color libraries

    m_paletteCardSeparator = MakeSeparator(this);
    m_paletteCardSeparator->hide();
    containerLayout->addWidget(m_paletteCardSeparator);

    m_paletteCardCollection = new PaletteCardCollection(m_currentColorController, m_undoStack, this);
    m_paletteCardCollection->hide();
    connect(m_paletteCardCollection, &PaletteCardCollection::removePaletteClicked, this, &ColorPicker::removePaletteCardRequested);
    connect(m_paletteCardCollection, &PaletteCardCollection::savePaletteClicked, this, &ColorPicker::savePalette);
    connect(m_paletteCardCollection, &PaletteCardCollection::paletteCountChanged, this, [this] {
        const bool visible = m_paletteCardCollection->count() > 0;
        m_paletteCardSeparator->setVisible(visible);
        m_paletteCardCollection->setVisible(visible);
    });
    containerLayout->addWidget(m_paletteCardCollection);

    // gamma

    m_gammaEditSeparator = MakeSeparator(this);
    containerLayout->addWidget(m_gammaEditSeparator);

    m_gammaEdit = new GammaEdit(this);
    connect(m_gammaEdit, &GammaEdit::gammaChanged, m_preview, &ColorPreview::setGamma);
    connect(m_gammaEdit, &GammaEdit::toggled, m_preview, &ColorPreview::setGammaEnabled);
    connect(m_gammaEdit, &GammaEdit::gammaChanged, m_paletteView, &PaletteView::setGamma);
    connect(m_gammaEdit, &GammaEdit::toggled, m_paletteView, &PaletteView::setGammaEnabled);
    connect(m_gammaEdit, &GammaEdit::gammaChanged, m_paletteCardCollection, &PaletteCardCollection::setGamma);
    connect(m_gammaEdit, &GammaEdit::toggled, m_paletteCardCollection, &PaletteCardCollection::setGammaEnabled);
    containerLayout->addWidget(m_gammaEdit);

    // buttons

    mainLayout->addWidget(MakeSeparator(this));

    m_dialogButtonBox = new QDialogButtonBox(QDialogButtonBox::StandardButtons(QDialogButtonBox::Ok | QDialogButtonBox::Cancel), this);
    mainLayout->addWidget(m_dialogButtonBox);

    connect(m_dialogButtonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(m_dialogButtonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    initContextMenu(configuration);

    setContextMenuPolicy(Qt::CustomContextMenu);
    connect(this, &QWidget::customContextMenuRequested, this, [this](const QPoint& point) {
        m_menu->exec(mapToGlobal(point));
    });

    m_undoAction = new QAction(tr("Undo"), this);
    m_undoAction->setShortcut(QKeySequence::Undo);
    m_undoAction->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    connect(m_undoAction, &QAction::triggered, m_undoStack, &QUndoStack::undo);
    addAction(m_undoAction);

    m_redoAction = new QAction(tr("Redo"), this);
    m_redoAction->setShortcut(QKeySequence::Redo);
    m_redoAction->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    connect(m_redoAction, &QAction::triggered, m_undoStack, &QUndoStack::redo);
    addAction(m_redoAction);

    setConfiguration(configuration);

    readSettings();
}

void ColorPicker::setConfiguration(Configuration configuration)
{
    switch (configuration)
    {
    case Configuration::A:
        applyConfigurationA();
        break;
    case Configuration::B:
        applyConfigurationB();
        break;
    case Configuration::C:
        applyConfigurationC();
        break;
    }
}

void ColorPicker::applyConfigurationA()
{
    m_alphaSlider->show();
    m_toggleHueGridButton->show();

    m_rgbaEdit->setMode(ColorRGBAEdit::Mode::Rgba);

    RemoveAllWidgets(m_hsvPickerLayout);
    m_hsvPickerLayout->addWidget(m_alphaSlider, 0, 0);
    m_hsvPickerLayout->addWidget(m_colorGrid, 0, 1);
    m_hsvPickerLayout->addWidget(m_hueSlider, 0, 2);
    m_hsvPickerLayout->addWidget(m_valueSlider, 0, 2);

    m_hsvPickerLayout->addWidget(m_eyedropperButton, 1, 0);
    m_hsvPickerLayout->addWidget(m_preview, 1, 1);
    m_hsvPickerLayout->addWidget(m_toggleHueGridButton, 1, 2);
    m_hsvPickerLayout->addWidget(m_warning, 2, 0, 1, 3);

    setColorGridMode(ColorGrid::Mode::SaturationValue);

    m_hslSliders->setMode(HSLSliders::Mode::Hsl);
    m_hslSliders->show();
    m_hsvSliders->hide();
    m_rgbSliders->hide();
    m_gammaEditSeparator->show();
    m_gammaEdit->show();
}

void ColorPicker::applyConfigurationB()
{
    m_alphaSlider->hide();
    m_toggleHueGridButton->show();

    m_rgbaEdit->setMode(ColorRGBAEdit::Mode::Rgba);

    RemoveAllWidgets(m_hsvPickerLayout);
    m_hsvPickerLayout->addWidget(m_colorGrid, 0, 0, 1, 2);
    m_hsvPickerLayout->addWidget(m_hueSlider, 0, 2);
    m_hsvPickerLayout->addWidget(m_valueSlider, 0, 2);

    m_hsvPickerLayout->addWidget(m_eyedropperButton, 1, 0);
    m_hsvPickerLayout->addWidget(m_preview, 1, 1);
    m_hsvPickerLayout->addWidget(m_toggleHueGridButton, 1, 2);
    m_hsvPickerLayout->addWidget(m_warning, 2, 0, 1, 3);

    setColorGridMode(ColorGrid::Mode::SaturationValue);

    m_hslSliders->hide();
    m_hsvSliders->hide();
    m_rgbSliders->show();
    m_gammaEditSeparator->hide();
    m_gammaEdit->hide();
}

void ColorPicker::applyConfigurationC()
{
    m_alphaSlider->hide();
    m_hueSlider->hide();
    m_valueSlider->hide();
    m_toggleHueGridButton->hide();

    m_rgbaEdit->setMode(ColorRGBAEdit::Mode::Rgb);
    m_rgbaEdit->setReadOnly(true);

    RemoveAllWidgets(m_hsvPickerLayout);
    m_hsvPickerLayout->addWidget(m_colorGrid, 0, 0, 1, 2);

    m_hsvPickerLayout->addWidget(m_eyedropperButton, 1, 0);
    m_hsvPickerLayout->addWidget(m_preview, 1, 1);
    m_hsvPickerLayout->addWidget(m_warning, 2, 0, 1, 3);

    m_colorGrid->setMode(ColorGrid::Mode::HueSaturation);

    m_hslSliders->setMode(HSLSliders::Mode::Hs);
    m_hslSliders->show();
    m_hsvSliders->hide();
    m_rgbSliders->hide();
    m_gammaEditSeparator->show();
    m_gammaEdit->show();
}

ColorPicker::~ColorPicker()
{
}

AZ::Color ColorPicker::currentColor() const
{
    return m_currentColorController->color();
}

void ColorPicker::setCurrentColor(const AZ::Color& color)
{
    if (m_previousColor.IsClose(INVALID_COLOR))
    {
        m_previousColor = color;
    }
    m_currentColorController->setColor(color);
}

AZ::Color ColorPicker::selectedColor() const
{
    return m_selectedColor;
}

void ColorPicker::setSelectedColor(const AZ::Color& color)
{
    if (AreClose(color, m_selectedColor))
    {
        return;
    }
    m_selectedColor = color;
    emit selectedColorChanged(m_selectedColor);
}

AZ::Color ColorPicker::getColor(ColorPicker::Configuration configuration, const AZ::Color& initial, const QString& title, const QStringList& palettePaths, QWidget* parent)
{
    ColorPicker dialog(configuration, parent);
    dialog.setWindowTitle(title);
    dialog.setCurrentColor(initial);
    dialog.setSelectedColor(initial);
    for (const QString& path : palettePaths)
    {
        dialog.importPalettesFromFolder(path);
    }

    if (dialog.exec() == QDialog::Accepted)
    {
        return dialog.currentColor();
    }

    return initial;
}

void ColorPicker::closeEvent(QCloseEvent* e)
{
    writeSettings();
    QDialog::closeEvent(e);
}

void ColorPicker::setColorGridMode(ColorGrid::Mode mode)
{
    m_colorGrid->setMode(mode);

    if (m_hueSlider->layout() && !m_valueSlider->layout())
    {
        // Configuration "C" has neither of these so we shouldn't change their visibility
        return;
    }

    if (mode == ColorGrid::Mode::SaturationValue)
    {
        m_hueSlider->show();
        m_valueSlider->hide();
    }
    else
    {
        m_hueSlider->hide();
        m_valueSlider->show();
    }
}

void ColorPicker::initContextMenu(ColorPicker::Configuration configuration)
{
    m_menu = new QMenu(this);

    m_showLinearValuesAction = m_menu->addAction(tr("Show linear values"));
    m_showLinearValuesAction->setCheckable(true);
    m_showLinearValuesAction->setChecked(true);
    connect(m_showLinearValuesAction, &QAction::toggled, m_rgbaEdit, &QWidget::setVisible);

    m_showHexValueAction = m_menu->addAction(tr("Show hex value"));
    m_showHexValueAction->setCheckable(true);
    m_showHexValueAction->setChecked(true);
    connect(m_showHexValueAction, &QAction::toggled, m_hexEdit, &QWidget::setVisible);

    m_menu->addSeparator();

    if (configuration != Configuration::C)
    {
        auto sliderGroup = new QActionGroup(this);
        sliderGroup->setExclusive(false);

        m_showHSLSlidersAction = m_menu->addAction(tr("Show HSL sliders"));
        m_showHSLSlidersAction->setCheckable(true);
        sliderGroup->addAction(m_showHSLSlidersAction);
        connect(m_showHSLSlidersAction, &QAction::toggled, m_hslSliders, &QWidget::setVisible);
        connect(m_showHSLSlidersAction, &QAction::toggled, this, [this](bool checked) {
            m_slidersSeparator->setVisible(checked || m_hsvSliders->isVisible() || m_rgbSliders->isVisible());
        });

        m_showHSVSlidersAction = m_menu->addAction(tr("Show HSV sliders"));
        m_showHSVSlidersAction->setCheckable(true);
        sliderGroup->addAction(m_showHSVSlidersAction);
        connect(m_showHSVSlidersAction, &QAction::toggled, m_hsvSliders, &QWidget::setVisible);
        connect(m_showHSVSlidersAction, &QAction::toggled, this, [this](bool checked) {
            m_slidersSeparator->setVisible(checked || m_hslSliders->isVisible() || m_rgbSliders->isVisible());
        });

        m_showRGBSlidersAction = m_menu->addAction(tr("Show RGB sliders"));
        m_showRGBSlidersAction->setCheckable(true);
        sliderGroup->addAction(m_showRGBSlidersAction);
        connect(m_showRGBSlidersAction, &QAction::toggled, m_rgbSliders, &QWidget::setVisible);
        connect(m_showRGBSlidersAction, &QAction::toggled, this, [this](bool checked) {
            m_slidersSeparator->setVisible(checked || m_hslSliders->isVisible() || m_hsvSliders->isVisible());
        });

        if (configuration == Configuration::A)
        {
            m_showHSLSlidersAction->setChecked(true);
        }
        else
        {
            m_showRGBSlidersAction->setChecked(true);
        }
    }
    else
    {
        m_showHSLSlidersAction = m_menu->addAction(tr("Show HS sliders"));
        m_showHSLSlidersAction->setCheckable(true);
        m_showHSLSlidersAction->setChecked(true);
        connect(m_showHSLSlidersAction, &QAction::toggled, m_hslSliders, &QWidget::setVisible);
        connect(m_showHSLSlidersAction, &QAction::toggled, m_slidersSeparator, &QWidget::setVisible);
    }

    m_menu->addSeparator();

    m_swatchSizeGroup = new QActionGroup(this);

    auto smallSwatches = m_menu->addAction(tr("Small swatches"));
    smallSwatches->setCheckable(true);
    connect(smallSwatches, &QAction::toggled, this, [this](bool checked) {
        if (checked)
        {
            const QSize size{ 16, 16 };
            m_paletteView->setItemSize(size);
            m_paletteCardCollection->setSwatchSize(size);
        }
    });
    m_swatchSizeGroup->addAction(smallSwatches);

    auto mediumSwatches = m_menu->addAction(tr("Medium swatches"));
    mediumSwatches->setCheckable(true);
    connect(mediumSwatches, &QAction::toggled, this, [this](bool checked) {
        if (checked)
        {
            const QSize size{ 24, 24 };
            m_paletteView->setItemSize(size);
            m_paletteCardCollection->setSwatchSize(size);
        }
    });
    m_swatchSizeGroup->addAction(mediumSwatches);

    auto largeSwatches = m_menu->addAction(tr("Large swatches"));
    largeSwatches->setCheckable(true);
    connect(largeSwatches, &QAction::toggled, this, [this](bool checked) {
        if (checked)
        {
            const QSize size{ 32, 32 };
            m_paletteView->setItemSize(size);
            m_paletteCardCollection->setSwatchSize(size);
        }
    });
    m_swatchSizeGroup->addAction(largeSwatches);

    auto hideSwatches = m_menu->addAction(tr("Hide swatches"));
    hideSwatches->setCheckable(true);
    connect(hideSwatches, &QAction::toggled, this, [this](bool checked) {
        m_paletteView->setVisible(!checked);
        m_quickPaletteSeparator->setVisible(!checked);
    });
    m_swatchSizeGroup->addAction(hideSwatches);

    smallSwatches->setChecked(true);

    m_menu->addSeparator();

    m_menu->addAction(tr("Import color palette..."), this, &ColorPicker::importPalette);
    m_menu->addAction(tr("New color palette"), this, &ColorPicker::newPalette);

    if (configuration != Configuration::B)
    {
        m_menu->addSeparator();

        m_showGammaAction = m_menu->addAction(tr("Show gamma"));
        m_showGammaAction->setCheckable(true);
        m_showGammaAction->setChecked(true);
        connect(m_showGammaAction, &QAction::toggled, m_gammaEdit, &QWidget::setVisible);
        connect(m_showGammaAction, &QAction::toggled, m_gammaEditSeparator, &QWidget::setVisible);
    }
}

void ColorPicker::importPalettesFromFolder(const QString& path)
{
    if (path.isEmpty())
    {
        return;
    }

    QDirIterator it(path, QStringList() << "*.pal", QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext())
    {
        importPaletteFromFile(it.next());
    }
}

bool ColorPicker::importPaletteFromFile(const QString& fileName)
{
    if (fileName.isEmpty())
    {
        return false;
    }

    auto palette = QSharedPointer<Palette>::create();
    if (!palette->load(fileName))
    {
        return false;
    }

    addPalette(palette, fileName, QFileInfo(fileName).baseName());
    return true;
}

void ColorPicker::importPalette()
{
    QString fileName = QFileDialog::getOpenFileName(this, tr("Import Color Palette"), {}, tr("Color Palettes (*.pal)"));
    if (!fileName.isEmpty() && !importPaletteFromFile(fileName))
    {
        QMessageBox::critical(this, tr("Color Palette Import Error"), tr("Failed to import \"%1\"").arg(fileName));
    }
}

void ColorPicker::newPalette()
{
    addPalette(QSharedPointer<Palette>::create(), {}, tr("Untitled"));
}

void ColorPicker::removePaletteCardRequested(QSharedPointer<PaletteCard> card)
{
    if (card->modified())
    {
        int result = QMessageBox::question(this, tr("Color Picker"), tr("There are unsaved changes to your palette. Are you sure you want to close?"));
        if (result != QMessageBox::Yes)
        {
            return;
        }
    }

    PaletteRemovedCommand* removed = new PaletteRemovedCommand(this, card, m_colorLibraries[card]);
    m_undoStack->push(removed);
}

void ColorPicker::addPalette(QSharedPointer<Palette> palette, const QString& fileName, const QString& title)
{
    QSharedPointer<PaletteCard> card = m_paletteCardCollection->makeCard(palette, title);
    PaletteAddedCommand* added = new PaletteAddedCommand(this, card, ColorLibrary{ fileName, palette });
    m_undoStack->push(added);
}

void ColorPicker::addPaletteCard(QSharedPointer<PaletteCard> card, ColorLibrary colorLibrary)
{        
    Palette loader;
    QString fileName = QStringLiteral("%1.pal").arg(card->title());
    bool loaded = loader.load(fileName);

    if (!loaded || (loaded && loader.colors() != card->palette()->colors()))
    {
        // If we loaded the data and it was different to what we know OR
        // If we can't load the palette file, then mark the palette as modified
        // Either way, we only mark it as modified if it's non-empty

        card->setModified(!card->palette()->colors().empty());
    }

    m_colorLibraries[card] = colorLibrary;
    m_paletteCardCollection->addCard(card);
}

void ColorPicker::removePaletteCard(QSharedPointer<PaletteCard> card)
{
    if (!m_paletteCardCollection->containsCard(card) || !m_colorLibraries.contains(card))
    {
        return;
    }
    
    m_colorLibraries.remove(card);
    m_paletteCardCollection->removeCard(card);
}

void AzQtComponents::ColorPicker::beginDynamicColorChange()
{
    m_dynamicColorChange = true;
}

void AzQtComponents::ColorPicker::endDynamicColorChange()
{
    AZ::Color newColor = m_currentColorController->color();
    if (!m_previousColor.IsClose(newColor))
    {
        auto command = new CurrentColorChangedCommand(this, m_previousColor, newColor);
        m_undoStack->push(command);
    }
    m_dynamicColorChange = false;
}

void ColorPicker::savePalette(QSharedPointer<PaletteCard> card)
{
    if (!m_paletteCardCollection->containsCard(card) || !m_colorLibraries.contains(card))
    {
        return;
    }

    QString dir = m_colorLibraries[card].fileName;
    QString fileName = QFileDialog::getSaveFileName(this, tr("Export Color Palette"), dir, tr("Color Palettes (*.pal)"));
    if (fileName.isEmpty())
    {
        return;
    }

    if (!card->palette()->save(fileName))
    {
        QMessageBox::critical(this, tr("Color Palette Export Error"), tr("Failed to export \"%1\"").arg(fileName));
        return;
    }

    m_colorLibraries[card].fileName = fileName;

    card->setTitle(QFileInfo(fileName).baseName());
    card->setModified(false);

    writeSettings();
}

int ColorPicker::colorLibraryIndex(const Palette* palette) const
{
    auto it = std::find_if(m_colorLibraries.begin(), m_colorLibraries.end(),
                           [palette](const ColorLibrary& entry) { return entry.palette.data() == palette; });
    return it == m_colorLibraries.end() ? -1 : std::distance(m_colorLibraries.begin(), it);
}

void ColorPicker::readSettings()
{
    QSettings settings;
    settings.beginGroup(g_colorPickerSection);

    QString sectionName = ConfigurationName(m_configuration);
    if (!settings.childGroups().contains(sectionName))
    {
        return;
    }
    settings.beginGroup(sectionName);

    bool showLinearValues = settings.value(g_showLinearValuesKey, true).toBool();
    m_showLinearValuesAction->setChecked(showLinearValues);

    bool showHexValues = settings.value(g_showHexValuesKey, true).toBool();
    m_showHexValueAction->setChecked(showHexValues);

    if (m_configuration != Configuration::C)
    {
        bool showHSLSliders = settings.value(g_showHSLSlidersKey, m_configuration == Configuration::A).toBool();
        m_showHSLSlidersAction->setChecked(showHSLSliders);

        bool showHSVSliders = settings.value(g_showHSVSlidersKey, false).toBool();
        m_showHSVSlidersAction->setChecked(showHSVSliders);

        bool showRGBSliders = settings.value(g_showRGBSlidersKey, m_configuration == Configuration::B).toBool();
        m_showRGBSlidersAction->setChecked(showRGBSliders);
    }
    else
    {
        bool showHSSliders = settings.value(g_showHSSlidersKey, true).toBool();
        m_showHSLSlidersAction->setChecked(showHSSliders);
    }

    if (settings.contains(g_quickPaletteKey))
    {
        m_quickPalette = settings.value(g_quickPaletteKey).value<Palette>();
    }

    QStringList colorLibraries = settings.value(g_colorLibrariesKey).toStringList();
    QStringList missingLibraries;
    for (const auto& fileName : colorLibraries)
    {
        if (!importPaletteFromFile(fileName))
        {
            missingLibraries.append(QDir::toNativeSeparators(fileName));
        }
    }

    if (!missingLibraries.empty())
    {
        QMessageBox::warning(this, tr("Failed to load color libraries"), tr("The following color libraries could not be located on disk:\n%1\n"
            "They will be removed from your saved settings. Please re-import them again if you can locate them.").arg(missingLibraries.join('\n')));
    }

    int swatchSize = settings.value(g_swatchSizeKey, 3).toInt();

    auto swatchSizeActions = m_swatchSizeGroup->actions();
    if (swatchSize >= 0 && swatchSize < swatchSizeActions.count())
    {
        swatchSizeActions[swatchSize]->setChecked(true);
    }

    if (m_configuration != Configuration::B)
    {
        bool showGammaEdit = settings.value(g_showGammaKey, true).toBool();
        m_showGammaAction->setChecked(showGammaEdit);

        bool gammaChecked = settings.value(g_gammaCheckedKey, false).toBool();
        m_gammaEdit->setChecked(gammaChecked);

        double gammaValue = settings.value(g_gammaValueKey, 1.0).toDouble();
        m_gammaEdit->setGamma(gammaValue);
    }
}

void ColorPicker::writeSettings() const
{
    QSettings settings;
    settings.beginGroup(QStringLiteral("%1/%2").arg(g_colorPickerSection, ConfigurationName(m_configuration)));

    settings.setValue(g_showLinearValuesKey, m_rgbaEdit->isVisible());
    settings.setValue(g_showHexValuesKey, m_hexEdit->isVisible());

    if (m_configuration != Configuration::C)
    {
        settings.setValue(g_showHSLSlidersKey, m_hslSliders->isVisible());
        settings.setValue(g_showHSVSlidersKey, m_hsvSliders->isVisible());
        settings.setValue(g_showRGBSlidersKey, m_rgbSliders->isVisible());
    }
    else
    {
        settings.setValue(g_showHSSlidersKey, m_hslSliders->isVisible());
    }

    settings.setValue(g_quickPaletteKey, QVariant::fromValue(m_quickPalette));

    QStringList colorLibraries;
    for (const auto& item : m_colorLibraries)
    {
        if (!item.fileName.isEmpty())
        {
            colorLibraries.append(item.fileName);
        }
    }
    settings.setValue(g_colorLibrariesKey, colorLibraries);

    auto swatchSizeActions = m_swatchSizeGroup->actions();
    auto it = std::find_if(swatchSizeActions.begin(), swatchSizeActions.end(),
                           [](QAction* action) { return action->isChecked(); });
    if (it != swatchSizeActions.end())
    {
        int swatchSize = std::distance(swatchSizeActions.begin(), it);
        settings.setValue(g_swatchSizeKey, swatchSize);
    }

    if (m_configuration != Configuration::B)
    {
        settings.setValue(g_showGammaKey, m_gammaEdit->isVisible());
        settings.setValue(g_gammaCheckedKey, m_gammaEdit->isChecked());
        settings.setValue(g_gammaValueKey, m_gammaEdit->gamma());
    }

    settings.endGroup();
    settings.sync();
}

bool ColorPicker::eventFilter(QObject* o, QEvent* e)
{
    if (o == m_containerWidget && e->type() == QEvent::Resize)
    {
        const int f = 2 * m_scrollArea->frameWidth();
        QSize sz{f, f};

        sz += static_cast<QResizeEvent*>(e)->size();

        if (sz.height() > m_config.maximumContentHeight)
        {
            sz.setHeight(m_config.maximumContentHeight);
            sz.setWidth(sz.width() + m_scrollArea->verticalScrollBar()->sizeHint().width());
        }

        m_scrollArea->setMinimumSize(sz);
    }

    return StyledDialog::eventFilter(o, e);
}

} // namespace AzQtComponents
