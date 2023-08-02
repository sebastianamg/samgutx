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

#include <cstddef>
#include <cmath>
#include <cstdint>
#include <type_traits>
#include <vector>
#include <array>
#include <functional>
#include <stdexcept>
#include <cassert>
#include <utility>
#include <unordered_map>
#include <sdsl/bit_vectors.hpp>
#include <samg/commons.hpp>
#include <stdexcept>

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
                    return std::log(x) / std::log(2.0); // NOTE Much more efficient than std::log2! 34 nanoseconds avg.
                    // return std::log2(x); // NOTE 180 nanoseconds avg.
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
                    static std::size_t r_bits, prev_m = 0; // Using simple memoization method for log2. 
                    
                    // auto start = std::chrono::high_resolution_clock::now();
                    // std::size_t r_bits = (std::size_t)GRCodec::log2(m);
                    if ( m != prev_m ) {
                        r_bits = (std::size_t)log2(m);
                        prev_m = m;
                    }
                    // auto end = std::chrono::high_resolution_clock::now();
                    // double time_taken = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
                    // printf("*** Rice encoding - LOG2 time %.2f[ns]\n",time_taken);

                    // start = std::chrono::high_resolution_clock::now();
                    sdsl::bit_vector v(r_bits + q + 1);
                    // std::size_t len = r_bits + q + 1;
                    // sdsl::bit_vector v(len, 1);
                    // end = std::chrono::high_resolution_clock::now();
                    // time_taken = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
                    // printf("*** Rice encoding - codeword bitmap declaration time %.2f[ns]\n",time_taken);

                    // start = std::chrono::high_resolution_clock::now();
                    v.set_int(0,r,GRCodec::BITS_PER_BYTE);

                    for (std::size_t i = r_bits + 1; i < (r_bits + q + 1); ++i) {
                        v[i] = 1;
                    } // v[r_bits] = 0 as the sdsl::bit_vector initialization default value is 0. 
                    // v[r_bits] = 0;
                    // v = v & (std::pow(2,len) - 1);
                    // end = std::chrono::high_resolution_clock::now();
                    // time_taken = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
                    // printf("*** Rice encoding time %.2f[ns]\n",time_taken);

                    // auto end = std::chrono::high_resolution_clock::now();
                    // double time_taken = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
                    // printf("*** Rice encoding %.2f[ns]\n",time_taken);

                    // std::cout << " bitmap = " << v << "; data[0] = " << v.data()[0] << std::endl;
                    

                    return v;
                }
                // static sdsl::bit_vector _rice_encode_(const Type n, const Type q, const Type r, const std::size_t m) {
                //     std::size_t r_bits = (std::size_t)GRCodec::log2(m);
                //     sdsl::bit_vector v(r_bits + q + 1);
                //     v.set_int(0,r,GRCodec::BITS_PER_BYTE);

                //     for (std::size_t i = r_bits + 1; i < (r_bits + q + 1); ++i) {
                //         v[i] = 1;
                //     } // v[r_bits] = 0 as the sdsl::bit_vector initialization default value is 0. 
                    
                //     return v;
                // }

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
                    // auto start = std::chrono::high_resolution_clock::now();
                    Type R = v.get_int( 0, sizeof(Type) * BITS_PER_BYTE ),
                         ans = ( m * A ) + R;
                    // auto end = std::chrono::high_resolution_clock::now();
                    // double time_taken = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
                    // printf("*** Rice decoding time %.2f[ns]\n",time_taken);

                    return ans;
                    // return ( m * A ) + R;
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
                 * @brief This function allows decoding an integer encoded as a bitmap v.
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

                /**
                 * @brief This function returns the current iterator index.
                 * 
                 * @return const std::uint64_t 
                 */
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
                typedef std::int64_t rseq_t; // Data type internally used by the relative sequence. It can be changed here to reduce memory footprint in case numbers in a relative sequence are small enough to fit in fewer bits.  
                typedef std::vector<RiceRuns::rseq_t> RelativeSequence;
                typedef std::vector<Type> AbsoluteSequence;

                static const std::size_t    RLE_THRESHOLD = 3,     // Minimum number of repetitions to be compressed.
                                            ESCAPE_RANGE_SPAN = 2; // Range span to reserve integers as special symbols (e.g. negative flag, repetition mark).
                
                static const Type   NEGATIVE_FLAG = 0,
                                    REPETITION_FLAG = 1;

                static const bool IS_NEGATIVE = true;
                
                sdsl::bit_vector encoded_sequence;
                std::size_t k; // Golomb-Rice parameter m = 2^k.

                /**
                 * @brief This function allows generating a gap centered in 0. The result is a value that belongs to either (-inf,-RiceRuns::ESCAPE_RANGE_SPAN+1] or [RiceRuns::ESCAPE_RANGE_SPAN,inf). This transformation of v allows to release values in [-RiceRuns::ESCAPE_RANGE_SPAN,RiceRuns::ESCAPE_RANGE_SPAN-1] to be used as scape symbols during encoding/decoding of, for instance, negative values.
                 * 
                 * @param v 
                 * @return RiceRuns::rseq_t 
                 */
                static RiceRuns::rseq_t transform_rval( RiceRuns::rseq_t v ) {
                    return v >= 0 ? v + RiceRuns::ESCAPE_RANGE_SPAN : v - RiceRuns::ESCAPE_RANGE_SPAN; // Transform relative value. 
                }

                /**
                 * @brief This function allows recovering an original value transformed by `transform_rval`.
                 * 
                 * @param v 
                 * @return RiceRuns::rseq_t 
                 */
                static RiceRuns::rseq_t recover_rval( RiceRuns::rseq_t v ) {
                    return v < 0 ? v + RiceRuns::ESCAPE_RANGE_SPAN : v - RiceRuns::ESCAPE_RANGE_SPAN; // Recover relative value.
                }

                /**
                 * @brief This function allows converting a sequence of unsigned Type integers into a relative sequence of signed integers by computing their sequential differences. The resultant sequence is transformed in the process to prevent the number 0 that is used to represent negative integers when numbers are encoded with variable-length integers.  
                 * 
                 * @param sequence 
                 * @return Sequence 
                 */
                static RelativeSequence _get_transformed_relative_sequence_( AbsoluteSequence sequence ) {
                    RelativeSequence ans;
                    
                    if( sequence.size() > 0 ) {
                        /* NOTE
                            1 3 3 6  5  0 0 5
                            1 2 0 3 -1 -5 0 5 <--- Original relative values.
                            2 3 1 4 -1 -5 1 6 <--- transformed relative values.
                        */
                        ans.push_back( transform_rval( sequence[0] ) );
                        for (std::size_t i = 1; i < sequence.size(); ++i) {
                            ans.push_back( transform_rval( ((RiceRuns::rseq_t)(sequence[i])) - ((RiceRuns::rseq_t)(sequence[i-1])) ) );
                        }
                    }
                    return ans;
                }

                /**
                 * @brief This function allows converting a relative sequence of sequential differences encoded as signed integers into an absolute sequence of unsigned Type integers. The resultant sequence is transformed in the process to revert previous transformation when relativization was applied by `_get_transformed_relative_sequence_` function.
                 * 
                 * @param relative_sequence 
                 * @return AbsoluteSequence static
                 */
                static AbsoluteSequence _get_transformed_absolute_sequence_( RelativeSequence relative_sequence ) {
                    AbsoluteSequence ans;

                    if( relative_sequence.size() > 0 ) {
                        /* NOTE
                            1 3 3 6  5  0 0 5 <--- Original absolute values.
                            1 2 0 3 -1 -5 0 5 <--- Original relative values.
                            2 3 1 4 -1 -5 1 6 <--- Transformed relative values.
                            1 2 0 3 -1 -5 0 5 <--- Recovered relative values.
                            1 3 3 6  5  0 0 5 <--- Recovered absolute values.

                        */
                        ans.push_back( recover_rval( relative_sequence[0] ) );
                        for (std::size_t i = 1; i < relative_sequence.size(); ++i) {
                            ans.push_back(
                                ((RiceRuns::rseq_t)ans.back()) + recover_rval( relative_sequence[i] )
                            );
                        }
                    }
                    return ans;
                }

                /**
                 * @brief This class represents a FSM for encoding. 
                 * 
                 */
                class FSMEncoder {
                    private:

                        bool is_init = false;

                        /**
                         * @brief States of the FSM.
                         * 
                         */
                        enum EState {
                            ES_Q0,      //0
                            ES_Q1,      //1
                            ES_Q2,      //2
                            ES_Q3,      //3
                            ES_Q4,      //4
                            ES_Q5,      //5
                            ES_Q6,      //6
                            ES_PSINK,   //7 Sink for positive integer.
                            ES_NSINK,   //8 Sink for negative integer.
                            ES_ERROR    //9
                        } current_state; // Current state.

                        /**
                         * @brief Cases generated by inputs in the FSM. 
                         * 
                         */
                        enum ECase {
                            EC_PINT,      //0 Positive ingeter.
                            EC_NINT,      //1 Negative ingeter.
                            EC_PIEQPRV, //2 Positive ingeter equals to previous.
                            EC_PINQPRV, //3 Positive ingeter not equals to previous.
                            EC_NIEQPRV, //4 Negative ingeter equals to previous.
                            EC_NINQPRV, //5 Negative ingeter not equals to previous.
                            EC_EOS,     //6 End of sequence.
                            EC_ERROR    //7
                        };

                        /**
                         * @brief Functions to be executed by each state.
                         * 
                         */
                        const std::array<std::function<void( GRCodec<Type>&, RiceRuns::rseq_t&, const RiceRuns::rseq_t, std::size_t& )>,10> sfunction = {
                            []( GRCodec<Type> &codec, RiceRuns::rseq_t &previous_n,const RiceRuns::rseq_t n, std::size_t &r ) { // ES_Q0
                                previous_n = n;
                                r = 1;
                            },
                            []( GRCodec<Type> &codec, RiceRuns::rseq_t &previous_n,const RiceRuns::rseq_t n, std::size_t &r ) { // ES_Q1
                                ++r;
                            },
                            []( GRCodec<Type> &codec, RiceRuns::rseq_t &previous_n,const RiceRuns::rseq_t n, std::size_t &r ) { // ES_Q2
                                _write_integer_( codec, previous_n, r );
                                previous_n = n;
                                r = 1;
                            },
                            []( GRCodec<Type> &codec, RiceRuns::rseq_t &previous_n,const RiceRuns::rseq_t n, std::size_t &r ) { // ES_Q3
                                _write_integer_( codec, previous_n, r );
                                previous_n = n;
                                r = 1;
                            },
                            []( GRCodec<Type> &codec, RiceRuns::rseq_t &previous_n,const RiceRuns::rseq_t n, std::size_t &r ) { // ES_Q4
                                _write_integer_( codec, previous_n, r, RiceRuns::IS_NEGATIVE );
                                previous_n = n;
                                r = 1;
                            },
                            []( GRCodec<Type> &codec, RiceRuns::rseq_t &previous_n,const RiceRuns::rseq_t n, std::size_t &r ) { // ES_Q5
                                ++r;
                            },
                            []( GRCodec<Type> &codec, RiceRuns::rseq_t &previous_n,const RiceRuns::rseq_t n, std::size_t &r ) { // ES_Q6
                                _write_integer_( codec, previous_n, r, RiceRuns::IS_NEGATIVE );
                                previous_n = n;
                                r = 1;
                            },
                            []( GRCodec<Type> &codec, RiceRuns::rseq_t &previous_n,const RiceRuns::rseq_t n, std::size_t &r ) { // ES_PSINK
                                _write_integer_( codec, previous_n, r );
                            },
                            []( GRCodec<Type> &codec, RiceRuns::rseq_t &previous_n,const RiceRuns::rseq_t n, std::size_t &r ) { // ES_NSINK
                                _write_integer_( codec, previous_n, r, RiceRuns::IS_NEGATIVE );
                            },
                            []( GRCodec<Type> &codec, RiceRuns::rseq_t &previous_n,const RiceRuns::rseq_t n, std::size_t &r ) { // ES_ERROR
                                throw std::runtime_error("Encoding error state!");
                            }
                        };

                        /**
                         * @brief States matrix.
                         * 
                         */
                        const std::array<std::array<EState,8>,10> fsm = {
                                             //   n > 0                n < 0                 n > 0 & n == n'       n > 0 & n != n'       n < 0 & n == n'       n < 0 & n != n'       EOS                ERROR
                            std::array<EState,8>({EState::ES_ERROR,    EState::ES_Q3,        EState::ES_Q1,        EState::ES_Q2,        EState::ES_ERROR,     EState::ES_ERROR,     EState::ES_PSINK,  EState::ES_ERROR}), // ES_Q0
                            std::array<EState,8>({EState::ES_ERROR,    EState::ES_Q3,        EState::ES_Q1,        EState::ES_Q2,        EState::ES_ERROR,     EState::ES_ERROR,     EState::ES_PSINK,  EState::ES_ERROR}), // ES_Q1
                            std::array<EState,8>({EState::ES_ERROR,    EState::ES_Q3,        EState::ES_Q1,        EState::ES_Q2,        EState::ES_ERROR,     EState::ES_ERROR,     EState::ES_PSINK,  EState::ES_ERROR}), // ES_Q2
                            std::array<EState,8>({EState::ES_Q6,       EState::ES_ERROR,     EState::ES_ERROR,     EState::ES_ERROR,     EState::ES_Q5,        EState::ES_Q4,        EState::ES_NSINK,  EState::ES_ERROR}), // ES_Q3
                            std::array<EState,8>({EState::ES_Q6,       EState::ES_ERROR,     EState::ES_ERROR,     EState::ES_ERROR,     EState::ES_Q5,        EState::ES_Q4,        EState::ES_NSINK,  EState::ES_ERROR}), // ES_Q4
                            std::array<EState,8>({EState::ES_Q6,       EState::ES_ERROR,     EState::ES_ERROR,     EState::ES_ERROR,     EState::ES_Q5,        EState::ES_Q4,        EState::ES_NSINK,  EState::ES_ERROR}), // ES_Q5
                            std::array<EState,8>({EState::ES_ERROR,    EState::ES_Q3,        EState::ES_Q1,        EState::ES_Q2,        EState::ES_ERROR,     EState::ES_ERROR,     EState::ES_PSINK,  EState::ES_ERROR}), // ES_Q6
                            std::array<EState,8>({EState::ES_ERROR,    EState::ES_ERROR,     EState::ES_ERROR,     EState::ES_ERROR,     EState::ES_ERROR,     EState::ES_ERROR,     EState::ES_ERROR,  EState::ES_ERROR}), // ES_PSINK
                            std::array<EState,8>({EState::ES_ERROR,    EState::ES_ERROR,     EState::ES_ERROR,     EState::ES_ERROR,     EState::ES_ERROR,     EState::ES_ERROR,     EState::ES_ERROR,  EState::ES_ERROR}), // ES_NSINK
                            std::array<EState,8>({EState::ES_ERROR,    EState::ES_ERROR,     EState::ES_ERROR,     EState::ES_ERROR,     EState::ES_ERROR,     EState::ES_ERROR,     EState::ES_ERROR,  EState::ES_ERROR})  // ES_ERROR
                        };

                        /**
                         * @brief This function writes the output to a Golomb-Rice encoder.
                         * 
                         * @param codec 
                         * @param n 
                         * @param r 
                         * @param is_negative 
                         */
                        static void _write_integer_( GRCodec<Type> &codec, RiceRuns::rseq_t n, const std::size_t r, const bool is_negative = false ) {
                            n = ( is_negative ) ? n * -1 : n;
                            if( r < RiceRuns::RLE_THRESHOLD ) {
                                for (std::size_t j = 0; j < r; ++j) {
                                    if( is_negative ) { codec.append(RiceRuns::NEGATIVE_FLAG); }
                                    codec.append(n);
                                }
                            } else {
                                codec.append(RiceRuns::REPETITION_FLAG);
                                if( is_negative ) { codec.append(RiceRuns::NEGATIVE_FLAG); }
                                codec.append(n);
                                codec.append(r);
                            }
                        }

                        /**
                         * @brief This function identifies the case generated by a next FSM input. 
                         * 
                         * @param rs 
                         * @param i 
                         * @param previous_n 
                         * @param n 
                         * @return const ECase 
                         */
                        static const ECase _get_case_( const RelativeSequence rs, std::size_t &i, const RiceRuns::rseq_t previous_n, RiceRuns::rseq_t &n ) {
                            if( i == rs.size() ) {
                                return ECase::EC_EOS;
                            }else {
                                n = rs[i++];
                                if( n > 0 && previous_n < 0 ) {
                                    return ECase::EC_PINT;
                                } else if( n < 0 && previous_n > 0 ) {
                                    return ECase::EC_NINT;
                                } else if( n > 0 && n == previous_n ) {
                                    return ECase::EC_PIEQPRV;
                                } else if( n > 0 && n != previous_n ) {
                                    return ECase::EC_PINQPRV;
                                } else if( n < 0 && n == previous_n ) {
                                    return ECase::EC_NIEQPRV;
                                } else if( n < 0 && n != previous_n ) {
                                    return ECase::EC_NINQPRV;
                                } else {
                                    return ECase::EC_ERROR;
                                }
                            }
                        }

                        /**
                         * @brief This function initializes the FSM.
                         * 
                         * @param rs 
                         * @param i 
                         * @param n 
                         */
                        void _init_( const RelativeSequence rs, std::size_t &i, RiceRuns::rseq_t &n ) {
                            i = 0;
                            n = rs[i++];
                            this->current_state = EState::ES_Q0;
                        }

                    public:

                        /**
                         * @brief This function allows moving the FSM to the next state.
                         * 
                         * @param rs 
                         * @param i 
                         * @param previous_n 
                         * @param n 
                         * @return EState 
                         */
                        EState next( const RelativeSequence rs, std::size_t &i, const RiceRuns::rseq_t previous_n, RiceRuns::rseq_t &n ) {
                            if( this->is_init ) {
                                this->current_state = fsm[this->current_state][ FSMEncoder::_get_case_( rs, i, previous_n, n ) ];
                            } else {
                                this->_init_( rs, i, n );
                                this->is_init = true;
                            }
                            return this->current_state;
                        }

                        /**
                         * @brief This function allows running the current state's associated function. 
                         * 
                         * @param codec 
                         * @param previous_n 
                         * @param n 
                         * @param r 
                         */
                        void run( GRCodec<Type> &codec, RiceRuns::rseq_t &previous_n, const RiceRuns::rseq_t n, std::size_t &r ) {
                            this->sfunction[this->current_state]( codec, previous_n, n, r );
                        }
                        
                        /**
                         * @brief This functions checks whether the FSM is at an end state or not.
                         * 
                         * @return true 
                         * @return false 
                         */
                        bool is_end_state() {
                            return  this->current_state == EState::ES_PSINK ||
                                    this->current_state == EState::ES_NSINK;
                        }

                        /**
                         * @brief This functions checks whether the FSM is at an error state or not.
                         * 
                         * @return true 
                         * @return false 
                         */
                        bool is_error_state() {
                            return this->current_state == EState::ES_ERROR;
                        }
                        
                };

                /**
                 * @brief This class represents a FSM for decoding. 
                 * 
                 */
                class FSMDecoder {
                    private:
                        bool is_init = false;

                        /**
                         * @brief States of the FSM.
                         * 
                         */
                        enum DState {
                            DS_Q0,      //0
                            DS_Q1,      //1
                            DS_Q2,      //2
                            DS_Q3,      //3
                            DS_Q4,      //4
                            DS_Q5,      //5
                            DS_Q6,      //6
                            DS_Q7,      //7
                            DS_Q8,      //8
                            DS_Q9,      //9
                            DS_SINK,    //10 Sink after a run was written.
                            DS_ERROR    //11
                        } current_state; // Current state.

                        /**
                         * @brief Cases generated by inputs in the FSM. 
                         * 
                         */
                        enum DCase {
                            DC_INT,     //0 Ingeter.
                            DC_NEGFLAG, //1 Negative flag.
                            DC_REPFLAG, //2 Repetition flag.
                            DC_EOS,     //3 End of sequence.
                            DC_ERROR    //4
                        };

                        /**
                         * @brief Functions to be executed by each state.
                         * 
                         */
                        const std::array<std::function<void( RelativeSequence&, Type&, const Type )>,12> sfunction = {
                            []( RelativeSequence &rs, Type &previous_n,const Type n ) { // DS_Q0
                                // Empty
                            },
                            []( RelativeSequence &rs, Type &previous_n,const Type n ) { // DS_Q1
                                _write_integer_( rs, n, 1 );
                            },
                            []( RelativeSequence &rs, Type &previous_n,const Type n ) { // DS_Q2
                                // Empty
                            },
                            []( RelativeSequence &rs, Type &previous_n,const Type n ) { // DS_Q3
                                _write_integer_( rs, n, 1, RiceRuns::IS_NEGATIVE );
                            },
                            []( RelativeSequence &rs, Type &previous_n,const Type n ) { // DS_04
                                // Empty
                            },
                            []( RelativeSequence &rs, Type &previous_n,const Type n ) { // DS_Q5
                                previous_n = n;
                            },
                            []( RelativeSequence &rs, Type &previous_n,const Type n ) { // DS_Q6
                                _write_integer_( rs, previous_n, n );
                            },
                            []( RelativeSequence &rs, Type &previous_n,const Type n ) { // DS_Q7
                                // Empty
                            },
                            []( RelativeSequence &rs, Type &previous_n,const Type n ) { // DS_Q8
                                previous_n = n;
                            },
                            []( RelativeSequence &rs, Type &previous_n,const Type n ) { // DS_Q9
                                _write_integer_( rs, previous_n, n, RiceRuns::IS_NEGATIVE );
                            },
                            []( RelativeSequence &rs, Type &previous_n,const Type n ) { // DS_SINK
                                // Empty
                            },
                            []( RelativeSequence &rs, Type &previous_n,const Type n ) { // DS_ERROR
                                throw std::runtime_error("Decoding error state!");
                            }
                        };

                        /**
                         * @brief States matrix.
                         * 
                         */
                        const std::array<std::array<DState,5>,12> fsm = {
                                             //   n!=NEG & n!=REP       n == NEG              n == REP               EOS                    ERROR
                            std::array<DState,5>({DState::DS_Q1,       DState::DS_Q2,        DState::DS_Q4,        DState::DS_SINK,      DState::DS_ERROR}), // DS_Q0
                            std::array<DState,5>({DState::DS_Q1,       DState::DS_Q2,        DState::DS_Q4,        DState::DS_SINK,      DState::DS_ERROR}), // DS_Q1
                            std::array<DState,5>({DState::DS_Q3,       DState::DS_ERROR,     DState::DS_ERROR,     DState::DS_ERROR,     DState::DS_ERROR}), // DS_Q2
                            std::array<DState,5>({DState::DS_Q1,       DState::DS_Q2,        DState::DS_Q4,        DState::DS_SINK,      DState::DS_ERROR}), // DS_Q3
                            std::array<DState,5>({DState::DS_Q5,       DState::DS_Q7,        DState::DS_ERROR,     DState::DS_ERROR,     DState::DS_ERROR}), // DS_Q4
                            std::array<DState,5>({DState::DS_Q6,       DState::DS_ERROR,     DState::DS_ERROR,     DState::DS_ERROR,     DState::DS_ERROR}), // DS_Q5
                            std::array<DState,5>({DState::DS_Q1,       DState::DS_Q2,        DState::DS_Q4,        DState::DS_SINK,      DState::DS_ERROR}), // DS_Q6
                            std::array<DState,5>({DState::DS_Q8,       DState::DS_ERROR,     DState::DS_ERROR,     DState::DS_ERROR,     DState::DS_ERROR}), // DS_Q7
                            std::array<DState,5>({DState::DS_Q9,       DState::DS_ERROR,     DState::DS_ERROR,     DState::DS_ERROR,     DState::DS_ERROR}), // DS_Q8
                            std::array<DState,5>({DState::DS_Q1,       DState::DS_Q2,        DState::DS_Q4,        DState::DS_SINK,      DState::DS_ERROR}), // DS_Q9
                            std::array<DState,5>({DState::DS_ERROR,    DState::DS_ERROR,     DState::DS_ERROR,     DState::DS_ERROR,     DState::DS_ERROR}), // DS_SINK
                            std::array<DState,5>({DState::DS_ERROR,    DState::DS_ERROR,     DState::DS_ERROR,     DState::DS_ERROR,     DState::DS_ERROR})  // DS_ERROR
                        };

                        /**
                         * @brief This function writes the output to a relative sequence vector.
                         * 
                         * @param rs 
                         * @param n 
                         * @param r 
                         * @param is_negative 
                         */
                        static void _write_integer_( RelativeSequence &rs, const Type n, const std::size_t r, const bool is_negative = false ) {
                            RiceRuns::rseq_t x = is_negative ? ((RiceRuns::rseq_t)n) * -1 : n;
                            for (std::size_t j = 0; j < r; ++j) {
                                rs.push_back(x);
                            }
                        }

                        /**
                         * @brief This function identifies the case generated by a next FSM input. 
                         * 
                         * @param codec 
                         * @param n 
                         * @return const DCase 
                         */
                        static const DCase _get_case_( GRCodec<Type> &codec, Type &n ) {
                            if( !codec.has_more() ) {
                                return DCase::DC_EOS;
                            }else {
                                n = codec.next();
                                if( n != RiceRuns::NEGATIVE_FLAG && n != RiceRuns::REPETITION_FLAG ) {
                                    return DCase::DC_INT;
                                } else if( n == RiceRuns::NEGATIVE_FLAG ) {
                                    return DCase::DC_NEGFLAG;
                                } else if( n == RiceRuns::REPETITION_FLAG ) {
                                    return DCase::DC_REPFLAG;
                                } else {
                                    return DCase::DC_ERROR;
                                }
                            }
                        }

                        /**
                         * @brief This function initializes the FSM.
                         * 
                         * @param codec 
                         * @param n 
                         */
                        void _init_( GRCodec<Type> &codec, Type &n ) {
                            this->current_state = DState::DS_Q0;
                        }

                    public:

                        /**
                         * @brief This function allows moving the FSM to the next state.
                         * 
                         * @param codec 
                         * @param previous_n 
                         * @param n 
                         * @return DState 
                         */
                        DState next( GRCodec<Type> &codec, Type &previous_n, Type &n ) {
                            if( !this->is_init ) {
                                this->_init_( codec, n );
                                this->is_init = true;
                            }
                            this->current_state = fsm[this->current_state][ FSMDecoder::_get_case_( codec, n ) ];
                            return this->current_state;
                        }

                        /**
                         * @brief This function allows running the current state's associated function. 
                         * 
                         * @param rs 
                         * @param previous_n 
                         * @param n 
                         */
                        void run( RelativeSequence &rs, Type &previous_n, const Type n ) {
                            this->sfunction[this->current_state]( rs, previous_n, n );
                        }
                        
                        /**
                         * @brief This functions checks whether the FSM is at an end state or not.
                         * 
                         * @return true 
                         * @return false 
                         */
                        bool is_end_state() {
                            return  this->current_state == DState::DS_SINK;
                        }

                        /**
                         * @brief This functions checks whether the FSM is at an error state or not.
                         * 
                         * @return true 
                         * @return false 
                         */
                        bool is_error_state() {
                            return this->current_state == DState::DS_ERROR;
                        }
                        
                };

            public:
                RiceRuns(const std::size_t k): 
                    k(k) 
                { }

                RiceRuns(sdsl::bit_vector encoded_sequence, const std::size_t k): 
                    k(k) ,
                    encoded_sequence(encoded_sequence)
                { }

                /**
                 * @brief This function encodes a sequence of Type integers using Rice-runs. 
                 * 
                 * @param sequence 
                 * @param k 
                 * @return sdsl::bit_vector 
                 */
                static sdsl::bit_vector encode( const AbsoluteSequence sequence, const std::size_t k ) {
                    GRCodec<Type> codec( 
                        std::pow( 2, k ), // By using a power of 2, `codec` acts as Rice encoder.
                        GRCodecType::GOLOMB_RICE 
                    );

                    FSMEncoder fsm;

                    RelativeSequence relative_sequence = RiceRuns::_get_transformed_relative_sequence_( sequence );

                    std::size_t r, // Let r be a repetition counter of n.
                                i; // Let i be the index to traverse the relative sequence.

                    RiceRuns::rseq_t    previous_n, // Previous sequence value.
                                        n;          // Next sequence value.

                    do {
                        fsm.next( relative_sequence, i, previous_n, n);
                        if( fsm.is_error_state() ) { break; }
                        fsm.run( codec, previous_n, n, r );
                    } while( !fsm.is_end_state() );

                    return codec.get_bit_vector();
                }

                /**
                 * @brief This function decodes an encoded sequence of Type integers using Rice-runs. 
                 * 
                 * @param encoded_sequence 
                 * @param k 
                 * @return AbsoluteSequence 
                 */
                static AbsoluteSequence decode( sdsl::bit_vector encoded_sequence, const std::size_t k  ) {
                    GRCodec<Type> codec(
                        encoded_sequence, 
                        std::pow( 2, k ),GRCodecType::GOLOMB_RICE // By using a power of 2, `codec` acts as Rice encoder.
                    );

                    FSMDecoder fsm;

                    Type    previous_n, 
                            n;

                    RelativeSequence relative_sequence;

                    do { 
                        fsm.next( codec, previous_n, n);
                        if( fsm.is_error_state() ) { break; }
                        fsm.run( relative_sequence, previous_n, n );
                    }while( !fsm.is_end_state() );

                    return RiceRuns::_get_transformed_absolute_sequence_( relative_sequence );
                }

                /**
                 * @brief This function returns the Golomb-Rice encoded bitmap.
                 * 
                 * @return sdsl::bit_vector 
                 */
                sdsl::bit_vector get_encoded_sequence() const {
                    return this->encoded_sequence;
                }

                const Type next() {

                }

        };
    }
}