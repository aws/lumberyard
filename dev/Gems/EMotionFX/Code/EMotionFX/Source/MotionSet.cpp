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

#include "MotionSet.h"
#include <AzFramework/StringFunc/StringFunc.h>
#include "MotionManager.h"
#include "Motion.h"
#include "EventManager.h"
#include "Importer/Importer.h"
#include <MCore/Source/DiskFile.h>
#include <MCore/Source/LogManager.h>
#include <MCore/Source/IDGenerator.h>
#include <MCore/Source/UnicodeStringIterator.h>
#include <MCore/Source/UnicodeCharacter.h>


namespace EMotionFX
{
    MotionSetCallback::MotionSetCallback()
        : BaseObject()
    {
        m_motionSet = nullptr;
    }


    MotionSetCallback::MotionSetCallback(MotionSet* motionSet)
        : BaseObject()
    {
        m_motionSet = motionSet;
    }


    MotionSetCallback::~MotionSetCallback()
    {
    }


    MotionSetCallback* MotionSetCallback::Create()
    {
        return new MotionSetCallback();
    }


    MotionSetCallback* MotionSetCallback::Create(MotionSet* motionSet)
    {
        return new MotionSetCallback(motionSet);
    }


    Motion* MotionSetCallback::LoadMotion(MotionSet::MotionEntry* entry)
    {
        AZ_Assert(m_motionSet, "Motion set is nullptr.");

        // Get the full file name and file extension.
        const AZStd::string filename = m_motionSet->ConstructMotionFilename(entry);

        // Check what type of file to load.
        Motion* motion = nullptr;
        motion = (Motion*)GetImporter().LoadSkeletalMotion(filename.c_str(), nullptr); // TODO: what about motion load settings?

        // Use the filename without path as motion name.
        if (motion)
        {
            AZStd::string motionName;
            AzFramework::StringFunc::Path::GetFileName(filename.c_str(), motionName);
            motion->SetName(motionName.c_str());
        }

        return motion;
    }


    //--------------------------------------------------------------------------------------------------------

    MotionSet::MotionEntry::MotionEntry()
        : BaseObject()
    {
        m_stringId   = MCORE_INVALIDINDEX32;
        m_motion     = nullptr;
        m_loadFailed = false;
    }


    MotionSet::MotionEntry::MotionEntry(const char* fileName, AZ::u32 stringId, Motion* motion)
        : BaseObject()
    {
        m_filename   = fileName;
        m_motion     = motion;
        m_loadFailed = false;
        m_stringId   = stringId;
    }


    MotionSet::MotionEntry::~MotionEntry()
    {
        // if the motion is owned by the runtime, is going to be deleted by the asset system
        if (m_motion && !m_motion->GetIsOwnedByRuntime())
        {
            m_motion->Destroy();
        }
    }


    MotionSet::MotionEntry* MotionSet::MotionEntry::Create()
    {
        return new MotionEntry();
    }


    MotionSet::MotionEntry* MotionSet::MotionEntry::Create(const char* fileName, const char* id, Motion* motion)
    {
        const AZ::u32 stringId = MCore::GetAzStringIdGenerator().GenerateIdForString(id);
        return new MotionEntry(fileName, stringId, motion);
    }


    const AZStd::string& MotionSet::MotionEntry::GetID() const
    {
        if (m_stringId == MCORE_INVALIDINDEX32)
        {
            return MCore::GetAzStringIdGenerator().GetStringById(0);
        }
        return MCore::GetAzStringIdGenerator().GetStringById(m_stringId);
    }


    // Check if the given filename is absolute or relative.
    bool MotionSet::MotionEntry::CheckIfIsAbsoluteFilename(const AZStd::string& filename)
    {
        // In the following cases the filename is absolute.
        if (AzFramework::StringFunc::Path::HasDrive(filename.c_str()))
        {
            // Absolute filename.
            return true;
        }

        // Relative filename.
        return false;
    }


