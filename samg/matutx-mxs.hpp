#pragma once
#include <samg/commons.hpp>
#include <samg/mmm-interface.hpp>

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

namespace samg {
    namespace matutx {
        typedef std::uint8_t Word;
        namespace wrapper {
            namespace serializer {
                class OfflineWordReaderWrapper : public samg::serialization::OfflineWordReader<samg::matutx::Word> {
                    private:
                        std::size_t max_value;
                        template<typename T> const T _read_integer_( ) {
                            if ( this->max_value <= std::numeric_limits<std::uint8_t>::max() ) {
                                return static_cast<T>( samg::serialization::OfflineWordReader<samg::matutx::Word>::next<std::uint8_t>( ) );
                            } else if ( this->max_value <= std::numeric_limits<std::uint16_t>::max() ) {
                                return static_cast<T>( samg::serialization::OfflineWordReader<samg::matutx::Word>::next<std::uint16_t>( ) );
                            } else if ( this->max_value <= std::numeric_limits<std::uint32_t>::max() ) {
                                return static_cast<T>( samg::serialization::OfflineWordReader<samg::matutx::Word>::next<std::uint32_t>( ) );
                            } else  if ( this->max_value <= std::numeric_limits<std::uint64_t>::max() ) {
                                return static_cast<T>( samg::serialization::OfflineWordReader<samg::matutx::Word>::next<std::uint64_t>( ) );
                            } else {
                                throw std::runtime_error("OfflineWordReaderWrapper/_read_integer_> Unrecognized datatype!");
                            }
                        }

                        template<typename T> const std::vector<T> _read_integer_vector_( const std::size_t length ) {
                            std::vector<T> ans = std::vector<T>( length );
                            for (std::size_t i = 0; i < length; i++) {
                                ans[ i ] = this->_read_integer_<T>( );
                            }
                            return ans;
                            // if (this->max_value <= std::numeric_limits<std::uint8_t>::max()) {
                            //     samg::serialization::OfflineWordWriter<samg::matutx::Word>::add_values<std::uint8_t>( static_cast<std::vector<std::uint8_t>>( values ) );
                            // } else if (this->max_value <= std::numeric_limits<std::uint16_t>::max()) {
                            //     samg::serialization::OfflineWordWriter<samg::matutx::Word>::add_values<std::uint16_t>( static_cast<std::vector<std::uint16_t>>( values ) );
                            // } else if (this->max_value <= std::numeric_limits<std::uint32_t>::max()) {
                            //     samg::serialization::OfflineWordWriter<samg::matutx::Word>::add_values<std::uint32_t>( static_cast<std::vector<std::uint32_t>>( values ) );
                            // } else  if (this->max_value <= std::numeric_limits<std::uint64_t>::max()) {
                            //     samg::serialization::OfflineWordWriter<samg::matutx::Word>::add_values<std::uint64_t>( static_cast<std::vector<std::uint64_t>>( values ) );
                            // } else{
                            //     throw std::runtime_error("OfflineWordWriterWrapper - Unrecognized datatype!");
                            // }
                        }
                    public:
                        OfflineWordReaderWrapper( const std::string file_name, const std::size_t max_value = 0ZU ) :
                        samg::serialization::OfflineWordReader<samg::matutx::Word>( file_name ),
                        max_value(max_value) {

                        }

                        template<typename TypeTrg> const TypeTrg next_metadata() {
                            return samg::serialization::OfflineWordReader<samg::matutx::Word>::next<TypeTrg>( );
                        }

                        template<typename TypeTrg> const TypeTrg next() {
                            return  this->_read_integer_<TypeTrg>( );
                        }

                        template<typename TypeTrg> const std::vector<TypeTrg> next_metadata_vector( const std::size_t length ) {
                            return samg::serialization::OfflineWordReader<samg::matutx::Word>::next<TypeTrg>( length );
                        }

                        template<typename TypeTrg> const std::vector<TypeTrg> next_vector( const std::size_t length ) {
                            return  this->_read_integer_vector_<TypeTrg>( length );
                        }

                        void set_max_value( const std::size_t max_value ) {
                            this->max_value = max_value;
                        }

