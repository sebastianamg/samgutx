#pragma once
#include <samg/commons.hpp>
#include <cmath>
#include <type_traits>
#include <regex>
#include <map>
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
                    virtual std::size_t get_number_of_dimensions() = 0;
                    virtual std::vector<std::uint64_t> get_max_per_dimension() = 0;
                    virtual std::uint64_t get_number_of_entries() = 0;
                    virtual bool has_next() = 0;
                    virtual std::uint64_t get_matrix_side_size() = 0;
                    virtual std::uint64_t get_matrix_size() = 0;
                    virtual std::float_t get_matrix_expected_density() = 0;
                    virtual std::float_t get_matrix_actual_density() = 0;
                    virtual std::string get_matrix_distribution() = 0;
                    virtual std::float_t get_gauss_mu() = 0;
                    virtual std::float_t get_gauss_sigma() = 0;
                    virtual std::uint64_t get_clustering() = 0;
                    virtual std::float_t get_clustering_distance_error() = 0;
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

                    std::size_t get_number_of_dimensions() override {
                        return this->max_per_dimension.size();
                    }
                    std::vector<std::uint64_t> get_max_per_dimension() override {
                        return this->max_per_dimension;
                    }
                    std::uint64_t get_number_of_entries() override {
                        return this->number_of_entries;
                    }
                    bool has_next() override {
                        // std::cout << "is_there_next_entry> number of entries = " << this->number_of_entries << "; eof? = " << this->input_file.eof() << std::endl;
                        return ( this->number_of_entries > 0 ) && ( this->entries_counter < this->number_of_entries ) && !this->input_file.eof();
                    }

                    std::uint64_t get_matrix_side_size() override {
                        // std::cout << "MDXReader>get_matrix_side_size> (1)" << std::endl;
                        return this->matrix_side_size;
                    }
                    std::uint64_t get_matrix_size() override {
                        return this->matrix_size;
                    }
                    std::float_t get_matrix_expected_density() override {
                        return this->matrix_expected_density;
                    }
                    std::float_t get_matrix_actual_density() override {
                        return this->matrix_actual_density;
                    }
                    std::string get_matrix_distribution() override {
                        return this->matrix_distribution;
                    }
                    std::float_t get_gauss_mu() override {
                        return this->gauss_mu;
                    }
                    std::float_t get_gauss_sigma() override {
                        return this->gauss_sigma;
                    }
                    std::uint64_t get_clustering() override {
                        return this->clustering;
                    }
                    std::float_t get_clustering_distance_error() override {
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

                    std::size_t get_number_of_dimensions() override {
                        return this->max_per_dimension.size();
                    }
                    std::vector<std::uint64_t> get_max_per_dimension() override {
                        return this->max_per_dimension;
                    }
                    std::uint64_t get_number_of_entries() override {
                        return this->number_of_entries;
                    }
                    bool has_next() override {
                        // std::cout << "is_there_next_entry> number of entries = " << this->number_of_entries << "; eof? = " << this->input_file.eof() << std::endl;
                        return ( this->number_of_entries > 0 ) && ( this->entries_counter < this->number_of_entries );
                    }

                    std::uint64_t get_matrix_side_size() override {
                        // std::cout << "GraphReader>get_matrix_side_size> (1)" << std::endl;
                        return this->matrix_side_size;
                    }
                    std::uint64_t get_matrix_size() override {
                        return this->matrix_size;
                    }
                    std::float_t get_matrix_expected_density() override {
                        return this->matrix_expected_density;
                    }
                    std::float_t get_matrix_actual_density() override {
                        return this->matrix_actual_density;
                    }
                    std::string get_matrix_distribution() override {
                        return this->matrix_distribution;
                    }
                    std::float_t get_gauss_mu() override {
                        return this->gauss_mu;
                    }
                    std::float_t get_gauss_sigma() override {
                        return this->gauss_sigma;
                    }
                    std::uint64_t get_clustering() override {
                        return this->clustering;
                    }
                    std::float_t get_clustering_distance_error() override {
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
    namespace serialization { 
        /**
         * @brief The class WordSequenceSerializer allows serializing and deserializing a sequence of integers defined through its template. 
         * 
         * @tparam Type 
         */
        template<typename Type> class WordSequenceSerializer {
            private:
                std::vector<Type> sequence;
                const std::size_t BITS_PER_BYTE = 8UL;
                const std::size_t WORD_SIZE = sizeof(Type)  * BITS_PER_BYTE;
                std::uint64_t current_index;

                /**
                 * @brief This function allows converting a std::string into a std::vector<T> 
                 * 
                 * @tparam T is the output std::vector data type.
                 * @param str 
                 * @return std::vector<T> 
                 */
                template<typename T> static std::vector<T> convert_string_to_vector(const std::string str) {
                    const T* T_ptr = reinterpret_cast<const T*>(str.data());
                    std::vector<T> result(T_ptr, T_ptr + (std::size_t)std::ceil((double)str.length() / (double)sizeof(T)));
                    return result;
                }

                /**
                 * @brief This function allows converting a std::vector<T> into a std::string.
                 * 
                 * @tparam T is the input std::vector data type.
                 * @param vector 
                 * @param length is the number of bytes of the original std::string stored in the input std::vector. 
                 * @return std::string 
                 */
                template<typename T> static std::string convert_vector_to_string(const std::vector<T>& vector, std::size_t length) {
                    const T* T_ptr = vector.data();
                    // std::size_t length = vector.size() * sizeof(T);
                    const char* char_ptr = reinterpret_cast<const char*>(T_ptr);
                    return std::string(char_ptr, length);
                }

                /**
                 * @brief This function allows serializing a sequence of unsigned integers.
                 * 
                 * @tparam UINT_T 
                 * @param data 
                 * @param file_name 
                 */
                template<typename UINT_T = std::uint32_t> static void store_binary_sequence(const std::vector<UINT_T> data, const std::string file_name) { 
                    std::ofstream file = std::ofstream(file_name, std::ios::binary);
                    if ( file.is_open() ) {
                        // Writing the vector's data to the file
                        file.write(reinterpret_cast<const char*>(data.data()), data.size() * sizeof(UINT_T));
                        file.close();
                        // std::cout << "Data written to file." << std::endl;
                    } else {
                        throw std::runtime_error("Failed to open file \""+file_name+"\"!");
                    }
                }

                /**
                 * @brief This function allows retrieving a serialized sequence of unsigned integers from a binary file.
                 * 
                 * @tparam UINT_T 
                 * @param file_name 
                 * @return std::vector<UINT_T> 
                 */
                template<typename UINT_T = std::uint32_t> static std::vector<UINT_T> retrieve_binary_sequence(const std::string file_name) {
                    std::vector<UINT_T> data;

                    std::ifstream file(file_name, std::ios::binary);
                    if (file) {
                        // Determine the size of the file
                        file.seekg(0, std::ios::end);
                        std::streamsize fileSize = file.tellg();
                        file.seekg(0, std::ios::beg);

                        // Resize the vector to hold the data
                        data.resize(fileSize / sizeof(UINT_T));

                        // Read the data from the file into the vector
                        file.read(reinterpret_cast<char*>(data.data()), fileSize);

                        file.close();
                        // std::cout << "Data retrieved from file." << std::endl;

                        return data;
                    } else {
                        throw std::runtime_error("Failed to open file \""+file_name+"\"!");
                    }
                }


            public:
                /**
                 * @brief Construct a new word sequence serializer object to start a new serialization.
                 */
                WordSequenceSerializer(): current_index(0ULL) {}
                
                /**
                 * @brief Construct a new word sequence serializer object that retrieves data from an input file.
                 * 
                 * @param file_name
                 */
                WordSequenceSerializer(const std::string file_name): current_index(0ULL) {
                    this->sequence = WordSequenceSerializer::retrieve_binary_sequence<Type>(file_name);
                }
                /**
                 * @brief Construct a new Word Sequence Serializer object from a serialized sequence.
                 * 
                 * @param sequence 
                 */
                WordSequenceSerializer( std::vector<Type> sequence ): 
                    sequence(sequence),
                    current_index(0ULL) {}

                /**
                 * @brief This function allows parsing integer values stored in an input vector of type TypeSrc into type TypeTrg.
                 * 
                 * @tparam TypeSrc 
                 * @tparam TypeTrg 
                 * @param V 
                 * @return std::vector<TypeTrg> 
                 */
                template<typename TypeSrc, typename TypeTrg = Type> std::vector<TypeTrg> parse_values(std::vector<TypeSrc> V) {
                    std::vector<TypeTrg> T;
                    if( sizeof(TypeTrg) == sizeof(TypeSrc) ) {
                        T.insert(T.end(),V.begin(),V.end());
                    } else { // sizeof(TypeSrc) != sizeof(Type)
                        const TypeTrg* x = reinterpret_cast<const TypeTrg*>(V.data());
                        std::size_t l = (std::size_t)std::ceil((double)(V.size()*sizeof(TypeSrc)) / (double)sizeof(TypeTrg));
                        T.insert(T.end(),x,x+l);
                    }
                    return T;
                }

                // /**
                //  * @brief This function allows parsing an integer value of type TypeSrc into type TypeTrg.
                //  * 
                //  * @tparam TypeSrc 
                //  * @tparam TypeTrg 
                //  * @param v 
                //  * @return TypeTrg 
                //  */
                // template<typename TypeSrc, typename TypeTrg=Type> TypeTrg parse_value(TypeSrc v) {
                //     std::vector<TypeSrc> V = {v};
                //     return this->parse_values<TypeSrc>(V)[0];
                // }
                // template<typename TypeSrc, typename TypeTrg=Type> std::vector<TypeTrg> parse_value(TypeSrc v) {
                //     std::vector<TypeSrc> V = {v};
                //     return this->parse_values<TypeSrc>(V);
                // }


                /**
                 * @brief This function allows adding an 8/16/32/64-bits value. 
                 * 
                 * @tparam TypeSrc 
                 * @param v 
                 */
                template<typename TypeSrc = Type> void add_value(TypeSrc v) {
                    static_assert(
                        std::is_same_v<TypeSrc, std::uint8_t> ||
                        std::is_same_v<TypeSrc, std::uint16_t> ||
                        std::is_same_v<TypeSrc, std::uint32_t> ||
                        std::is_same_v<TypeSrc, std::uint64_t>,
                        "typename must be one of std::uint8_t, std::uint16_t, std::uint32_t, or std::uint64_t");

                    // std::cout << "add_value(v) = " << v << std::endl;

                    std::vector<TypeSrc> V = {v};
                    std::vector<Type> X = this->parse_values<TypeSrc>(V);
                    this->sequence.insert(this->sequence.end(),X.begin(),X.end());
                    // this->sequence.push_back(this->parse_values<TypeSrc>(v));
                }

                // /**
                //  * @brief This function allows adding an 8-bits value. 
                //  * 
                //  * @param v 
                //  */
                // template<typename TypeSrc> void add_value(std::uint8_t v) {
                //     // std::vector<Type> trg = this->parse_value<std::uint8_t>(v);
                //     // this->sequence.insert(this->sequence.end(),trg.begin(),trg.end());
                //     this->sequence.push_back(this->parse_value<std::uint8_t>(v));
                // }

                // /**
                //  * @brief This function allows adding a 16-bits value. 
                //  * 
                //  * @param v 
                //  */
                // void add_value(std::uint16_t v) {
                //     // std::vector<Type> trg = this->parse_value<std::uint16_t>(v);
                //     // this->sequence.insert(this->sequence.end(),trg.begin(),trg.end());
                //     this->sequence.push_back(this->parse_value<std::uint16_t>(v));
                // }

                // /**
                //  * @brief This function allows adding a 32-bits value. 
                //  * 
                //  * @param v 
                //  */
                // void add_value(std::uint32_t v) {
                //     // std::vector<Type> trg = this->parse_value<std::uint32_t>(v);
                //     // this->sequence.insert(this->sequence.end(),trg.begin(),trg.end());
                //     this->sequence.push_back(this->parse_value<std::uint32_t>(v));
                // }

                // /**
                //  * @brief This function allows adding a 64-bits value. 
                //  * 
                //  * @param v 
                //  */
                // void add_value(std::uint64_t v) {
                //     // std::vector<Type> trg = this->parse_value<std::uint64_t>(v);
                //     // this->sequence.insert(this->sequence.end(),trg.begin(),trg.end());
                //     this->sequence.push_back(this->parse_value<std::uint64_t>(v));
                // }

                /**
                 * @brief This function allows adding a collection of l TypeSrc integer values. 
                 * 
                 * @tparam TypeSrc 
                 * @param v 
                 * @param l 
                 */
                template<typename TypeSrc = Type, typename TypeLength = std::size_t> void add_values(const TypeSrc *v, const TypeLength l) {
                    std::vector<TypeSrc> V;
                    V.insert(V.end(),v,v+l);
                    this->add_values<TypeSrc>(V);
                }

                /**
                 * @brief This function allows adding a collection of unsigned integer values. 
                 * 
                 * @tparam TypeSrc 
                 * @param V 
                 */
                template<typename TypeSrc = Type> void add_values(const std::vector<TypeSrc> V) {
                    static_assert(
                        std::is_same_v<TypeSrc, std::uint8_t> ||
                        std::is_same_v<TypeSrc, std::uint16_t> ||
                        std::is_same_v<TypeSrc, std::uint32_t> ||
                        std::is_same_v<TypeSrc, std::uint64_t>,
                        "typename must be one of std::uint8_t, std::uint16_t, std::uint32_t, or std::uint64_t");
                    for (TypeSrc v : V) {
                        this->add_value<TypeSrc>(v);
                    }
                    // std::vector<Type> X = this->parse_values<TypeSrc>(V);
                    // this->sequence.insert(this->sequence.end(),X.begin(),X.end());
                }

                /**
                 * @brief This function allows serializing a string.
                 * 
                 * @param v 
                 */
                void add_value(std::string v) {
                    std::vector<Type> V = WordSequenceSerializer::convert_string_to_vector<Type>(v);
                    this->add_value<std::size_t>(v.length()); // Storing the original length (number of characters/bytes) of the key. 
                    this->add_value<std::size_t>(V.size()); // Storing the length (number of words<Type>) of the key. 
                    this->sequence.insert(this->sequence.end(), V.begin(), V.end());
                }

                /**
                 * @brief This function allows adding a std::map<std::string,std::string> entry.
                 * 
                 * @param p map<std::string,std::string>'s entry
                 */
                void add_map_entry(const std::pair<std::string,std::string> p) {
                    // Adding key:
                    this->add_value(p.first);
                    // Adding value:
                    this->add_value(p.second);
                }

                /**
                 * @brief This function allows adding a map<std::string,std::string>.
                 * 
                 * @param m 
                 */
                void add_map(std::map<std::string,std::string> m) {
                    this->add_value<std::size_t>(m.size());
                    for (std::pair<std::string,std::string> p : m) {
                        this->add_map_entry(p);
                    }
                }

                /**
                 * @brief This function allows saving the serialization into a given file. 
                 * 
                 * @param file_name 
                 */
                void save(const std::string file_name) {
                    WordSequenceSerializer::store_binary_sequence<Type>(this->sequence,file_name);
                }

                // ***************************************************************

                /**
                 * @brief This function allows getting remaining values from the serialization starting from where an internal index is. 
                 * 
                 * @tparam TypeTrg 
                 * @return const std::vector<TypeTrg> 
                 */
                template<typename TypeTrg = Type> const std::vector<TypeTrg> get_remaining_values() {
                    std::vector<TypeTrg> V;
                    while(this->has_more()) {
                        V.push_back(this->get_value<TypeTrg>());
                    }
                    return V;
                }

                /**
                 * @brief This function allows retrieving the next `length` values of type TypeTrg.
                 * 
                 * @tparam TypeTrg 
                 * @param length 
                 * @return const std::vector<TypeTrg> 
                 */
                template<typename TypeTrg = Type> const std::vector<TypeTrg> get_next_values(std::uint64_t length) {
                    std::vector<TypeTrg> V;
                    // std::size_t n = std::ceil( (length * sizeof(TypeTrg)) / sizeof(Type) ); // Compute number of Type-words that contain the TypeTrg-words.
                    for (std::uint64_t i = 0 ; i < length /*&& this->has_more()*/; i++) {
                        V.push_back(this->get_value<TypeTrg>());
                    }
                    return V;
                }

                /**
                 * @brief This function allows retrieving the next `length` values of type TypeTrg starting at `beginning_index`.
                 * 
                 * @tparam TypeTrg 
                 * @param beginning_index 
                 * @param length 
                 * @return const std::vector<TypeTrg> 
                 */
                template<typename TypeTrg = Type> const std::vector<TypeTrg> get_values(std::uint64_t beginning_index, std::uint64_t length) const {
                    std::vector<TypeTrg> V;
                    // std::size_t n = std::ceil( (length * sizeof(TypeTrg)) / sizeof(Type) ); // Compute number of Type-words that contain the TypeTrg-words.
                    std::size_t step = ( sizeof(TypeTrg) <= sizeof(Type) ) ? 1 : ( sizeof(TypeTrg) / sizeof(Type) );
                    std::uint64_t limit = beginning_index + ( length * step );
                    for ( std::uint64_t i = beginning_index ; i < limit; i+=step ) {
                        V.push_back(this->get_value_at<TypeTrg>(i));
                    }
                    return V;
                }

                /**
                 * @brief This function allows getting the `i`-th TypeTrg type value from the serialization. 
                 * 
                 * @tparam TypeTrg 
                 * @param i 
                 * @return const Type 
                 */
                template<typename TypeTrg = Type> const Type get_value_at( std::uint64_t i ) const {
                    // if( sizeof(TypeTrg) <= sizeof(Type) ) {
                    //     return (TypeTrg) this->sequence[i];
                    // } else {
                        std::vector<Type> V;
                        std::uint64_t limit = std::ceil( sizeof(TypeTrg) / sizeof(Type) );
                        for ( ; i < limit; ++i ) {
                            V.push_back( this->sequence[i] );
                        }
                        std::vector<TypeTrg> T = this->parse_values<Type,TypeTrg>(V);
                        return T[0]; // Assuming T contains only one value.
                    // }
                }

                /**
                 * @brief This function allows retrieving the next value, based on an internal index. 
                 * 
                 * @tparam TypeTrg 
                 * @return const TypeTrg 
                 */
                template<typename TypeTrg = Type> const TypeTrg get_value() {
                    // if( sizeof(TypeTrg) <= sizeof(Type) ) {
                    //     return (TypeTrg) this->sequence[this->current_index++];
                    // } else {
                        std::vector<Type> V;
                        std::uint64_t limit = std::ceil( (double)sizeof(TypeTrg) / (double)sizeof(Type) );
                        for (std::size_t i = 0; i < limit; ++i ) {
                            V.push_back( this->sequence[this->current_index++] );
                        }
                        std::vector<TypeTrg> T = this->parse_values<Type,TypeTrg>(V);
                        // std::cout << "get_value (|TypeTrg|<"<< sizeof(TypeTrg) <<">) = " << T[0] << std::endl;
                        return T[0]; // Assuming T contains only one value.
                    // }
                }

                /**
                 * @brief This function allows retrieving a serialized string. 
                 * 
                 * @return std::string 
                 */
                std::string get_string_value() {
                    const std::size_t   bytes_length = this->get_value<std::size_t>(),
                                        words_legnth = this->get_value<std::size_t>();
                    std::vector<Type> V = this->get_next_values(words_legnth);
                    std::string str = WordSequenceSerializer::convert_vector_to_string<Type>(V,bytes_length);
                    return str;
                }

                /**
                 * @brief This function allows retrieving a serialized `std::map<std::string,std::string>`'s entry.
                 * 
                 * @return std::pair<std::string,std::string> 
                 */
                std::pair<std::string,std::string> get_map_entry() {
                    std::string key = this->get_string_value();
                    std::string value = this->get_string_value();
                    return std::pair<std::string,std::string>(key,value);
                }

                /**
                 * @brief This function allows retrieving a serialized `std::map<std::string,std::string>`.
                 * 
                 * @return std::map<std::string,std::string> 
                 */
                std::map<std::string,std::string> get_map() {
                    const std::size_t length = this->get_value<std::size_t>();
                    std::map<std::string,std::string> map;
                    for (std::uint64_t i = 0; i < length; i++) {
                        map.insert(this->get_map_entry());
                    }
                    return map;
                }

                /**
                 * @brief This method allows verifying whether the serialization has or not more elements. 
                 * 
                 * @return true 
                 * @return false 
                 */
                const bool has_more() const {
                    return this->current_index < this->sequence.size();
                }

                /**
                 * @brief This methods returns the current internal index.
                 * 
                 * @return const std::uint64_t 
                 */
                const std::uint64_t get_current_index() const {
                    return this->current_index;
                }

                /**
                 * @brief This method returns the number of Type words that composes the serialization. 
                 * 
                 * @return const std::uint64_t 
                 */
                const std::uint64_t size() const {
                    return this->sequence.size();
                }

                /**
                 * @brief This function returns the internal serialized sequence.
                 * 
                 * @return std::vector<Type> 
                 */
                std::vector<Type> get_serialized_sequence() const {
                    return this->sequence;
                }

                /**
                 * @brief This method displays the sequence of Type words that compose the serialization. 
                 * 
                 */
                const void print() {
                    std::cout << "WordSequenceSerializer --- sequence: " << std::endl;
                    for (Type v : this->sequence) {
                        std::cout << v << " ";
                    }
                    std::cout << std::endl;
                }
        };

        /**
         * @brief The class OfflineWordSerializer allows direct offline integer sequence serialization directly to a file. 
         * 
         * @tparam Type 
         */
        template<typename Type> class OfflineWordSerializer {
            private:
                const std::size_t BITS_PER_BYTE = 8UL;
                const std::size_t WORD_SIZE = sizeof(Type)  * BITS_PER_BYTE;
                std::size_t word_counter;
                const std::string file_name;
                std::ofstream file;

                /**
                 * @brief This function allows converting a std::string into a std::vector<T> 
                 * 
                 * @tparam T is the output std::vector data type.
                 * @param str 
                 * @return std::vector<T> 
                 */
                template<typename T> static std::vector<T> _convert_string_to_vector_(const std::string str) {
                    const T* T_ptr = reinterpret_cast<const T*>(str.data());
                    std::vector<T> result(T_ptr, T_ptr + (std::size_t)std::ceil((double)str.length() / (double)sizeof(T)));
                    return result;
                }

                /**
                 * @brief Serializes a sequence of unsigned integers.
                 * 
                 * @tparam UINT_T 
                 * @param data 
                 * @param file_name 
                 */
                template<typename UINT_T = std::uint32_t> static void _write_(const std::vector<UINT_T> data, std::ofstream& file) { 
                    if ( file.is_open() ) {
                        // Writing the vector's data to the file
                        file.write(reinterpret_cast<const char*>(data.data()), data.size() * sizeof(UINT_T));
                    } else {
                        throw std::runtime_error("Failed to write to file!");
                    }
                }

                /**
                 * @brief This function allows parsing integer values stored in an input vector of type TypeSrc into type TypeTrg.
                 * 
                 * @tparam TypeSrc 
                 * @tparam TypeTrg 
                 * @param V 
                 * @return std::vector<TypeTrg> 
                 */
                template<typename TypeSrc, typename TypeTrg = Type> static std::vector<TypeTrg> _parse_values_(std::vector<TypeSrc> V) {
                    std::vector<TypeTrg> T;
                    if( sizeof(TypeTrg) == sizeof(TypeSrc) ) {
                        T.insert(T.end(),V.begin(),V.end());
                    } else { // sizeof(TypeSrc) != sizeof(Type)
                        const TypeTrg* x = reinterpret_cast<const TypeTrg*>(V.data());
                        std::size_t l = (std::size_t)std::ceil((double)(V.size()*sizeof(TypeSrc)) / (double)sizeof(TypeTrg));
                        T.insert(T.end(),x,x+l);
                    }
                    return T;
                }

            public:
                /**
                 * @brief Constructs a new offline word serializer object.
                 * 
                 * @param file_name
                 */
                OfflineWordSerializer(const std::string file_name): 
                    file_name ( file_name ),
                    file ( std::ofstream(file_name, std::ios::binary) ),
                    word_counter (0ULL) {
                    if ( !file.is_open() ) {
                        throw std::runtime_error("Failed to open file \""+file_name+"\"!");
                    }
                }
             
                /**
                 * @brief Adds an 8/16/32/64-bits value. 
                 * @note It reduces any input to `Type` type. 
                 * 
                 * @tparam TypeSrc 
                 * @param v 
                 */
                template<typename TypeSrc = Type> void add_value(TypeSrc v) {
                    static_assert(
                        std::is_same_v<TypeSrc, std::uint8_t> ||
                        std::is_same_v<TypeSrc, std::uint16_t> ||
                        std::is_same_v<TypeSrc, std::uint32_t> ||
                        std::is_same_v<TypeSrc, std::uint64_t>,
                        "typename must be one of std::uint8_t, std::uint16_t, std::uint32_t, or std::uint64_t");

                    std::vector<TypeSrc> V = {v};
                    std::vector<Type> X = OfflineWordSerializer<Type>::_parse_values_<TypeSrc>(V);
                    OfflineWordSerializer<Type>::_write_<Type>( X, this->file );
                    this->word_counter += X.size();
                }

                /**
                 * @brief Adds a collection of l TypeSrc integer values. 
                 * 
                 * @tparam TypeSrc 
                 * @param v 
                 * @param l 
                 */
                template<typename TypeSrc = Type, typename TypeLength = std::size_t> void add_values(const TypeSrc *v, const TypeLength l) {
                    std::vector<TypeSrc> V;
                    V.insert(V.end(),v,v+l);
                    this->add_values<TypeSrc>(V);
                }

                /**
                 * @brief Adds a collection of unsigned integer values. 
                 * 
                 * @tparam TypeSrc 
                 * @param V 
                 */
                template<typename TypeSrc = Type> void add_values(const std::vector<TypeSrc> V) {
                    static_assert(
                        std::is_same_v<TypeSrc, std::uint8_t> ||
                        std::is_same_v<TypeSrc, std::uint16_t> ||
                        std::is_same_v<TypeSrc, std::uint32_t> ||
                        std::is_same_v<TypeSrc, std::uint64_t>,
                        "typename must be one of std::uint8_t, std::uint16_t, std::uint32_t, or std::uint64_t");
                    for (TypeSrc v : V) {
                        this->add_value<TypeSrc>(v);
                    }
                }

                /**
                 * @brief Serializes a string.
                 * 
                 * @param v 
                 */
                void add_value(std::string v) {
                    std::vector<Type> V = OfflineWordSerializer::_convert_string_to_vector_<Type>(v);
                    this->add_value<std::size_t>(v.length()); // Storing the original length (number of characters/bytes) of the key. 
                    this->add_value<std::size_t>(V.size()); // Storing the length (number of words<Type>) of the key. 
                    this->sequence.insert(this->sequence.end(), V.begin(), V.end());
                }

                /**
                 * @brief Adds a `std::map<std::string,std::string>` entry.
                 * 
                 * @param p map<std::string,std::string>'s entry
                 */
                void add_map_entry(const std::pair<std::string,std::string> p) {
                    // Adding key:
                    this->add_value(p.first);
                    // Adding value:
                    this->add_value(p.second);
                }

                /**
                 * @brief Adds a `map<std::string,std::string>`.
                 * 
                 * @param m 
                 */
                void add_map(std::map<std::string,std::string> m) {
                    this->add_value<std::size_t>(m.size());
                    for (std::pair<std::string,std::string> p : m) {
                        this->add_map_entry(p);
                    }
                }

                /**
                 * @brief Returns the number of written `Type` words. 
                 * 
                 * @return const std::size_t 
                 */
                const std::size_t size() const {
                    return this->word_counter;
                }

                /**
                 * @brief Closes binary file.
                 * 
                 */
                void close() {
                    this->file.close();
                }
        };

        /**
         * @brief Allows a direct integers sequence offline reading from a binary file. 
         * 
         * @tparam Type 
         */
        template<typename Type> class OfflineWordReader {
            private:
                const std::size_t   BITS_PER_BYTE = 8UL,
                                    WORD_SIZE = sizeof(Type)  * BITS_PER_BYTE,
                                    serialization_length;
                const std::string file_name;
                std::ifstream file;

                /**
                 * @brief Allows converting a std::vector<T> into a std::string.
                 * 
                 * @tparam T is the input std::vector data type.
                 * @param vector 
                 * @param length is the number of bytes of the original std::string stored in the input std::vector. 
                 * @return std::string 
                 */
                template<typename T> static std::string _convert_vector_to_string_(const std::vector<T>& vector, std::size_t length) {
                    const T* T_ptr = vector.data();
                    // std::size_t length = vector.size() * sizeof(T);
                    const char* char_ptr = reinterpret_cast<const char*>(T_ptr);
                    return std::string(char_ptr, length);
                }

                /**
                 * @brief This function allows parsing integer values stored in an input vector of type TypeSrc into type TypeTrg.
                 * 
                 * @tparam TypeSrc 
                 * @tparam TypeTrg 
                 * @param V 
                 * @return std::vector<TypeTrg> 
                 */
                template<typename TypeSrc, typename TypeTrg = Type> static std::vector<TypeTrg> _parse_values_(std::vector<TypeSrc> V) {
                    std::vector<TypeTrg> T;
                    if( sizeof(TypeTrg) == sizeof(TypeSrc) ) {
                        T.insert(T.end(),V.begin(),V.end());
                    } else { // sizeof(TypeSrc) != sizeof(Type)
                        const TypeTrg* x = reinterpret_cast<const TypeTrg*>(V.data());
                        std::size_t l = (std::size_t)std::ceil((double)(V.size()*sizeof(TypeSrc)) / (double)sizeof(TypeTrg));
                        T.insert(T.end(),x,x+l);
                    }
                    return T;
                }

                /**
                 * @brief Allows retrieving a serialized unsigned integer from a binary file.
                 * 
                 * @tparam UINT_T 
                 * @param file_name 
                 * @return UINT_T
                 */
                template<typename UINT_T = std::uint32_t> UINT_T _read_( std::ifstream& file ) {
                    if ( !file.is_open() ) {
                        throw std::runtime_error("The file \""+file_name+"\" is closed!");
                    }
                    
                    if ( file.eof() ) {
                        throw std::runtime_error("The file \""+file_name+"\" has no more data!");
                    }

                    std::vector<UINT_T> data = std::vector<UINT_T>( 1ULL );
                    
                    // Read the data from the file into the vector
                    file.read(reinterpret_cast<char*>(data.data()), sizeof(UINT_T));
                    
                    return data[0];
                }

            public:
                /**
                 * @brief Constructs a new offline word serializer object that either writes or retrieves data to or from a file.
                 * 
                 * @param file_name
                 */
                OfflineWordReader(const std::string file_name): 
                file_name ( file_name ),
                file ( std::ifstream(file_name, std::ios::binary) ) {
                    if ( !file.is_open() ) {
                        throw std::runtime_error("Failed to open file \""+file_name+"\"!");
                    }
                    
                    // Computing input length:
                    file.seekg(0, std::ios::end);
                    this->serialization_length = file.tellg();
                    file.seekg(0, std::ios::beg);
                }

                /**
                 * @brief Allows retrieving the next value. 
                 * 
                 * @tparam TypeTrg 
                 * @return const TypeTrg 
                 */
                template<typename TypeTrg = Type> const TypeTrg next() {
                    return OfflineWordReader<Type>::_read_<TypeTrg>( this->file );
                }

                /**
                 * @brief Allows getting all the remaining values from the serialization. 
                 * 
                 * @tparam TypeTrg 
                 * @return const std::vector<TypeTrg> 
                 */
                template<typename TypeTrg = Type> const std::vector<TypeTrg> get_remaining_values() {
                    std::vector<TypeTrg> V;
                    while(this->has_more()) {
                        V.push_back(this->next<TypeTrg>());
                    }
                    return V;
                }

                /**
                 * @brief Allows retrieving the next `length` values.
                 * 
                 * @tparam TypeTrg 
                 * @param length 
                 * @return const std::vector<TypeTrg> 
                 */
                template<typename TypeTrg = Type> const std::vector<TypeTrg> get_next_values(std::uint64_t length) {
                    std::vector<TypeTrg> V;
                    for (std::uint64_t i = 0 ; i < length ; i++) {
                        V.push_back(this->next<TypeTrg>());
                    }
                    return V;
                }

                /**
                 * @brief Retrieves a serialized string. 
                 * 
                 * @return std::string 
                 */
                std::string get_string_value() {
                    const std::size_t   bytes_length = this->next<std::size_t>(),
                                        words_legnth = this->next<std::size_t>();
                    std::vector<Type> V = this->get_next_values( words_legnth );
                    std::string str = OfflineWordReader::_convert_vector_to_string_<Type>( V , bytes_length );
                    return str;
                }

                /**
                 * @brief Retrieves a serialized `std::map<std::string,std::string>`'s entry.
                 * 
                 * @return std::pair<std::string,std::string> 
                 */
                std::pair<std::string,std::string> get_map_entry() {
                    std::string key = this->get_string_value();
                    std::string value = this->get_string_value();
                    // return std::pair<std::string,std::string>( key , value );
                    return std::pair<std::string,std::string>( key , value );
                }

                /**
                 * @brief Retrieves a serialized `std::map<std::string,std::string>`.
                 * 
                 * @return std::map<std::string,std::string> 
                 */
                std::map<std::string,std::string> get_map() {
                    const std::size_t length = this->next<std::size_t>();
                    std::map<std::string,std::string> map;
                    for (std::uint64_t i = 0; i < length; i++) {
                        map.insert( this->get_map_entry() );
                    }
                    return map;
                }

                /**
                 * @brief Verifies whether the serialization has or not more elements. 
                 * 
                 * @return true 
                 * @return false 
                 */
                const bool has_more() const {
                    return !this->file.eof();
                }

                /**
                 * @brief Returns the number of Type words that composes the serialization. 
                 * 
                 * @return const std::size_t 
                 */
                const std::size_t size() const {
                    return this->serialization_length;
                }

                /**
                 * @brief Closes binary file.
                 * 
                 */
                void close() {
                    this->file.close();
                }
        };
    }
}
