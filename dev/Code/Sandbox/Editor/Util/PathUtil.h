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

// Description : Utility functions to simplify working with paths.


#ifndef CRYINCLUDE_EDITOR_UTIL_PATHUTIL_H
#define CRYINCLUDE_EDITOR_UTIL_PATHUTIL_H
#pragma once

#include <ICryPak.h>
#if defined(AZ_PLATFORM_WINDOWS)
#include <shlwapi.h>
#endif
#include <Include/EditorCoreAPI.h>

#include <AzCore/IO/SystemFile.h> // for max path
#include <AzCore/std/string/string.h>
#include <AzFramework/StringFunc/StringFunc.h>

#include <QRegularExpression>
#include <QDir>

class QString;
class QStringList;

namespace Path
{
    //! creates an absolute path from a relative game path, used for saving game files.
    //! Example: Libs/Some/tokens.xml to c:/game/engine/GameName/Mods/ModName/Libs/Some/tokens.xml
    //! If you're not working on a mod, it will return it with the game folder prepended.
    //! This is the function you should use at all times to convert an asset ID to a full writable editor path.
    EDITOR_CORE_API AZStd::string MakeModPathFromGamePath(const char* input);

    //! Get the data folder where assets should be saved.
    //! if we're working on a mod, will return the mod's root (absolute path, with no slash at the end)
    //! if not, will return the default game root (absolute path, with no slash at the end)
    //! always returns a full path
    EDITOR_CORE_API AZStd::string GetEditingGameDataFolder();

    //! Get the root folder (in source control or other writable assets) where you should save root data.
    EDITOR_CORE_API AZStd::string GetEditingRootFolder();

    //! Set the current mod NAME for editing purposes.  After doing this the above functions will take this into account
    //! name only, please!
    EDITOR_CORE_API void SetModName(const char* input);


    //! converts path to lowercase given the cvar for ed_lowercasepaths
    inline QString CaselessPaths(const QString& strPath)
    {
        ICVar* pCvar = gEnv->pConsole->GetCVar("ed_lowercasepaths");
        if (pCvar)
        {
            int uselowercase = pCvar->GetIVal();
            if (uselowercase)
            {
                QString str = strPath;
                str = str.toLower();
                return str;
            }
        }
        return strPath;
    }

    //! Split full file name to path and filename
    //! @param filepath [IN] Full file name inclusing path.
    //! @param path [OUT] Extracted file path.
    //! @param file [OUT] Extracted file (with extension).
    inline void Split(const QString& filepath, QString& path, QString& file)
    {
        char path_buffer[_MAX_PATH];
        char drive[_MAX_DRIVE];
        char dir[_MAX_DIR];
        char fname[_MAX_FNAME];
        char ext[_MAX_EXT];
        _splitpath(filepath.toUtf8().data(), drive, dir, fname, ext);
        _makepath(path_buffer, drive, dir, 0, 0);
        path = path_buffer;
        _makepath(path_buffer, 0, 0, fname, ext);
        file = path_buffer;
    }
    inline void Split(const string& filepath, string& path, string& file)
    {
        char path_buffer[_MAX_PATH];
        char drive[_MAX_DRIVE];
        char dir[_MAX_DIR];
        char fname[_MAX_FNAME];
        char ext[_MAX_EXT];
        _splitpath(filepath, drive, dir, fname, ext);
        _makepath(path_buffer, drive, dir, 0, 0);
        path = path_buffer;
        _makepath(path_buffer, 0, 0, fname, ext);
        file = path_buffer;
    }

    //! Split full file name to path and filename
    //! @param filepath [IN] Full file name inclusing path.
    //! @param path [OUT] Extracted file path.
    //! @param filename [OUT] Extracted file (without extension).
    //! @param ext [OUT] Extracted files extension.
    inline void Split(const QString& filepath, QString& path, QString& filename, QString& fext)
    {
        char path_buffer[_MAX_PATH];
        char drive[_MAX_DRIVE];
        char dir[_MAX_DIR];
        char fname[_MAX_FNAME];
        char ext[_MAX_EXT];
        _splitpath(filepath.toUtf8().data(), drive, dir, fname, ext);
        _makepath(path_buffer, drive, dir, 0, 0);
        path = path_buffer;
        filename = fname;
        fext = ext;
    }
    inline void Split(const string& filepath, string& path, string& filename, string& fext)
    {
        char path_buffer[_MAX_PATH];
        char drive[_MAX_DRIVE];
        char dir[_MAX_DIR];
        char fname[_MAX_FNAME];
        char ext[_MAX_EXT];
        _splitpath(filepath, drive, dir, fname, ext);
        _makepath(path_buffer, drive, dir, 0, 0);
        path = path_buffer;
        filename = fname;
        fext = ext;
    }

