/**
 * @file matutx-snap.cpp
 * @author Sebastián AMG (@sebastianamg)
 * @brief Static library implemented using C++-98 standard that allows to read data from Snap format.  
 * @version 0.1
 * @date 2024-06-26
 * @note This code uses [link https://github.com/snap-stanford/snap snap]
 * 
 * @copyright (c) 2024 Sebastián AMG (@sebastianamg)
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
// #include <Snap.h>
// #include <samg/matutx-snap.hpp>
#include "matutx-snap.hpp"

namespace samg {
    namespace matutx {
        namespace reader {

            // void SnapReader::_update( ) {

            // }

            SnapReader::SnapReader( const char *file_name, const unsigned int src_col_id, const unsigned int dst_col_id ) :
                FILE_NAME ( TStr(file_name) ),
                graph ( TSnap::LoadEdgeList<PNGraph>(file_name, src_col_id, dst_col_id) ) {
                    this->graph->GetNIdV(this->v);
                    this->v.Sort();
                    this->nid = 0ULL;
                    this->ntrg = 0ULL;
                    this->NI = this->graph->GetNI((int)this->v[this->nid]);
                    this->NI.SortNIdV();
            }

            const char* SnapReader::get_input_file_name() {
                return this->FILE_NAME.CStr();
            }

            const unsigned long long int SnapReader::get_number_of_dimensions() {
                return 2ULL;
            }

            unsigned long long int* SnapReader::get_max_per_dimension() {
                unsigned long long int* maxd = new unsigned long long int[2];
                maxd[ 0 ] = maxd[ 1 ] = this->v[ this->v.Len()-1 ];
                return maxd;
            }

            const unsigned long long int SnapReader::get_number_of_entries() {
                return this->graph->GetEdges();
            }

            const unsigned long long int SnapReader::get_matrix_side_size() {
                return this->v[ this->v.Len()-1 ];
            }

            const unsigned long long int SnapReader::get_matrix_size() {
                return this->v[ this->v.Len()-1 ] * this->v[ this->v.Len()-1 ];
            }

            const float SnapReader::get_matrix_expected_density() {
                return ( (float) this->graph->GetEdges() ) / ( (float) this->get_matrix_size() );
            }

            const float SnapReader::get_matrix_actual_density() {
                return this->get_matrix_expected_density();
            }
            
            const char* SnapReader::get_matrix_distribution() {
                return "unknown";
            }

            const float SnapReader::get_gauss_mu() {
                return 0.0F;
            }

            const float SnapReader::get_gauss_sigma() {
                return 0.0F;
            }

            const unsigned long long int SnapReader::get_clustering() {
                return 0ULL;
            }

            const float SnapReader::get_clustering_distance_error() {
                return 0.0F;
            }

            const bool SnapReader::has_next() {
                if( ( (int) this->nid ) >= this->v.Len() ) {
                    return false;
                }

                while( this->NI.GetOutDeg() == 0 || ( ( (int) this->ntrg ) >= this->NI.GetOutDeg() ) ) {
                    this->nid++;
                    if( ( (int) this->nid ) < v.Len() ) {
                        this->NI = this->graph->GetNI( (int)this->v[ this->nid ] );
                        this->NI.SortNIdV( );
                        this->ntrg = 0ULL;
                        // break;
                    } else {
                        return false;
                    }
                }

                return  true;
            }

            unsigned long long int* SnapReader::next() {
                if( this->has_next() ) {
                    // if( ( (int) this->ntrg ) < this->NI.GetOutDeg() ) {
                    unsigned long long int* edge = new unsigned long long int[ 2 ];
                    edge[ 0 ] = (unsigned long long int) this->v[ this->nid ];
                    edge[ 1 ] = this->NI.GetOutNId( this->ntrg );
                    this->ntrg++;
                    return edge;
                }
                throw 123; // NOTE No more entries!
            }
        }
    }
}

// int main(int argc, char const *argv[]) {
//     // const char* file_name = "test.txt";
//     const TStr file_name_ = TStr(argv[1]);
//     const int src_col_id = 0,
//                 dst_col_id = 1;
//     PNGraph graph = TSnap::LoadEdgeList<PNGraph>(file_name_, src_col_id, dst_col_id);
    
//     TIntV v;
//     graph->GetNIdV(v);

//     v.Sort();
//     // graph->SortNodeAdjV();
//     // graph->GetNode().

//     for( std::size_t nid; nid < v.Len(); nid++ ) {
//         TNGraph::TNodeI NI = graph->GetNI((int)v[nid]);
//         NI.SortNIdV();
//         for ( std::size_t ntrg = 0; ntrg < NI.GetOutDeg() ; ntrg++) {
//             printf("%d\t%d\n", (int) v[nid],  NI.GetOutNId(ntrg));
//         }
//     }

    
//     // traverse the edges
//     // for (TNGraph::TEdgeI EI = graph->BegEI(); EI < graph->EndEI(); EI++) {
//     //     printf("edge (%d, %d)\n", EI.GetSrcNId(), EI.GetDstNId());
//     // }

//     // for (TNGraph::TNodeI NI = graph->BegNI(); NI < graph->EndNI(); NI++) {
//     //     for (int e = 0; e < NI.GetOutDeg(); e++) {
//     //         printf("edge (%d %d)\n", NI.GetId(), NI.GetOutNId(e));
//     //     }
//     // }

//     return 0;
// }
