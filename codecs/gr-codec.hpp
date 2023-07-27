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

#include <cmath>
#include <cstdint>
#include <type_traits>
#include <vector>
#include <stdexcept>
#include <cassert>
#include <utility>
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
         * 
         * @note It requires [https://github.com/simongog/sdsl-lite sdsl] library.
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
                std::uint64_t iterator_index;

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
                 * @param T
                 * @return true 
                 * @return false 
                 */
                static const bool _is_power_of_2_(const Type x) {
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
                 * @brief This function allows getting a sub-vector from v.
                 * 
                 * @param v 
                 * @param begin 
                 * @param length 
                 * @return sdsl::bit_vector 
                 */
                static sdsl::bit_vector _get_sub_vector_( const sdsl::bit_vector &v, const std::uint64_t begin, const std::size_t length ) {
                    sdsl::bit_vector rv(length);
                    for (std::size_t i = begin; i < (begin + length); i++) {
                        rv[i-begin] = v[i];
                    }
                    return rv;
                }

                /**
                 * @brief This function allows converting from unary into decimal.
                 * 
                 * @param v 
                 * @return const std::size_t 
                 * 
                 * @warning It is assumed that v starts with unary code.
                 */
                static const std::size_t _decode_unary_( const sdsl::bit_vector &v, std::uint64_t begin ) {
                    /* NOTE
                        987654
                        111110
                        begin = 9
                        i = 987654
                                 ^
                        Therefore 9 - 4 = 5 ones!
                    */
                    // Counting ones:
                    std::size_t i = begin;
                    for (; 0 <= i && i < v.size() ; --i) {
                        if( v[i] == 0 ) {
                            break;
                        }
                    }
                    return (begin - i);
                }

                /**
                 * @brief This method allows computing the next codeword length in a sequence in v.
                 * 
                 * @param v 
                 * @param m 
                 * @return const std::size_t 
                 */
                static const std::size_t _get_next_codeword_length_( const sdsl::bit_vector &v, const std::size_t m, const std::uint64_t begin ) {
                    std::size_t A = GRCodec::_decode_unary_( v, begin ), 
                                s = std::ceil(GRCodec::log2(m));
                    if(  GRCodec::_is_power_of_2_(m) ) {// Acting as Rice.
                        // Computing length:
                        return A + 1 + s;
                    } else { // Acting as Golomb.
                        // Retrieving R:
                        /* NOTE
                            9876543210
                            1111100101
                            
                            A = 5
                            begin = 9
                            s = 5
                            get sub-vector --> [ begin-A-1-(s-2) , length = s-1 ] === [ 0 , 3 ]
                        */
                        sdsl::bit_vector rv = GRCodec::_get_sub_vector_(v,begin-A-1-(s-2),s-1);
                        Type R = rv.get_int( 0, sizeof(Type) * BITS_PER_BYTE );
                        // Computing length:
                        std::size_t c = std::pow(2,std::ceil(GRCodec::log2(m))) - m;
                        return A + 1 + ( R >= c ? s : s - 1 );
                    }
                }

                /**
                 * @brief This function allows referring encoding computation to either Rice or Golomb functions based on whether n is a power of 2 or not, respectively. 
                 * 
                 * @param n 
                 * @param m 
                 * @param T
                 * @return std::vector<std::uint8_t> 
                 */
                static sdsl::bit_vector _encode_golomb_rice_(const Type n, const std::size_t m) {
                    Type    q = std::floor( ((std::double_t) n) / ((std::double_t)m) ),
                            r = n - ( m * q );

                    if(  GRCodec::_is_power_of_2_(m) ) { // Acting as Rice encoder.
                        return GRCodec::_rice_encode_( n, q, r, m );   
                    } else { // Acting as Golomb encoder.
                        return GRCodec::_golomb_encode_( n, q, r, m );
                    }
                }

                /**
                 * @brief This function allows encoding n using the Rice algorithm. 
                 * 
                 * @param n 
                 * @param q 
                 * @param r 
                 * @param m
                 * @return sdsl::bit_vector 
                 */
                static sdsl::bit_vector _rice_encode_(const Type n, const Type q, const Type r, const std::size_t m) {
                    std::size_t r_bits = (std::size_t)GRCodec::log2(m);
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
                 * @param m
                 * @return sdsl::bit_vector 
                 */
                static sdsl::bit_vector _golomb_encode_(const Type n, const Type q, const Type r, const std::size_t m) {
                    std::size_t c = std::pow(2,std::ceil(GRCodec::log2(m))) - m;
                    std::double_t x = GRCodec::log2(m);
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
                 * @param m
                 * @return Type
                 */
                static Type _decode_golomb_rice_(sdsl::bit_vector &v, const std::size_t m) {
                    std::size_t A = 0ul, 
                                s = std::ceil(GRCodec::log2(m)),
                                i = v.size()-1;

                    // Counting ones:
                    for (; 0 <= i && i < v.size() ; --i) {
                        if( v[i] == 1 ) {
                            ++A;
                            v[i] = 0;
                        } else { 
                            break;
                        }
                    }

                    if( GRCodec::_is_power_of_2_(m) ) { // Acting as Rice decoder.
                        return GRCodec::_rice_decode_( v, A, m );
                    } else { // Acting as Golomb decoder.
                        return GRCodec::_golomb_decode_( v, A, m );
                    }
                }

                /**
                 * @brief This function allows decoding n using the Rice algorithm. 
                 * 
                 * @param v 
                 * @param A 
                 * @param m
                 * @return Type 
                 */
                static Type _rice_decode_(sdsl::bit_vector &v, const std::size_t A, const std::size_t m) {
                    Type R = v.get_int( 0, sizeof(Type) * BITS_PER_BYTE );
                    return ( m * A ) + R;
                }

                /**
                 * @brief This function allows decoding n using the Golomb algorithm. 
                 * 
                 * @param v 
                 * @param A
                 * @param m 
                 * @return Type
                 */
                static Type _golomb_decode_(sdsl::bit_vector &v, const std::size_t A, const std::size_t m) {
                    std::size_t c = std::pow(2,std::ceil(GRCodec::log2(m))) - m;
                    Type    R = v.get_int( 0, sizeof(Type) * BITS_PER_BYTE ),
                            n = GRCodec::_rice_decode_( v, A, m );

                    return ( R >= c )? ( n - c ) : n;
                }

            public:
                GRCodecType type;

                GRCodec(const std::size_t m, const GRCodecType type=GRCodecType::GOLOMB_RICE): 
                    m(m), 
                    type(type)
                {
                    this->restart();
                }

                GRCodec(sdsl::bit_vector sequence, const std::size_t m, const GRCodecType type=GRCodecType::GOLOMB_RICE): 
                    sequence(sequence),
                    m(m), 
                    type(type)
                {
                    this->restart();
                }

                /**
                 * @brief This function allows encoding an integer n.
                 * 
                 * @param n 
                 * @param m 
                 * @param type 
                 * @return sdsl::bit_vector 
                 */
                static sdsl::bit_vector encode(const Type n, const std::size_t m, GRCodecType type = GRCodecType::GOLOMB_RICE ) {
                    switch( type ) {
                        case GRCodecType::GOLOMB_RICE:
                            return GRCodec::_encode_golomb_rice_( n, m );
                        default:
                            throw std::runtime_error("Not valid or not implemented algorithm!");
                    }
                }
                
                /**
                 * @brief This function allows decoding an integer from a bitmap v.
                 * 
                 * @param v 
                 * @param m 
                 * @param type 
                 * @return Type 
                 */
                static Type decode(sdsl::bit_vector v, const std::size_t m, GRCodecType type = GRCodecType::GOLOMB_RICE ) {
                    switch( type ) {
                        case GRCodecType::GOLOMB_RICE:
                            return GRCodec::_decode_golomb_rice_( v, m );
                        default:
                            throw std::runtime_error("Not valid or not implemented algorithm!");
                    }
                }

                /**
                 * @brief This function allows appending the encoded representation of n to an internal bitmap.
                 * 
                 * @param n 
                 * 
                 * @warning This implementation is O(n), where n is the number of bits in the internal bitmap.
                 */
                void append(const Type n) {
                    /* NOTE
                    ( 1 )
                    10111
                    ( 1 ) ( 2 ) 
                    10111 11001
                    ( 1 ) ( 2 ) ( 3 )
                    10111 11001 11100101
                    ( 1 ) ( 2 ) ( 3 )    ( 4 )
                    10111 11001 11100101 xxxxxx
                    */
                    // Encode n:
                    sdsl::bit_vector v = GRCodec::encode(n,this->m,this->type);

                    // Fuse both the new codeword and the previous codewords:
                    std::size_t s = v.size();
                    v.resize(s + this->sequence.size());
                    
                    for (std::size_t i = s; i < v.size(); ++i) {
                        v[i] = this->sequence[i-s];
                    }
                    this->sequence = v; // Store fused internal bitmap.

                    // Update iterator index:
                    this->iterator_index += s;
                }

                /**
                 * @brief This function iterates on the internal bitmap as decodes and returns the next integer. 
                 * 
                 * @return const Type 
                 */
                const Type next() {
                    // Retrieve codeword length:
                    std::size_t codeword_length = GRCodec::_get_next_codeword_length_( this->sequence , this->m , this->iterator_index );
                    // Copy sub-vetor defined from `0` to `codeword_length`:
                    sdsl::bit_vector v = GRCodec::_get_sub_vector_(this->sequence,this->iterator_index - ( codeword_length - 1 ) , codeword_length ); 
                    // Moving interal pointer `codeword_length` bits backward:
                    this->iterator_index -= codeword_length;
                    // Returning decoded codeword:
                    return  GRCodec::decode(v,this->m,this->type);
                } 

                /**
                 * @brief This function verifies whether the internal bitmap has or doesn't have more codewords to iterate on.
                 * 
                 * @return true 
                 * @return false 
                 */
                const bool has_more() const {
                    return this->sequence.size() > this->iterator_index && this->iterator_index >= 0ull;
                }

                /**
                 * @brief This function restarts the iteration on the internal bitmap.
                 * 
                 */
                void restart() {
                    this->iterator_index = this->sequence.size()-1;
                }

                /**
                 * @brief This function returns a copy of the internal bitmap.
                 * 
                 * @return const sdsl::bit_vector 
                 */
                const sdsl::bit_vector get_bit_vector() const {
                    return this->sequence;
                }

                /**
                 * @brief This function returns the number of stored bits.
                 * 
                 * @return const std::size_t 
                 */
                const std::size_t length() const {
                    return this->sequence.size();
                }

                const std::uint64_t get_current_iterator_index() const {
                    return this->iterator_index;
                }

                friend std::ostream & operator<<(std::ostream & strm, const GRCodec<Type> &codec) {
                    sdsl::bit_vector v = codec.get_bit_vector();
                    bool printer_prompt = false;
                    for (std::size_t i = v.size()-1; 0 <= i && i < v.size() ; --i) {
                        if( i == codec.get_current_iterator_index() ) {
                            strm << "|";
                            printer_prompt = true;
                        }
                        strm << v[i];
                    }
                    if( !printer_prompt ) {
                        strm << "|";
                    }
                    return strm;
                }
        };

        template<typename Type> std::pair<bool,std::vector<Type>> get_relative_sequence( std::vector<Type> sequence ) {
            std::vector<Type> ans;
            const bool  ASCENDING = true,
                        DESCENDING = false;
            bool    slope = ASCENDING,  // Assumed.
                    is_slope_set = false;  
            if( sequence.size() > 0 ) {
                /* NOTE
                    1 3 5 6 8 9 15 18
                    1 2 2 1 2 1 6 3
                    slope = ASCENDING | ASCENDING
                    is_slope_set = false | true
                */
                ans.push_back(sequence[0]);
                for (std::size_t i = 1; i < sequence.size(); ++i) {
                    if( !is_slope_set ) {
                        if( sequence[i-1] < sequence[i] ) {
                            slope = ASCENDING;
                            is_slope_set = true;
                        } else if( sequence[i-1] > sequence[i] ) {
                            slope = DESCENDING;
                            is_slope_set = true;
                        }
                    }
                    Type diff = (sequence[i-1] > sequence[i]) ? sequence[i-1] - sequence[i] : sequence[i] - sequence[i-1];
                    ans.push_back(diff);
                }
            }
            return std::pair<bool,std::vector<Type>>(slope,ans);
        }

        /**
         * @brief This class represents a Rice-runs encoding of integers of type Type.
         * 
         * @tparam Type 
         */
        template<typename Type> class RiceRuns {
            static_assert(
                    std::is_same_v<Type, std::uint8_t> ||
                    std::is_same_v<Type, std::uint16_t> ||
                    std::is_same_v<Type, std::uint32_t> ||
                    std::is_same_v<Type, std::uint64_t>,
                    "typename must be one of std::uint8_t, std::uint16_t, std::uint32_t, or std::uint64_t");
            
            private:
                sdsl::bit_vector encoded_sequence;
                std::size_t k; // Golomb-Rice parameter m = 2^k.
                static const std::size_t SPAN = 3;

            public:
                RiceRuns(const std::size_t k): 
                    k(k) 
                { }

                void encode(std::vector<Type> sequence) {
                    GRCodec<Type> codec(std::pow(2,this->k),GRCodecType::GOLOMB_RICE);
    
                    // Let c be a flag to restart search with a new found character:
                    bool is_first = true; 
    
                    // Let r be a repetition counter initially in 0:
                    std::size_t r = 0;
    
                    Type previous_n;
    
                    for (std::size_t i = 0; i < sequence.size(); ++i) {
                        if( is_first ) {
                            previous_n = sequence[i];
                            std::cout << "c=1 --- sequence[" << i << "] = " << sequence[i] << std::endl;
                            is_first = false;
                            ++r;
                        } else if( sequence[i] == previous_n ) {
                            ++r;
                        } else {
                            if( r < SPAN ) {
                                for (size_t j = 0; j < r; ++j) {
                                    codec.append(previous_n);
                                }
                            } else {
                                for (size_t j = 0; j < SPAN; ++j) {
                                    codec.append(previous_n);
                                }
                                codec.append(r);
                            }
                            is_first = true; 
                            r = 0ul;
                            --i;
                        }
                    }
                    this->encoded_sequence = codec.get_bit_vector();
                }

                std::vector<Type> decode() {
                    std::vector<Type> decoded,ans;
                    GRCodec<Type> codec(this->encoded_sequence, std::pow(2,this->k),GRCodecType::GOLOMB_RICE);
                    codec.restart();
                    while( codec.has_more() ) {
                        decoded.push_back( codec.next() );
                    }

                    // TODO ...

                    return ans;
                }

                sdsl::bit_vector get_encoded_sequence() const {
                    return this->encoded_sequence;
                }
        };
    }
}