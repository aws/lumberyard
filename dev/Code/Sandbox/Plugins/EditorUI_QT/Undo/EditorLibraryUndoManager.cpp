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
#include "stdafx.h"

#include <Include/IDataBaseItem.h>
#include <Include/IDataBaseLibrary.h>
#include <Include/IBaseLibraryManager.h>

#include <AzCore/std/smart_ptr/make_shared.h>

#include "EditorLibraryUndoManager.h"
#include "LibraryStateCommand.h"
#include "ItemStateCommand.h"

//#define DEBUG_PE_UNDO

#ifdef DEBUG_PE_UNDO
#include <QDebug>
#endif

namespace EditorUIPlugin
{

    ScopedLibraryModifyUndo::ScopedLibraryModifyUndo(const AZStd::string& libId)
    {
        m_libId = libId;

        m_modifyCmd = aznew LibraryModifyCommand(m_libId);
        m_modifyCmd->CaptureStart();

        m_deleteCmd = aznew LibraryDeleteCommand();
        m_deleteCmd->Capture(m_libId);
    }

    ScopedLibraryModifyUndo::~ScopedLibraryModifyUndo()
    {
        IDataBaseLibrary* lib = nullptr;
        EBUS_EVENT_RESULT(lib, EditorLibraryUndoRequestsBus, GetLibrary, m_libId);

        // If the library still exists go ahead with the Modify Undo Command.
        if (lib)
        {
            delete m_deleteCmd;
            m_modifyCmd->CaptureEnd();
            EBUS_EVENT(EditorLibraryUndoRequestsBus, AddUndo, m_modifyCmd);
        }
        // Else add the Delete Undo command, the user will be able to recover the library to the original state.
        else
        {
            delete m_modifyCmd;
            EBUS_EVENT(EditorLibraryUndoRequestsBus, AddUndo, m_deleteCmd);
        }
    }

    ScopedLibraryMoveUndo::ScopedLibraryMoveUndo(const AZStd::string& libId)
    {
        m_libId = libId;

        m_moveCmd = aznew LibraryMoveCommand(m_libId);
        m_moveCmd->CaptureStart();

        m_deleteCmd = aznew LibraryDeleteCommand();
        m_deleteCmd->Capture(m_libId);
    }

    ScopedLibraryMoveUndo::~ScopedLibraryMoveUndo()
    {
        IDataBaseLibrary* lib = nullptr;
        EBUS_EVENT_RESULT(lib, EditorLibraryUndoRequestsBus, GetLibrary, m_libId);

        // If the library still exists go ahead with the Move Undo Command.
        if (lib)
        {
            delete m_deleteCmd;
            m_moveCmd->CaptureEnd();
            EBUS_EVENT(EditorLibraryUndoRequestsBus, AddUndo, m_moveCmd);
        }
        // Else add the Delete Undo command, the user will be able to recover the library to the original state.
        else
        {
            delete m_moveCmd;
            EBUS_EVENT(EditorLibraryUndoRequestsBus, AddUndo, m_deleteCmd);
        }
    }

    ScopedSuspendUndo::ScopedSuspendUndo()
    {
        EBUS_EVENT(EditorLibraryUndoRequestsBus, Suspend, true);
    }

    ScopedSuspendUndo::~ScopedSuspendUndo()
    {
        EBUS_EVENT(EditorLibraryUndoRequestsBus, Suspend, false);
    }

    ScopedBatchUndo::ScopedBatchUndo(const AZStd::string& label)
    {
        EBUS_EVENT(EditorLibraryUndoRequestsBus, BeginUndoBatch, label.c_str());
    }

    ScopedBatchUndo::~ScopedBatchUndo()
    {
        EBUS_EVENT(EditorLibraryUndoRequestsBus, EndUndoBatch);
    }


    EditorLibraryUndoManager::EditorLibraryUndoManager(IBaseLibraryManager* libMgr)
        : m_curUndoBatch(nullptr)
        , m_libMgr(libMgr)
        , m_suspendCnt(0)
    {
        m_itemsCache = aznew ItemPreemptiveUndoCache(m_libMgr);
        //undo stack with maxinum 20 undos
        m_undoStack = new UndoSystem::UndoStack(20, nullptr);

        EditorLibraryUndoRequestsBus::Handler::BusConnect();
        EditorUIPlugin::LibraryItemCacheRequests::Bus::Handler::BusConnect();
    }

