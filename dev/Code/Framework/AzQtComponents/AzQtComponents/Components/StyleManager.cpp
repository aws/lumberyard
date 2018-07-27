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

#include <AzQtComponents/Components/StylesheetPreprocessor.h>
#include <AzQtComponents/Utilities/QtPluginPaths.h>
#include <AzQtComponents/Components/EditorProxyStyle.h>
#include <AzQtComponents/Components/StyleSheetCache.h>
#include <AzQtComponents/Components/Style.h>
#include <AzQtComponents/Components/TitleBarOverdrawHandler.h>

namespace AzQtComponents
{

#define STYLE_SHEET_VARIABLES_PATH_DARK "Code/Sandbox/Editor/Style/EditorStylesheetVariables_Dark.json" // file path on disk
#define STYLE_SHEET_VARIABLES_PATH_DARK_ALT ":Editor/Style/EditorStylesheetVariables_Dark.json" // in our qrc file as a fallback
#define STYLE_SHEET_PATH "Code/Sandbox/Editor/Style/NewEditorStylesheet.qss" // file path on disk
#define STYLE_SHEET_PATH_ALT ":Editor/Style/NewEditorStylesheet.qss" //in our qrc file as a fallback
#define FONT_PATH "Editor/Fonts"



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

    StyleManager::StyleManager(QObject* parent)
        : QObject(parent)
        , m_stylesheetPreprocessor(new StylesheetPreprocessor(this))
    {
    }

    StyleManager::~StyleManager()
    {
        delete m_stylesheetPreprocessor;
    }

    void StyleManager::Initialize(QApplication* application, bool useUI10)
    {
        InitializeSearchPaths(application);
        InitializeFonts();

        m_titleBarOverdrawHandler = TitleBarOverdrawHandler::createHandler(application, this);

        auto proxyStyle = new EditorProxyStyle(createBaseStyle());
        proxyStyle->setAutoWindowDecorationMode(EditorProxyStyle::AutoWindowDecorationMode_AnyWindow);
        application->setStyle(proxyStyle);

        m_stylesheetCache = new StyleSheetCache(application, this);
        SwitchUI(application, useUI10);
    }

    void StyleManager::SwitchUI(QApplication* application, bool useUI10)
    {
        if (m_stylesheetCache->isUI10() == useUI10)
        {
            // only switch if necessary
            return;
        }

        m_stylesheetCache->setIsUI10(useUI10);

        // Save the oldStyle (before reseting stylesheet!), so we can workaround a leak in Qt
        QPointer<QStyle> oldStyle = qApp->style();

        // Clean any previous style sheet before setting styles
        qApp->setStyleSheet(QString());

        if (useUI10) // DEPRECATED UI 1.0
        {
            auto legacyStyle = new EditorProxyStyle(createBaseStyle());
            legacyStyle->setAutoWindowDecorationMode(EditorProxyStyle::AutoWindowDecorationMode_AnyWindow);

            // Note: QApplication will automatically clean up the old style it's been using
            application->setStyle(legacyStyle);

            // Call refresh to load and set the stylesheets.
            Refresh(application);

            // Style chain is now: QStyleSheetStyle -> EditorProxyStyle -> Fusion

            // If the previous style was a ui2.0 don't leak it.
            // Using a QPointer so it doesn't double-delete when fixed in Qt.
            // FIXME: Fix this in Qt
            delete qobject_cast<Style*>(oldStyle);
        }
        else
        {
            // Our desired chain is Style -> QStyleSheetStyle -> Fusion but since
            // QStyleSheetStyle is private we have to do it in two steps, first set the Fusion style
            // and create a stylesheet, so we have QStyleSheetStyle using Fusion as base.
            // Finally create our proxy with QStyleSheetStyle as base

            // Create the base style that the stylesheet style should use.
            application->setStyle(createBaseStyle());

            // Call refresh to load and set the stylesheets.
            Refresh(application);

            // Set the UI 2.0 style, using the stylesheet style as the base style
            application->setStyle(new Style(application->style()));
        }
    }

    void StyleManager::InitializeFonts()
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

    void StyleManager::InitializeSearchPaths(QApplication* application)
    {
        // now that QT is initialized, we can use its path manipulation functions to set the rest up:
        QString rootDir = FindEngineRootDir(application);

        if (!rootDir.isEmpty())
        {
            QDir appPath(rootDir);
            QApplication::addLibraryPath(appPath.absolutePath());

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

    static void RefreshUI_1_0_Style(QApplication* application, StylesheetPreprocessor* stylesheetPreprocessor)
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

            application->setStyleSheet(processedStyle);
        }

        // fusion uses the current palette to create a uniform style across platforms
        // instead of using e.g. windows XP / vista glossy buttons
        QPalette newPal(application->palette());
        newPal.setColor(QPalette::Link, stylesheetPreprocessor->GetColorByName(QString("LinkColor")));
        application->setPalette(newPal);
    }

    void StyleManager::Refresh(QApplication* application)
    {
        //////////////////////////////////////////////////////////////////////////
        // DEPRECATED UI 1.0
        if (m_stylesheetCache->isUI10())
        {
            RefreshUI_1_0_Style(application, m_stylesheetPreprocessor);

            return;
        }
        //////////////////////////////////////////////////////////////////////////

        m_stylesheetCache->reloadStyleSheets();
    }

    const QColor& StyleManager::GetColorByName(const QString& name)
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