                        const std::uint8_t get_bytes( ) {
                            if ( this->max_value <= std::numeric_limits<std::uint8_t>::max() ) {
                                return sizeof(std::uint8_t); //1U;
                            } else if ( this->max_value <= std::numeric_limits<std::uint16_t>::max() ) {
                                return sizeof(std::uint16_t);//2U;
                            } else if ( this->max_value <= std::numeric_limits<std::uint32_t>::max() ) {
                                return sizeof(std::uint32_t);//4U;
                            } else  if ( this->max_value <= std::numeric_limits<std::uint64_t>::max() ) {
                                return sizeof(std::uint64_t);//8U;
                            } else {
                                throw std::runtime_error("OfflineWordReaderWrapper/get_bytes> Unrecognized datatype!");
                            }
                        }

                };

                class OfflineWordWriterWrapper : public samg::serialization::OfflineWordWriter<samg::matutx::Word> {
                    private:
                        std::size_t max_value;
                        template<typename T> void _add_integer_( const T &value ) {
                            if (this->max_value <= std::numeric_limits<std::uint8_t>::max()) {
                                samg::serialization::OfflineWordWriter<samg::matutx::Word>::add_value<std::uint8_t>( static_cast<std::uint8_t>( value ) );
                            } else if (this->max_value <= std::numeric_limits<std::uint16_t>::max()) {
                                samg::serialization::OfflineWordWriter<samg::matutx::Word>::add_value<std::uint16_t>( static_cast<std::uint16_t>( value ) );
                            } else if (this->max_value <= std::numeric_limits<std::uint32_t>::max()) {
                                samg::serialization::OfflineWordWriter<samg::matutx::Word>::add_value<std::uint32_t>( static_cast<std::uint32_t>( value ) );
                            } else  if (this->max_value <= std::numeric_limits<std::uint64_t>::max()) {
                                samg::serialization::OfflineWordWriter<samg::matutx::Word>::add_value<std::uint64_t>( static_cast<std::uint64_t>( value ) );
                            } else{
                                throw std::runtime_error("OfflineWordWriterWrapper/_add_integer_>  Unrecognized datatype!");
                            }
                        }

                        template<typename T> void _add_integer_vector_( const std::vector<T> &values ) {
                            for (T v : values) {
                                this->_add_integer_<T>( v );
                            }
                            // if (this->max_value <= std::numeric_limits<std::uint8_t>::max()) {
                            //     samg::serialization::OfflineWordWriter<samg::matutx::Word>::add_values<std::uint8_t>( static_cast<std::vector<std::uint8_t>>( values ) );
                            // } else if (this->max_value <= std::numeric_limits<std::uint16_t>::max()) {
                            //     samg::serialization::OfflineWordWriter<samg::matutx::Word>::add_values<std::uint16_t>( static_cast<std::vector<std::uint16_t>>( values ) );
                            // } else if (this->max_value <= std::numeric_limits<std::uint32_t>::max()) {
                            //     samg::serialization::OfflineWordWriter<samg::matutx::Word>::add_values<std::uint32_t>( static_cast<std::vector<std::uint32_t>>( values ) );
                            // } else  if (this->max_value <= std::numeric_limits<std::uint64_t>::max()) {
                            //     samg::serialization::OfflineWordWriter<samg::matutx::Word>::add_values<std::uint64_t>( static_cast<std::vector<std::uint64_t>>( values ) );
                            // } else{
                            //     throw std::runtime_error("OfflineWordWriterWrapper - Unrecognized datatype!");
                            // }
                        }
                    public:
                        
                        OfflineWordWriterWrapper( const std::string file_name, const std::size_t max_value ) :
                        samg::serialization::OfflineWordWriter<samg::matutx::Word>( file_name ),
                        max_value(max_value) {

                        }

                        template<typename TypeSrc> void add_metadata(const TypeSrc v) {
                            samg::serialization::OfflineWordWriter<samg::matutx::Word>::add_value<TypeSrc>( v );
                        }

                        template<typename TypeSrc> void add_metadata_vector( const std::vector<TypeSrc> &V ) {
                            samg::serialization::OfflineWordWriter<samg::matutx::Word>::add_values<TypeSrc>( V );
                        }

