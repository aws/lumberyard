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

// include MCore related files
#include "EMotionFXConfig.h"
#include <MCore/Source/Vector.h>
#include <MCore/Source/Quaternion.h>
#include <MCore/Source/Matrix4.h>
#include "Attachment.h"
#include "Transform.h"


namespace EMotionFX
{
    // forward declarations
    class ActorInstance;
    class Node;


    /**
     * The skin attachment class.
     * This represents an attachment that is influenced by multiple nodes, skinned to the main skeleton of the actor it gets attached to.
     * An example could be if you want to put on some pair of pants on the character. This can be used to customize your characters.
     * So this attachment will basically copy the tranformations of the main character to the bones/nodes inside the actor instance that represents this attachment.
     */
    class EMFX_API AttachmentSkin
        : public Attachment
    {
        AZ_CLASS_ALLOCATOR_DECL
    public:
        enum
        {
            TYPE_ID = 0x00000002
        };

        /**
         * The node map entry, which contains a link to a node inside the actor instance you attach to.
         */
        struct EMFX_API NodeMapping
        {
            MCore::Matrix   mLocalMatrix;       /**< The local space matrix to set the target node to. */
            MCore::Matrix   mGlobalMatrix;      /**< The global space matrix to set the target node to. */
            Transform       mLocalTransform;    /**< The local transform to set the target node to. */
            Node*           mSourceNode;        /**< The source node in the actor where this is attached to. */
        };

        /**
         * Create the attachment that is influenced by multiple nodes.
         * @param attachToActorInstance The actor instance to attach to, for example your main character.
         * @param attachment The actor instance that you want to attach to this actor instance, for example an actor instance that represents some new pants.
         */
        static AttachmentSkin* Create(ActorInstance* attachToActorInstance, ActorInstance* attachment);

        /**
         * Get the attachment type ID.
         * Every class inherited from this base class should have some TYPE ID.
         * @return The type ID of this attachment class.
         */
        uint32 GetType() const override                                 { return TYPE_ID; }

        /**
         * Get the attachment type string.
         * Every class inherited from this base class should have some type ID string, which should be equal to the class name really.
         * @return The type string of this attachment class, which should be the class name.
         */
        const char* GetTypeString() const override                      { return "AttachmentSkin"; }

        /**
         * Check if this attachment is being influenced by multiple nodes or not.
         * This is the case for attachments such as clothing items which get influenced by multiple nodes/bones inside the actor instance they are attached to.
         * @result Returns true if it is influenced by multiple nodes, otherwise false is returned.
         */
        bool GetIsInfluencedByMultipleNodes() const override final      { return true; }

        /**
         * Update the attachment.
         * This will internally update all matrices of the nodes inside the attachment that are being influenced by the actor instance it is attached to.
         */
        void Update() override;

        /**
         * Update all transformations of the actor instance that represents the attachment.
         * This is automatically called by EMotion FX internally.
         */
        virtual void UpdateNodeTransforms();

        /**
         * Get the mapping for a given node.
         * @param nodeIndex The node index inside the actor instance that represents the attachment.
         * @result A reference to the mapping information for this node.
         */
        MCORE_INLINE NodeMapping& GetNodeMapping(uint32 nodeIndex)                      { return mNodeMap[nodeIndex]; }

        /**
         * Get the mapping for a given node.
         * @param nodeIndex The node index inside the actor instance that represents the attachment.
         * @result A reference to the mapping information for this node.
         */
        MCORE_INLINE const NodeMapping& GetNodeMapping(uint32 nodeIndex) const          { return mNodeMap[nodeIndex]; }

    protected:
        MCore::Array<NodeMapping>   mNodeMap;           /**< Specifies which nodes. */

        /**
         * The constructor for a skin attachment.
         * @param attachToActorInstance The actor instance to attach to (for example a cowboy).
         * @param attachment The actor instance that you want to attach to this node (for example a gun).
         */
        AttachmentSkin(ActorInstance* attachToActorInstance, ActorInstance* attachment);

        /**
         * The destructor.
         * This does NOT delete the actor instance used by the attachment.
         */
        virtual ~AttachmentSkin();

        /**
         * Initialize the node map, which links the nodes inside the attachment with the actor where we attach to.
         * It is used to copy over the transformations from the main parent actor, to the actor instance representing the attachment object.
         */
        void InitNodeMap();
    };
}   // namespace EMotionFX