    // Check if the motion entry filename is absolute or relative.
    bool MotionSet::MotionEntry::CheckIfIsAbsoluteFilename() const
    {
        return CheckIfIsAbsoluteFilename(m_filename);
    }

    //----------------------------------------------------------------------------------------

    MotionSet::MotionSet(const char* name, MotionSet* parent)
        : BaseObject()
    {
        m_parentSet          = parent;
        m_id                 = MCore::GetIDGenerator().GenerateID();
        m_autoUnregister     = true;
        m_dirtyFlag          = false;
        m_callback           = MotionSetCallback::Create(this);

#if defined(EMFX_DEVELOPMENT_BUILD)
        m_isOwnedByRuntime   = false;
#endif // EMFX_DEVELOPMENT_BUILD

        SetName(name);

        // Automatically register the motion set.
        GetMotionManager().AddMotionSet(this);

        //
        GetEventManager().OnCreateMotionSet(this);
    }


    MotionSet::~MotionSet()
    {
        GetEventManager().OnDeleteMotionSet(this);

        // Automatically unregister the motion set.
        if (m_autoUnregister)
        {
            GetMotionManager().Lock();
            GetMotionManager().RemoveMotionSet(this, false);
            GetMotionManager().Unlock();
        }

        // Remove all motion entries from the set.
        Clear();

        // Remove the callback.
        if (m_callback)
        {
            m_callback->Destroy();
        }

        // Get rid of the child motion sets.
        for (MotionSet* childSet : m_childSets)
        {
            childSet->Destroy();
        }
        m_childSets.clear();
    }


    MotionSet* MotionSet::Create(const char* name, MotionSet* parent)
    {
        return new MotionSet(name, parent);
    }


    // Constuct a full valid path for a given motion file.
    AZStd::string MotionSet::ConstructMotionFilename(const MotionEntry* motionEntry)
    {
        MCore::LockGuardRecursive lock(m_mutex);

        // Is the motion entry filename empty?
        if (motionEntry->GetFilenameString().empty())
        {
            return AZStd::string();
        }

        // Check if the motion entry stores an absolute path and use that directly in this case.
        if (motionEntry->CheckIfIsAbsoluteFilename())
        {
            return motionEntry->GetFilename();
        }

        AZStd::string mediaRootFolder = GetEMotionFX().GetMediaRootFolder();
        AZ_Error("MotionSet", !mediaRootFolder.empty(), "No media root folder set. Cannot load file for motion entry '%s'.", motionEntry->GetFilename());

        // Construct the absolute path based on the current media root folder and the relative filename of the motion entry.
        return mediaRootFolder + motionEntry->GetFilename();
    }


    // Remove all motion entries from the set.
    void MotionSet::Clear()
    {
        MCore::LockGuardRecursive lock(m_mutex);

        for (const auto& item : m_motionEntries)
        {
            MotionEntry* motionEntry = item.second;
            motionEntry->Destroy();
        }

        m_motionEntries.clear();
    }


    void MotionSet::AddMotionEntry(MotionEntry* motionEntry)
    {
        MCore::LockGuardRecursive lock(m_mutex);
        m_motionEntries.insert(AZStd::make_pair<AZ::u32, MotionEntry*>(motionEntry->GetStringID(), motionEntry));
    }


    // Add a motion set as child set.
    void MotionSet::AddChildSet(MotionSet* motionSet)
    {
        MCore::LockGuardRecursive lock(m_mutex);
        m_childSets.push_back(motionSet);
    }


    // Build a list of unique string id values from all motion set entries.
    void MotionSet::BuildIdStringList(AZStd::vector<AZStd::string>& idStrings) const
    {
        MCore::LockGuardRecursive lock(m_mutex);

        // Iterate through all entries and add their unique id strings to the given list.
        const size_t numMotionEntries = m_motionEntries.size();
        idStrings.reserve(numMotionEntries);

        for (const auto& item : m_motionEntries)
        {
            MotionEntry* motionEntry = item.second;

            const AZStd::string& idString = motionEntry->GetID();
            if (idString.empty())
            {
                continue;
            }

            idStrings.push_back(idString);
        }
    }


