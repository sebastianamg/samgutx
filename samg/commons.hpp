#pragma once
#include <string>
#include <sstream>
#include <iostream>
#include <fstream>
#include <cstdint>
#include <iomanip>
#include <vector>
#include <queue>

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
        template<typename UINT_T = std::uint32_t> void print_vector(const std::string info, const std::vector<UINT_T> v, bool new_line = true) {
            std::cout << info;
            for (UINT_T x : v) {
                std::cout << x << " "; 
            }
            if( new_line ){
                std::cout << std::endl;
            }
        }
        template<typename UINT_T = std::uint32_t> void print_queue(const std::string info, std::queue<UINT_T> q, bool new_line = true) {
            std::cout << info;
            while( !q.empty() ) {
                std::cout << q.front() << " "; 
                q.pop();
            }
            if( new_line ){
                std::cout << std::endl;
            }
        }
        template<typename UINT_T = std::uint32_t> void print_array( const std::string info, UINT_T *buff, std::size_t length, bool new_line = true ) {
            std::cout << info;
            for (std::size_t i = 0; i < length; ++i) {
                std::cout << buff[i] << " "; 
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
            ss.imbue(std::locale("en_US.UTF-8"));  // Use the appropriate locale for your system
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

            while( repr.length() < length ){
                repr = "0" + repr;
            }

            return repr;
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
}