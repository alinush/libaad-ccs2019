#pragma once

#include <ostream>
#include <stdexcept>
#include <algorithm>
#include <vector>
#include <array>

#include <boost/dynamic_bitset.hpp>

#include <xassert/XAssert.h>
#include <xutils/Log.h>
#include <xutils/Utils.h>

using std::endl;

namespace libaad {

class BitString : public boost::dynamic_bitset<> {
public:
    using SuperType = boost::dynamic_bitset<>;
    friend std::ostream& operator<<(std::ostream& out, const libaad::BitString& bs);

protected:

public:
    BitString(int value)
        : BitString(value, Utils::numBits(value))
    {
    }

    BitString(int value, int numBits)
    {
        int oldNumBits = numBits;
        int oldVal = value;

        while (value) {
            numBits--;
            push_back(value & 1);
            value >>= 1;
        }

        if(numBits < 0) {
            logerror << "You only provided " << oldNumBits << " bits to represent the value " << oldVal << endl;
            throw std::logic_error("not enough bits to represent value as a BitString");
        }

        while(numBits-- > 0) {
            push_back(0);
        }

        reverse();
    }

    BitString(const char * bitStr)
    {
        std::string str(bitStr);
        pushString(str);
    }

    BitString(const std::string& bitStr)
    {
        pushString(bitStr);
    }

    BitString()
    {
    }

public:    
    BitString sibling() const {
        size_t sz = SuperType::size();
        if(sz == 0)
            throw std::logic_error("Empty bit string has no sibling");

        BitString copy(*this);
        bool bit = copy[sz - 1];
        copy[sz - 1] = !bit;
        return copy;
    }

    void reverse() {
        size_t n = size();
        for(size_t i = 0; i < n / 2; i++) {
            bool t = operator[](i);
            operator[](i) = operator[](n - i - 1);
            operator[](n - i - 1) = t;
        }
    }

    //BitString prefix(int sz) {
    //    if(SuperType::size() < static_cast<size_t>(sz))
    //        throw std::logic_error("size of BitString is too small to be cut down");
    //    
    //    auto copy(*this);
    //    while(copy.size() > static_cast<size_t>(sz)) {
    //        copy.pop_back();
    //    }

    //    return copy;
    //}
    //
    //BitString suffix(int sz) {
    //    auto copy(*this);
    //    std::reverse(copy.begin(), copy.end());
    //    auto suffix = copy.prefix(sz);
    //    std::reverse(suffix.begin(), suffix.end()); 
    //    return suffix;
    //}

    /**
     * Returns prefixes in lexicographical order. For example, prefixes(011)
     * returns [ empty, 0, 01, 011 ].
     */
    //std::vector<BitString> prefixes() const {
    //    std::vector<BitString> ps;
    //
    //    ps.push_back(BitString::empty());
    //    for(size_t i = 1; i < SuperType::size(); i++) {
    //        BitString pfx(static_cast<int>(i));
    //        std::copy(SuperType::begin(), SuperType::begin() + static_cast<long>(i), pfx.begin());
    //        ps.push_back(pfx);
    //    }
    //    ps.push_back(*this);
    //
    //    return ps;
    //}

    void pushString(const std::string& bitStr) {
        for(char c : bitStr) {
            if(c == '0')
                push_back(false);
            else if (c == '1')
                push_back(true);
            else
                throw std::invalid_argument("expected bit string of ones and zeros");
        }
    }

    /**
     * Appends a bit to the BitString
     */    
    BitString& operator<<(int bit) {
        if(bit != 0 && bit != 1)
            throw std::logic_error("Can only append 0 or 1 as bits to a BitString");
        push_back(bit == 0 ? false : true);
        return *this;
    }
    
    /**
     * Appends 8 bits to the BitString
     */    
    BitString& operator<<(const std::vector<unsigned char>& bytes) {
        for(unsigned char b : bytes) {
            for (int i = 0; i < 8; i++, b = static_cast<unsigned char>(b >> 1)) {
                if (b & 0x1) {
                    push_back(true);
                } else {
                    push_back(false);
                }
            }
        }

        return *this;
    }
    
    BitString& operator<<(const std::string& bits) {
        pushString(bits);
        return *this;
    }
    
    BitString& operator<<(const BitString& bits) {
        for(size_t i = 0; i < bits.size(); i++) {
            push_back(bits[i]);
        }
        return *this;
    }

    bool operator==(const BitString& rhs) const {
        if(SuperType::size() != rhs.size())
            return false;
        else {
            for(size_t i = 0; i < rhs.size(); i++) {
                if(operator[](i) != rhs[i])
                    return false;
            }
            return true;
        }
    }

    bool operator!=(const BitString& rhs) const {
        return ! operator==(rhs);
    }
    
    bool operator<=(const BitString& rhs) const {
        return operator<(rhs) || operator==(rhs);
    }
    bool operator>=(const BitString& rhs) const {
        return operator>(rhs) || operator==(rhs);
    }

    bool operator>(const BitString& rhs) const {
        return !operator<(rhs) && !operator==(rhs);
    }

    bool operator<(const BitString& rhs) const {
        bool ret;
        //logdbg << "Is " << *this << " < " << rhs << ": ";
        if(size() < rhs.size())
            ret = true;
        else if(size() > rhs.size())
            ret = false;
        else {
            ret = false;
            for(size_t i = 0; i < rhs.size(); i++) {
                bool left = operator[](i);
                bool right = rhs[i];
                if(left > right) {
                    ret = false;
                    break;
                } else if (left < right) {
                    ret = true;
                    break;
                } else {
                    // Strings are equal so far, continue.
                }
            }
            // Strings were equal, return false!
        }
        //logdbg << ret << endl;
        return ret;
    }

    std::string toString() const {
        if(size() == 0) {
            return "empty";
        } else {
            std::string str;
            for(size_t i = 0; i < size() ; i++) {
                str += (operator[](i) ? '1' : '0');
            }
            return str;
        }
    }

public:
    static const BitString& empty() {
        static BitString epsilon = BitString();
        return epsilon;
    }
};

} // end of libaad namespace