    EditorLibraryUndoManager::~EditorLibraryUndoManager()
    {
        EditorLibraryUndoRequestsBus::Handler::BusDisconnect();
        EditorUIPlugin::LibraryItemCacheRequests::Bus::Handler::BusDisconnect();
        delete m_itemsCache;
        delete m_undoStack;
        delete m_curUndoBatch;
    }

    void EditorLibraryUndoManager::BeginUndoBatch(const char* label)
    {
        if (IsSuspend())
        {
            return;
        }
        if (!m_curUndoBatch)
        {
            m_curUndoBatch = aznew UndoSystem::BatchCommand(label, 0);
        }
        else
        {
            //use tree stucture for undo batch so it can have right label for the undo operation
            UndoSystem::URSequencePoint* pCurrent = m_curUndoBatch;

            m_curUndoBatch = aznew UndoSystem::BatchCommand(label, 0);
            m_curUndoBatch->SetParent(pCurrent);
        }
    }

    void EditorLibraryUndoManager::EndUndoBatch()
    {
        //Note, if there is not a current batch then we return directly and treat this as expected.
        //There is a situation where a batch could be ended twice with no adverse effect, in CLibraryTreeView::EndRename().
        if (IsSuspend() || !m_curUndoBatch)
        {
            return;
        }

        if (m_curUndoBatch->GetParent())
        {
            // pop one up
            m_curUndoBatch = m_curUndoBatch->GetParent();
        }
        else
        {
            // we're at root
            if (m_curUndoBatch->HasRealChildren())
            {
                if (m_undoStack)
                {
#ifdef DEBUG_PE_UNDO
                    qDebug() << "ADD batch to UNDO " << m_curUndoBatch->GetName().c_str();
#endif
                    m_undoStack->Post(m_curUndoBatch);
                }
            }
            else
            {
                delete m_curUndoBatch;
            }

            m_curUndoBatch = nullptr;
        }
    }

    void EditorLibraryUndoManager::Redo()
    {
        AZ_Assert(m_curUndoBatch == nullptr, "unfinished undo recording");

        if (m_undoStack->CanRedo())
        {
            Suspend(true);
            m_undoStack->Redo();
            Suspend(false);
        }
    }

    void EditorLibraryUndoManager::Undo()
    {
        AZ_Assert(m_curUndoBatch == nullptr, "unfinished undo recording");

        if (m_undoStack->CanUndo())
        {
            Suspend(true);
            m_undoStack->Undo();
            Suspend(false);
        }
    }

    void EditorLibraryUndoManager::Reset()
    {
        AZ_Assert(m_curUndoBatch == nullptr, "Reset with unfinished undo recording");

        m_undoStack->Reset();
        ClearAllItemCache();
    }

    bool EditorLibraryUndoManager::HasUndo()
    {
        return m_undoStack->CanUndo();
    }

    bool EditorLibraryUndoManager::HasRedo()
    {
        return m_undoStack->CanRedo();
    }

    void EditorLibraryUndoManager::AddUndo(UndoSystem::URSequencePoint* sequecePoint)
    {
        if (IsSuspend())
        {
            delete sequecePoint;
            return;
        }

        if (m_curUndoBatch == nullptr)
        {
            m_undoStack->Post(sequecePoint);

#ifdef DEBUG_PE_UNDO
            qDebug() << "ADD UNDO: " << sequecePoint->GetName().c_str();
#endif
        }
        else
        {
#ifdef DEBUG_PE_UNDO
            qDebug() << "ADD to undo batch: " << sequecePoint->GetName().c_str() << " batch: " << m_curUndoBatch->GetName().c_str();
#endif
            sequecePoint->SetParent(m_curUndoBatch);
        }
    }

    void EditorLibraryUndoManager::UpdateItemCache(const AZStd::string& itemId)
    {
        m_itemsCache->UpdateCache(itemId);
    }

