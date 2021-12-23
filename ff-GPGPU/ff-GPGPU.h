//==============================================================
// Copyright © 2021 Intel Corporation
//
// SPDX-License-Identifier: MIT
// =============================================================
#pragma once

#ifdef DPCPP_DLL_EXPORTS
#define DPCPP_DLL_API __declspec(dllexport)
#else
#define DPCPP_DLL_API __declspec(dllimport)
#endif

#include <array>
#include <iostream>

// ARRAY type & data size for use in this example
constexpr size_t array_size = 10000;
using IntArray=std::array<int, array_size>;

DPCPP_DLL_API void initialize_array(IntArray &a);
DPCPP_DLL_API void vector_add(const IntArray &addend_1, const IntArray &addend_2, IntArray &sum_parallel);
