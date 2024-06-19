#pragma once
#include <samg/commons.hpp>
#include <cmath>
#include <regex>
#include <memory>
#include <samg/matutx-mdx.hpp>
#include <samg/matutx-mxs.hpp>
#include <samg/matutx-graph.hpp>


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
        typedef std::uint8_t Word;

        /***************************************************************/
        enum FileFormat {
            MTX, // MatrixMarket plain-text format.
            K2T, // k2-tree binary format.
            MDX, // MultidimensionalMatrixMarket plain-text format.
            MXS, // MultidimensionalMatrixMarket binary format.
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
            
            position = file_name.find(".mxs");
            if (position != std::string::npos) {
                return FileFormat::MXS;
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
                    case samg::matutx::FileFormat::MXS:
                        // std::cout << "get_instance> (2.2)" << std::endl;
                        return std::make_shared<MXSReader>(input_file_name);
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
}
