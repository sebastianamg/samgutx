#pragma once
#include <string>
#include <sstream>
#include <iostream>
#include <fstream>
#include <cstdint>
#include <iomanip>
#include <vector>
#include <queue>
#include <map>
#include <type_traits>
#include <typeinfo>
#include <cmath>
// #include <set>
#include <boost/algorithm/string.hpp>
// #include <boost/range.hpp>

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
 * 
 * @warning This header depends on Boost string Library!
 */

namespace samg {
    namespace utils {
        /***************************************************************/
        /**
         * Artifact for feedback/debugging purposes. 
         * @brief This function allows getting a string representation of an object.
         *  
         * @param obj is the input object.
         * 
         * @return string representation of obj.
        */
        template<typename T> std::string to_string(T obj) {
            std::ostringstream ss;
            ss << obj;
            return ss.str();
        }
        /***************************************************************/
        /**
         * @brief This method allows converting a vector of objects into its string representation. 
         * 
         * @tparam T is a data type.
         * @param obj is vector of objects of type T.
         * @return std::string 
         */
        template<typename T=std::uint64_t> std::string to_string(std::vector<T> obj, std::string separator = ", ") {
            std::ostringstream ss;
            for(std::size_t i=0;i<obj.size();i++) {
                ss << obj[i] << ( (i<obj.size()-1)? separator : "" );
            }
            return ss.str();
        }
        /***************************************************************/
        template<typename UINT_T = std::uint32_t> void print_vector(const std::string info, const std::vector<UINT_T> v, const bool new_line = true, const std::string separator=" ", const std::string to_remove="") {
            std::cout << info;
            for (UINT_T x : v) {
                std::string y = to_string<UINT_T>(x);
                for (char c : to_remove) {
                    y.erase(std::remove(y.begin(), y.end(), c), y.end());
                }
                std::cout << y << separator; 
            }
            if( new_line ){
                std::cout << std::endl;
            }
        }

        /**
         * @brief Prints a queue<UINT_T>.
         * 
         * @tparam UINT_T 
         * @param info 
         * @param q 
         * @param new_line 
         * @param separator 
         * @warning SIDE EFFECT!!! `q` is destroyed as the function goes through it!
         */
        template<typename UINT_T = std::uint32_t> void print_queue(const std::string info, std::queue<UINT_T> q, bool new_line = true, std::string separator=" " ) {
            std::cout << info;
            while( !q.empty() ) {
                std::cout << q.front() << separator; 
                q.pop();
            }
            if( new_line ){
                std::cout << std::endl;
            }
        }
        template<typename UINT_T = std::uint32_t> void print_array( const std::string info, const UINT_T *buff, std::size_t length, bool new_line = true, std::string separator=" " ) {
            std::cout << info;
            for (std::size_t i = 0; i < length; ++i) {
                std::cout << buff[i] << separator; 
            }
            if( new_line ){
                std::cout << std::endl;
            }
        }

