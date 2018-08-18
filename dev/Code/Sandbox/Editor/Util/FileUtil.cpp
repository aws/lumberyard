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
// Original file Copyright Crytek GMBH or its affiliates, used under license.

#include "StdAfx.h"
#include "STLPoolAllocator.h"
#include "FileUtil.h"
#include "IndexedFiles.h"
#include "CheckOutDialog.h"
#include "ISourceControl.h"
#include "Dialogs/Generic/UserOptions.h"
#include "Clipboard.h"
#include "IAssetItem.h"
#include "IAssetItemDatabase.h"
#include "Alembic/AlembicCompiler.h"
#include <CryFile.h>
#include <time.h>
#include <QMenu>
#include <QAction>
#include <QMessageBox>
#include <QApplication>
#include <QClipboard>
#include <QDesktopServices>
#include <QProcess>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QSettings>
#include <QThread>
#include "CryCommonTools/StringHelpers.h"
#include "QtUtil.h"
#include "QtUtilWin.h"
#include "Util/PathUtil.h"
#include "AutoDirectoryRestoreFileDialog.h"
#include "MainWindow.h"

#include <AzCore/PlatformIncl.h>
#include <AzCore/Component/TickBus.h>
#include <AzFramework/API/ApplicationAPI.h>
#include <AzQtComponents/Utilities/DesktopUtilities.h>
#include <AzToolsFramework/SourceControl/SourceControlAPI.h>
#include <AzToolsFramework/UI/UICore/ProgressShield.hxx>
#include <AzToolsFramework/UI/UICore/WidgetHelpers.h>
#include <AzToolsFramework/API/EditorAssetSystemAPI.h>
#include <AzToolsFramework/Thumbnails/SourceControlThumbnailBus.h>

#if defined(AZ_PLATFORM_WINDOWS)
#include <Shellapi.h>
#endif

bool CFileUtil::s_singleFileDlgPref[IFileUtil::EFILE_TYPE_LAST] = { true, true, true, true, true };
bool CFileUtil::s_multiFileDlgPref[IFileUtil::EFILE_TYPE_LAST] = { true, true, true, true, true };

CAutoRestoreMasterCDRoot::~CAutoRestoreMasterCDRoot()
{
    QDir::setCurrent(GetIEditor()->GetMasterCDFolder());
}

