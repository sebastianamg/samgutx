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
        template<typename Type> class GRCodec {
            static_assert(
                    std::is_same_v<TypeSrc, std::uint8_t> ||
                    std::is_same_v<TypeSrc, std::uint16_t> ||
                    std::is_same_v<TypeSrc, std::uint32_t> ||
                    std::is_same_v<TypeSrc, std::uint64_t>,
                    "typename must be one of std::uint8_t, std::uint16_t, std::uint32_t, or std::uint64_t");
            private:
                static const std::size_t BITS_PER_BYTE = sizeof(std::uint8_t)*8;
                std::size_t m;

                /**
                 * @brief This function allows computing the log2 of x.
                 * 
                 * @param x 
                 * @return std::double_t 
                 */
                std::double_t log2(double x) {
                   return std::log(x) / std::log(2.0);
                }

                /**
                 * @brief This function allows checking whether x is or not a power of 2.
                 * 
                 * @param x 
                 * @return true 
                 * @return false 
                 */
                bool _is_power_of_2_(Type x) {
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
                sdsl::bit_vector _encode_(const Type n, const Type q, const Type r) {
                    Type    q = std::floor( ((std::double_t) n) / ((std::double_t)this->m) ),
                            r = n - ( this->m * q );

                    if( this->_is_power_of_2_(n) ) { // Acting as Rice encoder.
                        return this->_rice_encode_(n, q, r);   
                    } else { // Acting as Golomb encoder.
                        return this->_golomb_encode_(n, q, r);
                    }
                }

                sdsl::bit_vector _rice_encode_(const Type n, const Type q, const Type r) {
                    sdsl::bit_vector q_unary(q+1);
                    for (std::size_t i = q; i > 0; i--) {
                        q_unary[i] = 1;
                    } // q_unary[0] = 0 as the sdsl::bit_vector initialization default value is 0. 
                    
                    std::size_t r_bits = (std::size_t)this->log2(r);
                    sdsl::bit_vector r_binary(r_bits);
                    r_binary.set_int(0,r,GRCodec::BITS_PER_BYTE);

                    const std::size_t r_initial_size = r_binary.size();
                    r_binary.bit_resize( r_initial_size + q_unary.size() );

                    for (std::size_t i = r_initial_size; i < r_binary.size(); i+=GRCodec::BITS_PER_BYTE){
                        std::uint8_t x = q_unary.get_int(i-r_initial_size,GRCodec::BITS_PER_BYTE);
                        r_binary.set_int(i,x,GRCodec::BITS_PER_BYTE);
                    }

                    return r_binary;
                }

                sdsl::bit_vector _golomb_encode_(const Type n, const Type q, const Type r) {
                    return sdsl::bit_vector(1);
                }

            public:
                enum GRCodecType {
                    GOLOMB_RICE,
                    EXPONENTIAL_GOLOMB
                } type;

                GRCodec(std::size_t m, GRCodecType type=GRCodecType::GOLOMB_RICE): 
                    m{m}, 
                    type{type} 
                {}


                sdsl::bit_vector _encode_(const Type n) {
                    switch (expression)
                    {
                        case GRCodecType::GOLOMB_RICE:
                            return this->_encode_(n);
                        default:
                            throw std::runtime_error("Not valid or not implemented algorithm!");
                    }
                }
                
                Type decode(sdsl::bit_vector v) {
                    return 0;
                }
        };
    }
}