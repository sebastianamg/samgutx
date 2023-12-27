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
#define DEBUG_OFFLINERICERUNSWRITER_ADD

#pragma once

#include <codecs/gr-code-debug.hpp>
#include <cstddef>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <type_traits>
#include <vector>
#include <array>
#include <queue>
#include <functional>
#include <stdexcept>
#include <cassert>
#include <utility>
#include <unordered_map>
#include <sdsl/bit_vectors.hpp>
#include <samg/commons.hpp>
#include <samg/matutx.hpp>
#include <stdexcept>
#include <typeinfo>


#define BITS_PER_BYTE 8

#define FLOORLOG_2(x,v)                     \
{                                           \
    unsigned _B_x = (x);           \
    (v) = 0;                                \
    for (; _B_x>1 ; _B_x>>=1, (v)++);       \
}   

namespace samg {
    namespace grcodec {
        namespace adapter {
            enum QueueAdapterType {
                Q_QUEUEADAPTER,
                // QUINT_QUEUEADAPTER,
                // QINT_QUEUEADAPTER,
                ITERATOR_QUEUEADAPTER
            };
            template<typename Type> class QueueAdapter {
                protected:
                    virtual void _swap_( QueueAdapter<Type>& other ) = 0;
                public:
                    // explicit QueueAdapter();
                    // QueueAdapter(const QueueAdapter&) = delete;
                    // QueueAdapter& operator=(const QueueAdapter&) = delete;
                    // ~QueueAdapter() = default;
                    
                    virtual Type front() = 0;
                    virtual Type back() = 0;
                    virtual void push( Type v ) = 0;
                    virtual bool empty() = 0;
                    virtual std::size_t size() = 0;
                    virtual void pop() = 0;
                    static void swap( QueueAdapter<Type>& a, QueueAdapter<Type>& b ) {
                        a._swap_( b );
                    }
            };
            template<typename Type> class QQueueAdapter : public QueueAdapter<Type> {
                private:
                    // std::queue<Type,std::deque<Type>> queue;
                    std::vector<Type> queue;

                    void _swap_( QueueAdapter<Type>& other ) override {
                        QQueueAdapter<Type>* x = dynamic_cast<QQueueAdapter<Type>*>( &other );
                        std::swap( this->queue, x->queue );
                    }

                public:
                    QQueueAdapter() :
                        // queue ( std::queue<Type>() ) {}
                        queue ( std::vector<Type>() ) {}

                    Type front() override {
                        Type tmp = this->queue.front();
                      //  std::cout << "QQueueAdapter / front> Type = " << typeid(Type).name() << " front (" << typeid(tmp).name() << ") = " << tmp << std::endl;
                        return tmp;
                    }

                    Type back() override {
                        return this->queue.back();
                    }

                    void push( Type v ) override {
                      //  std::cout << "QQueueAdapter / push> Type = " << typeid(Type).name() << " push (" << v << ") --- " << typeid(v).name() << std::endl;
                        // this->queue.push( v );
                        this->queue.push_back(v);
                    }

                    bool empty() override {
                        return this->queue.empty();
                    }

                    std::size_t size() override {
                        return this->queue.size();
                    }

                    void pop() override {
                        // this->queue.pop();
                        this->queue.erase(this->queue.begin());
                    }
            };

            template<typename Type> class IteratorQueueAdapter : public QueueAdapter<Type> {
                private:
                    typename std::vector<Type>::iterator    begin,
                                                            end;
                    typename std::vector<Type>::reverse_iterator rbegin;

                    void _swap_( QueueAdapter<Type>& other ) override {
                        throw std::runtime_error("IteratorQueueAdapter> Non-implemented method!");
                    }

                public:
                    IteratorQueueAdapter(
                        typename std::vector<Type>::iterator begin,
                        typename std::vector<Type>::iterator end,
                        typename std::vector<Type>::reverse_iterator rbegin
                    ) :
                        begin( begin ),
                        end( end ),
                        rbegin( rbegin ){}

                    Type front() override {
                        return *(this->begin);
                    }

                    Type back() override {
                        return *(this->rbegin);
                    }

                    void push( Type v ) override {
                        throw std::runtime_error("IteratorQueueAdapter> Non-implemented method!");
                    }

                    bool empty() override {
                        return this->begin == this->end;
                    }

                    std::size_t size() override {
                        return this->end - this->begin;
                    }

                    void pop() override {
                        this->begin++;
                    }
            };

            /**
             * @brief Creates an instance of `QueueAdapter`.
             * 
             * @tparam Type 
             * @param type 
             * @param begin 
             * @param end 
             * @param rbegin 
             * @return std::unique_ptr<QueueAdapter<Type>> 
             */
            template <typename Type> std::shared_ptr<QueueAdapter<Type>> get_instance(QueueAdapterType type,
                                                                typename std::vector<Type>::iterator begin = {},
                                                                typename std::vector<Type>::iterator end = {},
                                                                typename std::vector<Type>::reverse_iterator rbegin = {}) {
                switch (type) {
                    case QueueAdapterType::Q_QUEUEADAPTER:
                        return std::make_shared<QQueueAdapter<Type>>();
                    case QueueAdapterType::ITERATOR_QUEUEADAPTER:
                        return std::make_shared<IteratorQueueAdapter<Type>>(begin, end, rbegin);
                    default:
                        throw std::runtime_error("adapter/get_instance> Non-implemented QueueAdapter!");
                }
            }
        }

        namespace toolkits {/***************************************************************/

