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
// Modifications copyright Amazon.com, Inc. or its affiliates.   
// Original code Copyright 2006 Nemanja Trifunovic


#ifndef UTF8_FOR_CPP_UNCHECKED_H_2675DCD0_9480_4c0c_B92A_CC14C027B731
#define UTF8_FOR_CPP_UNCHECKED_H_2675DCD0_9480_4c0c_B92A_CC14C027B731

#include <AzCore/std/string/utf8/core.h>

namespace Utf8
{
    namespace Unchecked 
    {
        template <typename octet_iterator>
        octet_iterator append(AZ::u32 cp, octet_iterator result)
        {
            if (cp < 0x80)                        // one octet
                *(result++) = static_cast<AZ::u8>(cp);  
            else if (cp < 0x800) {                // two octets
                *(result++) = static_cast<AZ::u8>((cp >> 6)          | 0xc0);
                *(result++) = static_cast<AZ::u8>((cp & 0x3f)        | 0x80);
            }
            else if (cp < 0x10000) {              // three octets
                *(result++) = static_cast<AZ::u8>((cp >> 12)         | 0xe0);
                *(result++) = static_cast<AZ::u8>(((cp >> 6) & 0x3f) | 0x80);
                *(result++) = static_cast<AZ::u8>((cp & 0x3f)        | 0x80);
            }
            else {                                // four octets
                *(result++) = static_cast<AZ::u8>((cp >> 18)         | 0xf0);
                *(result++) = static_cast<AZ::u8>(((cp >> 12) & 0x3f)| 0x80);
                *(result++) = static_cast<AZ::u8>(((cp >> 6) & 0x3f) | 0x80);
                *(result++) = static_cast<AZ::u8>((cp & 0x3f)        | 0x80);
            }
            return result;
        }

        template <typename octet_iterator>
        AZ::u32 next(octet_iterator& it)
        {
            AZ::u32 cp = Utf8::Internal::mask8(*it);
            typename AZStd::iterator_traits<octet_iterator>::difference_type length = Utf8::Internal::sequence_length(it);
            switch (length) {
                case 1:
                    break;
                case 2:
                    it++;
                    cp = ((cp << 6) & 0x7ff) + ((*it) & 0x3f);
                    break;
                case 3:
                    ++it; 
                    cp = ((cp << 12) & 0xffff) + ((Utf8::Internal::mask8(*it) << 6) & 0xfff);
                    ++it;
                    cp += (*it) & 0x3f;
                    break;
                case 4:
                    ++it;
                    cp = ((cp << 18) & 0x1fffff) + ((Utf8::Internal::mask8(*it) << 12) & 0x3ffff);                
                    ++it;
                    cp += (Utf8::Internal::mask8(*it) << 6) & 0xfff;
                    ++it;
                    cp += (*it) & 0x3f; 
                    break;
            }
            ++it;
            return cp;        
        }

        template <typename octet_iterator>
        AZ::u32 peek_next(octet_iterator it)
        {
            return Utf8::Unchecked::next(it);    
        }

        template <typename octet_iterator>
        AZ::u32 prior(octet_iterator& it)
        {
            while (Utf8::Internal::is_trail(*(--it))) ;
            octet_iterator temp = it;
            return Utf8::Unchecked::next(temp);
        }

        // Deprecated in versions that include prior, but only for the sake of consistency (see Utf8::previous)
        template <typename octet_iterator>
        inline AZ::u32 previous(octet_iterator& it)
        {
            return Utf8::Unchecked::prior(it);
        }

        template <typename octet_iterator, typename distance_type>
        void advance (octet_iterator& it, distance_type n)
        {
            for (distance_type i = 0; i < n; ++i)
                Utf8::Unchecked::next(it);
        }

        template <typename octet_iterator>
        typename AZStd::iterator_traits<octet_iterator>::difference_type
        distance (octet_iterator first, octet_iterator last)
        {
            typename AZStd::iterator_traits<octet_iterator>::difference_type dist;
            for (dist = 0; first < last; ++dist) 
                Utf8::Unchecked::next(first);
            return dist;
        }

        template <typename u16bit_iterator, typename octet_iterator>
        octet_iterator utf16to8 (u16bit_iterator start, u16bit_iterator end, octet_iterator result)
        {       
            while (start != end) {
                AZ::u32 cp = Utf8::Internal::mask16(*start++);
            // Take care of surrogate pairs first
                if (Utf8::Internal::is_lead_surrogate(cp)) {
                    AZ::u32 trail_surrogate = Utf8::Internal::mask16(*start++);
                    cp = (cp << 10) + trail_surrogate + Internal::SURROGATE_OFFSET;
                }
                result = Utf8::Unchecked::append(cp, result);
            }
            return result;         
        }

        template <typename u16bit_iterator, typename octet_iterator>
        u16bit_iterator utf8to16 (octet_iterator start, octet_iterator end, u16bit_iterator result)
        {
            while (start < end) {
                AZ::u32 cp = Utf8::Unchecked::next(start);
                if (cp > 0xffff) { //make a surrogate pair
                    *result++ = static_cast<AZ::u16>((cp >> 10)   + Internal::LEAD_OFFSET);
                    *result++ = static_cast<AZ::u16>((cp & 0x3ff) + Internal::TRAIL_SURROGATE_MIN);
                }
                else
                    *result++ = static_cast<AZ::u16>(cp);
            }
            return result;
        }

        template <typename octet_iterator, typename u32bit_iterator>
        octet_iterator utf32to8 (u32bit_iterator start, u32bit_iterator end, octet_iterator result)
        {
            while (start != end)
                result = Utf8::Unchecked::append(*(start++), result);

            return result;
        }

        template <typename octet_iterator, typename u32bit_iterator>
        u32bit_iterator utf8to32 (octet_iterator start, octet_iterator end, u32bit_iterator result)
        {
            while (start < end)
                (*result++) = Utf8::Unchecked::next(start);

            return result;
        }

        // The iterator class
        template <typename octet_iterator>
          class iterator : public AZStd::iterator <AZStd::bidirectional_iterator_tag, AZ::u32> 
		  { 
            octet_iterator it;
            public:
            iterator () {}
            explicit iterator (const octet_iterator& octet_it): it(octet_it) {}
            // the default "big three" are OK
            octet_iterator base () const { return it; }
            AZ::u32 operator * () const
            {
                octet_iterator temp = it;
                return Utf8::Unchecked::next(temp);
            }
            bool operator == (const iterator& rhs) const 
            { 
                return (it == rhs.it);
            }
            bool operator != (const iterator& rhs) const
            {
                return !(operator == (rhs));
            }
            iterator& operator ++ () 
            {
                AZStd::advance(it, Utf8::Internal::sequence_length(it));
                return *this;
            }
            iterator operator ++ (int)
            {
                iterator temp = *this;
                AZStd::advance(it, Utf8::Internal::sequence_length(it));
                return temp;
            }  
            iterator& operator -- ()
            {
                Utf8::Unchecked::prior(it);
                return *this;
            }
            iterator operator -- (int)
            {
                iterator temp = *this;
                Utf8::Unchecked::prior(it);
                return temp;
            }
          }; // class iterator
    } // namespace Utf8::unchecked
} // namespace utf8 


#endif // header guard