    void EditorLibraryUndoManager::PurgeItemCache(const AZStd::string& itemId)
    {
        m_itemsCache->PurgeCache(itemId);
    }

    XmlNodeRef EditorLibraryUndoManager::RetrieveItem(const AZStd::string& itemId)
    {
        return m_itemsCache->Retrieve(itemId);
    }

    void EditorLibraryUndoManager::ClearAllItemCache()
    {
        m_itemsCache->Clear();
    }

    IDataBaseItem* EditorLibraryUndoManager::GetItem(const AZStd::string& itemId)
    {
        return m_libMgr->FindItemByName(itemId.c_str());
    }

    IDataBaseLibrary* EditorLibraryUndoManager::GetLibrary(const AZStd::string& libId)
    {
        return m_libMgr->FindLibrary(libId.c_str());
    }

    IBaseLibraryManager* EditorLibraryUndoManager::GetLibraryManager()
    {
        return m_libMgr;
    }

    void EditorLibraryUndoManager::AddLibraryCreateUndo(const AZStd::string& libName)
    {
        if (IsSuspend())
        {
            return;
        }

        LibraryCreateCommand* command = aznew LibraryCreateCommand();
        command->Capture(libName);
        AddUndo(command);
    }

    void EditorLibraryUndoManager::AddLibraryDeleteUndo(const AZStd::string& libName)
    {
        if (IsSuspend())
        {
            return;
        }

        LibraryDeleteCommand* command = aznew LibraryDeleteCommand();
        command->Capture(libName);
        AddUndo(command);
    }

    void EditorLibraryUndoManager::AddItemUndo(const AZStd::string& itemName, bool selected, int lodIdx)
    {
        if (IsSuspend())
        {
            return;
        }
        ItemStateCommand* command = aznew ItemStateCommand(AZStd::string("Modify item ") + itemName);
        command->Capture(itemName, selected, lodIdx);
        AddUndo(command);
    }

    void EditorLibraryUndoManager::AddItemGroupUndo(const AZStd::list<AZStd::string>& itemIds)
    {
        if (IsSuspend())
        {
            return;
        }
        ItemGroupChangeCommand* command = aznew ItemGroupChangeCommand("Modify group items");
        command->Capture(itemIds);
        AddUndo(command);
    }

    ScopedLibraryMoveUndoPtr EditorLibraryUndoManager::AddScopedLibraryMoveUndo(const AZStd::string& libName)
    {
        if (IsSuspend())
        {
            return nullptr;
        }

        ScopedLibraryMoveUndoPtr undoPtr = AZStd::make_shared<ScopedLibraryMoveUndo>(libName);
        return undoPtr;
    }

    ScopedLibraryModifyUndoPtr EditorLibraryUndoManager::AddScopedLibraryModifyUndo(const AZStd::string& libName)
    {
        if (IsSuspend())
        {
            return nullptr;
        }

        ScopedLibraryModifyUndoPtr undoPtr = AZStd::make_shared<ScopedLibraryModifyUndo>(libName);
        return undoPtr;
    }

    ScopedSuspendUndoPtr EditorLibraryUndoManager::AddScopedSuspendUndo()
    {
        ScopedSuspendUndoPtr undoPtr = AZStd::make_shared<ScopedSuspendUndo>();
        return undoPtr;
    }

    ScopedBatchUndoPtr EditorLibraryUndoManager::AddScopedBatchUndo(const AZStd::string& label)
    {
        ScopedBatchUndoPtr undoPtr = AZStd::make_shared<ScopedBatchUndo>(label);
        return undoPtr;
    }

    void EditorLibraryUndoManager::Suspend(bool isSuspend)
    {
        if (isSuspend)
        {
            m_suspendCnt++;
        }
        else
        {
            m_suspendCnt--;
            AZ_Assert(m_suspendCnt >= 0, "non paired suspend/resume");
        }
    }

    bool EditorLibraryUndoManager::IsSuspend()
    {
        return m_suspendCnt > 0;
    }

} //end namespace EditorUIPlugin
