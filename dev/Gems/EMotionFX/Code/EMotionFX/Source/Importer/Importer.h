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

#include "../EMotionFXConfig.h"
#include <MCore/Source/Endian.h>
#include <EMotionFX/Source/BaseObject.h>
#include <AzCore/std/string/string.h>


MCORE_FORWARD_DECLARE(File);
MCORE_FORWARD_DECLARE(Attribute);


namespace EMotionFX
{
    // forward declarations
    class Actor;
    class MotionSet;
    class SkeletalMotion;
    class WaveletSkeletalMotion;
    class ChunkProcessor;
    class SharedData;
    class VertexAttributeLayerAbstractData;
    class NodeMap;
    class Motion;
    class AnimGraph;

    /**
     * The EMotion FX importer, used to load actors, motions, animgraphs, motion sets and node maps and other EMotion FX related files.
     * The files can be loaded from memory or disk.
     *
     * Basic usage:
     *
     * <pre>
     * EMotionFX::Actor* actor = EMotionFX::GetImporter().LoadActor("TestActor.actor");
     * if (actor == nullptr)
     *     MCore::LogError("Failed to load the actor.");
     * </pre>
     *
     * The same applies to other types like motions, but using the LoadMotion, LoadAnimGraph and LoadMotionSet methods.
     */
    class EMFX_API Importer : public BaseObject
    {
        AZ_CLASS_ALLOCATOR_DECL
        friend class Initializer;
        friend class EMotionFXManager;

    public:
        /**
         * The abstract vertex layer data conversion callback type.
         * The callback is responsible for converting coordinate system and endian of the layer data.
         */
        typedef bool (MCORE_CDECL * AbstractLayerConverter)(VertexAttributeLayerAbstractData* layer, const MCore::Endian::EEndianType sourceEndianType);

        /**
         * The standard layer convert function.
         * This will convert the data inside a layer that is being read.
         * You can specify your own layer convert function if you need to convert your own layer types.
         * The conversions done will be coordinate system conversion and endian conversion.
         * The supported layer types are the ones found in the Mesh enum that contains Mesh::ATTRIB_POSITIONS.
         * @param layer The abstract vertex data layer to convert.
         * @param sourceEndianType The endian type in which the vertex data is currently stored.
         * @result Returns true when the layer has been successfully converted. In case of an unknown layer, false is returned.
         */
        static bool MCORE_CDECL StandardLayerConvert(VertexAttributeLayerAbstractData* layer, const MCore::Endian::EEndianType sourceEndianType);

        /**
         * The attribute data endian conversion callback.
         * This callback is responsible for converting the endian of the data stored inside a given attribute into the currently expected endian.
         */
        typedef bool (MCORE_CDECL * AttributeEndianConverter)(MCore::Attribute& attribute, MCore::Endian::EEndianType sourceEndianType);

        /**
         * The standard endian conversion callback which converts endian for attribute data.
         * This automatically handles the default types of the attributes.
         * @param attribute The attribute to convert the endian for.
         * @param sourceEndianType The endian type the data is stored in before conversion.
         * @result Returns true when conversion was successful or false when the attribute uses an unknown data type, which we don't know how to convert.
         */
        static bool MCORE_CDECL StandardAttributeEndianConvert(MCore::Attribute& attribute, MCore::Endian::EEndianType sourceEndianType);

