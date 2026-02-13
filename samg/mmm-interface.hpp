#pragma once
#include <samg/commons.hpp>
// #include <cmath>
// #include <regex>
// #include <memory>
// #include <cpp_properties/action/properties_action.hpp>
// #include <cpp_properties/actor/properties_actor.hpp>
// #include <cpp_properties/actor/traits/properties_actor_traits.hpp>
// #include <cpp_properties/parser.hpp>
// #include <webgraph/webgraph-3.6.11.h>

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
        namespace reader {
            // const std::size_t roundup_matrix_size( const std::uint64_t size, const std::size_t k ) {
            //     return (std::size_t) std::pow( k, std::ceil( std::log(size) / std::log(k) ) );
            // }

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
                    virtual std::uint64_t next_zvalue() = 0;
            };
        }

        namespace writer {
            class Writer {
                private:
                    const std::string output_file_name;
                    // const std::size_t n;
                    const std::vector<std::uint64_t> maxs;
                    const std::uint64_t e, s, c;
                    const std::float_t d, actual_d, cderr;
                    const std::string dist;
                public:
                    Writer(
                        const std::string output_file_name,
                        // const std::size_t n,
                        const std::vector<std::uint64_t> maxs,
                        const std::uint64_t e,
                        const std::uint64_t s,
                        const std::float_t d,
                        const std::float_t actual_d,
                        const std::string dist = "unknonwn",
                        const std::uint64_t c = 0ULL,
                        const std::float_t cderr = 0.0f
                    ):
                        output_file_name(output_file_name),
                        // n(n),
                        maxs(maxs),
                        e(e),
                        s(s),
                        d(d),
                        actual_d(actual_d),
                        dist(dist),
                        c(c),
                        cderr(cderr)
                    {}
                    const std::string get_output_file_name() const {
                        return this->output_file_name;
                    }
                    
                    virtual void add_entry(std::vector<std::uint64_t> entry) = 0;

                    void add_entries(samg::matutx::reader::Reader &reader) {
                        while( reader.has_next() ) {
                            this->add_entry( reader.next() );
                        }
                    }

                    virtual void close() = 0;
            };
        }
        /***************************************************************/
    }
}
