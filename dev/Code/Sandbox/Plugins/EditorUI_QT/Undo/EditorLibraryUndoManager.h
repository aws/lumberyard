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
#ifndef EDITORUIPLUGIN_UNDOMANAGER_H
#define EDITORUIPLUGIN_UNDOMANAGER_H

#pragma once

#include <api.h>
#include <AzToolsFramework/Undo/UndoSystem.h>
#include <Undo/ItemPreemptiveUndoCache.h>
#include <EditorUI_QTDLLBus.h>


namespace EditorUIPlugin
{
    AZ_PUSH_DISABLE_DLL_EXPORT_BASECLASS_WARNING
    class EDITOR_QT_UI_API EditorLibraryUndoManager
        : public EditorLibraryUndoRequestsBus::Handler
        , public EditorUIPlugin::LibraryItemCacheRequests::Bus::Handler
    {
    AZ_POP_DISABLE_DLL_EXPORT_BASECLASS_WARNING
    public:
        AZ_CLASS_ALLOCATOR(EditorLibraryUndoManager, AZ::SystemAllocator, 0);

        EditorLibraryUndoManager(IBaseLibraryManager* libMgr);
        ~EditorLibraryUndoManager();

        //EditorLibraryUndoRequests::Bus
        //start a batched undo which allow current sequence point have multiple sequence points as children. it also allow nest batched undos.
        void BeginUndoBatch(const char* label) override;
        void EndUndoBatch() override;

        void AddUndo(UndoSystem::URSequencePoint* seqPoint) override;
        
        void AddLibraryCreateUndo(const AZStd::string& libName) override;
        void AddLibraryDeleteUndo(const AZStd::string& libName) override;

        void AddItemUndo(const AZStd::string& libName, bool selected, int lodIdx) override;
        void AddItemGroupUndo(const AZStd::list<AZStd::string>& itemIds) override;

        ScopedLibraryMoveUndoPtr AddScopedLibraryMoveUndo(const AZStd::string& libName) override;
        ScopedLibraryModifyUndoPtr AddScopedLibraryModifyUndo(const AZStd::string& libName) override;

        ScopedSuspendUndoPtr AddScopedSuspendUndo() override;
        ScopedBatchUndoPtr AddScopedBatchUndo(const AZStd::string& label) override;

        void Suspend(bool isSuspend) override;
        bool IsSuspend() override;

        bool HasUndo() override;
        bool HasRedo() override;

        void Undo() override;
        void Redo() override;

        void Reset() override;
        
        IDataBaseItem* GetItem(const AZStd::string& itemId) override;
        IDataBaseLibrary* GetLibrary(const AZStd::string& libId) override;
        IBaseLibraryManager* GetLibraryManager() override;
        //end EditorLibraryUndoRequestsBus

        //LibraryItemCacheRequests::Bus
        void UpdateItemCache(const AZStd::string& itemId) override;
        void PurgeItemCache(const AZStd::string& itemId) override;
        class XmlNodeRef RetrieveItem(const AZStd::string& itemId) override;
        void ClearAllItemCache() override;
        //end LibraryItemCacheRequests::Bus

    protected:

        //undo stack for particle editor
        UndoSystem::UndoStack* m_undoStack;
        UndoSystem::URSequencePoint* m_curUndoBatch;
        
        //suspend counter
        int m_suspendCnt;

        //item caches
        ItemPreemptiveUndoCache* m_itemsCache;

        struct IBaseLibraryManager* m_libMgr;
    };

}

#endif //EDITORUIPLUGIN_UNDOMANAGER_H
