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


#ifndef EDITORUIQT_DLLBUS_H
#define EDITORUIQT_DLLBUS_H

#pragma once

#include <api.h>
#include <AzCore/EBus/EBus.h>
#include <AzToolsFramework/Undo/UndoSystem.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>

using namespace AzToolsFramework;


struct IBaseLibraryManager;
struct IDataBaseLibrary;
struct IDataBaseItem;

namespace EditorUIPlugin
{
    class LibraryModifyCommand;
    class LibraryMoveCommand;
    class LibraryDeleteCommand;

    class EDITOR_QT_UI_API ScopedLibraryModifyUndo
    {
    public:
        AZ_CLASS_ALLOCATOR(ScopedLibraryModifyUndo, AZ::SystemAllocator, 0);

        ScopedLibraryModifyUndo(const AZStd::string& libId);
        ~ScopedLibraryModifyUndo();
    private:
        AZStd::string m_libId;
        LibraryModifyCommand* m_modifyCmd;
        LibraryDeleteCommand* m_deleteCmd;
    };

    class EDITOR_QT_UI_API ScopedLibraryMoveUndo
    {
    public:
        AZ_CLASS_ALLOCATOR(ScopedLibraryMoveUndo, AZ::SystemAllocator, 0);

        ScopedLibraryMoveUndo(const AZStd::string& libId);
        ~ScopedLibraryMoveUndo();

    private:
        AZStd::string m_libId;
        LibraryMoveCommand* m_moveCmd;
        LibraryDeleteCommand* m_deleteCmd;
    };

    class EDITOR_QT_UI_API ScopedSuspendUndo
    {
    public:
        AZ_CLASS_ALLOCATOR(ScopedSuspendUndo, AZ::SystemAllocator, 0);

        ScopedSuspendUndo();
        ~ScopedSuspendUndo();
    private:
    };

    //use this scope if you want to have batched undos inside a fucntion
    class EDITOR_QT_UI_API ScopedBatchUndo
    {
    public:
        AZ_CLASS_ALLOCATOR(ScopedBatchUndo, AZ::SystemAllocator, 0);

        ScopedBatchUndo(const AZStd::string& label);
        ~ScopedBatchUndo();
    };

    typedef AZStd::shared_ptr<ScopedLibraryMoveUndo> ScopedLibraryMoveUndoPtr;
    typedef AZStd::shared_ptr<ScopedLibraryModifyUndo> ScopedLibraryModifyUndoPtr;
    typedef AZStd::shared_ptr<ScopedSuspendUndo> ScopedSuspendUndoPtr;
    typedef AZStd::shared_ptr<ScopedBatchUndo> ScopedBatchUndoPtr;

    class EditorLibraryUndoRequests
        : public AZ::EBusTraits
    {
    public:

        using Bus = AZ::EBus<EditorLibraryUndoRequests>;

        //////////////////////////////////////////////////////////////////////////
        // EBusTraits overrides
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        //////////////////////////////////////////////////////////////////////////

        /*!
        * Start batch undo so we can add multiple undo to one undo step.
        */
        virtual void BeginUndoBatch(const char* label) = 0;

        /*!
        * End current undo batch and save it to undo stack.
        */
        virtual void EndUndoBatch() = 0;

        /*!
        * Add a undo to stack. it could be added directly to stack or added to the current batch.
        */
        virtual void AddUndo(UndoSystem::URSequencePoint* seqPoint) = 0;
        
        /*!
        * Add a library create undo.
        * Usage: use this bus after the library was created
        */
        virtual void AddLibraryCreateUndo(const AZStd::string& libName) = 0;
        
        /*!
        * Add a library delete undo.
        * Usage: use this bus before the library is deleted
        */
        virtual void AddLibraryDeleteUndo(const AZStd::string& libName) = 0;

        /*!
        * Add an item modification undo. use lodIdx for modify a specific lod
        * Usage: use this bus after an item was modified
        */
        virtual void AddItemUndo(const AZStd::string& itemName, bool selected, int lodIdx = -1) = 0;
        
        /*!
        * This is for undo of a group of items in special circumstances, where it must apply undo and redo steps in the same order 
        * (normally batched undo and redo steps are done in opposite order).
        * The use case we have this requirement for is the lod for particle items.
        */
        virtual void AddItemGroupUndo(const AZStd::list<AZStd::string>& itemIds) = 0;

        /*!
        * Get a pointer to scoped undo for moving a library. The undo will be added when the instance deconstructs.
        * Usage: use the bus before moving the library. release the pointer when the move is done
        */
        virtual ScopedLibraryMoveUndoPtr AddScopedLibraryMoveUndo(const AZStd::string& libName) = 0;

        /*!
        * Get a pointer to scoped undo for modifying a library. The undo will be added when the instance deconstructs.
        * Usage: use the bus before library modification. release the pointer when the modification is done
        */
        virtual ScopedLibraryModifyUndoPtr AddScopedLibraryModifyUndo(const AZStd::string& libName) = 0;
        
        /*!
        * Get a pointer to scoped undo suspension. Undo will be resumed when the instance deconstructs.
        * Usage: use the bus to suspend undo. release the pointer resume undo
        */
        virtual ScopedSuspendUndoPtr AddScopedSuspendUndo() = 0;

        /*!
        * Get a pointer to scoped batch undo. Subsequent undo's will be batched until the instance deconstructs
        * Usage: use the bus to start a batched undo. release the pointer to end the batched undo
        */
        virtual ScopedBatchUndoPtr AddScopedBatchUndo(const AZStd::string& label) = 0;
                
        /*!
        * get the library item
        */
        virtual IDataBaseItem* GetItem(const AZStd::string& itemId) = 0;

        /*!
        * get the library
        */
        virtual IDataBaseLibrary* GetLibrary(const AZStd::string& libId) = 0;
        
        /*!
        * get the library manager
        */
        virtual IBaseLibraryManager* GetLibraryManager() = 0;
        
        /*!
        * If there is undo avalible
        */
        virtual bool HasUndo() = 0;

        /*!
        * If there is redo avalible
        */
        virtual bool HasRedo() = 0;

        /*!
        * Perform a undo
        */
        virtual void Undo() = 0;        

        /*!
        * Perform a redo
        */
        virtual void Redo() = 0;

        /*!
        * Suspend/resume undo recording
        */
        virtual void Suspend(bool isSuspend) = 0;

        /*!
        * Is undo suspended
        */
        virtual bool IsSuspend() = 0;

        /*!
        * Reset stack all
        */
        virtual void Reset() = 0;
    };