            /**
             * @brief Represents a toolkit for Golomb-Rice encoding.
             * 
             * @tparam Word 
             */
            template<typename Word> struct GolombRiceCommon {

                static const std::size_t WORD_bits = sizeof(Word) * BITS_PER_BYTE;
                
                static const std::size_t get_word_bits() {
                    return GolombRiceCommon::WORD_bits;
                }

                /**
                 * @brief This function frees pointer p.
                 * 
                 * @param p 
                 */
                static void rfree( Word *p ) {
                // static void rfree( void *p ) {
                    if (p) {
                        free(p);
                    }
                }

                /**
                 * @brief This function reserves space for n Word-type integers. 
                 * 
                 * @param n 
                 * @return Word* 
                 */
                static Word* rmalloc( std::size_t n ) {
                    Word *p;
                    if (n == 0) {
                        return NULL;
                    }
                    p = ( Word* ) malloc( n * sizeof(Word) );
                    if (p == NULL) {
                        throw std::runtime_error("Could not allocate "+ std::to_string(n) +" bytes\n");
                    }
                    return p;
                }

                /**
                 * @brief This function reallocate space for n Word-type integers in pointer p. 
                 * 
                 * @param p 
                 * @param n 
                 * @return Word* 
                 */
                static Word* rrealloc( Word *p, std::size_t n ) {
                    // std::cout << "RCodec/BinarySequence/rrealloc --- (1)" << std::endl;
                    if( p == NULL ) {
                        // std::cout << "RCodec/BinarySequence/rrealloc --- (2)" << std::endl;
                        return GolombRiceCommon<Word>::rmalloc( n );
                    }
                    if( n == 0 ) {
                        // std::cout << "RCodec/BinarySequence/rrealloc --- (3)" << std::endl;
                        GolombRiceCommon<Word>::rfree( p );
                        return NULL;
                    }
                    // std::cout << "RCodec/BinarySequence/rrealloc --- (4)" << std::endl;
                    p = ( Word* ) realloc( p, n * sizeof(Word) );
                    if(p == NULL ) {
                        // std::cout << "RCodec/BinarySequence/rrealloc --- (5)" << std::endl;
                        throw std::runtime_error("Could not re-allocate "+ std::to_string(n) +" bytes\n");
                    }
                    // std::cout << "RCodec/BinarySequence/rrealloc --- (6)" << std::endl;
                    return p;
                }

                /**
                 * @brief writes e[p..p+len-1] = s, len <= W
                 * 
                 * @param e 
                 * @param p 
                 * @param len 
                 * @param s 
                 */
                static inline void _bitwrite_( Word *e,  std::size_t p,  std::size_t len, const  Word s) {
                    e += p / GolombRiceCommon<Word>::WORD_bits;
                    p %= GolombRiceCommon<Word>::WORD_bits;
                    if (len == GolombRiceCommon<Word>::WORD_bits) {
                        *e |= (*e & ((1 << p) - 1)) | (s << p);
                        if (!p)
                            return;
                        e++;
                        *e = (*e & ~((1 << p) - 1)) | (s >> (GolombRiceCommon<Word>::WORD_bits - p));
                    } else {
                        if (p + len <= GolombRiceCommon<Word>::WORD_bits) {
                            *e = (*e & ~(((1 << len) - 1) << p)) | (s << p);
                            return;
                        }
                        *e = (*e & ((1 << p) - 1)) | (s << p);
                        e++;
                        len -= GolombRiceCommon<Word>::WORD_bits - p;
                        *e = (*e & ~((1 << len) - 1)) | (s >> (GolombRiceCommon<Word>::WORD_bits - p));
                    }
                }

                /**
                 * @brief returns e[p..p+len-1], assuming len <= W
                 * 
                 * @param e 
                 * @param p 
                 * @param len 
                 * @return Word 
                 */
                static Word _bitread_(Word *e, std::size_t p, const std::size_t len) {
                    Word answ;
                    e += p / GolombRiceCommon<Word>::WORD_bits;
                    p %= GolombRiceCommon<Word>::WORD_bits;
                    answ = *e >> p;
                    if (len == GolombRiceCommon<Word>::WORD_bits) {
                        if (p)
                            answ |= (*(e + 1)) << (GolombRiceCommon<Word>::WORD_bits - p);
                    } else {
                        if (p + len > GolombRiceCommon<Word>::WORD_bits)
                            answ |= (*(e + 1)) << (GolombRiceCommon<Word>::WORD_bits - p);
                        answ &= (1 << len) - 1;
                    }
                    return answ;
                }

                static Word _bitget_(Word* e, const std::size_t p) {
                    return (((e)[(p) / GolombRiceCommon<Word>::WORD_bits] >> ((p) % GolombRiceCommon<Word>::WORD_bits)) & 1);
                }

                /**
                 * TODO
                 * @brief 
                 * 
                 * @param buf 
                 * @param pos 
                 * @param nbits 
                 * @return Word 
                 */
                static inline Word _rice_size_(Word val, std::size_t nbits) {
                    // val-=OFFSET_LOWEST_VALUE;   //0 is never encoded. So encoding 1 as 0, 2 as 1, ... v as v-1
                    Word w,size;
                    size=nbits;
                    for (w = (val>>nbits); w > 0; w--) {
                        size++;
                    }
                    size++;
                    return size;
                }

                /**
                 * @brief This function returns the Rice-coding bit-length of the value. 
                 * 
                 * @param val 
                 * @param nbits 
                 * @return std::size_t 
                 */
                static inline std::size_t _value_size_( Word val, std::size_t nbits ) {
                    return std::floor( val / std::pow(2,nbits) ) + 1 + nbits; // unary encoding length + 1 (the 0 ending unary encoding) + remainder length.
                }

                /**
                 * @brief This function allows encoding n using the Rice algorithm. 
                 * 
                 * @param buf 
                 * @param pos 
                 * @param nbits 
                 * @return Word 
                 */
                static inline Word _rice_encode_(Word *buf, std::size_t pos, Word val, std::size_t nbits) {
                    // val-=OFFSET_LOWEST_VALUE;  //0 is never encoded. So encoding 1 as 0, 2 as 1, ... v as v-1
                    std::size_t w;
                    GolombRiceCommon<Word>::_bitwrite_ (buf, pos, nbits, val); 
                    pos+=nbits;
                    
                    for (w = (val>>nbits); w > 0; w--) {
                        GolombRiceCommon<Word>::_bitwrite_ (buf, pos, 1, 1); pos++;
                    }
                    GolombRiceCommon<Word>::_bitwrite_ (buf, pos, 1, 0); pos++;
                    return pos;
                }
                
                /**
                 * @brief This function allows decoding n using the Rice algorithm. 
                 * 
                 * @param val 
                 * @param nbits 
                 * @return Word 
                 */
                static Word _rice_decode_(Word *buf, std::size_t *pos, std::size_t nbits) {
                    // Retrieve reminder:
                    Word v = _bitread_(buf, *pos, nbits);
                    *pos+=nbits;

                    // Retrieve quotient:
                    //printf("\n\t [decoding] v del bitread vale = %d",v);
                    while ( GolombRiceCommon<Word>::_bitget_(buf,*pos) ) {
                        v += (1<<nbits);
                        (*pos)++;
                    }
                    (*pos)++;

                    return(v /*+ OFFSET_LOWEST_VALUE*/);   //1 was encoded as 0 ... v as v-1 ... !!
                }

                /**
                 * @brief Given a list of (n) integers (buff) [typically gap values from an inverted list], computes the optimal b parameter, that is, the number of bits that will be coded binary-wise from a given encoded value.
                 * @note $$ val = \lfloor (\log_2( \sum_{i=0}^{n-1} (diff[i])) / n )) \rfloor $$
                 * 
                 * @param sequence 
                 * @return std::size_t 
                 */
                static std::size_t compute_GR_parameter_for_list( std::vector<Word> sequence ) {
                    Word total =0;
                     std::size_t i;
                    for ( i = 0; i < sequence.size(); i++ ) total += sequence[i];
                    total /= sequence.size();
                    std::size_t val;
                    FLOORLOG_2(((uint)total),val);
                    
                    return val;
                }

                /**
                 * @brief Given a list of (n) integers (buff) [typically gap values from an inverted list], computes the optimal b parameter, that is, the number of bits that will be coded binary-wise from a given encoded value.
                 * @note $$ val = \lfloor (\log_2( \sum_{i=0}^{n-1} (diff[i])) / n )) \rfloor $$
                 * 
                 * @param sequence 
                 * @return std::size_t 
                 */
                static std::size_t compute_GR_parameter_for_list( adapter::QueueAdapter<Word>& sequence ) {
                    Word total =0;
                     std::size_t i;
                    while( !(sequence.empty()) ) {
                        total += sequence.front(); sequence.pop();
                    }
                    total /= sequence.size();
                    std::size_t val;
                    FLOORLOG_2(((uint)total),val);
                    
                    return val;
                }
            };

            template<typename Word> struct RunLengthCommon {
                using rseq_t = std::int64_t; //typedef unsigned long long int rseq_t; // Data type internally used by the relative sequence. It can be changed here to reduce memory footprint in case numbers in a relative sequence are small enough to fit in fewer bits.  
                
                template<typename TypeInt = rseq_t> using RelativeSequence = std::shared_ptr<samg::grcodec::adapter::QueueAdapter<TypeInt>>;
                
                template<typename TypeUInt = Word> using AbsoluteSequence = std::shared_ptr<samg::grcodec::adapter::QueueAdapter<TypeUInt>>;
                

                static const std::size_t    RLE_THRESHOLD = 3,     // Minimum number of repetitions to be compressed.
                                            ESCAPE_RANGE_SPAN = 2; // Range span to reserve integers as special symbols (e.g. negative flag, repetition mark).
                
                static const Word   NEGATIVE_FLAG = 0,
                                    REPETITION_FLAG = 1;

                static const bool   IS_NEGATIVE = true;
                
                /**
                 * @brief This function allows generating a gap centered in 0. The result is a value that belongs to either (-inf,-RunLengthCommon<Word>::ESCAPE_RANGE_SPAN+1] or [RunLengthCommon<Word>::ESCAPE_RANGE_SPAN,inf). This transformation of v allows to release values in [-RunLengthCommon<Word>::ESCAPE_RANGE_SPAN,RunLengthCommon<Word>::ESCAPE_RANGE_SPAN-1] to be used as scape symbols during encoding/decoding of, for instance, negative values.
                 * 
                 * @param v 
                 * @return rseq_t 
                 */
                static rseq_t transform_rval( rseq_t v ) {
                    return v >= 0 ? v + RunLengthCommon<Word>::ESCAPE_RANGE_SPAN : v - RunLengthCommon<Word>::ESCAPE_RANGE_SPAN; // Transform relative value. 
                }

                /**
                 * @brief This function allows recovering an original value transformed by `transform_rval`.
                 * 
                 * @param v 
                 * @return rseq_t 
                 */
                static rseq_t recover_rval( rseq_t v ) {
                    return v < 0 ? v + RunLengthCommon<Word>::ESCAPE_RANGE_SPAN : v - RunLengthCommon<Word>::ESCAPE_RANGE_SPAN; // Recover relative value.
                }

                /**
                 * @brief This function allows converting a sequence of unsigned Word integers into a relative sequence of signed integers by computing their sequential differences. The resultant sequence is transformed in the process to prevent the number 0 that is used to represent negative integers when numbers are encoded with variable-length integers.  
                 * 
                 * @param sequence 
                 * @return RelativeSequence 
                 */
                static RelativeSequence<> get_transformed_relative_sequence( AbsoluteSequence<> sequence ) {
                    RelativeSequence<> ans = adapter::get_instance<rseq_t>( adapter::QueueAdapterType::Q_QUEUEADAPTER );
                    // std::cout << sequence.size() << std::endl;
                    // if( sequence.size() > 0 ) {
                    if( !sequence.empty() ) {
                        /* NOTE
                            1 3 3 6  5  0 0 5
                            1 2 0 3 -1 -5 0 5 <--- Original relative values.
                            2 3 1 4 -1 -5 1 6 <--- transformed relative values.
                        */
                        Word    current,
                                previous = sequence.front();
                            
                        sequence.pop();

                        ans->push( transform_rval( previous ));
                        // ans.push_back( transform_rval( sequence[0] ) );
                        while( !sequence->empty() ) {
                            current = sequence.front(); sequence.pop();
                            // std::cout << current << std::endl;
                            // ans.push( transform_rval( ((rseq_t)(current)) - ((rseq_t)(previous)) ) );
                            ans->push( RunLengthCommon<Word>::get_transformed_relative_sequence( current, previous ));
                            previous = current;
                        }
                        // for (std::size_t i = 1; i < sequence.size(); ++i) {
                        //     ans.push_back( transform_rval( ((rseq_t)(sequence[i])) - ((rseq_t)(sequence[i-1])) ) );
                        // }
                    }
                    return ans;
                }
                /**
                 * @brief This function converts `current` into a relative value given `previous`.
                 * 
                 * @param previous 
                 * @param current 
                 * @return rseq_t 
                 */
                static rseq_t get_transformed_relative_sequence( rseq_t relativa_previous, rseq_t relativa_current ) {
                    rseq_t ans = transform_rval( relativa_current - relativa_previous );
                //  std::cout << "\t\tRiceRuns / get_transformed_relative_sequence> relativa_previous = " << relativa_previous << "; relativa_current = " << relativa_current << "; ans = " << ans << std::endl;
                    return ans;
                }

                /**
                 * @brief This function allows converting a relative sequence of sequential differences encoded as signed integers into an absolute sequence of unsigned Word integers. The resultant sequence is transformed in the process to revert previous transformation when relativization was applied by `get_transformed_relative_sequence` function.
                 * 
                 * @param relative_sequence 
                 * @return AbsoluteSequence
                 * 
                 * @warning Side effects on input!
                 */
                static AbsoluteSequence<> get_transformed_absolute_sequence( RelativeSequence<>& sequence ) {
                    AbsoluteSequence<> ans = adapter::get_instance<Word>( adapter::QueueAdapterType::Q_QUEUEADAPTER );

                    // if( relative_sequence.size() > 0 ) {
                    if( !sequence->empty() ) {
                        /* NOTE
                            1 3 3 6  5  0 0 5 <--- Original absolute values.
                            1 2 0 3 -1 -5 0 5 <--- Original relative values.
                            2 3 1 4 -1 -5 1 6 <--- Transformed relative values.
                            1 2 0 3 -1 -5 0 5 <--- Recovered relative values.
                            1 3 3 6  5  0 0 5 <--- Recovered absolute values.

                        */
                        ans->push( recover_rval( sequence->front() ));
                        sequence->pop();
                        // ans.push_back( recover_rval( relative_sequence[0] ) );
                        // for (std::size_t i = 1; i < sequence.size(); ++i) {
                        while( !sequence->empty() ){
                            ans->push( ((rseq_t)ans->back()) + recover_rval( sequence->front() ) );
                            sequence->pop();
                        }
                    }
                    return ans; //.release();
                }

            };
        