                        template<typename TypeSrc> void add_value(const TypeSrc v) {
                            this->_add_integer_<TypeSrc>( v );
                        }
                        
                        template<typename TypeSrc> void add_values(const std::vector<TypeSrc>& V) {
                            this->_add_integer_vector_<TypeSrc>( V );
                        }

                        void set_max_value( const std::size_t max_value ) {
                            this->max_value = max_value;
                        }
                };
            }
        }
        namespace reader {
            class MXSReader : public Reader {
                private:
                    // std::size_t MAX_VALUE, MAX_INDEX_VALUE;
                    // std::unique_ptr<samg::serialization::OfflineWordReader<samg::matutx::Word>> serializer;
                    std::unique_ptr<samg::matutx::wrapper::serializer::OfflineWordReaderWrapper> serializer;
                    std::vector<std::uint64_t> maxs;
                    std::uint64_t e, s, c;
                    std::float_t d, actual_d, cderr;
                    std::string dist;
                    std::size_t current_entry,  j/*, pp*/, ip;
                    // Let P be a vector of positive integers. --- Replaced by `this->serializer`. 
                    std::vector<std::uint64_t> Pi;// Let Pi be an array of n cells to store the latests added values to P.
                    std::vector<std::size_t> I; // Let I be a vector of positive integers.
                    std::vector<std::size_t> Ii;// Let Ii be an array of n cells to store pointers to I, initially as Ii = [0,1,2,...,n-1].

                    std::vector<std::uint64_t> _gen_coord_( std::vector<std::uint64_t> &Pi ) {
                        std::vector<std::uint64_t> c(Pi); // Let c be an array of |Pi| cells.
                        // for( std::size_t i = 0; i < Pi.size(); i++ ){
                        //     c[ i ] = Pi[ i ];
                        // }
                        return c;
                    }

                public:
                    MXSReader(const std::string input_file_name):
                        Reader(input_file_name),
                        current_entry(0)
                    {
                        // this->serializer = std::make_unique<samg::serialization::OfflineWordReader<samg::matutx::Word>>( input_file_name );
                        this->serializer = std::make_unique<samg::matutx::wrapper::serializer::OfflineWordReaderWrapper>( input_file_name );
                        // Read TAIL:
                        this->serializer->seek( -sizeof( std::size_t ) * 3, std::ios::end ); //  ( this->serializer->tell() - sizeof( std::size_t ) )
                        std::size_t tail_length = this->serializer->next_metadata<std::size_t>(),
                                    MAX_VALUE = this->serializer->next_metadata<std::size_t>(),
                                    MAX_INDEX_VALUE = this->serializer->next_metadata<std::size_t>();
                        this->serializer->set_max_value( MAX_INDEX_VALUE );
                        this->serializer->seek( -( ( tail_length * this->serializer->get_bytes( ) ) + ( 3 * sizeof( std::size_t ) ) ), std::ios::end );
                        this->I = this->serializer->next_vector<std::size_t>( tail_length );

                        /***************************************************************/
                        samg::utils::print_vector<std::size_t>("I:\n",this->I,true,"\n");
                        /***************************************************************/

                        // Read HEADER:
                        this->serializer->seek( 0ZU, std::ios::beg );
                        this->s = this->serializer->next_metadata<std::uint64_t>();
                        this->d = (( std::float_t ) this->serializer->next_metadata<std::size_t>( )) / 1000000.0f;
                        this->actual_d = (( std::float_t ) this->serializer->next_metadata<std::size_t>() ) / 1000000.0f;
                        this->dist = this->serializer->next_string();
                        this->c = this->serializer->next_metadata<std::uint64_t>();
                        this->cderr = (( std::float_t ) this->serializer->next_metadata<std::size_t>() ) / 1000000.0f;
                        
                        std::size_t n = this->serializer->next_metadata<std::size_t>();
                        this->maxs = this->serializer->next_metadata_vector<std::size_t>( n );

                        this->e = this->serializer->next_metadata<std::uint64_t>();
                        
                        for (size_t i = 0; i < maxs.size(); i++) {
                            this->Ii.push_back( i );
                            this->Pi.push_back( this->serializer->next<std::uint64_t>() );
                        }
                        this->j /*= this->pp*/ = this->ip = maxs.size();
                        this->j--;

                        this->serializer->set_max_value( MAX_VALUE );
                        
                    }

