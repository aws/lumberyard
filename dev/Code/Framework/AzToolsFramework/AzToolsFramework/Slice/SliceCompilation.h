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

#pragma once

#include <AzCore/Slice/SliceAsset.h>
#include <AzCore/Outcome/Outcome.h>
#include <AzCore/Component/ComponentExport.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>

namespace AZ
{
    class SerializeContext;
}

namespace AzToolsFramework
{
    using SliceCompilationResult = AZ::Outcome<AZ::Data::Asset<AZ::SliceAsset>, AZStd::string>;

    /**
     * Callback handler interface for processing compiled slices prior to stripping of editor-only entities.
     */
    class EditorOnlyEntityHandler
    {
    public:
        virtual ~EditorOnlyEntityHandler() = default;

        using Result = AZ::Outcome<void, AZStd::string>;

        /**
         * This handler is responsible for making any necessary modifications to other entities in the slice prior to the removal
         * of all editor-only entities.
         * After this callback returns, editor-only entities will be removed from the slice.
         * See \ref WorldEditorOnlyEntityHandler below for an example of processing and validation that occurs for standard world entities.
         * @param entities a list of all entities in the slice, including those marked as editor-only.
         * @param editorOnlyEntityIds a precomputed set containing Ids for all entities within the 'entities' list that were marked as editor-only.
         * @param serializeContext useful to inspect entity data for validation purposes.
         */
        virtual Result HandleEditorOnlyEntities(
            const AzToolsFramework::EntityList& /*entities*/, 
            const AzToolsFramework::EntityIdSet& /*editorOnlyEntityIds*/, 
            AZ::SerializeContext& /*serializeContext*/) { return AZ::Success();  }
    };

    /**
     * EditorOnlyEntity handler for world entities.
     * - Fixes up transform relationships so entities removed mid-hierarchy still result in valid runtime transform relationships
     *   and correct relative transforms.
     * - Validates that no editor entities are referenced by non-editor entities.
     */
    class WorldEditorOnlyEntityHandler : public AzToolsFramework::EditorOnlyEntityHandler
    {
    public:
        Result HandleEditorOnlyEntities(
            const AzToolsFramework::EntityList& entities, 
            const AzToolsFramework::EntityIdSet& editorOnlyEntityIds, 
            AZ::SerializeContext& serializeContext) override;

        // Adjust transform relationships to maintain integrity of the transform hierarchy at runtime, even if editor-only 
        // entities were positioned within the transform hierarchy.
        static void FixTransformRelationships(
            const AzToolsFramework::EntityList& entities, 
            const AzToolsFramework::EntityIdSet& editorOnlyEntityIds);

        // Verify that none of the runtime entities reference editor-only entities. Fail w/ details if so.
        static Result ValidateReferences(
            const AzToolsFramework::EntityList& entities, 
            const AzToolsFramework::EntityIdSet& editorOnlyEntityIds, 
            AZ::SerializeContext& serializeContext);
    };

    /**
     * Converts a source editor slice to a runtime-usable slice (i.e. dynamic slice).
     * All components in the source slice are passed through validation.
     * Additionally, all components are given the opportunity to export (or not export) themselves,
     * or another component, for the platform being exported (and its tags).
     * @param sourceSlice pointer to the source slice asset, which is required for successful compilation.
     * @param platformTags set of tags defined for the platform currently being executed for.
     * @param valid serialize context.
     * @param editorOnlyEntityHandler optional custom handler to process entities in a slice in preparation for the stripping of editor only entities.
     * @return Result an AZ::Outcome with a valid slice asset as the success payload, and an error string for the error payload.
     */
    SliceCompilationResult CompileEditorSlice(const AZ::Data::Asset<AZ::SliceAsset>& sourceSlice, const AZ::PlatformTagSet& platformTags, AZ::SerializeContext& serializeContext, EditorOnlyEntityHandler* editorOnlyEntityHandler = nullptr);

} // AzToolsFramework