            // template<typename Word> struct Batch {
            //     /**
            //      * @brief Encodes in a batch the content of a QueueAdapter.
            //      * 
            //      * @param codec 
            //      * @param queue 
            //      */
            //     static void batch_encode( samg::grcodec::base::writer::CodecFileWriter<Word>& codec, samg::grcodec::adapter::QueueAdapter<Word>& queue ) {
            //         while( !queue.empty() ) {
            //             codec.add( queue.front() );
            //             queue.pop();
            //         }
            //     }
            // };
        }

        namespace base {

            /**
             * @brief Interface for classes that handle metadata.
             * 
             */
            class MetadataKeeper { 
                public:
                    /**
                     * @brief Returnst the metadata.
                     * 
                     * @return const std::vector<std::size_t> 
                     */
                    virtual const std::vector<std::size_t> get_metadata() const = 0;

                    /**
                     * @brief Adds a new metadata at the end of the metadata list.
                     * 
                     * @param v 
                     */
                    virtual void add_metadata( std::size_t v ) = 0;

                    /**
                     * @brief Adds a new metadata at the beginning of the metadata list.
                     * 
                     * @param v 
                     */
                    virtual void push_metadata( std::size_t v ) = 0;
            };

            /**
             * @brief Partial implementation of MetadataKeeper.
             * 
             */
            class MetadataSaver : public MetadataKeeper {
                protected:
                    std::vector<std::size_t> metadata;
                public:
                    void add_metadata( std::size_t v ) override {
                        this->metadata.push_back( v );
                        // std::cout << "Codec/add_metadata> v = " << v << "; |metadata| = " << this->metadata.size() << "; @ " << (&(this->metadata)) << std::endl;
                    }

                    void push_metadata( std::size_t v ) override {
                        this->metadata.insert( this->metadata.begin(), v );
                        // this->push_metadata(v);
                        // std::cout << "Codec/push_metadata> v = " << v << "; |metadata| = " << this->metadata.size() << "; @ " << (&(this->metadata)) << std::endl;
                    }
            };

            template<typename Word> class Codec {
                static_assert(
                    std::is_same_v<Word, std::uint8_t> ||
                    std::is_same_v<Word, std::uint16_t> ||
                    std::is_same_v<Word, std::uint32_t> ||
                    std::is_same_v<Word, std::uint64_t>,
                    "typename must be one of std::uint8_t, std::uint16_t, std::uint32_t, or std::uint64_t"); 

            };

            class FileHandler {
                private:
                    const std::string file_name;
                public:
                    FileHandler( const std::string file_name ) :
                        file_name (file_name) {}

                    const std::string get_file_name() const {
                        return this->file_name;
                    }
            };

            namespace writer {
                template<typename Word> class CodecWriter : public Codec<Word> {
                    public:
                        /**
                         * @brief Adds a codeword to a bitmap.
                         * 
                         * @param n is the codeword.
                         */
                        virtual const bool add(const Word n) = 0;
                };

                template<typename Word> class CodecFileWriter : public CodecWriter<Word>, public FileHandler  {
                    public:
                        CodecFileWriter( const std::string file_name ) :
                            FileHandler(file_name) {}
                        /**
                         * @brief Closes serialization.
                         * 
                         */
                        virtual void close() = 0;
                };
            }

            namespace reader {
                template<typename Word> class CodecReader : public Codec<Word> {
                    public:
                        /**
                         * @brief Decodes and returns the next encoded codeword as it iterates on a bitmap. 
                         * 
                         * @return const Word 
                         */
                        virtual const Word next() = 0;

                        /**
                         * @brief Verifies whether a bitmap has or doesn't have more codewords to iterate on.
                         * 
                         * @return true 
                         * @return false 
                         */
                        virtual const bool has_more() const = 0;

                        /**
                         * @brief Restart the reading over again.
                         * 
                         */
                        virtual void restart() = 0;
                };

                template<typename Word> class CodecFileReader : public CodecReader<Word>, public FileHandler {
                    public:
                        CodecFileReader( const std::string file_name ) :
                            FileHandler(file_name) {}
                        /**
                         * @brief Closes serialization source.
                         * 
                         */
                        virtual void close() = 0;
                };

            }
        }
        /***************************************************************/
        /***************************************************************/
        /***************************************************************/

        namespace toolkits {
            template<typename Word> struct Batch {
                /**
                 * @brief Encodes in a batch the content of a QueueAdapter.
                 * 
                 * @param codec 
                 * @param queue 
                 */
                static void batch_encode( samg::grcodec::base::writer::CodecFileWriter<Word>& codec, samg::grcodec::adapter::QueueAdapter<Word>& queue ) {
                    while( !queue.empty() ) {
                        codec.add( queue.front() );
                        queue.pop();
                    }
                }
            };
        }

        namespace rice {
            