    const MotionSet::EntryMap& MotionSet::GetMotionEntries() const
    {
        return m_motionEntries;
    }


    void MotionSet::ReserveMotionEntries(uint32 numMotionEntries)
    {
        MCore::LockGuardRecursive lock(m_mutex);

        // Not supported yet by the AZStd::unordered_map.
        //m_motionEntries.reserve(numMotionEntries);
        MCORE_UNUSED(numMotionEntries);
    }


    // Find the motion entry for a given motion.
    MotionSet::MotionEntry* MotionSet::FindMotionEntry(Motion* motion) const
    {
        MCore::LockGuardRecursive lock(m_mutex);

        for (const auto& item : m_motionEntries)
        {
            MotionEntry* motionEntry = item.second;

            if (motionEntry->GetMotion() == motion)
            {
                return motionEntry;
            }
        }

        return nullptr;
    }


    void MotionSet::RemoveMotionEntry(MotionEntry* motionEntry)
    {
        MCore::LockGuardRecursive lock(m_mutex);

        motionEntry->Destroy();
        m_motionEntries.erase(motionEntry->GetStringID());
    }


    // Remove child set with the given id from the motion set.
    void MotionSet::RemoveChildSetByID(uint32 childSetID)
    {
        MCore::LockGuardRecursive lock(m_mutex);

        const size_t numChildSets = m_childSets.size();
        for (size_t i = 0; i < numChildSets; ++i)
        {
            // Compare the ids and remove the child if we found it.
            if (m_childSets[i]->GetID() == childSetID)
            {
                m_childSets.erase(m_childSets.begin() + i);
                return;
            }
        }
    }


    // Recursively find the root motion set.
    MotionSet* MotionSet::FindRootMotionSet() const
    {
        MCore::LockGuardRecursive lock(m_mutex);

        // Are we dealing with a root motion set already?
        if (!m_parentSet)
        {
            return const_cast<MotionSet*>(this);
        }

        return m_parentSet->FindRootMotionSet();
    }


    // Find the motion entry by a given string.
    MotionSet::MotionEntry* MotionSet::FindMotionEntryByStringID(const char* motionId) const
    {
        MCore::LockGuardRecursive lock(m_mutex);

        const AZ::u32 key = MCore::GetAzStringIdGenerator().GenerateIdForString(motionId);

        const auto& iterator = m_motionEntries.find(key);
        if (iterator == m_motionEntries.end())
        {
            return nullptr;
        }

        return iterator->second;
    }


    // Find the motion entry by string ID.
    MotionSet::MotionEntry* MotionSet::RecursiveFindMotionEntryByStringID(const char* stringID) const
    {
        MCore::LockGuardRecursive lock(m_mutex);

        // Is there a motion entry with the given id in the current motion set?
        MotionEntry* entry = FindMotionEntryByStringID(stringID);
        if (entry)
        {
            return entry;
        }

        // Recursively ask the parent motion sets in case we haven't found the motion id in the current motion set.
        if (m_parentSet)
        {
            return m_parentSet->FindMotionEntryByStringID(stringID);
        }

        // Motion id not found.
        return nullptr;
    }