        /**
         * The actor load settings.
         * You can pass such struct to the LoadActor method.
         */
        struct EMFX_API ActorSettings
        {
            bool mForceLoading;                             /**< Set to true in case you want to load the actor even if an actor with the given filename is already inside the actor manager. */
            bool mLoadMeshes;                               /**< Set to false if you wish to disable loading any meshes. */
            bool mLoadCollisionMeshes;                      /**< Set to false if you wish to disable loading any collision meshes. */
            bool mLoadStandardMaterialLayers;               /**< Set to false if you wish to disable loading any standard material layers. */
            bool mLoadSkinningInfo;                         /**< Set to false if you wish to disable loading of skinning (using bones) information. */
            bool mLoadLimits;                               /**< Set to false if you wish to disable loading of joint limits. */
            bool mLoadGeometryLODs;                         /**< Set to false if you wish to disable loading of geometry LOD levels. */
            bool mLoadSkeletalLODs;                         /**< Set to false if you wish to disable loading of skeletal LOD levels. */
            bool mLoadTangents;                             /**< Set to false if you wish to disable loading of tangent information. */
            bool mAutoGenTangents;                          /**< Set to false if you don't wish the tangents to be calculated at load time if there are no tangent layers present. */
            bool mLoadMorphTargets;                         /**< Set to false if you wish to disable loading any morph targets. */
            bool mDualQuatSkinning;                         /**< Set to true  if you wish to enable software skinning using dual quaternions. */
            bool mMakeGeomLODsCompatibleWithSkeletalLODs;   /**< Set to true if you wish to disable the process that makes sure no skinning influences are mapped to disabled bones. Default is false. */
            bool mUnitTypeConvert;                          /**< Set to false to disable automatic unit type conversion (between cm, meters, etc). On default this is enabled. */
            uint32 mThreadIndex;
            MCore::Array<uint32>    mChunkIDsToIgnore;      /**< Add chunk ID's to this array. Chunks with these ID's will not be processed. */
            MCore::Array<uint32>    mLayerIDsToIgnore;      /**< Add vertex attribute layer ID's to ignore. */
            AbstractLayerConverter  mLayerConvertFunction;  /**< The layer conversion callback. On default it uses the StandardLayerConvert function. */

            /**
             * Constructor.
             */
            ActorSettings()
            {
                mForceLoading                           = false;
                mLoadMeshes                             = true;
                mLoadCollisionMeshes                    = true;
                mLoadStandardMaterialLayers             = true;
                mLoadSkinningInfo                       = true;
                mLoadLimits                             = true;
                mLoadGeometryLODs                       = true;
                mLoadSkeletalLODs                       = true;
                mLoadTangents                           = true;
                mAutoGenTangents                        = true;
                mLoadMorphTargets                       = true;
                mDualQuatSkinning                       = false;
                mMakeGeomLODsCompatibleWithSkeletalLODs = false;
                mUnitTypeConvert                        = true;
                mThreadIndex                            = 0;
                mLayerConvertFunction                   = StandardLayerConvert;

                mChunkIDsToIgnore.SetMemoryCategory(EMFX_MEMCATEGORY_IMPORTER);
                mLayerIDsToIgnore.SetMemoryCategory(EMFX_MEMCATEGORY_IMPORTER);
            }
        };

        /**
         * The skeletal motion import options.
         * This can be used in combination with the LoadSkeletalMotion method.
         */
        struct EMFX_API SkeletalMotionSettings
        {
            bool mForceLoading;         /**< Set to true in case you want to load the motion even if a motion with the given filename is already inside the motion manager. */
            bool mLoadMotionEvents;     /**< Set to false if you wish to disable loading of motion events. */
            bool mAutoRegisterEvents;   /**< Set to true if you want to automatically register new motion event types. */
            bool mUnitTypeConvert;      /**< Set to false to disable automatic unit type conversion (between cm, meters, etc). On default this is enabled. */
            MCore::Array<uint32>    mChunkIDsToIgnore;  /**< Add the ID's of the chunks you wish to ignore. */

            /**
             * The constructor.
             */
            SkeletalMotionSettings()
            {
                mForceLoading       = false;
                mLoadMotionEvents   = true;
                mAutoRegisterEvents = true;
                mUnitTypeConvert    = true;
                mChunkIDsToIgnore.SetMemoryCategory(EMFX_MEMCATEGORY_IMPORTER);
            }
        };