                    const std::size_t get_number_of_dimensions() const override {
                        return this->maxs.size();
                    }
                    const std::vector<std::uint64_t> get_max_per_dimension() const override {
                        return this->maxs;
                    }
                    const std::uint64_t get_number_of_entries() const override {
                        return this->e;
                    }
                    const std::uint64_t get_matrix_side_size() const override {
                        return this->s;
                    }
                    const std::uint64_t get_matrix_size() const override {
                        return std::pow( s, maxs.size() );
                    }
                    const std::float_t get_matrix_expected_density() const override {
                        return this->d;
                    }
                    const std::float_t get_matrix_actual_density() const override {
                        return this->actual_d;
                    }
                    const std::string get_matrix_distribution() const override {
                        return this->dist;
                    }
                    const std::float_t get_gauss_mu() const override {
                        return 0.0f;
                    }
                    const std::float_t get_gauss_sigma() const override {
                        return 0.0f;
                    }
                    const std::uint64_t get_clustering() const override {
                        return this->c;
                    }
                    const std::float_t get_clustering_distance_error() const override {
                        return this->cderr;
                    }

                    const bool has_next() override {
                        return this->current_entry < e;
                    }
                    
                    std::vector<std::uint64_t> next() override {
                        if( !this->has_next() ){
                            throw std::runtime_error("No more entries!");
                        }
                        std::vector<std::uint64_t> C = this->_gen_coord_( this->Pi );
                        this->I[ this->Ii[ this->j ] ]--;

                        if( this->I[ this->Ii[ this->j ] ] > 0ZU ){
                            this->Pi[ this->j ] = this->serializer->next<std::uint64_t>();
                        } else {
                            // Checking backward:
                            while( this->I[ this->Ii[ this->j ] ] == 0ZU ){
                                this->j--;
                                this->I[ this->Ii[ this->j ] ]--;
                                if( this->j == 0 && this->I[ this->Ii[ this->j ] ] == 0ZU ){
                                    this->current_entry++;
                                    return C;// Decoding completed!
                                }
                            }
                            
                            // Checking forward:
                            while( this->j < this->maxs.size() ) {
                                this->Pi[ this->j++ ] = this->serializer->next<std::uint64_t>();
                                // this->j++;
                                if( this->j < this->maxs.size() ) {
                                    this->Ii[ this->j ] = this->ip;
                                    this->ip++;
                                }
                            }
                            this->j--;
                        }
                        this->current_entry++;
                        return C;
                    }
            };
        }
        namespace writer {
            class MXSWriter : public Writer {
                /**
                 * @brief MXS Format:
                 * 
                 * HEADER: s s^n d actual_d dist c cderr n max1 max2 max3 ... maxn e
                 * 
                 * PAYLOAD: l{n} v{n} l{n-1} v{n-1} l{n-2} v{n-3} l{n-4} ... v{2} l{1} v1{1} v2{1} v3{1} ... vm{1} ...
                 * 
                 * TAIL: I0 I1 I2 ... Im |I|<in bytes>
                 */
                private:
                    const std::size_t MAX_VALUE;
                    // std::unique_ptr<samg::serialization::OfflineWordWriter<samg::matutx::Word>> serializer;
                    std::unique_ptr<samg::matutx::wrapper::serializer::OfflineWordWriterWrapper> serializer;
                    bool is_open;
                    std::size_t p, MAX_INDEX_VALUE;
                    // Let P be a vector of positive integers. --- Replaced by `this->serializer`. 
                    std::vector<std::uint64_t> Pi;// Let Pi be an array of n cells to store the latests added values to P.
                    std::vector<std::size_t> I; // Let I be a vector of positive integers.
                    std::vector<std::size_t> Ii;// Let Ii be an array of n cells to store pointers to I, initially as Ii = [0,1,2,...,n-1].
                    bool first;