            namespace writer {
                /**
                 * @brief Writes a binary sequence in offline mode (word by word).
                 * 
                 * @tparam Word used to encode bits.
                 */
                template<typename Word> class OfflineRCodecWriter : public samg::grcodec::base::writer::CodecFileWriter<Word>, public samg::grcodec::base::MetadataSaver {
                    private:
                        static const std::size_t WORD_GROWING_SPAN = 1ULL; // Space in number of Word-type that the bitmap must grow. 
                        Word *sequence; // It is the bitmap.
                        const std::size_t   k; // Rice-code order.
                        std::size_t bit_index, // Index of current bit within the bitmap `sequence`.
                                    value_counter, // Number of encoded values.
                                    word_counter, // Number of used words.
                                    bit_counter, // Number of encoded bits.
                                    words_max_capacity; // Max capacity of `sequence` in words.
                        std::unique_ptr<samg::serialization::OfflineWordWriter<Word>> serializer;

                        /**
                         * @brief Prepends metadata to output file.
                         * 
                         */
                        void _save_metadata_() {
                            std::vector<std::size_t> met = this->get_metadata();
                            for (std::size_t v : this->get_metadata()) {
                                this->serializer->template add_value<std::size_t>( v );
                            }
                            this->serializer->template add_value<std::size_t>( this->metadata.size() );
                        } 

                    public:
                        /**
                         * @brief Construct a new Offline Binary Sequence object
                         * 
                         * @param file_name 
                         * @param k 
                         */
                        OfflineRCodecWriter( const std::string file_name, const std::size_t k ):
                            samg::grcodec::base::writer::CodecFileWriter<Word>::CodecFileWriter( file_name ),
                            k ( k ),
                            bit_index ( 0ULL ),
                            value_counter ( 0ULL ),
                            bit_counter ( 0ULL ),
                            word_counter ( 0ULL ), // ( OfflineRCodecWriter<Word>::WORD_GROWING_SPAN ),
                            words_max_capacity ( 0ULL ),// ( OfflineRCodecWriter<Word>::WORD_GROWING_SPAN ),
                            sequence ( samg::grcodec::toolkits::GolombRiceCommon<Word>::rmalloc( 0ULL ) ) //( samg::grcodec::toolkits::GolombRiceCommon<Word>::rmalloc( OfflineRCodecWriter<Word>::WORD_GROWING_SPAN ) ) 
                        {
                            // std::cout << "OfflineRCodecWriter/init> (1)" << std::endl;
                            this->serializer = std::make_unique<samg::serialization::OfflineWordWriter<Word>>( file_name );
                            // this->serializer->template add_value<std::size_t>( k );
                            // std::cout << "OfflineRCodecWriter/init> (2)" << std::endl;
                        }

                        /**
                         * @brief Returns the k constant
                         * 
                         * @return const std::size_t 
                         */
                        const std::size_t get_k() const {
                            return this->k;
                        }

                        const std::size_t get_value_counter() const {
                            return this->value_counter;
                        }

                        const std::size_t get_bit_counter() const {
                            return this->bit_counter;
                        }

                        const bool add( const Word n ) override {
                            const std::size_t n_bits = samg::grcodec::toolkits::GolombRiceCommon<Word>::_value_size_( n, this->k );
                            // std::cout << "OfflineRCodecWriter/add> [BEGIN] k = " << this->k << "; n = " << n << "; |n| = " << n_bits << "[b]; bit_index = " << this->bit_index << "; bit_counter = " << this->bit_counter << "; value_counter = " << this->value_counter << "; word_max_capacity = " << this->words_max_capacity << "; word_counter = " << this->word_counter << std::endl;
                            if( std::ceil( (double)(this->bit_index + n_bits ) / (double) samg::grcodec::toolkits::GolombRiceCommon<Word>::get_word_bits() ) > this->words_max_capacity ) {
                            // if( std::floor( (double)(this->bit_index + n_bits ) / (double) samg::grcodec::toolkits::GolombRiceCommon<Word>::get_word_bits() ) >= this->words_max_capacity ) {
                                // std::cout << "RCodec/BinarySequence/add --- growing bitmap from " << this->max_length << "[W]";
                                const std::size_t new_word_capacity = std::ceil( (double)n_bits / (double)samg::grcodec::toolkits::GolombRiceCommon<Word>::get_word_bits() ); //+ OfflineRCodecWriter::WORD_GROWING_SPAN );
                                // std::cout << "OfflineRCodecWriter/add>\t\tGrowing in "<< new_word_capacity <<"[W]..." << std::endl;
                                this->word_counter += new_word_capacity;
                                this->words_max_capacity += new_word_capacity;
                                // std::cout << " to " << this->max_length << " [W]" << std::endl;
                                this->sequence = samg::grcodec::toolkits::GolombRiceCommon<Word>::rrealloc( this->sequence, this->words_max_capacity );
                            }
                            this->bit_index = samg::grcodec::toolkits::GolombRiceCommon<Word>::_rice_encode_(this->sequence, this->bit_index, n, this->k);
                            this->bit_counter += n_bits;
                            ++(this->value_counter);

                            // Checking if sequence is ready to be written on secondary memory:
                            const double words_in_sequence = (double) this->bit_index  / (double) samg::grcodec::toolkits::GolombRiceCommon<Word>::get_word_bits();
                            const std::size_t words = std::floor( words_in_sequence );
                            if( words > 0 ) { // Flush to secondary memory:
                                // std::cout << "OfflineRCodecWriter/add>\t\tFlushing..." << std::endl;
                                const std::size_t delta_words = std::ceil( words_in_sequence ) - words;
                                this->serializer->template add_values<Word>( this->sequence, words );
                                // this->word_counter += ( this->words_max_capacity - delta_words );
                                this->words_max_capacity = delta_words > 0 ? delta_words : OfflineRCodecWriter::WORD_GROWING_SPAN;
                                Word* tmp_sequence = samg::grcodec::toolkits::GolombRiceCommon<Word>::rmalloc( this->words_max_capacity );
                                for (size_t i = 0; i < delta_words; i++) {
                                    tmp_sequence[i] = this->sequence[ words + i ];
                                }
                                samg::grcodec::toolkits::GolombRiceCommon<Word>::rfree( this->sequence );
                                this->sequence = tmp_sequence;
                                this->bit_index -= ( words * samg::grcodec::toolkits::GolombRiceCommon<Word>::get_word_bits() );
                            }
                            return true; // To fulfill inheritance requirements.
                            // std::cout << "OfflineRCodecWriter/add>   [END] k = " << this->k << "; n = " << n << "; |n| = " << n_bits << "[b]; bit_index = " << this->bit_index << "; bit_counter = " << this->bit_counter << "; value_counter = " << this->value_counter << "; word_max_capacity = " << this->words_max_capacity << "; word_counter = " << this->word_counter << std::endl;
                        }

                        const std::vector<std::size_t> get_metadata() const override {
                            return this->metadata;
                        }

                        void close( ) override {
                            // std::cout << "OfflineRCodecWriter/close> [BEGIN] k = " << this->k << "[b]; bit_index = " << this->bit_index << "; bit_counter = " << this->bit_counter << "; value_counter = " << this->value_counter << "; word_max_capacity = " << this->words_max_capacity << "; word_counter = " << this->word_counter << std::endl;
                            // Write pending words in the sequence:
                            const std::size_t pending_words = std::ceil( (double)this->bit_index  / (double) samg::grcodec::toolkits::GolombRiceCommon<Word>::get_word_bits() );
                            // std::cout << "OfflineRCodecWriter/close> \t\tpending_words = " << pending_words << std::endl;
                            if( pending_words > 0 ) { // Flush to secondary memory:
                                // std::cout << "OfflineRCodecWriter/close> \t\tFlusing..." << std::endl;
                                this->serializer->template add_values<Word>( this->sequence, pending_words );
                                // std::cout << "OfflineRCodecWriter/close> \t\tPrepending ---> k = " << this->k << "; bit_counter = " << this->bit_counter << std::endl;
                            }
                            // this->serializer->close();
                            // Appending metadata:
                            // std::cout << "OfflineRCodecWriter/close> |metadata| = " << this->get_metadata().size() << std::endl;
                            this->push_metadata( this->bit_counter );
                            this->push_metadata( this->k );
                            // std::cout << "OfflineRCodecWriter/close> |metadata| = " << this->get_metadata().size() << std::endl;
                            // this->metadata.insert(this->metadata.begin(), this->bit_counter );
                            // this->metadata.insert(this->metadata.begin(), this->k );
                            this->_save_metadata_();
                            // std::cout << "OfflineRCodecWriter/close> [END]" << std::endl;
                            this->serializer->close();
                        }

                };
            }

