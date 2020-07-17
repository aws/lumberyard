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

#include <AzCore/Debug/Trace.h>
#include <AzQtComponents/Components/StyleManager.h>
#include <QTextStream>
#include <QApplication>
#include <QPalette>
AZ_PUSH_DISABLE_WARNING(4251, "-Wunknown-warning-option") // 4251: 'QFileInfo::d_ptr': class 'QSharedDataPointer<QFileInfoPrivate>' needs to have dll-interface to be used by clients of class 'QFileInfo'
#include <QDir>
AZ_POP_DISABLE_WARNING
#include <QString>
#include <QFile>
#include <QFontDatabase>
#include <QStyleFactory>
#include <QPointer>
#include <QStyle>
#include <QWidget>
#include <QDebug>

#if !defined(AZ_PLATFORM_LINUX)
#include <QtWidgets/private/qstylesheetstyle_p.h>
#endif // !defined(AZ_PLATFORM_LINUX)

#include <AzQtComponents/Components/StylesheetPreprocessor.h>
#include <AzQtComponents/Utilities/QtPluginPaths.h>
#include <AzQtComponents/Components/EditorProxyStyle.h>
#include <AzQtComponents/Components/StyleSheetCache.h>
#include <AzQtComponents/Components/Style.h>
#include <AzQtComponents/Components/TitleBarOverdrawHandler.h>
#include <AzQtComponents/Components/AutoCustomWindowDecorations.h>

namespace AzQtComponents
{
#if !defined(AZ_PLATFORM_LINUX)

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

#endif // !defined(AZ_PLATFORM_LINUX)

    StyleManager* StyleManager::s_instance = nullptr;

    static QString FindPath(QApplication* application, const QString& path)
    {
#if !defined(AZ_PLATFORM_LINUX)
        QDir rootDir(FindEngineRootDir(application));

        // for now, assume the executable is in the dev/Bin64 directory. We'll find our resources relative to the dev directory
        return rootDir.absoluteFilePath(path);
#else
        AZ_Assert(false,"Not supported on Linux");
        return QString();
#endif // !defined(AZ_PLATFORM_LINUX)
    }

    static QStyle* createBaseStyle()
    {
#if !defined(AZ_PLATFORM_LINUX)
        return QStyleFactory::create("Fusion");
#else
        AZ_Assert(false,"Not supported on Linux");
        return nullptr;
#endif // !defined(AZ_PLATFORM_LINUX)

    }

    void StyleManager::addSearchPaths(const QString& searchPrefix, const QString& pathOnDisk, const QString& qrcPrefix)
    {
#if !defined(AZ_PLATFORM_LINUX)
        if (!s_instance)
        {
            qFatal("StyleManager::addSearchPaths called before instance was created");
            return;
        }

        s_instance->m_stylesheetCache->addSearchPaths(searchPrefix, pathOnDisk, qrcPrefix);
#else
        AZ_Assert(false,"Not supported on Linux");
#endif // !defined(AZ_PLATFORM_LINUX)
    }

    bool StyleManager::setStyleSheet(QWidget* widget, QString styleFileName)
    {
#if !defined(AZ_PLATFORM_LINUX)
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
#else
        AZ_Assert(false,"Not supported on Linux");
        return false;
#endif // !defined(AZ_PLATFORM_LINUX)
    }

    QStyleSheetStyle* StyleManager::styleSheetStyle(const QWidget* widget)
    {
#if !defined(AZ_PLATFORM_LINUX)
        Q_UNUSED(widget);
        // widget is currently unused, but would be required if Qt::AA_ManualStyleSheetStyle was
        // not set.

        if (!s_instance)
        {
            AZ_Warning("StyleManager", false, "StyleManager::styleSheetStyle called before instance was created");
            return nullptr;
        }

        if (!qApp->testAttribute(Qt::AA_ManualStyleSheetStyle))
        {
            qFatal("StyleManager::styleSheetStyle has not been implemented for automatically created QStyleSheetStyles");
            return nullptr;
        }

        return s_instance->m_useUI10 ? s_instance->m_styleSheetStyle10 : s_instance->m_styleSheetStyle20;
#else
        AZ_Assert(false,"Not supported on Linux");
        return nullptr;
#endif // !defined(AZ_PLATFORM_LINUX)
    }

    QStyle *StyleManager::baseStyle(const QWidget *widget)
    {
#if !defined(AZ_PLATFORM_LINUX)
        const auto sss = styleSheetStyle(widget);
        return sss ? sss->baseStyle() : nullptr;
#else
        AZ_Assert(false,"Not supported on Linux");
        return nullptr;
#endif // !defined(AZ_PLATFORM_LINUX)
    }

    void StyleManager::repolishStyleSheet(QWidget* widget)
    {
#if !defined(AZ_PLATFORM_LINUX)
        StyleManager::styleSheetStyle(widget)->repolish(widget);
#else
        AZ_Assert(false,"Not supported on Linux");
#endif // !defined(AZ_PLATFORM_LINUX)
    }

    bool StyleManager::isUi10()
    {
        if (!s_instance)
        {
            AZ_Warning("StyleManager", false, "StyleManager::isUi10 called before instance was created");
            return false;
        }

        return s_instance->m_useUI10;
    }

    StyleManager::StyleManager(QObject* parent)
        : QObject(parent)
        , m_stylesheetPreprocessor(new StylesheetPreprocessor(this))
        , m_stylesheetCache(new StyleSheetCache(this))
    {
#if !defined(AZ_PLATFORM_LINUX)
        if (s_instance)
        {
            qFatal("A StyleManager already exists");
        }
#else
        AZ_Assert(false,"Not supported on Linux");
#endif // !defined(AZ_PLATFORM_LINUX)
    }

