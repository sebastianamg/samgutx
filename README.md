[![License: BSD 2-Clause](https://img.shields.io/badge/License-BSD%202--Clause-red.svg)](https://opensource.org/licenses/BSD-2-Clause)


# Introduction

The following is the detail of one-header libraries available in this repository:
* `QMX`:
  * `qmx.hpp`: It is an unsigned 32-bits integer encoder/decoder modified from [@amallia's QMX repository](https://github.com/amallia/QMX). A mechanism to get a set of integers from a 128-bits `SIMD` word at a time was added. Copyright © 2014-2017 Andrew Trotman under the [BSD-2-Clause](https://opensource.org/license/bsd-2-clause/) licence. The implementation is based on:
  > A. Trotman and J. Lin, "In Vacuo and In Situ Evaluation of SIMD Codecs," in Proceedings of the 21st Australasian Document Computing Symposium, in ADCS ’16. New York, NY, USA: Association for Computing Machinery, Dec. 2016, pp. 1–8. doi: 10.1145/3015022.3015023. 
* `samg`:
  * `commons.hpp`: It contains methods for converting objects and vectors to string (the object requires implementing `operator<<`), printing a vector, copying data to a `std::stack` from a range defined by `begin` and `end` iterators, and converting time (`std::double_t`) to string.  
  * `logger.hpp`: It contains a wrap class for [`c-logger`](https://github.com/adaxiik/c-logger) implemented by @adaxiik. The wrapper class, called `samg::Logger` provides with methods to output `debug`, `info`, `warn`, `error`, and `fatal` messages, along with a method to directly output a `stdout` message. Furthermore, it provides with a mechanism to "turn" on and off the logger. Since this is a one-header file, @adaxiik's `c-logger` has been replicated within the file. 
  * `matutx.hpp`: It provides functions and a class to serialize a sequence of integers called `WordSequenceSerializer`. Available functions and a class are as follows:
    * `FileFormat identify_file_format(const std::string file_name)`: This function allows identifying a file extension based on the input file name. Available formats are defined by the `enum FileFormat`. 
    * `std::string append_info_and_extension(const std::string file_name, const std::string to_append,const std::string new_ext)`: This function allows appending a string to and replace the extension of a file name. 
    * `std::string change_extension(const std::string file_name, const std::string new_ext)`: This function allows replacing the extension of a file name by a new one, regardless the previous extension is.
    * `std::string change_extension(const std::string file_name, const std::string old_ext, const std::string new_ext)`: This function allows replacing a given old extension of file name by a new one.
    * `WordSequenceSerializer` class: This class allows serializing and deserializing a sequence of integers defined through its template. The class provides the following members:
      * `WordSequenceSerializer()`: This constructor gets the serializer ready to start a new serialization. 
      *  `WordSequenceSerializer(const std::string file_name)`: This constructor gets the serializer ready to deserialize data from a binary file.
      * `template<typename TypeSrc, typename TypeTrg = Type> std::vector<TypeTrg> parse_values(std::vector<TypeSrc> V)`: This function allows parsing integer values stored in an input vector of type TypeSrc into type TypeTrg.
      * `template<typename TypeSrc, typename TypeTrg=Type> TypeTrg parse_value(TypeSrc v)`: This function allows parsing an integer value of type TypeSrc into type TypeTrg.
      * `template<typename TypeSrc> void add_value(TypeSrc v)`: This function allows adding an `8`/`16`/`32`/`64`-bits value. 
      * `template<typename TypeSrc> void add_values(const TypeSrc *v, const std::size_t l)`: This function allows adding a collection of l unsigned integer values.
      * `template<typename TypeSrc> void add_values(const std::vector<TypeSrc> V)`: This function allows adding a collection of unsigned integer values. 
      * `void add_value(std::string v)`: This function allows serializing a string.
      * `void add_map_entry(const std::pair<std::string,std::string> p)`: This function allows adding a `std::map<std::string,std::string>` entry.
      * `void add_map(std::map<std::string,std::string> m)`: This function allows adding a `map<std::string,std::string>`.
      * `void save(const std::string file_name)`: This function allows saving the serialization into a given file. 
      * `std::vector<Type> get_remaining_values()`: This function allows getting remaining values from the serialization, starting from where an internal index is.
      * `std::vector<Type> get_next_values(std::uint64_t length)`: This function allows retrieving the next `length` values. 
      * `std::vector<Type> get_values(std::uint64_t beginning_index, std::uint64_t length)`: This function allows retrieving the next `length` values starting at `beginning_index`.
      * `const Type get_value(const std::uint64_t i) const`: This function allows getting the `i`-th value from the serialization. 
      * `const Type get_value()`: This function allows retrieving the next value, based on an internal index. 
      * `std::string get_string_value()`: This function allows retrieving a serialized string. 
      * `std::pair<std::string,std::string> get_map_entry()`: This function allows retrieving a serialized `std::map<std::string,std::string>`'s entry.
      * `std::map<std::string,std::string> get_map()`: This function allows retrieving a serialized `std::map<std::string,std::string>`.
      * `const bool has_more() const`: This method allows verifying whether the serialization has or not more elements. 
      * `const std::uint64_t get_current_index() const`: This method returns the current internal index.
      * `const std::uint64_t size() const`: This method returns the number of Type words that composes the serialization. 
      * `const void print()`: This method displays the sequence of Type words that compose the serialization.
* `GRCodec`: 
  * `gr-codec.hpp`: It contains an implementation of Golomb-Rice codec. It requires [sdsl-lite](https://github.com/simongog/sdsl-lite) library!

# Examples

## `GRCodec/samg::grcodec::GRCodec`

```c++
// test.cpp
// To compile: `g++-11 -ggdb -g3 -I ~/include/ -L ~/lib/ trial_bitvector.cpp -o trial_bitvector -lsdsl`
#include <GRCodec/gr-codec.hpp>
#include <iostream>
#include <sstream>

void show_bitvector(sdsl::bit_vector v) {
    for (std::size_t i = v.size()-1; 0 <= i && i < v.size() ; --i) {
        std::cout << v[i];
    }
}

int main(int argc, char const *argv[]) {
    const std::size_t m = 7;
    std::cout << "m = "<< m <<"" << std::endl;
    samg::grcodec::GRCodec<std::uint32_t> codec(m,samg::grcodec::GRCodecType::GOLOMB_RICE);
    
    for (std::uint32_t j = 0ul; j < 100ul; j++) {
        codec.append(j);
        std::cout << "\t GR( " << j << " ) " << " |bits|= " << codec.length() << " ---> " << codec <<std::endl;
    }
    
    while( codec.has_more() ) {
        std::cout << "\t next = " << codec.next() << " |bits|= " << codec.length() << " ---> " << codec <<std::endl;
    }

    return 0;
}
```

# Licence

All the software of this repository is licenced under the [BSD-2-Clause](https://opensource.org/license/bsd-2-clause/) as follows:

> Copyright (c) 2023 Sebastián AMG (@sebastianamg)
>
> Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
> 1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
> 2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
>
> THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS “AS IS” AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.