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

#include <AzQtComponents/Components/StyleManager.h>
#include <QTextStream>
#include <QApplication>
#include <QPalette>
#include <QDir>
#include <QString>
#include <QFile>
#include <QFontDatabase>
#include <QStyleFactory>
#include <QPointer>
#include <QStyle>
#include <QWidget>
#include <QDebug>
#include <QtWidgets/private/qstylesheetstyle_p.h>

#include <AzQtComponents/Components/StylesheetPreprocessor.h>
#include <AzQtComponents/Utilities/QtPluginPaths.h>
#include <AzQtComponents/Components/EditorProxyStyle.h>
#include <AzQtComponents/Components/StyleSheetCache.h>
#include <AzQtComponents/Components/Style.h>
#include <AzQtComponents/Components/TitleBarOverdrawHandler.h>
#include <AzQtComponents/Components/AutoCustomWindowDecorations.h>

namespace AzQtComponents
{
// UI 1.0
#define STYLE_SHEET_VARIABLES_PATH_DARK "Code/Sandbox/Editor/Style/EditorStylesheetVariables_Dark.json" // file path on disk
#define STYLE_SHEET_VARIABLES_PATH_DARK_ALT ":Editor/Style/EditorStylesheetVariables_Dark.json" // in our qrc file as a fallback
#define STYLE_SHEET_PATH "Code/Sandbox/Editor/Style/NewEditorStylesheet.qss" // file path on disk
#define STYLE_SHEET_PATH_ALT ":Editor/Style/NewEditorStylesheet.qss" //in our qrc file as a fallback
#define FONT_PATH "Editor/Fonts"

    static void RefreshUI_1_0_Style(QApplication* application, QStyleSheetStyle* styleSheetStyle10, StylesheetPreprocessor* stylesheetPreprocessor);

    // UI 2.0
    const QString g_styleSheetRelativePath = QStringLiteral("Code/Framework/AzQtComponents/AzQtComponents/Components/Widgets");
    const QString g_styleSheetResourcePath = QStringLiteral(":AzQtComponents/Widgets");
    const QString g_globalStyleSheetName = QStringLiteral("BaseStyleSheet.qss");
    const QString g_searchPathPrefix = QStringLiteral("AzQtComponentWidgets");

    StyleManager* StyleManager::s_instance = nullptr;

    static QString FindPath(QApplication* application, const QString& path)
    {
        QDir rootDir(FindEngineRootDir(application));

        // for now, assume the executable is in the dev/Bin64 directory. We'll find our resources relative to the dev directory
        return rootDir.absoluteFilePath(path);
    }

    static QStyle* createBaseStyle()
    {
        return QStyleFactory::create("Fusion");
    }

    void StyleManager::addSearchPaths(const QString& searchPrefix, const QString& pathOnDisk, const QString& qrcPrefix)
    {
        if (!s_instance)
        {
            qFatal("StyleManager::addSearchPaths called before instance was created");
            return;
        }

        s_instance->m_stylesheetCache->addSearchPaths(searchPrefix, pathOnDisk, qrcPrefix);
    }

    bool StyleManager::setStyleSheet(QWidget* widget, QString styleFileName)
    {
        if (!s_instance)
        {
            qFatal("StyleManager::setStyleSheet called before instance was created");
            return false;
        }

        if (!widget)
        {
            qFatal("StyleManager::setStyleSheet called with null widget pointer");
            return false;
        }

        if (!styleFileName.endsWith(StyleSheetCache::styleSheetExtension()))
        {
            styleFileName.append(StyleSheetCache::styleSheetExtension());
        }

        const auto styleSheet = s_instance->m_stylesheetCache->loadStyleSheet(styleFileName);
        if (styleSheet.isEmpty())
        {
            return false;
        }

        s_instance->m_widgetToStyleSheetMap.insert(widget, styleFileName);

        connect(widget, &QObject::destroyed, s_instance, &StyleManager::stopTrackingWidget, Qt::UniqueConnection);

        // Only apply widget specific style sheets to UI 2.0
        if (!s_instance->m_useUI10)
        {
            widget->setStyleSheet(styleSheet);
        }

        return true;
    }

