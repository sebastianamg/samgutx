#pragma once
#include <samg/commons.hpp>
#include <samg/mmm-interface.hpp>
#include <limits>
#include <cpp_properties/action/properties_action.hpp>
#include <tuple>
#include <cpp_properties/actor/properties_actor.hpp>
#include <cpp_properties/actor/traits/properties_actor_traits.hpp>
#include <cpp_properties/parser.hpp>
// #include <webgraph-graal/webgraph-3.6.11.h>
#include <boost/shared_ptr.hpp>
#include <boost/tuple/tuple.hpp>
// #include <webgraph/webgraph/webgraph.hpp>
#include <stdexcept>
#include <webgraph/webgraph/boost/integration.hpp>
#include <boost/graph/graph_traits.hpp> 
#include <boost/range/iterator_range_core.hpp>
#include <boost/graph/graph_utility.hpp>

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
        namespace reader {


            class GraphReader : public Reader { // NOTE: Patch-implementation! ...on account of memory corruption that happens when trying to use an edge-iterator. 
                private:
                    std::vector<std::uint64_t> max_per_dimension;
                    std::uint64_t number_of_entries;
                    std::uint64_t matrix_side_size;
                    std::uint64_t matrix_size;
                    std::float_t matrix_expected_density;
                    std::float_t matrix_actual_density;
                    std::string matrix_distribution;
                    std::float_t gauss_mu;
                    std::float_t gauss_sigma;
                    std::uint64_t clustering;
                    std::float_t clustering_distance_error;
                    webgraph::bv_graph::graph::graph_ptr graph;
                    boost::graph_traits<webgraph::bv_graph::graph>::edge_iterator m_edge_begin;
                    boost::graph_traits<webgraph::bv_graph::graph>::edge_iterator m_edge_end;
                    boost::graph_traits<webgraph::bv_graph::graph>::edge_iterator m_current_edge;

                    std::queue<std::vector<std::uint64_t>> data; // This is a placeholder for the data structure to hold the graph edges.
                    // // boost::graph_traits<webgraph::bv_graph::graph>::edge_iterator e, e_end;
                    // boost::iterator_range<boost::graph_traits<webgraph::bv_graph::graph>::edge_iterator> edge_range;
                    // boost::graph_traits<webgraph::bv_graph::graph>::edge_iterator current_iterator;
                    
                public:
                    GraphReader(std::string file_name) :
                        Reader(file_name),
                        // graph (webgraph::bv_graph::graph::load( samg::utils::get_file_basename(file_name) ))
                        // graph (webgraph::bv_graph::graph::load_sequential( samg::utils::get_file_basename(file_name) ))
                        graph (webgraph::bv_graph::graph::load_offline( samg::utils::get_file_basename(file_name) ))
                        // edge_range(boost::edges(*graph)),
                        // current_iterator(edge_range.begin())
                    {
                        this->max_per_dimension = std::vector<std::uint64_t>();
                        std::string properties_file = samg::utils::change_extension(file_name,"properties");
                        std::string defaults = samg::utils::read_from_file( properties_file.data() );
                        std::map<std::string, std::string> properties;
                        if( cpp_properties::parse(defaults.begin(),defaults.end(),properties) ) {
                            std::uint64_t nodes = std::atoll( properties["nodes"].data() );
                            std::uint64_t arcs = std::atoll( properties["arcs"].data() );
                            this->max_per_dimension = { nodes , nodes };
                            this->number_of_entries = arcs;
                            this->matrix_side_size = nodes;
                            this->matrix_size = nodes*nodes;
                            this->matrix_expected_density = (double) arcs / (double) this->matrix_size;
                            this->matrix_actual_density = (double) arcs / (double) this->matrix_size;
                            this->matrix_distribution = "null";
                            this->gauss_mu = -1.0f; // Properties might override this
                            this->gauss_sigma = -1.0f; // Properties might override this
                            this->clustering = 0ull; // Properties might override this
                            this->clustering_distance_error = -1.0f; // Properties might override this
                        } else {
                            throw std::runtime_error("Graph properties file \""+properties_file+"\" not available.");
                        }
                        // Loading data in memory:
                        // webgraph::bv_graph::graph::node_iterator n, n_end;
   
                        std::cout << "Loading graph data from file: " << file_name << std::endl;
                        
                        auto edge_pair = boost::edges(*(this->graph));

                        // Setting iterator:
                        this->m_edge_begin = edge_pair.first;
                        this->m_edge_end = edge_pair.second;
                        this->m_current_edge = this->m_edge_begin;
                    }

                    const std::size_t get_number_of_dimensions() const override {
                        return this->max_per_dimension.size();
                    }
                    const std::vector<std::uint64_t> get_max_per_dimension() const override {
                        return this->max_per_dimension;
                    }
                    const std::uint64_t get_number_of_entries() const override {
                        return this->number_of_entries;
                    }
                    const bool has_next() override {
                        return this->m_current_edge != this->m_edge_end;
                    }
                    const std::uint64_t get_matrix_side_size() const override {
                        return this->matrix_side_size;
                    }
                    const std::uint64_t get_matrix_size() const override {
                        return this->matrix_size;
                    }
                    const std::float_t get_matrix_expected_density() const override {
                        return this->matrix_expected_density;
                    }
                    const std::float_t get_matrix_actual_density() const override {
                        return this->matrix_actual_density;
                    }
                    const std::string get_matrix_distribution() const override {
                        return this->matrix_distribution;
                    }
                    const std::float_t get_gauss_mu() const override {
                        return this->gauss_mu;
                    }
                    const std::float_t get_gauss_sigma() const override {
                        return this->gauss_sigma;
                    }
                    const std::uint64_t get_clustering() const override {
                        return this->clustering;
                    }
                    const std::float_t get_clustering_distance_error() const override {
                        return this->clustering_distance_error;
                    }

                    std::vector<std::uint64_t> next() override {
                        if (!has_next()) {
                            throw std::out_of_range("No more edges to iterate.");
                        }
                        // Dereference the current iterator to get the edge,
                        // then post-increment the iterator to move it to the next position.
                        auto edge = *(this->m_current_edge);
                        this->m_current_edge++;
                        std::vector<std::uint64_t> current_edge_coordinates = {edge.first, edge.second};
                        return current_edge_coordinates;
                    }
            };


            // class GraphReader : public Reader { // NOTE: Patch-implementation! ...on account of memory corruption that happens when trying to use an edge-iterator. 
            //     private:
            //         std::vector<std::uint64_t> max_per_dimension;
            //         std::uint64_t number_of_entries;
            //         std::uint64_t matrix_side_size;
            //         std::uint64_t matrix_size;
            //         std::float_t matrix_expected_density;
            //         std::float_t matrix_actual_density;
            //         std::string matrix_distribution;
            //         std::float_t gauss_mu;
            //         std::float_t gauss_sigma;
            //         std::uint64_t clustering;
            //         std::float_t clustering_distance_error;
            //         boost::shared_ptr<webgraph::bv_graph::graph> graph;
            //         std::queue<std::vector<std::uint64_t>> data; // This is a placeholder for the data structure to hold the graph edges.
            //         // // boost::graph_traits<webgraph::bv_graph::graph>::edge_iterator e, e_end;
            //         // boost::iterator_range<boost::graph_traits<webgraph::bv_graph::graph>::edge_iterator> edge_range;
            //         // boost::graph_traits<webgraph::bv_graph::graph>::edge_iterator current_iterator;
                    
            //     public:
            //         GraphReader(std::string file_name) :
            //             Reader(file_name),
            //             // graph (webgraph::bv_graph::graph::load( samg::utils::get_file_basename(file_name) ))
            //             // graph (webgraph::bv_graph::graph::load_sequential( samg::utils::get_file_basename(file_name) ))
            //             graph (webgraph::bv_graph::graph::load_offline( samg::utils::get_file_basename(file_name) ))
            //             // edge_range(boost::edges(*graph)),
            //             // current_iterator(edge_range.begin())
            //         {
            //             this->max_per_dimension = std::vector<std::uint64_t>();
            //             std::string properties_file = samg::utils::change_extension(file_name,"properties");
            //             std::string defaults = samg::utils::read_from_file( properties_file.data() );
            //             std::map<std::string, std::string> properties;
            //             if( cpp_properties::parse(defaults.begin(),defaults.end(),properties) ) {
            //                 std::uint64_t nodes = std::atoll( properties["nodes"].data() );
            //                 std::uint64_t arcs = std::atoll( properties["arcs"].data() );
            //                 this->max_per_dimension = { nodes , nodes };
            //                 this->number_of_entries = arcs;
            //                 this->matrix_side_size = nodes;
            //                 this->matrix_size = nodes*nodes;
            //                 this->matrix_expected_density = (double) arcs / (double) this->matrix_size;
            //                 this->matrix_actual_density = (double) arcs / (double) this->matrix_size;
            //                 this->matrix_distribution = "null";
            //                 this->gauss_mu = -1.0f; // Properties might override this
            //                 this->gauss_sigma = -1.0f; // Properties might override this
            //                 this->clustering = 0ull; // Properties might override this
            //                 this->clustering_distance_error = -1.0f; // Properties might override this
            //             } else {
            //                 throw std::runtime_error("Graph properties file \""+properties_file+"\" not available.");
            //             }
            //             // Loading data in memory:
            //             // webgraph::bv_graph::graph::node_iterator n, n_end;
   
            //             std::cout << "Loading graph data from file: " << file_name << std::endl;
            //             // std::tie(n, n_end) = this->graph->get_node_iterator( 0 );
            //             std::cout << "Loaded!" << std::endl;
            //             // std::cout << "Node: " << *n << ":: ";
                        
            //             for (auto e : boost::make_iterator_range(boost::edges(*(this->graph)))) {
            //                 /* nothing */;
            //                 // std::cout << e.first << " " << e.second << " "; 
            //                 std::vector<std::uint64_t> current_edge_coordinates(this->max_per_dimension.size());
            //                 current_edge_coordinates[0] = e.first;
            //                 current_edge_coordinates[1] = e.second;
            //                 this->data.push(current_edge_coordinates);
            //             }
            //         }

            //         const std::size_t get_number_of_dimensions() const override {
            //             return this->max_per_dimension.size();
            //         }
            //         const std::vector<std::uint64_t> get_max_per_dimension() const override {
            //             return this->max_per_dimension;
            //         }
            //         const std::uint64_t get_number_of_entries() const override {
            //             return this->number_of_entries;
            //         }
            //         const bool has_next() override {
            //             return this->data.size() > 0; // Check if there are more edges to read.
            //         }
            //         const std::uint64_t get_matrix_side_size() const override {
            //             return this->matrix_side_size;
            //         }
            //         const std::uint64_t get_matrix_size() const override {
            //             return this->matrix_size;
            //         }
            //         const std::float_t get_matrix_expected_density() const override {
            //             return this->matrix_expected_density;
            //         }
            //         const std::float_t get_matrix_actual_density() const override {
            //             return this->matrix_actual_density;
            //         }
            //         const std::string get_matrix_distribution() const override {
            //             return this->matrix_distribution;
            //         }
            //         const std::float_t get_gauss_mu() const override {
            //             return this->gauss_mu;
            //         }
            //         const std::float_t get_gauss_sigma() const override {
            //             return this->gauss_sigma;
            //         }
            //         const std::uint64_t get_clustering() const override {
            //             return this->clustering;
            //         }
            //         const std::float_t get_clustering_distance_error() const override {
            //             return this->clustering_distance_error;
            //         }

            //         std::vector<std::uint64_t> next() override {
            //             std::vector<std::uint64_t> current_edge_coordinates = this->data.front();
            //             this->data.pop();
            //             return current_edge_coordinates;
            //         }
            // };









            // class GraphReader : public Reader {
            //     private:
            //         std::vector<std::uint64_t> max_per_dimension;
            //         std::uint64_t number_of_entries;
            //         std::uint64_t matrix_side_size;
            //         std::uint64_t matrix_size;
            //         std::float_t matrix_expected_density;
            //         std::float_t matrix_actual_density;
            //         std::string matrix_distribution;
            //         std::float_t gauss_mu;
            //         std::float_t gauss_sigma;
            //         std::uint64_t clustering;
            //         std::float_t clustering_distance_error;
            //         // boost::graph_traits<webgraph::bv_graph::graph>::edge_iterator e, e_end;
            //         boost::iterator_range<boost::graph_traits<webgraph::bv_graph::graph>::edge_iterator> edge_range;
            //         boost::graph_traits<webgraph::bv_graph::graph>::edge_iterator current_iterator;
            //         boost::shared_ptr<webgraph::bv_graph::graph> graph;
            //     public:
            //         GraphReader(std::string file_name) :
            //             Reader(file_name),
            //             graph (webgraph::bv_graph::graph::load_offline( samg::utils::get_file_basename(file_name) ))
            //             // edge_range(boost::edges(*graph)),
            //             // current_iterator(edge_range.begin())
            //         {
            //             this->max_per_dimension = std::vector<std::uint64_t>();
            //             std::string properties_file = samg::utils::change_extension(file_name,"properties");
            //             std::string defaults = samg::utils::read_from_file( properties_file.data() );
            //             std::map<std::string, std::string> properties;
            //             if( cpp_properties::parse(defaults.begin(),defaults.end(),properties) ) {
            //                 std::uint64_t nodes = std::atoll( properties["nodes"].data() );
            //                 std::uint64_t arcs = std::atoll( properties["arcs"].data() );
            //                 this->max_per_dimension = { nodes , nodes };
            //                 this->number_of_entries = arcs;
            //                 this->matrix_side_size = nodes;
            //                 this->matrix_size = nodes*nodes;
            //                 this->matrix_expected_density = (double) arcs / (double) this->matrix_size;
            //                 this->matrix_actual_density = (double) arcs / (double) this->matrix_size;
            //                 this->matrix_distribution = "null";
            //                 this->gauss_mu = -1.0f; // Properties might override this
            //                 this->gauss_sigma = -1.0f; // Properties might override this
            //                 this->clustering = 0ull; // Properties might override this
            //                 this->clustering_distance_error = -1.0f; // Properties might override this
            //             } else {
            //                 throw std::runtime_error("Graph properties file \""+properties_file+"\" not available.");
            //             }
            //             // std::tie( this->e, this->e_end ) = boost::edges( *(this->graph) );
            //             // Initialize the rest of the members
            //             this->edge_range = boost::edges(*graph);
            //             this->current_iterator = this->edge_range.begin();
            //         }

            //         const std::size_t get_number_of_dimensions() const override {
            //             return this->max_per_dimension.size();
            //         }
            //         const std::vector<std::uint64_t> get_max_per_dimension() const override {
            //             return this->max_per_dimension;
            //         }
            //         const std::uint64_t get_number_of_entries() const override {
            //             return this->number_of_entries;
            //         }
            //         const bool has_next() override {
            //             // return this->e != this->e_end;
            //             return this->current_iterator != this->edge_range.end();
            //         }
            //         const std::uint64_t get_matrix_side_size() const override {
            //             return this->matrix_side_size;
            //         }
            //         const std::uint64_t get_matrix_size() const override {
            //             return this->matrix_size;
            //         }
            //         const std::float_t get_matrix_expected_density() const override {
            //             return this->matrix_expected_density;
            //         }
            //         const std::float_t get_matrix_actual_density() const override {
            //             return this->matrix_actual_density;
            //         }
            //         const std::string get_matrix_distribution() const override {
            //             return this->matrix_distribution;
            //         }
            //         const std::float_t get_gauss_mu() const override {
            //             return this->gauss_mu;
            //         }
            //         const std::float_t get_gauss_sigma() const override {
            //             return this->gauss_sigma;
            //         }
            //         const std::uint64_t get_clustering() const override {
            //             return this->clustering;
            //         }
            //         const std::float_t get_clustering_distance_error() const override {
            //             return this->clustering_distance_error;
            //         }

            //         std::vector<std::uint64_t> next() override {
                        
            //             // Check if there are more edges.
            //             // this->has_next() is implemented as `return this->e != this->e_end;`
            //             if (!this->has_next()) {
            //                 throw std::runtime_error("GraphReader::next() called when no more entries exist.");
            //             }

            //             // Ensure the vector for storing coordinates is correctly sized.
            //             // this->max_per_dimension.size() should be 2 for typical graph edges.
            //             if (this->max_per_dimension.size() < 2) {
            //                 // This would indicate a setup issue, as max_per_dimension should reflect the graph's dimensionality.
            //                 throw std::logic_error("GraphReader: max_per_dimension is not correctly initialized for 2D coordinates.");
            //             }
            //             std::vector<std::uint64_t> current_edge_coordinates(this->max_per_dimension.size());

            //             // Access the source and target nodes of the current edge.
            //             // this->e is boost::graph_traits<webgraph::bv_graph::graph>::edge_iterator.
            //             // Dereferencing it (*(this->e)) gives an edge_descriptor.
            //             // For webgraph-cpp, the edge_descriptor is std::pair<node_t, node_t>.
            //             // this->e->first is equivalent to (*(this->e)).first.
            //             // webgraph::types::node_t is typically 'int', so casting to std::uint64_t is good practice.
            //             // current_edge_coordinates[0] = static_cast<std::uint64_t>(this->e->first);
            //             // current_edge_coordinates[1] = static_cast<std::uint64_t>(this->e->second);
            //             current_edge_coordinates[0] = static_cast<std::uint64_t>(this->current_iterator->first);
            //             current_edge_coordinates[1] = static_cast<std::uint64_t>(this->current_iterator->second);

            //             // Advance the iterator to the next edge.
            //             // The ASan error "CHECK failed: asan_allocator.cpp:190" occurs during this increment,
            //             // specifically within the webgraph-cpp library's iterator machinery when it performs a delete.
            //             // This indicates that the heap metadata was corrupted *before* this point.
            //             // this->e++;
            //             ++(this->current_iterator);

            //             return current_edge_coordinates;
            //         }
            // };
            // class GraphReader : public Reader {
            //     private:
            //         std::vector<std::uint64_t> max_per_dimension;
            //         std::uint64_t number_of_entries;
            //         std::uint64_t matrix_side_size;
            //         std::uint64_t matrix_size;
            //         std::float_t matrix_expected_density;
            //         std::float_t matrix_actual_density;
            //         std::string matrix_distribution;
            //         std::float_t gauss_mu;
            //         std::float_t gauss_sigma;
            //         std::uint64_t clustering;
            //         std::float_t clustering_distance_error;

            //         // boost::graph_traits<webgraph::bv_graph::graph>::vertex_iterator v, v_end;
            //         boost::graph_traits<webgraph::bv_graph::graph>::edge_iterator e, e_end;


            //         boost::shared_ptr<webgraph::bv_graph::graph> graph;
            //         // webgraph::bv_graph::graph::node_iterator entries_iter, entries_end;
            //         // std::vector<int> successors;
            //         // std::size_t successors_index;
            //         // graal_isolate_t *isolate = NULL;
            //         // graal_isolatethread_t *thread = NULL;

            //         // void _update_() {
            //         //     while( this->successors_index >= this->successors.size() && this->entries_iter != this->entries_end ) {
            //         //         this->successors = webgraph::bv_graph::successor_vector( this->entries_iter );
            //         //         ++(this->entries_iter);
            //         //         this->successors_index = 0;
            //         //     }
            //         // }

            //     public:
            //         GraphReader(std::string file_name) :
            //             Reader(file_name),
            //             graph (webgraph::bv_graph::graph::load_offline( samg::utils::get_file_basename(file_name) ))//,
            //             // successors_index(0ZU)
            //         {
            //             this->max_per_dimension = std::vector<std::uint64_t>();
            //             std::string properties_file = samg::utils::change_extension(file_name,"properties");
            //             std::string defaults = samg::utils::read_from_file( properties_file.data() );
            //             std::map<std::string, std::string> properties;
            //             if( cpp_properties::parse(defaults.begin(),defaults.end(),properties) ) {
            //                 std::uint64_t nodes = std::atoll( properties["nodes"].data() );
            //                 std::uint64_t arcs = std::atoll( properties["arcs"].data() );
            //                 this->max_per_dimension = { nodes , nodes };
            //                 this->number_of_entries = arcs;
            //                 this->matrix_side_size = nodes;
            //                 this->matrix_size = nodes*nodes;
            //                 this->matrix_expected_density = (double) arcs / (double) this->matrix_size;
            //                 this->matrix_actual_density = (double) arcs / (double) this->matrix_size;
            //                 this->matrix_distribution = "null";
            //                 std::float_t gauss_mu = -1.0f;
            //                 std::float_t gauss_sigma = -1.0f;
            //                 std::uint64_t clustering = 0ull;
            //                 std::float_t clustering_distance_error = -1.0f;
            //             } else {
            //                 throw std::runtime_error("Graph properties file \""+properties_file+"\" not available.");
            //             }

            //             // std::tie(this->entries_iter, this->entries_end) = this->graph->get_node_iterator( 0 );
            //             std::tie( this->e, this->e_end ) = boost::edges( *(this->graph) );
            //         }
            //         // ~GraphReader() {
            //         //     graal_tear_down_isolate(this->thread);
            //         // }

            //         const std::size_t get_number_of_dimensions() const override {
            //             return this->max_per_dimension.size();
            //         }
            //         const std::vector<std::uint64_t> get_max_per_dimension() const override {
            //             return this->max_per_dimension;
            //         }
            //         const std::uint64_t get_number_of_entries() const override {
            //             return this->number_of_entries;
            //         }
            //         const bool has_next() override {
            //             // return this->entries_iter != this->entries_end || this->successors_index < this->successors.size();
            //             return this->e != this->e_end;
            //         }

            //         const std::uint64_t get_matrix_side_size() const override {
            //             // std::cout << "GraphReader>get_matrix_side_size> (1)" << std::endl;
            //             return this->matrix_side_size;
            //         }
            //         const std::uint64_t get_matrix_size() const override {
            //             return this->matrix_size;
            //         }
            //         const std::float_t get_matrix_expected_density() const override {
            //             return this->matrix_expected_density;
            //         }
            //         const std::float_t get_matrix_actual_density() const override {
            //             return this->matrix_actual_density;
            //         }
            //         const std::string get_matrix_distribution() const override {
            //             return this->matrix_distribution;
            //         }
            //         const std::float_t get_gauss_mu() const override {
            //             return this->gauss_mu;
            //         }
            //         const std::float_t get_gauss_sigma() const override {
            //             return this->gauss_sigma;
            //         }
            //         const std::uint64_t get_clustering() const override {
            //             return this->clustering;
            //         }
            //         const std::float_t get_clustering_distance_error() const override {
            //             return this->clustering_distance_error;
            //         }

            //         std::vector<std::uint64_t> next() override {
                        
            //             // Check if there are more edges.
            //             // this->has_next() is implemented as `return this->e != this->e_end;`
            //             if (!this->has_next()) {
            //                 throw std::runtime_error("GraphReader::next() called when no more entries exist.");
            //             }

            //             // Ensure the vector for storing coordinates is correctly sized.
            //             // this->max_per_dimension.size() should be 2 for typical graph edges.
            //             if (this->max_per_dimension.size() < 2) {
            //                 // This would indicate a setup issue, as max_per_dimension should reflect the graph's dimensionality.
            //                 throw std::logic_error("GraphReader: max_per_dimension is not correctly initialized for 2D coordinates.");
            //             }
            //             std::vector<std::uint64_t> current_edge_coordinates(this->max_per_dimension.size());

            //             // Access the source and target nodes of the current edge.
            //             // this->e is boost::graph_traits<webgraph::bv_graph::graph>::edge_iterator.
            //             // Dereferencing it (*(this->e)) gives an edge_descriptor.
            //             // For webgraph-cpp, the edge_descriptor is std::pair<node_t, node_t>.
            //             // this->e->first is equivalent to (*(this->e)).first.
            //             // webgraph::types::node_t is typically 'int', so casting to std::uint64_t is good practice.
            //             current_edge_coordinates[0] = static_cast<std::uint64_t>(this->e->first);
            //             current_edge_coordinates[1] = static_cast<std::uint64_t>(this->e->second);

            //             // Advance the iterator to the next edge.
            //             // The ASan error "CHECK failed: asan_allocator.cpp:190" occurs during this increment,
            //             // specifically within the webgraph-cpp library's iterator machinery when it performs a delete.
            //             // This indicates that the heap metadata was corrupted *before* this point.
            //             ++(this->e);

            //             return current_edge_coordinates;

            //             // // this->_update_();
            //             // if( this->has_next() ) {
            //             //     // Assuming 2-dimensional matrix:
            //             //     std::vector<std::uint64_t> entries = std::vector<std::uint64_t>(this->max_per_dimension.size());
            //             //     entries[0] = this->e->first;//*(this->entries_iter);
            //             //     entries[1] = this->e->second;//successors[this->successors_index++];
            //             //     ++(this->e);
            //             //     return entries;
            //             // }
            //             // throw std::runtime_error("No more entries.");
            //         }
            // };

        };


        //     class GraphReader : public Reader {
        //         private:
        //             std::vector<std::uint64_t> max_per_dimension;
        //             std::uint64_t number_of_entries;
        //             std::uint64_t matrix_side_size;
        //             std::uint64_t matrix_size;
        //             std::float_t matrix_expected_density;
        //             std::float_t matrix_actual_density;
        //             std::string matrix_distribution;
        //             std::float_t gauss_mu;
        //             std::float_t gauss_sigma;
        //             std::uint64_t clustering;
        //             std::float_t clustering_distance_error;
                    
        //             boost::shared_ptr<webgraph::bv_graph::graph> graph;
        //             webgraph::bv_graph::graph::node_iterator entries_iter, entries_end;
        //             std::vector<int> successors;
        //             std::size_t successors_index;
        //             // graal_isolate_t *isolate = NULL;
        //             // graal_isolatethread_t *thread = NULL;

        //             void _update_() {
        //                 while( this->successors_index >= this->successors.size() && this->entries_iter != this->entries_end ) {
        //                     this->successors = webgraph::bv_graph::successor_vector( this->entries_iter );
        //                     ++(this->entries_iter);
        //                     this->successors_index = 0;
        //                 }
        //             }

        //         public:
        //             GraphReader(std::string file_name) :
        //                 Reader(file_name),
        //                 graph (webgraph::bv_graph::graph::load_offline( samg::utils::get_file_basename(file_name) )),
        //                 successors_index(0ZU)
        //             {
        //                 this->max_per_dimension = std::vector<std::uint64_t>();
        //                 std::string properties_file = samg::utils::change_extension(file_name,"properties");
        //                 std::string defaults = samg::utils::read_from_file( properties_file.data() );
        //                 std::map<std::string, std::string> properties;
        //                 if( cpp_properties::parse(defaults.begin(),defaults.end(),properties) ) {
        //                     std::uint64_t nodes = std::atoll( properties["nodes"].data() );
        //                     std::uint64_t arcs = std::atoll( properties["arcs"].data() );
        //                     this->max_per_dimension = { nodes , nodes };
        //                     this->number_of_entries = arcs;
        //                     this->matrix_side_size = nodes;
        //                     this->matrix_size = nodes*nodes;
        //                     this->matrix_expected_density = (double) arcs / (double) this->matrix_size;
        //                     this->matrix_actual_density = (double) arcs / (double) this->matrix_size;
        //                     this->matrix_distribution = "null";
        //                     std::float_t gauss_mu = -1.0f;
        //                     std::float_t gauss_sigma = -1.0f;
        //                     std::uint64_t clustering = 0ull;
        //                     std::float_t clustering_distance_error = -1.0f;
        //                 } else {
        //                     throw std::runtime_error("Graph properties file \""+properties_file+"\" not available.");
        //                 }

        //                 std::tie(this->entries_iter, this->entries_end) = this->graph->get_node_iterator( 0 );
        //             }
        //             // ~GraphReader() {
        //             //     graal_tear_down_isolate(this->thread);
        //             // }

        //             const std::size_t get_number_of_dimensions() const override {
        //                 return this->max_per_dimension.size();
        //             }
        //             const std::vector<std::uint64_t> get_max_per_dimension() const override {
        //                 return this->max_per_dimension;
        //             }
        //             const std::uint64_t get_number_of_entries() const override {
        //                 return this->number_of_entries;
        //             }
        //             const bool has_next() override {
        //                 return this->entries_iter != this->entries_end || this->successors_index < this->successors.size();
        //             }

        //             const std::uint64_t get_matrix_side_size() const override {
        //                 // std::cout << "GraphReader>get_matrix_side_size> (1)" << std::endl;
        //                 return this->matrix_side_size;
        //             }
        //             const std::uint64_t get_matrix_size() const override {
        //                 return this->matrix_size;
        //             }
        //             const std::float_t get_matrix_expected_density() const override {
        //                 return this->matrix_expected_density;
        //             }
        //             const std::float_t get_matrix_actual_density() const override {
        //                 return this->matrix_actual_density;
        //             }
        //             const std::string get_matrix_distribution() const override {
        //                 return this->matrix_distribution;
        //             }
        //             const std::float_t get_gauss_mu() const override {
        //                 return this->gauss_mu;
        //             }
        //             const std::float_t get_gauss_sigma() const override {
        //                 return this->gauss_sigma;
        //             }
        //             const std::uint64_t get_clustering() const override {
        //                 return this->clustering;
        //             }
        //             const std::float_t get_clustering_distance_error() const override {
        //                 return this->clustering_distance_error;
        //             }

        //             std::vector<std::uint64_t> next() override {
        //                 this->_update_();
        //                 if( this->has_next() ) {
        //                     // Assuming 2-dimensional matrix:
        //                     std::vector<std::uint64_t> entries = std::vector<std::uint64_t>(this->max_per_dimension.size());
        //                     entries[0] = *(this->entries_iter);
        //                     entries[1] = successors[this->successors_index++];
        //                     return entries;
        //                 }
        //                 throw std::runtime_error("No more entries.");
        //             }
        //     };

        // };
        //     class GraphReader : public Reader {
        //         private:
        //             std::vector<std::uint64_t> max_per_dimension;
        //             std::uint64_t number_of_entries;
        //             std::uint64_t matrix_side_size;
        //             std::uint64_t matrix_size;
        //             std::float_t matrix_expected_density;
        //             std::float_t matrix_actual_density;
        //             std::string matrix_distribution;
        //             std::float_t gauss_mu;
        //             std::float_t gauss_sigma;
        //             std::uint64_t clustering;
        //             std::float_t clustering_distance_error;
        //             std::size_t entries_counter;
        //             graal_isolate_t *isolate = NULL;
        //             graal_isolatethread_t *thread = NULL;

        //         public:
        //             GraphReader(std::string file_name) :
        //                 Reader(file_name),
        //                 entries_counter(0ull)
        //             {
        //                 this->max_per_dimension = std::vector<std::uint64_t>();
        //                 std::string properties_file = samg::utils::change_extension(file_name,"properties");
        //                 std::string defaults = samg::utils::read_from_file( properties_file.data() );
        //                 std::map<std::string, std::string> properties;
        //                 if( cpp_properties::parse(defaults.begin(),defaults.end(),properties) ) {
        //                     std::uint64_t nodes = std::atoll( properties["nodes"].data() );
        //                     std::uint64_t arcs = std::atoll( properties["arcs"].data() );
        //                     this->max_per_dimension = { nodes , nodes };
        //                     this->number_of_entries = arcs;
        //                     this->matrix_side_size = nodes;
        //                     this->matrix_size = nodes*nodes;
        //                     this->matrix_expected_density = (double) arcs / (double) this->matrix_size;
        //                     this->matrix_actual_density = (double) arcs / (double) this->matrix_size;
        //                     this->matrix_distribution = "null";
        //                     std::float_t gauss_mu = -1.0f;
        //                     std::float_t gauss_sigma = -1.0f;
        //                     std::uint64_t clustering = 0ull;
        //                     std::float_t clustering_distance_error = -1.0f;
        //                 } else {
        //                     throw std::runtime_error("Graph properties file \""+properties_file+"\" not available.");
        //                 }

        //                 if( graal_create_isolate( NULL, &(this->isolate), &(this->thread) ) != 0) {
        //                     throw std::runtime_error("GraalVM thread initialization error!");
        //                 }

        //                 // std::cout << "file_name = " << file_name << std::endl;
        //                 std::string grap_file = samg::utils::get_file_basename(file_name);
        //                 // std::cout << "grap_file = " << grap_file << std::endl;
        //                 if( !load_graph( this->thread, grap_file.data() ) ) {
        //                     throw std::runtime_error("Error in graph file loading!");
        //                 }
        //             }
        //             ~GraphReader() {
        //                 graal_tear_down_isolate(this->thread);
        //             }

        //             const std::size_t get_number_of_dimensions() const override {
        //                 return this->max_per_dimension.size();
        //             }
        //             const std::vector<std::uint64_t> get_max_per_dimension() const override {
        //                 return this->max_per_dimension;
        //             }
        //             const std::uint64_t get_number_of_entries() const override {
        //                 return this->number_of_entries;
        //             }
        //             const bool has_next() override {
        //                 // std::cout << "is_there_next_entry> number of entries = " << this->number_of_entries << "; eof? = " << this->input_file.eof() << std::endl;
        //                 return ( this->number_of_entries > 0 ) && ( this->entries_counter < this->number_of_entries );
        //             }

        //             const std::uint64_t get_matrix_side_size() const override {
        //                 // std::cout << "GraphReader>get_matrix_side_size> (1)" << std::endl;
        //                 return this->matrix_side_size;
        //             }
        //             const std::uint64_t get_matrix_size() const override {
        //                 return this->matrix_size;
        //             }
        //             const std::float_t get_matrix_expected_density() const override {
        //                 return this->matrix_expected_density;
        //             }
        //             const std::float_t get_matrix_actual_density() const override {
        //                 return this->matrix_actual_density;
        //             }
        //             const std::string get_matrix_distribution() const override {
        //                 return this->matrix_distribution;
        //             }
        //             const std::float_t get_gauss_mu() const override {
        //                 return this->gauss_mu;
        //             }
        //             const std::float_t get_gauss_sigma() const override {
        //                 return this->gauss_sigma;
        //             }
        //             const std::uint64_t get_clustering() const override {
        //                 return this->clustering;
        //             }
        //             const std::float_t get_clustering_distance_error() const override {
        //                 return this->clustering_distance_error;
        //             }

        //             std::vector<std::uint64_t> next() override {
        //                 std::string entry = std::string(get_next_entry(this->thread));
        //                 if( entry != "eos" ) {
        //                     std::vector<std::uint64_t> entries = std::vector<std::uint64_t>(this->max_per_dimension.size());
        //                     std::string token;
        //                     std::stringstream strm(entry);
        //                     for(std::size_t d=0;d<this->max_per_dimension.size()&&std::getline(strm,token,'\t');d++){
        //                         // entries.push_back(std::stoull(token));
        //                         entries[d] = std::stoull(token);
        //                         // std::cout << "(1)" << std::endl;
        //                     }
        //                     if(entries.size() == this->max_per_dimension.size()) {
        //                         // std::cout << "(3)" << std::endl;
        //                         this->entries_counter++;
        //                         return entries;
        //                     } else {
        //                         throw std::runtime_error("Wrong entry format.");
        //                     }
        //                 } else {
        //                     throw std::runtime_error("No more entries.");
        //                 }
        //             }
        //     };

        // };
    }
}
