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

#include <AzCore/EBus/EBus.h>
#include <AzCore/Outcome/Outcome.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/string/string_view.h>
#include <AzCore/std/containers/vector.h>

#include <LyzardSDK/Base.h>

namespace Templates
{
    /**
      * Options provided to CreateFromTemplate.
      */
    struct TemplateDescriptor
    {
        AZ_TYPE_INFO(TemplateDescriptor, "{E6D93269-6318-492B-9638-C9CDA8B7F1D9}");
        AZ_CLASS_ALLOCATOR(TemplateDescriptor, AZ::SystemAllocator, 0);

        /// Used to describe files to be copied
        struct CopyFileDescriptor
        {
            AZ_TYPE_INFO(CopyFileDescriptor, "{82745A14-191E-47DB-A5E3-756166C27E6C}");
            AZ_CLASS_ALLOCATOR(CopyFileDescriptor, AZ::SystemAllocator, 0);

            /// Input file path (relative to m_inputPath)
            AZStd::string m_inFile;
            /// Output file path (relative to m_outputPath)
            AZStd::string m_outFile;
            /// Whether or not to template the file
            bool m_isTemplated;
            /// Whether or not the file is optional
            bool m_isOptional;

            CopyFileDescriptor() = default;
            CopyFileDescriptor(const AZStd::string& inFile, const AZStd::string& outFile, bool templated, bool optional)
                : m_inFile(inFile)
                , m_outFile(outFile)
                , m_isTemplated(templated)
                , m_isOptional(optional)
            {}
        };

        // Used to describe empty directories to create
        struct CreateDirectoryDescriptor
        {
            AZ_TYPE_INFO(CreateDirectoryDescriptor, "{475ED58E-6144-4EF7-BDBE-92C998E29DD6}");
            AZ_CLASS_ALLOCATOR(CreateDirectoryDescriptor, AZ::SystemAllocator, 0);

            // Output directory path (relative to m_outputPath)
            AZStd::string m_outDir;

            CreateDirectoryDescriptor() = default;
            CreateDirectoryDescriptor(const AZStd::string& outDir)
                : m_outDir(outDir)
            {}
        };

        //////////////////////////////////////////////////////////////////////////
        // Transient Data

        /// Pair used to store 'before'->'after' substitutions
        using SubstitutionPair = AZStd::pair<AZStd::string /*in*/, AZStd::string /*out*/>;
        /// List of substitution pairs in template files
        /// Note: substitutions are applied to ALL output paths, even if they aren't marked as isTemplated.
        AZStd::vector<SubstitutionPair> m_substitutions;
        /// Engine root
        AZStd::string m_engineRoot;
        /// The final location to place files
        AZStd::string m_outputPath;
        /// Whether the output path can exist or not before proceeding to place the template files at their destination.
        bool m_outputPathCanExist = false;

        //////////////////////////////////////////////////////////////////////////
        // Serialized Data

        /// The path all inFiles are relative to (if relative, assumed to be relative to engine root)
        AZStd::string m_inputPath;
        /// List of files to copy (as CopyFileDescriptors)
        AZStd::vector<CopyFileDescriptor> m_copyFiles;
        /// List of directories to create (as CreateDirectoryDescriptors)
        AZStd::vector<CreateDirectoryDescriptor> m_createDirectories;
    };


    using TemplateDescriptorOutcome = AZ::Outcome<Templates::TemplateDescriptor, AZStd::string>;
    using FileProcessedCallback = AZStd::function<Lyzard::StringOutcome(const AZStd::string_view srcFile, const AZStd::string_view dstFile)>;

    class CreateFromTemplateRequests
        : public AZ::EBusTraits
    {
    public:
        //////////////////////////////////////////////////////////////////////////
        // EBusTraits overrides
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        //////////////////////////////////////////////////////////////////////////

        virtual ~CreateFromTemplateRequests() = default;

        /**
         * Loads a template descriptor.
         */
        virtual TemplateDescriptorOutcome LoadTemplate(const AZStd::string_view templatePath, const AZStd::string_view templateName) = 0;

        /**
         * Instantiate the template.
         *
         * Note that all input files are read into memory before being written out.
         * This does work with binary files, but attempting to mark a binary file as isTemplated is not recommended.
         * Large files may cause issues on systems with limited memory.
         */
        virtual Lyzard::StringOutcome CreateFromTemplate(const TemplateDescriptor& desc, FileProcessedCallback perFileCallback) = 0;

        /**
         * Validates that an identifier is acceptable for use as a file/folder name.
         *
         * \param[in] ident     The identifier to validate
         *
         * \returns Success if the identifier is valid, error string on invalid ident.
         */
        virtual Lyzard::StringOutcome IsValidIdentifier(const AZStd::string& ident) = 0;
    };

    using CreateFromTemplateRequestBus = AZ::EBus<CreateFromTemplateRequests>;

} // namespace Templates