            namespace reader {
                /**
                 * @brief Represents an offline binary sequence reader.
                 * 
                 * @tparam Word used to encode bits.
                 */
                template<typename Word> class OfflineRCodecReader : public samg::grcodec::base::reader::CodecFileReader<Word>, public samg::grcodec::base::MetadataSaver {
                    private:
                        const Word MAX;
                        Word R_MASK;
                        std::unique_ptr<samg::serialization::OfflineWordReader<Word>> serializer;
                        std::vector<Word> buffer;
                        std::size_t k,
                                    position,
                                    bit_limit,
                                    bit_counter,
                                    offset;
                        bool is_open;

                        const std::size_t _get_buffer_length_() {
                            return this->buffer.size() * samg::grcodec::toolkits::GolombRiceCommon<Word>::get_word_bits();
                        }

                        const bool _fecth_() {
                            static std::size_t counter = 0;
                            if( this->has_more() ) {
                                this->buffer.push_back( this->serializer->template next<Word>() );
                                counter++;
                                // std::cout << "OfflineRCodecReader/_fetch_> counter = " << counter << std::endl;
                                return true;
                            } else {
                                // std::cout << "OfflineRCodecReader/_fetch_> NO MORE ENTRIES!!! --- counter = " << counter << std::endl;
                                return false;
                            }
                        }

                        void _update_() {
                            // Relief buffer:
                            while( this->position >= samg::grcodec::toolkits::GolombRiceCommon<Word>::get_word_bits() ) {
                                this->buffer.erase(this->buffer.begin());
                                this->position -= samg::grcodec::toolkits::GolombRiceCommon<Word>::get_word_bits();
                            }
                            // Fill up buffer:
                            while( this->position >= this->_get_buffer_length_() && this->_fecth_());
                        }

                        void _retrieve_metadata_() {
                            // k, bit_counter, metadata_size
                            if( this->is_open ) {
                                // std::cout << "OfflineRCodecReader/_retrieve_metadata_> (0) serializer size = " << this->serializer->size() << std::endl;
                                std::size_t nbytes = this->serializer->size();
                                this->serializer->seek( nbytes - sizeof(std::size_t) , std::ios_base::beg );


                                std::size_t metadata_size = this->serializer->template next<std::size_t>();
                                // std::cout << "OfflineRCodecReader/_retrieve_metadata_> (1) tellg = " << this->serializer->tell() << "; metadata_size = " << metadata_size << std::endl;

                                this->serializer->seek( nbytes - ((metadata_size + 1) * sizeof(std::size_t)), std::ios_base::beg );

                                // std::cout << "OfflineRCodecReader/_retrieve_metadata_> (2) tellg = " << this->serializer->tell() << std::endl;

                                for (std::size_t i = 0; i < metadata_size; i++) {
                                    std::size_t v = this->serializer->template next<std::size_t>();
                                    this->add_metadata( v );
                                    // std::cout << "OfflineRCodecReader/_retrieve_metadata_> (2) \t\ttellg = " << this->serializer->tell() << "; v = " << v << std::endl;
                                }
                                
                                this->serializer->seek( 0, std::ios::beg );
                                // std::cout << "OfflineRCodecReader/_retrieve_metadata_> (3) tellg = " << this->serializer->tell() << std::endl;
                            }
                        }

                    public:
                        /**
                         * @brief Construct a new Binary Sequence object
                         * 
                         * @param file_name 
                         * @param offset in bits
                         * @param limit in bits
                         */
                        OfflineRCodecReader( const std::string file_name, const std::size_t offset = 0ULL, const std::size_t limit = 0ULL ) :
                            samg::grcodec::base::reader::CodecFileReader<Word>::CodecFileReader( file_name ),
                            MAX ( ~( (Word) 0 ) ),
                            is_open ( false ),
                            offset ( offset ) {
                            
                            // std::cout << "OfflineRCodecReader/init> (1)" << std::endl;

                            this->restart();

                            // std::cout << "OfflineRCodecReader/init> (2)" << std::endl;
                            
                            // Loading metadata:
                            this->_retrieve_metadata_();

                            // std::cout << "OfflineRCodecReader/init> (3)" << std::endl;
                            // std::vector<std::size_t>metadata = this->get_metadata();
                            // this->k = this->serializer->template next<std::size_t>();
                            // this->bit_limit = this->serializer->template next<std::size_t>();
                            this->k = this->metadata[0];
                            this->bit_limit = ( limit == 0 ) ? this->metadata[1] : limit;
                            this->metadata.erase( this->metadata.begin() ); // Erasing k from metadata.
                            this->metadata.erase( this->metadata.begin() ); // Erasing bit_limit from metadata.

                            // std::cout << "OfflineRCodecReader/init> (4) k = " << this->k << "; bit_limit = " << this->bit_limit << std::endl;
                            // std::cout << "OfflineRCodecReader/init> k = " << this->k << "; bit_limit = " << this->bit_limit << std::endl;

                            // Seting environment:
                            this->R_MASK = MAX << this->k;

                            // // Set the starting byte within the serialization based on the input offset:
                            // this->serializer->seek( std::ceil((std::double_t)offset/(std::double_t)BITS_PER_BYTE), std::ios::beg );
                            // std::cout << "OfflineRCodecReader/init> (5)" << std::endl;
                        }

                        /**
                         * @brief Returns the k constant
                         * 
                         * @return const std::size_t 
                         */
                        const std::size_t get_k() const {
                            return this->k;
                        }

                        const Word next( ) override { 
                            // std::cout << "OfflineRCodecReader/next> [BEGIN] k = " << this->k << "; bit_limit = " << this->bit_limit << "; position = " << this->position << "; bit_counter = " << this->bit_counter << std::endl;
                            // NOTE: Based on `samg::grcodec::toolkits::GolombRiceCommon<Word>::_rice_decode_`.
                            while( ( this->_get_buffer_length_() - this->position ) < this->k ) {
                                this->_fecth_();
                            }

                            // Retrieve reminder:
                            Word v = samg::grcodec::toolkits::GolombRiceCommon<Word>::_bitread_( buffer.data(), this->position, this->k );
                            this->position += this->k;
                            this->bit_counter += this->k;

                            // Retrieve quotient:
                            this->_update_();
                            while ( samg::grcodec::toolkits::GolombRiceCommon<Word>::_bitget_( buffer.data() , this->position ) ) {
                                v += ( 1 << this->k );
                                ++(this->position);
                                ++(this->bit_counter);
                                this->_update_();
                            }
                            ++(this->position);
                            ++(this->bit_counter);
                            // std::cout << "OfflineRCodecReader/next> \t\t[END] k = " << this->k << "; bit_limit = " << this->bit_limit << "; position = " << this->position << "; bit_counter = " << this->bit_counter << std::endl;
                            return v;
                        }

                        const bool has_more( ) const override {
                            return this->bit_counter < this->bit_limit; //this->serializer->has_more();
                        }

                        void restart() override {
                            this->close();
                            this->serializer = std::make_unique<samg::serialization::OfflineWordReader<Word>>( this->get_file_name() );
                            this->position = 0ULL;
                            this->bit_counter = 0ULL;
                            // Set the starting byte within the serialization based on the input offset:
                            this->serializer->seek( std::ceil((std::double_t)this->offset/(std::double_t)BITS_PER_BYTE), std::ios::beg );

                            this->is_open = true;

                        }

                        const std::vector<std::size_t> get_metadata() const override {
                            // Take out k and bit_counter entries in the metadata list:
                            // std::vector<std::size_t> tmp = std::vector<std::size_t>( this->metadata.begin() + 2, this->metadata.end() );
                            // return tmp;
                            return this->metadata;
                        }

                        void close( ) override {
                            if( this->is_open ) {
                                this->serializer->close();
                                this->serializer.reset();
                                this->is_open = false;
                            }
                        }
                };
            }
        }
        
        namespace golomb {
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
                    // static const std::size_t BITS_PER_BYTE = sizeof(std::uint8_t)*8;
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
                        
                        if ( m != prev_m ) {
                            r_bits = (std::size_t)log2(m);
                            prev_m = m;
                        }
                        
                        sdsl::bit_vector v(r_bits + q + 1);
                        
                        v.set_int(0,r,BITS_PER_BYTE);

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
                            v.set_int(0,r,BITS_PER_BYTE);
                        } else {
                            v.set_int(0,r+c,BITS_PER_BYTE);
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
                    enum GRCodecType {
                        GOLOMB_RICE,
                        EXPONENTIAL_GOLOMB
                    } type;

                    GRCodec():
                        m(8),
                        type(GRCodecType::GOLOMB_RICE)
                    {}

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
        }

