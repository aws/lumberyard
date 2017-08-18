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
#ifndef AZSTD_WILDCARD_MATCH_H
#define AZSTD_WILDCARD_MATCH_H

#include <AzCore/std/string/string.h>

namespace AZStd
{
    /**
     * Wildcard '*' and '?' matching support.  Case insensitive
     *
     * Example:
     * if (wildcard_match("bl?h.*", "blah.jpg"))
     * {
     *      we have a match!
     *  } else {
     *      no match
     *  }
     * TODO: wchar_t version
     */
    inline bool wildcard_match(const char* filter, const char* name)
    {
        if (!name)
        {
            return false;
        }

        if (!filter)
        {
            return true;
        }
        if (!filter[0] && !name[0])
        {
            return true;
        }

        // if the filter is empty it matches anything
        if (!filter[0])
        {
            return true;
        }

        if (!name[0] && !(filter[0] == 0 || filter[0] == '*'))
        {
            // if the name is empty and the filter is not one of the always true condition than return false
            return false;
        }

        if (strcmp(name, ".") == 0)
        {
            return false;
        }

        if (strcmp(name, "..") == 0)
        {
            return false;
        }

        // early out the "always true" filters
        if (strcmp(filter, "*.*") == 0)
        {
            return true;
        }

        if (strcmp(filter, "*") == 0)
        {
            return true;
        }

        //if we are here it implies that name is not empty and hence these filter will always match
        if (strcmp(filter, "*?") == 0 || strcmp(filter, "?*") == 0)
        {
            return true;
        }

        const char* findCharPos = name;
        const char* findFilterPos = filter;

        if (*findFilterPos == '*')
        {
            ++findFilterPos;
            if (*findFilterPos == '*')
            {
                return wildcard_match(findFilterPos, findCharPos);
            }

            const char* findEndCharPos = findCharPos + strlen(findCharPos) - 1;
            const char* findStartCharPos = findCharPos;

            // Our strategy is that we will start from the end and continue backtracking until either we find a match or we reach to the beginning of the name,
            // for eg if the name is                  "123.systemfile.pak.3" and the filter is  "*.pak.*" , the intial positions at which the pointers point to are as below
            //                                         ^                  ^                       ^
            //                                   findStartCharPos       findEndCharPos      findFilterPos
            // And than basically we will check whether the findfilter character matches the EndCharPosition character and if so than see whether there is a match.
            // This will result in checking whether ".3" matches ".pak.*", ".pak.3" matches ".pak.*" and so no untill either there is a match or we reach to the beginning of the name. 

            while (findEndCharPos >= findStartCharPos)
            {
                if (*findFilterPos == '?' || tolower(*findFilterPos) == tolower(*findEndCharPos))
                {
                    if (wildcard_match(findFilterPos, findEndCharPos))
                    {
                        return true;
                    }
                }

                --findEndCharPos;
            }

            return false;
        }
        else
        {
            if (*findFilterPos != '?')
            {
                // ordinary character
                if (tolower(*findCharPos) != tolower(*findFilterPos))
                {
                    return false;
                }
            }
            ++findCharPos;
            ++findFilterPos;

            if (findCharPos[0] && !findFilterPos[0])
            {
                return false;
            }
        }

        return wildcard_match(findFilterPos, findCharPos);

    }

    template<class Element, class Traits, class Allocator>
    bool wildcard_match(const basic_string<Element, Traits, Allocator>& pattern, const basic_string<Element, Traits, Allocator>& string)
    {
        return wildcard_match(pattern.c_str(), string.c_str());
    }

    template<class Element, class Traits, class Allocator>
    bool wildcard_match(const char* pattern, const basic_string<Element, Traits, Allocator>& string)
    {
        return wildcard_match(pattern, string.c_str());
    }
} // namespace AZStd

#endif // AZSTD_WILDCARD_MATCH_H