    QStyleSheetStyle* StyleManager::styleSheetStyle(const QWidget* widget)
    {
        Q_UNUSED(widget);
        // widget is currently unused, but would be required if Qt::AA_ManualStyleSheetStyle was
        // not set.

        if (!s_instance)
        {
            qFatal("StyleManager::styleSheetStyle called before instance was created");
            return nullptr;
        }

        if (!qApp->testAttribute(Qt::AA_ManualStyleSheetStyle))
        {
            qFatal("StyleManager::styleSheetStyle has not been implemented for automatically created QStyleSheetStyles");
            return nullptr;
        }

        return s_instance->m_useUI10 ? s_instance->m_styleSheetStyle10 : s_instance->m_styleSheetStyle20;
    }

    QStyle *StyleManager::baseStyle(const QWidget *widget)
    {
        const auto sss = styleSheetStyle(widget);
        return sss ? sss->baseStyle() : nullptr;
    }

    StyleManager::StyleManager(QObject* parent)
        : QObject(parent)
        , m_stylesheetPreprocessor(new StylesheetPreprocessor(this))
        , m_stylesheetCache(new StyleSheetCache(this))
    {
        if (s_instance)
        {
            qFatal("A StyleManager already exists");
        }
    }

    StyleManager::~StyleManager()
    {
        delete m_stylesheetPreprocessor;
        s_instance = nullptr;

        if (m_style10)
        {
            delete m_style10.data();
            m_style10.clear();
            m_styleSheetStyle10 = nullptr;
        }

        if (m_style20)
        {
            delete m_style20.data();
            m_style20.clear();
            m_styleSheetStyle20 = nullptr;
        }
    }

    void StyleManager::initialize(QApplication* application, bool useUI10)
    {
        if (s_instance)
        {
            qFatal("StyleManager::Initialize called more than once");
            return;
        }
        s_instance = this;

        application->setAttribute(Qt::AA_ManualStyleSheetStyle, true);
        application->setAttribute(Qt::AA_PropagateStyleToChildren, true);
        connect(application, &QCoreApplication::aboutToQuit, this, &StyleManager::cleanupStyles);

        initializeSearchPaths(application);
        initializeFonts();

        m_titleBarOverdrawHandler = TitleBarOverdrawHandler::createHandler(application, this);

        // The window decoration wrappers require the titlebar overdraw handler
        // so we can't initialize the custom window decoration monitor until the
        // titlebar overdraw handler has been initialized.
        m_autoCustomWindowDecorations = new AutoCustomWindowDecorations(this);
        m_autoCustomWindowDecorations->setMode(AutoCustomWindowDecorations::Mode_AnyWindow);

        // For historical reasons style 1.0 is chained as: QStyleSheetStyle -> EditorProxyStyle -> native
        // We could chain it as style 2.0, but could cause regressions.

        m_styleSheetStyle10 = new QStyleSheetStyle(new EditorProxyStyle(createBaseStyle()));
        m_styleSheetStyle10->ref(); // To prevent QApplication from deleting it
        m_style10 = m_styleSheetStyle10;

        // Style 2.0 is chained as: Style -> QStyleSheetStyle -> native, meaning any CSS limitation can be tackled in Style.cpp
        m_styleSheetStyle20 = new QStyleSheetStyle(createBaseStyle());
        m_style20 = new Style(m_styleSheetStyle20);

        // m_isUI10 defaults to true, use the switchUIInternal to ensure we set up
        // the style correctly when StyleManager::initialize is called.
        switchUIInternal(application, useUI10);

        connect(m_stylesheetCache, &StyleSheetCache::styleSheetsChanged, this, [this, application]
        {
            refresh(application);
        });
    }

    void StyleManager::switchUI(QApplication* application, bool useUI10)
    {
        if (m_useUI10 == useUI10)
        {
            // only switch if necessary
            return;
        }

        switchUIInternal(application, useUI10);
    }

