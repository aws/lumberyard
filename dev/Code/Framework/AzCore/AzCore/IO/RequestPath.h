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

#include <AzCore/base.h>
#include <AzCore/std/string/string.h>

namespace AZ
{
    namespace IO
    {
        //! The path to the file used by a request.
        class RequestPath
        {
        public:
            RequestPath() = default;
            RequestPath(const RequestPath& rhs);
            RequestPath(RequestPath&& rhs);

            RequestPath& operator=(const RequestPath& rhs);
            RequestPath& operator=(RequestPath&& rhs);

            bool operator==(const RequestPath& rhs) const;
            bool operator!=(const RequestPath& rhs) const;

            bool InitFromRelativePath(const char* path);
            bool InitFromAbsolutePath(const char* path);

            bool IsValid() const;
            void Clear();

            const char* GetAbsolutePath() const;
            const char* GetRelativePath() const;
            size_t GetHash() const;

        private:
            AZStd::string m_absolutePath;
            size_t m_absolutePathHash = 0;
            size_t m_relativePathOffset = 0;
        };
    } // namespace IO
} // namespace AZ

namespace AZStd
{
    template<>
    struct hash<AZ::IO::RequestPath>
    {
        using argument_type = AZ::IO::RequestPath;
        using result_type = AZStd::size_t;
        inline result_type operator()(const argument_type& value) const { return value.GetHash(); }
    };
} // namespace AZStd