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
        namespace reader {
            class MXSReader : public Reader {
                private:
                    std::unique_ptr<samg::serialization::OfflineWordReader<Word>> serializer;
                    std::vector<std::uint64_t> maxs;
                    std::uint64_t e, s, c;
                    std::float_t d, actual_d, cderr;
                    std::string dist;
                    std::size_t current_entry,  j, pp, ip;
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
                        this->serializer = std::make_unique<samg::serialization::OfflineWordReader<Word>>( input_file_name );
                        // Read TAIL:
                        this->serializer->seek( -sizeof( std::size_t ), std::ios::end ); //  ( this->serializer->tell() - sizeof( std::size_t ) )
                        std::size_t tail_length = this->serializer->next<std::size_t>();
                        this->serializer->seek( -( ( tail_length + 1 ) * sizeof( std::size_t ) ), std::ios::end );
                        this->I = this->serializer->next<std::size_t>( tail_length );
                        // Read HEADER:
                        this->serializer->seek( 0, std::ios::beg );
                        this->s = this->serializer->next<std::uint64_t>();
                        this->d = (( std::float_t ) this->serializer->next<std::size_t>( )) / 1000000.0f;
                        this->actual_d = (( std::float_t ) this->serializer->next<std::size_t>() ) / 1000000.0f;
                        this->dist = this->serializer->next_string();
                        this->c = this->serializer->next<std::uint64_t>();
                        this->cderr = (( std::float_t ) this->serializer->next<std::size_t>() ) / 1000000.0f;
                        
                        std::size_t n = this->serializer->next<std::size_t>();
                        this->maxs = this->serializer->next<std::size_t>( n );

                        this->e = this->serializer->next<std::uint64_t>();
                        
                        for (size_t i = 0; i < maxs.size(); i++) {
                            this->Ii.push_back( i );
                            this->Pi.push_back( this->serializer->next<std::uint64_t>() );
                        }
                        this->j = this->pp = this->ip = maxs.size();
                        this->j--;
                        
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
                                this->Pi[ this->j ] = this->serializer->next<std::uint64_t>();
                                this->j++;
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
            class Writer {
                private:
                    const std::string output_file_name;
                    // const std::size_t n;
                    const std::vector<std::uint64_t> maxs;
                    const std::uint64_t e, s, c;
                    const std::float_t d, actual_d, cderr;
                    const std::string dist;
                public:
                    Writer(
                        const std::string output_file_name,
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
                        output_file_name(output_file_name),
                        // n(n),
                        maxs(maxs),
                        e(e),
                        s(s),
                        d(d),
                        actual_d(actual_d),
                        dist(dist),
                        c(c),
                        cderr(cderr)
                    {}
                    const std::string get_output_file_name() const {
                        return this->output_file_name;
                    }
                    
                    virtual void add_entry(std::vector<std::uint64_t> entry) = 0;
                    virtual void close() = 0;
            };

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
                    std::unique_ptr<samg::serialization::OfflineWordWriter<Word>> serializer;
                    bool is_open;
                    std::size_t p;
                    // Let P be a vector of positive integers. --- Replaced by `this->serializer`. 
                    std::vector<std::uint64_t> Pi;// Let Pi be an array of n cells to store the latests added values to P.
                    std::vector<std::size_t> I; // Let I be a vector of positive integers.
                    std::vector<std::size_t> Ii;// Let Ii be an array of n cells to store pointers to I, initially as Ii = [0,1,2,...,n-1].
                    bool init;

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
                        p( 0ZU ),
                        init( true ),
                        Pi( std::vector<std::uint64_t>( maxs.size() ) ),
                        I( std::vector<std::size_t>() ),
                        Ii( std::vector<std::size_t>( maxs.size() ) )
                    {
                        this->serializer = std::make_unique<samg::serialization::OfflineWordWriter<Word>>( output_file_name );
                        // Writing HEADER:
                        this->serializer->add_value<std::uint64_t>( s );
                        // this->serializer->add_value<std::uint64_t>( std::pow( s , maxs.size() ));
                        this->serializer->add_value<std::size_t>( ( std::size_t ) (d * 1000000ZU) );
                        this->serializer->add_value<std::size_t>( ( std::size_t ) (actual_d * 1000000ZU) );
                        this->serializer->add_string( dist );
                        this->serializer->add_value<std::uint64_t>( c );
                        this->serializer->add_value<std::size_t>( ( std::size_t ) (cderr * 1000000ZU) );
                        this->serializer->add_value<std::size_t>( maxs.size() );
                        this->serializer->add_values<std::uint64_t>( maxs );
                        this->serializer->add_value<std::uint64_t>( e );
                        this->is_open = true;
                    }
                    void add_entry(std::vector<std::uint64_t> entry) override {
                        if( this->is_open ) {
                            const std::size_t n = entry.size();
                            if( this->init ){
                                for(std::size_t i = 0; i < n; i++) {
                                    this->serializer->add_value<std::uint64_t>( entry[ i ] );
                                    this->Pi.push_back( entry[ i ] );
                                    this->I.push_back( 1ZU );
                                    this->Ii.push_back( i );
                                }
                                this->init = false;
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
                        this->serializer->add_values<std::size_t>( this->I ); // Adding index.
                        this->serializer->add_value<std::size_t>( this->I.size() ); // Adding index length in elements (words).
                        this->serializer->close();
                        this->serializer.reset();
                        this->is_open = false;
                    }
            };
        }
    }
}