        template<typename TypeA,typename TypeB> void print_map( const std::string info, const std::map<TypeA,TypeB> map, bool new_line = true, std::string pair_separator=" = ", std::string separator="; " ) {
            std::cout << info;
            for (const std::pair<TypeA,TypeB>& p : map) {
                std::cout << p.first << pair_separator << p.second << separator; 
            }
            if( new_line ){
                std::cout << std::endl;
            }
        }
        /***************************************************************/
        template <typename InputIterator, typename Container>
        void copy_to_stack(InputIterator begin, InputIterator end, Container& stack) {
            while (begin != end) {
                stack.push(*begin);
                begin++;
            }
        }
        /***************************************************************/
        /**
         * @brief This function is an auxiliar function to convert a number into a comma-separated string representation.
         * 
         * @param n 
         * @return std::string 
         */
        std::string number_to_comma_separated_string( const double n, const std::size_t precision = 0 ) {
            std::stringstream ss;
            // ss.imbue(std::locale("en_US.UTF-8"));  // Use the appropriate locale for your system
            ss.imbue(std::locale("C"));  // Use the appropriate locale for your system
            ss << std::fixed << std::setprecision(precision) << n;
            return ss.str();
        }
        /***************************************************************/
        /**
         * @brief This function returns the size of a file. 
         * 
         * @param file_name 
         * @return std::size_t 
         */
        std::size_t get_file_size( const std::string file_name ) {
            // Open the file in read mode:
            std::ifstream file(file_name, std::ios::binary);
            // Get the file size:
            file.seekg(0, std::ios::end);
            std::size_t size = file.tellg();
            // Close the file:
            file.close();
            return size;
        }
        /***************************************************************/
        /**
         * @brief This function returns the text from a file. 
         * 
         * @param infile 
         * @return std::string
         * 
         * @warning Bear in mind the size of the text. 
         */
        inline std::string read_from_file(const std::string infile) {
            std::ifstream instream(infile);
            if (!instream.is_open()) {
                throw std::runtime_error("Couldn't open the file \""+infile+"\"!");
            }
            instream.unsetf(std::ios::skipws);      // No white space skipping!
            std::string txt = std::string(std::istreambuf_iterator<char>(instream.rdbuf()),
                std::istreambuf_iterator<char>());
            instream.close();
            return txt;
        }
        /***************************************************************/
        /**
         * @brief This funtion converts an encoded number from base `base` to base 10.
         * 
         * @param str 
         * @param base 
         * @return std::uint64_t 
         */
        std::uint64_t from_base(std::string str, int base = 10) {
            if (base < 2 or base > 36) {
                throw std::invalid_argument("base " + std::to_string(base) + " is not between 2 and 36");
            }
            std::uint64_t number = 0;
            for (char& c : str) {
                int digit = c;
                if (digit < 58) {
                    digit -= 48;
                }
                else if (digit < 91)
                {
                    digit -= 55;
                }
                else {
                    digit -= 87;
                }
                if (digit < 0 or digit >= base) {
                    throw std::invalid_argument( "input string is not a valid integer in base " + std::to_string(base) );
                }
                number = number * base + digit;
            }
            return number;
        }

        /**
         * @brief This function convert a number from base 10 to base `base`.
         * 
         * @param number 
         * @param base 
         * @param length 
         * @return std::string 
         */
        const std::string to_base(std::uint64_t number, const int base, const std::size_t length = 0 ) {
            if (base < 2 or base > 36) {
                throw std::invalid_argument("base " + std::to_string(base) + " is not between 2 and 36");
            }
            std::string repr = "";
            int digit;
            while (number) {
                std::uint64_t quotient = number / base;
                digit = number - quotient * base;
                number = quotient;
                if (digit < 10) {
                    digit += 48;
                }
                else {
                    digit += 87;
                }
                repr = char(digit) + repr;
            }
            // std::cout << "to_base("<<number<<") = " << repr << std::endl;
            while( repr.length() < length ){
                repr = "0" + repr;
            }
            // std::cout << "to_base("<<number<<") = (with "<<length<<" 0s) " << repr << std::endl;

            return repr;
        }

        /**
         * @brief Converts from n-dimensional coordinates of order `base` to z-order. 
         * 
         * @param c 
         * @param base 
         * @param len is the number of bits to correctly represent each element in the coordinate. 
         * @return std::size_t 
         */
        std::size_t to_zvalue( const std::vector<std::uint64_t>& c, const std::size_t base, const std::size_t len ) {

            // For each element in `c`, convert to base `base`:
            std::vector<std::string> p;
            for ( std::uint64_t v : c ) {
                p.push_back( samg::utils::to_base( v, base, len ) );
            }

            // Concatenate elements in `p` interleaving-wise:
            std::string v = "";
            for (size_t i = 0; i < len; i++) {
                for( std::string s : p ) {
                    v = v + s[i];
                }
            }

            // Convert `v` from base `base` to base 10:
            std::uint64_t zv_ans = samg::utils::from_base( v, base );

            return zv_ans;
        }

