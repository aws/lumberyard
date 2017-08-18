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

#include <AzQtComponents/Components/LumberyardStylesheet.h>
#include <QTextStream>
#include <QApplication>
#include <QPalette>
#include <QDir>
#include <QString>
#include <QFile>
#include <QFontDatabase>
#include <QStyleFactory>
#include <AzQtComponents/Components/StylesheetPreprocessor.h>
#include <AzQtComponents/Utilities/QtPluginPaths.h>
#include <AzQtComponents/Components/EditorProxyStyle.h>

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

    LumberyardStylesheet::LumberyardStylesheet(QObject* parent)
        : QObject(parent)
        , m_stylesheetPreprocessor(new StylesheetPreprocessor(this))
    {
    }

    LumberyardStylesheet::~LumberyardStylesheet()
    {
        delete m_stylesheetPreprocessor;
    }

    void LumberyardStylesheet::Initialize(QApplication* application)
    {
        InitializeSearchPaths(application);
        InitializeFonts();

        auto proxyStyle = new AzQtComponents::EditorProxyStyle(QStyleFactory::create("Fusion"));
        proxyStyle->setAutoWindowDecorationMode(AzQtComponents::EditorProxyStyle::AutoWindowDecorationMode_AnyWindow);
        application->setStyle(proxyStyle);

        Refresh(application);
    }

    void LumberyardStylesheet::InitializeFonts()
    {
        QFontDatabase::addApplicationFont(QString(FONT_PATH) + "/Open_Sans/OpenSans-Bold.ttf");
        QFontDatabase::addApplicationFont(QString(FONT_PATH) + "/Open_Sans/OpenSans-BoldItalic.ttf");
        QFontDatabase::addApplicationFont(QString(FONT_PATH) + "/Open_Sans/OpenSans-ExtraBold.ttf");
        QFontDatabase::addApplicationFont(QString(FONT_PATH) + "/Open_Sans/OpenSans-ExtraBoldItalic.ttf");
        QFontDatabase::addApplicationFont(QString(FONT_PATH) + "/Open_Sans/OpenSans-Italic.ttf");
        QFontDatabase::addApplicationFont(QString(FONT_PATH) + "/Open_Sans/OpenSans-Light.ttf");
        QFontDatabase::addApplicationFont(QString(FONT_PATH) + "/Open_Sans/OpenSans-LightItalic.ttf");
        QFontDatabase::addApplicationFont(QString(FONT_PATH) + "/Open_Sans/OpenSans-Regular.ttf");
        QFontDatabase::addApplicationFont(QString(FONT_PATH) + "/Open_Sans/OpenSans-Semibold.ttf");
        QFontDatabase::addApplicationFont(QString(FONT_PATH) + "/Open_Sans/OpenSans-SemiboldItalic.ttf");
    }

    void LumberyardStylesheet::InitializeSearchPaths(QApplication* application)
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

    void LumberyardStylesheet::Refresh(QApplication* application)
    {
        QFile styleSheetVariablesFile;
        styleSheetVariablesFile.setFileName(FindPath(application, STYLE_SHEET_VARIABLES_PATH_DARK));

        if (styleSheetVariablesFile.open(QFile::ReadOnly))
        {
            m_stylesheetPreprocessor->ReadVariables(styleSheetVariablesFile.readAll());
        }
        else
        {
            styleSheetVariablesFile.setFileName(STYLE_SHEET_VARIABLES_PATH_DARK_ALT);

            if (styleSheetVariablesFile.open(QFile::ReadOnly))
            {
                m_stylesheetPreprocessor->ReadVariables(styleSheetVariablesFile.readAll());
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
            QString processedStyle = m_stylesheetPreprocessor->ProcessStyleSheet(styleSheetFile.readAll());

            WriteStylesheetForQtDesigner(processedStyle);

            application->setStyleSheet(processedStyle);
        }

        // fusion uses the current palette to create a uniform style across platforms
        // instead of using e.g. windows XP / vista glossy buttons
        QPalette newPal(application->palette());
        newPal.setColor(QPalette::Link, m_stylesheetPreprocessor->GetColorByName(QString("LinkColor")));
        application->setPalette(newPal);
    }

    const QColor& LumberyardStylesheet::GetColorByName(const QString& name)
    {
        return m_stylesheetPreprocessor->GetColorByName(name);
    }

    bool LumberyardStylesheet::WriteStylesheetForQtDesigner(const QString& processedStyle)
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

#include <Components/LumberyardStylesheet.moc>

} // namespace AzQtComponents

#if defined(AZ_QT_COMPONENTS_STATIC)
    // If we're statically compiling the lib, we need to include the compiled rcc resources
    // somewhere to ensure that the linker doesn't optimize the symbols out (with Visual Studio at least)
    // With dlls, there's no step to optimize out the symbols, so we don't need to do this.
    #include <Components/rcc_resources.h>
#endif // #if defined(AZ_QT_COMPONENTS_STATIC)


