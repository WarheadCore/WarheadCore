/*
 * This file is part of the WarheadCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _UTIL_H
#define _UTIL_H

#include "Define.h"
#include "Errors.h"
#include "Containers.h"
#include "Duration.h"
#include <algorithm>
#include <cctype>
#include <string>
#include <string_view>
#include <vector>
#include <list>
#include <map>
#include <optional>
#include <ace/INET_Addr.h>

// Searcher for map of structs
template<typename T, class S> struct Finder
{
    T val_;
    T S::* idMember_;

    Finder(T val, T S::* idMember) : val_(val), idMember_(idMember) {}
    bool operator()(const std::pair<int, S>& obj) { return obj.second.*idMember_ == val_; }
};

WH_COMMON_API struct tm* localtime_r(time_t const* time, struct tm* result);
WH_COMMON_API time_t LocalTimeToUTCTime(time_t time);
WH_COMMON_API time_t GetLocalHourTimestamp(time_t time, uint8 hour, bool onlyAfterTime = true);
WH_COMMON_API tm TimeBreakdown(time_t t);

WH_COMMON_API std::optional<int32> MoneyStringToMoney(const std::string& moneyString);
WH_COMMON_API std::string secsToTimeString(uint64 timeInSecs, bool shortText = false);
WH_COMMON_API uint32 TimeStringToSecs(const std::string& timestring);
WH_COMMON_API std::string TimeToTimestampStr(time_t t);
WH_COMMON_API std::string TimeToHumanReadable(time_t t);

/* Return a random number in the range min..max. */
WH_COMMON_API int32 irand(int32 min, int32 max);

/* Return a random number in the range min..max (inclusive). */
WH_COMMON_API uint32 urand(uint32 min, uint32 max);

/* Return a random number in the range 0 .. UINT32_MAX. */
WH_COMMON_API uint32 rand32();

/* Return a random time in the range min..max (up to millisecond precision). Only works for values where millisecond difference is a valid uint32. */
WH_COMMON_API Milliseconds randtime(Milliseconds min, Milliseconds max);

/* Return a random number in the range min..max */
WH_COMMON_API float frand(float min, float max);

/* Return a random double from 0.0 to 1.0 (exclusive). */
WH_COMMON_API double rand_norm();

/* Return a random double from 0.0 to 100.0 (exclusive). */
WH_COMMON_API double rand_chance();

WH_COMMON_API uint32 urandweighted(size_t count, double const* chances);

/* Return true if a random roll fits in the specified chance (range 0-100). */
inline bool roll_chance_f(float chance)
{
    return chance > rand_chance();
}

/* Return true if a random roll fits in the specified chance (range 0-100). */
inline bool roll_chance_i(int32 chance)
{
    return chance > irand(0, 99);
}

inline void ApplyPercentModFloatVar(float& var, float val, bool apply)
{
    if (val == -100.0f)     // prevent set var to zero
        val = -99.99f;
    var *= (apply ? (100.0f + val) / 100.0f : 100.0f / (100.0f + val));
}

// Percentage calculation
template <class T, class U>
inline T CalculatePct(T base, U pct)
{
    return T(base * static_cast<float>(pct) / 100.0f);
}

template <class T, class U>
inline T AddPct(T& base, U pct)
{
    return base += CalculatePct(base, pct);
}

template <class T, class U>
inline T ApplyPct(T& base, U pct)
{
    return base = CalculatePct(base, pct);
}

template <class T>
inline T RoundToInterval(T& num, T floor, T ceil)
{
    return num = std::min(std::max(num, floor), ceil);
}

// UTF8 handling
WH_COMMON_API bool Utf8toWStr(std::string_view utf8str, std::wstring& wstr);

// in wsize==max size of buffer, out wsize==real string size
WH_COMMON_API bool Utf8toWStr(char const* utf8str, size_t csize, wchar_t* wstr, size_t& wsize);

inline bool Utf8toWStr(std::string_view utf8str, wchar_t* wstr, size_t& wsize)
{
    return Utf8toWStr(utf8str.data(), utf8str.size(), wstr, wsize);
}

