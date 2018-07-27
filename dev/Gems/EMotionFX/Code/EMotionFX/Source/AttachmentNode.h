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


namespace EMotionFX
{
    // forward declarations
    class ActorInstance;
    class Node;


    /**
     * A regular node attachment.
     * With node we mean that this attachment is only influenced by one given node in the ActorInstance it is attached to.
     * An example of this could be a gun attached to the hand node of a character.
     * Please keep in mind that the actor instance you attach can be a fully animated character as well. It is just being influenced by one single node of
     * the actor instance you attach it to.
     */
    class EMFX_API AttachmentNode
        : public Attachment
    {
        AZ_CLASS_ALLOCATOR_DECL
    public:
        enum
        {
            TYPE_ID = 0x00000001
        };

        /**
         * Create an attachment that is attached to a single node.
         * @param attachToActorInstance The actor instance to attach to, for example the main character in the game.
         * @param attachToNodeIndex The node to attach to. This has to be part of the actor where the attachToActorInstance is instanced from.
         * @param attachment The actor instance that you want to attach to this node (for example a gun).
         */
        static AttachmentNode* Create(ActorInstance* attachToActorInstance, uint32 attachToNodeIndex, ActorInstance* attachment);

        /**
         * Get the attachment type ID.
         * Every class inherited from this base class should have some type ID.
         * @return The type ID of this attachment class.
         */
        uint32 GetType() const override                                 { return TYPE_ID; }

        /**
         * Get the attachment type string.
         * Every class inherited from this base class should have some type ID string, which should be equal to the class name really.
         * @return The type string of this attachment class, which should be the class name.
         */
        const char* GetTypeString() const override                      { return "AttachmentNode"; }

        /**
         * Check if this attachment is being influenced by multiple nodes or not.
         * This is the case for attachments such as clothing items which get influenced by multiple nodes/bones inside the actor instance they are attached to.
         * @result Returns true if it is influenced by multiple nodes, otherwise false is returned.
         */
        bool GetIsInfluencedByMultipleNodes() const override final      { return false; }

        /**
         * Get the node where we attach something to.
         * This node is part of the actor from which the actor instance returned by GetAttachToActorInstance() is created.
         * @result The node index where we will attach this attachment to.
         */
        uint32 GetAttachToNodeIndex() const;

        /**
         * The main update method.
         */
        void Update() override;


    protected:
        uint32  mAttachedToNode;    /**< The node where the attachment is linked to. */

        /**
         * The constructor for a regular attachment.
         * @param attachToActorInstance The actor instance to attach to (for example a cowboy).
         * @param attachToNodeIndex The node to attach to. This has to be part of the actor where the attachToActorInstance is instanced from.
         * @param attachment The actor instance that you want to attach to this node (for example a gun).
         */
        AttachmentNode(ActorInstance* attachToActorInstance, uint32 attachToNodeIndex, ActorInstance* attachment);

        /**
         * The destructor.
         * This does NOT delete the actor instance used by the attachment.
         */
        virtual ~AttachmentNode();
    };
}   // namespace EMotionFX