    StyleManager::~StyleManager()
    {
#if !defined(AZ_PLATFORM_LINUX)
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
#else
        AZ_Assert(false,"Not supported on Linux");
#endif // !defined(AZ_PLATFORM_LINUX)
    }

    void StyleManager::initialize(QApplication* application, bool useUI10)
    {
#if !defined(AZ_PLATFORM_LINUX)
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
#else
        AZ_Assert(false,"Not supported on Linux");
#endif // !defined(AZ_PLATFORM_LINUX)
    }

    void StyleManager::switchUI(QApplication* application, bool useUI10)
    {
#if !defined(AZ_PLATFORM_LINUX)
        if (m_useUI10 == useUI10)
        {
            // only switch if necessary
            return;
        }

        switchUIInternal(application, useUI10);
#else
        AZ_Assert(false,"Not supported on Linux");
#endif // !defined(AZ_PLATFORM_LINUX)
    }

    void StyleManager::switchUIInternal(QApplication* application, bool useUI10)
    {
#if !defined(AZ_PLATFORM_LINUX)
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
#else
        AZ_Assert(false,"Not supported on Linux");
#endif // !defined(AZ_PLATFORM_LINUX)
    }

    void StyleManager::cleanupStyles()
    {
#if !defined(AZ_PLATFORM_LINUX)
        if (auto application = qobject_cast<QApplication*>(sender()))
        {
            application->setStyle(createBaseStyle());
        }
#else
        AZ_Assert(false,"Not supported on Linux");
#endif // !defined(AZ_PLATFORM_LINUX)
    }

    void StyleManager::stopTrackingWidget(QObject* object)
    {
#if !defined(AZ_PLATFORM_LINUX)
        const auto widget = qobject_cast<QWidget* const>(object);
        if (!widget)
        {
            return;
        }

        m_widgetToStyleSheetMap.remove(widget);

        // Remove any old stylesheet
        widget->setStyleSheet(QString());
#else
        AZ_Assert(false,"Not supported on Linux");
#endif // !defined(AZ_PLATFORM_LINUX)
    }

    void StyleManager::initializeFonts()
    {
#if !defined(AZ_PLATFORM_LINUX)
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
        QFontDatabase::addApplicationFont(amazonEmberPathSpecifier.arg("AmazonEmber_Lt.ttf"));
        QFontDatabase::addApplicationFont(amazonEmberPathSpecifier.arg("AmazonEmber_Medium.ttf"));
        QFontDatabase::addApplicationFont(amazonEmberPathSpecifier.arg("AmazonEmber_Rg.ttf"));
#else
        AZ_Assert(false,"Not supported on Linux");
#endif // !defined(AZ_PLATFORM_LINUX)
    }

    void StyleManager::initializeSearchPaths(QApplication* application)
    {
#if !defined(AZ_PLATFORM_LINUX)
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
#else
        AZ_Assert(false,"Not supported on Linux");
#endif // !defined(AZ_PLATFORM_LINUX)
    }

    static bool WriteStylesheetForQtDesigner(const QString& processedStyle)
    {
#if !defined(AZ_PLATFORM_LINUX)
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
#else
        AZ_Assert(false,"Not supported on Linux");
        return false;
#endif // !defined(AZ_PLATFORM_LINUX)
    }

    static void RefreshUI_1_0_Style(QApplication* application, QStyleSheetStyle* styleSheetStyle10, StylesheetPreprocessor* stylesheetPreprocessor)
    {
#if !defined(AZ_PLATFORM_LINUX)
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
#else
        AZ_Assert(false,"Not supported on Linux");
#endif // !defined(AZ_PLATFORM_LINUX)
    }

    void StyleManager::refresh(QApplication* application)
    {
#if !defined(AZ_PLATFORM_LINUX)
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

        // QMessageBox uses "QMdiSubWindowTitleBar" class to query the titlebar font
        // through QApplication::font() and (buggily) calculate required width of itself
        // to fit the title. It bypassess stylesheets. See QMessageBoxPrivate::updateSize().
        // When switching back to UI1.0 it's reinitialized to system defualt.
        QFont titleBarFont("Amazon Ember");
        titleBarFont.setPixelSize(18);
        QApplication::setFont(titleBarFont, "QMdiSubWindowTitleBar");
#else
        AZ_Assert(false,"Not supported on Linux");
#endif // !defined(AZ_PLATFORM_LINUX)
    }

    const QColor& StyleManager::getColorByName(const QString& name)
    {
#if !defined(AZ_PLATFORM_LINUX)
        return m_stylesheetPreprocessor->GetColorByName(name);
#else
        AZ_Assert(false,"Not supported on Linux");
        static QColor dummy;
        return dummy;
#endif // !defined(AZ_PLATFORM_LINUX)
    }
} // namespace AzQtComponents

#include <Components/StyleManager.moc>

#if defined(AZ_QT_COMPONENTS_STATIC)
    // If we're statically compiling the lib, we need to include the compiled rcc resources
    // somewhere to ensure that the linker doesn't optimize the symbols out (with Visual Studio at least)
    // With dlls, there's no step to optimize out the symbols, so we don't need to do this.
    #include <Components/rcc_resources.h>
#endif // #if defined(AZ_QT_COMPONENTS_STATIC)