        /**
         * @brief Convert `z_value` into `dims`-dimensional coordinates:
         * 
         * @param zvalue 
         * @param base 
         * @param dims 
         * @param len is the sum of number of bits that represent each element in the sought coordinate. The number of bits must be must be divisible by `dims`.
         * @return std::vector<std::uint64_t> 
         */
        std::vector<std::uint64_t> from_zvalue( const std::size_t zvalue, const std::size_t base, const std::size_t dims, const std::size_t len ) {
            // Convert zv from base 10 to base `base` and ensure length `len`: 
            const std::string zv_k = samg::utils::to_base( zvalue, base, len );

            // Separate components from zv_k:
            std::vector<std::string> zv_k_components( dims );
            for (std::size_t i = 0; i < zv_k.length(); i++) {
                zv_k_components[i%dims] += zv_k[i];
            }

            // Convert each component back to create a coordinate:
            std::vector<std::uint64_t> c;
            for (std::string c_zv : zv_k_components) {
                c.push_back( samg::utils::from_base( c_zv, base ) );
            }
            
            return c;
        }
        /***************************************************************/
        /**
         * @brief This function allows appending a string to and replace the extension of a file name. 
         * 
         * @param file_name 
         * @param to_append 
         * @param new_ext 
         * @return std::string 
         */
        std::string append_info_and_extension(const std::string file_name, const std::string to_append, std::string new_ext) {
            std::size_t position = new_ext.find(".");
            if (position == std::string::npos) {
                new_ext = "." + new_ext;
            }

            std::string new_file_name = file_name;
            position = new_file_name.find_last_of(".");
            if (position != std::string::npos) {
                new_file_name = new_file_name.substr(0,position) + to_append + new_ext;
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
        std::string change_extension(const std::string file_name, std::string new_ext) {
            std::size_t position = new_ext.find(".");
            if (position == std::string::npos) {
                new_ext = "." + new_ext;
            }

            std::string new_file_name = file_name;
            position = new_file_name.find_last_of(".");
            if (position != std::string::npos) {
                new_file_name.replace(position, new_file_name.length(), "");
            }
            new_file_name += new_ext;
            return new_file_name;
        }

        /**
         * @brief This function returns the file base name.
         * 
         * @param file_name 
         * @return std::string 
         */
        std::string get_file_basename(const std::string file_name) {
            std::string new_file_name = file_name;
            std::size_t position = new_file_name.find_last_of(".");
            if (position != std::string::npos) {
                new_file_name.replace(position, file_name.length() - position, "");
            }
            return new_file_name;
        }

        /**
         * @brief This function allows replacing a given old extension of file name by a new one and append a string before the new extension. 
         * 
         * @param file_name 
         * @param old_ext 
         * @param new_ext 
         * @param to_append 
         * @return std::string 
         */
        std::string change_extension(const std::string file_name, std::string old_ext, std::string new_ext, const std::string to_append="") {
            std::size_t position = old_ext.find(".");
            if (position == std::string::npos) {
                old_ext = "." + old_ext;
            }
            position = new_ext.find(".");
            if (position == std::string::npos) {
                new_ext = "." + new_ext;
            }

            std::string new_file_name = file_name;
            position = new_file_name.find(old_ext);
            if (position != std::string::npos) {
                new_file_name = new_file_name.replace(position, new_file_name.length(), "");
            }
            new_file_name += to_append + new_ext;
            return new_file_name;
        }
        /***************************************************************/
    }

    namespace serialization { 
        /**
         * @brief This function allows parsing integer values stored in an input vector of type TypeSrc into type TypeTrg.
         * 
         * @tparam TypeSrc 
         * @tparam TypeTrg 
         * @param V 
         * @return std::vector<TypeTrg> 
         */
        template<typename TypeSrc, typename TypeTrg> std::vector<TypeTrg> parse_values(const std::vector<TypeSrc>& V) {
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
         * @brief This function allows converting a std::string into a std::vector<T> 
         * 
         * @tparam T is the output std::vector data type.
         * @param str 
         * @return std::vector<T> 
         */
        template<typename T> static std::vector<T> convert_string_to_vector(const std::string& str) {
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

                // /**
                //  * @brief This function allows parsing integer values stored in an input vector of type TypeSrc into type TypeTrg.
                //  * 
                //  * @tparam TypeSrc 
                //  * @tparam TypeTrg 
                //  * @param V 
                //  * @return std::vector<TypeTrg> 
                //  */
                // template<typename TypeSrc, typename TypeTrg = Type> std::vector<TypeTrg> parse_values(std::vector<TypeSrc> V) {
                //     std::vector<TypeTrg> T;
                //     if( sizeof(TypeTrg) == sizeof(TypeSrc) ) {
                //         T.insert(T.end(),V.begin(),V.end());
                //     } else { // sizeof(TypeSrc) != sizeof(Type)
                //         const TypeTrg* x = reinterpret_cast<const TypeTrg*>(V.data());
                //         std::size_t l = (std::size_t)std::ceil((double)(V.size()*sizeof(TypeSrc)) / (double)sizeof(TypeTrg));
                //         T.insert(T.end(),x,x+l);
                //     }
                //     return T;
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
                    std::vector<Type> X = serialization::parse_values<TypeSrc,Type>(V);
                    this->sequence.insert(this->sequence.end(),X.begin(),X.end());
                }

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
                }

                /**
                 * @brief This function allows serializing a string.
                 * 
                 * @param v 
                 */
                void add_value(std::string v) {
                    std::vector<Type> V = serialization::convert_string_to_vector<Type>(v);
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
                    for (std::uint64_t i = 0 ; i < length ; i++) {
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
                    std::vector<Type> V;
                    std::uint64_t limit = std::ceil( sizeof(TypeTrg) / sizeof(Type) );
                    for ( ; i < limit; ++i ) {
                        V.push_back( this->sequence[i] );
                    }
                    std::vector<TypeTrg> T = serialization::parse_values<Type,TypeTrg>(V);
                    return T[0]; // Assuming T contains only one value.
                }

                /**
                 * @brief This function allows retrieving the next value, based on an internal index. 
                 * 
                 * @tparam TypeTrg 
                 * @return const TypeTrg 
                 */
                template<typename TypeTrg = Type> const TypeTrg get_value() {
                    std::vector<Type> V;
                    std::uint64_t limit = std::ceil( (double)sizeof(TypeTrg) / (double)sizeof(Type) );
                    for (std::size_t i = 0; i < limit; ++i ) {
                        V.push_back( this->sequence[this->current_index++] );
                    }
                    std::vector<TypeTrg> T = serialization::parse_values<Type,TypeTrg>(V);
                    return T[0]; // Assuming T contains only one value.
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
                    std::string str = serialization::convert_vector_to_string<Type>(V,bytes_length);
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
         * @brief The class OfflineWordWriter allows direct offline unsigned integer sequence serialization directly to a file. 
         * 
         * @tparam Type 
         */
        template<typename Type> class OfflineWordWriter {
            static_assert(
                std::is_same_v<Type, std::uint8_t> ||
                std::is_same_v<Type, std::uint16_t> ||
                std::is_same_v<Type, std::uint32_t> ||
                std::is_same_v<Type, std::uint64_t>,
                "typename must be one of std::uint8_t, std::uint16_t, std::uint32_t, or std::uint64_t");
            private:
                const std::size_t BITS_PER_BYTE = 8UL;
                const std::size_t WORD_SIZE = sizeof(Type)  * BITS_PER_BYTE;
                std::size_t type_word_counter;
                const std::string file_name;
                std::ofstream file;

                /**
                 * @brief Serializes a sequence of unsigned integers.
                 * 
                 * @tparam UINT_T 
                 * @param data 
                 * @param file_name 
                 */
                template<typename UINT_T> static void _write_(const std::vector<UINT_T>& data, std::ofstream& file) { 
                    if ( file.is_open() ) {
                        // Writing the vector's data to the file
                        file.write(reinterpret_cast<const char*>(data.data()), data.size() * sizeof(UINT_T));
                    } else {
                        throw std::runtime_error("Failed to write to file!");
                    }
                }

            public:
                /**
                 * @brief Constructs a new offline word serializer object.
                 * 
                 * @param file_name
                 */
                OfflineWordWriter(const std::string file_name): 
                    file_name ( file_name ),
                    file ( std::ofstream(file_name, std::ios::binary) ),
                    type_word_counter (0ULL) {
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
                template<typename TypeSrc> void add_value(TypeSrc v) {
                    static_assert(
                        std::is_same_v<TypeSrc, std::uint8_t> ||
                        std::is_same_v<TypeSrc, std::uint16_t> ||
                        std::is_same_v<TypeSrc, std::uint32_t> ||
                        std::is_same_v<TypeSrc, std::uint64_t>,
                        "typename must be one of std::uint8_t, std::uint16_t, std::uint32_t, or std::uint64_t");
                    // std::cout << "OfflineWordWriter / add_value> v = " << v << std::endl;

                    std::vector<TypeSrc> V = {v};
                    std::vector<Type> X = serialization::parse_values<TypeSrc,Type>(V);
                    OfflineWordWriter<Type>::_write_<Type>( X, this->file );
                    this->type_word_counter += X.size();
                }

                /**
                 * @brief Serializes a string.
                 * 
                 * @param v 
                 */
                void add_string(const std::string& v) {
                    std::vector<Type> V = serialization::convert_string_to_vector<Type>( v );
                    this->add_value<std::size_t>( v.length() ); // Storing the original length (number of characters/bytes) of the key. 
                    this->add_value<std::size_t>( V.size() ); // Storing the length (number of words<Type>) of the key. 
                    this->add_values<Type>( V );
                }

                /**
                 * @brief Adds a collection of l TypeSrc integer values. 
                 * 
                 * @tparam TypeSrc 
                 * @param v 
                 * @param l 
                 */
                template<typename TypeSrc> void add_values(const TypeSrc *v, const std::size_t l) {
                    // std::cout << "OfflineWordWriter/add_values> v = " << v << "; l = " << l << std::endl;
                    for (std::size_t i = 0; i < l; ++i) {
                        // std::cout << "OfflineWordWriter/add_values> (2) v[" << i << " / " << l << " ] = " << v[i] << std::endl;
                        this->add_value<TypeSrc>(v[i]);
                    }
                    // std::vector<TypeSrc> V;
                    // V.insert(V.begin(),v,v+l);
                    // std::cout << "OfflineWordWriter/add_values> (3)" << std::endl;
                    // this->add_values<TypeSrc>(V);
                    // std::cout << "OfflineWordWriter/add_values> (4)" << std::endl;
                }

                /**
                 * @brief Adds a collection of unsigned integer values. 
                 * 
                 * @tparam TypeSrc 
                 * @param V 
                 */
                template<typename TypeSrc> void add_values(const std::vector<TypeSrc>& V) {
                    for (TypeSrc v : V) {
                        this->add_value<TypeSrc>(v);
                    }
                }

                /**
                 * @brief Adds a `std::map<std::string,std::string>` entry.
                 * 
                 * @param p map<std::string,std::string>'s entry
                 */
                template<typename TypeA, typename TypeB> void add_map_entry(const std::pair<TypeA,TypeB>& p) {
                    static_assert(std::is_integral<TypeA>::value, "TypeA must be an integral type.");
                    static_assert(std::is_integral<TypeB>::value, "TypeB must be an integral type.");
                    // Adding key:
                    this->add_value<TypeA>(p.first);
                    // Adding value:
                    this->add_value<TypeB>(p.second);
                }
                template<typename TypeA = std::string, typename TypeB> void add_map_entry(const std::pair<std::string,TypeB>& p) {
                    static_assert(std::is_same_v<TypeA, std::string>, "TypeA must be std::string type.");
                    static_assert(std::is_integral<TypeB>::value, "TypeB must be an integral type.");
                    // Adding key:
                    this->add_string(p.first);
                    // Adding value:
                    this->add_value<TypeB>(p.second);
                }
                template<typename TypeA, typename TypeB = std::string> void add_map_entry(const std::pair<TypeA,std::string>& p) {
                    static_assert(std::is_integral<TypeA>::value, "TypeA must be an integral type.");
                    static_assert(std::is_same_v<TypeB, std::string>, "TypeB must be std::string type.");
                    
                    // Adding key:
                    this->add_value<TypeA>(p.first);
                    // Adding value:
                    this->add_string(p.second);
                }
                template<typename TypeA = std::string,typename TypeB = std::string> void add_map_entry(const std::pair<std::string,std::string>& p) {
                    static_assert(std::is_same_v<TypeA, std::string>, "TypeA must be std::string type.");
                    static_assert(std::is_same_v<TypeB, std::string>, "TypeB must be std::string type.");
                    // Adding key:
                    this->add_string(p.first);
                    // Adding value:
                    this->add_string(p.second);
                }
                // template<typename TypeA, typename TypeB> void add_map_entry(const std::pair<TypeA,TypeB>& p) {
                //     // Adding key:
                //      if( std::is_same<TypeA,std::string>::value ) {
                //         this->add_string(p.first);
                //     } else /*if( std::is_integral<TypeA>::value ) */{
                //         this->add_value<TypeA>(p.first);
                //     } /*else {
                //         throw std::runtime_error("\""+ std::string(typeid(TypeA).name()) +"\" is an unsuported datatype!");
                //     }*/
                    
                //     // Adding value:
                //     if( std::is_same<TypeB,std::string>::value ) {
                //         this->add_string(p.second);
                //     } else /*if( std::is_integral<TypeB>::value ) */{
                //         this->add_value<TypeB>(p.second);
                //     } /*else {
                //         throw std::runtime_error("\""+ std::string(typeid(TypeB).name()) +"\" is an unsuported datatype!");
                //     }*/
                // }

                /**
                 * @brief Adds a `map<std::string,std::string>`.
                 * 
                 * @param m 
                 */
                template<typename TypeA, typename TypeB> void add_map(const std::map<TypeA,TypeB>& m) {
                    this->add_value<std::size_t>( m.size() );
                    for (const std::pair<TypeA,TypeB>& p : m) {
                        this->add_map_entry<TypeA,TypeB>(p);
                    }
                }

                /**
                 * @brief Returns the number of written `Type` words. 
                 * 
                 * @return const std::size_t 
                 */
                const std::size_t size() const {
                    return this->type_word_counter;
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
         * @brief Allows a direct unsigned integers sequence offline reading from a binary file. 
         * 
         * @tparam Type 
         */
        template<typename Type> class OfflineWordReader {
            static_assert(
                std::is_same_v<Type, std::uint8_t> ||
                std::is_same_v<Type, std::uint16_t> ||
                std::is_same_v<Type, std::uint32_t> ||
                std::is_same_v<Type, std::uint64_t>,
                "typename must be one of std::uint8_t, std::uint16_t, std::uint32_t, or std::uint64_t");
            private:
                const std::size_t   BITS_PER_BYTE = 8UL,
                                    WORD_SIZE = sizeof(Type)  * BITS_PER_BYTE;
                const std::string file_name;
                std::ifstream file;
                std::size_t serialization_length;

                /**
                 * @brief Allows retrieving a serialized unsigned integer from a binary file.
                 * 
                 * @tparam UINT_T 
                 * @param file_name 
                 * @return UINT_T
                 */
                template<typename UINT_T> UINT_T _read_( std::ifstream& file ) {
                    return OfflineWordReader<Type>::_read_<UINT_T>( file, 1ULL )[0];
                }

                template<typename UINT_T> std::vector<UINT_T> _read_( std::ifstream& file, const std::size_t nwords ) {
                    if ( !file.is_open() ) {
                        throw std::runtime_error("The file is closed!");
                    }
                    
                    if ( file.eof() ) {
                        throw std::runtime_error("The file has no more data!");
                    }

                    std::vector<UINT_T> data = std::vector<UINT_T>( nwords );
                    
                    // Read the data from the file into the vector
                    file.read( reinterpret_cast<char*>(data.data()), sizeof( UINT_T ) * nwords );
                    
                    return data;
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
                template<typename TypeTrg> const TypeTrg next() {
                    return OfflineWordReader<Type>::_read_<TypeTrg>( this->file );
                }

                /**
                 * @brief Allows retrieving the next `length` values.
                 * 
                 * @tparam TypeTrg 
                 * @param length 
                 * @return const std::vector<TypeTrg> 
                 */
                template<typename TypeTrg> const std::vector<TypeTrg> next(std::size_t length) {
                    // std::vector<TypeTrg> V;
                    // for (std::uint64_t i = 0 ; i < length ; i++) {
                    //     V.push_back(this->next<TypeTrg>());
                    // }
                    // return V;
                    return OfflineWordReader<Type>::_read_<TypeTrg>( this->file, length );
                }

                /**
                 * @brief Allows getting all the remaining values from the serialization. 
                 * 
                 * @tparam TypeTrg 
                 * @return const std::vector<TypeTrg> 
                 */
                template<typename TypeTrg> const std::vector<TypeTrg> next_remaining() {
                    std::vector<TypeTrg> V;
                    while( this->has_more() ) {
                        V.push_back(this->next<TypeTrg>());
                    }
                    return V;
                }

                /**
                 * @brief Retrieves a serialized string. 
                 * 
                 * @return std::string 
                 */
                std::string next_string() {
                    const std::size_t   bytes_length = this->next<std::size_t>(),
                                        words_legnth = this->next<std::size_t>();
                    std::vector<Type> V = this->next<Type>( words_legnth );
                    std::string str = serialization::convert_vector_to_string<Type>( V , bytes_length );
                    return str;
                }

                /**
                 * @brief Retrieves a serialized `std::map<std::string,std::string>`'s entry.
                 * 
                 * @return std::pair<TypeA,TypeB> 
                 */
                template<typename TypeA, typename TypeB> std::pair<TypeA,TypeB> next_map_entry() {
                    TypeA key;
                    if constexpr(std::is_integral_v<TypeA>) {
                        key = this->next<TypeA>();
                    } else if constexpr(std::is_same_v<TypeA, std::string>) {
                        key = this->next_string();
                    } else {
                        throw std::runtime_error("\""+ std::string(typeid(TypeA).name()) +"\" is an unsuported datatype!");
                    }
                    TypeB value;
                    if constexpr(std::is_integral_v<TypeB>) {
                        value = this->next<TypeB>();
                    } else if constexpr(std::is_same_v<TypeB, std::string>) {
                        value = this->next_string();
                    } else {
                        throw std::runtime_error("\""+ std::string(typeid(TypeB).name()) +"\" is an unsuported datatype!");
                    }
                    // return std::pair<std::string,std::string>( key , value );
                    return std::make_pair( key , value );
                }

                /**
                 * @brief Retrieves a serialized `std::map<std::string,std::string>`.
                 * 
                 * @return std::map<std::string,std::string> 
                 */
                template<typename TypeA, typename TypeB> std::map<TypeA,TypeB> get_map() {
                    const std::size_t length = this->next<std::size_t>();
                    std::map<TypeA,TypeB> map;
                    for (std::uint64_t i = 0; i < length; i++) {
                        map.insert( this->next_map_entry<TypeA,TypeB>() );
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