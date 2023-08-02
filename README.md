[![License: BSD 2-Clause](https://img.shields.io/badge/License-BSD%202--Clause-blue.svg)](https://opensource.org/licenses/BSD-2-Clause)
[![`gr-codec` Build Status](https://shields.io/badge/gr--codec_build-passing-brightgreen.svg)](https://shields.io/)

# Introduction

The following is the detail of one-header libraries available in this repository:
* `codecs`:
  * `qmx.hpp`: It is an unsigned 32-bits integer encoder/decoder modified from [@amallia's QMX repository](https://github.com/amallia/QMX). A mechanism to get a set of integers from a 128-bits `SIMD` word at a time was added. Copyright © 2014-2017 Andrew Trotman under the [BSD-2-Clause](https://opensource.org/license/bsd-2-clause/) licence. The implementation is based on:
  > A. Trotman and J. Lin, "In Vacuo and In Situ Evaluation of SIMD Codecs," in Proceedings of the 21st Australasian Document Computing Symposium, in ADCS ’16. New York, NY, USA: Association for Computing Machinery, Dec. 2016, pp. 1–8. doi: 10.1145/3015022.3015023. 
  * `gr-codec.hpp`: It contains an implementation of ***Rice-runs*** and ***Golomb-Rice*** codecs. The library [sdsl-lite](https://github.com/simongog/sdsl-lite) is required! The following classes and functions are available:
    * `enum GRCodecType`: Available values are `GOLOMB_RICE`, and `EXPONENTIAL_GOLOMB`. However, at the moment `GOLOMB_RICE` is operative only. 
    * `template<typename Type> class GRCodec`:
      * `static sdsl::bit_vector encode(const Type n, const std::size_t m, GRCodecType type = GRCodecType::GOLOMB_RICE )`: This static function allows encoding an integer `n`.
      * `static Type decode(sdsl::bit_vector v, const std::size_t m, GRCodecType type = GRCodecType::GOLOMB_RICE )`: This static function allows decoding an integer encoded as a bitmap `v`.
      * `append(const Type n)`: This function allows appending the encoded representation of `n` to an internal bitmap.
      * `const Type next()`: This function iterates on the internal bitmap as decodes and returns the next integer. 
      * `const bool has_more()`: This function verifies whether the internal bitmap has or doesn't have more codewords to iterate on.
      * `void restart()`: This function restarts the iteration on the internal bitmap.
      * `const sdsl::bit_vector get_bit_vector()`: This function returns a copy of the internal bitmap.
      * `const std::size_t length()`: This function returns the number of stored bits.
      * `const std::uint64_t get_current_iterator_index()`: This function returns the current iterator index.
    * `template<typename Type> class RiceRuns`:
      * `void encode(const std::vector<Type> sequence)`: This method encodes a sequence of Type integers using Rice-runs.
      * `std::vector<Type> decode()`: This method decodes an encoded sequence of Type integers using Rice-runs.
    * Additionally, the file `gr-codec-test.cpp` contains test cases used to check the correctiveness of both ***Golomb-Rice*** and ***Rice-runs*** implementations. 
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

# Examples

## `codecs/samg::grcodec::GRCodec`

```c++
// gr-test.cpp
// To compile: `g++-11 -ggdb -g3 -I ~/include/ -L ~/lib/ gr-test.cpp -o gr-test -lsdsl`
#include <codecs/gr-codec.hpp>
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

## `codecs/samg::grcodec::RiceRuns`

```c++
// riceruns-test.cpp
// To Compile: g++-11 -ggdb -g3 -I ~/include/ -L ~/lib/ riceruns-test.cpp -o riceruns-test -lsdsl

#include <codecs/gr-codec.hpp>
#include <samg/commons.hpp>
#include <iostream>
#include <sstream>
#include <array>

int main(int argc, char const *argv[]) {
    using Type = std::uint16_t;

	// It encodes 3-dimensional coordinates sorted by dimension in ascending order.
    const std::vector<Type> sequence {0,0,1,0,0,2,0,0,3,0,0,6,0,0,7,0,1,0,0,1,1,0,1,3,0,1,7,0,2,0,0,2,1,0,2,2,0,2,4,0,2,5,0,3,0,0,3,1,0,3,3,0,3,7,0,4,1,0,4,6,0,5,0,0,5,2,0,5,4,0,5,6,0,5,7,0,6,0,0,6,1,0,6,2,0,6,3,0,6,7,0,7,0,0,7,4,0,7,7,1,0,2,1,0,5,1,1,0,1,1,1,1,1,2,1,1,3,1,1,5,1,1,6,1,1,7,1,2,0,1,2,6,1,2,7,1,3,1,1,3,2,1,3,4,1,3,6,1,3,7,1,4,3,1,4,4,1,4,6,1,5,0,1,5,1,1,5,2,1,5,4,1,5,6,1,6,2,1,6,3,1,6,4,1,7,1,1,7,5,1,7,7,2,0,2,2,0,3,2,0,4,2,0,7,2,1,2,2,1,3,2,1,4,2,1,5,2,1,6,2,2,0,2,2,1,2,2,7,2,3,0,2,3,1,2,3,2,2,3,3,2,3,4,2,3,5,2,3,6,2,4,0,2,4,6,2,4,7,2,5,1,2,5,5,2,5,7,2,6,0,2,6,3,2,7,1,2,7,5,2,7,6,3,0,0,3,0,2,3,0,5,3,1,0,3,1,4,3,1,5,3,2,1,3,2,2,3,2,3,3,3,3,3,3,4,3,3,6,3,3,7,3,4,0,3,4,2,3,4,4,3,4,5,3,4,7,3,5,2,3,5,3,3,6,0,3,6,3,3,6,5,3,6,6,3,7,4,3,7,5,3,7,6,4,0,0,4,0,1,4,0,2,4,0,4,4,0,5,4,0,6,4,1,0,4,1,1,4,1,2,4,1,3,4,1,5,4,1,6,4,2,3,4,2,4,4,2,5,4,2,7,4,3,0,4,3,3,4,3,4,4,3,5,4,3,7,4,4,2,4,4,3,4,4,6,4,5,1,4,5,2,4,5,5,4,5,7,4,6,1,4,6,2,4,6,4,4,6,5,4,6,7,4,7,3,5,0,6,5,1,1,5,1,2,5,1,3,5,1,4,5,1,5,5,1,6,5,2,0,5,2,1,5,2,3,5,2,4,5,2,6,5,3,2,5,3,3,5,4,2,5,4,5,5,4,6,5,4,7,5,5,0,5,5,2,5,5,3,5,5,4,5,5,6,5,6,0,5,6,1,5,6,2,5,6,3,5,6,4,5,6,5,5,7,1,5,7,3,5,7,4,5,7,5,5,7,6,6,0,0,6,0,3,6,0,5,6,0,6,6,1,2,6,1,3,6,1,4,6,1,7,6,2,4,6,2,5,6,3,0,6,3,1,6,3,4,6,3,5,6,3,7,6,4,0,6,4,2,6,4,3,6,4,6,6,4,7,6,5,2,6,5,3,6,5,4,6,5,6,6,6,1,6,6,3,6,6,6,6,6,7,6,7,0,6,7,4,6,7,7,7,0,0,7,0,2,7,0,4,7,0,5,7,0,7,7,1,2,7,1,3,7,1,5,7,1,7,7,2,0,7,2,1,7,2,4,7,2,5,7,3,0,7,3,1,7,3,3,7,3,4,7,3,7,7,4,3,7,4,6,7,5,0,7,5,1,7,5,4,7,5,7,7,6,0,7,6,2,7,6,3,7,6,5,7,6,6,7,6,7,7,7,1,7,7,2,7,7,3,7,7,5,7,7,6,7,7,7};

	// Unzip dimensions of each coordinates in three different vectors. 
    const std::size_t N_SEQS = 3;
    std::array<std::vector<Type>,N_SEQS> sequences;

    for (size_t i = 0; i < sequence.size(); ++i) {
        sequences[i % N_SEQS].push_back(sequence[i]);
    }
    
	// Encode and decode each vector using k = 3.
	const std::size_t k = 3;
    for (size_t i = 0; i < sequences.size(); ++i) {

		// Create Rice-runs encoder:
        samg::grcodec::RiceRuns<Type> codec(k);
        
		// Encode sequences[i]:
		codec.encode(sequences[i]);

		// Getting Rice-runs bitmap:
        sdsl::bit_vector bm = codec.get_encoded_sequence();
        
		// Decode sequence:
		std::vector<Type> dv = codec.decode();

		// Display results:
        samg::utils::print_vector<Type>("Original Sequence " + std::to_string(i+1) + " (||="+ std::to_string(sequences[i].size()) +"):",sequences[i]);

        std::cout << "Golomb-Rice Encoded Sequence " << (i+1) << " --- || = " << bm.size() << " ---> " << bm << std::endl;
        
		samg::utils::print_vector<Type>("Decoded Sequence " + std::to_string(i+1) + " (||="+ std::to_string(dv.size()) + "): ",dv);
        
		// Compare original and decoded sequences:
		bool same = sequences[i].size() == dv.size();
        for (size_t j = 0; same && j < dv.size(); ++j) {
			same = sequences[i][j] == dv[j];
        }
        if( same ) {
            std::cout << "Test " << i << ": Successful!" << std::endl;
        } else {
            std::cerr << "Test " << i << ": Wrong!" << std::endl;
        }
        
        std::cout << "---------------------------------------------------------------" << std::endl;
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