                public:
                    MXSWriter(const std::string output_file_name,
                        // const std::size_t n,
                        const std::vector<std::uint64_t> maxs,
                        const std::uint64_t e,
                        const std::uint64_t s,
                        const std::float_t d,
                        const std::float_t actual_d,
                        const std::string dist = "unknonwn",
                        const std::uint64_t c = 0ULL,
                        const std::float_t cderr = 0.0f
                    ):
                        Writer(
                            output_file_name,
                            // n,
                            maxs,
                            e,
                            s,
                            d,
                            actual_d,
                            dist,
                            c,
                            cderr
                        ),
                        MAX_VALUE ( maxs[0] ),
                        MAX_INDEX_VALUE ( 1ZU ),
                        p( 0ZU ),
                        first( true ),
                        Pi( std::vector<std::uint64_t>( maxs.size() ) ),
                        I( std::vector<std::size_t>() ),
                        Ii( std::vector<std::size_t>( maxs.size() ) )
                    {
                        // this->serializer = std::make_unique<samg::serialization::OfflineWordWriter<samg::matutx::Word>>( output_file_name );
                        this->serializer = std::make_unique<samg::matutx::wrapper::serializer::OfflineWordWriterWrapper>( output_file_name, this->MAX_VALUE );
                        // Writing HEADER:
                        this->serializer->add_metadata<std::uint64_t>( s );
                        // this->serializer->add_value<std::uint64_t>( std::pow( s , maxs.size() ));
                        this->serializer->add_metadata<std::size_t>( ( std::size_t ) (d * 1000000ZU) );
                        this->serializer->add_metadata<std::size_t>( ( std::size_t ) (actual_d * 1000000ZU) );
                        this->serializer->add_string( dist );
                        this->serializer->add_metadata<std::uint64_t>( c );
                        this->serializer->add_metadata<std::size_t>( ( std::size_t ) (cderr * 1000000ZU) );
                        this->serializer->add_metadata<std::size_t>( maxs.size() );
                        this->serializer->add_metadata_vector<std::uint64_t>( maxs );
                        this->serializer->add_metadata<std::uint64_t>( e );
                        this->is_open = true;
                    }
                    void add_entry( std::vector<std::uint64_t> entry ) override {
                        if( this->is_open ) {
                            const std::size_t n = entry.size();
                            if( this->first ){
                                for(std::size_t i = 0; i < n; i++) {
                                    this->serializer->add_value<std::uint64_t>( entry[ i ] );
                                    // this->Pi.push_back( entry[ i ] );
                                    this->Pi[ i ] = entry[ i ];
                                    this->I.push_back( 1ZU );
                                    // this->Ii.push_back( i );
                                    this->Ii[ i ] = i;
                                }
                                this->first = false;
                            } else {
                                std::size_t j = 0ZU;
                                while( j < n ){
                                    if( entry[ j ] != Pi[ j ] ) {
                                        break;
                                    }
                                    j++;
                                }
                                while( j < n ) {
                                    this->serializer->add_value<std::uint64_t>( entry[ j ] );
                                    this->Pi[ j ] = entry[ j ];
                                    this->I[ Ii[ j ] ]++;
                                    if( this->I[ Ii[ j ] ] > MAX_INDEX_VALUE ) { MAX_INDEX_VALUE = this->I[ Ii[ j ] ];}
                                    j++;
                                    if( j < n ) {
                                        this->I.push_back( 0ZU );
                                        this->Ii[ j ] = this->I.size() - 1;
                                    } 
                                }
                            }
                        }
                    }

                    void close() override {
                        this->serializer->set_max_value( this->MAX_INDEX_VALUE );
                        this->serializer->add_values<std::size_t>( this->I ); // Adding index.
                        
                        /***************************************************************/
                        samg::utils::print_vector<std::size_t>("I:\n",this->I,true,"\n");
                        /***************************************************************/
                        
                        this->serializer->add_metadata<std::size_t>( this->I.size() ); // Adding index length in elements (words).
                        this->serializer->add_metadata<std::size_t>( this->MAX_VALUE );// Adding sequence max value among all dimensions.
                        this->serializer->add_metadata<std::size_t>( this->MAX_INDEX_VALUE );// Adding index max value. 
                        this->serializer->close();
                        this->serializer.reset();
                        this->is_open = false;
                    }
            };
        }
    }
}