    //! Split path into segments
    //! @param filepath [IN] path
    inline QStringList SplitIntoSegments(const QString& path)
    {
        return path.split(QRegularExpression(QStringLiteral(R"([\\/])")), QString::SkipEmptyParts);
    }

    //! Extract extension from full specified file path.
    inline QString GetExt(const QString& filepath)
    {
        char ext[_MAX_EXT];
        _splitpath(filepath.toUtf8().data(), 0, 0, 0, ext);
        if (ext[0] == '.')
        {
            return ext + 1;
        }

        return ext;
    }

    //! Extract path from full specified file path.
    inline QString GetPath(const QString& filepath)
    {
        char path_buffer[_MAX_PATH];
        char drive[_MAX_DRIVE];
        char dir[_MAX_DIR];
        _splitpath(filepath.toUtf8().data(), drive, dir, 0, 0);
        _makepath(path_buffer, drive, dir, 0, 0);
        return CaselessPaths(path_buffer);
    }

    //! Extract file name with extension from full specified file path.
    inline QString GetFile(const QString& filepath)
    {
        char path_buffer[_MAX_PATH];
        char fname[_MAX_FNAME];
        char ext[_MAX_EXT];
        _splitpath(filepath.toUtf8().data(), 0, 0, fname, ext);
        _makepath(path_buffer, 0, 0, fname, ext);
        return CaselessPaths(path_buffer);
    }

    //! Extract file name without extension from full specified file path.
    inline QString GetFileName(const QString& filepath)
    {
        char fname[_MAX_FNAME];
        _splitpath(filepath.toUtf8().data(), 0, 0, fname, 0);
        return fname;
    }

    //! add a backslash if needed
    inline QString AddBackslash(QString path)
    {
        if (path.isEmpty() || path.endsWith(QStringLiteral("\\")))
        {
            return path;
        }

        return CaselessPaths(path + "\\");
    }

    //! add a slash if needed
    inline QString AddSlash(const QString& path)
    {
        if (path.isEmpty() || path.endsWith(QStringLiteral("/")))
        {
            return path;
        }

        return CaselessPaths(path + "/");
    }

    template<size_t size>
    inline void AddBackslash(CryStackStringT<char, size>* path)
    {
        if (path->empty() || (*path)[path->size() - 1] != '\\')
        {
            (*path) += '\\';
        }
    }

    template<size_t size>
    inline void AddSlash(CryStackStringT<char, size>* path)
    {
        if (path->empty() || (*path)[path->size() - 1] != '/')
        {
            (*path) += '/';
        }
    }

    inline QString AddPathSlash(const QString& path)
    {
#if defined(AZ_PLATFORM_WINDOWS)
        return AddBackslash(path);
#else
        return AddSlash(path);
#endif
    }

    //! Replace extension for given file.
    inline QString ReplaceExtension(const QString& filepath, const QString& ext)
    {
        AZStd::string newPath = filepath.toUtf8().data();
        AzFramework::StringFunc::Path::ReplaceExtension(newPath, ext.toUtf8().data());
        QString returnString(newPath.c_str());
        return CaselessPaths(returnString);
    }

    //! Replace extension for given file.
    inline QString RemoveExtension(const QString& filepath)
    {
        char path_buffer[_MAX_PATH];
        char drive[_MAX_DRIVE];
        char dir[_MAX_DIR];
        char fname[_MAX_FNAME];
        _splitpath(filepath.toUtf8().data(), drive, dir, fname, 0);
        _makepath(path_buffer, drive, dir, fname, 0);
        return path_buffer;
    }

    //! Makes a fully specified file path from path and file name.
    inline QString Make(const QString& dir, const QString& filename, const QString& ext)
    {
        char path_buffer[_MAX_PATH];
        _makepath(path_buffer, NULL, dir.toUtf8().data(), filename.toUtf8().data(), ext.toUtf8().data());
        return CaselessPaths(path_buffer);
    }

    //////////////////////////////////////////////////////////////////////////
    EDITOR_CORE_API QString GetRelativePath(const QString& fullPath, bool bRelativeToGameFolder = false);

    //////////////////////////////////////////////////////////////////////////
    // Description:
    // given the assetID of a produced asset, constructs the full path to the SOURCE ASSET that was used to produce it.
    // Ex. Objects/box.dds will be converted to C:\Test\Game\Objects\box.tif (or bmp or whatever)
    EDITOR_CORE_API QString GamePathToFullPath(const QString& path);

    //////////////////////////////////////////////////////////////////////////
    inline QString FullPathToGamePath(const QString& path)
    {
        return CaselessPaths(GetRelativePath(path, true));
    }
    inline string FullPathToGamePath(const char* path)
    {
        return CaselessPaths(GetRelativePath(path, true)).toUtf8().data();
    }

    QString FullPathToLevelPath(const QString& path);

    //////////////////////////////////////////////////////////////////////////
    // Description:
    // Turn any path into an asset ID.

