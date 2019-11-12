#pragma once

#include <sstream>
#include <string>
#include <sstream>        
#include <gmp.h>

#include <aad/BitString.h>
#include <aad/EllipticCurves.h>
#include <aad/PicoSha2.h>

#include <xassert/XAssert.h>

namespace boost {
    template <typename B, typename A>
    std::size_t hash_value(const boost::dynamic_bitset<B, A>& bs) {
        return boost::hash_value(bs.m_bits);
    }
}

using libaad::Fr;
using libaad::G1;

namespace libaad {

constexpr int MerkleHashSize = 32;

/**
 * Used to store a hash of a node in a Merkle tree.
 */
class MerkleHash {
protected:
    std::vector<unsigned char> hash;

    friend std::ostream& operator<<(std::ostream&, const MerkleHash& );

public:
    MerkleHash() {
    }

    MerkleHash(const G1& acc, const MerkleHash& left, const MerkleHash& right) {
        // Get accumulator as hexadecimal string
        // WARNING: Assumes libff hash been compiled with BINARY_OUTPUT undefined
        std::stringstream ss;
        ss << acc;
        std::string str = ss.str();
        
        hash.resize(MerkleHashSize);

        // Computes SHA256(left | hex(acc) | right)
        picosha2::hash256_one_by_one hasher;
        hasher.process(left.hash.begin(), left.hash.end());
        hasher.process(str.begin(), str.end());
        hasher.process(right.hash.begin(), right.hash.end());
        hasher.finish();
        hasher.get_hash_bytes(hash.begin(), hash.end());
    }

    MerkleHash(const G1& acc)
        : MerkleHash(acc, empty(), empty())
    {}

public:
    bool isUnset() const { return hash.empty(); }

    bool operator!=(const MerkleHash& other) const {
        return operator==(other) == false;
    }

    bool operator==(const MerkleHash& other) const {
        return hash == other.hash;
    }

public:
    static const MerkleHash& empty() {
        static MerkleHash allZeros;

        if(allZeros.hash.empty()) {
            for(int i = 0; i < MerkleHashSize; i++) {
                allZeros.hash.push_back(0x00);
            }
        }

        return allZeros;
    }

    static const MerkleHash& dummy() {
        static MerkleHash allOnes;

        if(allOnes.hash.empty()) {
            for(int i = 0; i < MerkleHashSize; i++) {
                allOnes.hash.push_back(0xFF);
            }
        }

        return allOnes;
    }
};

std::ostream& operator<<(std::ostream& out, const MerkleHash& h) {
    if(h == MerkleHash::dummy())
        out << "'dummy'";
    else if(h == MerkleHash::empty())
        out << "'empty'";
    else
        out << "some " << h.hash.size() << "-byte hash";
    return out;
}

/**
 * Takes a bit string (could be an AT node or a hash of a key) and cryptographically
 * hashes it into a finite field element.
 */
Fr hashToField(const BitString& bs) {
    std::string hashHex;
    
    picosha2::hash256_hex_string(bs.toString(), hashHex);

    //std::vector<boost::dynamic_bitset<>::block_type> bytes;
    //boost::to_block_range(bs, std::back_inserter(bytes));
    //picosha2::hash256_one_by_one hasher;
    // NOTE: I'm a bit skeptical that this does the right thing because block_type is ulong 
    // and picosha copies it to char's using std::copy, making it seem like it's casting a
    // ulong to a char.
    //hasher.process(bytes.begin(), bytes.end());
    //hasher.finish();
    //get_hash_hex_string(hasher, hashHex);

    hashHex.pop_back(); // small enough for field

    mpz_t rop;
    mpz_init(rop);
    mpz_set_str(rop, hashHex.c_str(), 16);

    Fr fr = libff::bigint<Fr::num_limbs>(rop);
    mpz_clear(rop);

    return fr;
}

void hashToField(const std::vector<BitString>& in, std::vector<Fr>& hashes) {
    for(auto& bs : in) {
        hashes.push_back(hashToField(bs));
    }
}

void hashToField(std::vector<BitString>::const_iterator beg, std::vector<BitString>::const_iterator end, std::vector<Fr>& hashes) {
    while(beg != end) {
        hashes.push_back(hashToField(*beg));
        beg++;
    }
}

template<class T>
class NonCryptoHash {
public:
    size_t operator()(const T& v) const {
        // First, convert value to string
        std::stringstream ss;
        std::string str;
        ss << v;
        ss >> str;

        // Hash using SHA...
        std::vector<unsigned char> h(picosha2::k_digest_size);
        picosha2::hash256(str, h);

        // ...and truncate
        size_t bytes = sizeof(size_t);
        size_t truncHash = 0;
        for(size_t i = 0; i < bytes; i++) {
            truncHash += h[i];
            truncHash <<= 8;
        }
        
        return truncHash;
    }
};

BitString hashString(const std::string& s) {
    std::vector<unsigned char> hash(picosha2::k_digest_size);
    picosha2::hash256(s, hash);  // SHA256(k)
    BitString bs;
    bs << hash;
    return bs;
}

BitString hashKey(const std::string& k) {
    return hashString(k);
}

BitString hashValue(const std::string& v, int idx) {
    std::vector<unsigned char> valHash(picosha2::k_digest_size);
    std::vector<unsigned char> idxHash(picosha2::k_digest_size);
    std::vector<unsigned char> valIdxHash(picosha2::k_digest_size);

    picosha2::hash256(v, valHash);  // SHA256(v)
    picosha2::hash256(std::to_string(idx), idxHash); // SHA256(idx)

    // Computes SHA256(SHA256(v) | SHA256(idx))
    picosha2::hash256_one_by_one hasher;
    hasher.process(valHash.begin(), valHash.end());
    hasher.process(idxHash.begin(), idxHash.end());
    hasher.finish();
    hasher.get_hash_bytes(valIdxHash.begin(), valIdxHash.end());

    BitString hash;
    hash << valIdxHash;

    return hash;
}

BitString hashKeyValuePair(const std::string& k, const std::string& v, int idx) {
    assertEqual(picosha2::k_digest_size, 256 / 8);

    BitString hash;
    hash << hashKey(k);
    hash << hashValue(v, idx);

    return hash;
}

} // end of libaad namespace
