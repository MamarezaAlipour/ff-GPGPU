﻿#pragma once
#include "ff_p.hpp"

typedef sycl::buffer<uint64_t, 2> buf_2d_u64_t;
typedef sycl::buffer<uint64_t, 1> buf_1d_u64_t;

typedef sycl::accessor<uint64_t, 2, sycl::access::mode::read,
    sycl::access::target::global_buffer>
    buf_2d_u64_rd_t;
typedef sycl::accessor<uint64_t, 2, sycl::access::mode::write,
    sycl::access::target::global_buffer>
    buf_2d_u64_wr_t;
typedef sycl::accessor<uint64_t, 2, sycl::access::mode::read_write,
    sycl::access::target::global_buffer>
    buf_2d_u64_rw_t;
typedef sycl::accessor<uint64_t, 1, sycl::access::mode::read,
    sycl::access::target::global_buffer>
    buf_1d_u64_rd_t;
typedef sycl::accessor<uint64_t, 1, sycl::access::mode::write,
    sycl::access::target::global_buffer>
    buf_1d_u64_wr_t;
typedef sycl::accessor<uint64_t, 1, sycl::access::mode::read_write,
    sycl::access::target::global_buffer>
    buf_1d_u64_rw_t;

inline constexpr uint64_t TWO_ADICITY = 32ul;
inline constexpr uint64_t TWO_ADIC_ROOT_OF_UNITY = 1753635133440165772ul;

SYCL_EXTERNAL uint64_t get_root_of_unity(uint64_t n);

sycl::event compute_dft_matrix(sycl::queue& q, buf_2d_u64_t& mat,
    buf_1d_u64_t& omega, const uint64_t dim,
    const uint64_t wg_size);

sycl::event compute_matrix_vector_multiplication(
    sycl::queue& q, buf_2d_u64_t& mat, buf_1d_u64_t& vec, buf_1d_u64_t& res,
    const uint64_t dim, const uint64_t wg_size);

sycl::event compute_vector_scalar_multilication(sycl::queue& q,
    buf_1d_u64_t& vec,
    const uint64_t factor,
    const uint64_t dim,
    const uint64_t wg_size);

void forward_transform(sycl::queue& q, buf_1d_u64_t& vec, buf_1d_u64_t& res,
    const uint64_t dim, const uint64_t wg_size);

void inverse_transform(sycl::queue& q, buf_1d_u64_t& vec, buf_1d_u64_t& res,
    const uint64_t dim, const uint64_t wg_size);

sycl::event compute_omega(sycl::queue& q, buf_1d_u64_t& omega,
    const uint64_t domain_size);

sycl::event compute_omega_inv(sycl::queue& q, buf_1d_u64_t& omega_inv,
    const uint64_t domain_size);

SYCL_EXTERNAL uint64_t bit_rev(uint64_t v, uint64_t max_bit_width);

SYCL_EXTERNAL uint64_t rev_all_bits(uint64_t n);

SYCL_EXTERNAL uint64_t permute_index(uint64_t idx, uint64_t size);

void cooley_tukey_fft(sycl::queue& q, buf_1d_u64_t& vec, buf_1d_u64_t& res,
    const uint64_t dim, const uint64_t wg_size);

void cooley_tukey_ifft(sycl::queue& q, buf_1d_u64_t& vec, buf_1d_u64_t& res,
    const uint64_t dim, const uint64_t wg_size);

// Computes blocked in-place parallel *square* matrix transposition
//
// Inspired from CUDA implementation
// https://forums.developer.nvidia.com/t/efficient-in-place-transpose-of-multiple-square-float-matrices/34327/4
//
// If matrix is not square, consider padding empty rows,
// as it's easy to do in row-major indexing
//
// Matrix is represented as 1d array
sycl::event matrix_transpose(sycl::queue& q, uint64_t* data, const uint64_t dim,
    std::vector<sycl::event> evts);

sycl::event matrix_transposed_initialise(sycl::queue& q, uint64_t* vec_src,
    uint64_t* vec_dst, const uint64_t rows,
    const uint64_t cols,
    const uint64_t width,
    const uint64_t wg_size,
    std::vector<sycl::event> evts);

/* Performs parallel in-place (I)FFT based on Cooley-Tukey style while taking
 USM based memory pointer as input data location.
 It works in 2D execution space of dimension `rows x cols`, with along x-axis
 width of matrix at max `width`, which may be == cols || == 2 * cols.
 In simple terms it performs, `rows`-many `cols`-point (I)FFT, in parallel.
 Whether FFT/ IFFT to be performed, it depends on provided `omega`.
 For kernel execution ordering, consider using events vector parameter
 and return event type properly, otherwise it'll result into data race, as
 dependency needs to be managed manually as I'm not using SYCL buffers
*/
sycl::event row_wise_transform(sycl::queue& q, uint64_t* vec, uint64_t* omega,
    const uint64_t rows, const uint64_t cols,
    const uint64_t width, const uint64_t wg_size,
    std::vector<sycl::event> evts);

// Computes powers of ω, which is n-th root of unity, if `n` is NTT
// domain size, such as {ω^0, ω^1, ω^2, ..., ω^(dim-1)}
sycl::event compute_twiddles(sycl::queue& q, uint64_t* twiddles,
    uint64_t* omega, const uint64_t dim,
    const uint64_t wg_size,
    std::vector<sycl::event> evts);

// Multiplies powers of ω ( n-th root of unity ) to each element
// of vector, which is here being interpreted as matrix of
// dimension N2 x N1, where N1 == width ( check function param ) or
// 2 * N1 == width. That's why during pointer arithmetic
// memory address linearization is performed using
// `width` to be column count of matrix ( which is actually `vec` i.e. domain )
//
// This function expects to receive one pointer to vector which stores
// powers of ω upto degree `rows` ( = N2 ), which are reused when computing
// final twiddle factor exponentiation ( along x-axis/ column identifier ) &
// multiplication
sycl::event twiddle_multiplication(sycl::queue& q, uint64_t* vec,
    uint64_t* twiddles, const uint64_t rows,
    const uint64_t cols, const uint64_t width,
    const uint64_t wg_size,
    std::vector<sycl::event> evts);

// Six step FFT algorithm based NTT implementation, which I've
// adapted from https://doi.org/10.1109/FPT.2013.6718406
//
// Returns forward transform result in same input vector
// so you'll loose your input, if you don't keep another copy of it
void six_step_fft(sycl::queue& q, uint64_t* vec, const uint64_t dim,
    const uint64_t wg_size);

// Six step FFT algorithm based Inverse NTT implementation, which I've
// adapted from https://doi.org/10.1109/FPT.2013.6718406
//
// Returns inverse transform result in same input vector
// so you'll loose your input, if you don't keep another copied version of it
void six_step_ifft(sycl::queue& q, uint64_t* vec, const uint64_t dim,
    const uint64_t wg_size);