    inline QString MakeGamePath(const QString& path)
    {
        QString fullpath = Path::GamePathToFullPath(path);

        // if its in a mod, we still want the 'asset id' of it.
        QString dataFolder = Path::AddPathSlash(QString(Path::GetEditingGameDataFolder().c_str()));
        if (fullpath.length() > dataFolder.length() && QString::compare(fullpath, dataFolder, Qt::CaseInsensitive) == 0)
        {
            fullpath = fullpath.right(fullpath.length() - dataFolder.length());
            fullpath.replace('\\', '/'); // Slashes use for game files.
            return fullpath;
        }

        fullpath = GetRelativePath(path, true);
        if (fullpath.isEmpty())
        {
            fullpath = path;
        }
        fullpath.replace('\\', '/'); // Slashes use for game files.
        return CaselessPaths(fullpath);
    }


    inline QString GetAudioLocalizationFolder(bool returnAbsolutePath)
    {
        // Omit the trailing slash!
        QString sLocalizationFolder(QString(PathUtil::GetLocalizationFolder()).left(static_cast<int>(PathUtil::GetLocalizationFolder().size()) - 1));

        if (!sLocalizationFolder.isEmpty())
        {
            sLocalizationFolder = returnAbsolutePath ? (QString(Path::GetEditingGameDataFolder().c_str()) + "/" + sLocalizationFolder + "/dialog/") : sLocalizationFolder + "/dialog/";
        }
        else
        {
            gEnv->pSystem->Warning(VALIDATOR_MODULE_EDITOR, VALIDATOR_WARNING, VALIDATOR_FLAG_AUDIO, 0, "The localization folder is not set! Please make sure it is by checking the setting of cvar \"sys_localization_folder\"!");
        }

        return sLocalizationFolder;
    }

    //! Returns the aliased path to the user Sandbox folder
    EDITOR_CORE_API QString GetUserSandboxFolder();

    //! Returns the resolved, non-aliased path to the user Sandbox folder
    EDITOR_CORE_API QString GetResolvedUserSandboxFolder();

    //! Convert a path to the uniform form.
    EDITOR_CORE_API QString ToUnixPath(const QString& strPath, bool bCallCaselessPath = true);

    //! Makes a fully specified file path from path and file name.
    EDITOR_CORE_API QString Make(const QString& path, const QString& file);

    // This had to be created because _splitpath is too dumb about console drives.
    EDITOR_CORE_API void SplitPath(const QString& rstrFullPathFilename, QString& rstrDriveLetter, QString& rstrDirectory, QString& rstrFilename, QString& rstrExtension);

    // Requires a path from Splithpath: no drive letter and backslash at the end.
    EDITOR_CORE_API void GetDirectoryQueue(const QString& rstrSourceDirectory, QStringList& rcstrDirectoryTree);

    // Converts all slashes to backslashes so MS things won't complain.
    EDITOR_CORE_API void ConvertSlashToBackSlash(QString& rstrStringToConvert);

    // Converts backslashes into forward slashes.
    EDITOR_CORE_API void ConvertBackSlashToSlash(QString& rstrStringToConvert);

    // Surrounds a string with quotes if necessary. This is useful for calling other programs.
    EDITOR_CORE_API void SurroundWithQuotes(QString& rstrSurroundString);

    // This function takes a directory string input and returns the parent director string.
    EDITOR_CORE_API void GetParentDirectoryString(QString& strInputParentDirectory);

    // Gets the temporary directory path (which may not exist).
    EDITOR_CORE_API QString GetWindowsTempDirectory();

    // This function returns the full path used to run the editor.
    EDITOR_CORE_API QString GetExecutableFullPath();

    // This function returns the directory above Bin32 or Bin64 from which the editor was run (does not work with Unicode paths).
    EDITOR_CORE_API QString GetExecutableParentDirectory();

    // This function returns the directory above Bin32 or Bin64 from which the editor was run (also works with Unicode paths).
    EDITOR_CORE_API QString GetExecutableParentDirectoryUnicode();

    // This function replaces the filename from a path, keeping extension and directory/drive path.
    // WARNING: do not use the same variable in the last parameter and in any of the others.
    EDITOR_CORE_API QString& ReplaceFilename(const QString& strFilepath, const QString& strFilename, QString& strOutputFilename, bool bCallCaselessPath = true);

    //! \return true if the given path is a folder and not a file
    EDITOR_CORE_API bool IsFolder(const char* pPath);

    EDITOR_CORE_API void ConvertSlashToBackSlash(QString& str);
    EDITOR_CORE_API void ConvertBackSlashToSlash(QString& str);
    EDITOR_CORE_API QString RemoveBackslash(QString path);
};

inline QString operator /(const QString& first, const QString& second)
{
    return Path::Make(first, second);
}


#endif // CRYINCLUDE_EDITOR_UTIL_PATHUTIL_H
