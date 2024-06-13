#pragma once
#include <samg/commons.hpp>
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
        namespace writer {
            typedef unsigned char Word;
            class Writer {
                private:
                    const std::string output_file_name;
                    const std::size_t n;
                    const std::vector<std::uint64_t> maxs;
                    const std::uint64_t e;
                    const std::uint64_t s;
                    const std::float_t d;
                    const std::float_t actual_d;
                    const std::string dist;
                    const std::uint64_t c;
                    const std::float_t cderr;
                public:
                    Writer(
                        const std::string output_file_name,
                        const std::size_t n,
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
                        n(n),
                        maxs(maxs),
                        e(e),
                        s(s),
                        d(d),
                        actual_d(actual_d),
                        dist(dist),
                        c(c),
                        cderr(cderr)
                    {}
                    std::string get_output_file_name(){
                        return this->output_file_name;
                    }
                    // virtual void set_number_of_dimensions(std::size_t n) = 0;
                    // virtual void set_max_per_dimension(std::vector<std::uint64_t> maxs) = 0;
                    // virtual void set_number_of_entries(std::uint64_t e) = 0;
                    // virtual void set_matrix_side_size(std::uint64_t s) = 0;
                    // virtual void set_matrix_expected_density(std::float_t d) = 0;
                    // virtual void set_matrix_actual_density(std::float_t actual_d) = 0;
                    // virtual void set_matrix_distribution(std::string dist) = 0;
                    // virtual void set_clustering(std::uint64_t c) = 0;
                    // virtual void set_clustering_distance_error(std::float_t cderr) = 0;
                    virtual void add_entry(std::vector<std::uint64_t> entry) = 0;
                    virtual void close() = 0;
            };

            class MXSWriter : public Writer {
                /**
                 * @brief MXS Format:
                 * 
                 * HEADER: s s^n d actual_d dist c cderr n max1 max2 max3 ... maxn e
                 * 
                 * PAYLOAD: l{n} v{n} l{n-1} v{n-1} l{n-2} v{n-3} l{n-4} ... v{2} l{1} v1{1} v2{1} v3{1} ... vm{1} ...
                 * 
                 */
                private:
                    std::unique_ptr<samg::serialization::OfflineWordWriter<Word>> serializer;
                    bool is_open;
                public:
                    MXSWriter(const std::string output_file_name,
                        const std::size_t n,
                        const std::vector<std::uint64_t> maxs,
                        const std::uint64_t e,
                        const std::uint64_t s,
                        const std::float_t d,
                        const std::float_t actual_d,
                        const std::string dist = "unknonwn",
                        const std::uint64_t c = 0ULL,
                        const std::float_t cderr = 0.0f
                    ):
                        Writer(
                            output_file_name,
                            n,
                            maxs,
                            e,
                            s,
                            d,
                            actual_d,
                            dist,
                            c,
                            cderr
                        )
                    {
                        this->serializer = std::make_unique<samg::serialization::OfflineWordWriter<Word>>( output_file_name );
                        this->is_open = true;
                        // Writing HEADER:
                        this->serializer->add_value<std::uint64_t>(s);
                        this->serializer->add_value<std::uint64_t>(std::pow(s,n));
                        this->serializer->add_value<std::size_t>( (std::size_t) (d * 1000000ZU) );
                        this->serializer->add_value<std::size_t>( (std::size_t) (actual_d * 1000000ZU));
                        this->serializer->add_string(dist);
                        this->serializer->add_value<std::uint64_t>(c);
                        this->serializer->add_value<std::size_t>( (std::size_t) (cderr * 1000000ZU));
                        this->serializer->add_value<std::size_t>(n);
                        for (std::uint64_t m : maxs ) {
                            this->serializer->add_value<std::uint64_t>(m);
                        }
                        this->serializer->add_value<std::uint64_t>(e);
                    }
                    void add_entry(std::vector<std::uint64_t> entry) {
                        if( this->is_open ) {

                        }
                    }
                    void close() {
                        this->serializer->close();
                        this->serializer.reset();
                    }
            };
        }
    }
}