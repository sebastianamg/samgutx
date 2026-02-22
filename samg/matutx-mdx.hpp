#pragma once
#include <samg/commons.hpp>
#include <samg/mmm-interface.hpp>
#include <regex>

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
            class MDXReader : public Reader {
                private:
                    std::ifstream input_file;
                    std::vector<std::uint64_t> max_per_dimension;
                    std::uint64_t number_of_entries;
                    std::uint64_t matrix_side_size;
                    std::uint64_t matrix_size;
                    std::float_t matrix_expected_density;
                    std::float_t matrix_actual_density;
                    std::string matrix_distribution;
                    std::float_t gauss_mu;
                    std::float_t gauss_sigma;
                    std::uint64_t clustering;
                    std::float_t clustering_distance_error;
                    std::size_t entries_counter;

                    // std::size_t n;
                    // std::size_t b;
                    // std::size_t d;
                    // std::uint64_t initial_M;
                    // std::size_t bd;
                    samg::utils::ZValueConverter z_converter;

                    static inline std::string trim_right_copy(
                        const std::string& s,
                        const std::string& delimiters = " \f\n\r\t\v" ) {
                        return s.substr( 0, s.find_last_not_of( delimiters ) + 1 );
                    }

                    static inline std::string trim_left_copy(
                        const std::string& s,
                        const std::string& delimiters = " \f\n\r\t\v" ) {
                        return s.substr( s.find_first_not_of( delimiters ) );
                    }

                    static inline std::string trim_copy(
                        const std::string& s,
                        const std::string& delimiters = " \f\n\r\t\v" ) {
                        return MDXReader::trim_left_copy( MDXReader::trim_right_copy( s, delimiters ), delimiters );
                    }

                public:
                    MDXReader(std::string file_name, const std::size_t k = 2ZU ) :
                        Reader(file_name),
                        entries_counter(0ull)
                    {
                        this->max_per_dimension = std::vector<std::uint64_t>();
                        // this->input_file_name = file_name;
                        this->input_file = std::ifstream(file_name);
                        std::string line;
                        if(std::getline(this->input_file,line) && line.rfind("%%MultidimensionalMatrixMarket",0)==0) {
                            // Retrieve meta-data:
                            std::string token;
                            while(std::getline(this->input_file,line) && line.rfind("%",0)==0) {
                                if(line.rfind("% Matrix side size: ",0)==0) {
                                    std::stringstream strm = std::stringstream(line);
                                    if(!std::getline(strm,token,':')) {// Skipping entry header.
                                        throw std::runtime_error("Expected matrix side size header.");
                                    }
                                    if(!std::getline(strm,token,':')) {
                                        throw std::runtime_error("Expected matrix side size.");
                                    }
                                    this->matrix_side_size = stoull(token);
                                } else if(line.rfind("% Matrix size: ",0)==0) {
                                    std::stringstream strm(line);
                                    if(!std::getline(strm,token,':')) {// Skipping entry header.
                                        throw std::runtime_error("Expected matrix size header.");
                                    }
                                    if(!std::getline(strm,token,':')) {
                                        throw std::runtime_error("Expected matrix size.");
                                    }
                                    this->matrix_size = stoull(token);
                                } else if(line.rfind("% Matrix expected density: ",0)==0) {
                                    std::stringstream strm(line);
                                    if(!std::getline(strm,token,':')) {// Skipping entry header.
                                        throw std::runtime_error("Expected matrix expected density header.");
                                    }
                                    if(!std::getline(strm,token,':')) {
                                        throw std::runtime_error("Expected matrix expected density.");
                                    }
                                    this->matrix_expected_density = stof(token);
                                } else if(line.rfind("% Matrix actual density: ",0)==0) {
                                    std::stringstream strm(line);
                                    if(!std::getline(strm,token,':')) {// Skipping entry header.
                                        throw std::runtime_error("Expected matrix actual density header.");
                                    }
                                    if(!std::getline(strm,token,':')) {
                                        throw std::runtime_error("Expected matrix actual density.");
                                    }
                                    this->matrix_actual_density = stof(token);
                                } else if(line.rfind("% Distribution: ",0)==0) {
                                    std::stringstream strm(line);
                                    if(!std::getline(strm,token,':')) {// Skipping entry header.
                                        throw std::runtime_error("Expected matrix distribution header.");
                                    }
                                    if(!std::getline(strm,token,':')) {
                                        throw std::runtime_error("Expected matrix distribution.");
                                    }
                                    this->matrix_distribution = ( token.empty() || std::regex_match (token, std::regex("[ \f\n\r\t\v]+") ) )? "Not specified" : trim_copy(token);
                                } else if(line.rfind("% mu: ",0)==0) {
                                    std::stringstream strm(line);
                                    if(!std::getline(strm,token,':')) {// Skipping entry header.
                                        throw std::runtime_error("Expected mu header.");
                                    }
                                    if(!std::getline(strm,token,':')) {
                                        throw std::runtime_error("Expected mu value.");
                                    }
                                    this->gauss_mu = stof(token);
                                } else if(line.rfind("% sigma: ",0)==0) {
                                    std::stringstream strm(line);
                                    if(!std::getline(strm,token,':')) {// Skipping entry header.
                                        throw std::runtime_error("Expected sigma header.");
                                    }
                                    if(!std::getline(strm,token,':')) {
                                        throw std::runtime_error("Expected sigma value.");
                                    }
                                    this->gauss_sigma = stof(token);
                                } else if(line.rfind("% Clustering: ",0)==0) {
                                    std::stringstream strm(line);
                                    if(!std::getline(strm,token,':')) {// Skipping entry header.
                                        throw std::runtime_error("Expected clustering header.");
                                    }
                                    if(!std::getline(strm,token,':')) {
                                        throw std::runtime_error("Expected clustering value.");
                                    }
                                    this->clustering = stoull(token);
                                } else if(line.rfind("% Clustering distance error: ",0)==0) {
                                    std::stringstream strm(line);
                                    if(!std::getline(strm,token,':')) {// Skipping entry header.
                                        throw std::runtime_error("Expected clustering distance error header.");
                                    }
                                    if(!std::getline(strm,token,':')) {
                                        throw std::runtime_error("Expected clustering distance error value.");
                                    }
                                    this->clustering_distance_error  = stof(token);
                                }// else skip other comments...
                            }
                            
                            // std::string token;
                            std::stringstream strm(line);
                            if(std::getline(strm,token,' ')) {
                                // Get number of dimensions:
                                // std::cout << "Number of dimensions: " << token << std::endl;
                                std::uint8_t number_of_dimensions = std::stoul(token);
                                // Get maximum values per dimensions:
                                for(int d=0;d < number_of_dimensions && std::getline(strm,token,' ');d++){
                                    this->max_per_dimension.push_back(std::stoull(token));
                                }
                                // Get number of non-zero entries:
                                if(std::getline(strm,token,' ')) {
                                    this->number_of_entries = stoull(token);
                                } else {
                                    throw std::runtime_error("Expected number of entries.");
                                }
                            } else {
                                throw std::runtime_error("Expected number of dimensions.");
                            }
                        } else {
                            throw std::runtime_error("Wrong MDX format (\""+file_name+"\").");
                        }
                        // this->b = samg::utils::get_required_bits( k );//(k == 1UL ? 0UL : std::bit_width(k - 1UL)), // Number of bits per coordinate component considered for Z-ordering.
                        // this->d = samg::utils::get_required_digits( this->matrix_side_size, b );//(s == 0) ? 0 : static_cast<std::size_t>(std::ceil(std::log2(s) / static_cast<double>(b))), // Number of digits to encode a component considered for Z-ordering.
                        // this->n = this->max_per_dimension.size(); // Number of dimensions of the matrix.
                        // this->initial_M = samg::utils::get_initial_mask( b );
                        // this->bd = b * d;
                        this->z_converter = samg::utils::ZValueConverter( this->matrix_side_size, this->max_per_dimension.size(), k );
                        
                    }
                    ~MDXReader() {
                        this->input_file.close();
                    }

                    const std::size_t get_number_of_dimensions() const override {
                        return this->max_per_dimension.size();
                    }
                    const std::vector<std::uint64_t> get_max_per_dimension() const override {
                        return this->max_per_dimension;
                    }
                    const std::uint64_t get_number_of_entries() const override {
                        return this->number_of_entries;
                    }
                    const bool has_next() override {
                        // std::cout << "is_there_next_entry> number of entries = " << this->number_of_entries << "; eof? = " << this->input_file.eof() << std::endl;
                        return ( this->number_of_entries > 0 ) && ( this->entries_counter < this->number_of_entries ) && !this->input_file.eof();
                    }

                    const std::uint64_t get_matrix_side_size() const override {
                        // std::cout << "MDXReader>get_matrix_side_size> (1)" << std::endl;
                        return this->matrix_side_size;
                    }
                    const std::uint64_t get_matrix_size() const override {
                        return this->matrix_size;
                    }
                    const std::float_t get_matrix_expected_density() const override {
                        return this->matrix_expected_density;
                    }
                    const std::float_t get_matrix_actual_density() const override {
                        return this->matrix_actual_density;
                    }
                    const std::string get_matrix_distribution() const override {
                        return this->matrix_distribution;
                    }
                    const std::float_t get_gauss_mu() const override {
                        return this->gauss_mu;
                    }
                    const std::float_t get_gauss_sigma() const override {
                        return this->gauss_sigma;
                    }
                    const std::uint64_t get_clustering() const override {
                        return this->clustering;
                    }
                    const std::float_t get_clustering_distance_error() const override {
                        return this->clustering_distance_error;
                    }

                    std::vector<std::uint64_t> next() override {
                        std::string line;
                        while(std::getline(this->input_file,line)){
                            if(line.rfind("%",0)!=0) {
                                std::vector<std::uint64_t> entries = std::vector<std::uint64_t>();
                                std::string token;
                                std::stringstream strm(line);
                                for(std::size_t d=0;d<this->max_per_dimension.size()&&std::getline(strm,token,' ');d++){
                                    entries.push_back(std::stoull(token));
                                    // std::cout << "(1)" << std::endl;
                                }
                                if(entries.size() == this->max_per_dimension.size()) {
                                    // std::cout << "(3)" << std::endl;
                                    this->entries_counter++;
                                    /* ===================== */
                                    std::cout << "MDX ENTRY: " << samg::utils::to_string(entries) << std::endl;
                                    /* ===================== */
                                    return entries;
                                } else {
                                    throw std::runtime_error("Wrong entry format.");
                                }
                                // std::cout << "(2)" << std::endl;
                            }
                        }
                        // std::cout << "|Entries| = " << entries.size() << "; line = '" << line << "'" << std::endl;
                        throw std::runtime_error("No more entries.");
                    }

                    std::uint64_t next_zvalue() override {
                        // return samg::utils::to_zvalue3( this->next(), this->n, this->b, this->d, this->bd, this->initial_M );
                        return this->z_converter.to_zvalue( this->next() );
                    }
            };
        }
    }
}