WH_COMMON_API bool WStrToUtf8(std::wstring_view wstr, std::string& utf8str);

// size==real string size
WH_COMMON_API bool WStrToUtf8(wchar_t* wstr, size_t size, std::string& utf8str);

// set string to "" if invalid utf8 sequence
WH_COMMON_API size_t utf8length(std::string& utf8str);
WH_COMMON_API void utf8truncate(std::string& utf8str, size_t len);

inline bool isBasicLatinCharacter(wchar_t wchar)
{
    if (wchar >= L'a' && wchar <= L'z')                      // LATIN SMALL LETTER A - LATIN SMALL LETTER Z
        return true;
    if (wchar >= L'A' && wchar <= L'Z')                      // LATIN CAPITAL LETTER A - LATIN CAPITAL LETTER Z
        return true;
    return false;
}

inline bool isExtendedLatinCharacter(wchar_t wchar)
{
    if (isBasicLatinCharacter(wchar))
        return true;
    if (wchar >= 0x00C0 && wchar <= 0x00D6)                  // LATIN CAPITAL LETTER A WITH GRAVE - LATIN CAPITAL LETTER O WITH DIAERESIS
        return true;
    if (wchar >= 0x00D8 && wchar <= 0x00DE)                  // LATIN CAPITAL LETTER O WITH STROKE - LATIN CAPITAL LETTER THORN
        return true;
    if (wchar == 0x00DF)                                     // LATIN SMALL LETTER SHARP S
        return true;
    if (wchar >= 0x00E0 && wchar <= 0x00F6)                  // LATIN SMALL LETTER A WITH GRAVE - LATIN SMALL LETTER O WITH DIAERESIS
        return true;
    if (wchar >= 0x00F8 && wchar <= 0x00FE)                  // LATIN SMALL LETTER O WITH STROKE - LATIN SMALL LETTER THORN
        return true;
    if (wchar >= 0x0100 && wchar <= 0x012F)                  // LATIN CAPITAL LETTER A WITH MACRON - LATIN SMALL LETTER I WITH OGONEK
        return true;
    if (wchar == 0x1E9E)                                     // LATIN CAPITAL LETTER SHARP S
        return true;
    return false;
}

inline bool isCyrillicCharacter(wchar_t wchar)
{
    if (wchar >= 0x0410 && wchar <= 0x044F)                  // CYRILLIC CAPITAL LETTER A - CYRILLIC SMALL LETTER YA
        return true;
    if (wchar == 0x0401 || wchar == 0x0451)                  // CYRILLIC CAPITAL LETTER IO, CYRILLIC SMALL LETTER IO
        return true;
    return false;
}

inline bool isEastAsianCharacter(wchar_t wchar)
{
    if (wchar >= 0x1100 && wchar <= 0x11F9)                  // Hangul Jamo
        return true;
    if (wchar >= 0x3041 && wchar <= 0x30FF)                  // Hiragana + Katakana
        return true;
    if (wchar >= 0x3131 && wchar <= 0x318E)                  // Hangul Compatibility Jamo
        return true;
    if (wchar >= 0x31F0 && wchar <= 0x31FF)                  // Katakana Phonetic Ext.
        return true;
    if (wchar >= 0x3400 && wchar <= 0x4DB5)                  // CJK Ideographs Ext. A
        return true;
    if (wchar >= 0x4E00 && wchar <= 0x9FC3)                  // Unified CJK Ideographs
        return true;
    if (wchar >= 0xAC00 && wchar <= 0xD7A3)                  // Hangul Syllables
        return true;
    if (wchar >= 0xFF01 && wchar <= 0xFFEE)                  // Halfwidth forms
        return true;
    return false;
}

inline bool isNumeric(wchar_t wchar)
{
    return (wchar >= L'0' && wchar <= L'9');
}

inline bool isNumeric(char c)
{
    return (c >= '0' && c <= '9');
}

inline bool isNumeric(char const* str)
{
    for (char const* c = str; *c; ++c)
        if (!isNumeric(*c))
            return false;

    return true;
}