    void StyleManager::switchUIInternal(QApplication* application, bool useUI10)
    {
        const bool wasUI20 = !m_useUI10;
        m_useUI10 = useUI10;

        if (m_useUI10 && wasUI20)
        {
            // Iterate the widgets and clean the widget specific style sheets before we change the application style to UI 1.0
            const QString emptySheet;
            auto i = m_widgetToStyleSheetMap.constBegin();
            while (i != m_widgetToStyleSheetMap.constEnd())
            {
                i.key()->setStyleSheet(emptySheet);
                ++i;
            }
        }

        QStyle* newStyle = useUI10 ? m_style10.data() : m_style20.data();
        QStyle* oldStyle = application->style();
        application->setStyle(newStyle);

        if (oldStyle == m_style10)
        {
            // Increase the reference count because m_style10 is a QStyleSheetStyle and it gets
            // dereferenced when it is replaced in QApplication::setStyle. This stops m_style10 from
            // being deleted.
            m_styleSheetStyle10->ref();
        }
        if (newStyle == m_style20)
        {
            // Because m_style20 is not a QStyleSheetStyle, QApplication::setStyle attempts to
            // delete it if m_style20 is a child of the application, so reparent it to prevent this
            // from happening.
            newStyle->setParent(this);
        }
        refresh(application);
    }

    void StyleManager::cleanupStyles()
    {
        if (auto application = qobject_cast<QApplication*>(sender()))
        {
            application->setStyle(createBaseStyle());
        }
    }

    void StyleManager::stopTrackingWidget(QObject* object)
    {
        const auto widget = qobject_cast<QWidget* const>(object);
        if (!widget)
        {
            return;
        }

        m_widgetToStyleSheetMap.remove(widget);

        // Remove any old stylesheet
        widget->setStyleSheet(QString());
    }

    void StyleManager::initializeFonts()
    {
        // yes, the path specifier could've included OpenSans- and .ttf, but I
        // wanted anyone searching for OpenSans-Bold.ttf to find something so left it this way
        QString openSansPathSpecifier = QStringLiteral(":/AzQtFonts/Fonts/Open_Sans/%1");
        QFontDatabase::addApplicationFont(openSansPathSpecifier.arg("OpenSans-Bold.ttf"));
        QFontDatabase::addApplicationFont(openSansPathSpecifier.arg("OpenSans-BoldItalic.ttf"));
        QFontDatabase::addApplicationFont(openSansPathSpecifier.arg("OpenSans-ExtraBold.ttf"));
        QFontDatabase::addApplicationFont(openSansPathSpecifier.arg("OpenSans-ExtraBoldItalic.ttf"));
        QFontDatabase::addApplicationFont(openSansPathSpecifier.arg("OpenSans-Italic.ttf"));
        QFontDatabase::addApplicationFont(openSansPathSpecifier.arg("OpenSans-Light.ttf"));
        QFontDatabase::addApplicationFont(openSansPathSpecifier.arg("OpenSans-LightItalic.ttf"));
        QFontDatabase::addApplicationFont(openSansPathSpecifier.arg("OpenSans-Regular.ttf"));
        QFontDatabase::addApplicationFont(openSansPathSpecifier.arg("OpenSans-Semibold.ttf"));
        QFontDatabase::addApplicationFont(openSansPathSpecifier.arg("OpenSans-SemiboldItalic.ttf"));

        QString amazonEmberPathSpecifier = QStringLiteral(":/AzQtFonts/Fonts/AmazonEmber/%1");
        QFontDatabase::addApplicationFont(amazonEmberPathSpecifier.arg("amazon_ember_lt-webfont.ttf"));
        QFontDatabase::addApplicationFont(amazonEmberPathSpecifier.arg("amazon_ember_rg-webfont.ttf"));
    }

    void StyleManager::initializeSearchPaths(QApplication* application)
    {
        // now that QT is initialized, we can use its path manipulation functions to set the rest up:
        QString rootDir = FindEngineRootDir(application);

        if (!rootDir.isEmpty())
        {
            QDir appPath(rootDir);
            QApplication::addLibraryPath(appPath.absolutePath());

            // Set the StyleSheetCache fallback prefix
            const auto pathOnDisk = appPath.absoluteFilePath(g_styleSheetRelativePath);
            m_stylesheetCache->setFallbackSearchPaths(g_searchPathPrefix, pathOnDisk, g_styleSheetResourcePath);

            // add the expected editor paths
            // this allows you to refer to your assets relative, like
            // STYLESHEETIMAGES:something.txt
            // UI:blah/blah.png
            // EDITOR:blah/something.txt
            QDir::addSearchPath("STYLESHEETIMAGES", appPath.filePath("Editor/Styles/StyleSheetImages"));
            QDir::addSearchPath("UI", appPath.filePath("Editor/UI"));
            QDir::addSearchPath("EDITOR", appPath.filePath("Editor"));
        }
    }