    // Find motion by string ID.
    Motion* MotionSet::RecursiveFindMotionByStringID(const char* stringID, bool loadOnDemand) const
    {
        MCore::LockGuardRecursive lock(m_mutex);

        if (!stringID)
        {
            return nullptr;
        }

        MotionEntry* entry = RecursiveFindMotionEntryByStringID(stringID);
        if (!entry)
        {
            return nullptr;
        }

        Motion* motion = entry->GetMotion();
        if (loadOnDemand)
        {
            motion = LoadMotion(entry);
        }
        /*
            if (motion == nullptr && loadOnDemand && entry->GetLoadingFailed() == false)    // if loading on demand is enabled and the motion hasn't loaded yet
            {
                // TODO: and replace this with a loading callback

                // get the full file name and file extension
                const AZStd::string fileName  = ConstructMotionFilename( entry );
                const AZStd::string extension = fileName.ExtractFileExtension();

                // check what type of file to load
                if (extension.CheckIfIsEqualNoCase("motion"))
                    motion = (Motion*)GetImporter().LoadSkeletalMotion( fileName.AsChar(), nullptr );   // TODO: what about motion load settings?
                else
                if (extension.CheckIfIsEqualNoCase("xpm"))
                    motion = (Motion*)GetImporter().LoadMorphMotion( fileName.AsChar(), nullptr );  // TODO: same here, what about the importer loading settings
                else
                    AZ_Warning("EMotionFX", "MotionSet - Loading on demand of motion '%s' (id=%s) failed as the file extension isn't known.", fileName.AsChar(), stringID);

                // mark that we already tried to load this motion, so that we don't retry this next time
                if (motion == nullptr)
                    entry->SetLoadingFailed( true );
                else
                    motion->SetName( fileName.AsChar() );

                entry->SetMotion( motion );
            }
        */
        return motion;
    }


    void MotionSet::SetMotionEntryId(MotionEntry* motionEntry, const char* newId)
    {
        AZ_Assert(newId, "Motion id must not be nullptr.");
        const AZ::u32 oldStringId = motionEntry->GetStringID();
        const AZ::u32 newStringId = MCore::GetAzStringIdGenerator().GenerateIdForString(newId);

        motionEntry->SetStringId(newStringId);

        // Update the hash-map.
        m_motionEntries.erase(oldStringId);
        m_motionEntries[newStringId] = motionEntry;
    }


    // Adjust the dirty flag.
    void MotionSet::SetDirtyFlag(bool dirty)
    {
        MCore::LockGuardRecursive lock(m_mutex);
        m_dirtyFlag = dirty;
    }


    Motion* MotionSet::LoadMotion(MotionEntry* entry) const
    {
        MCore::LockGuardRecursive lock(m_mutex);
        Motion* motion = entry->GetMotion();

        // If loading on demand is enabled and the motion hasn't loaded yet.
        if (!motion && !entry->GetFilenameString().empty() && !entry->GetLoadingFailed())
        {
            motion = m_callback->LoadMotion(entry);

            // Mark that we already tried to load this motion, so that we don't retry this next time.
            if (!motion)
            {
                entry->SetLoadingFailed(true);
            }

            entry->SetMotion(motion);
        }

        return motion;
    }


    // Pre-load all motions.
    void MotionSet::Preload()
    {
        MCore::LockGuardRecursive lock(m_mutex);

        // Pre-load motions for all motion entries.
        for (const auto& item : m_motionEntries)
        {
            MotionEntry* motionEntry = item.second;

            if (motionEntry->GetFilenameString().empty())
            {
                continue;
            }

            LoadMotion(motionEntry);
        }

        // Pre-load all child sets.
        for (MotionSet* childSet : m_childSets)
        {
            childSet->Preload();
        }
    }


    // Reload all motions.
    void MotionSet::Reload()
    {
        MCore::LockGuardRecursive lock(m_mutex);

        for (const auto& item : m_motionEntries)
        {
            MotionEntry* motionEntry = item.second;
            motionEntry->Reset();
        }

        // Pre-load the motions again.
        Preload();
    }


    // Adjust the auto unregistering from the motion manager on delete.
    void MotionSet::SetAutoUnregister(bool enabled)
    {
        MCore::LockGuardRecursive lock(m_mutex);
        m_autoUnregister = enabled;
    }