        /**
         * The motion set import options.
         * This can be used in combination with the LoadMotionSet method.
         */
        struct EMFX_API MotionSetSettings
        {
            bool mForceLoading;         /**< Set to true in case you want to load the motion set even if a motion set with the given filename is already inside the motion manager. */
            bool m_isOwnedByRuntime;
            /**
             * The constructor.
             */
            MotionSetSettings()
            {
                mForceLoading       = false;
                m_isOwnedByRuntime  = false;
            }
        };


        /**
         * The node map import options.
         * This can be used in combination with the LoadNodeMap method.
         */
        struct EMFX_API NodeMapSettings
        {
            bool                    mAutoLoadSourceActor;   /**< Should we automatically try to load the source actor? (default=true) */
            bool                    mLoadNodes;             /**< Add nodes to the map? (default=true) */

            /**
             * The constructor.
             */
            NodeMapSettings()
            {
                mAutoLoadSourceActor    = true;
                mLoadNodes              = true;
            }
        };


        /**
         * The anim graph import settings.
         */
        struct EMFX_API AnimGraphSettings
        {
            bool                    mForceLoading;              /**< Set to true in case you want to load the anim graph even if an anim graph with the given filename is already inside the anim graph manager. */
            bool                    mDisableNodeVisualization;  /**< Force disabling of node visualization code execution inside the anim graph nodes? */

            /**
             * The constructor, which uses the standard endian conversion routine on default.
             */
            AnimGraphSettings()
            {
                mForceLoading                   = false;
                mDisableNodeVisualization       = true;
            }
        };


        struct EMFX_API ImportParameters
        {
            Actor*                              mActor;
            Motion*                             mMotion;
            MotionSet*                          mMotionSet;
            Importer::ActorSettings*            mActorSettings;
            Importer::SkeletalMotionSettings*   mSkeletalMotionSettings;
            MCore::Array<SharedData*>*          mSharedData;
            MCore::Endian::EEndianType          mEndianType;

            AnimGraph*                          mAnimGraph;
            Importer::AnimGraphSettings*        mAnimGraphSettings;

            NodeMap*                            mNodeMap;
            Importer::NodeMapSettings*          mNodeMapSettings;
            bool                                m_isOwnedByRuntime;

            ImportParameters()
            {
                mActor                  = nullptr;
                mMotion                 = nullptr;
                mMotionSet              = nullptr;
                mActorSettings          = nullptr;
                mSkeletalMotionSettings = nullptr;
                mSharedData             = nullptr;
                mAnimGraphSettings     = nullptr;
                mAnimGraph             = nullptr;
                mNodeMap                = nullptr;
                mNodeMapSettings        = nullptr;
                mEndianType             = MCore::Endian::ENDIAN_LITTLE;
                m_isOwnedByRuntime      = false;
            }
        };


        /**
         * The file types.
         * This can be used to identify the file type, with the CheckFileType methods.
         */
        enum EFileType
        {
            FILETYPE_UNKNOWN        = 0,    /**< An unknown file, or something went wrong. */
            FILETYPE_ACTOR,                 /**< An actor file (.actor). */
            FILETYPE_SKELETALMOTION,        /**< A skeletal motion file (.motion). */
            FILETYPE_WAVELETSKELETALMOTION, /**< A wavelet compressed skeletal motion (.motion). */
            FILETYPE_PMORPHMOTION,          /**< A  morph motion file (.xpm). */
            FILETYPE_ANIMGRAPH,            /**< A animgraph file (.animgraph). */
            FILETYPE_MOTIONSET,             /**< A motion set file (.motionSet). */
            FILETYPE_NODEMAP                /**< A node map file (.nodeMap). */
        };

        /**
         * The type of skeletal motion.
         */
        enum ESkeletalMotionType
        {
            SKELMOTIONTYPE_NORMAL   = 0,    /**< A regular keyframe and keytrack based skeletal motion. */
            SKELMOTIONTYPE_WAVELET  = 1     /**< A wavelet compressed skeletal motion. */
        };

        struct FileInfo
        {
            MCore::Endian::EEndianType  mEndianType;
        };

        //-------------------------------------------------------------------------------------------------