    static bool WriteStylesheetForQtDesigner(const QString& processedStyle)
    {
        QString outputStylePath = QDir::cleanPath(QDir::homePath() + QDir::separator() + "lumberyard_stylesheet.qss");
        QFile outputStyleFile(outputStylePath);
        bool successfullyWroteStyleFile = false;
        if (outputStyleFile.open(QFile::WriteOnly))
        {
            QTextStream outStream(&outputStyleFile);
            outStream << processedStyle;
            outputStyleFile.close();
            successfullyWroteStyleFile = true;
        }

        return successfullyWroteStyleFile;
    }

    static void RefreshUI_1_0_Style(QApplication* application, QStyleSheetStyle* styleSheetStyle10, StylesheetPreprocessor* stylesheetPreprocessor)
    {
        QFile styleSheetVariablesFile;
        styleSheetVariablesFile.setFileName(FindPath(application, STYLE_SHEET_VARIABLES_PATH_DARK));

        if (styleSheetVariablesFile.open(QFile::ReadOnly))
        {
            stylesheetPreprocessor->ReadVariables(styleSheetVariablesFile.readAll());
        }
        else
        {
            styleSheetVariablesFile.setFileName(STYLE_SHEET_VARIABLES_PATH_DARK_ALT);

            if (styleSheetVariablesFile.open(QFile::ReadOnly))
            {
                stylesheetPreprocessor->ReadVariables(styleSheetVariablesFile.readAll());
            }
        }

        QFile styleSheetFile(FindPath(application, STYLE_SHEET_PATH));
        bool proceedLoading(false);
        if (styleSheetFile.open(QFile::ReadOnly))
        {
            proceedLoading = true;
        }
        else
        {
            styleSheetFile.setFileName(STYLE_SHEET_PATH_ALT);
            if (styleSheetFile.open(QFile::ReadOnly))
            {
                proceedLoading = true;
            }
        }

        if (proceedLoading)
        {
            QString processedStyle = stylesheetPreprocessor->ProcessStyleSheet(styleSheetFile.readAll());

            WriteStylesheetForQtDesigner(processedStyle);

            styleSheetStyle10->setGlobalSheet(processedStyle);
        }

        // fusion uses the current palette to create a uniform style across platforms
        // instead of using e.g. windows XP / vista glossy buttons
        QPalette newPal(application->palette());
        newPal.setColor(QPalette::Link, stylesheetPreprocessor->GetColorByName(QString("LinkColor")));
        application->setPalette(newPal);
    }

    void StyleManager::refresh(QApplication* application)
    {
        //////////////////////////////////////////////////////////////////////////
        // DEPRECATED UI 1.0
        if (m_useUI10)
        {
            RefreshUI_1_0_Style(application, m_styleSheetStyle10, m_stylesheetPreprocessor);

            return;
        }
        //////////////////////////////////////////////////////////////////////////

        const auto globalStyleSheet = m_stylesheetCache->loadStyleSheet(g_globalStyleSheetName);
        m_styleSheetStyle20->setGlobalSheet(globalStyleSheet);

        // Iterate widgets and update the stylesheet (the base style has already been set)
        auto i = m_widgetToStyleSheetMap.constBegin();
        while (i != m_widgetToStyleSheetMap.constEnd())
        {
            const auto styleSheet = m_stylesheetCache->loadStyleSheet(i.value());
            i.key()->setStyleSheet(styleSheet);
            ++i;
        }
    }

    const QColor& StyleManager::getColorByName(const QString& name)
    {
        return m_stylesheetPreprocessor->GetColorByName(name);
    }
} // namespace AzQtComponents

#include <Components/StyleManager.moc>

#if defined(AZ_QT_COMPONENTS_STATIC)
    // If we're statically compiling the lib, we need to include the compiled rcc resources
    // somewhere to ensure that the linker doesn't optimize the symbols out (with Visual Studio at least)
    // With dlls, there's no step to optimize out the symbols, so we don't need to do this.
    #include <Components/rcc_resources.h>
#endif // #if defined(AZ_QT_COMPONENTS_STATIC)


