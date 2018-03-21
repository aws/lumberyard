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

#include "pch.h"
#include "Strings.h"
#include <ICryAnimation.h>
#include <StringUtils.h>
#include "CharacterList.h"
#include "SkeletonContent.h"
#include "Serialization.h"
#include "EntryListImpl.h"
#include "IEditorFileMonitor.h"
#include "IEditor.h"
#include <IBackgroundTaskManager.h>
#include "Util/PathUtil.h"
#include "Serialization/BinArchive.h"
#include "GizmoSink.h"
#include "Explorer.h"
#include "ScanAndLoadFilesTask.h"
#include "Expected.h"
#include "IResourceSelectorHost.h"
#include "../../EditorCommon/ListSelectionDialog.h"
#include <QIcon>

namespace CharacterTool {
    using std::vector;

    static bool LoadFile(vector<char>* buf, const char* filename)
    {
        AZ::IO::HandleType fileHandle = gEnv->pCryPak->FOpen(filename, "rb");
        if (fileHandle == AZ::IO::InvalidHandle)
        {
            return false;
        }

        gEnv->pCryPak->FSeek(fileHandle, 0, SEEK_END);
        size_t size = gEnv->pCryPak->FTell(fileHandle);
        gEnv->pCryPak->FSeek(fileHandle, 0, SEEK_SET);

        buf->resize(size);
        bool result = gEnv->pCryPak->FRead(&(*buf)[0], size, fileHandle) == size;
        gEnv->pCryPak->FClose(fileHandle);
        return result;
    }


    const char* GetFilename(const char* path);

    static string GetFilenameBase(const char* path)
    {
        const char* name = GetFilename(path);
        const char* ext = strrchr(name, '.');
        if (ext != 0)
        {
            return string(name, ext);
        }
        else
        {
            return string(name);
        }
    }

    string GetRelativePath(const string& fullPath, bool bRelativeToGameFolder);

    bool CHRParamsLoader::Load(EntryBase* entryBase, const char* filename, LoaderContext* context)
    {
        AZStd::string fullFilePath = Path::GamePathToFullPath(filename).toUtf8().data();

        SEntry<SkeletonContent>* entry = static_cast<SEntry<SkeletonContent>*>(entryBase);

        XmlNodeRef root = GetIEditor()->GetSystem()->LoadXmlFromFile(fullFilePath.c_str());
        if (root)
        {
            entry->content.skeletonParameters.skeletonFileName = PathUtil::ReplaceExtension(filename, CRY_SKEL_FILE_EXT);
            return entry->content.skeletonParameters.LoadFromXML(root, &entry->dataLostDuringLoading);
        }
        else
        {
            return false;
        }
    }

    bool CHRParamsLoader::Save(EntryBase* entryBase, const char* filename, LoaderContext* context, string& errorString)
    {
        AZStd::string fullFilePath = Path::GamePathToFullPath(filename).toUtf8().data();

        SEntry<SkeletonContent>* entry = static_cast<SEntry<SkeletonContent>*>(entryBase);
        if (!entry->content.IsValid())
        { 
            errorString = entry->content.GetErrorString();
            return false;
        }

        XmlNodeRef root = entry->content.skeletonParameters.SaveToXML();
        if (!root)
        {
            return false;
        }

        char path[ICryPak::g_nMaxPath] = "";
        gEnv->pCryPak->AdjustFileName(fullFilePath.c_str(), path, 0);
        if (!root->saveToFile(path))
        {
            return false;
        }

        return true;
    }

    // ---------------------------------------------------------------------------

    void CHRParamsDependencies::Extract(vector<string>* paths, const EntryBase* entryBase)
    {
        const SEntry<SkeletonContent>* entry = static_cast<const SEntry<SkeletonContent>*>(entryBase);

        entry->content.GetDependencies(&*paths);
    }

    // ---------------------------------------------------------------------------