        static Importer* Create();

        //-------------------------------------------------------------------------------------------------

        /**
         * Load an actor from a given file.
         * A file does not have to be stored on disk, but can also be in memory or in an archive or
         * on some network stream. Anything is possible.
         * @param f The file to load the actor from (after load, the file will be closed).
         * @param settings The settings to use for loading. When set to nullptr, all defaults will be used and everything will be loaded.
         * @param filename The file name to set inside the Actor object. This is not going to load the actor from the file specified to this parameter, but just updates the value returned by actor->GetFileName().
         * @result Returns a pointer to the loaded actor, or nullptr when something went wrong and the actor could not be loaded.
         */
        Actor* LoadActor(MCore::File* f, ActorSettings* settings = nullptr, const char* filename = "");

        /**
         * Loads an actor from a file on disk.
         * @param filename The name of the file on disk.
         * @param settings The settings to use for loading. When set to nullptr, all defaults will be used and everything will be loaded.
         * @result Returns a pointer to the loaded actor, or nullptr when something went wrong and the actor could not be loaded.
         */
        Actor* LoadActor(AZStd::string filename, ActorSettings* settings = nullptr);

        /**
         * Loads an actor from memory.
         * @param memoryStart The start address of the file in memory.
         * @param lengthInBytes The length of the file, in bytes.
         * @param settings The settings to use for loading. When set to nullptr, all defaults will be used and everything will be loaded.
         * @param filename The file name to set inside the Actor object. This is not going to load the actor from the file specified to this parameter, but just updates the value returned by actor->GetFileName().
         * @result Returns a pointer to the loaded actor, or nullptr when something went wrong and the actor could not be loaded.
         */
        Actor* LoadActor(uint8* memoryStart, size_t lengthInBytes, ActorSettings* settings = nullptr, const char* filename = "");

        bool ExtractActorFileInfo(FileInfo* outInfo, const char* filename) const;

        //-------------------------------------------------------------------------------------------------

        /**
         * Load a motion from a given file.
         * A file does not have to be stored on disk, but can also be in memory or in an archive or
         * on some network stream. Anything is possible.
         * @param f The file to load the motion from (after load, the file will be closed).
         * @param settings The settings to use during loading, or nullptr when you want to use default settings, which would load everything.
         * @result Returns a pointer to the loaded motion, or nullptr when something went wrong and the motion could not be loaded.
         */
        SkeletalMotion* LoadSkeletalMotion(MCore::File* f, SkeletalMotionSettings* settings = nullptr);

        /**
         * Loads a motion from a file on disk.
         * @param filename The name of the file on disk.
         * @param settings The settings to use during loading, or nullptr when you want to use default settings, which would load everything.
         * @result Returns a pointer to the loaded motion, or nullptr when something went wrong and the motion could not be loaded.
         */
        SkeletalMotion* LoadSkeletalMotion(AZStd::string filename, SkeletalMotionSettings* settings = nullptr);

        /**
         * Loads a motion from memory.
         * @param memoryStart The start address of the file in memory.
         * @param lengthInBytes The length of the file, in bytes.
         * @param settings The settings to use during loading, or nullptr when you want to use default settings, which would load everything.
         * @result Returns a pointer to the loaded motion, or nullptr when something went wrong and the motion could not be loaded.
         */
        SkeletalMotion* LoadSkeletalMotion(uint8* memoryStart, size_t lengthInBytes, SkeletalMotionSettings* settings = nullptr);

        bool ExtractSkeletalMotionFileInfo(FileInfo* outInfo, const char* filename) const;

        //-------------------------------------------------------------------------------------------------

        /**
         * Load a anim graph file from a given file object.
         * @param f The file object.
         * @param settings The importer settings, or nullptr to use default settings.
         * @result The anim graph object, or nullptr when failed.
         */
        AnimGraph* LoadAnimGraph(MCore::File* f, AnimGraphSettings* settings = nullptr);

