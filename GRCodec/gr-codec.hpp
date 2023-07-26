/**
 * ---------------------------------------------------------------
 * Released under the 2-Clause BSD License 
 * (a.k.a. Simplified BSD License or FreeBSD License)
 * @note [link https://opensource.org/license/bsd-2-clause/ BSD-2-Clause]
 * ---------------------------------------------------------------
 * 
 * @copyright (c) 2023 Sebastián AMG (@sebastianamg)
 * 
 * Redistribution and use in source and binary forms, with or 
 * without modification, are permitted provided 
 * that the following conditions are met:
 *  1.  Redistributions of source code must retain the above 
 *      copyright notice, this list of conditions and the 
 *      following disclaimer.
 * 
 *  2.  Redistributions in binary form must reproduce the 
 *      above copyright notice, this list of conditions and
 *      the following disclaimer in the documentation and/or 
 *      other materials provided with the distribution.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND 
 * CONTRIBUTORS “AS IS” AND ANY EXPRESS OR IMPLIED WARRANTIES, 
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF 
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE 
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR 
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT 
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; 
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN 
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF 
 * THE POSSIBILITY OF SUCH DAMAGE. 
 */
#pragma once

#include <vector>
#include <cstdint>
#include <cmath>
#include <stdexcept>
#include <sdsl/bit_vectors.hpp>

namespace samg {
    namespace grcodec {

        enum GRCodecType {
            GOLOMB_RICE,
            EXPONENTIAL_GOLOMB
        };

