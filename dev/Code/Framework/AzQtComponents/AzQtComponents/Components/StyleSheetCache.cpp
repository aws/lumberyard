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

#include <AzQtComponents/Components/StyleSheetCache.h>
#include <AzQtComponents/Utilities/QtPluginPaths.h>
#include <QProxyStyle>
#include <QApplication>
#include <QWidget>
#include <QDir>
#include <QFile>
#include <QSet>
#include <QFileSystemWatcher>
#include <QStringList>
#include <QRegExp>
#include <QDebug>

namespace AzQtComponents
{

const QString g_styleSheetRelativePath = QStringLiteral("Code/Framework/AzQtComponents/AzQtComponents/Components/Widgets");
const QString g_styleSheetResourcePath = QStringLiteral(":AzQtComponents/Widgets");
const QString g_globalStyleSheetName = QStringLiteral("BaseStyleSheet.qss");
const QString g_styleSheetExtension = QStringLiteral(".qss");
const QString g_searchPathPrefix = QStringLiteral("AzQtComponentWidgets");

StyleSheetCache::StyleSheetCache(QApplication* application, QObject* parent)
: QObject(parent)
, m_application(application)
, m_rootDir(FindEngineRootDir(application))
, m_fileWatcher(new QFileSystemWatcher(this))
, m_importExpression(new QRegExp("^\\s*@import\\s+\"?([^\"]+)\"?\\s*;(.*)$"))
{
    QDir rootDir(m_rootDir);
    QString pathOnDisk = rootDir.absoluteFilePath(g_styleSheetRelativePath);

    // set the search paths such so that the disk is searched first, and if a file isn't found there,
    // it will fall back to search in the resources loaded from the qrc files
    QDir::setSearchPaths(g_searchPathPrefix, QStringList() << pathOnDisk << g_styleSheetResourcePath);

    connect(m_fileWatcher, &QFileSystemWatcher::fileChanged, this, &StyleSheetCache::fileOnDiskChanged);
}

StyleSheetCache::~StyleSheetCache()
{
}

bool StyleSheetCache::isUI10() const
{
    return m_isUI10;
}

void StyleSheetCache::setIsUI10(bool isUI10)
{
    if (isUI10 != m_isUI10)
    {
        m_isUI10 = isUI10;

        reloadStyleSheets();
    }
}

void StyleSheetCache::reloadStyleSheets()
{
    m_styleSheetCache.clear();

    applyGlobalStyleSheet();
}

void StyleSheetCache::fileOnDiskChanged(const QString& filePath)
{
    qDebug() << "All styleSheets reloading, triggered by " << filePath << " changing on disk.";

    // Much easier to just reload all stylesheets, instead of trying to figure out
    // which ones are affected by this file.
    // If we were to worry about just one file, we'd need to keep track of dependency
    // info when preprocessing @import's
    reloadStyleSheets();
}

QString StyleSheetCache::loadStyleSheet(QString styleFileName)
{
    // include the file extension here; it'll make life easier when comparing file paths
    if (!styleFileName.endsWith(g_styleSheetExtension))
    {
        styleFileName.append(g_styleSheetExtension);
    }

    // check the cache
    if (m_styleSheetCache.contains(styleFileName))
    {
        return m_styleSheetCache[styleFileName];
    }

    QString filePath = findStyleSheetPath(styleFileName);
    if (filePath.isEmpty())
    {
        return QString();
    }

    // watch this file for changes now, if it's not loaded from resources
    if (!filePath.startsWith(QStringLiteral(":")))
    {
        m_fileWatcher->addPath(filePath);
    }

    QString loadedStyleSheet;

    if (QFile::exists(filePath))
    {
        QFile styleSheetFile;
        styleSheetFile.setFileName(filePath);
        if (styleSheetFile.open(QFile::ReadOnly))
        {
            loadedStyleSheet = styleSheetFile.readAll();
        }
    }

    // pre-process stylesheet
    loadedStyleSheet = preprocess(styleFileName, loadedStyleSheet);

    m_styleSheetCache.insert(styleFileName, loadedStyleSheet);

    return loadedStyleSheet;
}

class MiniLessParser
{
public:
    MiniLessParser(StyleSheetCache* cache);

