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
    register unsigned _B_x = (x);           \
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

            // class QUIntQueueAdapter : public QueueAdapter<std::uint64_t> {
            //     private:
            //         // std::queue<std::uint64_t,std::deque<std::uint64_t>> queue;
            //         std::vector<std::uint64_t> queue;

            //         void _swap_( QueueAdapter<std::uint64_t>& other ) override {
            //             QUIntQueueAdapter* x = dynamic_cast<QUIntQueueAdapter*>( &other );
            //             std::swap( this->queue, x->queue );
            //         }

            //     public:
            //         QUIntQueueAdapter() :
            //             // queue ( std::queue<std::uint64_t>() ) {}
            //             queue ( std::vector<std::uint64_t>() ) {}

            //         std::uint64_t front() override {
            //             std::uint64_t tmp = this->queue.front();
            //           //  std::cout << "QUIntQueueAdapter / front> std::uint64_t = " << typeid(std::uint64_t).name() << " front (" << typeid(tmp).name() << ") = " << tmp << std::endl;
            //             return tmp;
            //         }

            //         std::uint64_t back() override {
            //             return this->queue.back();
            //         }

            //         void push( std::uint64_t v ) override {
            //           //  std::cout << "QUIntQueueAdapter / push> std::uint64_t = " << typeid(std::uint64_t).name() << " push (" << v << ") --- " << typeid(v).name() << std::endl;
            //             // this->queue.push( v );
            //             this->queue.push_back(v);
            //         }

            //         bool empty() override {
            //             return this->queue.empty();
            //         }

            //         std::size_t size() override {
            //             return this->queue.size();
            //         }

            //         void pop() override {
            //             // this->queue.pop();
            //             this->queue.erase(this->queue.begin());
            //         }
            // };

            // class QIntQueueAdapter : public QueueAdapter<std::int64_t> {
            //     private:
            //         // std::queue<std::int64_t,std::deque<std::int64_t>> queue;
            //         std::vector<std::int64_t> queue;

            //         void _swap_( QueueAdapter<std::int64_t>& other ) override {
            //             QIntQueueAdapter* x = dynamic_cast<QIntQueueAdapter*>( &other );
            //             std::swap( this->queue, x->queue );
            //         }

            //     public:
            //         QIntQueueAdapter() :
            //             // queue ( std::queue<std::int64_t>() ) {}
            //             queue ( std::vector<std::int64_t>() ) {}

            //         std::int64_t front() override {
            //             std::int64_t tmp = this->queue.front();
            //           //  std::cout << "QUIntQueueAdapter / front> std::int64_t = " << typeid(std::int64_t).name() << " front (" << typeid(tmp).name() << ") = " << tmp << std::endl;
            //             return tmp;
            //         }

            //         std::int64_t back() override {
            //             return this->queue.back();
            //         }

            //         void push( std::int64_t v ) override {
            //           //  std::cout << "QUIntQueueAdapter / push> std::int64_t = " << typeid(std::int64_t).name() << " push (" << v << ") --- " << typeid(v).name() << std::endl;
            //             // this->queue.push( v );
            //             this->queue.push_back(v);
            //         }

            //         bool empty() override {
            //             return this->queue.empty();
            //         }

            //         std::size_t size() override {
            //             return this->queue.size();
            //         }

            //         void pop() override {
            //             // this->queue.pop();
            //             this->queue.erase(this->queue.begin());
            //         }
            // };

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

            // template<typename Type> QueueAdapter<Type>& get_instance( QueueAdapterType type,
            //     typename std::vector<Type>::iterator begin = {}, 
            //     typename std::vector<Type>::iterator end = {},
            //     typename std::vector<Type>::reverse_iterator rbegin = {}
            // ) {
            //     switch( type ) {
            //         case QueueAdapterType::Q_QUEUEADAPTER:
            //             return *( new QQueueAdapter<Type>( ) );
            //         case QueueAdapterType::ITERATOR_QUEUEADAPTER:
            //             return *( new IteratorQueueAdapter<Type>( begin, end, rbegin ) );
            //         default:
            //             throw std::runtime_error("adapter/get_instance> Non-implemented QueueAdapter!");
            //     }
            // }
            template <typename Type> std::unique_ptr<QueueAdapter<Type>> get_instance(QueueAdapterType type,
                                                                typename std::vector<Type>::iterator begin = {},
                                                                typename std::vector<Type>::iterator end = {},
                                                                typename std::vector<Type>::reverse_iterator rbegin = {}) {
                switch (type) {
                    case QueueAdapterType::Q_QUEUEADAPTER:
                        return std::make_unique<QQueueAdapter<Type>>();
                    // case QueueAdapterType::QUINT_QUEUEADAPTER:
                    //     // return static_cast<std::unique_ptr<QueueAdapter<std::uint64_t>>>( std::make_unique<QUIntQueueAdapter>() );
                    //     // return std::make_unique<QUIntQueueAdapter>();
                    //     return std::make_unique<QQueueAdapter<Type>>();
                    // case QueueAdapterType::QINT_QUEUEADAPTER:
                    //     // return static_cast<std::unique_ptr<QueueAdapter<std::int64_t>>>( std::make_unique<QIntQueueAdapter>() );
                    //     // return std::make_unique<QIntQueueAdapter>();
                    //     return std::make_unique<QQueueAdapter<Type>>();
                    case QueueAdapterType::ITERATOR_QUEUEADAPTER:
                        return std::make_unique<IteratorQueueAdapter<Type>>(begin, end, rbegin);
                    default:
                        throw std::runtime_error("adapter/get_instance> Non-implemented QueueAdapter!");
                }
            }
        }

        // TODO: Add `uint computeGolombRiceParameter_forList(uint *buff, uint n)` from [bc.hpp].
        /**
         * @brief This class represents a Golomb-Rice encoding of a sequence of integers based on the [https://github.com/migumar2/uiHRDC uiuiHRDC] library.
         * 
         * @tparam Word 
         * 
         * @note It requires [https://github.com/simongog/sdsl-lite sdsl] library.
         */
        template<typename Word, typename Length = std::uint64_t> class RCodec {
            static_assert(
                    std::is_same_v<Word, std::uint8_t> ||
                    std::is_same_v<Word, std::uint16_t> ||
                    std::is_same_v<Word, std::uint32_t> ||
                    std::is_same_v<Word, std::uint64_t>,
                    "typename must be one of std::uint8_t, std::uint16_t, std::uint32_t, or std::uint64_t");
            static_assert(
                std::is_same_v<Length, std::uint8_t> ||
                std::is_same_v<Length, std::uint16_t> ||
                std::is_same_v<Length, std::uint32_t> ||
                std::is_same_v<Length, std::uint64_t>,
                "Second typename must be one of std::uint8_t, std::uint16_t, std::uint32_t, or std::uint64_t");
            private:
                typedef std::uint8_t bit_t;

                static const std::size_t WORD = sizeof(Word) * BITS_PER_BYTE;
                
                // BinarySequence/*<Word,Length>*/ sequence;
                Length iterator;

                // /**
                //  * @brief This function allows computing the log2 of x.
                //  * 
                //  * @param x 
                //  * @return std::double_t 
                //  */
                // static const std::double_t log2(const double x) {
                //     return std::log(x) / std::log(2.0); // NOTE Much more efficient than std::log2! 34 nanoseconds avg.
                //     // return std::log2(x); // NOTE 180 nanoseconds avg.
                // }

                /**
                 * @brief writes e[p..p+len-1] = s, len <= W
                 * 
                 * @param e 
                 * @param p 
                 * @param len 
                 * @param s 
                 */
                static inline void _bitwrite_(register Word *e, register Length p, register std::size_t len, const register Word s) {
                    e += p / RCodec<Word,Length>::WORD;
                    p %= RCodec<Word,Length>::WORD;
                    if (len == RCodec<Word,Length>::WORD) {
                        *e |= (*e & ((1 << p) - 1)) | (s << p);
                        if (!p)
                            return;
                        e++;
                        *e = (*e & ~((1 << p) - 1)) | (s >> (RCodec<Word,Length>::WORD - p));
                    } else {
                        if (p + len <= RCodec<Word,Length>::WORD) {
                            *e = (*e & ~(((1 << len) - 1) << p)) | (s << p);
                            return;
                        }
                        *e = (*e & ((1 << p) - 1)) | (s << p);
                        e++;
                        len -= RCodec<Word,Length>::WORD - p;
                        *e = (*e & ~((1 << len) - 1)) | (s >> (RCodec<Word,Length>::WORD - p));
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
                static Word _bitread_(Word *e, Length p, const std::size_t len) {
                    Word answ;
                    e += p / RCodec<Word,Length>::WORD;
                    p %= RCodec<Word,Length>::WORD;
                    answ = *e >> p;
                    if (len == RCodec<Word,Length>::WORD) {
                        if (p)
                            answ |= (*(e + 1)) << (RCodec<Word,Length>::WORD - p);
                    } else {
                        if (p + len > RCodec<Word,Length>::WORD)
                            answ |= (*(e + 1)) << (RCodec<Word,Length>::WORD - p);
                        answ &= (1 << len) - 1;
                    }
                    return answ;
                }

                static Word _bitget_(Word* e, const Length p) {
                    return (((e)[(p) / RCodec<Word,Length>::WORD] >> ((p) % RCodec<Word,Length>::WORD)) & 1);
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
                 * @brief This function returns the size in bits of the value Rice-encode. 
                 * 
                 * @param val 
                 * @param nbits 
                 * @return Length 
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
                static inline Word _rice_encode_(Word *buf, Length pos, Word val, std::size_t nbits) {
                    // val-=OFFSET_LOWEST_VALUE;  //0 is never encoded. So encoding 1 as 0, 2 as 1, ... v as v-1
                    std::size_t w;
                    RCodec<Word, Length>::_bitwrite_ (buf, pos, nbits, val); 
                    pos+=nbits;
                    
                    for (w = (val>>nbits); w > 0; w--) {
                        RCodec<Word, Length>::_bitwrite_ (buf, pos, 1, 1); pos++;
                    }
                    RCodec<Word, Length>::_bitwrite_ (buf, pos, 1, 0); pos++;
                    return pos;
                }
                
                /**
                 * @brief This function allows decoding n using the Rice algorithm. 
                 * 
                 * @param val 
                 * @param nbits 
                 * @return Word 
                 */
                static Word _rice_decode_(Word *buf, Length *pos, std::size_t nbits) {
                    Word v;
                    v = _bitread_(buf, *pos, nbits);
                    *pos+=nbits;
                    //printf("\n\t [decoding] v del bitread vale = %d",v);
                    while (RCodec<Word, Length>::_bitget_(buf,*pos))
                    {
                        v += (1<<nbits);
                        (*pos)++;
                    }

                    (*pos)++;
                    return(v /*+ OFFSET_LOWEST_VALUE*/);   //1 was encoded as 0 ... v as v-1 ... !!
                }

            public:

                /**
                 * @brief This struct represents a binary sequence.
                 * 
                 * @tparam Word used to encode bits.
                 * @tparam Length used to represents lengths of the binary sequence (i.e. actual length and indexes to traverse the sequence).
                 */
                typedef struct _BinarySequence_ {
                /*template<typename Word, typename Length = std::uint64_t>*/ 
                    static_assert(
                        std::is_same_v<Word, std::uint8_t> ||
                        std::is_same_v<Word, std::uint16_t> ||
                        std::is_same_v<Word, std::uint32_t> ||
                        std::is_same_v<Word, std::uint64_t>,
                        "First typename must be one of std::uint8_t, std::uint16_t, std::uint32_t, or std::uint64_t");
                    static_assert(
                        std::is_same_v<Length, std::uint8_t> ||
                        std::is_same_v<Length, std::uint16_t> ||
                        std::is_same_v<Length, std::uint32_t> ||
                        std::is_same_v<Length, std::uint64_t>,
                        "Second typename must be one of std::uint8_t, std::uint16_t, std::uint32_t, or std::uint64_t");

                    static const Length WORD_GROWING_SPAN = 4096; // Space in number of Word-type that the bitmap must grow. 
                    Word    *sequence; // It is the bitmap.
                    std::size_t k;
                    Length  length, // Index of current bit within the bitmap 'sequence'.
                            src_length, // Number of encoded integers.
                            max_length; // Maximum capacity of the bitmap in Word(s).
                    bool free_sequence = false; // Use to determine whether free or not the `sequence` pointer. It could be freed only when the pointer is created internally in the struct. If it comes from outside, is not freed to prevent side effects.
                    
                    /**
                     * @brief Construct a new Binary Sequence object
                     * 
                     * @param file_name 
                     */
                    _BinarySequence_( const std::string file_name )/*:
                        free_sequence(true) */
                    {
                        // TODO Test it!
                        // std::cout << "BinarySequence --- (1)" << std::endl;
                        samg::matutx::WordSequenceSerializer<Word> serializer = samg::matutx::WordSequenceSerializer<Word>( file_name );
                        // std::cout << "BinarySequence --- (2)" << std::endl;
                        this->k = serializer.template get_value<std::size_t>();
                        // std::cout << "BinarySequence --- (3): k = " << this->k << std::endl;
                        this->length = serializer.template get_value<Length>();
                        // std::cout << "BinarySequence --- (4): length = " << this->length << std::endl;
                        this->src_length = serializer.template get_value<Length>();
                        // std::cout << "BinarySequence --- (5): src_length = " << this->src_length << std::endl;
                        this->max_length = serializer.template get_value<Length>();
                        // std::cout << "BinarySequence --- (6): max_length = " << this->max_length << std::endl;
                        // std::vector<Word> seq = serializer.get_next_values( this->src_length );
                        std::vector<Word> seq = serializer.get_next_values( this->max_length );
                        // samg::utils::print_vector<Word>("_BinarySequence_ - sequence right after retrieval: ",seq);
                        // std::cout << "BinarySequence --- (7)" << std::endl;
                        this->sequence = BinarySequence::rmalloc(this->max_length);
                        std::memcpy( this->sequence, seq.data(), this->max_length * sizeof(Word) );
                        // this->sequence = BinarySequence/*<Word,Length>*/::rrealloc( this->sequence, this->max_length );
                        // std::cout << "BinarySequence --- (8) --- end!" << std::endl;
                        this->free_sequence = true;
                    }

                    /**
                     * @brief Construct a new Binary Sequence object from a serialization.
                     * 
                     * @param file_name 
                     */
                    _BinarySequence_( std::vector<Word> serialization )/*:
                        free_sequence(true) */
                    {
                        // TODO Test it!
                        // std::cout << "BinarySequence --- (1)" << std::endl;
                        samg::matutx::WordSequenceSerializer<Word> serializer = samg::matutx::WordSequenceSerializer<Word>( serialization );
                        // std::cout << "BinarySequence --- (2)" << std::endl;
                        this->k = serializer.template get_value<std::size_t>();
                        // std::cout << "BinarySequence --- (3): k = " << this->k << std::endl;
                        this->length = serializer.template get_value<Length>();
                        // std::cout << "BinarySequence --- (4): length = " << this->length << std::endl;
                        this->src_length = serializer.template get_value<Length>();
                        // std::cout << "BinarySequence --- (5): src_length = " << this->src_length << std::endl;
                        this->max_length = serializer.template get_value<Length>();
                        // std::cout << "BinarySequence --- (6): max_length = " << this->max_length << std::endl;
                        // std::vector<Word> seq = serializer.get_next_values( this->src_length );
                        std::vector<Word> seq = serializer.get_next_values( this->max_length );
                        // samg::utils::print_vector<Word>("_BinarySequence_ - sequence right after retrieval: ",seq);
                        // std::cout << "BinarySequence --- (7)" << std::endl;
                        this->sequence = BinarySequence::rmalloc(this->max_length);
                        std::memcpy( this->sequence, seq.data(), this->max_length * sizeof(Word) );
                        // this->sequence = BinarySequence/*<Word,Length>*/::rrealloc( this->sequence, this->max_length );
                        // std::cout << "BinarySequence --- (8) --- end!" << std::endl;
                    }
                    
                    /**
                     * @brief Construct a new Binary Sequence object
                     * 
                     * @param k 
                     */
                    _BinarySequence_( const std::size_t k ):
                        // free_sequence(true),
                        // bitmap_length( src_length * 2 * sizeof(Word) ),
                        k(k),
                        length(0),
                        src_length(0),
                        max_length( WORD_GROWING_SPAN )
                    {
                        this->sequence = BinarySequence/*<Word,Length>*/::rmalloc( this->max_length );
                    }

                    /**
                     * @brief Construct a new Binary Sequence object
                     * 
                     * @param sequence 
                     * @param sequence_length is the bit-index of sequence.
                     * @param src_length is the number of Word-type integers stored in sequence.
                     * @param k 
                     */
                    _BinarySequence_( Word *sequence, const Length sequence_length, const Length src_length, const std::size_t k ):
                        // free_sequence(false),
                        k(k),
                        sequence( sequence ),
                        length( sequence_length ),
                        src_length( src_length ),
                        max_length( src_length )
                        // max_length( sequence_length )
                    {}

                    // ~_BinarySequence_() {
                    //     if(this->free_sequence) {
                    //         BinarySequence::rfree( this->sequence );
                    //     }
                    // }

                    /**
                     * @brief This function adds a new integer n with length n_bits[b] in the bitmap.
                     * 
                     * @param n 
                     * @param n_bits 
                     */
                    void add( Word n, std::size_t n_bits ) {
                        // std::cout << "RCodec/BinarySequence/add --- n = " << n << "; |n| = " << n_bits << "[b]; length = " << this->length << "; max_length [B] = " << (this->max_length * sizeof(Word)) << std::endl;
                        if( std::ceil( (double)(this->length + n_bits ) / (double) BITS_PER_BYTE ) >= ( this->max_length * sizeof(Word) ) ) {
                            // std::cout << "RCodec/BinarySequence/add --- growing bitmap from " << this->max_length << "[W]";
                            this->max_length += ( std::ceil( (double)n_bits / (double)BITS_PER_BYTE ) + BinarySequence::WORD_GROWING_SPAN);
                            // std::cout << " to " << this->max_length << " [W]" << std::endl;
                            this->sequence = BinarySequence::rrealloc( this->sequence, this->max_length );
                        }
                        this->length = RCodec<Word, Length>::_rice_encode_(this->sequence, this->length, n, this->k);
                        ++(this->src_length);
                        // RCodec<Word,Length>::display_binary_sequence(*this,false);
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
                    static Word* rmalloc( Length n ) {
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
                    // static void* rmalloc( Length n ) {
                    //     void *p;
                    //     if (n == 0)
                    //         return NULL;
                    //     p = (void *)malloc(n);
                    //     if (p == NULL) {
                    //         throw std::runtime_error("Could not allocate "+ std::to_string(n) +" bytes\n");
                    //     }
                    //     return p;
                    // }

                    /**
                     * @brief This function reallocate space for n Word-type integers in pointer p. 
                     * 
                     * @param p 
                     * @param n 
                     * @return Word* 
                     */
                    static Word* rrealloc( Word *p, Length n ) {
                        // std::cout << "RCodec/BinarySequence/rrealloc --- (1)" << std::endl;
                        if( p == NULL ) {
                            // std::cout << "RCodec/BinarySequence/rrealloc --- (2)" << std::endl;
                            return BinarySequence/*<Word, Length>*/::rmalloc( n );
                        }
                        if( n == 0 ) {
                            // std::cout << "RCodec/BinarySequence/rrealloc --- (3)" << std::endl;
                            BinarySequence/*<Word, Length>*/::rfree( p );
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
                    // static void* rrealloc( void *p, Length n ) {
                    //     if( p == NULL )
                    //         return RCodec<Word, Length>::rmalloc(n);
                    //     if( n == 0 ) {
                    //         RCodec<Word, Length>::rfree(p);
                    //         return NULL;
                    //     }
                    //     p = (void *)realloc( p, n );
                    //     if(p == NULL ) {
                    //         throw std::runtime_error("Could not re-allocate "+ std::to_string(n) +" bytes\n");
                    //     }
                    //     return p;
                    // }

                    /**
                     * @brief This function serializes this binary sequence.
                     * 
                     * @param file_name 
                     */
                    void save( std::string file_name ) {
                        // TODO Test it!
                        // std::cout << "save --- (1)" << std::endl;
                        samg::matutx::WordSequenceSerializer<Word> serializer = samg::matutx::WordSequenceSerializer<Word>();
                        // std::cout << "save --- (2)" << std::endl;
                        serializer.template add_value<std::size_t>( this->k );
                        // std::cout << "save --- (3)" << std::endl;
                        serializer.template add_value<Length>( this->length );
                        // std::cout << "save --- (4)" << std::endl;
                        serializer.template add_value<Length>( this->src_length );
                        // std::cout << "save --- (5)" << std::endl;
                        serializer.template add_value<Length>( this->max_length );
                        // std::cout << "save --- (6)" << std::endl;
                        // serializer.template add_values<Word,Length>( this->sequence, this->bitmap_length / sizeof(Word) );
                        // serializer.template add_values<Word,Length>( this->sequence, this->src_length );
                        serializer.template add_values<Word,Length>( this->sequence, this->max_length );
                        // std::cout << "save --- (7)" << std::endl;
                        serializer.save( file_name );
                        // std::cout << "save --- (8) --- end!" << std::endl;
                        // serializer.print();
                    }

                    /**
                     * @brief This function serializes this binary sequence.
                     * 
                     * @param file_name 
                     */
                    std::vector<Word> get_serialization( ) {
                        // TODO Test it!
                        // std::cout << "save --- (1)" << std::endl;
                        samg::matutx::WordSequenceSerializer<Word> serializer = samg::matutx::WordSequenceSerializer<Word>();
                        // std::cout << "save --- (2)" << std::endl;
                        serializer.template add_value<std::size_t>( this->k );
                        // std::cout << "save --- (3) this->k = " << this->k << std::endl;
                        serializer.template add_value<Length>( this->length );
                        // std::cout << "save --- (4) this->length = " << this->length << std::endl;
                        serializer.template add_value<Length>( this->src_length );
                        // std::cout << "save --- (5) this->src_length = " << this->src_length << std::endl;
                        serializer.template add_value<Length>( this->max_length );
                        // std::cout << "save --- (6) this->max_length = " << this->max_length << std::endl;
                        // serializer.template add_values<Word,Length>( this->sequence, this->bitmap_length / sizeof(Word) );
                        // serializer.template add_values<Word,Length>( this->sequence, this->src_length );
                        serializer.template add_values<Word,Length>( this->sequence, this->max_length );
                        // std::cout << "save --- (7)" << std::endl;
                        return serializer.get_serialized_sequence();
                    }
                } BinarySequence;

                BinarySequence sequence;
                
                RCodec( BinarySequence/*<Word,Length>*/ bs ): 
                    sequence( bs )
                { this->restart(); }

                RCodec( const std::size_t k ): 
                    sequence( BinarySequence/*<Word,Length>*/( k ) )
                { this->restart(); }

                RCodec( Word *sequence, const Length sequence_length, const Length src_length, const std::size_t k ): 
                    sequence( BinarySequence/*<Word,Length>*/( sequence, sequence_length, src_length, k ) )
                { this->restart(); }

                /**
                 * @brief Given a list of (n) integers (buff) [typically gap values from an inverted list], computes the optimal b parameter, that is, the number of bits that will be coded binary-wise from a given encoded value.
                 * @note $$ val = \lfloor (\log_2( \sum_{i=0}^{n-1} (diff[i])) / n )) \rfloor $$
                 * 
                 * @param sequence 
                 * @return std::size_t 
                 */
                static std::size_t compute_GR_parameter_for_list( std::vector<Word> sequence ) {
                    Word total =0;
                    register Length i;
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
                    register Length i;
                    while( !(sequence.empty()) ) {
                        total += sequence.front(); sequence.pop();
                    }
                    total /= sequence.size();
                    std::size_t val;
                    FLOORLOG_2(((uint)total),val);
                    
                    return val;
                }

                /**
                 * @brief This function encodes an integer n.
                 * 
                 * @param sequence 
                 * @param k 
                 * @return BinarySequence
                 */
                static BinarySequence/*<Word,Length>*/ encode( std::vector<Word> sequence, const std::size_t k ) {
                    // std::cout << "encode (1)" << std::endl;
                    BinarySequence/*<Word,Length>*/ bs( k );
                    // std::cout << "encode (2) --- bitmap_length = " << bs.bitmap_length << std::endl;

                    // std::size_t MAX_N_BITS = 0; // !NOTE For testing/debugging purposes!
                    // Length n_length; // !NOTE For testing/debugging purposes!
                    for ( Length i = 0; i < sequence.size(); ++i ) {
                        // n_length = RCodec<Word,Length>::_value_size_( sequence[i], k ); // !NOTE For testing/debugging purposes!
                        // MAX_N_BITS = ( n_length > MAX_N_BITS ) ? n_length : MAX_N_BITS; // !NOTE For testing/debugging purposes!

                        // bs.length = RCodec<Word, Length>::_rice_encode_(bs.sequence, bs.length, sequence[i], k);
                        // ++(bs.src_length);

                        bs.add( sequence[i], RCodec<Word,Length>::_value_size_( sequence[i], bs.k ) );
                    }
                    // std::cout << "encode> MAX_N_BITS = " << MAX_N_BITS << std::endl;
                    // std::cout << "encode (3)" << std::endl;

                    return bs;
                }
                
                /**
                 * @brief This function allows decoding an integer encoded as a bitmap.
                 * 
                 * @param bs 
                 * @return std::vector<Word>
                 */
                static std::vector<Word> decode( BinarySequence/*<Word,Length>*/ &bs ) {
                    Length    position = 0;
                    std::vector<Word> sequence;

                    while( position < bs.length ) {
                        Word v = RCodec<Word, Length>::_rice_decode_( bs.sequence, &position, bs.k );
                        sequence.push_back(v);
                    }
                    return sequence;
                }

                /**
                 * @brief This function allows appending the encoded representation of n to an internal bitmap.
                 * 
                 * @param n 
                 * 
                 * @warning This implementation is O(n), where n is the number of bits in the internal bitmap.
                 */
                // std::size_t MAX_N_BITS = 0; // !NOTE For testing/debugging purposes!
                void append(const Word n) {
                    // Length n_length = RCodec<Word,Length>::_value_size_( n, this->sequence.k );
                    // // MAX_N_BITS = ( n_length > MAX_N_BITS ) ? n_length : MAX_N_BITS;// !NOTE For testing/debugging purposes!
                    // // std::cout << "RiceRuns/append --- n = " << n << "; |n| = " << n_length << "[b]" << std::endl;
                    // if( std::ceil( (double)(this->sequence.length + n_length) / (double) RCodec<Word,Length>::BITS_PER_BYTE ) >= ( this->sequence.max_length * sizeof(Word) ) ) {
                    //     // this->sequence.bitmap_length *= 2;
                    //     this->sequence.max_length += BinarySequence/*<Word,Length>*/::WORD_GROWING_SPAN;
                    //     this->sequence.sequence = BinarySequence/*<Word,Length>*/::rrealloc( this->sequence.sequence, this->sequence.max_length );
                    //     // std::cout << "RiceRuns/append --- n = " << n << "\tgrowing BinarySequence bitmap!" << std::endl;
                    // }
                    // this->sequence.length = RCodec<Word, Length>::_rice_encode_( this->sequence.sequence, this->sequence.length, n, this->sequence.k );
                    // ++(this->sequence.src_length);

                    this->sequence.add( n, RCodec<Word,Length>::_value_size_( n, this->sequence.k ) );

                    // RCodec<Word,Length>::display_binary_sequence(this->sequence);
                    // ++(this->iterator);
                }

                /**
                 * @brief This function iterates on the internal bitmap as decodes and returns the next integer. 
                 * 
                 * @return const Word 
                 */
                const Word next() {
                    return RCodec<Word, Length>::_rice_decode_( this->sequence.sequence, &(this->iterator), this->sequence.k );
                } 

                /**
                 * @brief This function verifies whether the internal bitmap has or doesn't have more codewords to iterate on.
                 * 
                 * @return true 
                 * @return false 
                 */
                const bool has_more() const {
                    return this->iterator < this->sequence.length;
                }

                /**
                 * @brief This function restarts the iteration on the internal bitmap.
                 * 
                 */
                void restart() {
                    this->iterator = 0;
                }

                /**
                 * @brief This function returns a copy of the internal bitmap.
                 * 
                 * @return BinarySequence 
                 */
                BinarySequence/*<Word, Length>*/ get_binary_sequence() const {
                    return this->sequence;
                }

                /**
                 * @brief This function returns the number of stored bits.
                 * 
                 * @return const Length
                 */
                const Length length() const {
                    return this->sequence.length;
                }

                /**
                 * @brief This function returns the number of encoded integers.
                 * 
                 * @return const Length
                 */
                const Length size() const {
                    return this->sequence.src_length;
                }

                /**
                 * @brief This function returns the current iterator index.
                 * 
                 * @return Length
                 */
                const Length get_current_iterator_index() const {
                    return this->iterator;
                }

                /**
                 * @brief This function displays the bitmap and the metadata of a BinarySequence.
                 * 
                 * @param info 
                 * @param bs 
                 * @param show_bitmap 
                 */
                static void display_binary_sequence( std::string info, BinarySequence bs, bool show_bitmap = true ) {
                    std::cout << info << std::endl;
                    std::cout << "\tStored integers in "<<samg::utils::number_to_comma_separated_string(bs.max_length)<<"[W]: "<< samg::utils::number_to_comma_separated_string(bs.src_length) << std::endl;
                    std::cout << "\t|Bitmap| in bytes: " << samg::utils::number_to_comma_separated_string( std::ceil( (double)bs.length / (double)BITS_PER_BYTE) ) << "[B] / " << samg::utils::number_to_comma_separated_string( bs.max_length * sizeof(Word) ) << "[B]" << std::endl;
                    std::cout << "\t|Bitmap| in bits: " << samg::utils::number_to_comma_separated_string( bs.length ) << "[b] / " << samg::utils::number_to_comma_separated_string( bs.max_length * sizeof(Word) * BITS_PER_BYTE ) << "[b]" << std::endl;
                    std::cout << "\tCompression rate: Potential size<" << samg::utils::number_to_comma_separated_string( bs.src_length * sizeof(Word) * BITS_PER_BYTE ) << "[b]>; Actual size<"<< samg::utils::number_to_comma_separated_string( bs.length ) << "[b]>; rate = " << samg::utils::number_to_comma_separated_string( ((double)bs.length / (double)(bs.src_length * sizeof(Word) * BITS_PER_BYTE)) * 100 , 2 ) << "%" << std::endl;
                    std::cout << "\tBitmap index @ " << samg::utils::number_to_comma_separated_string(bs.length) << std::endl;
                    std::cout << "\tk: "<< samg::utils::number_to_comma_separated_string(bs.k) << std::endl;

                    if( show_bitmap ) {
                        std::cout << "\tBitmap: ";
                        for (Length i = 0; i < bs.length; i++) {
                            if ( RCodec<Word, Length>::_bitget_( bs.sequence, i ) )
                                std::cout << "1";
                            else 
                                std::cout << "0";
                        }
                    }
                    std::cout << std::endl;
                }

                friend std::ostream & operator<<(std::ostream & strm, const RCodec<Word,Length> &codec) {
                    BinarySequence/*<Word, Length>*/ bs = codec.get_binary_sequence();
                    Word    *buff = bs.sequence,
                            n = bs.length;
                    bool printer_prompt = false;
                    for ( Length i = 0; i < n ; i++ ) {
                        if( i == codec.get_current_iterator_index() ) {
                            strm << "|";
                            printer_prompt = true;
                        }
                        if ( RCodec<Word, Length>::_bitget_( buff, i ) )
                            strm << "1";
                        else 
                            strm << "0";
                    }
                    if( !printer_prompt ) {
                        strm << "|";
                    }
                    return strm;
                }
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

        /**
         * @brief This class represents a Rice-runs encoding of integers of type Word. This codec uses RCodec class to encode codewords. 
         * 
         * @tparam Word 
         * @tparam Length 
         */
        template<typename Word, typename Length = std::uint64_t> class RiceRuns {
            static_assert(
                    std::is_same_v<Word, std::uint8_t> ||
                    std::is_same_v<Word, std::uint16_t> ||
                    std::is_same_v<Word, std::uint32_t> ||
                    std::is_same_v<Word, std::uint64_t>,
                    "typename must be one of std::uint8_t, std::uint16_t, std::uint32_t, or std::uint64_t");
            static_assert(
                std::is_same_v<Length, std::uint8_t> ||
                std::is_same_v<Length, std::uint16_t> ||
                std::is_same_v<Length, std::uint32_t> ||
                std::is_same_v<Length, std::uint64_t>,
                "Second typename must be one of std::uint8_t, std::uint16_t, std::uint32_t, or std::uint64_t");
            
            private:
                using rseq_t = std::int64_t; //typedef unsigned long long int rseq_t; // Data type internally used by the relative sequence. It can be changed here to reduce memory footprint in case numbers in a relative sequence are small enough to fit in fewer bits.  
                // typedef std::queue<RiceRuns<Word,Length>::rseq_t> RelativeSequence;
                template<typename TypeInt = rseq_t> using RelativeSequence = std::unique_ptr<adapter::QueueAdapter<TypeInt>>;//adapter::QueueAdapter<TypeInt>; std::unique_ptr<adapter::QIntQueueAdapter>;
                // typedef std::queue<Word> AbsoluteSequence;
                template<typename TypeUInt = Word> using AbsoluteSequence = std::unique_ptr<adapter::QueueAdapter<TypeUInt>>;//adapter::QueueAdapter<TypeUInt>; std::unique_ptr<adapter::QUIntQueueAdapter>;
                // typedef std::queue<Word> AbsoluteBuffer;

                static const Length RLE_THRESHOLD = 3,     // Minimum number of repetitions to be compressed.
                                    ESCAPE_RANGE_SPAN = 2; // Range span to reserve integers as special symbols (e.g. negative flag, repetition mark).
                
                static const Word   NEGATIVE_FLAG = 0,
                                    REPETITION_FLAG = 1;

                static const bool   IS_NEGATIVE = true;
                
                /**
                 * @brief This function allows generating a gap centered in 0. The result is a value that belongs to either (-inf,-RiceRuns<Word,Length>::ESCAPE_RANGE_SPAN+1] or [RiceRuns<Word,Length>::ESCAPE_RANGE_SPAN,inf). This transformation of v allows to release values in [-RiceRuns<Word,Length>::ESCAPE_RANGE_SPAN,RiceRuns<Word,Length>::ESCAPE_RANGE_SPAN-1] to be used as scape symbols during encoding/decoding of, for instance, negative values.
                 * 
                 * @param v 
                 * @return RiceRuns<Word,Length>::rseq_t 
                 */
                static RiceRuns<Word,Length>::rseq_t transform_rval( RiceRuns<Word,Length>::rseq_t v ) {
                    return v >= 0 ? v + RiceRuns<Word,Length>::ESCAPE_RANGE_SPAN : v - RiceRuns<Word,Length>::ESCAPE_RANGE_SPAN; // Transform relative value. 
                }

                /**
                 * @brief This function allows recovering an original value transformed by `transform_rval`.
                 * 
                 * @param v 
                 * @return RiceRuns<Word,Length>::rseq_t 
                 */
                static RiceRuns<Word,Length>::rseq_t recover_rval( RiceRuns<Word,Length>::rseq_t v ) {
                    return v < 0 ? v + RiceRuns<Word,Length>::ESCAPE_RANGE_SPAN : v - RiceRuns<Word,Length>::ESCAPE_RANGE_SPAN; // Recover relative value.
                }

                /**
                 * @brief This function allows converting a sequence of unsigned Word integers into a relative sequence of signed integers by computing their sequential differences. The resultant sequence is transformed in the process to prevent the number 0 that is used to represent negative integers when numbers are encoded with variable-length integers.  
                 * 
                 * @param sequence 
                 * @return RelativeSequence 
                 */
                static RelativeSequence<> _get_transformed_relative_sequence_( AbsoluteSequence<> sequence ) {
                    RelativeSequence<> ans = adapter::get_instance<RiceRuns<Word,Length>::rseq_t>( adapter::QueueAdapterType::Q_QUEUEADAPTER );
                    // std::cout << sequence.size() << std::endl;
                    // if( sequence.size() > 0 ) {
                    if( !sequence.empty() ) {
                        /* NOTE
                            1 3 3 6  5  0 0 5
                            1 2 0 3 -1 -5 0 5 <--- Original relative values.
                            2 3 1 4 -1 -5 1 6 <--- transformed relative values.
                        */
                       Word current,
                            previous = sequence.front();
                        sequence.pop();

                        ans->push( transform_rval( previous ));
                        // ans.push_back( transform_rval( sequence[0] ) );
                        while( !sequence->empty() ) {
                            current = sequence.front(); sequence.pop();
                            // std::cout << current << std::endl;
                            // ans.push( transform_rval( ((RiceRuns<Word,Length>::rseq_t)(current)) - ((RiceRuns<Word,Length>::rseq_t)(previous)) ) );
                            ans->push( RiceRuns<Word,Length>::_get_transformed_relative_sequence_( current, previous ));
                            previous = current;
                        }
                        // for (std::size_t i = 1; i < sequence.size(); ++i) {
                        //     ans.push_back( transform_rval( ((RiceRuns<Word,Length>::rseq_t)(sequence[i])) - ((RiceRuns<Word,Length>::rseq_t)(sequence[i-1])) ) );
                        // }
                    }
                    return ans;
                }
                /**
                 * @brief This function converts `current` into a relative value given `previous`.
                 * 
                 * @param previous 
                 * @param current 
                 * @return RiceRuns<Word,Length>::rseq_t 
                 */
                static RiceRuns<Word,Length>::rseq_t _get_transformed_relative_sequence_( RiceRuns<Word,Length>::rseq_t relativa_previous, RiceRuns<Word,Length>::rseq_t relativa_current ) {
                    RiceRuns<Word,Length>::rseq_t ans = transform_rval( relativa_current - relativa_previous );
                  //  std::cout << "\t\tRiceRuns / _get_transformed_relative_sequence_> relativa_previous = " << relativa_previous << "; relativa_current = " << relativa_current << "; ans = " << ans << std::endl;
                    return ans;
                }

                /**
                 * @brief This function allows converting a relative sequence of sequential differences encoded as signed integers into an absolute sequence of unsigned Word integers. The resultant sequence is transformed in the process to revert previous transformation when relativization was applied by `_get_transformed_relative_sequence_` function.
                 * 
                 * @param relative_sequence 
                 * @return AbsoluteSequence
                 * 
                 * @warning Side effects on input!
                 */
                static AbsoluteSequence<> _get_transformed_absolute_sequence_( RelativeSequence<>& sequence ) {
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
                            ans->push( ((RiceRuns<Word,Length>::rseq_t)ans->back()) + recover_rval( sequence->front() ) );
                            sequence->pop();
                        }
                    }
                    return ans; //.release();
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
                        const std::array<std::function<void( RCodec<Word,Length>&, RiceRuns<Word,Length>::rseq_t&, const RiceRuns<Word,Length>::rseq_t, Length& )>,10> sfunction = {
                            []( RCodec<Word,Length> &codec, RiceRuns<Word,Length>::rseq_t &previous_n,const RiceRuns<Word,Length>::rseq_t n, Length &r ) { // ES_Q0
                                previous_n = n;
                                r = 1;
                            },
                            []( RCodec<Word,Length> &codec, RiceRuns<Word,Length>::rseq_t &previous_n,const RiceRuns<Word,Length>::rseq_t n, Length &r ) { // ES_Q1
                                ++r;
                            },
                            []( RCodec<Word,Length> &codec, RiceRuns<Word,Length>::rseq_t &previous_n,const RiceRuns<Word,Length>::rseq_t n, Length &r ) { // ES_Q2
                                _write_integer_( codec, previous_n, r );
                                previous_n = n;
                                r = 1;
                            },
                            []( RCodec<Word,Length> &codec, RiceRuns<Word,Length>::rseq_t &previous_n,const RiceRuns<Word,Length>::rseq_t n, Length &r ) { // ES_Q3
                                _write_integer_( codec, previous_n, r );
                                previous_n = n;
                                r = 1;
                            },
                            []( RCodec<Word,Length> &codec, RiceRuns<Word,Length>::rseq_t &previous_n,const RiceRuns<Word,Length>::rseq_t n, Length &r ) { // ES_Q4
                                _write_integer_( codec, previous_n, r, RiceRuns<Word,Length>::IS_NEGATIVE );
                                previous_n = n;
                                r = 1;
                            },
                            []( RCodec<Word,Length> &codec, RiceRuns<Word,Length>::rseq_t &previous_n,const RiceRuns<Word,Length>::rseq_t n, Length &r ) { // ES_Q5
                                ++r;
                            },
                            []( RCodec<Word,Length> &codec, RiceRuns<Word,Length>::rseq_t &previous_n,const RiceRuns<Word,Length>::rseq_t n, Length &r ) { // ES_Q6
                                _write_integer_( codec, previous_n, r, RiceRuns<Word,Length>::IS_NEGATIVE );
                                previous_n = n;
                                r = 1;
                            },
                            []( RCodec<Word,Length> &codec, RiceRuns<Word,Length>::rseq_t &previous_n,const RiceRuns<Word,Length>::rseq_t n, Length &r ) { // ES_PSINK
                                _write_integer_( codec, previous_n, r );
                            },
                            []( RCodec<Word,Length> &codec, RiceRuns<Word,Length>::rseq_t &previous_n,const RiceRuns<Word,Length>::rseq_t n, Length &r ) { // ES_NSINK
                                _write_integer_( codec, previous_n, r, RiceRuns<Word,Length>::IS_NEGATIVE );
                            },
                            []( RCodec<Word,Length> &codec, RiceRuns<Word,Length>::rseq_t &previous_n,const RiceRuns<Word,Length>::rseq_t n, Length &r ) { // ES_ERROR
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
                        static void _write_integer_( RCodec<Word,Length> &codec, RiceRuns<Word,Length>::rseq_t n, const Length r, const bool is_negative = false ) {
                            n = ( is_negative ) ? n * -1 : n;
                            if( r < RiceRuns<Word,Length>::RLE_THRESHOLD ) {
                                for (Length j = 0; j < r; ++j) {
                                    if( is_negative ) { codec.append(RiceRuns<Word,Length>::NEGATIVE_FLAG); }
                                    codec.append(n);
                                }
                                // std::cout << " Write ---> r<" << r << "> x (" << (is_negative ? " NEG<00>":"") << " n<" << n << "> )" << std::endl;
                            } else {
                                codec.append(RiceRuns<Word,Length>::REPETITION_FLAG);
                                if( is_negative ) { codec.append(RiceRuns<Word,Length>::NEGATIVE_FLAG); }
                                codec.append(n);
                                codec.append(r);

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
                        // static const ECase _get_case_( const RelativeSequence rs, std::size_t &i, const RiceRuns<Word,Length>::rseq_t previous_n, RiceRuns<Word,Length>::rseq_t &n ) {
                        static const ECase _get_case_( RelativeSequence<>& rs, const RiceRuns<Word,Length>::rseq_t previous_n, RiceRuns<Word,Length>::rseq_t &n ) {
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
                        void _init_( RelativeSequence<>& rs, RiceRuns<Word,Length>::rseq_t &n ) {
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
                        // EState next( const RelativeSequence rs, std::size_t &i, const RiceRuns<Word,Length>::rseq_t previous_n, RiceRuns<Word,Length>::rseq_t &n ) {
                        EState next( RelativeSequence<>& rs, const RiceRuns<Word,Length>::rseq_t previous_n, RiceRuns<Word,Length>::rseq_t &n ) {
                            // std::cout << "FSMEncoder/next> (1)" << std::endl;
                            if( this->is_init ) {
                              //  std::cout << "\t\t\tFSMEncoder/next> (1) previous_n = " << previous_n << "; n = " << n << std::endl;
                                // this->current_state = fsm[this->current_state][ FSMEncoder::_get_case_( rs, i, previous_n, n ) ];
                                this->current_state = fsm[this->current_state][ FSMEncoder::_get_case_( rs, previous_n, n ) ];
                                // std::cout << "FSMEncoder/next> (1.1.2)" << std::endl;
                            } else {
                              //  std::cout << "\t\t\tFSMEncoder/next> (2 - init) previous_n = " << previous_n << "; n = " << n << std::endl;
                                // this->_init_( rs, i, n );
                                this->_init_( rs, n );
                                // std::cout << "FSMEncoder/next> (1.2.2)" << std::endl;
                                this->is_init = true;
                                // std::cout << "FSMEncoder/next> (1.2.3)" << std::endl;
                            }
                          //  std::cout << "\t\t\tFSMEncoder/next> current_state = " << this->current_state << "; previous_n = " << previous_n << "; n = " << n << std::endl;
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
                        void run( RCodec<Word,Length> &codec, RiceRuns<Word,Length>::rseq_t &previous_n, const RiceRuns<Word,Length>::rseq_t n, Length &r ) {
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
                            this->is_init = false;
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
                        const std::array<std::function<void( RelativeSequence<>&, Word&, const Word )>,12> sfunction = {
                            []( RelativeSequence<> &rs, Word &previous_n,const Word n ) { // DS_Q0
                                // Empty
                            },
                            []( RelativeSequence<> &rs, Word &previous_n,const Word n ) { // DS_Q1
                                _write_integer_( rs, n, 1 );
                            },
                            []( RelativeSequence<> &rs, Word &previous_n,const Word n ) { // DS_Q2
                                // Empty
                            },
                            []( RelativeSequence<> &rs, Word &previous_n,const Word n ) { // DS_Q3
                                _write_integer_( rs, n, 1, RiceRuns<Word,Length>::IS_NEGATIVE );
                            },
                            []( RelativeSequence<> &rs, Word &previous_n,const Word n ) { // DS_04
                                // Empty
                            },
                            []( RelativeSequence<> &rs, Word &previous_n,const Word n ) { // DS_Q5
                                previous_n = n;
                            },
                            []( RelativeSequence<> &rs, Word &previous_n,const Word n ) { // DS_Q6
                                _write_integer_( rs, previous_n, n );
                            },
                            []( RelativeSequence<> &rs, Word &previous_n,const Word n ) { // DS_Q7
                                // Empty
                            },
                            []( RelativeSequence<> &rs, Word &previous_n,const Word n ) { // DS_Q8
                                previous_n = n;
                            },
                            []( RelativeSequence<> &rs, Word &previous_n,const Word n ) { // DS_Q9
                                _write_integer_( rs, previous_n, n, RiceRuns<Word,Length>::IS_NEGATIVE );
                            },
                            []( RelativeSequence<> &rs, Word &previous_n,const Word n ) { // DS_SINK
                                // Empty
                            },
                            []( RelativeSequence<> &rs, Word &previous_n,const Word n ) { // DS_ERROR
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
                        static void _write_integer_( RelativeSequence<>& rs, const Word n, const Length r, const bool is_negative = false ) {
                            RiceRuns<Word,Length>::rseq_t x = is_negative ? ((RiceRuns<Word,Length>::rseq_t)n) * -1 : n;
                            for (Length j = 0; j < r; ++j) {
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
                        static const DCase _get_case_( RCodec<Word,Length> &codec, Word &n ) {
                            if( !codec.has_more() ) {
                                return DCase::DC_EOS;
                            }else {
                                n = codec.next();
                                if( n != RiceRuns<Word,Length>::NEGATIVE_FLAG && n != RiceRuns<Word,Length>::REPETITION_FLAG ) {
                                    return DCase::DC_INT;
                                } else if( n == RiceRuns<Word,Length>::NEGATIVE_FLAG ) {
                                    return DCase::DC_NEGFLAG;
                                } else if( n == RiceRuns<Word,Length>::REPETITION_FLAG ) {
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
                        DState next( RCodec<Word,Length> &codec, Word &previous_n, Word &n ) {
                            if( !this->is_init ) {
                                this->_init_( n );
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
                        void run( RelativeSequence<>& rs, Word &previous_n, const Word n ) {
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

                        // FSMDecoder& operator=(const FSMDecoder &other_object) {
                        //     if (this != &other_object) {
                        //     // Copy the members of the other object to this object.
                        //     for (int i = 0; i < other_object.members.size(); i++) {
                        //         this->members[i] = other_object.members[i];
                        //     }
                        //     }
                        //     return *this;
                        // }
                };


                bool _encode_( ) {
                    // Encode:
                    rseq_t relative_v;
                    // std::cout << "\tRiceRuns/encode/do-while (4)" << std::endl;
                    this->encoding_fsm.next( this->encoding_buffer, this->encoding_previous_relative_n, relative_v );
                    // std::cout << "\tRiceRuns/encode/do-while (5)" << std::endl;
                  //  std::cout << "\t\tRiceRuns / _encode_ --- pre> encoding_previous_relative_n = " << this->encoding_previous_relative_n<< "; relative_v = " << relative_v << "; encoding_r = " << this->encoding_r << "; |encoding_buffer| = " << this->encoding_buffer->size() << std::endl;
                    if( this->encoding_fsm.is_error_state() ) { std::runtime_error("Encoding error state!"); }
                    this->encoding_fsm.run( this->codec, this->encoding_previous_relative_n, relative_v, this->encoding_r );
                  //  std::cout << "\t\tRiceRuns / _encode_ --- post> encoding_previous_relative_n = " << this->encoding_previous_relative_n<< "; relative_v = " << relative_v << "; encoding_r = " << this->encoding_r << "; |encoding_buffer| = " << this->encoding_buffer->size() << std::endl;
                    return !this->encoding_fsm.is_end_state();
                }

                // Attributes for relative-sequence traversal:
                RCodec<Word,Length>             codec;
                FSMDecoder                      decoding_fsm;
                FSMEncoder                      encoding_fsm;
                Word                            encoding_previous_n,
                                                decoding_previous_n, 
                                                decoding_n;
                Length                          encoding_r; // Repetition of current encoding value.
                RiceRuns<Word,Length>::rseq_t   encoding_relative_n,
                                                encoding_previous_relative_n,
                                                decoding_previous_relative_value;
                bool                            encoding_is_first,
                                                decoding_is_first;
                AbsoluteSequence<>              decoding_next_buffer;
                RelativeSequence<>              encoding_buffer;

            public:
                RiceRuns( typename RCodec<Word,Length>::BinarySequence bs ): 
                    codec( 
                        RCodec<Word,Length>(
                            bs
                        )
                    ),
                    encoding_is_first(true),
                    decoding_is_first(true),
                    decoding_next_buffer(adapter::get_instance<Word>( adapter::QueueAdapterType::Q_QUEUEADAPTER )),
                    encoding_buffer(adapter::get_instance<rseq_t>( adapter::QueueAdapterType::Q_QUEUEADAPTER ))
                { this->restart_encoded_sequence_iterator(false); }

                RiceRuns( const std::size_t k ): 
                    codec( 
                        RCodec<Word,Length>(
                            k
                        )
                    ),
                    encoding_is_first(true),
                    decoding_is_first(true),
                    decoding_next_buffer(adapter::get_instance<Word>( adapter::QueueAdapterType::Q_QUEUEADAPTER )),
                    encoding_buffer(adapter::get_instance<rseq_t>( adapter::QueueAdapterType::Q_QUEUEADAPTER ))
                { this->restart_encoded_sequence_iterator(false); }

                RiceRuns( Word *sequence, const Length sequence_length, const Length src_length, const std::size_t k ): 
                    codec( 
                        RCodec<Word,Length>(
                             sequence,
                             sequence_length,
                             src_length,
                             k 
                        )
                    ),
                    encoding_is_first(true),
                    decoding_is_first(true),
                    decoding_next_buffer(adapter::get_instance<Word>( adapter::QueueAdapterType::Q_QUEUEADAPTER )),
                    encoding_buffer(adapter::get_instance<rseq_t>( adapter::QueueAdapterType::Q_QUEUEADAPTER ))
                { this->restart_encoded_sequence_iterator(false); }

                // RiceRuns( const RiceRuns<Word,Length> &codec ) {
                //     this->codec = codec.codec;
                //     this->decoding_fsm = codec.decoding_fsm;
                //     this->previous_n = codec.previous_n;
                //     this->n = codec.n;
                //     this->previous_relative_value = codec.previous_relative_value;
                //     this->is_first = codec.is_first;
                //     this->next_buffer = codec.next_buffer;
                //     // this->restart_encoded_sequence_iterator(); 
                // }

                // ~RiceRuns() {
                //     delete this->codec;
                // }

                bool encode( Word v ) {
                    // Relativize:
                    RiceRuns<Word,Length>::rseq_t relative_v;
                    if( this->encoding_is_first ) {
                        relative_v = RiceRuns<Word,Length>::transform_rval( v );
                        this->encoding_is_first = false;
                    } else {
                        // this->encoding_buffer.push( this->encoding_previous_relative_n );
                        relative_v = RiceRuns<Word,Length>::_get_transformed_relative_sequence_( this->encoding_previous_n, v );
                    }
                  //  std::cout << "\tRiceRuns / encode> (*)" << std::endl;
                    this->encoding_buffer->push( relative_v );
                  //  std::cout << "\tRiceRuns / encode> v = " << v << "; relative_v (" << typeid(relative_v).name() << ") = " << relative_v << "; |encoding_buffer| = " << this->encoding_buffer->size() << "; encoding_previous_n = " << this->encoding_previous_n << std::endl;
                    this->encoding_previous_n = v;
                    
                    return !this->_encode_();
                }

                bool finish_encode( ) {
                    // this->encoding_buffer.pop();
                    return !this->_encode_();
                }

                /**
                 * @brief This function encodes a sequence of Word integers using Rice-runs. 
                 * 
                 * @param sequence 
                 * @param k 
                 * @return BinarySequence
                 */
                static typename RCodec<Word,Length>::BinarySequence encode( AbsoluteSequence<>& sequence, const std::size_t k ) {
                    // std::cout << "RiceRuns/encode (1)" << std::endl;
                    RCodec<Word,Length> codec( 
                        // sequence.size(),
                        k
                    );


                    // std::cout << "RiceRuns/encode (2)" << std::endl;

                    FSMEncoder fsm;

                    RelativeSequence<> relative_sequence = RiceRuns<Word,Length>::_get_transformed_relative_sequence_( sequence );

                    // samg::utils::print_queue<RiceRuns<Word,Length>::rseq_t>("Relative transformed queue |"+ std::to_string(relative_sequence.size()) +"|: ", relative_sequence);

                    // std::cout << "RiceRuns/encode (3)" << std::endl;

                    Length r; // Let r be a repetition counter of n.

                    RiceRuns<Word,Length>::rseq_t    previous_n, // Previous sequence value.
                                                     n;          // Next sequence value.

                    do {
                        // std::cout << "\tRiceRuns/encode/do-while (4)" << std::endl;
                        fsm.next( relative_sequence, previous_n, n);
                        // std::cout << "\tRiceRuns/encode/do-while (5)" << std::endl;
                        if( fsm.is_error_state() ) { break; }
                        fsm.run( codec, previous_n, n, r );
                        // std::cout << "\tRiceRuns/encode/do-while (6)" << std::endl;
                    } while( !fsm.is_end_state() );

                    // std::cout << "RiceRuns/encode> MAX_N_BITS = " << codec.MAX_N_BITS << "[b] / " << ( sizeof(Word) * 8 ) << std::endl;

                    return codec.get_binary_sequence();
                }

                /**
                 * @brief This function decodes an encoded sequence of Word integers using Rice-runs. 
                 * 
                 * @param bs 
                 * @return AbsoluteSequence 
                 */
                static AbsoluteSequence<> decode( typename RCodec<Word,Length>::BinarySequence bs  ) {
                    return RiceRuns<Word,Length>::decode( bs.sequence, bs.length, bs.src_length, bs.k );
                }

                /**
                 * @brief This function decodes an encoded sequence of Word integers using Rice-runs. 
                 * 
                 * @param sequence 
                 * @param sequence_length 
                 * @param src_length 
                 * @param k 
                 * @return AbsoluteSequence 
                 */
                static AbsoluteSequence<> decode( Word *sequence, const Length sequence_length, const Length src_length, const std::size_t k  ) {
                    RCodec<Word,Length> codec(
                        sequence,
                        sequence_length, 
                        src_length,
                        k
                    );

                    FSMDecoder fsm;

                    Word    previous_n, 
                            n;

                    RelativeSequence<> relative_sequence = adapter::get_instance<RiceRuns<Word,Length>::rseq_t>( adapter::QueueAdapterType::Q_QUEUEADAPTER );

                    do { 
                        fsm.next( codec, previous_n, n);
                        if( fsm.is_error_state() ) { break; }
                        fsm.run( relative_sequence, previous_n, n );
                    }while( !fsm.is_end_state() );

                    return RiceRuns<Word,Length>::_get_transformed_absolute_sequence_( relative_sequence );
                }

                /**
                 * @brief This function returns the Golomb-Rice encoded bitmap.
                 * 
                 * @return BinarySequence
                 */
                typename RCodec<Word,Length>::BinarySequence get_encoded_sequence() const {
                    return this->codec.get_binary_sequence();
                }

                /**
                 * @brief This function sets the internal encoded sequence.
                 * 
                 * @param bs 
                 */
                void set_encoded_sequence( typename RCodec<Word,Length>::BinarySequence bs ) {
                    this->set_encoded_sequence( bs.sequence, bs.length, bs.src_length, bs.k );
                }

                /**
                 * @brief This function sets the internal encoded sequence.
                 * 
                 * @param sequence 
                 * @param sequence_length 
                 * @param src_length 
                 * @param k 
                 */
                void set_encoded_sequence( Word *sequence, const Length sequence_length, const Length src_length, const std::size_t k ) {
                    this->codec = RCodec<Word,Length>(
                        sequence, 
                        sequence_length, 
                        src_length, 
                        k
                    );
                    this->restart_encoded_sequence_iterator();
                }

                /**
                 * @brief This function allows retrieving one by one positive integers from the Rice-runs encoded internal bitmap. 
                 * 
                 * @return const Word 
                 */
                const Word next() {
                    if( this->decoding_next_buffer->empty() ) {
                        std::uint8_t s;
                        RelativeSequence<> relative_sequence  = adapter::get_instance<RiceRuns<Word,Length>::rseq_t>( adapter::QueueAdapterType::Q_QUEUEADAPTER );
                        do { 
                            s = this->decoding_fsm.next( this->codec, this->decoding_previous_n, this->decoding_n);
                            if( decoding_fsm.is_error_state() ) { break; }
                            decoding_fsm.run( relative_sequence, this->decoding_previous_n, this->decoding_n );
                        }while( !decoding_fsm.is_output_state() );

                        // If it is not the first time executing `next`, then insert the previous relativized absolute value to the beginning of the just retrieved relative sequence:
                        if( !this->decoding_is_first ) {
                            RelativeSequence<> tmp = adapter::get_instance<RiceRuns<Word,Length>::rseq_t>( adapter::QueueAdapterType::Q_QUEUEADAPTER );
                            tmp->push( this->decoding_previous_relative_value );
                            while( !relative_sequence->empty() ) {
                                tmp->push( relative_sequence->front() );
                                relative_sequence->pop();
                            }
                            // std::swap( tmp, relative_sequence );
                            adapter::QueueAdapter<RiceRuns<Word,Length>::rseq_t>::swap( *tmp, *relative_sequence );
                        }

                        // Retrieve a transformed sequence from the relative one:
                        this->decoding_next_buffer.reset( RiceRuns<Word,Length>::_get_transformed_absolute_sequence_( relative_sequence ).release() );
                        
                        // Back up the last relativized (transformed) absolute value:
                        this->decoding_previous_relative_value = RiceRuns<Word,Length>::transform_rval( this->decoding_next_buffer->back() );
                        
                        // It it is not the first time executing `next`, then remove the previously added last relativized absolute value:
                        // (it was already used to compute the next absolute value(s))
                        if( !this->decoding_is_first ){
                            this->decoding_next_buffer->pop();
                        }
                        
                        this->decoding_is_first = false;
                    }

                    Word v = this->decoding_next_buffer->front();
                    this->decoding_next_buffer->pop();
                    return v;
                }

                /**
                 * @brief This function restarts the traversal of the encoded sequence.
                 * 
                 */
                void restart_encoded_sequence_iterator( bool reset_buffers = true ) {
                    this->codec.restart();
                    this->decoding_fsm.restart();
                    this->encoding_fsm.restart();

                    if( reset_buffers ) {
                        if( this->decoding_next_buffer ) { 
                            this->decoding_next_buffer.reset();
                        }
                        this->decoding_next_buffer = adapter::get_instance<Word>( adapter::QueueAdapterType::Q_QUEUEADAPTER );

                        if( this->encoding_buffer ){
                            this->encoding_buffer.reset();
                        }
                        this->encoding_buffer = adapter::get_instance<rseq_t>( adapter::QueueAdapterType::Q_QUEUEADAPTER );
                    }

                    this->encoding_previous_n = 0;
                    this->decoding_previous_n = 0;
                    this->decoding_n = 0;
                    this->encoding_r = 0;
                    this->encoding_relative_n = 0;
                    this->encoding_previous_relative_n = 0;
                    this->decoding_previous_relative_value = 0;
                }

                /**
                 * @brief This function returns whether the internal encoded sequence has more codewords to decode or not.
                 * 
                 * @return true 
                 * @return false 
                 */
                bool has_more() {
                    // return this->codec.has_more();
                    return  this->codec.has_more() || 
                            !(this->decoding_next_buffer.empty());
                }

        };
    }
}