        /**
         * @brief This class represents a Golomb-Rice encoding of a sequence of integers.
         * 
         * @tparam Type 
         */
        template<typename Type> class GRCodec {
            static_assert(
                    std::is_same_v<Type, std::uint8_t> ||
                    std::is_same_v<Type, std::uint16_t> ||
                    std::is_same_v<Type, std::uint32_t> ||
                    std::is_same_v<Type, std::uint64_t>,
                    "typename must be one of std::uint8_t, std::uint16_t, std::uint32_t, or std::uint64_t");
            private:
                static const std::size_t BITS_PER_BYTE = sizeof(std::uint8_t)*8;
                std::size_t m;
                sdsl::bit_vector sequence;

                /**
                 * @brief This function allows computing the log2 of x.
                 * 
                 * @param x 
                 * @return std::double_t 
                 */
                static const std::double_t log2(const double x) {
                   return std::log(x) / std::log(2.0);
                }

                /**
                 * @brief This function allows checking whether x is or not a power of 2.
                 * 
                 * @param x 
                 * @return true 
                 * @return false 
                 */
                const bool _is_power_of_2_(const Type x) const {
                    /* Examples:
                    x         = 0000 0111 = 7
                    x - 1     = 0000 0110
                    x & (x-1) = 0000 0110 --> false
                    ---------------------
                    x         = 0001 0000 = 16
                    x - 1     = 0000 1111
                    x & (x-1) = 0000 0000 --> true
                    ---------------------
                    x         = 0010 0101 = 37
                    x - 1     = 0010 0100
                    x & (x-1) = 0010 0100 --> false
                    */
                    return (x > 0) && ((x & (x - 1)) == 0);
                }

                /**
                 * @brief This function allows referring encoding computation to either Rice or Golomb functions based on whether n is a power of 2 or not, respectively. 
                 * 
                 * @param n 
                 * @param q 
                 * @param r 
                 * @return std::vector<std::uint8_t> 
                 */
                sdsl::bit_vector _encode_golomb_rice_(const Type n) {
                    Type    q = std::floor( ((std::double_t) n) / ((std::double_t)this->m) ),
                            r = n - ( this->m * q );

                    if( this->_is_power_of_2_(this->m) ) { // Acting as Rice encoder.
                        return this->_rice_encode_(n, q, r);   
                    } else { // Acting as Golomb encoder.
                        return this->_golomb_encode_(n, q, r);
                    }
                }

                /**
                 * @brief This function allows encoding n using the Rice algorithm. 
                 * 
                 * @param n 
                 * @param q 
                 * @param r 
                 * @return sdsl::bit_vector 
                 */
                sdsl::bit_vector _rice_encode_(const Type n, const Type q, const Type r) {
                    std::size_t r_bits = (std::size_t)this->log2(this->m);
                    sdsl::bit_vector v(r_bits + q + 1);
                    v.set_int(0,r,GRCodec::BITS_PER_BYTE);

                    for (std::size_t i = r_bits + 1; i < (r_bits + q + 1); ++i) {
                        v[i] = 1;
                    } // v[r_bits] = 0 as the sdsl::bit_vector initialization default value is 0. 
                    
                    return v;
                }

                /**
                 * @brief This function allows encoding n using the Golomb algorithm. 
                 * 
                 * @param n 
                 * @param q 
                 * @param r 
                 * @return sdsl::bit_vector 
                 */
                sdsl::bit_vector _golomb_encode_(const Type n, const Type q, const Type r) {
                    std::size_t c = std::pow(2,std::ceil(GRCodec::log2(this->m))) - this->m;
                    std::double_t x = this->log2(this->m);
                    bool phase = r < c;
                    std::size_t r_bits = (std::size_t) ( phase ? std::floor(x) : std::ceil(x) );
                    sdsl::bit_vector v(r_bits + q + 1);
                    
                    if( phase ) {
                        v.set_int(0,r,GRCodec::BITS_PER_BYTE);
                    } else {
                        v.set_int(0,r+c,GRCodec::BITS_PER_BYTE);
                    }

                    for (std::size_t i = r_bits + 1; i < (r_bits + q + 1); ++i) {
                        v[i] = 1;
                    } // v[r_bits] = 0 as the sdsl::bit_vector initialization default value is 0. 
                    
                    return v;
                }

                /**
                 * @brief This function allows referring decoding computation to either Rice or Golomb functions based on whether n is a power of 2 or not, respectively. 
                 * 
                 * @param v 
                 * @return Type 
                 */
                Type _decode_golomb_rice_(sdsl::bit_vector &v) {
                    std::size_t A = 0ul, 
                                s = std::ceil(this->log2(this->m)),
                                i = v.size()-1;

                    for (; 0 <= i && i < v.size() ; --i) {
                        if( v[i] == 1 ) {
                            ++A;
                            v[i] = 0;
                        } else { 
                            break;
                        }
                    }

                    if( this->_is_power_of_2_(this->m) ) { // Acting as Rice decoder.
                        return this->_rice_decode_(v, A);
                    } else { // Acting as Golomb decoder.
                        return this->_golomb_decode_(v, A);
                    }
                }

                /**
                 * @brief This function allows decoding n using the Rice algorithm. 
                 * 
                 * @param v 
                 * @param A 
                 * @return Type 
                 */
                Type _rice_decode_(sdsl::bit_vector &v, const std::size_t A) {
                    Type R = v.get_int( 0, sizeof(Type) * BITS_PER_BYTE );
                    return ( this->m * A ) + R;
                }

                /**
                 * @brief This function allows decoding n using the Golomb algorithm. 
                 * 
                 * @param v 
                 * @param A 
                 * @return Type 
                 */
                Type _golomb_decode_(sdsl::bit_vector &v, const std::size_t A) {
                    std::size_t c = std::pow(2,std::ceil(GRCodec::log2(this->m))) - this->m;
                    Type R = v.get_int( 0, sizeof(Type) * BITS_PER_BYTE ),
                            n = this->_rice_decode_(v,A);

                    return ( R >= c )? ( n - c ) : n;
                }

            public:
                GRCodecType type;

                explicit GRCodec(const std::size_t m, const GRCodecType type=GRCodecType::GOLOMB_RICE): 
                    m(m), 
                    type(type) 
                {}

                sdsl::bit_vector encode(const Type n) {
                    switch (this->type) {
                        case GRCodecType::GOLOMB_RICE:
                            return this->_encode_golomb_rice_(n);
                        default:
                            throw std::runtime_error("Not valid or not implemented algorithm!");
                    }
                }
                
                Type decode(sdsl::bit_vector v) {
                    switch (this->type) {
                        case GRCodecType::GOLOMB_RICE:
                            return this->_decode_golomb_rice_(v);
                        default:
                            throw std::runtime_error("Not valid or not implemented algorithm!");
                    }
                }

                sdsl::bit_vector get_bit_vector() {
                    return this->sequence;
                }

                friend std::ostream & operator<<(std::ostream & strm, const GRCodec &codec) {
                    sdsl::bit_vector v = codec.get_bit_vector();
                    for (std::size_t i = v.size()-1; 0 <= i && i < v.size() ; --i) {
                        strm << v[i];
                    }
                    return strm;
                }
        };
    }
}