        namespace runlength {
            namespace writer {
                /**
                 * @brief This class represents an unsigned integer Rice-runs encoder. This codec uses OfflineRCodecWriter class. 
                 * 
                 * @tparam Word 
                 * @tparam std::size_t 
                 */
                template<typename Word> class OfflineRiceRunsWriter : public samg::grcodec::base::writer::CodecFileWriter<Word>, samg::grcodec::base::MetadataKeeper {
                    private:
                        /**
                         * @brief This class represents a FSM for encoding. 
                         * 
                         */
                        class FSMEncoder {
                            private:

                                bool is_initilized = false;

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
                                const std::array<std::function<void( std::shared_ptr<samg::grcodec::base::writer::CodecWriter<Word>>, typename samg::grcodec::toolkits::RunLengthCommon<Word>::rseq_t&, const typename samg::grcodec::toolkits::RunLengthCommon<Word>::rseq_t, std::size_t&)>,10> sfunction = {
                                    []( std::shared_ptr<samg::grcodec::base::writer::CodecWriter<Word>>codec, typename samg::grcodec::toolkits::RunLengthCommon<Word>::rseq_t &previous_n,const typename samg::grcodec::toolkits::RunLengthCommon<Word>::rseq_t n, std::size_t &r ) { // ES_Q0
                                        previous_n = n;
                                        r = 1;
                                    },
                                    []( std::shared_ptr<samg::grcodec::base::writer::CodecWriter<Word>>codec, typename samg::grcodec::toolkits::RunLengthCommon<Word>::rseq_t &previous_n,const typename samg::grcodec::toolkits::RunLengthCommon<Word>::rseq_t n, std::size_t &r ) { // ES_Q1
                                        ++r;
                                    },
                                    []( std::shared_ptr<samg::grcodec::base::writer::CodecWriter<Word>>codec, typename samg::grcodec::toolkits::RunLengthCommon<Word>::rseq_t &previous_n,const typename samg::grcodec::toolkits::RunLengthCommon<Word>::rseq_t n, std::size_t &r ) { // ES_Q2
                                        _write_integer_( codec, previous_n, r );
                                        previous_n = n;
                                        r = 1;
                                    },
                                    []( std::shared_ptr<samg::grcodec::base::writer::CodecWriter<Word>>codec, typename samg::grcodec::toolkits::RunLengthCommon<Word>::rseq_t &previous_n,const typename samg::grcodec::toolkits::RunLengthCommon<Word>::rseq_t n, std::size_t &r ) { // ES_Q3
                                        _write_integer_( codec, previous_n, r );
                                        previous_n = n;
                                        r = 1;
                                    },
                                    []( std::shared_ptr<samg::grcodec::base::writer::CodecWriter<Word>>codec, typename samg::grcodec::toolkits::RunLengthCommon<Word>::rseq_t &previous_n,const typename samg::grcodec::toolkits::RunLengthCommon<Word>::rseq_t n, std::size_t &r ) { // ES_Q4
                                        _write_integer_( codec, previous_n, r, samg::grcodec::toolkits::RunLengthCommon<Word>::IS_NEGATIVE );
                                        previous_n = n;
                                        r = 1;
                                    },
                                    []( std::shared_ptr<samg::grcodec::base::writer::CodecWriter<Word>>codec, typename samg::grcodec::toolkits::RunLengthCommon<Word>::rseq_t &previous_n,const typename samg::grcodec::toolkits::RunLengthCommon<Word>::rseq_t n, std::size_t &r ) { // ES_Q5
                                        ++r;
                                    },
                                    []( std::shared_ptr<samg::grcodec::base::writer::CodecWriter<Word>>codec, typename samg::grcodec::toolkits::RunLengthCommon<Word>::rseq_t &previous_n,const typename samg::grcodec::toolkits::RunLengthCommon<Word>::rseq_t n, std::size_t &r ) { // ES_Q6
                                        _write_integer_( codec, previous_n, r, samg::grcodec::toolkits::RunLengthCommon<Word>::IS_NEGATIVE );
                                        previous_n = n;
                                        r = 1;
                                    },
                                    []( std::shared_ptr<samg::grcodec::base::writer::CodecWriter<Word>>codec, typename samg::grcodec::toolkits::RunLengthCommon<Word>::rseq_t &previous_n,const typename samg::grcodec::toolkits::RunLengthCommon<Word>::rseq_t n, std::size_t &r ) { // ES_PSINK
                                        _write_integer_( codec, previous_n, r );
                                    },
                                    []( std::shared_ptr<samg::grcodec::base::writer::CodecWriter<Word>>codec, typename samg::grcodec::toolkits::RunLengthCommon<Word>::rseq_t &previous_n,const typename samg::grcodec::toolkits::RunLengthCommon<Word>::rseq_t n, std::size_t &r ) { // ES_NSINK
                                        _write_integer_( codec, previous_n, r, samg::grcodec::toolkits::RunLengthCommon<Word>::IS_NEGATIVE );
                                    },
                                    []( std::shared_ptr<samg::grcodec::base::writer::CodecWriter<Word>>codec, typename samg::grcodec::toolkits::RunLengthCommon<Word>::rseq_t &previous_n,const typename samg::grcodec::toolkits::RunLengthCommon<Word>::rseq_t n, std::size_t &r ) { // ES_ERROR
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
                                static void _write_integer_( std::shared_ptr<samg::grcodec::base::writer::CodecWriter<Word>>codec, typename samg::grcodec::toolkits::RunLengthCommon<Word>::rseq_t n, const std::size_t r, const bool is_negative = false ) {
                                    // std::cout << "\t\t\tOfflineRiceRunsWriter/FSMEncoder/_write_integer_> n = " << n << "; r = " << r << "; is_negative = " << is_negative << std::endl;
                                    n = ( is_negative ) ? n * -1 : n;
                                    if( r < samg::grcodec::toolkits::RunLengthCommon<Word>::RLE_THRESHOLD ) {
                                        // std::cout << "\t\t\tOfflineRiceRunsWriter/FSMEncoder/_write_integer_> (1)" << std::endl;
                                        for (std::size_t j = 0; j < r; ++j) {
                                            if( is_negative ) { codec->add(samg::grcodec::toolkits::RunLengthCommon<Word>::NEGATIVE_FLAG); }
                                            codec->add(n);
                                            // std::cout << "\t\t\tOfflineRiceRunsWriter/FSMEncoder/_write_integer_> (1.1)" << std::endl;
                                        }
                                        // std::cout << "\t\t\tOfflineRiceRunsWriter/FSMEncoder/_write_integer_> (1.2)" << std::endl;
                                        // std::cout << " Write ---> r<" << r << "> x (" << (is_negative ? " NEG<00>":"") << " n<" << n << "> )" << std::endl;
                                    } else {
                                        // std::cout << "\t\t\tOfflineRiceRunsWriter/FSMEncoder/_write_integer_> (2)" << std::endl;
                                        codec->add(samg::grcodec::toolkits::RunLengthCommon<Word>::REPETITION_FLAG);
                                        // std::cout << "\t\t\tOfflineRiceRunsWriter/FSMEncoder/_write_integer_> (2.1)" << std::endl;
                                        if( is_negative ) { codec->add(samg::grcodec::toolkits::RunLengthCommon<Word>::NEGATIVE_FLAG); }
                                        // std::cout << "\t\t\tOfflineRiceRunsWriter/FSMEncoder/_write_integer_> (2.2)" << std::endl;
                                        codec->add(n);
                                        // std::cout << "\t\t\tOfflineRiceRunsWriter/FSMEncoder/_write_integer_> (2.3)" << std::endl;
                                        codec->add(r);
                                        // std::cout << "\t\t\tOfflineRiceRunsWriter/FSMEncoder/_write_integer_> (2.4)" << std::endl;

                                        // std::cout << " Write ---> REP<01>" << (is_negative ? " NEG<00>":"") << " n<" << n << "> r<" << r << ">" << std::endl;
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
                                // static const ECase _get_case_( const RelativeSequence rs, std::size_t &i, const typename samg::grcodec::toolkits::RunLengthCommon<Word>::rseq_t previous_n, typename samg::grcodec::toolkits::RunLengthCommon<Word>::rseq_t &n ) {
                                static const ECase _get_case_( typename samg::grcodec::toolkits::RunLengthCommon<Word>::RelativeSequence<> rs, const typename samg::grcodec::toolkits::RunLengthCommon<Word>::rseq_t previous_n, typename samg::grcodec::toolkits::RunLengthCommon<Word>::rseq_t &n ) {
                                    // if( i == rs.size() ) {
                                    if( rs->empty() ) {
                                        return ECase::EC_EOS;
                                    }else {
                                        // n = rs[i++];
                                        n = rs->front(); rs->pop();
                                    //  std::cout << "\t\t\tFSMEncoder/_get_case_> previous_n = " << previous_n << "; n = " << n << std::endl;
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
                                void _init_( typename samg::grcodec::toolkits::RunLengthCommon<Word>::RelativeSequence<> rs, typename samg::grcodec::toolkits::RunLengthCommon<Word>::rseq_t &n ) {
                                    // i = 0;
                                    // n = rs[i++];
                                    n = rs->front(); rs->pop();
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
                                // EState next( const RelativeSequence rs, std::size_t &i, const typename samg::grcodec::toolkits::RunLengthCommon<Word>::rseq_t previous_n, typename samg::grcodec::toolkits::RunLengthCommon<Word>::rseq_t &n ) {
                                EState next( typename samg::grcodec::toolkits::RunLengthCommon<Word>::RelativeSequence<> rs, const typename samg::grcodec::toolkits::RunLengthCommon<Word>::rseq_t previous_n, typename samg::grcodec::toolkits::RunLengthCommon<Word>::rseq_t &n ) {
                                    // std::cout << "FSMEncoder/next> (1)" << std::endl;
                                    if( this->is_initilized ) {
                                    //  std::cout << "\t\t\tFSMEncoder/next> (1) previous_n = " << previous_n << "; n = " << n << std::endl;
                                        // this->current_state = fsm[this->current_state][ FSMEncoder::_get_case_( rs, i, previous_n, n ) ];
                                        this->current_state = fsm[this->current_state][ FSMEncoder::_get_case_( rs, previous_n, n ) ];
                                        // std::cout << "FSMEncoder/next> (1.1.2)" << std::endl;
                                    } else {
                                    //  std::cout << "\t\t\tFSMEncoder/next> (2 - init) previous_n = " << previous_n << "; n = " << n << std::endl;
                                        // this->_init_( rs, i, n );
                                        this->_init_( rs, n );
                                        // std::cout << "FSMEncoder/next> (1.2.2)" << std::endl;
                                        this->is_initilized = true;
                                        // std::cout << "FSMEncoder/next> (1.2.3)" << std::endl;
                                    }
                                    // std::cout << "\t\t\tOfflineRiceRunsWriter/FSMEncoder/next> current_state = " << this->current_state << "; previous_n = " << previous_n << "; n = " << n << std::endl;
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
                                void run( std::shared_ptr<samg::grcodec::base::writer::CodecWriter<Word>> codec, typename samg::grcodec::toolkits::RunLengthCommon<Word>::rseq_t &previous_n, const typename samg::grcodec::toolkits::RunLengthCommon<Word>::rseq_t n, std::size_t &r ) {
                                    // std::cout << "\t\t\tOfflineRiceRunsWriter/FSMEncoder/run> current_state = " << this->current_state << "; previous_n = " << previous_n << "; n = " << n << "; r = " << r << std::endl;
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

                                /**
                                 * @brief This function restarts the FSM.
                                 * 
                                 */
                                void restart() {
                                    this->is_initilized = false;
                                }
                                
                        };

                        // Attributes for relative-sequence traversal:
                        std::shared_ptr<samg::grcodec::rice::writer::OfflineRCodecWriter<Word>> codec;
                        FSMEncoder                      encoding_fsm;
                        Word                            encoding_previous_n;
                        std::size_t                     encoding_r; // Repetition of current encoding value.
                        typename samg::grcodec::toolkits::RunLengthCommon<Word>::rseq_t encoding_previous_relative_n;
                        bool                            encoding_is_first;
                        typename samg::grcodec::toolkits::RunLengthCommon<Word>::RelativeSequence<> encoding_buffer;
                    
                    protected:
                        std::shared_ptr<samg::grcodec::rice::writer::OfflineRCodecWriter<Word>> get_codec() const {
                            return this->codec;
                        }

                        void restart( ) {
                            // std::cout << "OfflineRiceRunsWriter/restart> (1)" << std::endl;
                            if( this->encoding_buffer ) {
                                if( !this->encode() ){
                                    throw std::runtime_error("Ending encoding error!");
                                }
                                // std::cout << "OfflineRiceRunsWriter/restart> (2)" << std::endl;
                                this->encoding_buffer.reset();
                            }
                            // std::cout << "OfflineRiceRunsWriter/restart> (3)" << std::endl;
                            this->encoding_buffer = adapter::get_instance<typename samg::grcodec::toolkits::RunLengthCommon<Word>::rseq_t>( adapter::QueueAdapterType::Q_QUEUEADAPTER );
                            // std::cout << "OfflineRiceRunsWriter/restart> (4)" << std::endl;
                            this->encoding_fsm.restart();
                            this->encoding_is_first = true;
                            this->encoding_previous_n = 0;
                            this->encoding_r = 0;
                            // this->encoding_relative_n = 0;
                            this->encoding_previous_relative_n = 0;
                            // std::cout << "OfflineRiceRunsWriter/restart> (5)" << std::endl;
                        }

                        bool encode( ) {
                            // Encode:
                            typename samg::grcodec::toolkits::RunLengthCommon<Word>::rseq_t relative_v;
                            // std::cout << "\tRiceRuns/encode> (1)" << std::endl;
                            this->encoding_fsm.next( this->encoding_buffer, this->encoding_previous_relative_n, relative_v );
                            // std::cout << "\tRiceRuns/encode> (2)" << std::endl;
                            // std::cout << "\t\tOfflineRiceRunsWriter/_encode_ --- pre> encoding_previous_relative_n = " << this->encoding_previous_relative_n<< "; relative_v = " << relative_v << "; encoding_r = " << this->encoding_r << "; |encoding_buffer| = " << this->encoding_buffer->size() << std::endl;
                            if( this->encoding_fsm.is_error_state() ) { std::runtime_error("Encoding error state!"); }
                            // std::cout << "\tRiceRuns/encode> (3)" << std::endl;
                            this->encoding_fsm.run( this->codec, this->encoding_previous_relative_n, relative_v, this->encoding_r );
                            // std::cout << "\t\tOfflineRiceRunsWriter/_encode_ --- post> encoding_previous_relative_n = " << this->encoding_previous_relative_n<< "; relative_v = " << relative_v << "; encoding_r = " << this->encoding_r << "; |encoding_buffer| = " << this->encoding_buffer->size() << std::endl;
                            return this->encoding_fsm.is_end_state();
                        }

                    public:
                        // OfflineRiceRunsWriter( const std::string file_name, const std::size_t k ) { 
                        OfflineRiceRunsWriter( std::shared_ptr<samg::grcodec::rice::writer::OfflineRCodecWriter<Word>> codec ) :
                            samg::grcodec::base::writer::CodecFileWriter<Word>::CodecFileWriter( codec->get_file_name() ) { 
                            // std::cout << "OfflineRiceRunsWriter/init> (1)" << std::endl;
                            // this->codec = std::make_shared<samg::grcodec::rice::writer::OfflineRCodecWriter<Word>>( file_name, k );
                            this->codec = codec;

                            // std::cout << "OfflineRiceRunsWriter/init> (2)" << std::endl;
                            // this->encoding_buffer = adapter::get_instance<typename samg::grcodec::toolkits::RunLengthCommon<Word>::rseq_t>( adapter::QueueAdapterType::Q_QUEUEADAPTER );
                            this->restart();

                            // std::cout << "OfflineRiceRunsWriter/init> (3)" << std::endl;
                        }

                        
                        const bool add( Word v ) override {
                            // std::cout << "---------------------------------------------------------------\nOfflineRiceRunsWriter/add(v = " << v << ")" << std::endl;
                            // Relativize:
                            typename samg::grcodec::toolkits::RunLengthCommon<Word>::rseq_t relative_v;
                            if( this->encoding_is_first ) {
                                relative_v = samg::grcodec::toolkits::RunLengthCommon<Word>::transform_rval( v );
                                this->encoding_is_first = false;
                            } else {
                                // this->encoding_buffer.push( this->encoding_previous_relative_n );
                                relative_v = samg::grcodec::toolkits::RunLengthCommon<Word>::get_transformed_relative_sequence( this->encoding_previous_n, v );
                            }
                            // std::cout << "\tOfflineRiceRunsWriter/add> pushing relative_v..." << std::endl;
                            PRINT_OFFLINERICERUNSWRITER_ADD("\tOfflineRiceRunsWriter/add> pushing relative_v...");
                            this->encoding_buffer->push( relative_v );
                            // std::cout << "\tOfflineRiceRunsWriter/add> v = " << v << "; relative_v (" << typeid(relative_v).name() << ") = " << relative_v << "; |encoding_buffer| = " << this->encoding_buffer->size() << "; encoding_previous_n = " << this->encoding_previous_n << std::endl;
                            PRINT_OFFLINERICERUNSWRITER_ADD("\tOfflineRiceRunsWriter/add> v = %llu; relative_v = %llu; |encoding_buffer| = %llu; encoding_previous_n = %llu", v, relative_v, this->encoding_buffer->size(),this->encoding_previous_n);
                            this->encoding_previous_n = v;
                            
                            return this->encode();
                        }

                        void close( ) override {
                            // this->encoding_buffer.pop();
                            if( !this->encode() ){
                                throw std::runtime_error("Ending encoding error!");
                            }
                            this->codec->close();
                            this->codec.reset();
                            this->encoding_buffer.reset();
                        }

                        const std::vector<std::size_t> get_metadata() const {
                            return this->codec->get_metadata();
                        }

                        void add_metadata( std::size_t v ) {
                            this->codec->add_metadata( v );
                        }

                        void push_metadata( std::size_t v ) {
                            this->codec->push_metadata( v );
                        }

                };
            }
            namespace reader {
                /**
                 * @brief This class represents an unsigned integer Rice-runs decoder. This codec uses OfflineRCodecReader class. 
                 * 
                 * @tparam Word 
                 */
                template<typename Word> class OfflineRiceRunsReader : public samg::grcodec::base::reader::CodecFileReader<Word>, samg::grcodec::base::MetadataKeeper {
                    private:
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
                                const std::array<std::function<void( typename samg::grcodec::toolkits::RunLengthCommon<Word>::RelativeSequence<>, Word&, const Word )>,12> sfunction = {
                                    []( typename samg::grcodec::toolkits::RunLengthCommon<Word>::RelativeSequence<>rs, Word &previous_n,const Word n ) { // DS_Q0
                                        // Empty
                                    },
                                    []( typename samg::grcodec::toolkits::RunLengthCommon<Word>::RelativeSequence<>rs, Word &previous_n,const Word n ) { // DS_Q1
                                        _write_integer_( rs, n, 1 );
                                    },
                                    []( typename samg::grcodec::toolkits::RunLengthCommon<Word>::RelativeSequence<>rs, Word &previous_n,const Word n ) { // DS_Q2
                                        // Empty
                                    },
                                    []( typename samg::grcodec::toolkits::RunLengthCommon<Word>::RelativeSequence<>rs, Word &previous_n,const Word n ) { // DS_Q3
                                        _write_integer_( rs, n, 1, samg::grcodec::toolkits::RunLengthCommon<Word>::IS_NEGATIVE );
                                    },
                                    []( typename samg::grcodec::toolkits::RunLengthCommon<Word>::RelativeSequence<>rs, Word &previous_n,const Word n ) { // DS_04
                                        // Empty
                                    },
                                    []( typename samg::grcodec::toolkits::RunLengthCommon<Word>::RelativeSequence<>rs, Word &previous_n,const Word n ) { // DS_Q5
                                        previous_n = n;
                                    },
                                    []( typename samg::grcodec::toolkits::RunLengthCommon<Word>::RelativeSequence<>rs, Word &previous_n,const Word n ) { // DS_Q6
                                        _write_integer_( rs, previous_n, n );
                                    },
                                    []( typename samg::grcodec::toolkits::RunLengthCommon<Word>::RelativeSequence<>rs, Word &previous_n,const Word n ) { // DS_Q7
                                        // Empty
                                    },
                                    []( typename samg::grcodec::toolkits::RunLengthCommon<Word>::RelativeSequence<>rs, Word &previous_n,const Word n ) { // DS_Q8
                                        previous_n = n;
                                    },
                                    []( typename samg::grcodec::toolkits::RunLengthCommon<Word>::RelativeSequence<>rs, Word &previous_n,const Word n ) { // DS_Q9
                                        _write_integer_( rs, previous_n, n, samg::grcodec::toolkits::RunLengthCommon<Word>::IS_NEGATIVE );
                                    },
                                    []( typename samg::grcodec::toolkits::RunLengthCommon<Word>::RelativeSequence<>rs, Word &previous_n,const Word n ) { // DS_SINK
                                        // Empty
                                    },
                                    []( typename samg::grcodec::toolkits::RunLengthCommon<Word>::RelativeSequence<>rs, Word &previous_n,const Word n ) { // DS_ERROR
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
                                static void _write_integer_( typename samg::grcodec::toolkits::RunLengthCommon<Word>::RelativeSequence<> rs, const Word n, const std::size_t r, const bool is_negative = false ) {
                                    typename samg::grcodec::toolkits::RunLengthCommon<Word>::rseq_t x = is_negative ? ((typename samg::grcodec::toolkits::RunLengthCommon<Word>::rseq_t)n) * -1 : n;
                                    for (std::size_t j = 0; j < r; ++j) {
                                        // rs.push_back(x);
                                        rs->push(x);
                                    }
                                }

                                /**
                                 * @brief This function identifies the case generated by a next FSM input. 
                                 * 
                                 * @param codec 
                                 * @param n 
                                 * @return const DCase 
                                 */
                                static const DCase _get_case_( std::shared_ptr<samg::grcodec::base::reader::CodecReader<Word>> codec, Word &n ) {
                                    if( !codec->has_more() ) {
                                        return DCase::DC_EOS;
                                    }else {
                                        n = codec->next();
                                        if( n != samg::grcodec::toolkits::RunLengthCommon<Word>::NEGATIVE_FLAG && n != samg::grcodec::toolkits::RunLengthCommon<Word>::REPETITION_FLAG ) {
                                            return DCase::DC_INT;
                                        } else if( n == samg::grcodec::toolkits::RunLengthCommon<Word>::NEGATIVE_FLAG ) {
                                            return DCase::DC_NEGFLAG;
                                        } else if( n == samg::grcodec::toolkits::RunLengthCommon<Word>::REPETITION_FLAG ) {
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
                                void _init_( Word &n ) {
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
                                DState next( std::shared_ptr<samg::grcodec::base::reader::CodecReader<Word>> codec, Word &previous_n, Word &n ) {
                                    if( !this->is_init ) {
                                        this->_init_( n );
                                        this->is_init = true;
                                    }
                                    // DCase c = FSMDecoder::_get_case_( codec, n );
                                    // this->current_state = fsm[this->current_state][ c ];
                                    // std::cout << "\t\t\tOfflineRiceRunsReader/FSMDecoder/next> current_state = "<< this->current_state << "; case = " << c << std::endl;
                                    // return this->current_state;
                                    return this->current_state = fsm[this->current_state][ FSMDecoder::_get_case_( codec, n ) ];
                                }

                                /**
                                 * @brief This function allows running the current state's associated function. 
                                 * 
                                 * @param rs 
                                 * @param previous_n 
                                 * @param n 
                                 */
                                void run( typename samg::grcodec::toolkits::RunLengthCommon<Word>::RelativeSequence<> rs, Word &previous_n, const Word n ) {
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
                                
                                bool is_output_state() {
                                    return  this->current_state == DState::DS_Q1 ||
                                            this->current_state == DState::DS_Q3 ||
                                            this->current_state == DState::DS_Q6 ||
                                            this->current_state == DState::DS_Q9;
                                }

                                /**
                                 * @brief This function restarts the FSM.
                                 * 
                                 */
                                void restart() {
                                    this->is_init = false;
                                }
                        };

                        // Attributes for relative-sequence traversal:
                        std::shared_ptr<samg::grcodec::rice::reader::OfflineRCodecReader<Word>> codec;
                        FSMDecoder                      decoding_fsm;
                        Word                            decoding_previous_n, 
                                                        decoding_n;
                        typename samg::grcodec::toolkits::RunLengthCommon<Word>::rseq_t decoding_previous_relative_value;
                        typename samg::grcodec::toolkits::RunLengthCommon<Word>::AbsoluteSequence<> decoding_next_buffer;
                        // typename samg::grcodec::toolkits::RunLengthCommon<Word>::RelativeSequence<> encoding_buffer;
                        // bool    is_open,
                        bool decoding_is_first;

                    public:
                        OfflineRiceRunsReader( std::shared_ptr<samg::grcodec::rice::reader::OfflineRCodecReader<Word>> codec ):
                            samg::grcodec::base::reader::CodecFileReader<Word>::CodecFileReader( codec->get_file_name() ),
                            codec ( codec )  { 
                            // this->encoding_is_first = true;
                            // this->is_open = false;
                            // std::cout << "OfflineRiceRunsReader/init> (1) " << std::endl;
                            this->restart(); 
                            // std::cout << "OfflineRiceRunsReader/init> (2) " << std::endl;
                        }

                        const Word next() override {
                            // if( !this->is_open ) {
                            //     throw std::runtime_error("Stream from file \""+this->get_file_name()+"\" is not openned!");
                            // }
                            // std::cout << "OfflineRiceRunsReader/next> decoding_previous_n = " << this->decoding_previous_n << "; decoding_n = " << this->decoding_n << std::endl;
                            if( this->decoding_next_buffer->empty() ) {
                                std::uint8_t s;
                                typename samg::grcodec::toolkits::RunLengthCommon<Word>::RelativeSequence<> relative_sequence  = adapter::get_instance<typename samg::grcodec::toolkits::RunLengthCommon<Word>::rseq_t>( adapter::QueueAdapterType::Q_QUEUEADAPTER );
                                do { 
                                    s = this->decoding_fsm.next( this->codec, this->decoding_previous_n, this->decoding_n);
                                    if( this->decoding_fsm.is_error_state() ) { break; }
                                    // std::cout << "\t\tOfflineRiceRunsReader/next> pre-run --- decoding_previous_n = " << this->decoding_previous_n << "; decoding_n = " << this->decoding_n << "; |relative_sequence| = " << relative_sequence->size() << std::endl;
                                    this->decoding_fsm.run( relative_sequence, this->decoding_previous_n, this->decoding_n );
                                    // std::cout << "\t\tOfflineRiceRunsReader/next> post-run --- decoding_previous_n = " << this->decoding_previous_n << "; decoding_n = " << this->decoding_n << "; |relative_sequence| = " << relative_sequence->size() << std::endl;
                                }while( !this->decoding_fsm.is_output_state() );

                                // If it is not the first time executing `next`, then insert the previous relativized absolute value to the beginning of the just retrieved relative sequence:
                                if( !this->decoding_is_first ) {
                                    typename samg::grcodec::toolkits::RunLengthCommon<Word>::RelativeSequence<> tmp = adapter::get_instance<typename samg::grcodec::toolkits::RunLengthCommon<Word>::rseq_t>( adapter::QueueAdapterType::Q_QUEUEADAPTER );
                                    tmp->push( this->decoding_previous_relative_value );
                                    while( !relative_sequence->empty() ) {
                                        tmp->push( relative_sequence->front() );
                                        relative_sequence->pop();
                                    }
                                    // std::swap( tmp, relative_sequence );
                                    adapter::QueueAdapter<typename samg::grcodec::toolkits::RunLengthCommon<Word>::rseq_t>::swap( *tmp, *relative_sequence );
                                }

                                // Retrieve a transformed sequence from the relative one:
                                this->decoding_next_buffer.reset();
                                this->decoding_next_buffer = samg::grcodec::toolkits::RunLengthCommon<Word>::get_transformed_absolute_sequence( relative_sequence );
                                
                                // Back up the last relativized (transformed) absolute value:
                                this->decoding_previous_relative_value = samg::grcodec::toolkits::RunLengthCommon<Word>::transform_rval( this->decoding_next_buffer->back() );
                                // std::cout << "\t\tOfflineRiceRunsReader/next> decoding_previous_relative_value = " << this->decoding_previous_relative_value << std::endl;
                                
                                // It it is not the first time executing `next`, then remove the previously added last relativized absolute value:
                                // (it was already used to compute the next absolute value(s))
                                if( !this->decoding_is_first ){
                                    this->decoding_next_buffer->pop();
                                }
                                
                                this->decoding_is_first = false;
                            }

                            Word v = this->decoding_next_buffer->front();
                            this->decoding_next_buffer->pop();
                            // std::cout << "\t\tOfflineRiceRunsReader/next> v = " << v << "; |decoding_next_buffer| = " << this->decoding_next_buffer->size() << std::endl;
                            return v;
                        }

                        void restart( ) override {
                            // std::cout << "OfflineRiceRunsReader/restart> (1) " << std::endl;
                            this->decoding_previous_n = 0;
                            this->decoding_n = 0;
                            this->decoding_previous_relative_value = 0;
                            this->decoding_is_first = true;

                            // if( !(this->is_open) ) {
                            //     this->codec = std::make_shared<samg::grcodec::rice::reader::OfflineRCodecReader<Word>>( this->get_file_name() );
                            //     this->is_open = true;
                            // } else if( !soft_restart ) {
                            //         this->codec->restart();
                            // }
                            // if( !soft_restart ) {
                            // std::cout << "OfflineRiceRunsReader/restart> (2) " << std::endl;
                            this->codec->restart();
                            // }

                            // std::cout << "OfflineRiceRunsReader/restart> (3) " << std::endl;

                            this->decoding_fsm.restart();

                            // std::cout << "OfflineRiceRunsReader/restart> (4) " << std::endl;
                        
                            if( this->decoding_next_buffer ) { 
                                this->decoding_next_buffer.reset();
                            }
                            this->decoding_next_buffer = adapter::get_instance<Word>( adapter::QueueAdapterType::Q_QUEUEADAPTER );

                            // std::cout << "OfflineRiceRunsReader/restart> (5) " << std::endl;
                            // if( this->encoding_buffer ){
                            //     this->encoding_buffer.reset();
                            // }
                            // this->encoding_buffer = adapter::get_instance<typename samg::grcodec::toolkits::RunLengthCommon<Word>::rseq_t>( adapter::QueueAdapterType::Q_QUEUEADAPTER );
                        }
                        
                        const bool has_more() const override {
                            // return this->codec.has_more();
                            return  //this->is_open && (
                                        this->codec->has_more() || 
                                        !(this->decoding_next_buffer->empty());
                                    //);
                        }

                        void close() override {
                            // if( this->is_open ) {
                                this->codec->close();
                                this->codec.reset();
                                this->decoding_next_buffer.reset();
                                // this->encoding_buffer.reset();
                                // this->is_open = false;
                            // }
                        }


                        const std::vector<std::size_t> get_metadata() const {
                            return this->codec->get_metadata();
                        }

                        void add_metadata( std::size_t v ) {
                            throw std::runtime_error("Non-implemented method!");
                        }

                        void push_metadata( std::size_t v ) {
                            throw std::runtime_error("Non-implemented method!");
                        }
                };
            }
        }
    }
}