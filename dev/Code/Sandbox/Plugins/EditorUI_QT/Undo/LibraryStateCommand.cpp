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
#include "EditorUI_QT_Precompiled.h"
#include "LibraryStateCommand.h"
#include <Include/IBaseLibraryManager.h>

#include <EditorUI_QTDLLBus.h>

namespace EditorUIPlugin
{
    LibraryStateCommand::LibraryStateCommand(const AZStd::string& friendlyName)
        : UndoSystem::URSequencePoint(friendlyName, 0)
        , m_libUndoPos(-1)
        , m_libRedoPos(-1)
    {
    }

    LibraryStateCommand::~LibraryStateCommand()
    {
    }

    int LibraryStateCommand::GetLibraryIndex(const AZStd::string& libId) const
    {
        IBaseLibraryManager* libMgr = nullptr;
        EBUS_EVENT_RESULT(libMgr, EditorLibraryUndoRequestsBus, GetLibraryManager);
        AZ_Assert(libMgr, "libMgr: null pointer");
        return libMgr->FindLibraryIndex(libId.c_str());
    }

    void LibraryStateCommand::Capture(const AZStd::string& libId, bool isModified)
    {
        m_libId = libId;

        IDataBaseLibrary* lib =  nullptr;
        EBUS_EVENT_RESULT(lib, EditorLibraryUndoRequestsBus, GetLibrary, libId);

        AZ_Assert(lib, "library doesn't exist for capture undo");

        if (lib == nullptr)
        {
            return;
        }

        if (isModified)
        {   //save the stats for redo if it's modified
            AZ_Assert(m_redoState == nullptr, "You can't capture undo more than once");
            m_redoState = GetIEditor()->GetSystem()->CreateXmlNode(m_libId.c_str());
            lib->Serialize(m_redoState, false);
            m_libRedoPos = GetLibraryIndex(m_libId);
            EBUS_EVENT_RESULT(m_redoExpand, LibraryPanelRequests::Bus, GetLibTreeExpandInfo, libId);
        }
        else
        {
            //save the stats for undo before it's modified
            AZ_Assert(m_undoState == nullptr, "You can't capture undo more than once");
            m_undoState = GetIEditor()->GetSystem()->CreateXmlNode(m_libId.c_str());
            lib->Serialize(m_undoState, false);
            m_libUndoPos = GetLibraryIndex(m_libId);
            EBUS_EVENT_RESULT(m_undoExpand, LibraryPanelRequests::Bus, GetLibTreeExpandInfo, libId);
        }
    }

    void LibraryStateCommand::RestoreLibrary(XmlNodeRef& libSaveData, int newPos, const LibTreeExpandInfo& expandInfo) const
    {
        IBaseLibraryManager* libMgr = nullptr;
        EBUS_EVENT_RESULT(libMgr, EditorLibraryUndoRequestsBus, GetLibraryManager);
        
        if (libSaveData == nullptr)
        {
            //delete the lib
            libMgr->DeleteLibrary(m_libId.c_str(), true);
            GetIEditor()->Notify(eNotify_OnDataBaseUpdate);
            return;
        }

        IDataBaseLibrary* lib = libMgr->FindLibrary(m_libId.c_str());
        
        if (lib == nullptr)
        {
            lib = libMgr->AddLibrary(m_libId.c_str(), false, true);
        }
       
        lib->Serialize(libSaveData, true);

        if (newPos >= 0)
        {
            libMgr->ChangeLibraryOrder(lib, static_cast<unsigned int> (newPos));
        }

        GetIEditor()->Notify(eNotify_OnDataBaseUpdate);

        EBUS_EVENT(LibraryPanelRequests::Bus, LoadLibTreeExpandInfo, m_libId, expandInfo);
    }

    void LibraryStateCommand::Undo()
    {
        RestoreLibrary(m_undoState, m_libUndoPos, m_undoExpand);
    }

    void LibraryStateCommand::Redo()
    {
        RestoreLibrary(m_redoState, m_libRedoPos, m_redoExpand);
    }
    
    LibraryDeleteCommand::LibraryDeleteCommand()
        : LibraryStateCommand("Delete Library")
    {

    }

    void LibraryDeleteCommand::Capture(const AZStd::string& libId)
    {
        LibraryStateCommand::Capture(libId, false);
        AZ_Assert(m_redoState == nullptr, "LibraryDeleteCommand should be used before library deleted");
    }

    LibraryCreateCommand::LibraryCreateCommand()
        : LibraryStateCommand("Add Library")
    {

    }

    void LibraryCreateCommand::Capture(const AZStd::string& libId)
    {
        LibraryStateCommand::Capture(libId, true);
        AZ_Assert(m_undoState == nullptr, "LibraryCreateCommand should be used after library created");
    }

    LibraryMoveCommand::LibraryMoveCommand(const AZStd::string& libId)
        : UndoSystem::URSequencePoint("Move Library", 0)
        , m_libId(libId)
        , m_undoPos(-1)
        , m_redoPos(-1)
    {

    }

    void LibraryMoveCommand::Undo()
    {
        MoveLibrary(m_undoPos);
    }

    void LibraryMoveCommand::Redo()
    {
        MoveLibrary(m_redoPos);
    }

    void LibraryMoveCommand::CaptureStart()
    {
        m_undoPos = GetLibraryIndex();
    }

    void LibraryMoveCommand::CaptureEnd()
    {
        m_redoPos = GetLibraryIndex();
    }

    int LibraryMoveCommand::GetLibraryIndex() const
    {
        IBaseLibraryManager* libMgr = nullptr;
        EBUS_EVENT_RESULT(libMgr, EditorLibraryUndoRequestsBus, GetLibraryManager);
        AZ_Assert(libMgr, "libMgr: null pointer");
        return libMgr->FindLibraryIndex(m_libId.c_str());
    }

    void LibraryMoveCommand::MoveLibrary(int newPosition) const
    {
        if (newPosition < 0)
        {
            return;
        }
        IBaseLibraryManager* libMgr = nullptr;
        EBUS_EVENT_RESULT(libMgr, EditorLibraryUndoRequestsBus, GetLibraryManager);
        AZ_Assert(libMgr, "libMgr: null pointer");;
        IDataBaseLibrary* lib = libMgr->FindLibrary(m_libId.c_str());
        AZ_Assert(lib, "lib: null pointer");;
        libMgr->ChangeLibraryOrder(lib, newPosition);
        
        GetIEditor()->Notify(eNotify_OnDataBaseUpdate);
    }


    LibraryModifyCommand::LibraryModifyCommand(const AZStd::string& libId)
        : LibraryStateCommand("Modify Library")
    {
        LibraryStateCommand::m_libId = libId;
    }
    
    void LibraryModifyCommand::CaptureStart()
    {
        LibraryStateCommand::Capture(m_libId, false);
    }

    void LibraryModifyCommand::CaptureEnd()
    {
        LibraryStateCommand::Capture(m_libId, true);
    }
     
} // namespace EditorUIPlugin