inline bool isNumericOrSpace(wchar_t wchar)
{
    return isNumeric(wchar) || wchar == L' ';
}

inline bool isBasicLatinString(std::wstring_view wstr, bool numericOrSpace)
{
    for (wchar_t c : wstr)
        if (!isBasicLatinCharacter(c) && (!numericOrSpace || !isNumericOrSpace(c)))
            return false;
    return true;
}

inline bool isExtendedLatinString(std::wstring_view wstr, bool numericOrSpace)
{
    for (wchar_t c : wstr)
        if (!isExtendedLatinCharacter(c) && (!numericOrSpace || !isNumericOrSpace(c)))
            return false;
    return true;
}

inline bool isCyrillicString(std::wstring_view wstr, bool numericOrSpace)
{
    for (wchar_t c : wstr)
        if (!isCyrillicCharacter(c) && (!numericOrSpace || !isNumericOrSpace(c)))
            return false;
    return true;
}

inline bool isEastAsianString(std::wstring_view wstr, bool numericOrSpace)
{
    for (wchar_t c : wstr)
        if (!isEastAsianCharacter(c) && (!numericOrSpace || !isNumericOrSpace(c)))
            return false;
    return true;
}

inline wchar_t wcharToUpper(wchar_t wchar)
{
    if (wchar >= L'a' && wchar <= L'z')                      // LATIN SMALL LETTER A - LATIN SMALL LETTER Z
        return wchar_t(uint16(wchar) - 0x0020);
    if (wchar == 0x00DF)                                     // LATIN SMALL LETTER SHARP S
        return wchar_t(0x1E9E);
    if (wchar >= 0x00E0 && wchar <= 0x00F6)                  // LATIN SMALL LETTER A WITH GRAVE - LATIN SMALL LETTER O WITH DIAERESIS
        return wchar_t(uint16(wchar) - 0x0020);
    if (wchar >= 0x00F8 && wchar <= 0x00FE)                  // LATIN SMALL LETTER O WITH STROKE - LATIN SMALL LETTER THORN
        return wchar_t(uint16(wchar) - 0x0020);
    if (wchar >= 0x0101 && wchar <= 0x012F)                  // LATIN SMALL LETTER A WITH MACRON - LATIN SMALL LETTER I WITH OGONEK (only %2=1)
    {
        if (wchar % 2 == 1)
            return wchar_t(uint16(wchar) - 0x0001);
    }
    if (wchar >= 0x0430 && wchar <= 0x044F)                  // CYRILLIC SMALL LETTER A - CYRILLIC SMALL LETTER YA
        return wchar_t(uint16(wchar) - 0x0020);
    if (wchar == 0x0451)                                     // CYRILLIC SMALL LETTER IO
        return wchar_t(0x0401);

    return wchar;
}

inline wchar_t wcharToUpperOnlyLatin(wchar_t wchar)
{
    return isBasicLatinCharacter(wchar) ? wcharToUpper(wchar) : wchar;
}

inline wchar_t wcharToLower(wchar_t wchar)
{
    if (wchar >= L'A' && wchar <= L'Z')                      // LATIN CAPITAL LETTER A - LATIN CAPITAL LETTER Z
        return wchar_t(uint16(wchar) + 0x0020);
    if (wchar >= 0x00C0 && wchar <= 0x00D6)                  // LATIN CAPITAL LETTER A WITH GRAVE - LATIN CAPITAL LETTER O WITH DIAERESIS
        return wchar_t(uint16(wchar) + 0x0020);
    if (wchar >= 0x00D8 && wchar <= 0x00DE)                  // LATIN CAPITAL LETTER O WITH STROKE - LATIN CAPITAL LETTER THORN
        return wchar_t(uint16(wchar) + 0x0020);
    if (wchar >= 0x0100 && wchar <= 0x012E)                  // LATIN CAPITAL LETTER A WITH MACRON - LATIN CAPITAL LETTER I WITH OGONEK (only %2=0)
    {
        if (wchar % 2 == 0)
            return wchar_t(uint16(wchar) + 0x0001);
    }
    if (wchar == 0x1E9E)                                     // LATIN CAPITAL LETTER SHARP S
        return wchar_t(0x00DF);
    if (wchar == 0x0401)                                     // CYRILLIC CAPITAL LETTER IO
        return wchar_t(0x0451);
    if (wchar >= 0x0410 && wchar <= 0x042F)                  // CYRILLIC CAPITAL LETTER A - CYRILLIC CAPITAL LETTER YA
        return wchar_t(uint16(wchar) + 0x0020);

    return wchar;
}

