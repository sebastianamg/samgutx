#pragma once
#include <string>
#include <iostream>

namespace samg {
    enum FileFormat {
        MTX,
        K2T,
        MDX,
        KNT,
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

        return FileFormat::Unknown;
    }

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
            WordSequenceSerializer(): current_index(0ULL) {}
            WordSequenceSerializer(const std::string file_name): current_index(0ULL) {
                this->sequence = WordSequenceSerializer::retrieve_binary_sequence<Type>(file_name);
            }

            template<typename TypeSrc, typename TypeTrg = Type> std::vector<TypeTrg> parse_values(std::vector<TypeSrc> V) {
                std::vector<TypeTrg> T;
                if( sizeof(TypeSrc) == sizeof(TypeTrg) ) {
                    T.insert(T.end(),V.begin(),V.end());
                } else { // sizeof(TypeSrc) != sizeof(Type)
                    const TypeTrg* x = reinterpret_cast<const TypeTrg*>(V.data());
                    std::size_t l = (std::size_t)std::ceil((double)(V.size()*sizeof(TypeSrc))/ (double)sizeof(TypeTrg));
                    T.insert(T.end(),x,x+l);
                }
                return T;
            }

            template<typename TypeSrc, typename TypeTrg=Type> std::vector<TypeTrg> parse_value(TypeSrc v) {
                std::vector<TypeSrc> V = {v};
                return this->parse_values<TypeSrc>(V);
            }

            void add_value(std::uint8_t v) {
                std::vector<Type> trg = this->parse_value<std::uint8_t>(v);
                this->sequence.insert(this->sequence.end(),trg.begin(),trg.end());
            }
            void add_value(std::uint16_t v) {
                std::vector<Type> trg = this->parse_value<std::uint16_t>(v);
                this->sequence.insert(this->sequence.end(),trg.begin(),trg.end());
            }
            void add_value(std::uint32_t v) {
                std::vector<Type> trg = this->parse_value<std::uint32_t>(v);
                this->sequence.insert(this->sequence.end(),trg.begin(),trg.end());
            }
            void add_value(std::uint64_t v) {
                std::vector<Type> trg = this->parse_value<std::uint64_t>(v);
                this->sequence.insert(this->sequence.end(),trg.begin(),trg.end());
            }
            template<typename TypeSrc> void add_values(const TypeSrc *v, const std::size_t l) {
                std::vector<TypeSrc> V;
                V.insert(V.end(),v,v+l);
                this->add_values(V);
            }
            template<typename TypeSrc> void add_values(const std::vector<TypeSrc> V) {
                std::vector<Type> X = this->parse_values<TypeSrc>(V);
                this->sequence.insert(this->sequence.end(),X.begin(),X.end());
            }
            void add_value(std::string v) {
                std::vector<Type> V = WordSequenceSerializer::convert_string_to_vector<Type>(v);
                this->add_value(v.length()); // Storing the original length (number of characters/bytes) of the key. 
                this->add_value(V.size()); // Storing the length (number of words<Type>) of the key. 
                this->sequence.insert(this->sequence.end(), V.begin(), V.end());
            }
            void add_map_entry(const std::pair<std::string,std::string> p) {
                // Adding key:
                this->add_value(p.first);
                // Adding value:
                this->add_value(p.second);
            }
            void add_map(std::map<std::string,std::string> m) {
                this->add_value(m.size());
                for (std::pair<std::string,std::string> p : m) {
                    this->add_map_entry(p);
                }
            }

            void save(const std::string file_name) {
                WordSequenceSerializer::store_binary_sequence<Type>(this->sequence,file_name);
            }

            std::vector<Type> get_remaining_values() {
                std::vector<Type> V;
                while(this->has_more()) {
                    V.push_back(this->get_value());
                }
                return V;
            }

            std::vector<Type> get_next_values(std::uint64_t length) {
                std::vector<Type> V ;
                for (std::uint64_t i = 0 ; i < length; i++) {
                    V.push_back(this->get_value());
                }
                return V;
            }

            std::vector<Type> get_values(std::uint64_t beginning_index, std::uint64_t length) const {
                std::vector<Type> V;
                for (std::uint64_t i = beginning_index ; i < (beginning_index+length); i++) {
                    V.push_back(this->get_value(i));
                }
                return V;
            }

            const Type get_value(const std::uint64_t i) const {
                return this->sequence[i];
            }

            const Type get_value() {
                return this->sequence[this->current_index++];
            }

            std::string get_string_value() {
                const std::uint64_t bytes_length = this->get_value(),
                                    words_legnth = this->get_value();
                std::vector<Type> V = this->get_next_values(words_legnth);
                std::string str = WordSequenceSerializer::convert_vector_to_string<Type>(V,bytes_length);
                return str;
            }

            std::pair<std::string,std::string> get_map_entry() {
                std::string key = this->get_string_value();
                std::string value = this->get_string_value();
                return std::pair<std::string,std::string>(key,value);
            }

            std::map<std::string,std::string> get_map() {
                const std::uint64_t length = this->get_value();
                std::map<std::string,std::string> map;
                for (std::uint64_t i = 0; i < length; i++) {
                    map.insert(this->get_map_entry());
                }
                return map;
            }

            const bool has_more() {
                return this->current_index < this->sequence.size();
            }

            const std::uint64_t get_current_index() {
                return this->current_index;
            }

            const std::uint64_t size() const {
                return this->sequence.size();
            }

            const void print() {
                std::cout << "WordSequenceSerializer --- sequence: " << std::endl;
                for (Type v : this->sequence) {
                    std::cout << v << std::endl;
                }
                
            }
    };
}
