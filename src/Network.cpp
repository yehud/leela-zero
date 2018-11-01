/*
    This file is part of Leela Zero.
    Copyright (C) 2017-2018 Gian-Carlo Pascutto and contributors

    Leela Zero is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Leela Zero is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Leela Zero.  If not, see <http://www.gnu.org/licenses/>.
*/


#include "config.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <iterator>
#include <memory>
#include <sstream>
#include <string>
#include <boost/utility.hpp>
#include <boost/format.hpp>
#include <boost/spirit/home/x3.hpp>
<<<<<<< HEAD
=======
#ifndef USE_BLAS
#include <Eigen/Dense>
#endif
>>>>>>> upstream/master

#ifdef __APPLE__
#include <Accelerate/Accelerate.h>
#endif
#ifdef USE_MKL
#include <mkl.h>
#endif
#ifdef USE_OPENBLAS
#include <cblas.h>
#endif
#include "zlib.h"
<<<<<<< HEAD
=======

#include "Network.h"
#include "CPUPipe.h"
>>>>>>> upstream/master
#ifdef USE_OPENCL
#include "OpenCLScheduler.h"
#include "UCTNode.h"
#endif
#include "FastBoard.h"
#include "FastState.h"
#include "FullBoard.h"
#include "GameState.h"
#include "GTP.h"
#include "NNCache.h"
#include "Random.h"
#include "ThreadPool.h"
#include "Timing.h"
#include "Utils.h"

namespace x3 = boost::spirit::x3;
using namespace Utils;

<<<<<<< HEAD
// Input + residual block tower
static std::vector<std::vector<float>> conv_weights;
static std::vector<std::vector<float>> conv_biases;
static std::vector<std::vector<float>> batchnorm_means;
static std::vector<std::vector<float>> batchnorm_stddivs;

// Policy head
static std::vector<float> conv_pol_w;
static std::vector<float> conv_pol_b;
static std::array<float, 2> bn_pol_w1;
static std::array<float, 2> bn_pol_w2;

static std::array<float, (BOARD_SQUARES + 1) * BOARD_SQUARES * 2> ip_pol_w;
static std::array<float, BOARD_SQUARES + 1> ip_pol_b;

// Value head
static std::vector<float> conv_val_w;
static std::vector<float> conv_val_b;
static std::array<float, 1> bn_val_w1;
static std::array<float, 1> bn_val_w2;

static std::array<float, BOARD_SQUARES * 256> ip1_val_w;
static std::array<float, 256> ip1_val_b;

static std::array<float, 256> ip2_val_w;
static std::array<float, 1> ip2_val_b;
static bool value_head_not_stm;

// Symmetry helper
static std::array<std::array<int, BOARD_SQUARES>, 8> symmetry_nn_idx_table;

=======
#ifndef USE_BLAS
// Eigen helpers
template <typename T>
using EigenVectorMap =
    Eigen::Map<Eigen::Matrix<T, Eigen::Dynamic, 1>>;
template <typename T>
using ConstEigenVectorMap =
    Eigen::Map<const Eigen::Matrix<T, Eigen::Dynamic, 1>>;
template <typename T>
using ConstEigenMatrixMap =
    Eigen::Map<const Eigen::Matrix<T, Eigen::Dynamic, Eigen::Dynamic>>;
#endif

// Symmetry helper
static std::array<std::array<int, NUM_INTERSECTIONS>,
                  Network::NUM_SYMMETRIES> symmetry_nn_idx_table;

float Network::benchmark_time(int centiseconds) {
    const auto cpus = cfg_num_threads;

    ThreadGroup tg(thread_pool);
    std::atomic<int> runcount{0};

    GameState state;
    state.init_game(BOARD_SIZE, 7.5);

    // As a sanity run, try one run with self check.
    // Isn't enough to guarantee correctness but better than nothing,
    // plus for large nets self-check takes a while (1~3 eval per second)
    get_output(&state, Ensemble::RANDOM_SYMMETRY, -1, true, true);

    const Time start;
    for (auto i = 0; i < cpus; i++) {
        tg.add_task([this, &runcount, start, centiseconds, state]() {
            while (true) {
                runcount++;
                get_output(&state, Ensemble::RANDOM_SYMMETRY, -1, true);
                const Time end;
                const auto elapsed = Time::timediff_centis(start, end);
                if (elapsed >= centiseconds) {
                    break;
                }
            }
        });
    }
    tg.wait_all();

    const Time end;
    const auto elapsed = Time::timediff_centis(start, end);
    return 100.0f * runcount.load() / elapsed;
}