inline char charToUpper(char c) { return std::toupper(c); }
inline char charToLower(char c) { return std::tolower(c); }

WH_COMMON_API void wstrToUpper(std::wstring& str);
WH_COMMON_API void wstrToLower(std::wstring& str);
WH_COMMON_API void strToUpper(std::string& str);
WH_COMMON_API void strToLower(std::string& str);

WH_COMMON_API std::wstring GetMainPartOfName(std::wstring const& wname, uint32 declension);

WH_COMMON_API bool utf8ToConsole(std::string_view utf8str, std::string& conStr);
WH_COMMON_API bool consoleToUtf8(std::string_view conStr, std::string& utf8str);
WH_COMMON_API bool Utf8FitTo(std::string_view str, std::wstring_view search);
WH_COMMON_API void utf8printf(FILE* out, const char* str, ...);
WH_COMMON_API void vutf8printf(FILE* out, const char* str, va_list* ap);
WH_COMMON_API bool Utf8ToUpperOnlyLatin(std::string& utf8String);

WH_COMMON_API bool IsIPAddress(char const* ipaddress);

/// Checks if address belongs to the a network with specified submask
WH_COMMON_API bool IsIPAddrInNetwork(ACE_INET_Addr const& net, ACE_INET_Addr const& addr, ACE_INET_Addr const& subnetMask);

/// Transforms ACE_INET_Addr address into string format "dotted_ip:port"
WH_COMMON_API std::string GetAddressString(ACE_INET_Addr const& addr);

WH_COMMON_API uint32 CreatePIDFile(const std::string& filename);
WH_COMMON_API uint32 GetPID();

WH_COMMON_API std::string ByteArrayToHexStr(uint8 const* bytes, uint32 length, bool reverse = false);
WH_COMMON_API void HexStrToByteArray(std::string_view str, uint8* out, bool reverse = false);
WH_COMMON_API bool StringEqualI(std::string_view str1, std::string_view str2);
WH_COMMON_API bool StringStartsWith(std::string_view haystack, std::string_view needle);

WH_COMMON_API bool StringContainsStringI(std::string_view haystack, std::string_view needle);
template <typename T>
inline bool ValueContainsStringI(std::pair<T, std::string> const& haystack, std::string_view needle)
{
    return StringContainsStringI(haystack.second, needle);
}
#endif

//handler for operations on large flags
#ifndef _FLAG96
#define _FLAG96

// simple class for not-modifyable list
template <typename T>
class HookList
{
    typedef typename std::list<T>::iterator ListIterator;
private:
    typename std::list<T> m_list;
public:
    HookList<T>& operator+=(T t)
    {
        m_list.push_back(t);
        return *this;
    }
    HookList<T>& operator-=(T t)
    {
        m_list.remove(t);
        return *this;
    }
    size_t size()
    {
        return m_list.size();
    }
    ListIterator begin()
    {
        return m_list.begin();
    }
    ListIterator end()
    {
        return m_list.end();
    }
};

class WH_COMMON_API flag96
{
private:
    uint32 part[3];

public:
    flag96(uint32 p1 = 0, uint32 p2 = 0, uint32 p3 = 0)
    {
        part[0] = p1;
        part[1] = p2;
        part[2] = p3;
    }

    inline bool IsEqual(uint32 p1 = 0, uint32 p2 = 0, uint32 p3 = 0) const
    {
        return (part[0] == p1 && part[1] == p2 && part[2] == p3);
    }

    inline bool HasFlag(uint32 p1 = 0, uint32 p2 = 0, uint32 p3 = 0) const
    {
        return (part[0] & p1 || part[1] & p2 || part[2] & p3);
    }

