/**
 * @file matutx-snap.cpp
 * @author Sebastián AMG (@sebastianamg)
 * @brief Static library implemented using C++-98 standard that allows to read data from Snap format.  
 * @version 0.1
 * @date 2024-06-26
 * @note This code uses [link https://github.com/snap-stanford/snap snap]
 * @note compilation to generate the static library for debugging: g++-13 -std=c++98 -ggdb -g -O0 -Wall -DNDEBUG -fopenmp -I ~/include/Snap-6.0/snap-core/ -I ~/include/Snap-
6.0/glib-core/ -I ~/include/samg/ -c ./samg/matutx-snap.cpp -o ./samg/matutx-snap.o -lsnap -lrt && ar rcs samg/libmatutx-snap.a samg/matutx-snap.o
 * @note compilation to generate the static library for production: g++-13 -std=c++98 -O3 -DNDEBUG -fopenmp -I ~/include/Snap-6.0/snap-core/ -I ~/include/Snap-
6.0/glib-core/ -I ~/include/samg/ -c ./samg/matutx-snap.cpp -o ./samg/matutx-snap.o -lsnap -lrt && ar rcs samg/libmatutx-snap.a samg/matutx-snap.o
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

#ifndef MATUTX_SNAP_H
#define MATUTX_SNAP_H

#include <Snap.h>

namespace samg {
    namespace matutx {
        namespace reader {
            class SnapReader {
                private:
                    const TStr FILE_NAME;
                    PNGraph graph;
                    TIntV v;
                    unsigned long long int nid, ntrg;
                    TNGraph::TNodeI NI;
                    // void _update_( );
                    // unsigned long long int n;
                    // unsigned long long int b;
                    // unsigned long long int d;
                    // unsigned long long int initial_M;
                    // unsigned long long int bd;
                    samg::utils::ZValueConverter z_converter;

                public:
                    SnapReader( const char *file_name, const unsigned int src_col_id = 0U, const unsigned int dst_col_id = 1U, const unsigned int k=2ULL  );
                    const char* get_input_file_name();
                    const unsigned long long int get_number_of_dimensions();
                    unsigned long long int* get_max_per_dimension();
                    const unsigned long long int get_number_of_entries();
                    const unsigned long long int get_matrix_side_size();
                    const unsigned long long int get_matrix_size();
                    const float get_matrix_expected_density();
                    const float get_matrix_actual_density();
                    const char* get_matrix_distribution();
                    const float get_gauss_mu();
                    const float get_gauss_sigma();
                    const unsigned long long int get_clustering();
                    const float get_clustering_distance_error();
                    const bool has_next();
                    const unsigned long long int* next();
                    const unsigned long long int next_zvalue();
            };
        }
    }
}

#endif