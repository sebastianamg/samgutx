#pragma once
#include <string>
#include <sstream>
#include <cstdint>
#include <iomanip>

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
        template<typename T=std::uint64_t> std::string to_string(std::vector<T> obj) {
            std::ostringstream ss;
            for(std::size_t i=0;i<obj.size();i++) {
                ss << obj[i] << ((i<obj.size()-1)?", ":"");
            }
            return ss.str();
        }
        /***************************************************************/
        template<typename UINT_T = std::uint32_t> void print_vector(const std::string info, const std::vector<UINT_T> v) {
            std::cout << info << std::endl;
            for (UINT_T x : v) {
                std::cout << x << " "; 
            }
            std::cout << std::endl;
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
         * @brief This function is an auxiliar function to convert time into a comma-separated string representation.
         * 
         * @param time 
         * @return std::string 
         */
        std::string convert_time_to_string(const std::double_t time) {
            std::stringstream ss;
            ss.imbue(std::locale("en_US.UTF-8"));  // Use the appropriate locale for your system
            ss << std::fixed << std::setprecision(0) << time;
            return ss.str();
        }
        /***************************************************************/
    }
}