    inline void Set(uint32 p1 = 0, uint32 p2 = 0, uint32 p3 = 0)
    {
        part[0] = p1;
        part[1] = p2;
        part[2] = p3;
    }

    inline bool operator<(flag96 const& right) const
    {
        for (uint8 i = 3; i > 0; --i)
        {
            if (part[i - 1] < right.part[i - 1])
                return true;
            else if (part[i - 1] > right.part[i - 1])
                return false;
        }
        return false;
    }

    inline bool operator==(flag96 const& right) const
    {
        return
            (
                part[0] == right.part[0] &&
                part[1] == right.part[1] &&
                part[2] == right.part[2]
            );
    }

    inline bool operator!=(flag96 const& right) const
    {
        return !(*this == right);
    }

    inline flag96& operator=(flag96 const& right)
    {
        part[0] = right.part[0];
        part[1] = right.part[1];
        part[2] = right.part[2];
        return *this;
    }
    /* requried as of C++ 11 */
#if __cplusplus >= 201103L
    flag96(const flag96&) = default;
    flag96(flag96&&) = default;
#endif

    inline flag96 operator&(flag96 const& right) const
    {
        return flag96(part[0] & right.part[0], part[1] & right.part[1], part[2] & right.part[2]);
    }

    inline flag96& operator&=(flag96 const& right)
    {
        part[0] &= right.part[0];
        part[1] &= right.part[1];
        part[2] &= right.part[2];
        return *this;
    }

    inline flag96 operator|(flag96 const& right) const
    {
        return flag96(part[0] | right.part[0], part[1] | right.part[1], part[2] | right.part[2]);
    }

    inline flag96& operator |=(flag96 const& right)
    {
        part[0] |= right.part[0];
        part[1] |= right.part[1];
        part[2] |= right.part[2];
        return *this;
    }

    inline flag96 operator~() const
    {
        return flag96(~part[0], ~part[1], ~part[2]);
    }

    inline flag96 operator^(flag96 const& right) const
    {
        return flag96(part[0] ^ right.part[0], part[1] ^ right.part[1], part[2] ^ right.part[2]);
    }

    inline flag96& operator^=(flag96 const& right)
    {
        part[0] ^= right.part[0];
        part[1] ^= right.part[1];
        part[2] ^= right.part[2];
        return *this;
    }

    inline operator bool() const
    {
        return (part[0] != 0 || part[1] != 0 || part[2] != 0);
    }

    inline bool operator !() const
    {
        return !(bool(*this));
    }

    inline uint32& operator[](uint8 el)
    {
        return part[el];
    }

    inline uint32 const& operator [](uint8 el) const
    {
        return part[el];
    }
};

enum ComparisionType
{
    COMP_TYPE_EQ = 0,
    COMP_TYPE_HIGH,
    COMP_TYPE_LOW,
    COMP_TYPE_HIGH_EQ,
    COMP_TYPE_LOW_EQ,
    COMP_TYPE_MAX
};

template <class T>
bool CompareValues(ComparisionType type, T val1, T val2)
{
    switch (type)
    {
        case COMP_TYPE_EQ:
            return val1 == val2;
        case COMP_TYPE_HIGH:
            return val1 > val2;
        case COMP_TYPE_LOW:
            return val1 < val2;
        case COMP_TYPE_HIGH_EQ:
            return val1 >= val2;
        case COMP_TYPE_LOW_EQ:
            return val1 <= val2;
        default:
            // incorrect parameter
            ABORT();
            return false;
    }
}

/*
* SFMT wrapper satisfying UniformRandomNumberGenerator concept for use in <random> algorithms
*/
class WH_COMMON_API SFMTEngine
{
public:
    typedef uint32 result_type;

    static constexpr result_type min() { return std::numeric_limits<result_type>::min(); }
    static constexpr result_type max() { return std::numeric_limits<result_type>::max(); }
    result_type operator()() const { return rand32(); }

    static SFMTEngine& Instance();
};

#endif