    bool CDFLoader::Load(EntryBase* entryBase, const char* filename, LoaderContext* context)
    {
        AZStd::string fullFileName = Path::GamePathToFullPath(filename).toUtf8().data();
        SEntry<CharacterContent>* entry = static_cast<SEntry<CharacterContent>*>(entryBase);
        return entry->content.cdf.LoadFromXmlFile(fullFileName.c_str());
    }

    bool CDFLoader::Save(EntryBase* entryBase, const char* filename, LoaderContext* context, string&)
    {
        AZStd::string fullFileName = Path::GamePathToFullPath(filename).toUtf8().data();
        SEntry<CharacterContent>* entry = static_cast<SEntry<CharacterContent>*>(entryBase);
        if (!entry->content.cdf.Save(fullFileName.c_str()))
        {
            return false;
        }
        return true;
    }

    // ---------------------------------------------------------------------------

    void CDFDependencies::Extract(vector<string>* paths, const EntryBase* entryBase)
    {
        const SEntry<CharacterContent>* entry = static_cast<const SEntry<CharacterContent>*>(entryBase);

        entry->content.GetDependencies(&*paths);
    }

    // ---------------------------------------------------------------------------

    void CharacterContent::GetDependencies(vector<string>* paths) const
    {
        paths->push_back(PathUtil::ReplaceExtension(cdf.skeleton, ".chrparams"));
    }

    void CharacterContent::Serialize(Serialization::IArchive& ar)
    {
        switch (engineLoadState)
        {
        case CHARACTER_NOT_LOADED:
            ar.Warning(*this, "Selected character is different from the one in the viewport.");
            break;
        case CHARACTER_INCOMPLETE:
            ar.Warning(*this, "Incomplete character definition. Clear other warnings so that this character can be loaded by the engine.");
            break;
        case CHARACTER_LOAD_FAILED:
            if (!hasDefinitionFile)
            {
                ar.Error(*this, "The engine failed to load the character.");
            }
            else
            {
                ar.Error(*this, "The engine failed to load the character. Check if specified skeleton is valid.");
            }
            break;
        default:
            break;
        }

        if (!hasDefinitionFile)
        {
            string msg = "Contains no properties.";
            ar(msg, "msg", "<!");
            return;
        }

        cdf.Serialize(ar);
    }

    // ---------------------------------------------------------------------------
    QString AttachmentNameSelector(const SResourceSelectorContext& x, const QString& previousValue, ICharacterInstance* characterInstance)
    {
        if (!characterInstance)
        {
            return previousValue;
        }

        QWidget parent(x.parentWidget);
        parent.setWindowModality(Qt::ApplicationModal);

        ListSelectionDialog dialog(&parent);
        dialog.setWindowTitle("Attachment Selection");
        dialog.setWindowIcon(QIcon(GetIEditor()->GetResourceSelectorHost()->ResourceIconPath(x.typeName)));

        IAttachmentManager* attachmentManager = characterInstance->GetIAttachmentManager();
        if (!attachmentManager)
        {
            return previousValue;
        }

        dialog.SetColumnText(0, "Name");
        dialog.SetColumnText(1, "Type");

        for (int i = 0; i < attachmentManager->GetAttachmentCount(); ++i)
        {
            IAttachment* attachment = attachmentManager->GetInterfaceByIndex(i);
            if (!attachment)
            {
                continue;
            }

            dialog.AddRow(attachment->GetName());
            int type = attachment->GetType();
            const char* typeString = type == CA_BONE ? "Bone" :
                type == CA_SKIN ? "Skin" :
                type == CA_FACE ? "Face" : "";
            dialog.AddRowColumn(typeString);
        }

        return dialog.ChooseItem(previousValue);
    }
    REGISTER_RESOURCE_SELECTOR("Attachment", AttachmentNameSelector, "Editor/Icons/animation/attachment.png")
}

#include <CharacterTool/CharacterList.moc>