bool CFileUtil::CompileLuaFile(const char* luaFilename)
{
    QString luaFile = luaFilename;

    if (luaFile.isEmpty())
    {
        return false;
    }

    // Check if this file is in Archive.
    {
        CCryFile file;
        if (file.Open(luaFilename, "rb"))
        {
            // Check if in pack.
            if (file.IsInPak())
            {
                return true;
            }
        }
    }

    luaFile = Path::GamePathToFullPath(luaFilename);

    // First try compiling script and see if it have any errors.
    QString LuaCompiler;
    QString CompilerOutput;

    // Create the filepath of the lua compiler
    QString szExeFileName = qApp->applicationFilePath();
    QString exePath = Path::GetPath(szExeFileName);

#if defined(AZ_PLATFORM_WINDOWS)
    const char* luaCompiler = "LuaCompiler.exe";
#else
    const char* luaCompiler = "lua";
#endif
    LuaCompiler = Path::AddPathSlash(exePath) + luaCompiler + " ";

    AZStd::string path = luaFile.toUtf8().data();
    EBUS_EVENT(AzFramework::ApplicationRequests::Bus, NormalizePath, path);

    QString finalPath = path.c_str();
    finalPath = "\"" + finalPath + "\"";

    // Add the name of the Lua file
    QString cmdLine = LuaCompiler + finalPath;

    // Execute the compiler and capture the output
    if (!GetIEditor()->ExecuteConsoleApp(cmdLine, CompilerOutput))
    {
        QMessageBox::critical(QApplication::activeWindow(), QString(), QObject::tr("Error while executing '%1', make sure the file is in" \
            " your Master CD folder !").arg(luaCompiler));
        return false;
    }

    // Check return string
    if (!CompilerOutput.isEmpty())
    {
        // Errors while compiling file.

        // Show output from Lua compiler
        if (QMessageBox::critical(QApplication::activeWindow(), QObject::tr("Lua Compiler"),
            QObject::tr("Error output from Lua compiler:\r\n%1\r\nDo you want to edit the file ?").arg(CompilerOutput), QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes)
        {
            int line = 0;
            QString cmdLine = luaFile;
            int index = CompilerOutput.indexOf("at line");
            if (index >= 0)
            {
                azsscanf(CompilerOutput.mid(index).toUtf8().data(), "at line %d", &line);
            }
            // Open the Lua file for editing
            EditTextFile(luaFile.toUtf8().data(), line);
        }
        return false;
    }
    return true;
}
//////////////////////////////////////////////////////////////////////////
bool CFileUtil::ExtractFile(QString& file, bool bMsgBoxAskForExtraction, const char* pDestinationFilename)
{
    CCryFile cryfile;
    if (cryfile.Open(file.toUtf8().data(), "rb"))
    {
        // Check if in pack.
        if (cryfile.IsInPak())
        {
            const char* sPakName = cryfile.GetPakPath();

            if (bMsgBoxAskForExtraction)
            {
                // Cannot edit file in pack, suggest to extract it for editing.
                if (QMessageBox::critical(QApplication::activeWindow(), QString(), QObject::tr("File %1 is inside a PAK file %2\r\nDo you want it to be extracted for editing ?").arg(file, sPakName), QMessageBox::Yes | QMessageBox::No) == QMessageBox::No)
                {
                    return false;
                }
            }

            if (pDestinationFilename)
            {
                file = pDestinationFilename;
            }

            CFileUtil::CreatePath(Path::GetPath(file));

            // Extract it from Pak file.
            QFile diskFile(file);

            if (diskFile.open(QFile::WriteOnly))
            {
                // Copy data from packed file to disk file.
                char* data = new char[cryfile.GetLength()];
                cryfile.ReadRaw(data, cryfile.GetLength());
                diskFile.write(data, cryfile.GetLength());
                delete []data;
            }
            else
            {
                Warning("Failed to create file %s on disk", file.toUtf8().data());
            }
        }
        else
        {
            file = cryfile.GetAdjustedFilename();
        }

        return true;
    }

    return false;
}
//////////////////////////////////////////////////////////////////////////
void CFileUtil::EditTextFile(const char* txtFile, int line, IFileUtil::ETextFileType fileType)
{
    QString file = txtFile;

    QString fullPathName =  Path::GamePathToFullPath(file);
    ExtractFile(fullPathName);
    QString cmd(fullPathName);
#if defined (AZ_PLATFORM_WINDOWS)
    cmd.replace('/', '\\');
    if (line != 0)
    {
        cmd = QStringLiteral("%1/%2/0").arg(cmd).arg(line);
    }
#endif

    QString TextEditor = gSettings.textEditorForScript;
    if (fileType == IFileUtil::FILE_TYPE_SHADER)
    {
        TextEditor = gSettings.textEditorForShaders;
    }
    else if (fileType == IFileUtil::FILE_TYPE_BSPACE)
    {
        TextEditor = gSettings.textEditorForBspaces;
    }

    // attempt to open it with the text editor from the preferences.
#if defined(AZ_PLATFORM_APPLE)
    // 'open' already detaches, so dont use startDetach, otherwise we can't see the exit code
    if (QProcess::execute(QStringLiteral("open"), { QStringLiteral("-a"), TextEditor, fullPathName }) != 0)
#else
    if (!QProcess::startDetached(TextEditor, { cmd }))
#endif
    {
        // allow the user to opt for notepad, but also tell them about the preferences panel

        if (QMessageBox::question(QApplication::activeWindow(), QString(), QObject::tr("Can't open the file. You can specify a text editor in the Preferences dialog.  Do you want to open the file in the default platform editor?"), QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes)
        {
            QDesktopServices::openUrl(QUrl::fromLocalFile(fullPathName));
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CFileUtil::EditTextureFile(const char* textureFile, bool bUseGameFolder)
{
    using namespace AzToolsFramework;
    using namespace AzToolsFramework::AssetSystem;

    AZStd::string fullTexturePath;
    bool fullTexturePathFound = false;
    AZStd::string relativePath = textureFile;
    AssetSystemRequestBus::BroadcastResult(fullTexturePathFound, &AssetSystemRequest::GetFullSourcePathFromRelativeProductPath, relativePath, fullTexturePath);

    QWidget* parentWidget = QApplication::activeWindow() ? QApplication::activeWindow() : MainWindow::instance();

    if (!fullTexturePathFound)
    {
        QString messageString = QString("Failed to find absolute path to %1 - could not open texture editor").arg(textureFile);
        QMessageBox::warning(parentWidget, "Cannot open file!", messageString);
        return;
    }

    bool failedToLaunch = true;
    QByteArray textureEditorPath = gSettings.textureEditor.toUtf8();

#if defined(AZ_PLATFORM_WINDOWS)
    // Use the Win32 API calls to open the right editor; the OS knows how to do this even better than
    // Qt does.
    QString fullTexturePathFixedForWindows = QString(fullTexturePath.data()).replace('/', '\\');
    QByteArray fullTexturePathFixedForWindowsUtf8 = fullTexturePathFixedForWindows.toUtf8();
    HINSTANCE hInst = ShellExecute(NULL, "open", textureEditorPath.data(), fullTexturePathFixedForWindowsUtf8.data(), NULL, SW_SHOWNORMAL);
    failedToLaunch = ((DWORD_PTR)hInst <= 32);
#elif defined(AZ_PLATFORM_APPLE_OSX)
    failedToLaunch = QProcess::execute(QString("/usr/bin/open"), {"-a", gSettings.textureEditor, QString(fullTexturePath.data()) }) != 0;
#else
    failedToLaunch = !QProcess::startDetached(gSettings.textureEditor, { QString(fullTexturePath.data()) });
#endif

    if (failedToLaunch)
    {
        //failed
        QString messageString = QString("Failed to open %1 with texture editor %2").arg(fullTexturePath.data()).arg(textureEditorPath.data());
        QMessageBox::warning(parentWidget, "Cannot open file!", messageString);
    }
}

//////////////////////////////////////////////////////////////////////////
bool CFileUtil::EditMayaFile(const char* filepath, const bool bExtractFromPak, const bool bUseGameFolder)
{
    QString dosFilepath = QtUtil::ToQString(PathUtil::ToDosPath(filepath));
    if (bExtractFromPak)
    {
        ExtractFile(dosFilepath);
    }

    if (bUseGameFolder)
    {
        const QString sGameFolder = Path::GetEditingGameDataFolder().c_str();
        int nLength = sGameFolder.toUtf8().count();
        if (strnicmp(filepath, sGameFolder.toUtf8().data(), nLength) != 0)
        {
            dosFilepath = sGameFolder + '\\' + filepath;
        }

        dosFilepath = PathUtil::ToDosPath(dosFilepath.toUtf8().data());
    }

    const QString fullPath = Path::GetExecutableParentDirectory() + '\\' + dosFilepath;

    if (gSettings.animEditor.isEmpty())
    {
        AzQtComponents::ShowFileOnDesktop(fullPath);
    }
    else
    {
        if (!QProcess::startDetached(gSettings.animEditor, { fullPath }))
        {
            CryMessageBox("Can't open the file. You can specify a source editor in Sandbox Preferences or create an association in Windows.", "Cannot open file!", MB_OK | MB_ICONERROR);
        }
    }
    return TRUE;
}

//////////////////////////////////////////////////////////////////////////
bool CFileUtil::EditFile(const char* filePath, const bool bExtrackFromPak, const bool bUseGameFolder)
{
    QString extension = filePath;
    extension.remove(0, extension.lastIndexOf('.'));

    if (extension.compare(".ma") == 0)
    {
        return EditMayaFile(filePath, bExtrackFromPak, bUseGameFolder);
    }
    else if ((extension.compare(".bspace") == 0) || (extension.compare(".comb") == 0))
    {
        EditTextFile(filePath, 0, IFileUtil::FILE_TYPE_BSPACE);
        return TRUE;
    }

    return FALSE;
}

//////////////////////////////////////////////////////////////////////////
bool CFileUtil::CalculateDccFilename(const QString& assetFilename, QString& dccFilename)
{
    if (ExtractDccFilenameFromAssetDatabase(assetFilename, dccFilename))
    {
        return true;
    }

    if (ExtractDccFilenameUsingNamingConventions(assetFilename, dccFilename))
    {
        return true;
    }

    GetIEditor()->GetEnv()->pLog->LogError("Failed to find psd file for texture: '%s'", assetFilename.toUtf8().data());
    return false;
}

//////////////////////////////////////////////////////////////////////////
bool CFileUtil::ExtractDccFilenameFromAssetDatabase(const QString& assetFilename, QString& dccFilename)
{
    IAssetItemDatabase* pCurrentDatabaseInterface = NULL;
    std::vector<IClassDesc*> assetDatabasePlugins;
    IEditorClassFactory* pClassFactory = GetIEditor()->GetClassFactory();
    pClassFactory->GetClassesByCategory("Asset Item DB", assetDatabasePlugins);

    for (size_t i = 0; i < assetDatabasePlugins.size(); ++i)
    {
        if (assetDatabasePlugins[i]->QueryInterface(__uuidof(IAssetItemDatabase), (void**)&pCurrentDatabaseInterface) == S_OK)
        {
            if (!pCurrentDatabaseInterface)
            {
                continue;
            }

            QString assetDatabaseDccFilename;
            IAssetItem* pAssetItem = pCurrentDatabaseInterface->GetAsset(assetFilename.toUtf8().data());
            if (pAssetItem)
            {
                if ((pAssetItem->GetFlags() & IAssetItem::eFlag_Cached))
                {
                    QVariant v = pAssetItem->GetAssetFieldValue("dccfilename");
                    assetDatabaseDccFilename = v.toString();
                    if (!v.isNull())
                    {
                        dccFilename = assetDatabaseDccFilename;
                        dccFilename = Path::GetRelativePath(dccFilename, false);

                        uint32 attr = CFileUtil::GetAttributes(dccFilename.toUtf8().data());

                        if (CFileUtil::FileExists(dccFilename))
                        {
                            return true;
                        }
                        else if (GetIEditor()->IsSourceControlAvailable() && (attr & SCC_FILE_ATTRIBUTE_MANAGED))
                        {
                            return CFileUtil::GetLatestFromSourceControl(dccFilename.toUtf8().data());
                        }
                    }
                }
            }
        }
    }
    return false;
}

//////////////////////////////////////////////////////////////////////////
bool CFileUtil::ExtractDccFilenameUsingNamingConventions(const QString& assetFilename, QString& dccFilename)
{
    //else to try find it by naming conventions
    QString tempStr = assetFilename;
    int foundSplit = -1;
    if ((foundSplit = tempStr.lastIndexOf('.')) > 0)
    {
        QString first = tempStr.mid(0, foundSplit);
        tempStr = first + ".psd";
    }
    if (CFileUtil::FileExists(tempStr))
    {
        dccFilename = tempStr;
        return true;
    }

    //else try to find it by replacing post fix _<description> with .psd
    tempStr = assetFilename;
    foundSplit = -1;
    if ((foundSplit = tempStr.lastIndexOf('_')) > 0)
    {
        QString first = tempStr.mid(0, foundSplit);
        tempStr = first + ".psd";
    }
    if (CFileUtil::FileExists(tempStr))
    {
        dccFilename = tempStr;
        return true;
    }

    return false;
}

//////////////////////////////////////////////////////////////////////////
void CFileUtil::FormatFilterString(QString& filter)
{
    const int numPipeChars = std::count(filter.begin(), filter.end(), '|');
    if (numPipeChars == 1)
    {
        filter = QStringLiteral("%1||").arg(filter);
    }
    else if (numPipeChars > 1)
    {
        assert(numPipeChars % 2 != 0);
        if (!filter.contains("||"))
        {
            filter = QStringLiteral("%1||").arg(filter);
        }
    }
    else if (!filter.isEmpty())
    {
        filter = QStringLiteral("%1|%2||").arg(filter, filter);
    }
}

//////////////////////////////////////////////////////////////////////////
bool CFileUtil::SelectFile(const QString& fileSpec, const QString& searchFolder, QString& fullFileName)
{
    QtUtil::QtMFCScopedHWNDCapture cap;
    CAutoDirectoryRestoreFileDialog dlg(QFileDialog::AcceptOpen, QFileDialog::ExistingFile, {}, searchFolder, fileSpec, {}, {}, cap);

    if (dlg.exec())
    {
        fullFileName = dlg.selectedFiles().first();
        return true;
        /*
        if (!fileName.IsEmpty())
        {
            relativeFileName = Path::GetRelativePath( fileName );
            if (!relativeFileName.IsEmpty())
            {
                return true;
            }
            else
            {
                Warning( "You must select files from %s folder",(const char*)GetIEditor()->GetMasterCDFolder(); );
            }
        }
        */
    }

    //  CSelectFileDlg cDialog;
    //  bool res = cDialog.SelectFileName( &fileName,&relativeFileName,fileSpec,searchFolder );
    return false;
}

bool CFileUtil::SelectFiles(const QString& fileSpec, const QString& searchFolder, QStringList& files)
{
    QtUtil::QtMFCScopedHWNDCapture cap;
    CAutoDirectoryRestoreFileDialog dlg(QFileDialog::AcceptOpen, QFileDialog::ExistingFiles, {}, searchFolder, fileSpec, {}, {}, cap);

    if (dlg.exec())
    {
        const QStringList selected = dlg.selectedFiles();
        foreach(const QString&file, selected)
        {
            files.push_back(file);
        }
    }

    if (!files.empty())
    {
        return true;
    }

    return false;
}

bool CFileUtil::SelectSaveFile(const QString& fileFilter, const QString& defaulExtension, const QString& startFolder, QString& fileName)
{
    QDir folder(startFolder);
    QtUtil::QtMFCScopedHWNDCapture cap;
    CAutoDirectoryRestoreFileDialog dlg(QFileDialog::AcceptSave, {}, defaulExtension, folder.filePath(fileName), fileFilter, {}, {}, cap);

    if (dlg.exec())
    {
        fileName = dlg.selectedFiles().first();
        return true;
    }

    return false;
}

//////////////////////////////////////////////////////////////////////////
// Get directory contents.
//////////////////////////////////////////////////////////////////////////
inline bool ScanDirectoryFiles(const QString& root, const QString& path, const QString& fileSpec, IFileUtil::FileArray& files, bool bSkipPaks)
{
    bool anyFound = false;
    QString dir = Path::AddPathSlash(root + path);

    QString findFilter = Path::Make(dir, fileSpec);
    ICryPak* pIPak = GetIEditor()->GetSystem()->GetIPak();

    // Add all directories.
    _finddata_t fd;
    intptr_t fhandle;

    fhandle = pIPak->FindFirst(findFilter.toUtf8().data(), &fd);
    if (fhandle != -1)
    {
        do
        {
            // Skip back folders.
            if (fd.name[0] == '.')
            {
                continue;
            }
            if (fd.attrib & _A_SUBDIR) // skip sub directories.
            {
                continue;
            }

            if (bSkipPaks && (fd.attrib & _A_IN_CRYPAK))
            {
                continue;
            }

            anyFound = true;

            IFileUtil::FileDesc file;
            file.filename = path + fd.name;
            file.attrib = fd.attrib;
            file.size = fd.size;
            file.time_access = fd.time_access;
            file.time_create = fd.time_create;
            file.time_write = fd.time_write;

            files.push_back(file);
        } while (pIPak->FindNext(fhandle, &fd) == 0);
        pIPak->FindClose(fhandle);
    }

    /*
    CFileFind finder;
    BOOL bWorking = finder.FindFile( Path::Make(dir,fileSpec) );
    while (bWorking)
    {
        bWorking = finder.FindNextFile();

        if (finder.IsDots())
            continue;

        if (!finder.IsDirectory())
        {
            anyFound = true;

            IFileUtil::FileDesc fd;
            fd.filename = dir + finder.GetFileName();
            fd.nFileSize = finder.GetLength();

            finder.GetCreationTime( &fd.ftCreationTime );
            finder.GetLastAccessTime( &fd.ftLastAccessTime );
            finder.GetLastWriteTime( &fd.ftLastWriteTime );

            fd.dwFileAttributes = 0;
            if (finder.IsArchived())
                fd.dwFileAttributes |= FILE_ATTRIBUTE_ARCHIVE;
            if (finder.IsCompressed())
                fd.dwFileAttributes |= FILE_ATTRIBUTE_COMPRESSED;
            if (finder.IsNormal())
                fd.dwFileAttributes = FILE_ATTRIBUTE_NORMAL;
            if (finder.IsHidden())
                fd.dwFileAttributes = FILE_ATTRIBUTE_HIDDEN;
            if (finder.IsReadOnly())
                fd.dwFileAttributes = FILE_ATTRIBUTE_READONLY;
            if (finder.IsSystem())
                fd.dwFileAttributes = FILE_ATTRIBUTE_SYSTEM;
            if (finder.IsTemporary())
                fd.dwFileAttributes = FILE_ATTRIBUTE_TEMPORARY;

            files.push_back(fd);
        }
    }
    */

    return anyFound;
}

//////////////////////////////////////////////////////////////////////////
// Get directory contents.
//////////////////////////////////////////////////////////////////////////
inline int ScanDirectoryRecursive(const QString& root, const QString& path, const QString& fileSpec, IFileUtil::FileArray& files, bool recursive, bool addDirAlso,
    IFileUtil::ScanDirectoryUpdateCallBack updateCB, bool bSkipPaks)
{
    bool anyFound = false;
    QString dir = Path::AddPathSlash(root + path);

    if (updateCB)
    {
        QString msg = QObject::tr("Scanning %1...").arg(dir);
        if (updateCB(msg) == false)
        {
            return -1;
        }
    }

    if (ScanDirectoryFiles(root, Path::AddPathSlash(path), fileSpec, files, bSkipPaks))
    {
        anyFound = true;
    }

    if (recursive)
    {
        /*
        CFileFind finder;
        BOOL bWorking = finder.FindFile( Path::Make(dir,"*.*") );
        while (bWorking)
        {
            bWorking = finder.FindNextFile();

            if (finder.IsDots())
                continue;

            if (finder.IsDirectory())
            {
                // Scan directory.
                if (ScanDirectoryRecursive( root,Path::AddBackslash(path+finder.GetFileName()),fileSpec,files,recursive ))
                    anyFound = true;
            }
        }
        */

        ICryPak* pIPak = GetIEditor()->GetSystem()->GetIPak();

        // Add all directories.
        _finddata_t fd;
        intptr_t fhandle;

        fhandle = pIPak->FindFirst(Path::Make(dir, "*.*").toUtf8().data(), &fd);
        if (fhandle != -1)
        {
            do
            {
                // Skip back folders.
                if (fd.name[0] == '.')
                {
                    continue;
                }

                if (!(fd.attrib & _A_SUBDIR)) // skip not directories.
                {
                    continue;
                }

                if (bSkipPaks && (fd.attrib & _A_IN_CRYPAK))
                {
                    continue;
                }

                if (addDirAlso)
                {
                    IFileUtil::FileDesc Dir;
                    Dir.filename = path + fd.name;
                    Dir.attrib = fd.attrib;
                    Dir.size = fd.size;
                    Dir.time_access = fd.time_access;
                    Dir.time_create = fd.time_create;
                    Dir.time_write = fd.time_write;
                    files.push_back(Dir);
                }

                // Scan directory.
                int result = ScanDirectoryRecursive(root, Path::AddPathSlash(path + fd.name), fileSpec, files, recursive, addDirAlso, updateCB, bSkipPaks);
                if (result == -1)
                // Cancel the scan immediately.
                {
                    pIPak->FindClose(fhandle);
                    return -1;
                }
                else if (result == 1)
                {
                    anyFound = true;
                }
            } while (pIPak->FindNext(fhandle, &fd) == 0);
            pIPak->FindClose(fhandle);
        }
    }

    return anyFound ? 1 : 0;
}

//////////////////////////////////////////////////////////////////////////
bool CFileUtil::ScanDirectory(const QString& path, const QString& file, IFileUtil::FileArray& files, bool recursive,
    bool addDirAlso, IFileUtil::ScanDirectoryUpdateCallBack updateCB, bool bSkipPaks)
{
    QString fileSpec = Path::GetFile(file);
    QString localPath = Path::GetPath(file);
    return ScanDirectoryRecursive(Path::AddPathSlash(path), localPath, fileSpec, files, recursive, addDirAlso, updateCB, bSkipPaks) > 0;
}

void CFileUtil::ShowInExplorer(const QString& path)
{
    QString fullpath(Path::GetExecutableParentDirectory() + "\\" + Path::GamePathToFullPath(path));
    AzQtComponents::ShowFileOnDesktop(fullpath);
}

/*
bool CFileUtil::ScanDirectory( const QString &startDirectory,const QString &searchPath,const QString &fileSpec,FileArray &files, bool recursive=true )
{
    return ScanDirectoryRecursive(startDirectory,SearchPath,file,files,recursive );
}
*/

//////////////////////////////////////////////////////////////////////////
bool CFileUtil::OverwriteFile(const QString& filename)
{
    AZ::IO::FileIOBase* fileIO = AZ::IO::FileIOBase::GetInstance();
    AZ_Assert(fileIO, "FileIO is not initialized.");

    QString adjFileName = Path::GamePathToFullPath(filename);

    AZStd::string filePath = adjFileName.toUtf8().data();
    if (!fileIO->IsReadOnly(filePath.c_str()))
    {
        using namespace AzToolsFramework;
        SourceControlCommandBus::Broadcast(&SourceControlCommandBus::Events::RequestEdit, filePath.c_str(), true, [](bool, const SourceControlFileInfo&) {});
        return true;
    }

    // Otherwise, show the checkout dialog
    if (!CCheckOutDialog::IsForAll())
    {
        QtUtil::QtMFCScopedHWNDCapture cap;
        CCheckOutDialog dlg(adjFileName, cap);
        dlg.exec();
    }

    bool opSuccess = false;
    switch (CCheckOutDialog::LastResult())
    {
    case CCheckOutDialog::CANCEL:
        break;
    case CCheckOutDialog::CHECKOUT:
        opSuccess = CheckoutFile(filePath.c_str());
        break;
    case CCheckOutDialog::OVERWRITE:
        opSuccess = AZ::IO::SystemFile::SetWritable(filePath.c_str(), true);
        break;
    default:
        AZ_Assert(false, "Unsupported result returned from CCheckoutDialog");
    }

    return opSuccess;
}

//////////////////////////////////////////////////////////////////////////
void BlockAndWait(const bool& opComplete, QWidget* parent, const char* message)
{
    bool useProgressShiled = false;
    if (QApplication::instance()->thread() == QThread::currentThread())
    {
        if (!parent)
        {
            parent = QApplication::activeWindow() ? QApplication::activeWindow() : MainWindow::instance();
        }

        useProgressShiled = parent ? true : false;
    }

    // if we have a valid window, use a progress shield
    // Otherwise just wait.  Be sure to call Qt processEvents
    if (useProgressShiled)
    {
        AzToolsFramework::ProgressShield::LegacyShowAndWait(parent, parent->tr(message),
            [&opComplete](int& current, int& max)
            {
                current = 0;
                max = 0;
                return opComplete;
            },
            500);
    }
    else
    {
        while (!opComplete)
        {
            QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents, 16);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
bool CFileUtil::CheckoutFile(const char* filename, QWidget* parentWindow)
{
    using namespace AzToolsFramework;

    bool scOpSuccess = false;
    bool scOpComplete = false;
    SourceControlCommandBus::Broadcast(&SourceControlCommandBus::Events::RequestEdit, filename, true,
        [&scOpSuccess, &scOpComplete, filename](bool success, const SourceControlFileInfo& /*info*/)
        {
            scOpSuccess = success;
            scOpComplete = true;
            Thumbnailer::SourceControlThumbnailRequestBus::Broadcast(&Thumbnailer::SourceControlThumbnailRequests::FileStatusChanged, filename);
        }
    );

    BlockAndWait(scOpComplete, parentWindow, "Checking out for edit...");
    return scOpSuccess;
}

//////////////////////////////////////////////////////////////////////////
bool CFileUtil::RevertFile(const char* filename, QWidget* parentWindow)
{
    using namespace AzToolsFramework;

    bool scOpSuccess = false;
    bool scOpComplete = false;
    SourceControlCommandBus::Broadcast(&SourceControlCommandBus::Events::RequestRevert, filename,
        [&scOpSuccess, &scOpComplete, filename](bool success, const SourceControlFileInfo& /*info*/)
        {
            scOpSuccess = success;
            scOpComplete = true;
            Thumbnailer::SourceControlThumbnailRequestBus::Broadcast(&Thumbnailer::SourceControlThumbnailRequests::FileStatusChanged, filename);
        }
    );

    BlockAndWait(scOpComplete, parentWindow, "Discarding Changes...");
    return scOpSuccess;
}

//////////////////////////////////////////////////////////////////////////
bool CFileUtil::RenameFile(const char* sourceFile, const char* targetFile, QWidget* parentWindow)
{
    using namespace AzToolsFramework;

    bool scOpSuccess = false;
    bool scOpComplete = false;
    SourceControlCommandBus::Broadcast(&SourceControlCommandBus::Events::RequestRename, sourceFile, targetFile,
        [&scOpSuccess, &scOpComplete](bool success, const SourceControlFileInfo& /*info*/)
        {
            scOpSuccess = success;
            scOpComplete = true;
        }
    );

    BlockAndWait(scOpComplete, parentWindow, "Renaming file...");
    return scOpSuccess;
}

bool  override;

//////////////////////////////////////////////////////////////////////////
bool CFileUtil::DeleteFromSourceControl(const char* filename, QWidget* parentWindow)
{
    using namespace AzToolsFramework;

    bool scOpSuccess = false;
    bool scOpComplete = false;
    SourceControlCommandBus::Broadcast(&SourceControlCommandBus::Events::RequestDelete, filename,
        [&scOpSuccess, &scOpComplete, filename](bool success, const SourceControlFileInfo& /*info*/)
        {
            scOpSuccess = success;
            scOpComplete = true;
            Thumbnailer::SourceControlThumbnailRequestBus::Broadcast(&Thumbnailer::SourceControlThumbnailRequests::FileStatusChanged, filename);
        }
    );

    BlockAndWait(scOpComplete, parentWindow, "Marking for deletion...");
    return scOpSuccess;
}

//////////////////////////////////////////////////////////////////////////
bool CFileUtil::GetLatestFromSourceControl(const char* filename, QWidget* parentWindow)
{
    using namespace AzToolsFramework;

    bool scOpSuccess = false;
    bool scOpComplete = false;
    SourceControlCommandBus::Broadcast(&SourceControlCommandBus::Events::RequestLatest, filename,
        [&scOpSuccess, &scOpComplete, filename](bool success, const SourceControlFileInfo& /*info*/)
        {
            scOpSuccess = success;
            scOpComplete = true;
            Thumbnailer::SourceControlThumbnailRequestBus::Broadcast(&Thumbnailer::SourceControlThumbnailRequests::FileStatusChanged, filename);
        }
    );

    BlockAndWait(scOpComplete, parentWindow, "Requesting latest verison of file...");
    return scOpSuccess;
}

//////////////////////////////////////////////////////////////////////////
bool CFileUtil::GetFileInfoFromSourceControl(const char* filename, AzToolsFramework::SourceControlFileInfo& fileInfo, QWidget* parentWindow)
{
    using namespace AzToolsFramework;

    bool scOpSuccess = false;
    bool scOpComplete = false;
    SourceControlCommandBus::Broadcast(&SourceControlCommandBus::Events::GetFileInfo, filename,
        [&fileInfo, &scOpSuccess, &scOpComplete](bool success, const SourceControlFileInfo& info)
        {
            fileInfo = info;
            scOpSuccess = success;
            scOpComplete = true;
        }
    );

    BlockAndWait(scOpComplete, parentWindow, "Getting file status...");
    return scOpSuccess;
}

// Create new directory, check if directory already exist.
static bool CheckAndCreateDirectory(const QString& path)
{
    // QFileInfo does not handle mixed separators (/ and \) gracefully, so cleaning up path
    const QString cleanPath = QDir::cleanPath(path).replace('\\', '/');
    QFileInfo fileInfo(cleanPath);
    if (fileInfo.isDir())
    {
        return true;
    }
    else if (!fileInfo.exists())
    {
        return QDir().mkpath(cleanPath);
    }
    return false;
}

static bool MoveFileReplaceExisting(const QString& existingFileName, const QString& newFileName)
{
    bool moveSuccessful = false;

    // Delete the new file if it already exists
    QFile newFile(newFileName);
    if (newFile.exists())
    {
        newFile.setPermissions(newFile.permissions() | QFile::ReadOther | QFile::WriteOther);
        newFile.remove();
    }

    // Rename the existing file if it exists
    QFile existingFile(existingFileName);
    if (existingFile.exists())
    {
        existingFile.setPermissions(existingFile.permissions() | QFile::ReadOther | QFile::WriteOther);
        moveSuccessful = existingFile.rename(newFileName);
    }

    return moveSuccessful;
}
//////////////////////////////////////////////////////////////////////////
bool CFileUtil::CreateDirectory(const char* directory)
{
    QString path = directory;
    if (GetIEditor()->GetConsoleVar("ed_lowercasepaths"))
    {
        path = path.toLower();
    }

    return CheckAndCreateDirectory(path);
}

//////////////////////////////////////////////////////////////////////////
void CFileUtil::BackupFile(const char* filename)
{
    // Make a backup of previous file.
    bool makeBackup = true;

    QString bakFilename = Path::ReplaceExtension(filename, "bak");

    // Check if backup needed.
    QFile bak(filename);
    if (bak.open(QFile::ReadOnly))
    {
        if (bak.size() <= 0)
        {
            makeBackup = false;
        }
    }
    else
    {
        makeBackup = false;
    }
    bak.close();

    if (makeBackup)
    {
        QString bakFilename2 = Path::ReplaceExtension(bakFilename, "bak2");
        MoveFileReplaceExisting(bakFilename, bakFilename2);
        MoveFileReplaceExisting(filename, bakFilename);
    }
}

//////////////////////////////////////////////////////////////////////////
void CFileUtil::BackupFileDated(const char* filename, bool bUseBackupSubDirectory /*=false*/)
{
    bool makeBackup = true;
    {
        // Check if backup needed.
        QFile bak(filename);
        if (bak.open(QFile::ReadOnly))
        {
            if (bak.size() <= 0)
            {
                makeBackup = false;
            }
        }
        else
        {
            makeBackup = false;
        }
    }

    if (makeBackup)
    {
        // Generate new filename
        time_t ltime;
        time(&ltime);
        tm today;
#if AZ_TRAIT_USE_SECURE_CRT_FUNCTIONS
        localtime_s(&today, &ltime);
#else
        today = *localtime(&ltime);
#endif

        char sTemp[128];
        strftime(sTemp, sizeof(sTemp), ".%Y%m%d.%H%M%S.", &today);
        QString bakFilename = Path::RemoveExtension(filename) + sTemp + Path::GetExt(filename);

        if (bUseBackupSubDirectory)
        {
            QString sBackupPath = Path::ToUnixPath(Path::GetPath(filename)) + QString("/backups");
            CFileUtil::CreateDirectory(sBackupPath.toUtf8().data());
            bakFilename = sBackupPath + QString("/") + Path::GetFile(bakFilename);
        }

        // Do the backup
        MoveFileReplaceExisting(filename, bakFilename);
    }
}

//////////////////////////////////////////////////////////////////////////
bool CFileUtil::Deltree(const char* szFolder, bool bRecurse)
{
    return QDir(szFolder).removeRecursively();
}

//////////////////////////////////////////////////////////////////////////
bool   CFileUtil::Exists(const QString& strPath, bool boDirectory, IFileUtil::FileDesc* pDesc)
{
    ICryPak* pIPak = GetIEditor()->GetSystem()->GetIPak();
    intptr_t                nFindHandle(NULL);
    _finddata_t         stFoundData;
    bool                        boIsDirectory(false);

    memset(&stFoundData, 0, sizeof(_finddata_t));
    nFindHandle = pIPak->FindFirst(strPath.toUtf8().data(), &stFoundData);
    // If it found nothing, no matter if it is a file or directory, it was not found.
    if (nFindHandle == -1)
    {
        return false;
    }
    pIPak->FindClose(nFindHandle);

    if (stFoundData.attrib & _A_SUBDIR)
    {
        boIsDirectory = true;
    }
    else if (pDesc)
    {
        pDesc->filename = strPath;
        pDesc->attrib = stFoundData.attrib;
        pDesc->size = stFoundData.size;
        pDesc->time_access = stFoundData.time_access;
        pDesc->time_create = stFoundData.time_create;
        pDesc->time_write = stFoundData.time_write;
    }

    // If we are seeking directories...
    if (boDirectory)
    {
        // The return value will tell us if the found element is a directory.
        return boIsDirectory;
    }
    else
    {
        // If we are not seeking directories...
        // We return true if the found element is not a directory.
        return !boIsDirectory;
    }
}

//////////////////////////////////////////////////////////////////////////
bool CFileUtil::FileExists(const QString& strFilePath, IFileUtil::FileDesc* pDesc)
{
    return Exists(strFilePath, false, pDesc);
}

//////////////////////////////////////////////////////////////////////////
bool CFileUtil::PathExists(const QString& strPath)
{
    return Exists(strPath, true);
}

bool CFileUtil::GetDiskFileSize(const char* pFilePath, uint64& rOutSize)
{
    const QFileInfo fi(pFilePath);
    rOutSize = fi.size();
    return fi.exists();
}

//////////////////////////////////////////////////////////////////////////
bool   CFileUtil::IsFileExclusivelyAccessable(const QString& strFilePath)
{
    // this was simply trying to open the file before, so keep it like that
    QFile f(strFilePath);
    return f.open(QFile::ReadOnly);
}
//////////////////////////////////////////////////////////////////////////
bool   CFileUtil::CreatePath(const QString& strPath)
{
#if defined(AZ_PLATFORM_APPLE_OSX)
    bool pathCreated = true;

    QString cleanPath = QDir::cleanPath(strPath);
    QDir path(cleanPath);
    if (!path.exists())
    {
        pathCreated = path.mkpath(cleanPath);
    }

    return pathCreated;
#else
    QString                                 strDriveLetter;
    QString                                 strDirectory;
    QString                                 strFilename;
    QString                                 strExtension;
    QString                                 strCurrentDirectoryPath;
    QStringList        cstrDirectoryQueue;
    size_t                                  nCurrentPathQueue(0);
    size_t                                  nTotalPathQueueElements(0);
    BOOL                                        bnLastDirectoryWasCreated(FALSE);

    if (PathExists(strPath))
    {
        return true;
    }

    Path::SplitPath(strPath, strDriveLetter, strDirectory, strFilename, strExtension);
    Path::GetDirectoryQueue(strDirectory, cstrDirectoryQueue);

    if (!strDriveLetter.isEmpty())
    {
        strCurrentDirectoryPath = strDriveLetter;
        strCurrentDirectoryPath += "\\";
    }


    nTotalPathQueueElements = cstrDirectoryQueue.size();
    for (nCurrentPathQueue = 0; nCurrentPathQueue < nTotalPathQueueElements; ++nCurrentPathQueue)
    {
        strCurrentDirectoryPath += cstrDirectoryQueue[nCurrentPathQueue];
        strCurrentDirectoryPath += "\\";
        // The value which will go out of this loop is the result of the attempt to create the
        // last directory, only.

        strCurrentDirectoryPath = Path::CaselessPaths(strCurrentDirectoryPath);
        bnLastDirectoryWasCreated = QDir().mkpath(strCurrentDirectoryPath);
    }

    if (!bnLastDirectoryWasCreated)
    {
        if (!QDir(strCurrentDirectoryPath).exists())
        {
            return false;
        }
    }

    return true;
#endif
}

//////////////////////////////////////////////////////////////////////////
bool   CFileUtil::DeleteFile(const QString& strPath)
{
    QFile(strPath).setPermissions(QFile::ReadOther | QFile::WriteOther);
    return QFile::remove(strPath);
}
//////////////////////////////////////////////////////////////////////////
bool CFileUtil::RemoveDirectory(const QString& strPath)
{
    return Deltree(strPath.toUtf8().data(), true);
}

void CFileUtil::ForEach(const QString& path, std::function<void(const QString&)> predicate, bool recurse)
{
    bool trailingSlash = path.endsWith('/') || path.endsWith('\\');
    const QString dirName = trailingSlash ? path.left(path.length() - 1) : path;
    QDirIterator::IteratorFlags flags = QDirIterator::NoIteratorFlags;

    if (recurse)
    {
        flags = QDirIterator::Subdirectories;
    }

    QDirIterator dirIterator(path, {"*.*"}, QDir::Files, flags);
    while (dirIterator.hasNext())
    {
        const QString fileName = QFileInfo(dirIterator.next()).fileName();
        const QString filePath = Path::ToUnixPath(Path::Make(dirName, fileName));
        predicate(filePath);
    }
}

//////////////////////////////////////////////////////////////////////////
IFileUtil::ECopyTreeResult CFileUtil::CopyTree(const QString& strSourceDirectory, const QString& strTargetDirectory, bool boRecurse, bool boConfirmOverwrite, const char* const ignoreFilesAndFolders)
{
    static CUserOptions             oFileOptions;
    static CUserOptions             oDirectoryOptions;

    CUserOptions::CUserOptionsReferenceCountHelper  oFileOptionsHelper(oFileOptions);
    CUserOptions::CUserOptionsReferenceCountHelper  oDirectoryOptionsHelper(oDirectoryOptions);

    IFileUtil::ECopyTreeResult                      eCopyResult(IFileUtil::ETREECOPYOK);

    __finddata64_t                      fd;

    intptr_t                                    hfil(0);

    QStringList            cFiles;
    QStringList            cDirectories;

    size_t                                      nCurrent(0);
    size_t                                      nTotal(0);

    // For this function to work properly, it has to first process all files in the directory AND JUST AFTER IT
    // work on the sub-folders...this is NOT OBVIOUS, but imagine the case where you have a hierarchy of folders,
    // all with the same names and all with the same files names inside. If you would make a depth-first search
    // you could end up with the files from the deepest folder in ALL your folders.

    std::vector<string> ignoredPatterns;
    StringHelpers::Split(ignoreFilesAndFolders, "|", false, ignoredPatterns);

    QDirIterator::IteratorFlags flags = QDirIterator::NoIteratorFlags;

    if (boRecurse)
    {
        flags = QDirIterator::Subdirectories;
    }

    QDirIterator dirIterator(strSourceDirectory, {"*.*"}, QDir::Files, flags);

    if (!dirIterator.hasNext())
    {
        return IFileUtil::ETREECOPYOK;
    }

    while (dirIterator.hasNext())
    {
        const QString filePath = dirIterator.next();
        const QString fileName = QFileInfo(filePath).fileName();

        bool ignored = false;
        for (const string& ignoredFile : ignoredPatterns)
        {
            if (StringHelpers::CompareIgnoreCase(fileName.toStdString().c_str(), ignoredFile.c_str()) == 0)
            {
                ignored = true;
                break;
            }
        }
        if (ignored)
        {
            continue;
        }

        QFileInfo fileInfo(filePath);
        if (fileInfo.isDir())
        {
            if (boRecurse)
            {
                cDirectories.push_back(fileName);
            }
        }
        else
        {
            cFiles.push_back(fileName);
        }
    }

    // First we copy all files (maybe not all, depending on the user options...)
    nTotal = cFiles.size();
    for (nCurrent = 0; nCurrent < nTotal; ++nCurrent)
    {
        BOOL        bnLastFileWasCopied(FALSE);
        QString name(strSourceDirectory);
        QString strTargetName(strTargetDirectory);
        name += QDir::separator();

        if (eCopyResult == IFileUtil::ETREECOPYUSERCANCELED)
        {
            return eCopyResult;
        }

        name += cFiles[nCurrent];
        strTargetName += cFiles[nCurrent];

        if (boConfirmOverwrite)
        {
            if (QFileInfo::exists(strTargetName))
            {
                // If the directory already exists...
                // we must warn our user about the possible actions.
                int             nUserOption(0);

                if (boConfirmOverwrite)
                {
                    // If the option is not valid to all folder, we must ask anyway again the user option.
                    if (!oFileOptions.IsOptionToAll())
                    {
                        const int ret = QMessageBox::question(AzToolsFramework::GetActiveWindow(),
                            QObject::tr("Confirm file overwrite?"),
                            QObject::tr("There is already a file named \"%1\" in the target folder. Do you want to move this file anyway replacing the old one?")
                                .arg(QtUtil::ToQString(fd.name)),
                            QMessageBox::YesToAll | QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);

                        switch (ret) {
                        case QMessageBox::YesToAll: /* fall-through */
                        case QMessageBox::Yes:    nUserOption = IDYES; break;
                        case QMessageBox::No:     nUserOption = IDNO; break;
                        case QMessageBox::Cancel: nUserOption = IDCANCEL; break;
                        }

                        oFileOptions.SetOption(nUserOption, ret == QMessageBox::YesToAll);
                    }
                    else
                    {
                        nUserOption = oFileOptions.GetOption();
                    }
                }

                switch (nUserOption)
                {
                case IDYES:
                {
                    // Actually, we need to do nothing in this case.
                }
                break;

                case IDNO:
                {
                    eCopyResult = IFileUtil::ETREECOPYUSERDIDNTCOPYSOMEITEMS;
                    continue;
                }
                break;

                // This IS ALWAYS for all... so it's easy to deal with.
                case IDCANCEL:
                {
                    return IFileUtil::ETREECOPYUSERCANCELED;
                }
                break;
                }
            }
        }

        bnLastFileWasCopied = QFile::copy(name, strTargetName);
        if (!bnLastFileWasCopied)
        {
            eCopyResult = IFileUtil::ETREECOPYFAIL;
        }
    }

    // Now we can recurse into the directories, if needed.
    nTotal = cDirectories.size();
    for (nCurrent = 0; nCurrent < nTotal; ++nCurrent)
    {
        QString name(strSourceDirectory);
        QString strTargetName(strTargetDirectory);
        BOOL        bnLastDirectoryWasCreated(FALSE);

        if (eCopyResult == IFileUtil::ETREECOPYUSERCANCELED)
        {
            return eCopyResult;
        }

        name += cDirectories[nCurrent];
        name += "/";

        strTargetName += cDirectories[nCurrent];
        strTargetName += "/";
        bnLastDirectoryWasCreated = QDir().mkpath(strTargetName);

        if (!bnLastDirectoryWasCreated)
        {
            if (!QDir(strTargetName).exists())
            {
                return IFileUtil::ETREECOPYFAIL;
            }
            else
            {
                // If the directory already exists...
                // we must warn our user about the possible actions.
                int             nUserOption(0);

                if (boConfirmOverwrite)
                {
                    // If the option is not valid to all folder, we must ask anyway again the user option.
                    if (!oDirectoryOptions.IsOptionToAll())
                    {
                        const int ret = QMessageBox::question(AzToolsFramework::GetActiveWindow(),
                            QObject::tr("Confirm directory overwrite?"),
                            QObject::tr("There is already a folder named \"%1\" in the target folder. Do you want to move this folder anyway?")
                                .arg(fd.name),
                            QMessageBox::YesToAll | QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);

                        switch (ret) {
                        case QMessageBox::YesToAll: /* fall-through */
                        case QMessageBox::Yes:    nUserOption = IDYES; break;
                        case QMessageBox::No:     nUserOption = IDNO; break;
                        case QMessageBox::Cancel: nUserOption = IDCANCEL; break;
                        }

                        oDirectoryOptions.SetOption(nUserOption, ret == QMessageBox::YesToAll);
                    }
                    else
                    {
                        nUserOption = oDirectoryOptions.GetOption();
                    }
                }

                switch (nUserOption)
                {
                case IDYES:
                {
                    // Actually, we need to do nothing in this case.
                }
                break;

                case IDNO:
                {
                    // If no, we just need to go to the next item.
                    eCopyResult = IFileUtil::ETREECOPYUSERDIDNTCOPYSOMEITEMS;
                    continue;
                }
                break;

                // This IS ALWAYS for all... so it's easy to deal with.
                case IDCANCEL:
                {
                    return IFileUtil::ETREECOPYUSERCANCELED;
                }
                break;
                }
            }
        }

        eCopyResult = CopyTree(name, strTargetName, boRecurse, boConfirmOverwrite, ignoreFilesAndFolders);
    }

    return eCopyResult;
}
//////////////////////////////////////////////////////////////////////////
IFileUtil::ECopyTreeResult   CFileUtil::CopyFile(const QString& strSourceFile, const QString& strTargetFile, bool boConfirmOverwrite, ProgressRoutine pfnProgress, bool* pbCancel)
{
    CUserOptions                            oFileOptions;
    IFileUtil::ECopyTreeResult                      eCopyResult(IFileUtil::ETREECOPYOK);

    BOOL                                            bnLastFileWasCopied(FALSE);
    QString                                     name(strSourceFile);
    QString                                     strQueryFilename;
    QString                                     strFullStargetName;

    QString strTargetName(strTargetFile);
    if (GetIEditor()->GetConsoleVar("ed_lowercasepaths"))
    {
        strTargetName = strTargetName.toLower();
    }

    QString strDriveLetter, strDirectory, strFilename, strExtension;
    Path::SplitPath(strTargetName, strDriveLetter, strDirectory, strFilename, strExtension);
    strFullStargetName = strDriveLetter;
    strFullStargetName += strDirectory;

    if (strFilename.isEmpty())
    {
        strFullStargetName += Path::GetFileName(strSourceFile);
        strFullStargetName += ".";
        strFullStargetName += Path::GetExt(strSourceFile);
    }
    else
    {
        strFullStargetName += strFilename;
        strFullStargetName += strExtension;
    }


    if (boConfirmOverwrite)
    {
        if (QFileInfo::exists(strFullStargetName))
        {
            strQueryFilename = strFilename;
            if (strFilename.isEmpty())
            {
                strQueryFilename = Path::GetFileName(strSourceFile);
                strQueryFilename += ".";
                strQueryFilename += Path::GetExt(strSourceFile);
            }
            else
            {
                strQueryFilename += strExtension;
            }

            // If the directory already exists...
            // we must warn our user about the possible actions.
            int             nUserOption(0);

            if (boConfirmOverwrite)
            {
                // If the option is not valid to all folder, we must ask anyway again the user option.
                if (!oFileOptions.IsOptionToAll())
                {
                    const int ret = QMessageBox::question(AzToolsFramework::GetActiveWindow(),
                        QObject::tr("Confirm file overwrite?"),
                        QObject::tr("There is already a file named \"%1\" in the target folder. Do you want to move this file anyway replacing the old one?")
                            .arg(strQueryFilename),
                        QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);

                    switch (ret) {
                    case QMessageBox::Yes:    nUserOption = IDYES; break;
                    case QMessageBox::No:     nUserOption = IDNO; break;
                    case QMessageBox::Cancel: nUserOption = IDCANCEL; break;
                    }

                    oFileOptions.SetOption(nUserOption, false);
                }
                else
                {
                    nUserOption = oFileOptions.GetOption();
                }
            }

            switch (nUserOption)
            {
            case IDYES:
            {
                // Actually, we need to do nothing in this case.
            }
            break;

            case IDNO:
            {
                return eCopyResult = IFileUtil::ETREECOPYUSERCANCELED;
            }
            break;

            // This IS ALWAYS for all... so it's easy to deal with.
            case IDCANCEL:
            {
                return IFileUtil::ETREECOPYUSERCANCELED;
            }
            break;
            }
        }
    }

    bnLastFileWasCopied = false;

    QFile source(name);
    if (source.open(QFile::ReadOnly))
    {
        QFile out(strFullStargetName);
        if (out.open(QFile::ReadWrite))
        {
            bnLastFileWasCopied = true;
            char block[4096];
            qint64 totalRead = 0;
            while (!source.atEnd())
            {
                qint64 in = source.read(block, sizeof(block));
                if (in <= 0)
                {
                    break;
                }
                totalRead += in;
                if (in != out.write(block, in))
                {
                    bnLastFileWasCopied = false;
                    break;
                }
                if (pbCancel && *pbCancel == true)
                {
                    bnLastFileWasCopied = false;
                    break;
                }
                if (pfnProgress)
                {
                    pfnProgress(source.size(), totalRead, 0, 0, 0, 0, 0, 0, 0);
                }
            }
            if (totalRead != source.size())
            {
                bnLastFileWasCopied = false;
            }
        }
    }

    if (!bnLastFileWasCopied)
    {
        eCopyResult = IFileUtil::ETREECOPYFAIL;
    }

    return eCopyResult;
}
//////////////////////////////////////////////////////////////////////////
IFileUtil::ECopyTreeResult   CFileUtil::MoveTree(const QString& strSourceDirectory, const QString& strTargetDirectory, bool boRecurse, bool boConfirmOverwrite)
{
    static CUserOptions             oFileOptions;
    static CUserOptions             oDirectoryOptions;

    CUserOptions::CUserOptionsReferenceCountHelper  oFileOptionsHelper(oFileOptions);
    CUserOptions::CUserOptionsReferenceCountHelper  oDirectoryOptionsHelper(oDirectoryOptions);

    IFileUtil::ECopyTreeResult                  eCopyResult(IFileUtil::ETREECOPYOK);

    __finddata64_t                      fd;

    intptr_t                                    hfil(0);

    QStringList            cFiles;
    QStringList            cDirectories;

    size_t                                      nCurrent(0);
    size_t                                      nTotal(0);

    // For this function to work properly, it has to first process all files in the directory AND JUST AFTER IT
    // work on the sub-folders...this is NOT OBVIOUS, but imagine the case where you have a hierarchy of folders,
    // all with the same names and all with the same files names inside. If you would make a depth-first search
    // you could end up with the files from the deepest folder in ALL your folders.

    QDirIterator::IteratorFlags flags = QDirIterator::NoIteratorFlags;

    if (boRecurse)
    {
        flags = QDirIterator::Subdirectories;
    }

    QDirIterator dirIterator(strSourceDirectory, {"*.*"}, QDir::Files, flags);

    if (!dirIterator.hasNext())
    {
        return IFileUtil::ETREECOPYOK;
    }

    while (dirIterator.hasNext())
    {
        const QString filePath = dirIterator.next();
        const QString fileName = QFileInfo(filePath).fileName();

        QFileInfo fileInfo(filePath);
        if (fileInfo.isDir())
        {
            if (boRecurse)
            {
                cDirectories.push_back(fileName);
            }
        }
        else
        {
            cFiles.push_back(fileName);
        }
    }

    // First we copy all files (maybe not all, depending on the user options...)
    nTotal = cFiles.size();
    for (nCurrent = 0; nCurrent < nTotal; ++nCurrent)
    {
        BOOL        bnLastFileWasCopied(FALSE);
        QString name(strSourceDirectory);
        QString strTargetName(strTargetDirectory);

        if (eCopyResult == IFileUtil::ETREECOPYUSERCANCELED)
        {
            return eCopyResult;
        }

        name += cFiles[nCurrent];
        strTargetName += cFiles[nCurrent];

        if (boConfirmOverwrite)
        {
            if (QFileInfo::exists(strTargetName))
            {
                // If the directory already exists...
                // we must warn our user about the possible actions.
                int             nUserOption(0);

                if (boConfirmOverwrite)
                {
                    // If the option is not valid to all folder, we must ask anyway again the user option.
                    if (!oFileOptions.IsOptionToAll())
                    {
                        const int ret = QMessageBox::question(AzToolsFramework::GetActiveWindow(),
                            QObject::tr("Confirm file overwrite?"),
                            QObject::tr("There is already a file named \"%1\" in the target folder. Do you want to move this file anyway replacing the old one?")
                                .arg(fd.name),
                            QMessageBox::YesToAll | QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);

                        switch (ret) {
                        case QMessageBox::YesToAll: /* fall-through */
                        case QMessageBox::Yes:    nUserOption = IDYES; break;
                        case QMessageBox::No:     nUserOption = IDNO; break;
                        case QMessageBox::Cancel: nUserOption = IDCANCEL; break;
                        }

                        oFileOptions.SetOption(nUserOption, ret == QMessageBox::YesToAll);
                    }
                    else
                    {
                        nUserOption = oFileOptions.GetOption();
                    }
                }

                switch (nUserOption)
                {
                case IDYES:
                {
                    // Actually, we need to do nothing in this case.
                }
                break;

                case IDNO:
                {
                    eCopyResult = IFileUtil::ETREECOPYUSERDIDNTCOPYSOMEITEMS;
                    continue;
                }
                break;

                // This IS ALWAYS for all... so it's easy to deal with.
                case IDCANCEL:
                {
                    return IFileUtil::ETREECOPYUSERCANCELED;
                }
                break;
                }
            }
        }
        bnLastFileWasCopied = MoveFileReplaceExisting(name, strTargetName);

        if (!bnLastFileWasCopied)
        {
            eCopyResult = IFileUtil::ETREECOPYFAIL;
        }
    }

    // Now we can recurse into the directories, if needed.
    nTotal = cDirectories.size();
    for (nCurrent = 0; nCurrent < nTotal; ++nCurrent)
    {
        QString name(strSourceDirectory);
        QString strTargetName(strTargetDirectory);
        BOOL        bnLastDirectoryWasCreated(FALSE);

        if (eCopyResult == IFileUtil::ETREECOPYUSERCANCELED)
        {
            return eCopyResult;
        }

        name += cDirectories[nCurrent];
        name += "/";

        strTargetName += cDirectories[nCurrent];
        strTargetName += "/";
        bnLastDirectoryWasCreated = QDir().mkdir(strTargetName);

        if (!bnLastDirectoryWasCreated)
        {
            if (!QDir(strTargetName).exists())
            {
                return IFileUtil::ETREECOPYFAIL;
            }
            else
            {
                // If the directory already exists...
                // we must warn our user about the possible actions.
                int             nUserOption(0);

                if (boConfirmOverwrite)
                {
                    // If the option is not valid to all folder, we must ask anyway again the user option.
                    if (!oDirectoryOptions.IsOptionToAll())
                    {
                        const int ret = QMessageBox::question(AzToolsFramework::GetActiveWindow(),
                            QObject::tr("Confirm directory overwrite?"),
                            QObject::tr("There is already a folder named \"%1\" in the target folder. Do you want to move this folder anyway?")
                                .arg(fd.name),
                            QMessageBox::YesToAll | QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);

                        switch (ret) {
                        case QMessageBox::YesToAll: /* fall-through */
                        case QMessageBox::Yes:    nUserOption = IDYES; break;
                        case QMessageBox::No:     nUserOption = IDNO; break;
                        case QMessageBox::Cancel: nUserOption = IDCANCEL; break;
                        }

                        oDirectoryOptions.SetOption(nUserOption, ret == QMessageBox::YesToAll);
                    }
                    else
                    {
                        nUserOption = oDirectoryOptions.GetOption();
                    }
                }

                switch (nUserOption)
                {
                case IDYES:
                {
                    // Actually, we need to do nothing in this case.
                }
                break;

                case IDNO:
                {
                    // If no, we just need to go to the next item.
                    eCopyResult = IFileUtil::ETREECOPYUSERDIDNTCOPYSOMEITEMS;
                    continue;
                }
                break;

                // This IS ALWAYS for all... so it's easy to deal with.
                case IDCANCEL:
                {
                    return IFileUtil::ETREECOPYUSERCANCELED;
                }
                break;
                }
            }
        }

        eCopyResult = MoveTree(name, strTargetName, boRecurse, boConfirmOverwrite);
    }

    CFileUtil::RemoveDirectory(strSourceDirectory);

    return eCopyResult;
}

QString CFileUtil::PopupQMenu(const QString& filename, const QString& fullGamePath, QWidget* parent)
{
    QStringList extraItemsFront;
    return PopupQMenu(filename, fullGamePath, parent, nullptr, extraItemsFront);
}

QString CFileUtil::PopupQMenu(const QString& filename, const QString& fullGamePath, QWidget* parent, bool* pIsSelected, const QStringList& extraItemsFront)
{
    QStringList extraItemsBack;
    return PopupQMenu(filename, fullGamePath, parent, nullptr, extraItemsFront, extraItemsBack);
}

QString CFileUtil::PopupQMenu(const QString& filename, const QString& fullGamePath, QWidget* parent, bool* pIsSelected, const QStringList& extraItemsFront, const QStringList& extraItemsBack)
{
    QMenu menu;

    foreach(QString text, extraItemsFront)
    {
        if (!text.isEmpty())
        {
            menu.addAction(text);
        }
    }
    if (extraItemsFront.count())
    {
        menu.addSeparator();
    }

    PopulateQMenu(parent, &menu, filename, fullGamePath, pIsSelected);
    if (extraItemsBack.count())
    {
        menu.addSeparator();
    }
    foreach(QString text, extraItemsBack)
    {
        if (!text.isEmpty())
        {
            menu.addAction(text);
        }
    }

    QAction* result = menu.exec(QCursor::pos());
    return result ? result->text() : QString();
}

void CFileUtil::PopulateQMenu(QWidget* caller, QMenu* menu, const QString& filename, const QString& fullGamePath)
{
    PopulateQMenu(caller, menu, filename, fullGamePath, nullptr);
}

void CFileUtil::PopulateQMenu(QWidget* caller, QMenu* menu, const QString& filename, const QString& fullGamePath, bool* isSelected)
{
    QString fullPath;

    if (isSelected)
    {
        *isSelected = false;
    }

    if (!filename.isEmpty())
    {
        QString path = Path::MakeGamePath(fullGamePath);
        path = Path::AddSlash(path) + filename;
        fullPath = Path::GamePathToFullPath(path);
    }
    else
    {
        fullPath = fullGamePath;
    }

    uint32 nFileAttr = CFileUtil::GetAttributes(fullPath.toUtf8().data());

    QAction* action;

    // NOTE: isSelected being passed in implies that the menu filled from this call must have exec() called on it, and not show.
    if (isSelected)
    {
        action = new QAction(QObject::tr("Select"), nullptr);
        QObject::connect(action, &QAction::triggered, action, [isSelected]() { *isSelected = true; });
        if (menu->isEmpty())
        {
            menu->addAction(action);
        }
        else
        {
            menu->insertAction(menu->actions()[0], action);
        }
    }

#ifdef AZ_PLATFORM_WINDOWS
    const char* exploreActionName = "Open in Explorer";
#elif defined(AZ_PLATFORM_APPLE_OSX)
    const char* exploreActionName = "Open in Finder";
#else
    const char* exploreActionName = "Open in file browser";
#endif
    action = menu->addAction(QObject::tr(exploreActionName), [=]()
    {
        if (nFileAttr & SCC_FILE_ATTRIBUTE_INPAK)
        {
            QString path = QDir::toNativeSeparators(Path::GetPath(fullPath));
            QDesktopServices::openUrl(QUrl::fromLocalFile(path));
        }
        else
        {
            AzQtComponents::ShowFileOnDesktop(fullPath);
        }
    });
    action->setDisabled(nFileAttr & SCC_FILE_ATTRIBUTE_INPAK);

    action = menu->addAction(QObject::tr("Copy Name To Clipboard"), [=]()
    {
        if (filename.isEmpty())
        {
            QFileInfo fi(fullGamePath);
            QString file = fi.completeBaseName();
            QApplication::clipboard()->setText(file);
        }
        else
        {
            QApplication::clipboard()->setText(filename);
        }
    });

    action = menu->addAction(QObject::tr("Copy Path To Clipboard"), [fullPath]() { QApplication::clipboard()->setText(fullPath); });

    if (!filename.isEmpty() && GetIEditor()->IsSourceControlAvailable() && nFileAttr != SCC_FILE_ATTRIBUTE_INVALID)
    {
        bool isEnableSC = nFileAttr & SCC_FILE_ATTRIBUTE_MANAGED;
        bool isInPak = nFileAttr & SCC_FILE_ATTRIBUTE_INPAK;
        menu->addSeparator();
        if (isInPak && !isEnableSC)
        {
            menu->addAction(QObject::tr("File In Pak (Read Only)"));
            menu->setDisabled(true);
        }
        else
        {
            action = menu->addAction(QObject::tr("Check Out"), [fullPath, caller]()
            {
                if (!CheckoutFile(fullPath.toUtf8().data(), caller))
                {
                    QMessageBox::warning(caller, QObject::tr("Error"),
                        QObject::tr("Source Control Check Out Failed.\r\nCheck if Source Control Provider is correctly setup and working directory is correct."));
                }
            });
            action->setEnabled(isEnableSC && !isInPak && (nFileAttr & SCC_FILE_ATTRIBUTE_READONLY));

            action = menu->addAction(QObject::tr("Undo Check Out"), [fullPath, caller]()
            {
                if (!RevertFile(fullPath.toUtf8().data(), caller))
                {
                    QMessageBox::warning(caller, QObject::tr("Error"),
                        QObject::tr("Source Control Undo Check Out Failed.\r\nCheck if Source Control Provider is correctly setup and working directory is correct."));
                }
            });
            action->setEnabled(isEnableSC && !isInPak && (nFileAttr & SCC_FILE_ATTRIBUTE_CHECKEDOUT));

            action = menu->addAction(QObject::tr("Get Latest Version"), [fullPath, caller]()
            {
                if (GetIEditor()->IsSourceControlAvailable())
                {
                    if (!CFileUtil::GetLatestFromSourceControl(fullPath.toUtf8().data(), caller))
                    {
                        QMessageBox::warning(caller, QObject::tr("Error"),
                            QObject::tr("Source Control failed to get latest.\r\nCheck if Source Control Provider is correctly setup and working directory is correct."));
                    }
                }
            });
            action->setEnabled(isEnableSC);

            action = menu->addAction(QObject::tr("Add To Source Control"), [fullPath, caller]()
            {
                if (!CheckoutFile(fullPath.toUtf8().data(), caller))
                {
                    QMessageBox::warning(caller, QObject::tr("Error"),
                        QObject::tr("Source Control Add Failed.\r\nCheck if Source Control Provider is correctly setup and working directory is correct."));
                }
            });
            action->setDisabled(isEnableSC);
        }
    }
}

void CFileUtil::GatherAssetFilenamesFromLevel(std::set<QString>& rOutFilenames, bool bMakeLowerCase, bool bMakeUnixPath)
{
    rOutFilenames.clear();
    CBaseObjectsArray objArr;
    CUsedResources usedRes;
    IMaterialManager* pMtlMan = GetIEditor()->Get3DEngine()->GetMaterialManager();
    IParticleManager* pPartMan = GetIEditor()->Get3DEngine()->GetParticleManager();

    GetIEditor()->GetObjectManager()->GetObjects(objArr);

    for (size_t i = 0, iCount = objArr.size(); i < iCount; ++i)
    {
        CBaseObject* pObj = objArr[i];

        usedRes.files.clear();
        pObj->GatherUsedResources(usedRes);

        for (CUsedResources::TResourceFiles::iterator iter = usedRes.files.begin(); iter != usedRes.files.end(); ++iter)
        {
            QString tmpStr = (*iter);

            if (bMakeLowerCase)
            {
                tmpStr = tmpStr.toLower();
            }

            if (bMakeUnixPath)
            {
                tmpStr = Path::ToUnixPath(tmpStr);
            }

            rOutFilenames.insert(tmpStr);
        }
    }

    uint32 mtlCount = 0;

    pMtlMan->GetLoadedMaterials(NULL, mtlCount);

    if (mtlCount > 0)
    {
        std::vector<_smart_ptr<IMaterial>> arrMtls;

        arrMtls.resize(mtlCount);
        pMtlMan->GetLoadedMaterials(&arrMtls, mtlCount);

        for (size_t i = 0; i < mtlCount; ++i)
        {
            _smart_ptr<IMaterial> pMtl = arrMtls[i];

            size_t subMtls = pMtl->GetSubMtlCount();

            // for the main material
            IRenderShaderResources* pShaderRes = pMtl->GetShaderItem().m_pShaderResources;

            // add the material filename
            rOutFilenames.insert(pMtl->GetName());

            if (pShaderRes)
            {
                for ( auto iter= pShaderRes->GetTexturesResourceMap()->begin() ;
                    iter!= pShaderRes->GetTexturesResourceMap()->end() ; ++iter )
                {
                    SEfResTexture*  pTex = &(iter->second);
                    // add the texture filename
                    rOutFilenames.insert(pTex->m_Name.c_str());
                }
            }

            // for the submaterials
            for (size_t s = 0; s < subMtls; ++s)
            {
                _smart_ptr<IMaterial> pSubMtl = pMtl->GetSubMtl(s);

                // fill up dependencies
                if (pSubMtl)
                {
                    IRenderShaderResources* pShaderRes = pSubMtl->GetShaderItem().m_pShaderResources;

                    rOutFilenames.insert(pSubMtl->GetName());

                    if (pShaderRes)
                    {
                        for (auto iter = pShaderRes->GetTexturesResourceMap()->begin();
                            iter != pShaderRes->GetTexturesResourceMap()->end(); ++iter)
                        {
                            SEfResTexture*  pTex = &(iter->second);
                            rOutFilenames.insert(pTex->m_Name.c_str());
                        }
                    }
                }
            }
        }
    }
}

uint32 CFileUtil::GetAttributes(const char* filename, bool bUseSourceControl /*= true*/)
{
    using namespace AzToolsFramework;

    bool scOpSuccess = false;
    SourceControlFileInfo fileInfo;

    if (bUseSourceControl)
    {
        using SCRequest = SourceControlConnectionRequestBus;

        SourceControlState state = SourceControlState::Disabled;
        SCRequest::BroadcastResult(state, &SCRequest::Events::GetSourceControlState);

        if (state == SourceControlState::ConfigurationInvalid)
        {
            return SCC_FILE_ATTRIBUTE_INVALID;
        }

        if (state == SourceControlState::Active)
        {
            using SCCommand = SourceControlCommandBus;

            bool scOpComplete = false;
            SCCommand::Broadcast(&SCCommand::Events::GetFileInfo, filename,
                [&fileInfo, &scOpSuccess, &scOpComplete](bool success, const SourceControlFileInfo& info)
                {
                    fileInfo = info;
                    scOpSuccess = success;
                    scOpComplete = true;
                }
            );

            BlockAndWait(scOpComplete, nullptr, "Getting file status...");

            // we intended to use source control, but the operation failed.
            // do not fall through to checking as if bUseSourceControl was false
            if (!scOpSuccess)
            {
                return SCC_FILE_ATTRIBUTE_INVALID;
            }
        }
    }

    CCryFile file;
    bool isCryFile = file.Open(filename, "rb");

    // Using source control and our fstat succeeded.
    // Translate SourceControlStatus to (legacy) ESccFileAttributes
    if (scOpSuccess)
    {
        uint32 sccFileAttr = AZ::IO::SystemFile::Exists(filename) ? SCC_FILE_ATTRIBUTE_NORMAL : SCC_FILE_ATTRIBUTE_INVALID;

        if (fileInfo.HasFlag(SourceControlFlags::SCF_Tracked))
        {
            sccFileAttr |= SCC_FILE_ATTRIBUTE_MANAGED;
        }

        if (fileInfo.HasFlag(SourceControlFlags::SCF_OpenByUser))
        {
            sccFileAttr |= SCC_FILE_ATTRIBUTE_MANAGED | SCC_FILE_ATTRIBUTE_CHECKEDOUT;
        }

        if ((sccFileAttr & SCC_FILE_ATTRIBUTE_MANAGED) == SCC_FILE_ATTRIBUTE_MANAGED)
        {
            if (fileInfo.HasFlag(SourceControlFlags::SCF_OutOfDate))
            {
                sccFileAttr |= SCC_FILE_ATTRIBUTE_NOTATHEAD;
            }

            if (fileInfo.HasFlag(SourceControlFlags::SCF_OtherOpen))
            {
                sccFileAttr |= SCC_FILE_ATTRIBUTE_CHECKEDOUT | SCC_FILE_ATTRIBUTE_BYANOTHER;
            }

            if (fileInfo.HasFlag(SourceControlFlags::SCF_PendingAdd))
            {
                sccFileAttr |= SCC_FILE_ATTRIBUTE_ADD;
            }
        }

        if (fileInfo.IsReadOnly())
        {
            sccFileAttr |= SCC_FILE_ATTRIBUTE_READONLY;
        }

        if (file.IsInPak())
        {
            sccFileAttr |= SCC_FILE_ATTRIBUTE_READONLY | SCC_FILE_ATTRIBUTE_INPAK;
        }

        return sccFileAttr;
    }

    // We've asked not to use source control OR we disabled source control
    if (!isCryFile)
    {
        return SCC_FILE_ATTRIBUTE_INVALID;
    }

    if (file.IsInPak())
    {
        return SCC_FILE_ATTRIBUTE_READONLY | SCC_FILE_ATTRIBUTE_INPAK;
    }

    DWORD dwAttrib = ::GetFileAttributes(file.GetAdjustedFilename());
    if (dwAttrib == INVALID_FILE_ATTRIBUTES)
    {
        return SCC_FILE_ATTRIBUTE_INVALID;
    }

    if (dwAttrib & FILE_ATTRIBUTE_READONLY)
    {
        return SCC_FILE_ATTRIBUTE_NORMAL | SCC_FILE_ATTRIBUTE_READONLY;
    }

    return SCC_FILE_ATTRIBUTE_NORMAL;
}

bool CFileUtil::CompareFiles(const QString& strFilePath1, const QString& strFilePath2)
{
    // Get the size of both files.  If either fails we say they are different (most likely one doesn't exist)
    uint64 size1, size2;
    if (!GetDiskFileSize(strFilePath1.toUtf8().data(), size1) || !GetDiskFileSize(strFilePath2.toUtf8().data(), size2))
    {
        return false;
    }

    // If the files are different sizes return false
    if (size1 != size2)
    {
        return false;
    }

    // Sizes are the same, we need to compare the bytes.  Try to open both files for read.
    CCryFile file1, file2;
    if (!file1.Open(strFilePath1.toUtf8().data(), "rb") || !file2.Open(strFilePath2.toUtf8().data(), "rb"))
    {
        return false;
    }

    const uint64 bufSize = 4096;

    char buf1[bufSize], buf2[bufSize];

    for (uint64 i = 0; i < size1; i += bufSize)
    {
        size_t amtRead1 = file1.ReadRaw(buf1, bufSize);
        size_t amtRead2 = file2.ReadRaw(buf2, bufSize);

        // Not a match if we didn't read the same amount from each file
        if (amtRead1 != amtRead2)
        {
            return false;
        }

        // Not a match if we didn't read the amount of data we expected
        if (amtRead1 != bufSize && i + amtRead1 != size1)
        {
            return false;
        }

        // Not a match if the buffers aren't the same
        if (memcmp(buf1, buf2, amtRead1) != 0)
        {
            return false;
        }
    }

    return true;
}

bool CFileUtil::SortAscendingFileNames(const IFileUtil::FileDesc& desc1, const IFileUtil::FileDesc& desc2)
{
    return desc1.filename.compare(desc2.filename) == -1 ? true : false;
}

bool CFileUtil::SortDescendingFileNames(const IFileUtil::FileDesc& desc1, const IFileUtil::FileDesc& desc2)
{
    return desc1.filename.compare(desc2.filename) == 1 ? true : false;
}

bool CFileUtil::SortAscendingDates(const IFileUtil::FileDesc& desc1, const IFileUtil::FileDesc& desc2)
{
    return desc1.time_write < desc2.time_write;
}

bool CFileUtil::SortDescendingDates(const IFileUtil::FileDesc& desc1, const IFileUtil::FileDesc& desc2)
{
    return desc1.time_write > desc2.time_write;
}

bool CFileUtil::SortAscendingSizes(const IFileUtil::FileDesc& desc1, const IFileUtil::FileDesc& desc2)
{
    return desc1.size > desc2.size;
}

bool CFileUtil::SortDescendingSizes(const IFileUtil::FileDesc& desc1, const IFileUtil::FileDesc& desc2)
{
    return desc1.size < desc2.size;
}


bool CFileUtil::IsAbsPath(const QString& filepath)
{
    return (!filepath.isEmpty() && ((filepath[1] == ':' && (filepath[2] == '\\' || filepath[2] == '/')
                          || (filepath[0] == '\\' || filepath[0] == '/'))));
}

CTempFileHelper::CTempFileHelper(const char* pFileName)
{
    char resolvedPath[AZ_MAX_PATH_LEN] = { 0 };
    AZ::IO::FileIOBase::GetDirectInstance()->ResolvePath(pFileName, resolvedPath, AZ_MAX_PATH_LEN);
    m_fileName = QString::fromUtf8(resolvedPath);

    m_tempFileName = Path::ReplaceExtension(m_fileName, "tmp");

    CFileUtil::DeleteFile(m_tempFileName);
}

CTempFileHelper::~CTempFileHelper()
{
    CFileUtil::DeleteFile(m_tempFileName);
}

bool CTempFileHelper::UpdateFile(bool bBackup)
{
    // First, check if the files are actually different
    if (!CFileUtil::CompareFiles(m_tempFileName, m_fileName))
    {
        // If the file changed, make sure the destination file is writable
        if (!CFileUtil::OverwriteFile(m_fileName))
        {
            CFileUtil::DeleteFile(m_tempFileName);
            return false;
        }

        // Back up the current file if requested
        if (bBackup)
        {
            CFileUtil::BackupFile(m_fileName.toUtf8().data());
        }

        // Move the temp file over the top of the destination file
        return MoveFileReplaceExisting(m_tempFileName, m_fileName);
    }
    // If the files are the same, just delete the temp file and return.
    else
    {
        CFileUtil::DeleteFile(m_tempFileName);
        return true;
    }
}