    // Do we auto unregister from the motion manager on delete?
    bool MotionSet::GetAutoUnregister() const
    {
        MCore::LockGuardRecursive lock(m_mutex);
        return m_autoUnregister;
    }


    void MotionSet::SetIsOwnedByRuntime(bool isOwnedByRuntime)
    {
#if defined(EMFX_DEVELOPMENT_BUILD)
        m_isOwnedByRuntime = isOwnedByRuntime;
#endif
    }


    bool MotionSet::GetIsOwnedByRuntime() const
    {
#if defined(EMFX_DEVELOPMENT_BUILD)
        return m_isOwnedByRuntime;
#else
        return true;
#endif
    }

    bool MotionSet::GetDirtyFlag() const
    {
        MCore::LockGuardRecursive lock(m_mutex);

        // Is the given motion set dirty?
        if (m_dirtyFlag)
        {
            return true;
        }

        // Is any of the child motion sets dirty?
        for (MotionSet* childSet : m_childSets)
        {
            if (childSet->GetDirtyFlag())
            {
                return true;
            }
        }

        // Neither the given set nor any of the child sets is dirty.
        return false;
    }


    void MotionSet::SetCallback(MotionSetCallback* callback, bool delExistingOneFromMem)
    {
        MCore::LockGuardRecursive lock(m_mutex);

        if (delExistingOneFromMem && callback)
        {
            m_callback->Destroy();
        }

        m_callback = callback;
        if (callback)
        {
            callback->SetMotionSet(this);
        }
    }


    void MotionSet::SetID(uint32 id)
    {
        MCore::LockGuardRecursive lock(m_mutex);
        m_id = id;
    }


    uint32 MotionSet::GetID() const
    {
        MCore::LockGuardRecursive lock(m_mutex);
        return m_id;
    }


    void MotionSet::SetName(const char* name)
    {
        MCore::LockGuardRecursive lock(m_mutex);
        m_name = name;
    }


    const char* MotionSet::GetName() const
    {
        MCore::LockGuardRecursive lock(m_mutex);
        return m_name.c_str();
    }


    const AZStd::string& MotionSet::GetNameString() const
    {
        MCore::LockGuardRecursive lock(m_mutex);
        return m_name;
    }


    void MotionSet::SetFilename(const char* filename)
    {
        MCore::LockGuardRecursive lock(m_mutex);
        m_filename = filename;
    }


    const char* MotionSet::GetFilename() const
    {
        MCore::LockGuardRecursive lock(m_mutex);
        return m_filename.c_str();
    }


    const AZStd::string& MotionSet::GetFilenameString() const
    {
        MCore::LockGuardRecursive lock(m_mutex);
        return m_filename;
    }


    uint32 MotionSet::GetNumChildSets() const
    {
        MCore::LockGuardRecursive lock(m_mutex);
        return static_cast<uint32>(m_childSets.size());
    }


    MotionSet* MotionSet::GetChildSet(uint32 index) const
    {
        MCore::LockGuardRecursive lock(m_mutex);
        return m_childSets[index];
    }


    MotionSet* MotionSet::GetParentSet() const
    {
        MCore::LockGuardRecursive lock(m_mutex);
        return m_parentSet;
    }


    MotionSetCallback* MotionSet::GetCallback() const
    {
        MCore::LockGuardRecursive lock(m_mutex);
        return m_callback;
    }


    void MotionSet::Log()
    {
        AZ_Printf("EMotionFX", " - MotionSet");
        AZ_Printf("EMotionFX", "     + Name = '%s'", m_name.c_str());
        AZ_Printf("EMotionFX", "     - Entries (%d)", GetNumMotionEntries());

        int nr = 0;
        for (const auto& item : m_motionEntries)
        {
            MotionEntry* motionEntry = item.second;
            AZ_Printf("EMotionFX", "         + #%d: Id='%s', Filename='%s'", nr, motionEntry->GetID().c_str(), motionEntry->GetFilename());
            nr++;
        }
    }
} // namespace EMotionFX