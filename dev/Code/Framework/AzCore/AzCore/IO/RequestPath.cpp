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

#include <AzCore/Debug/Trace.h>
#include <AzCore/IO/FileIO.h>
#include <AzCore/IO/RequestPath.h>

namespace AZ
{
    namespace IO
    {
        // RequestPath
        RequestPath::RequestPath(const RequestPath& rhs)
        {
            m_absolutePath = rhs.m_absolutePath;
            m_absolutePathHash = rhs.m_absolutePathHash;
            m_relativePathOffset = rhs.m_relativePathOffset;
        }

        RequestPath::RequestPath(RequestPath&& rhs)
        {
            m_absolutePath = AZStd::move(rhs.m_absolutePath);
            m_absolutePathHash = rhs.m_absolutePathHash;
            m_relativePathOffset = rhs.m_relativePathOffset;
        }

        RequestPath& RequestPath::operator=(const RequestPath& rhs)
        {
            m_absolutePath = rhs.m_absolutePath;
            m_absolutePathHash = rhs.m_absolutePathHash;
            m_relativePathOffset = rhs.m_relativePathOffset;
            return *this;
        }

        RequestPath& RequestPath::operator=(RequestPath&& rhs)
        {
            m_absolutePath = AZStd::move(rhs.m_absolutePath);
            m_absolutePathHash = rhs.m_absolutePathHash;
            m_relativePathOffset = rhs.m_relativePathOffset;
            return *this;
        }

        bool RequestPath::operator==(const RequestPath& rhs) const
        {
            if (m_absolutePathHash == rhs.m_absolutePathHash)
            {
                return m_absolutePath == rhs.m_absolutePath;
            }
            return false;
        }

        bool RequestPath::operator!=(const RequestPath& rhs) const
        {
            return !operator==(rhs);
        }

        bool RequestPath::InitFromRelativePath(const char* path)
        {
            AZ_Assert(path, "Path used to create a RequestPath can't be null.");
            AZ_Assert(FileIOBase::GetInstance(), "Trying to create a RequestPath before the low level file system has been initialized.");

            char fullPath[AZ::IO::MaxPathLength];
            if (!FileIOBase::GetInstance()->ResolvePath(path, fullPath, AZ_ARRAY_SIZE(fullPath)))
            {
                return false;
            }

            m_absolutePath = fullPath;

            // Remove the alias from the relative path.
            if (*path == '@')
            {
                const char* correctedPath = path;
                correctedPath++;
                while (*correctedPath != '@' && *correctedPath != 0)
                {
                    correctedPath++;
                }
                if (*correctedPath == '@')
                {
                    correctedPath++;
                    path = correctedPath;
                }
            }

            size_t relativePathLength = strlen(path);
            if (m_absolutePath.length() > relativePathLength)
            {
                m_relativePathOffset = m_absolutePath.length() - relativePathLength;
            }

            m_absolutePathHash = AZStd::hash<AZStd::string>{}(m_absolutePath);
            return true;
        }

        bool RequestPath::InitFromAbsolutePath(const char* path)
        {
            AZ_Assert(path, "Path used to create a RequestPath can't be null.");

            m_absolutePath = path;
            m_relativePathOffset = 0;
            m_absolutePathHash = AZStd::hash<AZStd::string>{}(m_absolutePath);
            return true;
        }

        bool RequestPath::IsValid() const
        {
            return !m_absolutePath.empty();
        }

        void RequestPath::Clear()
        {
            m_absolutePath.clear();
            m_absolutePathHash = 0;
            m_relativePathOffset = 0;
        }

        const char* RequestPath::GetAbsolutePath() const
        {
            return m_absolutePath.c_str();
        }

        const char* RequestPath::GetRelativePath() const
        {
            return m_absolutePath.empty() ? "" : (m_absolutePath.c_str() + m_relativePathOffset);
        }

        size_t RequestPath::GetHash() const
        {
            return m_absolutePathHash;
        }
    } // namespace IO
} // namespace AZ