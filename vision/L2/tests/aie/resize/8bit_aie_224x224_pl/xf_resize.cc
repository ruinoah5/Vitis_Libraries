/*
 * Copyright (C) 2019-2022, Xilinx, Inc.
 * Copyright (C) 2022-2023, Advanced Micro Devices, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "kernels.h"
#include "imgproc/xf_resize.hpp"

template <int WIDTH_IN, int HEIGHT_IN, int WIDTH_OUT, int HEIGHT_OUT, int IMG_HEIGHT_OUT>
void ResizeRunner<WIDTH_IN, HEIGHT_IN, WIDTH_OUT, HEIGHT_OUT, IMG_HEIGHT_OUT>::run(
    adf::input_buffer<uint8_t>& input, adf::output_buffer<uint8_t>& output) {
    xf::cv::aie::Resize<TILE_WIDTH_IN, TILE_HEIGHT_IN, TILE_WIDTH_OUT, TILE_HEIGHT_OUT, IMG_HEIGHT_OUT> resize(
        mPos, mwtsX, mwtsY);
    resize.runImpl(input, output);
}