        /**
         * Load a anim graph file by filename.
         * @param filename The filename to load from.
         * @param settings The anim graph importer settings, or nullptr to use default settings.
         * @result The anim graph object, or nullptr in case loading failed.
         */
        AnimGraph* LoadAnimGraph(AZStd::string, AnimGraphSettings* settings = nullptr);

        /**
         * Load a anim graph file from a memory location.
         * @param memoryStart The start address of the file in memory.
         * @param lengthInBytes The length of the file, in bytes.
         * @param settings The settings to use during loading, or nullptr when you want to use default settings, which would load everything.
         * @result The anim graph object, or nullptr in case loading failed.
         */
        AnimGraph* LoadAnimGraph(uint8* memoryStart, size_t lengthInBytes, AnimGraphSettings* settings = nullptr);

        //-------------------------------------------------------------------------------------------------

        /**
         * Load a motion set from a given file.
         * A file does not have to be stored on disk, but can also be in memory or in an archive or  on some network stream. Anything is possible.
         * @param f The file to load the motion set from (after load, the file will be closed).
         * @param settings The motion set importer settings, or nullptr to use default settings.
         * @result The motion set object, or nullptr in case loading failed.
         */
        MotionSet* LoadMotionSet(MCore::File* f, MotionSetSettings* settings = nullptr);

        /**
         * Loads a motion set from a file on disk.
         * @param filename The name of the file on disk.
         * @param settings The motion set importer settings, or nullptr to use default settings.
         * @result The motion set object, or nullptr in case loading failed.
         */
        MotionSet* LoadMotionSet(AZStd::string filename, MotionSetSettings* settings = nullptr);

        /**
         * Loads a motion set from memory.
         * @param memoryStart The start address of the file in memory.
         * @param lengthInBytes The length of the file, in bytes.
         * @param settings The motion set importer settings, or nullptr to use default settings.
         * @result The motion set object, or nullptr in case loading failed.
         */
        MotionSet* LoadMotionSet(uint8* memoryStart, size_t lengthInBytes, MotionSetSettings* settings = nullptr);

        //-------------------------------------------------------------------------------------------------

        /**
         * Load a node map from a given file.
         * A file does not have to be stored on disk, but can also be in memory or in an archive or  on some network stream. Anything is possible.
         * @param f The file to load the motion set from (after load, the file will be closed).
         * @param settings The node map importer settings, or nullptr to use default settings.
         * @result The node map object, or nullptr in case loading failed.
         */
        NodeMap* LoadNodeMap(MCore::File* f, NodeMapSettings* settings = nullptr);

        /**
         * Loads a node map from a file on disk.
         * @param filename The name of the file on disk.
         * @param settings The node map importer settings, or nullptr to use default settings.
         * @result The node map object, or nullptr in case loading failed.
         */
        NodeMap* LoadNodeMap(AZStd::string filename, NodeMapSettings* settings = nullptr);

        /**
         * Loads a node map from memory.
         * @param memoryStart The start address of the file in memory.
         * @param lengthInBytes The length of the file, in bytes.
         * @param settings The node map importer settings, or nullptr to use default settings.
         * @result The motion set object, or nullptr in case loading failed.
         */
        NodeMap* LoadNodeMap(uint8* memoryStart, size_t lengthInBytes, NodeMapSettings* settings = nullptr);

        //-------------------------------------------------------------------------------------------------

        /**
         * Register a new chunk processor.
         * It can either be a new version of a chunk processor to extend the current file format or
         * a complete new chunk processor. It will be added to the processor database and will then be executable.
         * @param processorToRegister The processor to register to the importer.
         */
        void RegisterChunkProcessor(ChunkProcessor* processorToRegister);

        /**
         * Find shared data objects which have the same type as the ID passed as parameter.
         * @param sharedDataArray The shared data array to search in.
         * @param type The shared data ID to search for.
         * @return A pointer to the shared data object, or nullptr when no shared data of this type has been found.
         */
        static SharedData* FindSharedData(MCore::Array<SharedData*>* sharedDataArray, uint32 type);

