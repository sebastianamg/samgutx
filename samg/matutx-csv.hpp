#pragma once
#include <samg/commons.hpp>
#include <samg/mmm-interface.hpp>
#include <rapidcsv/rapidcsv.h>

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
        namespace reader {
            class CSVReader : public Reader {
                private:
                    rapidcsv::Document doc;
                    std::vector<std::size_t> selected_columns;
                    std::size_t index;
                    std::uint64_t global_max;

                    // std::size_t n;
                    // std::size_t b;
                    // std::size_t d;
                    // std::uint64_t initial_M;
                    // std::size_t bd;
                    samg::utils::ZValueConverter z_converter;

                    const std::uint64_t get_global_max( ) const {
                        // if( this->index < this->doc.GetRowCount() ) {
                        //     throw std::runtime_error("Unkown glogal maximum.");
                        // }
                        return this->global_max;
                    }

                public:
                    CSVReader( const std::string file_name, const char separator = ',' , const std::vector<std::size_t> selected_columns = std::vector<std::size_t>(), const std::int8_t first_row=-1, const std::int8_t first_column=-1, const std::uint64_t k = 2ULL ):
                        Reader ( file_name ),
                        doc ( rapidcsv::Document(file_name, rapidcsv::LabelParams( first_column , first_row ), rapidcsv::SeparatorParams( separator )) ),
                        selected_columns ( selected_columns ),
                        index ( 0ZU ),
                        global_max ( 0ULL )
                    {
                        
                        // Setting selected columns:   
                        if( this->selected_columns.size() == 0 ) {
                            this->selected_columns = std::vector<std::size_t>();
                            for (std::size_t i = 0; i < this->doc.GetColumnCount(); i++) {
                                this->selected_columns.push_back(i);
                            }    
                        }

                        // Analyzing selected columns looking for the global max:
                        this->global_max = 0ULL;
                        for (std::size_t i = 0; i < this->doc.GetRowCount(); i++) {
                            std::vector<std::uint64_t> tmp = doc.GetRow<std::uint64_t>( i );
                            for (std::size_t j = 0; j < this->selected_columns.size(); j++ ){
                                // Update global max:
                                if( tmp[ this->selected_columns[ j ] ] > this->global_max ) { 
                                    this->global_max = tmp[ this->selected_columns[ j ] ];
                                }
                            }
                        }
                        std::cout << "MAX: " << this->global_max << "; selected columns: " << this->selected_columns.size() << std::endl;
                        this->z_converter = samg::utils::ZValueConverter( this->get_matrix_side_size(), this->get_number_of_dimensions(), k );
                        // this->b = samg::utils::get_required_bits( k );//(k == 1UL ? 0UL : std::bit_width(k - 1UL)), // Number of bits per coordinate component considered for Z-ordering.
                        // this->d = samg::utils::get_required_digits( this->get_matrix_side_size(), b );//(s == 0) ? 0 : static_cast<std::size_t>(std::ceil(std::log2(s) / static_cast<double>(b))), // Number of digits to encode a component considered for Z-ordering.
                        // this->n = this->get_number_of_dimensions(); // Number of dimensions of the matrix.
                        // this->initial_M = samg::utils::get_initial_mask( b );
                        // this->bd = b * d;
                    }
                    // ~CSVReader() {   
                    // }

                    const std::size_t get_number_of_dimensions() const override {
                        return this->selected_columns.size();
                    }
                    const std::vector<std::uint64_t> get_max_per_dimension() const override {
                        return std::vector<std::uint64_t>( this->get_number_of_dimensions(), this->get_global_max() );
                    }
                    const std::uint64_t get_number_of_entries() const override {
                        return this->doc.GetRowCount();
                    }
                    const bool has_next() override {
                        return this->index < this->doc.GetRowCount();
                    }
                    const std::uint64_t get_matrix_side_size() const override {
                        return this->get_global_max() + 1;
                    }
                    const std::uint64_t get_matrix_size() const override {
                        return std::pow( this->get_matrix_side_size(), this->get_number_of_dimensions() );
                    }
                    const std::float_t get_matrix_expected_density() const override {
                        return ( (std::float_t) this->get_number_of_entries() ) / ( (std::float_t) this->get_matrix_size() );
                    }
                    const std::float_t get_matrix_actual_density() const override {
                        return this->get_matrix_expected_density();
                    }
                    const std::string get_matrix_distribution() const override {
                        return std::string( "Unknown" );
                    }
                    const std::float_t get_gauss_mu() const override {
                        return 0.0F;
                    }
                    const std::float_t get_gauss_sigma() const override {
                        return 0.0F;
                    }
                    const std::uint64_t get_clustering() const override {
                        return 0ULL;
                    }
                    const std::float_t get_clustering_distance_error() const override {
                        return 0.0F;
                    }

                    const std::vector<std::uint64_t> next() override {
                        if( this->has_next() ){
                            std::vector<std::uint64_t>  ans( this->selected_columns.size() ),
                                                        tmp = this->doc.GetRow<std::uint64_t>( this->index++ );

                            for (size_t i = 0; i < this->selected_columns.size(); i++){
                                ans[ i ] = tmp[ this->selected_columns[ i ] ];
                                // if( ans[ i ] > this->global_max ) {
                                //     this->global_max = ans[ i ];
                                // }
                            }

                            // samg::utils::print_vector<std::uint64_t>("\n",ans);
                            
                            return ans;
                        }
                        throw std::runtime_error("No more entries.");
                    }

                    const std::uint64_t next_zvalue() override {
                        // return samg::utils::to_zvalue3(this->next(), this->n, this->b, this->d, this->bd, this->initial_M); 
                        return this->z_converter.to_zvalue( this->next() );
                    }

            };
        }
    }
}
