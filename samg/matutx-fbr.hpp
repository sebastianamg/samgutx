#pragma once
#include <samg/commons.hpp>
#include <samg/mmm-interface.hpp>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <limits>
#include <optional>
#include <regex>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

/**
 * ---------------------------------------------------------------
 * Released under the 2-Clause BSD License 
 * (a.k.a. Simplified BSD License or FreeBSD License)
 * @note [link https://opensource.org/license/bsd-2-clause/ BSD-2-Clause]
 * ---------------------------------------------------------------
 * 
 * @copyright (c) 2026 Sebastián AMG (@sebastianamg)
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

            /**
             * @brief In-memory Reader for headerless Flat Binary Raster files (.bin / .fbr, row-major 32-bit signed integers)
             *        matching the exact encoding and 5-tier dimension resolution of bin2mxs.cpp.
             *
             * @details Loads the flat binary cell array (int32_t) into memory in a single contiguous read,
             *          automatically resolves (rows, cols) via explicit overrides, sidecar/manifest files,
             *          filename patterns, the Silva-Coira benchmark catalog, or divisor-constrained
             *          spatial row-autocorrelation, normalizes negative elevation values via z_offset,
             *          and streams lexicographically ordered coordinates (row, col, z) [3D] or (row, col) [2D]
             *          directly from memory with O(1) restart() and O(1) contains() support.
             */
            class FBRReader : public Reader {
                public:
                    using ElemT = std::int32_t;

                    struct GridDimensions {
                        std::uint64_t rows = 0;
                        std::uint64_t cols = 0;
                        std::string resolution_method;
                        std::optional<std::int64_t> sidecar_nodata = std::nullopt;
                    };

                private:
                    std::vector<ElemT> cells;
                    std::uint64_t rows = 0;
                    std::uint64_t cols = 0;
                    std::string resolution_method;
                    bool is_2d = false;
                    std::int64_t threshold = 1;
                    std::optional<std::int64_t> nodata_val = std::nullopt;
                    std::int64_t min_val = 0;
                    std::int64_t max_val = 0;
                    std::int64_t z_offset = 0;

                    std::vector<std::uint64_t> maxs;
                    std::uint64_t e = 0;
                    std::uint64_t s = 0;
                    std::uint64_t c = 0;
                    std::float_t d = 0.0f;
                    std::float_t actual_d = 0.0f;
                    std::float_t cderr = 0.0f;
                    std::string dist = "raster";

                    std::size_t current_entry = 0;
                    std::size_t cell_idx = 0;
                    std::uint64_t current_row = 0;
                    std::uint64_t current_col = 0;
                    std::vector<std::uint64_t> _next_result;
                    samg::utils::ZValueConverter z_converter;

                    static std::optional<GridDimensions> try_parse_sidecar(
                        const std::filesystem::path& bin_path,
                        std::uint64_t total_cells)
                    {
                        namespace fs = std::filesystem;
                        const std::vector<fs::path> candidates = {
                            fs::path(bin_path).replace_extension(".hdr"),
                            fs::path(bin_path.string() + ".hdr"),
                            fs::path(bin_path).replace_extension(".dim"),
                            fs::path(bin_path).replace_extension(".info"),
                            fs::path(bin_path).replace_extension(".meta")
                        };

                        for (const auto& sidecar : candidates) {
                            if (!fs::exists(sidecar) || !fs::is_regular_file(sidecar)) {
                                continue;
                            }
                            std::ifstream in(sidecar);
                            if (!in) continue;

                            std::uint64_t r_parsed = 0, c_parsed = 0;
                            std::optional<std::int64_t> nodata = std::nullopt;
                            std::string line;
                            while (std::getline(in, line)) {
                                for (char& ch : line) {
                                    if (ch == '=' || ch == ':') ch = ' ';
                                }
                                std::istringstream iss(line);
                                std::string key;
                                if (!(iss >> key)) continue;
                                std::string key_lower = key;
                                std::transform(key_lower.begin(), key_lower.end(), key_lower.begin(),
                                               [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });

                                if (key_lower == "nrows" || key_lower == "rows" || key_lower == "lines") {
                                    iss >> r_parsed;
                                } else if (key_lower == "ncols" || key_lower == "cols" || key_lower == "samples") {
                                    iss >> c_parsed;
                                } else if (key_lower == "nodata" || key_lower == "nodata_value") {
                                    std::int64_t nd = 0;
                                    if (iss >> nd) nodata = nd;
                                } else {
                                    try {
                                        std::uint64_t r_cand = std::stoull(key);
                                        std::uint64_t c_cand = 0;
                                        if ((iss >> c_cand) && r_cand * c_cand == total_cells) {
                                            return GridDimensions{r_cand, c_cand, "sidecar (" + sidecar.filename().string() + ")", nodata};
                                        }
                                    } catch (...) {}
                                }
                            }
                            if (r_parsed > 0 && c_parsed > 0 && r_parsed * c_parsed == total_cells) {
                                return GridDimensions{r_parsed, c_parsed, "sidecar (" + sidecar.filename().string() + ")", nodata};
                            }
                        }

                        const fs::path parent = bin_path.parent_path().empty() ? fs::path(".") : bin_path.parent_path();
                        const std::string fname = bin_path.filename().string();
                        const std::string stem  = bin_path.stem().string();
                        for (const char* manifest_name : {"dataset.txt", "datasets.txt", "list.txt", "dimensions.txt"}) {
                            fs::path manifest = parent / manifest_name;
                            if (!fs::exists(manifest)) continue;
                            std::ifstream in(manifest);
                            std::string line;
                            while (std::getline(in, line)) {
                                if (line.find(fname) == std::string::npos && line.find(stem) == std::string::npos) continue;
                                std::istringstream iss(line);
                                std::vector<std::string> tokens;
                                for (std::string tok; iss >> tok;) tokens.push_back(tok);
                                for (std::size_t i = 0; i + 1 < tokens.size(); ++i) {
                                    try {
                                        std::uint64_t r_cand = std::stoull(tokens[i]);
                                        std::uint64_t c_cand = std::stoull(tokens[i + 1]);
                                        if (r_cand > 0 && c_cand > 0 && r_cand * c_cand == total_cells) {
                                            return GridDimensions{r_cand, c_cand, std::string("manifest (") + manifest_name + ")", std::nullopt};
                                        }
                                    } catch (...) {}
                                }
                            }
                        }

                        return std::nullopt;
                    }

                    static std::optional<GridDimensions> try_parse_filename_pattern(
                        const std::filesystem::path& bin_path,
                        std::uint64_t total_cells)
                    {
                        const std::string stem = bin_path.stem().string();
                        static const std::regex dim_re(R"((\d{2,6})[xX_](\d{2,6}))");
                        auto begin = std::sregex_iterator(stem.begin(), stem.end(), dim_re);
                        auto end   = std::sregex_iterator();

                        for (auto it = begin; it != end; ++it) {
                            const std::uint64_t a = std::stoull((*it)[1].str());
                            const std::uint64_t b = std::stoull((*it)[2].str());
                            if (a > 0 && b > 0 && a * b == total_cells) {
                                return GridDimensions{a, b, "filename pattern (" + (*it)[0].str() + ")", std::nullopt};
                            }
                        }
                        return std::nullopt;
                    }

                    static GridDimensions infer_dimensions_from_cells(
                        const std::vector<ElemT>& cell_data,
                        std::uint64_t total_cells,
                        double min_aspect,
                        double max_aspect)
                    {
                        struct KnownShape { std::uint64_t rows; std::uint64_t cols; const char* label; };
                        static constexpr KnownShape kKnownCatalog[] = {
                            {3600,  3600,  "Silva-Coira WorldClim clim-1x1"},
                            {7200,  7200,  "Silva-Coira WorldClim clim-2x2"},
                            {10800, 10800, "Silva-Coira WorldClim clim-3x3"},
                            {14400, 14400, "Silva-Coira WorldClim clim-4x4"},
                            {3881,  5841,  "SGI LiDAR DTM (MDT05-0533-H30)"},
                            {4100,  5849,  "Silva-Coira SGI DTM dfm-1x1"},
                            {8242,  11737, "Silva-Coira SGI DTM dfm-2x2"},
                            {12403, 17643, "Silva-Coira SGI DTM dfm-3x3"},
                            {16564, 23564, "Silva-Coira SGI DTM dfm-4x4"}
                        };
                        for (const auto& k : kKnownCatalog) {
                            if (k.rows * k.cols == total_cells) {
                                return GridDimensions{k.rows, k.cols, k.label, std::nullopt};
                            }
                        }

                        std::vector<std::pair<std::uint64_t, std::uint64_t>> candidates;
                        const auto root = static_cast<std::uint64_t>(
                            std::llround(std::sqrt(static_cast<double>(total_cells))));

                        for (std::uint64_t div = 1; div * div <= total_cells; ++div) {
                            if (total_cells % div != 0) continue;
                            const std::uint64_t quot = total_cells / div;

                            const double aspect1 = static_cast<double>(div) / static_cast<double>(quot);
                            if (aspect1 >= min_aspect && aspect1 <= max_aspect) {
                                candidates.emplace_back(div, quot);
                            }
                            if (div != quot) {
                                const double aspect2 = static_cast<double>(quot) / static_cast<double>(div);
                                if (aspect2 >= min_aspect && aspect2 <= max_aspect) {
                                    candidates.emplace_back(quot, div);
                                }
                            }
                        }

                        if (candidates.empty()) {
                            if (root * root == total_cells) {
                                return GridDimensions{root, root, "perfect-square fallback", std::nullopt};
                            }
                            throw std::runtime_error(
                                "FBRReader: No factor pair (rows, cols) of total_cells=" + std::to_string(total_cells) +
                                " satisfies the aspect ratio bounds [" + std::to_string(min_aspect) + ", " +
                                std::to_string(max_aspect) + "].");
                        }

                        if (candidates.size() == 1) {
                            return GridDimensions{
                                candidates[0].first,
                                candidates[0].second,
                                (candidates[0].first == candidates[0].second)
                                    ? "unique perfect-square factor"
                                    : "unique aspect-bounded factor",
                                std::nullopt};
                        }

                        std::uint64_t max_cand_cols = 0;
                        for (const auto& [r_cand, c_cand] : candidates) {
                            max_cand_cols = std::max(max_cand_cols, c_cand);
                        }

                        const std::uint64_t sample_cells = std::min<std::uint64_t>(
                            total_cells, std::max<std::uint64_t>(max_cand_cols * 64ULL, 1ULL << 20));
                        const std::uint64_t start_cell = (total_cells - sample_cells) / 2ULL;

                        double best_score = std::numeric_limits<double>::infinity();
                        std::pair<std::uint64_t, std::uint64_t> best_pair = candidates[0];

                        for (const auto& [r_cand, c_cand] : candidates) {
                            if (c_cand >= sample_cells) continue;
                            const std::uint64_t limit = sample_cells - c_cand;
                            const std::uint64_t step  = std::max<std::uint64_t>(1ULL, limit / 65536ULL);

                            double diff_sum = 0.0;
                            std::uint64_t count = 0;
                            for (std::uint64_t i = 0; i < limit; i += step) {
                                const double v1 = static_cast<double>(cell_data[start_cell + i]);
                                const double v2 = static_cast<double>(cell_data[start_cell + i + c_cand]);
                                diff_sum += std::abs(v1 - v2);
                                ++count;
                            }
                            const double mean_vertical_diff = (count > 0)
                                ? (diff_sum / static_cast<double>(count))
                                : std::numeric_limits<double>::infinity();
                            if (mean_vertical_diff < best_score) {
                                best_score = mean_vertical_diff;
                                best_pair  = {r_cand, c_cand};
                            }
                        }

                        return GridDimensions{
                            best_pair.first,
                            best_pair.second,
                            "spatial row-autocorrelation variogram minimum (mean |A[i]-A[i+C]| = " +
                                std::to_string(best_score) + ")",
                            std::nullopt};
                    }

                public:
                    FBRReader(
                        const std::string input_file_name,
                        const std::size_t k = 2UL,
                        const std::uint64_t explicit_rows = 0ULL,
                        const std::uint64_t explicit_cols = 0ULL,
                        const bool is_2d_mode = false,
                        const std::int64_t threshold_val = 1LL,
                        const std::optional<std::int64_t> explicit_nodata = std::nullopt,
                        const double min_aspect = 0.2,
                        const double max_aspect = 5.0
                    ) :
                        Reader(input_file_name),
                        is_2d(is_2d_mode),
                        threshold(threshold_val)
                    {
                        namespace fs = std::filesystem;
                        const fs::path input_path(input_file_name);
                        if (!fs::exists(input_path) || !fs::is_regular_file(input_path)) {
                            throw std::runtime_error("FBRReader: Input file does not exist or is not a regular file: " + input_file_name);
                        }

                        const std::uint64_t file_bytes = fs::file_size(input_path);
                        if (file_bytes == 0 || file_bytes % sizeof(ElemT) != 0) {
                            throw std::runtime_error(
                                "FBRReader: File size (" + std::to_string(file_bytes) +
                                " bytes) is not a non-zero multiple of 4-byte int32_t cells.");
                        }
                        const std::uint64_t total_cells = file_bytes / sizeof(ElemT);

                        // Bulk-load the entire flat binary raster into memory in a single read
                        this->cells.resize(total_cells);
                        {
                            std::ifstream ifs(input_file_name, std::ios::binary);
                            if (!ifs) {
                                throw std::runtime_error("FBRReader: Could not open input file: " + input_file_name);
                            }
                            ifs.read(reinterpret_cast<char*>(this->cells.data()),
                                     static_cast<std::streamsize>(file_bytes));
                            if (static_cast<std::uint64_t>(ifs.gcount()) != file_bytes) {
                                throw std::runtime_error("FBRReader: Unexpected EOF while loading " + input_file_name);
                            }
                        }

                        // Multi-tier dimension resolution
                        GridDimensions dims;
                        if (explicit_rows > 0 && explicit_cols > 0) {
                            dims.rows = explicit_rows;
                            dims.cols = explicit_cols;
                            dims.resolution_method = "explicit (rows, cols)";
                        } else if (explicit_rows > 0) {
                            if (total_cells % explicit_rows != 0) {
                                throw std::runtime_error("FBRReader: total_cells is not divisible by explicit_rows.");
                            }
                            dims.rows = explicit_rows;
                            dims.cols = total_cells / explicit_rows;
                            dims.resolution_method = "explicit rows + total_cells / rows";
                        } else if (explicit_cols > 0) {
                            if (total_cells % explicit_cols != 0) {
                                throw std::runtime_error("FBRReader: total_cells is not divisible by explicit_cols.");
                            }
                            dims.cols = explicit_cols;
                            dims.rows = total_cells / explicit_cols;
                            dims.resolution_method = "explicit cols + total_cells / cols";
                        } else if (auto sidecar_dims = try_parse_sidecar(input_path, total_cells)) {
                            dims = *sidecar_dims;
                        } else if (auto fname_dims = try_parse_filename_pattern(input_path, total_cells)) {
                            dims = *fname_dims;
                        } else {
                            dims = infer_dimensions_from_cells(this->cells, total_cells, min_aspect, max_aspect);
                        }

                        if (dims.rows == 0 || dims.cols == 0 || dims.rows * dims.cols != total_cells) {
                            throw std::runtime_error(
                                "FBRReader: Dimension mismatch: rows (" + std::to_string(dims.rows) +
                                ") * cols (" + std::to_string(dims.cols) +
                                ") != total_cells (" + std::to_string(total_cells) + ").");
                        }

                        this->rows = dims.rows;
                        this->cols = dims.cols;
                        this->resolution_method = dims.resolution_method;
                        this->nodata_val = explicit_nodata.has_value() ? explicit_nodata : dims.sidecar_nodata;

                        // Single in-memory scan for value bounds and active entry count
                        this->min_val = std::numeric_limits<std::int64_t>::max();
                        this->max_val = std::numeric_limits<std::int64_t>::min();
                        std::uint64_t valid_entries = 0;

                        for (std::size_t i = 0; i < total_cells; ++i) {
                            const std::int64_t v = static_cast<std::int64_t>(this->cells[i]);
                            if (this->nodata_val && v == *this->nodata_val) continue;
                            if (this->is_2d) {
                                if (v >= this->threshold) ++valid_entries;
                            } else {
                                if (v < this->min_val) this->min_val = v;
                                if (v > this->max_val) this->max_val = v;
                                ++valid_entries;
                            }
                        }

                        this->e = valid_entries;
                        if (this->e == 0) {
                            this->min_val = 0;
                            this->max_val = 0;
                        }

                        this->z_offset = (!this->is_2d && this->min_val < 0) ? -this->min_val : 0;
                        const std::uint64_t max_z = (this->is_2d || this->e == 0)
                            ? 0ULL
                            : static_cast<std::uint64_t>(this->max_val + this->z_offset);

                        this->maxs = this->is_2d
                            ? std::vector<std::uint64_t>{this->rows - 1, this->cols - 1}
                            : std::vector<std::uint64_t>{this->rows - 1, this->cols - 1, max_z};

                        this->s = this->is_2d
                            ? std::max(this->rows, this->cols)
                            : std::max({this->rows, this->cols, max_z + 1});

                        const double domain_volume = std::pow(
                            static_cast<double>(this->s),
                            static_cast<double>(this->maxs.size()));
                        this->actual_d = (domain_volume > 0.0)
                            ? static_cast<std::float_t>(static_cast<double>(this->e) / domain_volume)
                            : 0.0f;
                        this->d = this->actual_d;
                        this->dist = "raster";
                        this->c = 0ULL;
                        this->cderr = 0.0f;

                        this->_next_result.resize(this->maxs.size(), 0ULL);
                        this->z_converter = samg::utils::ZValueConverter(this->s, this->maxs.size(), k);
                        this->restart();
                    }

                    void restart() {
                        this->current_entry = 0ZU;
                        this->cell_idx = 0ZU;
                        this->current_row = 0ULL;
                        this->current_col = 0ULL;
                    }

                    std::uint64_t get_rows() const { return this->rows; }
                    std::uint64_t get_cols() const { return this->cols; }
                    std::int64_t get_min_value() const { return this->min_val; }
                    std::int64_t get_max_value() const { return this->max_val; }
                    std::int64_t get_z_offset() const { return this->z_offset; }
                    const std::string& get_resolution_method() const { return this->resolution_method; }
                    const std::vector<ElemT>& get_raw_cells() const { return this->cells; }
                    std::size_t get_memory_size_bytes() const { return this->cells.size() * sizeof(ElemT); }
                    std::size_t get_memory_size_bits() const { return this->get_memory_size_bytes() * 8ZU; }

                    template<typename CoordIntType>
                    bool contains(const std::vector<CoordIntType>& coord) const {
                        if (coord.size() != this->maxs.size()) return false;
                        if constexpr (std::is_signed_v<CoordIntType>) {
                            for (const auto& val : coord) {
                                if (val < 0) return false;
                            }
                        }
                        const std::uint64_t r = static_cast<std::uint64_t>(coord[0]);
                        const std::uint64_t c_idx = static_cast<std::uint64_t>(coord[1]);
                        if (r >= this->rows || c_idx >= this->cols) return false;

                        const std::int64_t v = static_cast<std::int64_t>(this->cells[r * this->cols + c_idx]);
                        if (this->nodata_val && v == *this->nodata_val) return false;

                        if (this->is_2d) {
                            return v >= this->threshold;
                        } else {
                            const std::uint64_t z = static_cast<std::uint64_t>(coord[2]);
                            return static_cast<std::uint64_t>(v + this->z_offset) == z;
                        }
                    }

                    const std::size_t get_number_of_dimensions() const override {
                        return this->maxs.size();
                    }
                    const std::vector<std::uint64_t> get_max_per_dimension() const override {
                        return this->maxs;
                    }
                    const std::uint64_t get_number_of_entries() const override {
                        return this->e;
                    }
                    const std::uint64_t get_matrix_side_size() const override {
                        return this->s;
                    }
                    const std::uint64_t get_matrix_size() const override {
                        return static_cast<std::uint64_t>(std::pow(this->s, this->maxs.size()));
                    }
                    const std::float_t get_matrix_expected_density() const override {
                        return this->d;
                    }
                    const std::float_t get_matrix_actual_density() const override {
                        return this->actual_d;
                    }
                    const std::string get_matrix_distribution() const override {
                        return this->dist;
                    }
                    const std::float_t get_gauss_mu() const override {
                        return 0.0f;
                    }
                    const std::float_t get_gauss_sigma() const override {
                        return 0.0f;
                    }
                    const std::uint64_t get_clustering() const override {
                        return this->c;
                    }
                    const std::float_t get_clustering_distance_error() const override {
                        return this->cderr;
                    }

                    const bool has_next() override {
                        return this->current_entry < this->e;
                    }

                    const std::vector<std::uint64_t>& next() override {
                        if (!this->has_next()) {
                            throw std::runtime_error("No more entries!");
                        }
                        while (this->cell_idx < this->cells.size()) {
                            const std::int64_t v = static_cast<std::int64_t>(this->cells[this->cell_idx++]);
                            const std::uint64_t r = this->current_row;
                            const std::uint64_t c_idx = this->current_col;
                            if (++this->current_col == this->cols) {
                                this->current_col = 0ULL;
                                ++this->current_row;
                            }
                            if (this->nodata_val && v == *this->nodata_val) {
                                continue;
                            }
                            if (this->is_2d) {
                                if (v >= this->threshold) {
                                    this->_next_result[0] = r;
                                    this->_next_result[1] = c_idx;
                                    ++this->current_entry;
                                    return this->_next_result;
                                }
                            } else {
                                this->_next_result[0] = r;
                                this->_next_result[1] = c_idx;
                                this->_next_result[2] = static_cast<std::uint64_t>(v + this->z_offset);
                                ++this->current_entry;
                                return this->_next_result;
                            }
                        }
                        throw std::runtime_error("No more entries!");
                    }

                    const std::uint64_t next_zvalue() override {
                        return this->z_converter.to_zvalue(this->next());
                    }

                    const std::vector<std::uint64_t> get_zvalues() override {
                        std::vector<std::uint64_t> ans;
                        ans.reserve(this->e - this->current_entry);
                        while (this->has_next()) {
                            ans.push_back(this->next_zvalue());
                        }
                        return ans;
                    }
            };

            using BINReader = FBRReader;

        }
    }
}