        /**
         * Enable or disable logging.
         * @param enabled Set to true if you want to enable logging, or false when you want to disable it.
         */
        void SetLoggingEnabled(bool enabled);

        /**
         * Check if logging is enabled or not.
         * @return Returns true when the importer will perform logging, or false when it will be totally silent.
         */
        bool GetLogging() const;

        /**
         * Set if the importer should log the processor details or not.
         * @param detailLoggingActive Set to true when you want to enable this feature, otherwise set it to false.
         */
        void SetLogDetails(bool detailLoggingActive);

        /**
         * Check if detail-logging is enabled or not.
         * @return Returns true when detail-logging is enabled, otherwise false is returned.
         */
        bool GetLogDetails() const;

        /**
         * Convert an attribute layer type into a string, which is more clear to understand.
         * @param layerID The layer ID, see ActorFileFormat.h for available ID values, such as FileFormat::Actor_VERTEXDATA_NORMALS.
         * @result A string containing the description.
         */
        static const char* ActorVertexAttributeLayerTypeToString(uint32 layerID);

        /**
         * Check the type of a given file on disk.
         * @param filename The file on disk to check.
         * @result Returns the file type. The FILETYPE_UNKNOWN will be returned when something goes wrong or the file format is unknown.
         */
        EFileType CheckFileType(const char* filename);

        /**
         * Check the type of a given file (in memory or disk).
         * The file will be closed after executing this method!
         * @param file The file object to check.
         * @result Returns the file type. The FILETYPE_UNKNOWN will be returned when something goes wrong or the file format is unknown.
         */
        EFileType CheckFileType(MCore::File* file);


    private:
        MCore::Array<ChunkProcessor*>       mChunkProcessors;   /**< The registered chunk processors. */
        bool                                mLoggingActive;     /**< Contains if the importer should perform logging or not or not. */
        bool                                mLogDetails;        /**< Contains if the importer should perform detail-logging or not. */

        /**
         * The constructor.
         */
        Importer();

        /**
         * The destructor.
         */
        ~Importer();

        /**
         * Verify if the given file is a valid actor file that can be processed by the importer.
         * Please note that the specified must already been opened and must also be pointing to the location where the
         * Actor file header will be stored (the start of the file). The file will not be closed after this method!
         * The endian type of the file will be written inside the outEndianType parameter.
         * Also note that the file position (read position / cursor) will point after the header after this function has been executed.
         * @param f The file to perform the check on.
         * @param outEndianType The value that will contain the endian type used by the file.
         * @result Returns true when the file is a valid actor file that can be processed by the importer. Otherwise false is returned.
         */
        bool CheckIfIsValidActorFile(MCore::File* f, MCore::Endian::EEndianType* outEndianType) const;

        /**
         * Verify if the given file is a valid motion file that can be processed by the importer.
         * Please note that the specified must already been opened and must also be pointing to the location where the
         * Motion file header will be stored (the start of the file). The file will not be closed after this method!
         * The endian type of the file will be written inside the outEndianType parameter.
         * Also note that the file position (read position / cursor) will point after the header after this function has been executed.
         * @param f The file to perform the check on.
         * @param outEndianType The value that will contain the endian type used by the file.
         * @param outMotionType The skeletal motion type will be stored in this parameter.
         * @result Returns true when the file is a valid actor file that can be processed by the importer. Otherwise false is returned.
         */
        bool CheckIfIsValidSkeletalMotionFile(MCore::File* f, MCore::Endian::EEndianType* outEndianType, ESkeletalMotionType* outMotionType) const;

        /**
         * Verify if the given file is a valid motion set file that can be processed by the importer.
         * Please note that the specified must already been opened and must also be pointing to the location where the
         * XPM file header will be stored (the start of the file). The file will not be closed after this method!
         * The endian type of the file will be written inside the outEndianType parameter.
         * Also note that the file position (read position / cursor) will point after the header after this function has been executed.
         * @param f The file to perform the check on.
         * @param outEndianType The value that will contain the endian type used by the file.
         * @result Returns true when the file is a valid actor file that can be processed by the importer. Otherwise false is returned.
         */
        bool CheckIfIsValidMotionSetFile(MCore::File* f, MCore::Endian::EEndianType* outEndianType) const;