    /**
    * Used to notify when things in a library change that are happening due to outside influence
    * and not directly because of GUI actions the user is taking
    */
    class LibraryChangeEvents
        : public AZ::EBusTraits
    {
    public:
        
        //! The Library Manager changed the library (ie, something was loaded or selected by a script or other source)
        virtual void LibraryChangedInManager(const char* libraryName) = 0;
        using Bus = AZ::EBus<LibraryChangeEvents>;
    };
    
    class LibraryItemCacheRequests
        : public AZ::EBusTraits
    {
    public:

        using Bus = AZ::EBus<LibraryItemCacheRequests>;

        //////////////////////////////////////////////////////////////////////////
        // EBusTraits overrides
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        //////////////////////////////////////////////////////////////////////////

        /*!
        * Update a library item's cache.
        */
        virtual void UpdateItemCache(const AZStd::string& itemId) = 0;

        /*!
        * remove the cache for the library item, if there is one
        */
        virtual void PurgeItemCache(const AZStd::string& itemId) = 0;

        /*!
        * retrieve the last known state for a library item
        */
        virtual class XmlNodeRef RetrieveItem(const AZStd::string& itemId) = 0;

        /*!
        * clear all item chaches
        */
        virtual void ClearAllItemCache() = 0;

    };


    class LibraryItemUIRequests
        : public AZ::EBusTraits
    {
    public:

        using Bus = AZ::EBus<LibraryItemUIRequests>;

        //////////////////////////////////////////////////////////////////////////
        // EBusTraits overrides
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        //////////////////////////////////////////////////////////////////////////
        /*!
        * select an item.
        */
        virtual void UpdateItemUI(const AZStd::string& itemId, bool selected, int lodIdx) = 0;
        
        /*!
        * Explicitly refresh UI of an item.
        */
        virtual void RefreshItemUI() = 0;
        
    };


    struct EDITOR_QT_UI_API LibTreeExpandInfo
    {
        AZStd::string LibState;
        AZStd::string ItemState;
    };

    class LibraryPanelRequests
        : public AZ::EBusTraits
    {
    public:

        using Bus = AZ::EBus<LibraryPanelRequests>;

        //////////////////////////////////////////////////////////////////////////
        /*!
        * get a library's expand info.
        */
        virtual LibTreeExpandInfo GetLibTreeExpandInfo(const AZStd::string& libId) = 0;

        //////////////////////////////////////////////////////////////////////////
        /*!
        * get a library's expand info.
        */
        virtual void LoadLibTreeExpandInfo(const AZStd::string& libId, const LibTreeExpandInfo& expandInfo) = 0;
    };
};

typedef AZ::EBus<EditorUIPlugin::EditorLibraryUndoRequests> EditorLibraryUndoRequestsBus;
typedef AZ::EBus<EditorUIPlugin::LibraryItemCacheRequests> LibraryItemCacheRequestsBus;
typedef AZ::EBus<EditorUIPlugin::LibraryItemUIRequests> LibraryItemUIRequestsBus;

#endif //EDITORUIQT_DLLBUS_H