    QString process(const QString& styleSheet);

private:

    enum class State
    {
        Default,
        Comment,
    };

    void parseLine(const QString& line);
    int parseForImportStatement(const QString& line);

    State m_state = State::Default;
    QRegExp m_importStatement;
    QChar m_lastCharacter;
    QString m_result;
    int m_lineNumber = 0;
    StyleSheetCache* m_cache = nullptr;
};

MiniLessParser::MiniLessParser(StyleSheetCache* cache)
    : m_importStatement("@import\\s+\"?([^\"]+)\"?\\s*;")
    , m_cache(cache)
{
}

QString MiniLessParser::process(const QString& styleSheet)
{
    m_state = State::Default;
    m_result.reserve(styleSheet.size());
    m_lineNumber = 0;
    m_lastCharacter = QChar();

    QStringList lines = styleSheet.split(QRegExp("[\\n\\r]"), QString::SkipEmptyParts);
    for (QString& line : lines)
    {
        parseLine(line);

        m_lineNumber++;
    }

    return m_result;
}

void MiniLessParser::parseLine(const QString& line)
{
    for (int i = 0; i < line.size(); i++)
    {
        QChar currentChar = line[i];
        switch (m_state)
        {
            case State::Default:
            {
                if (currentChar == '@')
                {
                    // parse for import
                    i += parseForImportStatement(line.mid(i));
                    currentChar = QChar();
                }
                else
                {
                    if ((m_lastCharacter == '/') && (currentChar == '*'))
                    {
                        m_state = State::Comment;
                    }

                    m_result += currentChar;
                }
            }
            break;

            case State::Comment:
            {
                if ((m_lastCharacter == '*') && (currentChar == '/'))
                {
                    m_state = State::Default;
                }
                m_result += currentChar;
            }
            break;
        }

        m_lastCharacter = currentChar;
    }

    m_result += "\n";

    // reset our state
    m_lastCharacter = QChar();
    m_state = State::Default;
}

int MiniLessParser::parseForImportStatement(const QString& line)
{
    QString importName;
    int ret = 0;
    int pos = m_importStatement.indexIn(line, 0);
    if (pos != -1)
    {
        importName = m_importStatement.cap(1);
        ret = m_importStatement.cap(0).size();

        if (!importName.isEmpty())
        {
            // attempt to import
            QString subStyleSheet = m_cache->loadStyleSheet(importName);
            if (!subStyleSheet.isEmpty())
            {
                // error?
            }

            m_result += subStyleSheet;
        }
    }

    return ret;
}

QString StyleSheetCache::preprocess(QString styleFileName, QString loadedStyleSheet)
{
    // Add in really basic support in here for less style @import statements
    // This allows us to split up css chunks into other files, to group things
    // in a much saner way

    // check for dumb recursion
    if (m_processingFiles.contains(styleFileName))
    {
        qDebug() << QString("Recursion found while processing styleSheets in the following order:");
        for (QString& file : m_processingStack)
        {
            qDebug() << file;
        }

        return loadedStyleSheet;
    }

    m_processingStack.push_back(styleFileName);
    m_processingFiles.insert(styleFileName);

    // take a guess at the size of the string needed with imports
    QString result;
    result.reserve(loadedStyleSheet.size() * 3);

    // run our mini less parser on the stylesheet, to process imports now
    MiniLessParser lessParser(this);
    result = lessParser.process(loadedStyleSheet);

    m_processingStack.pop_back();
    m_processingFiles.remove(styleFileName);

    return result;
}

QString StyleSheetCache::findStyleSheetPath(QString styleFileName)
{
    return QString("%1:%2").arg(g_searchPathPrefix, styleFileName);
}

void StyleSheetCache::applyGlobalStyleSheet()
{
    QString globalStyleSheet = loadStyleSheet(g_globalStyleSheetName);
    m_application->setStyleSheet(globalStyleSheet);
}

} // namespace AzQtComponents

#include <Components/StyleSheetCache.moc>
