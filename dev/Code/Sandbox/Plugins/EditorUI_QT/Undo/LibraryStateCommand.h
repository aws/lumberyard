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
#ifndef EDITORUIPLUGIN_LIBRARYSTATECOMMAND_H
#define EDITORUIPLUGIN_LIBRARYSTATECOMMAND_H

#pragma once

#include <api.h>
#include <AzCore/base.h>
#include <AzCore/std/containers/vector.h>

#include <AzCore/Memory/SystemAllocator.h>
#include <AzToolsFramework/Undo/UndoSystem.h>


namespace EditorUIPlugin
{
    // The library State URSequencePoint stores the state of an particle library before and after some change to it.
    // it does so by serializing the entire library, so its a good "default behavior" that cannot miss any particular change.
    // this Library State Command should be able to capture everything in its entirety.
    class LibraryStateCommand
        : public UndoSystem::URSequencePoint
    {
    public:
        AZ_RTTI(LibraryStateCommand, "{A4EB7570-54D3-4793-A2CB-08701E35E6C4}", UndoSystem::URSequencePoint);
        AZ_CLASS_ALLOCATOR(LibraryStateCommand, AZ::SystemAllocator, 0);
        
        void Undo() override;
        void Redo() override;

        // capture the the library's states
        void Capture(const AZStd::string& libId, bool isModified);

    protected:
        //protected constructor, user shouldn't use this command directly. 
        LibraryStateCommand(const AZStd::string& friendlyName);
        virtual ~LibraryStateCommand();

        void RestoreLibrary(XmlNodeRef& libSaveData, int newPos, const LibTreeExpandInfo& expandInfo) const;
        int GetLibraryIndex(const AZStd::string& libId) const;
        
        //The name of the captured library (name is the id we can use)
        AZStd::string m_libId;                              
        
        //the index of this library in lib manager
        int m_libUndoPos;                                       
        int m_libRedoPos;
        
        //undo data for library content
        XmlNodeRef m_undoState;
        XmlNodeRef m_redoState;

        //undo data of library expand in the lib tree view
        LibTreeExpandInfo m_undoExpand;
        LibTreeExpandInfo m_redoExpand;
        
        // DISABLE COPY
        LibraryStateCommand(const LibraryStateCommand& other) = delete;
        const LibraryStateCommand& operator= (const LibraryStateCommand& other) = delete;
    };

    class LibraryDeleteCommand
        : public LibraryStateCommand
    {
    public:
        AZ_RTTI(LibraryDeleteCommand, "{7ACD7DFC-AB3D-4841-B519-971158386303}", LibraryStateCommand);
        AZ_CLASS_ALLOCATOR(LibraryDeleteCommand, AZ::SystemAllocator, 0);

        LibraryDeleteCommand();
        void Capture(const AZStd::string& libId);
    };

    class LibraryCreateCommand
        : public LibraryStateCommand
    {
    public:
        AZ_RTTI(LibraryCreateCommand, "{9BF08386-6425-44D4-ADFA-6AC17E31D023}", LibraryStateCommand);
        AZ_CLASS_ALLOCATOR(LibraryCreateCommand, AZ::SystemAllocator, 0);

        LibraryCreateCommand();
        void Capture(const AZStd::string& libId);
    };

    class LibraryMoveCommand
        : public UndoSystem::URSequencePoint
    {
    public:
        AZ_RTTI(LibraryMoveCommand, "{1760318A-A35C-40C7-AD36-4DBD9A1DA895}", UndoSystem::URSequencePoint);
        AZ_CLASS_ALLOCATOR(LibraryMoveCommand, AZ::SystemAllocator, 0);
        
        LibraryMoveCommand(const AZStd::string& libId);

        void Undo() override;
        void Redo() override;

        // capture the 
        void CaptureStart();
        void CaptureEnd();

    protected:
        void MoveLibrary(int newPosition) const;
        int GetLibraryIndex() const;

        AZStd::string m_libId;                              ///< The name of the captured library (name is the id we can use)

        int m_undoPos;                                     //the initial position before move
        int m_redoPos;                                       //the position after move
        
        // DISABLE COPY
        LibraryMoveCommand(const LibraryMoveCommand& other) = delete;
        const LibraryMoveCommand& operator= (const LibraryMoveCommand& other) = delete;
    };

    class LibraryModifyCommand
        : public LibraryStateCommand
    {
    public:
        AZ_RTTI(LibraryModifyCommand, "{77C691A9-7C09-4D93-835A-C1A949D13829}", LibraryStateCommand);
        AZ_CLASS_ALLOCATOR(LibraryModifyCommand, AZ::SystemAllocator, 0);

        LibraryModifyCommand(const AZStd::string& libId);
        
        // capture the 
        void CaptureStart();
        void CaptureEnd();

    protected:                                                             
        // DISABLE COPY
        LibraryModifyCommand(const LibraryModifyCommand& other) = delete;
        const LibraryModifyCommand& operator= (const LibraryModifyCommand& other) = delete;
    };
} // namespace EditorUIPlugin

#endif // EDITORUIPLUGIN_LIBRARYSTATECOMMAND_H