>>>>>>> upstream/master
void Network::benchmark(const GameState* const state, const int iterations) {
    const auto cpus = cfg_num_threads;
    const Time start;

    ThreadGroup tg(thread_pool);
    std::atomic<int> runcount{0};

    for (auto i = 0; i < cpus; i++) {
<<<<<<< HEAD
        tg.add_task([&runcount, iterations, state]() {
            while (runcount < iterations) {
                runcount++;
                get_scored_moves(state, Ensemble::RANDOM_SYMMETRY, -1, true);
=======
        tg.add_task([this, &runcount, iterations, state]() {
            while (runcount < iterations) {
                runcount++;
                get_output(state, Ensemble::RANDOM_SYMMETRY, -1, true);
>>>>>>> upstream/master
            }
        });
    }
    tg.wait_all();

    const Time end;
    const auto elapsed = Time::timediff_seconds(start, end);
    myprintf("%5d evaluations in %5.2f seconds -> %d n/s\n",
             runcount.load(), elapsed, int(runcount.load() / elapsed));
}

<<<<<<< HEAD
void Network::process_bn_var(std::vector<float>& weights, const float epsilon) {
=======
template<class container>
void process_bn_var(container& weights) {
    constexpr float epsilon = 1e-5f;
>>>>>>> upstream/master
    for (auto&& w : weights) {
        w = 1.0f / std::sqrt(w + epsilon);
    }
}

std::vector<float> Network::winograd_transform_f(const std::vector<float>& f,
                                                 const int outputs,
                                                 const int channels) {
    // F(4x4, 3x3) Winograd filter transformation
    // transpose(G.dot(f).dot(G.transpose()))
    // U matrix is transposed for better memory layout in SGEMM
    auto U = std::vector<float>(WINOGRAD_TILE * outputs * channels);
<<<<<<< HEAD
    const auto G = std::array<float, WINOGRAD_TILE>{ 1.0,  0.0,  0.0,
                                                     0.5,  0.5,  0.5,
                                                     0.5, -0.5,  0.5,
                                                     0.0,  0.0,  1.0};
    auto temp = std::array<float, 12>{};

    for (auto o = 0; o < outputs; o++) {
        for (auto c = 0; c < channels; c++) {
            for (auto i = 0; i < 4; i++){
                for (auto j = 0; j < 3; j++) {
                    auto acc = 0.0f;
                    for (auto k = 0; k < 3; k++) {
                        acc += G[i*3 + k] * f[o*channels*9 + c*9 + k*3 + j];
=======
    const auto G = std::array<float, 3 * WINOGRAD_ALPHA>
                    { 1.0f,        0.0f,      0.0f,
                      -2.0f/3.0f, -SQ2/3.0f, -1.0f/3.0f,
                      -2.0f/3.0f,  SQ2/3.0f, -1.0f/3.0f,
                      1.0f/6.0f,   SQ2/6.0f,  1.0f/3.0f,
                      1.0f/6.0f,  -SQ2/6.0f,  1.0f/3.0f,
                      0.0f,        0.0f,      1.0f};

    auto temp = std::array<float, 3 * WINOGRAD_ALPHA>{};

    constexpr auto max_buffersize = 8;
    auto buffersize = max_buffersize;

    if (outputs % buffersize != 0) {
        buffersize = 1;
    }

    std::array<float, max_buffersize * WINOGRAD_ALPHA * WINOGRAD_ALPHA> buffer;

    for (auto c = 0; c < channels; c++) {
        for (auto o_b = 0; o_b < outputs/buffersize; o_b++) {
            for (auto bufferline = 0; bufferline < buffersize; bufferline++) {
                const auto o = o_b * buffersize + bufferline;

                for (auto i = 0; i < WINOGRAD_ALPHA; i++) {
                    for (auto j = 0; j < 3; j++) {
                        auto acc = 0.0f;
                        for (auto k = 0; k < 3; k++) {
                            acc += G[i*3 + k] * f[o*channels*9 + c*9 + k*3 + j];
                        }
                        temp[i*3 + j] = acc;
>>>>>>> upstream/master
                    }
                }

<<<<<<< HEAD
            for (auto xi = 0; xi < 4; xi++) {
                for (auto nu = 0; nu < 4; nu++) {
                    auto acc = 0.0f;
                    for (auto k = 0; k < 3; k++) {
                        acc += temp[xi*3 + k] * G[nu*3 + k];
=======
                for (auto xi = 0; xi < WINOGRAD_ALPHA; xi++) {
                    for (auto nu = 0; nu < WINOGRAD_ALPHA; nu++) {
                        auto acc = 0.0f;
                        for (auto k = 0; k < 3; k++) {
                            acc += temp[xi*3 + k] * G[nu*3 + k];
                        }
                        buffer[(xi * WINOGRAD_ALPHA + nu) * buffersize + bufferline] = acc;
>>>>>>> upstream/master
                    }
                }
            }
<<<<<<< HEAD
        }
    }

    return U;
}

std::vector<float> Network::zeropad_U(const std::vector<float>& U,
                                      const int outputs, const int channels,
                                      const int outputs_pad,
                                      const int channels_pad) {
    // Fill with zeroes
    auto Upad = std::vector<float>(WINOGRAD_TILE * outputs_pad * channels_pad);

    for (auto o = 0; o < outputs; o++) {
        for (auto c = 0; c < channels; c++) {
            for (auto xi = 0; xi < WINOGRAD_ALPHA; xi++){
                for (auto nu = 0; nu < WINOGRAD_ALPHA; nu++) {
                    Upad[xi * (WINOGRAD_ALPHA * outputs_pad * channels_pad)
                         + nu * (outputs_pad * channels_pad)
                         + c * outputs_pad +
                          o] =
                    U[xi * (WINOGRAD_ALPHA * outputs * channels)
                      + nu * (outputs * channels)
=======
            for (auto i = 0; i < WINOGRAD_ALPHA * WINOGRAD_ALPHA; i++) {
                for (auto entry = 0; entry < buffersize; entry++) {
                    const auto o = o_b * buffersize + entry;
                    U[i * outputs * channels
>>>>>>> upstream/master
                      + c * outputs
                      + o] =
                    buffer[buffersize * i + entry];
                }
            }
        }
    }

    return U;
}

std::pair<int, int> Network::load_v1_network(std::istream& wtfile) {
    // Count size of the network
    myprintf("Detecting residual layers...");
    // We are version 1 or 2
<<<<<<< HEAD
    if (value_head_not_stm) {
=======
    if (m_value_head_not_stm) {
>>>>>>> upstream/master
        myprintf("v%d...", 2);
    } else {
        myprintf("v%d...", 1);
    }
    // First line was the version number
    auto linecount = size_t{1};
    auto channels = 0;
    auto line = std::string{};
    while (std::getline(wtfile, line)) {
        auto iss = std::stringstream{line};
        // Third line of parameters are the convolution layer biases,
        // so this tells us the amount of channels in the residual layers.
        // We are assuming all layers have the same amount of filters.
        if (linecount == 2) {
            auto count = std::distance(std::istream_iterator<std::string>(iss),
                                       std::istream_iterator<std::string>());
            myprintf("%d channels...", count);
            channels = count;
        }
        linecount++;
    }
    // 1 format id, 1 input layer (4 x weights), 14 ending weights,
    // the rest are residuals, every residual has 8 x weight lines
    auto residual_blocks = linecount - (1 + 4 + 14);
    if (residual_blocks % 8 != 0) {
        myprintf("\nInconsistent number of weights in the file.\n");
        return {0, 0};
    }
    residual_blocks /= 8;
    myprintf("%d blocks.\n", residual_blocks);

    // Re-read file and process
    wtfile.clear();
    wtfile.seekg(0, std::ios::beg);

    // Get the file format id out of the way
    std::getline(wtfile, line);

    const auto plain_conv_layers = 1 + (residual_blocks * 2);
    const auto plain_conv_wts = plain_conv_layers * 4;
    linecount = 0;
    while (std::getline(wtfile, line)) {
        std::vector<float> weights;
        auto it_line = line.cbegin();
        const auto ok = phrase_parse(it_line, line.cend(),
                                     *x3::float_, x3::space, weights);
        if (!ok || it_line != line.cend()) {
            myprintf("\nFailed to parse weight file. Error on line %d.\n",
                    linecount + 2); //+1 from version line, +1 from 0-indexing
            return {0,0};
        }
        if (linecount < plain_conv_wts) {
            if (linecount % 4 == 0) {
                m_fwd_weights->m_conv_weights.emplace_back(weights);
            } else if (linecount % 4 == 1) {
                // Redundant in our model, but they encode the
                // number of outputs so we have to read them in.
                m_fwd_weights->m_conv_biases.emplace_back(weights);
            } else if (linecount % 4 == 2) {
                m_fwd_weights->m_batchnorm_means.emplace_back(weights);
            } else if (linecount % 4 == 3) {
                process_bn_var(weights);
                m_fwd_weights->m_batchnorm_stddevs.emplace_back(weights);
            }
        } else {
            switch (linecount - plain_conv_wts) {
                case  0: m_fwd_weights->m_conv_pol_w = std::move(weights); break;
                case  1: m_fwd_weights->m_conv_pol_b = std::move(weights); break;
                case  2: std::copy(cbegin(weights), cend(weights),
                                   begin(m_bn_pol_w1)); break;
                case  3: std::copy(cbegin(weights), cend(weights),
                                   begin(m_bn_pol_w2)); break;
                case  4: std::copy(cbegin(weights), cend(weights),
                                   begin(m_ip_pol_w)); break;
                case  5: std::copy(cbegin(weights), cend(weights),
                                   begin(m_ip_pol_b)); break;
                case  6: m_fwd_weights->m_conv_val_w = std::move(weights); break;
                case  7: m_fwd_weights->m_conv_val_b = std::move(weights); break;
                case  8: std::copy(cbegin(weights), cend(weights),
                                   begin(m_bn_val_w1)); break;
                case  9: std::copy(cbegin(weights), cend(weights),
                                   begin(m_bn_val_w2)); break;
                case 10: std::copy(cbegin(weights), cend(weights),
                                   begin(m_ip1_val_w)); break;
                case 11: std::copy(cbegin(weights), cend(weights),
                                   begin(m_ip1_val_b)); break;
                case 12: std::copy(cbegin(weights), cend(weights),
                                   begin(m_ip2_val_w)); break;
                case 13: std::copy(cbegin(weights), cend(weights),
                                   begin(m_ip2_val_b)); break;
            }
<<<<<<< HEAD
        } else if (linecount == plain_conv_wts) {
            conv_pol_w = std::move(weights);
        } else if (linecount == plain_conv_wts + 1) {
            conv_pol_b = std::move(weights);
        } else if (linecount == plain_conv_wts + 2) {
            std::copy(cbegin(weights), cend(weights), begin(bn_pol_w1));
        } else if (linecount == plain_conv_wts + 3) {
            process_bn_var(weights);
            std::copy(cbegin(weights), cend(weights), begin(bn_pol_w2));
        } else if (linecount == plain_conv_wts + 4) {
            std::copy(cbegin(weights), cend(weights), begin(ip_pol_w));
        } else if (linecount == plain_conv_wts + 5) {
            std::copy(cbegin(weights), cend(weights), begin(ip_pol_b));
        } else if (linecount == plain_conv_wts + 6) {
            conv_val_w = std::move(weights);
        } else if (linecount == plain_conv_wts + 7) {
            conv_val_b = std::move(weights);
        } else if (linecount == plain_conv_wts + 8) {
            std::copy(cbegin(weights), cend(weights), begin(bn_val_w1));
        } else if (linecount == plain_conv_wts + 9) {
            process_bn_var(weights);
            std::copy(cbegin(weights), cend(weights), begin(bn_val_w2));
        } else if (linecount == plain_conv_wts + 10) {
            std::copy(cbegin(weights), cend(weights), begin(ip1_val_w));
        } else if (linecount == plain_conv_wts + 11) {
            std::copy(cbegin(weights), cend(weights), begin(ip1_val_b));
        } else if (linecount == plain_conv_wts + 12) {
            std::copy(cbegin(weights), cend(weights), begin(ip2_val_w));
        } else if (linecount == plain_conv_wts + 13) {
            std::copy(cbegin(weights), cend(weights), begin(ip2_val_b));
        }
        linecount++;
    }
=======
        }
        linecount++;
    }
    process_bn_var(m_bn_pol_w2);
    process_bn_var(m_bn_val_w2);
>>>>>>> upstream/master

    return {channels, static_cast<int>(residual_blocks)};
}

std::pair<int, int> Network::load_network_file(const std::string& filename) {
    // gzopen supports both gz and non-gz files, will decompress
    // or just read directly as needed.
    auto gzhandle = gzopen(filename.c_str(), "rb");
    if (gzhandle == nullptr) {
        myprintf("Could not open weights file: %s\n", filename.c_str());
        return {0, 0};
    }
    // Stream the gz file in to a memory buffer stream.
    auto buffer = std::stringstream{};
    constexpr auto chunkBufferSize = 64 * 1024;
    std::vector<char> chunkBuffer(chunkBufferSize);
    while (true) {
        auto bytesRead = gzread(gzhandle, chunkBuffer.data(), chunkBufferSize);
        if (bytesRead == 0) break;
        if (bytesRead < 0) {
            myprintf("Failed to decompress or read: %s\n", filename.c_str());
            gzclose(gzhandle);
            return {0, 0};
        }
        assert(bytesRead <= chunkBufferSize);
        buffer.write(chunkBuffer.data(), bytesRead);
    }
    gzclose(gzhandle);

    // Read format version
    auto line = std::string{};
    auto format_version = -1;
    if (std::getline(buffer, line)) {
        auto iss = std::stringstream{line};
        // First line is the file format version id
        iss >> format_version;
        if (iss.fail() || (format_version != 1 && format_version != 2)) {
            myprintf("Weights file is the wrong version.\n");
            return {0, 0};
        } else {
            // Version 2 networks are identical to v1, except
<<<<<<< HEAD
            // that they return the score for black instead of
            // the player to move. This is used by ELF Open Go.
            if (format_version == 2) {
                value_head_not_stm = true;
            } else {
                value_head_not_stm = false;
=======
            // that they return the value for black instead of
            // the player to move. This is used by ELF Open Go.
            if (format_version == 2) {
                m_value_head_not_stm = true;
            } else {
                m_value_head_not_stm = false;
>>>>>>> upstream/master
            }
            return load_v1_network(buffer);
        }
    }
    return {0, 0};
}

<<<<<<< HEAD
void Network::initialize() {
    // Prepare symmetry table
    for (auto s = 0; s < 8; s++) {
        for (auto v = 0; v < BOARD_SQUARES; v++) {
            symmetry_nn_idx_table[s][v] = get_nn_idx_symmetry(v, s);
        }
    }
=======
std::unique_ptr<ForwardPipe>&& Network::init_net(int channels,
    std::unique_ptr<ForwardPipe>&& pipe) {
>>>>>>> upstream/master

    pipe->initialize(channels);
    pipe->push_weights(WINOGRAD_ALPHA, INPUT_CHANNELS, channels, m_fwd_weights);

<<<<<<< HEAD
    // Residual block convolutions
    for (auto i = size_t{0}; i < residual_blocks * 2; i++) {
        conv_weights[weight_index] =
            winograd_transform_f(conv_weights[weight_index],
                                 channels, channels);
        weight_index++;
    }

    // Biases are not calculated and are typically zero but some networks might
    // still have non-zero biases.
    // Move biases to batchnorm means to make the output match without having
    // to separately add the biases.
    for (auto i = size_t{0}; i < conv_biases.size(); i++) {
        for (auto j = size_t{0}; j < batchnorm_means[i].size(); j++) {
            batchnorm_means[i][j] -= conv_biases[i][j];
            conv_biases[i][j] = 0.0f;
        }
    }

    for (auto i = size_t{0}; i < bn_val_w1.size(); i++) {
        bn_val_w1[i] -= conv_val_b[i];
        conv_val_b[i] = 0.0f;
    }

    for (auto i = size_t{0}; i < bn_pol_w1.size(); i++) {
        bn_pol_w1[i] -= conv_pol_b[i];
        conv_pol_b[i] = 0.0f;
    }

#ifdef USE_OPENCL
    myprintf("Initializing OpenCL.\n");
    opencl.initialize(channels);

    for (const auto & opencl_net : opencl.get_networks()) {
        const auto tuners = opencl_net->getOpenCL().get_sgemm_tuners();

        const auto mwg = tuners[0];
        const auto kwg = tuners[2];
        const auto vwm = tuners[3];

        weight_index = 0;

        const auto m_ceil = ceilMultiple(ceilMultiple(channels, mwg), vwm);
        const auto k_ceil = ceilMultiple(ceilMultiple(INPUT_CHANNELS, kwg), vwm);

        const auto Upad = zeropad_U(conv_weights[weight_index],
                                    channels, INPUT_CHANNELS,
                                    m_ceil, k_ceil);

        // Winograd filter transformation changes filter size to 4x4
        opencl_net->push_input_convolution(WINOGRAD_ALPHA, INPUT_CHANNELS,
            channels, Upad,
            batchnorm_means[weight_index], batchnorm_stddivs[weight_index]);
        weight_index++;

        // residual blocks
        for (auto i = size_t{0}; i < residual_blocks; i++) {
            const auto Upad1 = zeropad_U(conv_weights[weight_index],
                                         channels, channels,
                                         m_ceil, m_ceil);
            const auto Upad2 = zeropad_U(conv_weights[weight_index + 1],
                                         channels, channels,
                                         m_ceil, m_ceil);
            opencl_net->push_residual(WINOGRAD_ALPHA, channels, channels,
                                      Upad1,
                                      batchnorm_means[weight_index],
                                      batchnorm_stddivs[weight_index],
                                      Upad2,
                                      batchnorm_means[weight_index + 1],
                                      batchnorm_stddivs[weight_index + 1]);
            weight_index += 2;
        }

        // Output head convolutions
        opencl_net->push_convolve1(channels, OUTPUTS_POLICY, conv_pol_w);
        opencl_net->push_convolve1(channels, OUTPUTS_VALUE, conv_val_w);
=======
    return std::move(pipe);
}

#ifdef USE_HALF
void Network::select_precision(int channels) {
    if (cfg_precision == precision_t::AUTO) {
        auto score_fp16 = float{-1.0};
        auto score_fp32 = float{-1.0};

        myprintf("Initializing OpenCL (autodetecting precision).\n");

        // Setup fp16 here so that we can see if we can skip autodetect.
        // However, if fp16 sanity check fails we will return a fp32 and pray it works.
        auto fp16_net = std::make_unique<OpenCLScheduler<half_float::half>>();
        if (!fp16_net->needs_autodetect()) {
            try {
                myprintf("OpenCL: using fp16/half compute support.\n");
                m_forward = init_net(channels, std::move(fp16_net));
                benchmark_time(1); // a sanity check run
            } catch (...) {
                myprintf("OpenCL: fp16/half failed despite driver claiming support.\n");
                myprintf("Falling back to single precision\n");
                m_forward.reset();
                m_forward = init_net(channels,
                    std::make_unique<OpenCLScheduler<float>>());
            }
            return;
        }

        // Start by setting up fp32.
        try {
            m_forward.reset();
            m_forward = init_net(channels,
                std::make_unique<OpenCLScheduler<float>>());
            score_fp32 = benchmark_time(100);
        } catch (...) {
            // empty - if exception thrown just throw away fp32 net
        }

        // Now benchmark fp16.
        try {
            m_forward.reset();
            m_forward = init_net(channels, std::move(fp16_net));
            score_fp16 = benchmark_time(100);
        } catch (...) {
            // empty - if exception thrown just throw away fp16 net
        }

        if (score_fp16 < 0.0f && score_fp32 < 0.0f) {
            myprintf("Both single precision and half precision failed to run.\n");
            throw std::runtime_error("Failed to initialize net.");
        } else if (score_fp16 < 0.0f) {
            myprintf("Using OpenCL single precision (half precision failed to run).\n");
            m_forward.reset();
            m_forward = init_net(channels,
                std::make_unique<OpenCLScheduler<float>>());
        } else if (score_fp32 < 0.0f) {
            myprintf("Using OpenCL half precision (single precision failed to run).\n");
        } else if (score_fp32 * 1.05f > score_fp16) {
            myprintf("Using OpenCL single precision (less than 5%% slower than half).\n");
            m_forward.reset();
            m_forward = init_net(channels,
                std::make_unique<OpenCLScheduler<float>>());
        } else {
            myprintf("Using OpenCL half precision (at least 5%% faster than single).\n");
        }
        return;
    } else if (cfg_precision == precision_t::SINGLE) {
        myprintf("Initializing OpenCL (single precision).\n");
        m_forward = init_net(channels,
            std::make_unique<OpenCLScheduler<float>>());
        return;
    } else if (cfg_precision == precision_t::HALF) {
        myprintf("Initializing OpenCL (half precision).\n");
        m_forward = init_net(channels,
            std::make_unique<OpenCLScheduler<half_float::half>>());
        return;
>>>>>>> upstream/master
    }
}
#endif

void Network::initialize(int playouts, const std::string & weightsfile) {
#ifdef USE_BLAS
#ifndef __APPLE__
#ifdef USE_OPENBLAS
    openblas_set_num_threads(1);
    myprintf("BLAS Core: %s\n", openblas_get_corename());
#endif
#ifdef USE_MKL
    //mkl_set_threading_layer(MKL_THREADING_SEQUENTIAL);
    mkl_set_num_threads(1);
    MKLVersion Version;
    mkl_get_version(&Version);
    myprintf("BLAS core: MKL %s\n", Version.Processor);
#endif
#endif
#else
    myprintf("BLAS Core: built-in Eigen %d.%d.%d library.\n",
             EIGEN_WORLD_VERSION, EIGEN_MAJOR_VERSION, EIGEN_MINOR_VERSION);
#endif
<<<<<<< HEAD
}

#ifdef USE_BLAS
void Network::winograd_transform_in(const std::vector<float>& in,
                                    std::vector<float>& V,
                                    const int C) {
    constexpr auto W = BOARD_SIZE;
    constexpr auto H = BOARD_SIZE;
    constexpr auto WTILES = (W + 1) / 2;
    constexpr auto P = WTILES * WTILES;

    std::array<std::array<float, WTILES * 2 + 2>, WTILES * 2 + 2> in_pad;
    for (auto xin = size_t{0}; xin < in_pad.size(); xin++) {
        in_pad[0][xin]     = 0.0f;
        in_pad[H + 1][xin] = 0.0f;
        in_pad[H + 2][xin] = 0.0f;
    }
    for (auto yin = size_t{1}; yin < in_pad[0].size() - 2; yin++) {
        in_pad[yin][0]     = 0.0f;
        in_pad[yin][W + 1] = 0.0f;
        in_pad[yin][W + 2] = 0.0f;
    }

    for (auto ch = 0; ch < C; ch++) {
        for (auto yin = 0; yin < H; yin++) {
            for (auto xin = 0; xin < W; xin++) {
                in_pad[yin + 1][xin + 1] = in[ch*(W*H) + yin*W + xin];
            }
        }
        for (auto block_y = 0; block_y < WTILES; block_y++) {
            // Tiles overlap by 2
            const auto yin = 2 * block_y;
            for (auto block_x = 0; block_x < WTILES; block_x++) {
                const auto xin = 2 * block_x;

                // Calculates transpose(B).x.B
                // B = [[ 1.0,  0.0,  0.0,  0.0],
                //      [ 0.0,  1.0, -1.0,  1.0],
                //      [-1.0,  1.0,  1.0,  0.0],
                //      [ 0.0,  0.0,  0.0, -1.0]]

                using WinogradTile =
                    std::array<std::array<float, WINOGRAD_ALPHA>, WINOGRAD_ALPHA>;
                WinogradTile T1, T2;

                T1[0][0] = in_pad[yin + 0][xin + 0] - in_pad[yin + 2][xin + 0];
                T1[0][1] = in_pad[yin + 0][xin + 1] - in_pad[yin + 2][xin + 1];
                T1[0][2] = in_pad[yin + 0][xin + 2] - in_pad[yin + 2][xin + 2];
                T1[0][3] = in_pad[yin + 0][xin + 3] - in_pad[yin + 2][xin + 3];
                T1[1][0] = in_pad[yin + 1][xin + 0] + in_pad[yin + 2][xin + 0];
                T1[1][1] = in_pad[yin + 1][xin + 1] + in_pad[yin + 2][xin + 1];
                T1[1][2] = in_pad[yin + 1][xin + 2] + in_pad[yin + 2][xin + 2];
                T1[1][3] = in_pad[yin + 1][xin + 3] + in_pad[yin + 2][xin + 3];
                T1[2][0] = in_pad[yin + 2][xin + 0] - in_pad[yin + 1][xin + 0];
                T1[2][1] = in_pad[yin + 2][xin + 1] - in_pad[yin + 1][xin + 1];
                T1[2][2] = in_pad[yin + 2][xin + 2] - in_pad[yin + 1][xin + 2];
                T1[2][3] = in_pad[yin + 2][xin + 3] - in_pad[yin + 1][xin + 3];
                T1[3][0] = in_pad[yin + 1][xin + 0] - in_pad[yin + 3][xin + 0];
                T1[3][1] = in_pad[yin + 1][xin + 1] - in_pad[yin + 3][xin + 1];
                T1[3][2] = in_pad[yin + 1][xin + 2] - in_pad[yin + 3][xin + 2];
                T1[3][3] = in_pad[yin + 1][xin + 3] - in_pad[yin + 3][xin + 3];

                T2[0][0] = T1[0][0] - T1[0][2];
                T2[0][1] = T1[0][1] + T1[0][2];
                T2[0][2] = T1[0][2] - T1[0][1];
                T2[0][3] = T1[0][1] - T1[0][3];
                T2[1][0] = T1[1][0] - T1[1][2];
                T2[1][1] = T1[1][1] + T1[1][2];
                T2[1][2] = T1[1][2] - T1[1][1];
                T2[1][3] = T1[1][1] - T1[1][3];
                T2[2][0] = T1[2][0] - T1[2][2];
                T2[2][1] = T1[2][1] + T1[2][2];
                T2[2][2] = T1[2][2] - T1[2][1];
                T2[2][3] = T1[2][1] - T1[2][3];
                T2[3][0] = T1[3][0] - T1[3][2];
                T2[3][1] = T1[3][1] + T1[3][2];
                T2[3][2] = T1[3][2] - T1[3][1];
                T2[3][3] = T1[3][1] - T1[3][3];

                const auto offset = ch * P + block_y * WTILES + block_x;
                for (auto i = 0; i < WINOGRAD_ALPHA; i++) {
                    for (auto j = 0; j < WINOGRAD_ALPHA; j++) {
                        V[(i*WINOGRAD_ALPHA + j)*C*P + offset] = T2[i][j];
                    }
                }
            }
=======

    m_fwd_weights = std::make_shared<ForwardPipeWeights>();

    // Make a guess at a good size as long as the user doesn't
    // explicitly set a maximum memory usage.
    m_nncache.set_size_from_playouts(playouts);

    // Prepare symmetry table
    for (auto s = 0; s < NUM_SYMMETRIES; ++s) {
        for (auto v = 0; v < NUM_INTERSECTIONS; ++v) {
            const auto newvtx =
                get_symmetry({v % BOARD_SIZE, v / BOARD_SIZE}, s);
            symmetry_nn_idx_table[s][v] =
                (newvtx.second * BOARD_SIZE) + newvtx.first;
            assert(symmetry_nn_idx_table[s][v] >= 0
                   && symmetry_nn_idx_table[s][v] < NUM_INTERSECTIONS);
>>>>>>> upstream/master
        }
    }

<<<<<<< HEAD
void Network::winograd_sgemm(const std::vector<float>& U,
                             const std::vector<float>& V,
                             std::vector<float>& M,
                             const int C, const int K) {
    constexpr auto P = (BOARD_SIZE + 1) * (BOARD_SIZE + 1) / WINOGRAD_ALPHA;

    for (auto b = 0; b < WINOGRAD_TILE; b++) {
        const auto offset_u = b * K * C;
        const auto offset_v = b * C * P;
        const auto offset_m = b * K * P;

        cblas_sgemm(CblasRowMajor, CblasTrans, CblasNoTrans,
                    K, P, C,
                    1.0f,
                    &U[offset_u], K,
                    &V[offset_v], P,
                    0.0f,
                    &M[offset_m], P);
    }
}

void Network::winograd_transform_out(const std::vector<float>& M,
                                     std::vector<float>& Y,
                                     const int K) {
    constexpr auto W = BOARD_SIZE;
    constexpr auto H = BOARD_SIZE;
    constexpr auto WTILES = (W + 1) / 2;
    constexpr auto P = WTILES * WTILES;

    for (auto k = 0; k < K; k++) {
        const auto kHW = k * W * H;
        for (auto block_x = 0; block_x < WTILES; block_x++) {
            const auto x = 2 * block_x;
            for (auto block_y = 0; block_y < WTILES; block_y++) {
                const auto y = 2 * block_y;

                const auto b = block_y * WTILES + block_x;
                using WinogradTile =
                    std::array<std::array<float, WINOGRAD_ALPHA>, WINOGRAD_ALPHA>;
                WinogradTile temp_m;
                for (auto xi = 0; xi < WINOGRAD_ALPHA; xi++) {
                    for (auto nu = 0; nu < WINOGRAD_ALPHA; nu++) {
                        temp_m[xi][nu] =
                            M[xi*(WINOGRAD_ALPHA*K*P) + nu*(K*P)+ k*P + b];
                    }
                }

                // Calculates transpose(A).temp_m.A
                //    A = [1.0,  0.0],
                //        [1.0,  1.0],
                //        [1.0, -1.0],
                //        [0.0, -1.0]]

                const std::array<std::array<float, 2>, 2> o = {
                    temp_m[0][0] + temp_m[0][1] + temp_m[0][2] +
                    temp_m[1][0] + temp_m[1][1] + temp_m[1][2] +
                    temp_m[2][0] + temp_m[2][1] + temp_m[2][2],
                    temp_m[0][1] - temp_m[0][2] - temp_m[0][3] +
                    temp_m[1][1] - temp_m[1][2] - temp_m[1][3] +
                    temp_m[2][1] - temp_m[2][2] - temp_m[2][3],
                    temp_m[1][0] + temp_m[1][1] + temp_m[1][2] -
                    temp_m[2][0] - temp_m[2][1] - temp_m[2][2] -
                    temp_m[3][0] - temp_m[3][1] - temp_m[3][2],
                    temp_m[1][1] - temp_m[1][2] - temp_m[1][3] -
                    temp_m[2][1] + temp_m[2][2] + temp_m[2][3] -
                    temp_m[3][1] + temp_m[3][2] + temp_m[3][3]
                };

                const auto y_ind = kHW + (y)*W + (x);
                Y[y_ind] = o[0][0];
                if (x + 1 < W) {
                    Y[y_ind + 1] = o[0][1];
                }
                if (y + 1 < H) {
                    Y[y_ind + W] = o[1][0];
                    if (x + 1 < W) {
                        Y[y_ind + W + 1] = o[1][1];
                    }
                }
            }
=======
    // Load network from file
    size_t channels, residual_blocks;
    std::tie(channels, residual_blocks) = load_network_file(weightsfile);
    if (channels == 0) {
        exit(EXIT_FAILURE);
    }

    auto weight_index = size_t{0};
    // Input convolution
    // Winograd transform convolution weights
    m_fwd_weights->m_conv_weights[weight_index] =
        winograd_transform_f(m_fwd_weights->m_conv_weights[weight_index],
                             channels, INPUT_CHANNELS);
    weight_index++;

    // Residual block convolutions
    for (auto i = size_t{0}; i < residual_blocks * 2; i++) {
        m_fwd_weights->m_conv_weights[weight_index] =
            winograd_transform_f(m_fwd_weights->m_conv_weights[weight_index],
                                 channels, channels);
        weight_index++;
    }

    // Biases are not calculated and are typically zero but some networks might
    // still have non-zero biases.
    // Move biases to batchnorm means to make the output match without having
    // to separately add the biases.
    auto bias_size = m_fwd_weights->m_conv_biases.size();
    for (auto i = size_t{0}; i < bias_size; i++) {
        auto means_size = m_fwd_weights->m_batchnorm_means[i].size();
        for (auto j = size_t{0}; j < means_size; j++) {
            m_fwd_weights->m_batchnorm_means[i][j] -= m_fwd_weights->m_conv_biases[i][j];
            m_fwd_weights->m_conv_biases[i][j] = 0.0f;
>>>>>>> upstream/master
        }
    }

    for (auto i = size_t{0}; i < m_bn_val_w1.size(); i++) {
        m_bn_val_w1[i] -= m_fwd_weights->m_conv_val_b[i];
        m_fwd_weights->m_conv_val_b[i] = 0.0f;
    }

    for (auto i = size_t{0}; i < m_bn_pol_w1.size(); i++) {
        m_bn_pol_w1[i] -= m_fwd_weights->m_conv_pol_b[i];
        m_fwd_weights->m_conv_pol_b[i] = 0.0f;
    }

#ifdef USE_OPENCL
    if (cfg_cpu_only) {
        myprintf("Initializing CPU-only evaluation.\n");
        m_forward = init_net(channels, std::make_unique<CPUPipe>());
    } else {
#ifdef USE_OPENCL_SELFCHECK
        // initialize CPU reference first, so that we can self-check
        // when doing fp16 vs. fp32 detections
        m_forward_cpu = init_net(channels, std::make_unique<CPUPipe>());
#endif
#ifdef USE_HALF
        // HALF support is enabled, and we are using the GPU.
        // Select the precision to use at runtime.
        select_precision(channels);
#else
        myprintf("Initializing OpenCL (single precision).\n");
        m_forward = init_net(channels,
                             std::make_unique<OpenCLScheduler<float>>());
#endif
    }

<<<<<<< HEAD
template<unsigned int filter_size>
void convolve(const size_t outputs,
              const std::vector<float>& input,
              const std::vector<float>& weights,
              const std::vector<float>& biases,
              std::vector<float>& output) {
    // The size of the board is defined at compile time
    constexpr unsigned int width = BOARD_SIZE;
    constexpr unsigned int height = BOARD_SIZE;
    constexpr auto board_squares = width * height;
    constexpr auto filter_len = filter_size * filter_size;
    const auto input_channels = weights.size() / (biases.size() * filter_len);
    const auto filter_dim = filter_len * input_channels;
    assert(outputs * board_squares == output.size());

    std::vector<float> col(filter_dim * width * height);
    im2col<filter_size>(input_channels, input, col);

    // Weight shape (output, input, filter_size, filter_size)
    // 96 18 3 3
    // C←αAB + βC
    // outputs[96,19x19] = weights[96,18x3x3] x col[18x3x3,19x19]
    // M Number of rows in matrices A and C.
    // N Number of columns in matrices B and C.
    // K Number of columns in matrix A; number of rows in matrix B.
    // lda The size of the first dimention of matrix A; if you are
    // passing a matrix A[m][n], the value should be m.
    //    cblas_sgemm(CblasRowMajor, TransA, TransB, M, N, K, alpha, A, lda, B,
    //                ldb, beta, C, N);

    cblas_sgemm(CblasRowMajor, CblasNoTrans, CblasNoTrans,
                // M        N            K
                outputs, board_squares, filter_dim,
                1.0f, &weights[0], filter_dim,
                &col[0], board_squares,
                0.0f, &output[0], board_squares);

    for (unsigned int o = 0; o < outputs; o++) {
        for (unsigned int b = 0; b < board_squares; b++) {
            output[(o * board_squares) + b] += biases[o];
        }
    }
=======
#else //!USE_OPENCL
    myprintf("Initializing CPU-only evaluation.\n");
    m_forward = init_net(channels, std::make_unique<CPUPipe>());
#endif

    // Need to estimate size before clearing up the pipe.
    get_estimated_size();
    m_fwd_weights.reset();
>>>>>>> upstream/master
}

template<unsigned int inputs,
         unsigned int outputs,
         bool ReLU,
         size_t W>
std::vector<float> innerproduct(const std::vector<float>& input,
                                const std::array<float, W>& weights,
                                const std::array<float, outputs>& biases) {
    std::vector<float> output(outputs);

#ifdef USE_BLAS
    cblas_sgemv(CblasRowMajor, CblasNoTrans,
                // M     K
                outputs, inputs,
                1.0f, &weights[0], inputs,
                &input[0], 1,
                0.0f, &output[0], 1);
<<<<<<< HEAD

=======
#else
    EigenVectorMap<float> y(output.data(), outputs);
    y.noalias() =
        ConstEigenMatrixMap<float>(weights.data(),
                                   inputs,
                                   outputs).transpose()
        * ConstEigenVectorMap<float>(input.data(), inputs);
#endif
>>>>>>> upstream/master
    const auto lambda_ReLU = [](const auto val) { return (val > 0.0f) ?
                                                          val : 0.0f; };
    for (unsigned int o = 0; o < outputs; o++) {
        auto val = biases[o] + output[o];
        if (ReLU) {
            val = lambda_ReLU(val);
        }
        output[o] = val;
    }

    return output;
}

template <size_t spatial_size>
void batchnorm(const size_t channels,
               std::vector<float>& data,
               const float* const means,
               const float* const stddivs,
<<<<<<< HEAD
               const float* const eltwise = nullptr)
{
=======
               const float* const eltwise = nullptr) {
>>>>>>> upstream/master
    const auto lambda_ReLU = [](const auto val) { return (val > 0.0f) ?
                                                          val : 0.0f; };
    for (auto c = size_t{0}; c < channels; ++c) {
        const auto mean = means[c];
        const auto scale_stddiv = stddivs[c];
<<<<<<< HEAD

        if (eltwise == nullptr) {
            // Classical BN
            const auto arr = &data[c * spatial_size];
=======
        const auto arr = &data[c * spatial_size];

        if (eltwise == nullptr) {
            // Classical BN
>>>>>>> upstream/master
            for (auto b = size_t{0}; b < spatial_size; b++) {
                arr[b] = lambda_ReLU(scale_stddiv * (arr[b] - mean));
            }
        } else {
            // BN + residual add
<<<<<<< HEAD
            const auto arr = &data[c * spatial_size];
=======
>>>>>>> upstream/master
            const auto res = &eltwise[c * spatial_size];
            for (auto b = size_t{0}; b < spatial_size; b++) {
                arr[b] = lambda_ReLU((scale_stddiv * (arr[b] - mean)) + res[b]);
            }
        }
    }
}

<<<<<<< HEAD
void Network::forward_cpu(const std::vector<float>& input,
                          std::vector<float>& output_pol,
                          std::vector<float>& output_val) {
    // Input convolution
    constexpr auto width = BOARD_SIZE;
    constexpr auto height = BOARD_SIZE;
    constexpr auto tiles = (width + 1) * (height + 1) / 4;
    // Calculate output channels
    const auto output_channels = conv_biases[0].size();
    // input_channels is the maximum number of input channels of any
    // convolution. Residual blocks are identical, but the first convolution
    // might be bigger when the network has very few filters
    const auto input_channels = std::max(static_cast<size_t>(output_channels),
                                         static_cast<size_t>(INPUT_CHANNELS));
    auto conv_out = std::vector<float>(output_channels * width * height);

    auto V = std::vector<float>(WINOGRAD_TILE * input_channels * tiles);
    auto M = std::vector<float>(WINOGRAD_TILE * output_channels * tiles);

    winograd_convolve3(output_channels, input, conv_weights[0], V, M, conv_out);
    batchnorm<BOARD_SQUARES>(output_channels, conv_out,
                             batchnorm_means[0].data(),
                             batchnorm_stddivs[0].data());

    // Residual tower
    auto conv_in = std::vector<float>(output_channels * width * height);
    auto res = std::vector<float>(output_channels * width * height);
    for (auto i = size_t{1}; i < conv_weights.size(); i += 2) {
        auto output_channels = conv_biases[i].size();
        std::swap(conv_out, conv_in);
        winograd_convolve3(output_channels, conv_in,
                           conv_weights[i], V, M, conv_out);
        batchnorm<BOARD_SQUARES>(output_channels, conv_out,
                                 batchnorm_means[i].data(),
                                 batchnorm_stddivs[i].data());

        output_channels = conv_biases[i + 1].size();
        std::swap(conv_in, res);
        std::swap(conv_out, conv_in);
        winograd_convolve3(output_channels, conv_in,
                           conv_weights[i + 1], V, M, conv_out);
        batchnorm<BOARD_SQUARES>(output_channels, conv_out,
                                 batchnorm_means[i + 1].data(),
                                 batchnorm_stddivs[i + 1].data(),
                                 res.data());
    }
    convolve<1>(OUTPUTS_POLICY, conv_out, conv_pol_w, conv_pol_b, output_pol);
    convolve<1>(OUTPUTS_VALUE, conv_out, conv_val_w, conv_val_b, output_val);
}

template<typename T>
T relative_difference(const T a, const T b) {
    // Handle NaN
    if (std::isnan(a) || std::isnan(b)) {
        return std::numeric_limits<T>::max();
    }

    constexpr auto small_number = 1e-3f;
    auto fa = std::fabs(a);
    auto fb = std::fabs(b);

    if (fa > small_number && fb > small_number) {
        // Handle sign difference
        if ((a < 0) != (b < 0)) {
            return std::numeric_limits<T>::max();
        }
    } else {
        // Handle underflow
        fa = std::max(fa, small_number);
        fb = std::max(fb, small_number);
    }

    return fabs(fa - fb) / std::min(fa, fb);
}

void compare_net_outputs(std::vector<float>& data,
                         std::vector<float>& ref) {
    // We accept an error up to 5%, but output values
    // smaller than 1/1000th are "rounded up" for the comparison.
    constexpr auto relative_error = 5e-2f;
    for (auto idx = size_t{0}; idx < data.size(); ++idx) {
        const auto err = relative_difference(data[idx], ref[idx]);
        if (err > relative_error) {
            printf("Error in OpenCL calculation: expected %f got %f "
                   "(error=%f%%)\n", ref[idx], data[idx], err * 100.0);
            printf("Update your GPU drivers or reduce the amount of games "
                   "played simultaneously.\n");
            throw std::runtime_error("OpenCL self-check mismatch.");
        }
=======
#ifdef USE_OPENCL_SELFCHECK
void Network::compare_net_outputs(const Netresult& data,
                                  const Netresult& ref) {
    // Calculates L2-norm between data and ref.
    constexpr auto max_error = 0.2f;

    auto error = 0.0f;

    for (auto idx = size_t{0}; idx < data.policy.size(); ++idx) {
        const auto diff = data.policy[idx] - ref.policy[idx];
        error += diff * diff;
    }
    const auto diff_pass = data.policy_pass - ref.policy_pass;
    const auto diff_winrate = data.winrate - ref.winrate;
    error += diff_pass * diff_pass;
    error += diff_winrate * diff_winrate;

    error = std::sqrt(error);

    if (error > max_error || std::isnan(error)) {
        printf("Error in OpenCL calculation: Update your GPU drivers "
               "or reduce the amount of games played simultaneously.\n");
        throw std::runtime_error("OpenCL self-check mismatch.");
>>>>>>> upstream/master
    }
}
#endif

std::vector<float> softmax(const std::vector<float>& input,
                           const float temperature = 1.0f) {
    auto output = std::vector<float>{};
    output.reserve(input.size());

    const auto alpha = *std::max_element(cbegin(input), cend(input));
    auto denom = 0.0f;

    for (const auto in_val : input) {
        auto val = std::exp((in_val - alpha) / temperature);
        denom += val;
        output.push_back(val);
    }

    for (auto& out : output) {
        out /= denom;
    }

    return output;
}

<<<<<<< HEAD
Network::Netresult Network::get_scored_moves(
    const GameState* const state, const Ensemble ensemble,
    const int symmetry, const bool skip_cache) {
=======
bool Network::probe_cache(const GameState* const state,
                          Network::Netresult& result) {
    if (m_nncache.lookup(state->board.get_hash(), result)) {
        return true;
    }
    // If we are not generating a self-play game, try to find
    // symmetries if we are in the early opening.
    if (!cfg_noise && !cfg_random_cnt
        && state->get_movenum()
           < (state->get_timecontrol().opening_moves(BOARD_SIZE) / 2)) {
        for (auto sym = 0; sym < Network::NUM_SYMMETRIES; ++sym) {
            if (sym == Network::IDENTITY_SYMMETRY) {
                continue;
            }
            const auto hash = state->get_symmetry_hash(sym);
            if (m_nncache.lookup(hash, result)) {
                decltype(result.policy) corrected_policy;
                for (auto idx = size_t{0}; idx < NUM_INTERSECTIONS; ++idx) {
                    const auto sym_idx = symmetry_nn_idx_table[sym][idx];
                    corrected_policy[idx] = result.policy[sym_idx];
                }
                result.policy = std::move(corrected_policy);
                return true;
            }
        }
    }

    return false;
}

Network::Netresult Network::get_output(
    const GameState* const state, const Ensemble ensemble,
    const int symmetry, const bool skip_cache, const bool force_selfcheck) {
>>>>>>> upstream/master
    Netresult result;
    if (state->board.get_boardsize() != BOARD_SIZE) {
        return result;
    }

    if (!skip_cache) {
        // See if we already have this in the cache.
<<<<<<< HEAD
        if (NNCache::get_NNCache().lookup(state->board.get_hash(), result)) {
=======
        if (probe_cache(state, result)) {
>>>>>>> upstream/master
            return result;
        }
    }

    if (ensemble == DIRECT) {
<<<<<<< HEAD
        assert(symmetry >= 0 && symmetry <= 7);
        result = get_scored_moves_internal(state, symmetry);
    } else if (ensemble == AVERAGE) {
        for (auto sym = 0; sym < 8; ++sym) {
            auto tmpresult = get_scored_moves_internal(state, sym);
            result.winrate += tmpresult.winrate / 8.0f;
            result.policy_pass += tmpresult.policy_pass / 8.0f;

            for (auto idx = size_t{0}; idx < BOARD_SQUARES; idx++) {
                result.policy[idx] += tmpresult.policy[idx] / 8.0f;
=======
        assert(symmetry >= 0 && symmetry < NUM_SYMMETRIES);
        result = get_output_internal(state, symmetry);
    } else if (ensemble == AVERAGE) {
        for (auto sym = 0; sym < NUM_SYMMETRIES; ++sym) {
            auto tmpresult = get_output_internal(state, sym);
            result.winrate +=
                tmpresult.winrate / static_cast<float>(NUM_SYMMETRIES);
            result.policy_pass +=
                tmpresult.policy_pass / static_cast<float>(NUM_SYMMETRIES);

            for (auto idx = size_t{0}; idx < NUM_INTERSECTIONS; idx++) {
                result.policy[idx] +=
                    tmpresult.policy[idx] / static_cast<float>(NUM_SYMMETRIES);
>>>>>>> upstream/master
            }
        }
    } else {
        assert(ensemble == RANDOM_SYMMETRY);
        assert(symmetry == -1);
<<<<<<< HEAD
        const auto rand_sym = Random::get_Rng().randfix<8>();
        result = get_scored_moves_internal(state, rand_sym);
    }

    // v2 format (ELF Open Go) returns black value, not stm
    if (value_head_not_stm) {
=======
        const auto rand_sym = Random::get_Rng().randfix<NUM_SYMMETRIES>();
        result = get_output_internal(state, rand_sym);
#ifdef USE_OPENCL_SELFCHECK
        // Both implementations are available, self-check the OpenCL driver by
        // running both with a probability of 1/2000.
        // selfcheck is done here because this is the only place NN
        // evaluation is done on actual gameplay.
        if (m_forward_cpu != nullptr
            && (force_selfcheck || Random::get_Rng().randfix<SELFCHECK_PROBABILITY>() == 0)
        ) {
            auto result_ref = get_output_internal(state, rand_sym, true);
            compare_net_outputs(result, result_ref);
        }
#else
        (void)force_selfcheck;
#endif
    }

    // v2 format (ELF Open Go) returns black value, not stm
    if (m_value_head_not_stm) {
>>>>>>> upstream/master
        if (state->board.get_to_move() == FastBoard::WHITE) {
            result.winrate = 1.0f - result.winrate;
        }
    }

    // Insert result into cache.
<<<<<<< HEAD
    NNCache::get_NNCache().insert(state->board.get_hash(), result);
=======
    m_nncache.insert(state->board.get_hash(), result);
>>>>>>> upstream/master

    return result;
}

<<<<<<< HEAD
Network::Netresult Network::get_scored_moves_internal(
    const GameState* const state, const int symmetry) {
    assert(symmetry >= 0 && symmetry <= 7);
=======
Network::Netresult Network::get_output_internal(
    const GameState* const state, const int symmetry, bool selfcheck) {
    assert(symmetry >= 0 && symmetry < NUM_SYMMETRIES);
>>>>>>> upstream/master
    constexpr auto width = BOARD_SIZE;
    constexpr auto height = BOARD_SIZE;

    const auto input_data = gather_features(state, symmetry);
    std::vector<float> policy_data(OUTPUTS_POLICY * width * height);
    std::vector<float> value_data(OUTPUTS_VALUE * width * height);
<<<<<<< HEAD
#ifdef USE_HALF
    std::vector<net_t> policy_data_n(OUTPUTS_POLICY * width * height);
    std::vector<net_t> value_data_n(OUTPUTS_VALUE * width * height);
#endif
#ifdef USE_OPENCL
#ifdef USE_HALF
    opencl.forward(input_data, policy_data_n, value_data_n);
    std::copy(begin(policy_data_n), end(policy_data_n), begin(policy_data));
    std::copy(begin(value_data_n), end(value_data_n), begin(value_data));
#else
    opencl.forward(input_data, policy_data, value_data);
#endif
#elif defined(USE_BLAS) && !defined(USE_OPENCL)
    forward_cpu(input_data, policy_data, value_data);
#endif
#ifdef USE_OPENCL_SELFCHECK
    // Both implementations are available, self-check the OpenCL driver by
    // running both with a probability of 1/2000.
    if (Random::get_Rng().randfix<SELFCHECK_PROBABILITY>() == 0) {
        auto cpu_policy_data = std::vector<float>(policy_data.size());
        auto cpu_value_data = std::vector<float>(value_data.size());
        forward_cpu(input_data, cpu_policy_data, cpu_value_data);
        compare_net_outputs(policy_data, cpu_policy_data);
        compare_net_outputs(value_data, cpu_value_data);
=======
#ifdef USE_OPENCL_SELFCHECK
    if (selfcheck) {
        m_forward_cpu->forward(input_data, policy_data, value_data);
    } else {
        m_forward->forward(input_data, policy_data, value_data);
>>>>>>> upstream/master
    }
#else
    m_forward->forward(input_data, policy_data, value_data);
    (void) selfcheck;
#endif

    // Get the moves
<<<<<<< HEAD
    batchnorm<BOARD_SQUARES>(OUTPUTS_POLICY, policy_data,
        bn_pol_w1.data(), bn_pol_w2.data());
    const auto policy_out =
        innerproduct<OUTPUTS_POLICY * BOARD_SQUARES, BOARD_SQUARES + 1, false>(
            policy_data, ip_pol_w, ip_pol_b);
    const auto outputs = softmax(policy_out, cfg_softmax_temp);

    // Now get the score
    batchnorm<BOARD_SQUARES>(OUTPUTS_VALUE, value_data,
        bn_val_w1.data(), bn_val_w2.data());
    const auto winrate_data =
        innerproduct<BOARD_SQUARES, 256, true>(value_data, ip1_val_w, ip1_val_b);
    const auto winrate_out =
        innerproduct<256, 1, false>(winrate_data, ip2_val_w, ip2_val_b);

    // Sigmoid
    const auto winrate_sig = (1.0f + std::tanh(winrate_out[0])) / 2.0f;

    Netresult result;

    for (auto idx = size_t{0}; idx < BOARD_SQUARES; idx++) {
=======
    batchnorm<NUM_INTERSECTIONS>(OUTPUTS_POLICY, policy_data,
        m_bn_pol_w1.data(), m_bn_pol_w2.data());
    const auto policy_out =
        innerproduct<OUTPUTS_POLICY * NUM_INTERSECTIONS, POTENTIAL_MOVES, false>(
            policy_data, m_ip_pol_w, m_ip_pol_b);
    const auto outputs = softmax(policy_out, cfg_softmax_temp);

    // Now get the value
    batchnorm<NUM_INTERSECTIONS>(OUTPUTS_VALUE, value_data,
        m_bn_val_w1.data(), m_bn_val_w2.data());
    const auto winrate_data =
        innerproduct<OUTPUTS_VALUE * NUM_INTERSECTIONS, VALUE_LAYER, true>(
            value_data, m_ip1_val_w, m_ip1_val_b);
    const auto winrate_out =
        innerproduct<VALUE_LAYER, 1, false>(winrate_data, m_ip2_val_w, m_ip2_val_b);

    // Map TanH output range [-1..1] to [0..1] range
    const auto winrate = (1.0f + std::tanh(winrate_out[0])) / 2.0f;

    Netresult result;

    for (auto idx = size_t{0}; idx < NUM_INTERSECTIONS; idx++) {
>>>>>>> upstream/master
        const auto sym_idx = symmetry_nn_idx_table[symmetry][idx];
        result.policy[sym_idx] = outputs[idx];
    }

<<<<<<< HEAD
    result.policy_pass = outputs[BOARD_SQUARES];
    result.winrate = winrate_sig;
=======
    result.policy_pass = outputs[NUM_INTERSECTIONS];
    result.winrate = winrate;
>>>>>>> upstream/master

    return result;
}

void Network::show_heatmap(const FastState* const state,
                           const Netresult& result,
                           const bool topmoves) {
    std::vector<std::string> display_map;
    std::string line;

    for (unsigned int y = 0; y < BOARD_SIZE; y++) {
        for (unsigned int x = 0; x < BOARD_SIZE; x++) {
<<<<<<< HEAD
            auto score = 0;
            const auto vertex = state->board.get_vertex(x, y);
            if (state->board.get_square(vertex) == FastBoard::EMPTY) {
                score = result.policy[y * BOARD_SIZE + x] * 1000;
            }

            line += boost::str(boost::format("%3d ") % score);
=======
            auto policy = 0;
            const auto vertex = state->board.get_vertex(x, y);
            if (state->board.get_state(vertex) == FastBoard::EMPTY) {
                policy = result.policy[y * BOARD_SIZE + x] * 1000;
            }

            line += boost::str(boost::format("%3d ") % policy);
>>>>>>> upstream/master
        }

        display_map.push_back(line);
        line.clear();
    }

    for (int i = display_map.size() - 1; i >= 0; --i) {
        myprintf("%s\n", display_map[i].c_str());
    }
<<<<<<< HEAD
    const auto pass_score = int(result.policy_pass * 1000);
    myprintf("pass: %d\n", pass_score);
    myprintf("winrate: %f\n", result.winrate);

    if (topmoves) {
        std::vector<Network::ScoreVertexPair> moves;
        for (auto i=0; i < BOARD_SQUARES; i++) {
            const auto x = i % BOARD_SIZE;
            const auto y = i / BOARD_SIZE;
            const auto vertex = state->board.get_vertex(x, y);
            if (state->board.get_square(vertex) == FastBoard::EMPTY) {
=======
    const auto pass_policy = int(result.policy_pass * 1000);
    myprintf("pass: %d\n", pass_policy);
    myprintf("winrate: %f\n", result.winrate);

    if (topmoves) {
        std::vector<Network::PolicyVertexPair> moves;
        for (auto i=0; i < NUM_INTERSECTIONS; i++) {
            const auto x = i % BOARD_SIZE;
            const auto y = i / BOARD_SIZE;
            const auto vertex = state->board.get_vertex(x, y);
            if (state->board.get_state(vertex) == FastBoard::EMPTY) {
>>>>>>> upstream/master
                moves.emplace_back(result.policy[i], vertex);
            }
        }
        moves.emplace_back(result.policy_pass, FastBoard::PASS);

        std::stable_sort(rbegin(moves), rend(moves));

        auto cum = 0.0f;
        size_t tried = 0;
        while (cum < 0.85f && tried < moves.size()) {
            if (moves[tried].first < 0.01f) break;
            myprintf("%1.3f (%s)\n",
                    moves[tried].first,
                    state->board.move_to_text(moves[tried].second).c_str());
            cum += moves[tried].first;
            tried++;
        }
    }
}

void Network::fill_input_plane_pair(const FullBoard& board,
<<<<<<< HEAD
                                    std::vector<net_t>::iterator black,
                                    std::vector<net_t>::iterator white,
                                    const int symmetry) {
    for (auto idx = 0; idx < BOARD_SQUARES; idx++) {
        const auto sym_idx = symmetry_nn_idx_table[symmetry][idx];
        const auto x = sym_idx % BOARD_SIZE;
        const auto y = sym_idx / BOARD_SIZE;
        const auto color = board.get_square(x, y);
        if (color == FastBoard::BLACK) {
            black[idx] = net_t(true);
        } else if (color == FastBoard::WHITE) {
            white[idx] = net_t(true);
=======
                                    std::vector<float>::iterator black,
                                    std::vector<float>::iterator white,
                                    const int symmetry) {
    for (auto idx = 0; idx < NUM_INTERSECTIONS; idx++) {
        const auto sym_idx = symmetry_nn_idx_table[symmetry][idx];
        const auto x = sym_idx % BOARD_SIZE;
        const auto y = sym_idx / BOARD_SIZE;
        const auto color = board.get_state(x, y);
        if (color == FastBoard::BLACK) {
            black[idx] = float(true);
        } else if (color == FastBoard::WHITE) {
            white[idx] = float(true);
>>>>>>> upstream/master
        }
    }
}

<<<<<<< HEAD
std::vector<net_t> Network::gather_features(const GameState* const state,
                                            const int symmetry) {
    assert(symmetry >= 0 && symmetry <= 7);
    auto input_data = std::vector<net_t>(INPUT_CHANNELS * BOARD_SQUARES);
=======
std::vector<float> Network::gather_features(const GameState* const state,
                                            const int symmetry) {
    assert(symmetry >= 0 && symmetry < NUM_SYMMETRIES);
    auto input_data = std::vector<float>(INPUT_CHANNELS * NUM_INTERSECTIONS);
>>>>>>> upstream/master

    const auto to_move = state->get_to_move();
    const auto blacks_move = to_move == FastBoard::BLACK;

    const auto black_it = blacks_move ?
                          begin(input_data) :
<<<<<<< HEAD
                          begin(input_data) + INPUT_MOVES * BOARD_SQUARES;
    const auto white_it = blacks_move ?
                          begin(input_data) + INPUT_MOVES * BOARD_SQUARES :
                          begin(input_data);
    const auto to_move_it = blacks_move ?
        begin(input_data) + 2 * INPUT_MOVES * BOARD_SQUARES :
        begin(input_data) + (2 * INPUT_MOVES + 1) * BOARD_SQUARES;
=======
                          begin(input_data) + INPUT_MOVES * NUM_INTERSECTIONS;
    const auto white_it = blacks_move ?
                          begin(input_data) + INPUT_MOVES * NUM_INTERSECTIONS :
                          begin(input_data);
    const auto to_move_it = blacks_move ?
        begin(input_data) + 2 * INPUT_MOVES * NUM_INTERSECTIONS :
        begin(input_data) + (2 * INPUT_MOVES + 1) * NUM_INTERSECTIONS;
>>>>>>> upstream/master

    const auto moves = std::min<size_t>(state->get_movenum() + 1, INPUT_MOVES);
    // Go back in time, fill history boards
    for (auto h = size_t{0}; h < moves; h++) {
        // collect white, black occupation planes
        fill_input_plane_pair(state->get_past_board(h),
<<<<<<< HEAD
                              black_it + h * BOARD_SQUARES,
                              white_it + h * BOARD_SQUARES,
                              symmetry);
    }

    std::fill(to_move_it, to_move_it + BOARD_SQUARES, net_t(true));
=======
                              black_it + h * NUM_INTERSECTIONS,
                              white_it + h * NUM_INTERSECTIONS,
                              symmetry);
    }

    std::fill(to_move_it, to_move_it + NUM_INTERSECTIONS, float(true));
>>>>>>> upstream/master

    return input_data;
}

<<<<<<< HEAD
int Network::get_nn_idx_symmetry(const int vertex, int symmetry) {
    assert(vertex >= 0 && vertex < BOARD_SQUARES);
    assert(symmetry >= 0 && symmetry < 8);
    auto x = vertex % BOARD_SIZE;
    auto y = vertex / BOARD_SIZE;
    int newx;
    int newy;
=======
std::pair<int, int> Network::get_symmetry(const std::pair<int, int>& vertex,
                                          const int symmetry,
                                          const int board_size) {
    auto x = vertex.first;
    auto y = vertex.second;
    assert(x >= 0 && x < board_size);
    assert(y >= 0 && y < board_size);
    assert(symmetry >= 0 && symmetry < NUM_SYMMETRIES);
>>>>>>> upstream/master

    if ((symmetry & 4) != 0) {
        std::swap(x, y);
    }

<<<<<<< HEAD
    if (symmetry == 0) {
        newx = x;
        newy = y;
    } else if (symmetry == 1) {
        newx = x;
        newy = BOARD_SIZE - y - 1;
    } else if (symmetry == 2) {
        newx = BOARD_SIZE - x - 1;
        newy = y;
    } else {
        assert(symmetry == 3);
        newx = BOARD_SIZE - x - 1;
        newy = BOARD_SIZE - y - 1;
    }

    const auto newvtx = (newy * BOARD_SIZE) + newx;
    assert(newvtx >= 0 && newvtx < BOARD_SQUARES);
    return newvtx;
=======
    if ((symmetry & 2) != 0) {
        x = board_size - x - 1;
    }

    if ((symmetry & 1) != 0) {
        y = board_size - y - 1;
    }

    assert(x >= 0 && x < board_size);
    assert(y >= 0 && y < board_size);
    assert(symmetry != IDENTITY_SYMMETRY || vertex == std::make_pair(x, y));
    return {x, y};
}

size_t Network::get_estimated_size() {
    if (estimated_size != 0) {
        return estimated_size;
    }
    auto result = size_t{0};

    const auto lambda_vector_size =  [](const std::vector<std::vector<float>> &v) {
        auto result = size_t{0};
        for (auto it = begin(v); it != end(v); ++it) {
            result += it->size() * sizeof(float);
        }
        return result;
    };

    result += lambda_vector_size(m_fwd_weights->m_conv_weights);
    result += lambda_vector_size(m_fwd_weights->m_conv_biases);
    result += lambda_vector_size(m_fwd_weights->m_batchnorm_means);
    result += lambda_vector_size(m_fwd_weights->m_batchnorm_stddevs);

    result += m_fwd_weights->m_conv_pol_w.size() * sizeof(float);
    result += m_fwd_weights->m_conv_pol_b.size() * sizeof(float);

    // Policy head
    result += OUTPUTS_POLICY * sizeof(float); // m_bn_pol_w1
    result += OUTPUTS_POLICY * sizeof(float); // m_bn_pol_w2
    result += OUTPUTS_POLICY * NUM_INTERSECTIONS
                             * POTENTIAL_MOVES * sizeof(float); //m_ip_pol_w
    result += POTENTIAL_MOVES * sizeof(float); // m_ip_pol_b

    // Value head
    result += m_fwd_weights->m_conv_val_w.size() * sizeof(float);
    result += m_fwd_weights->m_conv_val_b.size() * sizeof(float);
    result += OUTPUTS_VALUE * sizeof(float); // m_bn_val_w1
    result += OUTPUTS_VALUE * sizeof(float); // m_bn_val_w2

    result += OUTPUTS_VALUE * NUM_INTERSECTIONS
                            * VALUE_LAYER * sizeof(float); // m_ip1_val_w
    result += VALUE_LAYER * sizeof(float);  // m_ip1_val_b

    result += VALUE_LAYER * sizeof(float); // m_ip2_val_w
    result += sizeof(float); // m_ip2_val_b
    return estimated_size = result;
}

size_t Network::get_estimated_cache_size() {
    return m_nncache.get_estimated_size();
}

void Network::nncache_resize(int max_count) {
    return m_nncache.resize(max_count);
>>>>>>> upstream/master
}
