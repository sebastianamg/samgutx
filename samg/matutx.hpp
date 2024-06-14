#pragma once
#include <samg/commons.hpp>
#include <cmath>
#include <regex>
#include <memory>
#include <cpp_properties/action/properties_action.hpp>
#include <cpp_properties/actor/properties_actor.hpp>
#include <cpp_properties/actor/traits/properties_actor_traits.hpp>
#include <cpp_properties/parser.hpp>
#include <webgraph/webgraph-3.6.11.h>

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

        /***************************************************************/
        enum FileFormat {
            MTX, // MatrixMarket plain-text format.
            K2T, // k2-tree binary format.
            MDX, // MultidimensionalMatrixMarket plain-text format.
            KNT, // kn-tree binary format.
            QMX, // QMX binary format.
            RRN, // Rice-runs binary format.
            GRAPH, // Graph format from LAW webgraph framework [https://law.di.unimi.it/index.php].
            Unknown
        };

        /**
         * @brief This function allows identifying a file extension based on the input file name.
         * 
         * @param file_name 
         * @return FileFormat 
         */
        FileFormat identify_file_format(const std::string file_name) {
            std::size_t position = file_name.find(".mdx");
            if (position != std::string::npos) {
                return FileFormat::MDX;
            } 
            
            position = file_name.find(".mtx");
            if (position != std::string::npos) {
                return FileFormat::MTX;
            } 

            position = file_name.find(".knt");
            if (position != std::string::npos) {
                return FileFormat::KNT;
            } 

            position = file_name.find(".k2t");
            if (position != std::string::npos) {
                return FileFormat::K2T;
            } 

            position = file_name.find(".qmx");
            if (position != std::string::npos) {
                return FileFormat::QMX;
            } 

            position = file_name.find(".rrn");
            if (position != std::string::npos) {
                return FileFormat::RRN;
            } 

            position = file_name.find(".graph");
            if (position != std::string::npos) {
                return FileFormat::GRAPH;
            } 

            return FileFormat::Unknown;
        }
        /***************************************************************/
        namespace reader {
            const std::size_t roundup_matrix_size( const std::uint64_t size, const std::size_t k ) {
                return (std::size_t) std::pow( k, std::ceil( std::log(size) / std::log(k) ) );
            }

            class Reader {
                private:
                    std::string input_file_name;
                public:
                    Reader(const std::string input_file_name):
                        input_file_name(input_file_name)
                    {}
                    std::string get_input_file_name(){
                        return this->input_file_name;
                    }
                    virtual const std::size_t get_number_of_dimensions() const = 0;
                    virtual const std::vector<std::uint64_t> get_max_per_dimension() const = 0;
                    virtual const std::uint64_t get_number_of_entries() const = 0;
                    virtual const bool has_next() = 0;
                    virtual const std::uint64_t get_matrix_side_size() const = 0;
                    virtual const std::uint64_t get_matrix_size() const = 0;
                    virtual const std::float_t get_matrix_expected_density() const = 0;
                    virtual const std::float_t get_matrix_actual_density() const = 0;
                    virtual const std::string get_matrix_distribution() const = 0;
                    virtual const std::float_t get_gauss_mu() const = 0;
                    virtual const std::float_t get_gauss_sigma() const = 0;
                    virtual const std::uint64_t get_clustering() const = 0;
                    virtual const std::float_t get_clustering_distance_error() const = 0;
                    virtual std::vector<std::uint64_t> next() = 0;
            };
            
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
                    MDXReader(std::string file_name):
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
            };

            // ***************************************************************
            class GraphReader : public Reader {
                private:
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
                    graal_isolate_t *isolate = NULL;
                    graal_isolatethread_t *thread = NULL;

                public:
                    GraphReader(std::string file_name) :
                        Reader(file_name),
                        entries_counter(0ull)
                    {
                        this->max_per_dimension = std::vector<std::uint64_t>();
                        std::string properties_file = samg::utils::change_extension(file_name,"properties");
                        std::string defaults = samg::utils::read_from_file( properties_file.data() );
                        std::map<std::string, std::string> properties;
                        if( cpp_properties::parse(defaults.begin(),defaults.end(),properties) ) {
                            std::uint64_t nodes = std::atoll( properties["nodes"].data() );
                            std::uint64_t arcs = std::atoll( properties["arcs"].data() );
                            this->max_per_dimension = { nodes , nodes };
                            this->number_of_entries = arcs;
                            this->matrix_side_size = nodes;
                            this->matrix_size = nodes*nodes;
                            this->matrix_expected_density = (double) arcs / (double) this->matrix_size;
                            this->matrix_actual_density = (double) arcs / (double) this->matrix_size;
                            this->matrix_distribution = "null";
                            std::float_t gauss_mu = -1.0f;
                            std::float_t gauss_sigma = -1.0f;
                            std::uint64_t clustering = 0ull;
                            std::float_t clustering_distance_error = -1.0f;
                        } else {
                            throw std::runtime_error("Graph properties file \""+properties_file+"\" not available.");
                        }

                        if( graal_create_isolate( NULL, &(this->isolate), &(this->thread) ) != 0) {
                            throw std::runtime_error("GraalVM thread initialization error!");
                        }

                        // std::cout << "file_name = " << file_name << std::endl;
                        std::string grap_file = samg::utils::get_file_basename(file_name);
                        // std::cout << "grap_file = " << grap_file << std::endl;
                        if( !load_graph( this->thread, grap_file.data() ) ) {
                            throw std::runtime_error("Error in graph file loading!");
                        }
                    }
                    ~GraphReader() {
                        graal_tear_down_isolate(this->thread);
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
                        return ( this->number_of_entries > 0 ) && ( this->entries_counter < this->number_of_entries );
                    }

                    const std::uint64_t get_matrix_side_size() const override {
                        // std::cout << "GraphReader>get_matrix_side_size> (1)" << std::endl;
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
                        std::string entry = std::string(get_next_entry(this->thread));
                        if( entry != "eos" ) {
                            std::vector<std::uint64_t> entries = std::vector<std::uint64_t>();
                            std::string token;
                            std::stringstream strm(entry);
                            for(std::size_t d=0;d<this->max_per_dimension.size()&&std::getline(strm,token,'\t');d++){
                                entries.push_back(std::stoull(token));
                                // std::cout << "(1)" << std::endl;
                            }
                            if(entries.size() == this->max_per_dimension.size()) {
                                // std::cout << "(3)" << std::endl;
                                this->entries_counter++;
                                return entries;
                            } else {
                                throw std::runtime_error("Wrong entry format.");
                            }
                        } else {
                            throw std::runtime_error("No more entries.");
                        }
                    }
            };

            std::shared_ptr<Reader> create_instance(const std::string& input_file_name) {
                // std::cout << "get_instance> (1)" << std::endl;
                switch (samg::matutx::identify_file_format(input_file_name)) {
                    case samg::matutx::FileFormat::GRAPH:
                        // std::cout << "get_instance> (2.1)" << std::endl;
                        // return *(std::make_unique<GraphReader>(input_file_name));
                        return std::make_shared<GraphReader>(input_file_name);
                    case samg::matutx::FileFormat::MDX:
                        // std::cout << "get_instance> (2.2)" << std::endl;
                        return std::make_shared<MDXReader>(input_file_name);
                    default:
                        throw std::runtime_error("Unrecognized file format!");
                }
            }

            void destroy_instance( Reader& reader ) {
                delete &reader;
            }
        };

        namespace streamer {
            template<typename IntType> class IntStreamerAdapter {
                private:
                    std::queue<IntType> buffer;
                    std::shared_ptr<samg::matutx::reader::Reader> reader;
                public:
                    IntStreamerAdapter(std::shared_ptr<samg::matutx::reader::Reader> reader):
                        reader(reader) {}

                    const bool has_next() {
                        return !(this->buffer.empty()) || this->reader->has_next();
                    } 

                    const IntType next() {
                        if( this->buffer.empty() ) {
                            for (std::uint64_t v : this->reader->next()) {
                                this->buffer.push((IntType) v);
                            }
                        }
                        IntType ans = this->buffer.front();
                        this->buffer.pop();
                        return ans;
                    }
            };
        }

        /***************************************************************/
    }
}
