#pragma once
#include <samg/commons.hpp>
#include <cmath>
#include <regex>
#include <memory>
#include <samg/matutx-mdx.hpp>
#include <samg/matutx-mxs.hpp>
#include <samg/matutx-graph.hpp>
#include <samg/matutx-csv.hpp>


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
            CSV, // Comma Separated Values.
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

            position = file_name.find(".csv");
            if (position != std::string::npos) {
                return FileFormat::CSV;
            } 

            return FileFormat::Unknown;
        }
        /***************************************************************/
        namespace reader {
            const std::size_t roundup_matrix_size( const std::uint64_t size, const std::size_t k ) {
                return (std::size_t) std::pow( k, std::ceil( std::log(size) / std::log(k) ) );
            }

            std::shared_ptr<Reader> create_instance(const std::string& input_file_name) {
                switch (samg::matutx::identify_file_format(input_file_name)) {
                    case samg::matutx::FileFormat::GRAPH:
                        return std::make_shared<GraphReader>(input_file_name);
                    case samg::matutx::FileFormat::MDX:
                        return std::make_shared<MDXReader>(input_file_name);
                    case samg::matutx::FileFormat::MXS:
                        return std::make_shared<MXSReader>(input_file_name);
                    case samg::matutx::FileFormat::CSV:
                        return std::make_shared<CSVReader>(input_file_name);
                    default:
                        throw std::runtime_error("Unrecognized file format!");
                }
            }

            void destroy_instance( Reader& reader ) {
                delete &reader;
            }
        }

        namespace streamer {
            /***************************************************************/
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
            /***************************************************************/
            // #include <vector>
            // #include <cstdint>
            // #include <algorithm>
            // #include <stdexcept>
            // #include <iostream>

            /**
             * @brief A class for representing an n-dimensional Cartesian space with dynamic insertion capabilities. This class compresses the coordinates into a compact representation using a set of contiguous arrays, allowing for efficient storage and traversal.
             * @note The space complexity of CSMR class is O(n + m), where n is the number of dimensions and m is the total number of unique coordinates inserted. The traversal methods operate in O(n) time per coordinate retrieval, making it efficient for large datasets.
             * 
             */
            template<typename IntType = std::uint64_t>
            class CSMR {
            private:
                std::size_t num_dims;
                bool is_sealed;
                bool is_first_insertion;
                std::vector<IntType> last_coord;
                IntType max_coord_val;

                // The 2n - 1 contiguous flat arrays
                // ind[d] stores the unique coordinate indices for dimension d
                std::vector<std::vector<IntType>> ind;
                // ptr[d] stores the boundary offsets for dimension d to d+1
                std::vector<std::vector<std::size_t>> ptr;

                // Traversal State variables for the iterator (DFS mimicking)
                std::vector<std::size_t> I;
                std::vector<std::size_t> J;
                std::vector<IntType> current_coord;
                std::size_t d;
                std::vector<IntType> _next_result;
                bool _has_next;

                /**
                 * @brief Finalizes the structure by capping all pointer arrays. 
                 * Required before any traversal can begin.
                 */
                void seal() {
                    if (is_sealed || is_first_insertion) {
                        is_sealed = true;
                        return;
                    }
                    // Cap off the boundaries of the ptr arrays with the final sizes
                    for (std::size_t k = 0; k < num_dims - 1; ++k) {
                        ptr[k].push_back(ind[k + 1].size());
                    }
                    is_sealed = true;
                }

                /**
                 * @brief Proactively traverses the tree to find the next valid leaf.
                 * Updates _has_next instantaneously so the sequence wrapper never lies.
                 */
                void advance() {
                    while (true) {
                        if (I[d] < J[d]) {
                            current_coord[d] = ind[d][I[d]];

                            if (d == num_dims - 1) {
                                // LEAF NODE: Cache the result and pause traversal
                                _next_result = current_coord;
                                I[d]++; 
                                _has_next = true;
                                return; 
                            } else {
                                // INTERNAL NODE: Branch down
                                I[d + 1] = ptr[d][I[d]];
                                J[d + 1] = (I[d] + 1 < ptr[d].size()) ? ptr[d][I[d] + 1] : ind[d + 1].size();
                                d++;
                            }
                        } else {
                            // EXHAUSTION: Backtrack
                            if (d == 0) {
                                _has_next = false; // Accurately flags the end of the tree
                                return; 
                            }
                            d--;
                            I[d]++;
                        }
                    }
                }

            public:
                /**
                 * @brief Constructor for dynamic insertion
                 * @param n Number of dimensions
                 */
                CSMR(std::size_t n) : num_dims(n), is_sealed(false), is_first_insertion(true), max_coord_val(0) {
                    if (n == 0) throw std::invalid_argument("Dimensions must be > 0");
                    ind.resize(n);
                    if (n > 1) ptr.resize(n - 1);
                    
                    // Pre-allocate traversal arrays
                    I.resize(n, 0);
                    J.resize(n, 0);
                    current_coord.resize(n, 0);
                }

                /**
                 * @brief Constructor for bulk loading
                 * @param n Number of dimensions
                 * @param coords Vector of n-dimensional coordinates (must be lexicographically sorted)
                 */
                CSMR(std::size_t n, const std::vector<std::vector<IntType>>& coords) : CSMR(n) {
                    for (const auto& coord : coords) {
                        add(coord);
                    }
                    seal();
                }

                /**
                 * @brief Constructor for bulk loading from a different coordinate type
                 */
                template<typename OtherIntType, typename = std::enable_if_t<!std::is_same_v<OtherIntType, IntType>>>
                CSMR(std::size_t n, const std::vector<std::vector<OtherIntType>>& coords) : CSMR(n) {
                    for (const auto& coord : coords) {
                        add(coord);
                    }
                    seal();
                }

                /**
                 * @brief Adds a coordinate dynamically.
                 * Coordinates MUST be added in lexicographical order.
                 */
                void add(const std::vector<IntType>& coord) {
                    if (is_sealed) throw std::logic_error("Cannot add coordinates after traversing (structure is sealed).");
                    if (coord.size() != num_dims) throw std::invalid_argument("Coordinate dimension mismatch.");

                    // Track maximum coordinate value to determine number of nodes
                    for (const auto& val : coord) {
                        if (val > max_coord_val) max_coord_val = val;
                    }

                    if (is_first_insertion) {
                        for (std::size_t k = 0; k < num_dims; ++k) {
                            ind[k].push_back(coord[k]);
                            if (k < num_dims - 1) {
                                ptr[k].push_back(0); // Start of children in next dimension
                            }
                        }
                        last_coord = coord;
                        is_first_insertion = false;
                    } else {
                        // Find the highest dimension (smallest index) where the prefix diverges
                        std::size_t diff_d = 0;
                        while (diff_d < num_dims && coord[diff_d] == last_coord[diff_d]) {
                            diff_d++;
                        }

                        if (diff_d == num_dims) return; // Duplicate coordinate, ignore.
                        if (coord[diff_d] < last_coord[diff_d]) {
                            throw std::invalid_argument("Coordinates must be added in lexicographical order.");
                        }

                        // Branch the prefix tree from the divergence point
                        for (std::size_t k = diff_d; k < num_dims; ++k) {
                            ind[k].push_back(coord[k]);
                            if (k < num_dims - 1) {
                                // Start of children for this newly branched node
                                ptr[k].push_back(ind[k + 1].size());
                            }
                        }
                        last_coord = coord;
                    }
                }

                /**
                 * @brief Adds a coordinate of a different integer type dynamically.
                 */
                template<typename OtherIntType, typename = std::enable_if_t<!std::is_same_v<OtherIntType, IntType>>>
                void add(const std::vector<OtherIntType>& coord) {
                    if (coord.size() != num_dims) throw std::invalid_argument("Coordinate dimension mismatch.");
                    std::vector<IntType> converted;
                    converted.reserve(coord.size());
                    for (const auto& val : coord) {
                        converted.push_back(static_cast<IntType>(val));
                    }
                    add(converted);
                }

                /**
                 * @brief Checks if a specific coordinate exists in the structure.
                 * This is an efficient O(sum(log |ind_d|)) operation.
                 * @note The structure must be sealed before calling this.
                 */
                bool contains(const std::vector<IntType>& coord) const {
                    if (!is_sealed) {
                        // // Seal on demand if not already done.
                        // seal();
                        throw std::logic_error("Cannot check if a coordinate exists within an unsealed structure. Call restart() or next() first.");
                    }
                    if (coord.size() != num_dims) return false;

                    // Iteratively search through the dimensions
                    std::size_t start_idx = 0;
                    std::size_t end_idx = ind[0].size();

                    for (std::size_t d = 0; d < num_dims; ++d) {
                        // Binary search for the current dimension's value in the valid range
                        auto it = std::lower_bound(ind[d].begin() + start_idx, ind[d].begin() + end_idx, coord[d]);

                        if (it == (ind[d].begin() + end_idx) || *it != coord[d]) {
                            // Value not found in this dimension's segment
                            return false;
                        }

                        // If found, update the search range for the next dimension
                        if (d < num_dims - 1) {
                            std::size_t current_idx = std::distance(ind[d].begin(), it);
                            start_idx = ptr[d][current_idx];
                            end_idx = (current_idx + 1 < ptr[d].size()) ? ptr[d][current_idx + 1] : ind[d + 1].size();
                        }
                    }

                    // If we successfully traversed all dimensions, the coordinate exists.
                    return true;
                }

                /**
                 * @brief Checks if a specific coordinate of a different integer type exists in the structure.
                 */
                template<typename OtherIntType, typename = std::enable_if_t<!std::is_same_v<OtherIntType, IntType>>>
                bool contains(const std::vector<OtherIntType>& coord) const {
                    if (coord.size() != num_dims) return false;
                    std::vector<IntType> converted;
                    converted.reserve(coord.size());
                    for (const auto& val : coord) {
                        converted.push_back(static_cast<IntType>(val));
                    }
                    return contains(converted);
                }

                /**
                 * @brief Returns the number of dimensions of the space.
                 */
                const std::size_t get_number_of_dimensions() const {
                    return num_dims;
                }

                /**
                 * @brief Returns the number of nodes, calculated as the max coordinate value + 1.
                 */
                const std::uint64_t get_number_of_nodes() const {
                    return static_cast<std::uint64_t>(max_coord_val) + 1;
                }

                /**
                 * @brief Returns the total number of edges (unique coordinates) stored.
                 * @note The structure must be sealed before calling this method.
                 */
                const std::uint64_t get_number_of_edges() const {
                    if (!is_sealed) {
                        throw std::logic_error("CSMR must be sealed before getting edge count. Call restart() or next() first.");
                    }
                    return ind.empty() ? 0 : ind.back().size();
                }

                /**
                 * @brief Prepares the CSMR for a sequential iterator traversal.
                 */
                void restart() {
                    if (!is_sealed) seal();

                    if (ind.empty() || ind[0].empty()) {
                        _has_next = false;
                        return;
                    }

                    std::fill(I.begin(), I.end(), 0);
                    std::fill(J.begin(), J.end(), 0);
                    std::fill(current_coord.begin(), current_coord.end(), 0);

                    d = 0;
                    J[0] = ind[0].size();
                    
                    // Prime the pump! Find and cache the very first element immediately.
                    advance(); 
                }

                /**
                 * @brief Checks if there are more coordinates available in the sequence.
                 * 
                 * @return true 
                 * @return false 
                 */
                bool has_next() const {
                    return _has_next;
                }

                /**
                 * @brief Returns the next coordinate in the sequence.
                 * 
                 * @return std::vector<IntType> 
                 */
                std::vector<IntType> next() {
                    if (!_has_next) throw std::out_of_range("No more coordinates available.");
                    
                    // 1. Grab the cached result
                    // Using move semantics to avoid a copy if _next_result is not needed afterward.
                    std::vector<IntType> result = std::move(_next_result);
                    
                    // 2. Immediately pre-fetch the next one so _has_next updates instantly
                    advance(); 
                    
                    return result;
                }

                /**
                 * @brief Exhausts the structure to retrieve all coordinates.
                 */
                std::vector<std::vector<IntType>> get_sequence() {
                    std::vector<std::vector<IntType>> full_sequence;
                    restart();
                    while (has_next()) {
                        full_sequence.push_back(next());
                    }
                    return full_sequence;
                }
            };

            /***************************************************************/
        }
        /***************************************************************/
    }
}
