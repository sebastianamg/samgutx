#pragma once
#include <string>
#include <iostream>
#include <type_traits>

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

        enum FileFormat {
            MTX, // MatrixMarket plain-text format.
            K2T, // k2-tree binary format.
            MDX, // MultidimensionalMatrixMarket plain-text format.
            KNT, // kn-tree binary format.
            QMX, // QMX binary format.
            RRN, // Rice-runs binary format.
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

            return FileFormat::Unknown;
        }

        /**
         * @brief This function allows appending a string to and replace the extension of a file name. 
         * 
         * @param file_name 
         * @param to_append 
         * @param new_ext 
         * @return std::string 
         */
        std::string append_info_and_extension(const std::string file_name, const std::string to_append,const std::string new_ext) {
            std::string new_file_name = file_name;
            std::size_t position = new_file_name.find_last_of(".");
            if (position != std::string::npos) {
                new_file_name = new_file_name.substr(0,position) + "-" + to_append + "." + new_ext;
            } else {
                new_file_name += + "-" + to_append + "." + new_ext;
            }
            return new_file_name;
        }

        /**
         * @brief This function allows replacing the extension of a file name by a new one regardless the previous extension is. 
         * 
         * @param file_name 
         * @param new_ext 
         * @return std::string 
         */
        std::string change_extension(const std::string file_name, const std::string new_ext) {
            std::string new_file_name = file_name;
            std::size_t position = new_file_name.find_last_of(".") + 1UL;
            if (position != std::string::npos) {
                new_file_name.replace(position, new_ext.length(), new_ext);
            } else {
                new_file_name += "."+new_ext;
            }
            return new_file_name;
        }

        /**
         * @brief This function allows replacing a given old extension of file name by a new one. 
         * 
         * @param file_name 
         * @param old_ext 
         * @param new_ext 
         * @return std::string 
         */
        std::string change_extension(const std::string file_name, const std::string old_ext, const std::string new_ext) {
            std::string new_file_name = file_name;
            std::size_t position = new_file_name.find(old_ext);
            if (position != std::string::npos) {
                new_file_name.replace(position, new_ext.length(), new_ext);
            } else {
                new_file_name += ".knt";
            }
            return new_file_name;
        }

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
                    std::ofstream file(file_name, std::ios::binary);
                    if (file) {
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
                WordSequenceSerializer( std::vector<Type> sequence): 
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
                 * @brief This function allows adding a collection of l unsigned integer values. 
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
                        this->add_value(v);
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
    }
}