        /**
         * Verify if the given file is a valid anim graph file that can be processed by the importer.
         * Please note that the specified must already been opened and must also be pointing to the location where the
         * XPM file header will be stored (the start of the file). The file will not be closed after this method!
         * The endian type of the file will be written inside the outEndianType parameter.
         * Also note that the file position (read position / cursor) will point after the header after this function has been executed.
         * @param f The file to perform the check on.
         * @param outEndianType The value that will contain the endian type used by the file.
         * @result Returns true when the file is a valid actor file that can be processed by the importer. Otherwise false is returned.
         */
        bool CheckIfIsValidAnimGraphFile(MCore::File* f, MCore::Endian::EEndianType* outEndianType) const;

        /**
         * Verify if the given file is a valid node map file that can be processed by the importer.
         * Please note that the specified must already been opened and must also be pointing to the location where the
         * XPM file header will be stored (the start of the file). The file will not be closed after this method!
         * The endian type of the file will be written inside the outEndianType parameter.
         * Also note that the file position (read position / cursor) will point after the header after this function has been executed.
         * @param f The file to perform the check on.
         * @param outEndianType The value that will contain the endian type used by the file.
         * @result Returns true when the file is a valid actor file that can be processed by the importer. Otherwise false is returned.
         */
        bool CheckIfIsValidNodeMapFile(MCore::File* f, MCore::Endian::EEndianType* outEndianType) const;

        /**
         * Add shared data.
         * Shared data is stored in the importer and provides the chunk processors to use and modify it.
         * Shared data objects need to be inherited from the Importer::SharedData base class and must get
         * their unique ID. Use shared data if you have several chunk processors that share data between them.
         * This function will add a new shared data object to the importer.
         * @param sharedData The array which holds the shared data objects.
         * @param data A pointer to your shared data object.
         */
        static void AddSharedData(MCore::Array<SharedData*>& sharedData, SharedData* data);

        /*
         * Precreate the standard shared data objects.
         * @param sharedData The shared data array to work on.
         */
        static void PrepareSharedData(MCore::Array<SharedData*>& sharedData);

        /**
         * Reset all shared data objects.
         * Resetting these objects will clear/empty their internal data.
         */
        static void ResetSharedData(MCore::Array<SharedData*>& sharedData);

        /**
         * Find the chunk processor which has a given ID and version number.
         * @param chunkID The ID of the chunk processor.
         * @param version The version number of the chunk processor.
         * @return A pointer to the chunk processor if it has been found, or nullptr when no processor could be found
         *         which is able to process the given chunk or the given version of the chunk.
         */
        ChunkProcessor* FindChunk(uint32 chunkID, uint32 version) const;

        /**
         * Register the standard chunk processors to be able to import all standard LMA chunks.
         * Shared data which is essential for the standard chunks will also be automatically created and added.
         */
        void RegisterStandardChunks();

        /**
         * Read and process the next chunk.
         * @param file The file to read from.
         * @param importParams The import parameters.
         * @result Returns false when the chunk could not be processed, due to an end of file or other file error.
         */
        bool ProcessChunk(MCore::File* file, Importer::ImportParameters& importParams);

        /**
         * Validate and resolve any conflicting settings inside a specific actor import settings object.
         * @param settings The actor settings to verify and fix/modify when needed.
         */
        void ValidateActorSettings(ActorSettings* settings);

        /**
         * Validate and resolve any conflicting settings inside a specific skeltal motion import settings object.
         * @param settings The skeletal motion settings to verify and fix/modify when needed.
         */
        void ValidateSkeletalMotionSettings(SkeletalMotionSettings* settings);
    };
} // namespace EMotionFX
