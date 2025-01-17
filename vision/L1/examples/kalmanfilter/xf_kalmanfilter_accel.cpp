/*
 * Copyright 2022 Xilinx, Inc.
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

#include "xf_kalmanfilter_accel_config.h"

static constexpr int __XF_DEPTH_NN = KF_N * KF_N;
static constexpr int __XF_DEPTH_NC = KF_N * KF_C;
static constexpr int __XF_DEPTH_MN = KF_M * KF_N;
static constexpr int __XF_DEPTH_N1 = KF_N;
static constexpr int __XF_DEPTH_M1 = KF_M;
static constexpr int __XF_DEPTH_C1 = KF_C;

void kalmanfilter_accel(ap_uint<INPUT_PTR_WIDTH>* in_A,
                        ap_uint<INPUT_PTR_WIDTH>* in_B,
                        ap_uint<INPUT_PTR_WIDTH>* in_Uq,
                        ap_uint<INPUT_PTR_WIDTH>* in_Dq,
                        ap_uint<INPUT_PTR_WIDTH>* in_H,
                        ap_uint<INPUT_PTR_WIDTH>* in_X0,
                        ap_uint<INPUT_PTR_WIDTH>* in_U0,
                        ap_uint<INPUT_PTR_WIDTH>* in_D0,
                        ap_uint<INPUT_PTR_WIDTH>* in_R,
                        ap_uint<INPUT_PTR_WIDTH>* in_u,
                        ap_uint<INPUT_PTR_WIDTH>* in_y,
                        unsigned char control_flag,
                        ap_uint<OUTPUT_PTR_WIDTH>* out_X,
                        ap_uint<OUTPUT_PTR_WIDTH>* out_U,
                        ap_uint<OUTPUT_PTR_WIDTH>* out_D) {
// clang-format off
    #pragma HLS INTERFACE m_axi      port=in_A      offset=slave  bundle=gmem0	depth=__XF_DEPTH_NN
    #pragma HLS INTERFACE m_axi      port=in_B      offset=slave  bundle=gmem1	depth=__XF_DEPTH_NC	
    #pragma HLS INTERFACE m_axi      port=in_Uq     offset=slave  bundle=gmem2	depth=__XF_DEPTH_NN
    #pragma HLS INTERFACE m_axi      port=in_Dq     offset=slave  bundle=gmem3	depth=__XF_DEPTH_N1
    #pragma HLS INTERFACE m_axi      port=in_H      offset=slave  bundle=gmem4	depth=__XF_DEPTH_MN
    #pragma HLS INTERFACE m_axi      port=in_X0     offset=slave  bundle=gmem5	depth=__XF_DEPTH_N1
    #pragma HLS INTERFACE m_axi      port=in_U0     offset=slave  bundle=gmem6	depth=__XF_DEPTH_NN
    #pragma HLS INTERFACE m_axi      port=in_D0     offset=slave  bundle=gmem7	depth=__XF_DEPTH_N1
    #pragma HLS INTERFACE m_axi      port=in_R      offset=slave  bundle=gmem8	depth=__XF_DEPTH_M1
    #pragma HLS INTERFACE m_axi      port=in_u      offset=slave  bundle=gmem9	depth=__XF_DEPTH_C1
    #pragma HLS INTERFACE m_axi      port=in_y      offset=slave  bundle=gmem10	depth=__XF_DEPTH_M1
    #pragma HLS INTERFACE m_axi      port=out_X     offset=slave  bundle=gmem11	depth=__XF_DEPTH_N1
    #pragma HLS INTERFACE m_axi      port=out_U     offset=slave  bundle=gmem12	depth=__XF_DEPTH_NN
    #pragma HLS INTERFACE m_axi      port=out_D     offset=slave  bundle=gmem13	depth=__XF_DEPTH_N1
    #pragma HLS INTERFACE s_axilite  port=control_flag 	          bundle=control
    #pragma HLS INTERFACE s_axilite  port=return 		          bundle=control
    // clang-format on
    xf::cv::Mat<IN_TYPE, KF_N, KF_N, NPPCX, XF_CV_DEPTH_A> A_mat(KF_N, KF_N);
#if KF_C != 0
    xf::cv::Mat<IN_TYPE, KF_N, KF_C, NPPCX, XF_CV_DEPTH_B> B_mat(KF_N, KF_C);
#endif
    xf::cv::Mat<IN_TYPE, KF_N, KF_N, NPPCX, XF_CV_DEPTH_UQ> Uq_mat(KF_N, KF_N);
    xf::cv::Mat<IN_TYPE, KF_N, 1, NPPCX, XF_CV_DEPTH_DQ> Dq_mat(KF_N, 1);
    xf::cv::Mat<IN_TYPE, KF_M, KF_N, NPPCX, XF_CV_DEPTH_H> H_mat(KF_M, KF_N);
    xf::cv::Mat<IN_TYPE, KF_N, 1, NPPCX, XF_CV_DEPTH_X0> X0_mat(KF_N, 1);
    xf::cv::Mat<IN_TYPE, KF_N, KF_N, NPPCX, XF_CV_DEPTH_U0> U0_mat(KF_N, KF_N);
    xf::cv::Mat<IN_TYPE, KF_N, 1, NPPCX, XF_CV_DEPTH_D0> D0_mat(KF_N, 1);
    xf::cv::Mat<IN_TYPE, KF_M, 1, NPPCX, XF_CV_DEPTH_R> R_mat(KF_M, 1);
#if KF_C != 0
    xf::cv::Mat<IN_TYPE, KF_C, 1, NPPCX, XF_CV_DEPTH_U> u_mat(KF_C, 1);
#endif
    xf::cv::Mat<IN_TYPE, KF_M, 1, NPPCX, XF_CV_DEPTH_Y> y_mat(KF_M, 1);

    xf::cv::Mat<OUT_TYPE, KF_N, 1, NPPCX, XF_CV_DEPTH_XOUT> Xout_mat(KF_N, 1);
    xf::cv::Mat<OUT_TYPE, KF_N, KF_N, NPPCX, XF_CV_DEPTH_UOUT> Uout_mat(KF_N, KF_N);
    xf::cv::Mat<OUT_TYPE, KF_N, 1, NPPCX, XF_CV_DEPTH_DOUT> Dout_mat(KF_N, 1);

// clang-format off
    #pragma HLS DATAFLOW
    // clang-format on

    xf::cv::accel_utils obj_inA, obj_inUq, obj_inU0, obj_inH, obj_inB;
    xf::cv::accel_utils obj_inDq, obj_inX0, obj_inD0, obj_inR, obj_iny;
    xf::cv::accel_utils obj_inu, obj_outU, obj_outD, obj_outX;

    // Retrieve xf::cv::Mat objects from img_in data:
    obj_inA.Array2xfMat<INPUT_PTR_WIDTH, IN_TYPE, KF_N, KF_N, NPPCX, XF_CV_DEPTH_A>(in_A, A_mat);
    obj_inUq.Array2xfMat<INPUT_PTR_WIDTH, IN_TYPE, KF_N, KF_N, NPPCX, XF_CV_DEPTH_UQ>(in_Uq, Uq_mat);
    obj_inU0.Array2xfMat<INPUT_PTR_WIDTH, IN_TYPE, KF_N, KF_N, NPPCX, XF_CV_DEPTH_U0>(in_U0, U0_mat);
    obj_inH.Array2xfMat<INPUT_PTR_WIDTH, IN_TYPE, KF_M, KF_N, NPPCX, XF_CV_DEPTH_H>(in_H, H_mat);
#if KF_C != 0
    obj_inB.Array2xfMat<INPUT_PTR_WIDTH, IN_TYPE, KF_N, KF_C, NPPCX, XF_CV_DEPTH_B>(in_B, B_mat);
#endif
    obj_inDq.Array2xfMat<INPUT_PTR_WIDTH, IN_TYPE, KF_N, 1, NPPCX, XF_CV_DEPTH_DQ>(in_Dq, Dq_mat);
    obj_inX0.Array2xfMat<INPUT_PTR_WIDTH, IN_TYPE, KF_N, 1, NPPCX, XF_CV_DEPTH_X0>(in_X0, X0_mat);
    obj_inD0.Array2xfMat<INPUT_PTR_WIDTH, IN_TYPE, KF_N, 1, NPPCX, XF_CV_DEPTH_D0>(in_D0, D0_mat);
    obj_inR.Array2xfMat<INPUT_PTR_WIDTH, IN_TYPE, KF_M, 1, NPPCX, XF_CV_DEPTH_R>(in_R, R_mat);
    obj_iny.Array2xfMat<INPUT_PTR_WIDTH, IN_TYPE, KF_M, 1, NPPCX, XF_CV_DEPTH_Y>(in_y, y_mat);
#if KF_C != 0
    obj_inu.Array2xfMat<INPUT_PTR_WIDTH, IN_TYPE, KF_C, 1, NPPCX, XF_CV_DEPTH_U>(in_u, u_mat);
#endif

#if KF_C != 0
    // Run xfOpenCV kernel:
    xf::cv::KalmanFilter<KF_N, KF_M, KF_C, KF_MTU, KF_MMU, XF_USE_URAM, 0, IN_TYPE, NPPCX, XF_CV_DEPTH_A, XF_CV_DEPTH_B,
                         XF_CV_DEPTH_UQ, XF_CV_DEPTH_DQ, XF_CV_DEPTH_H, XF_CV_DEPTH_X0, XF_CV_DEPTH_U0, XF_CV_DEPTH_D0,
                         XF_CV_DEPTH_R, XF_CV_DEPTH_U, XF_CV_DEPTH_Y, XF_CV_DEPTH_XOUT, XF_CV_DEPTH_UOUT,
                         XF_CV_DEPTH_DOUT>(A_mat, B_mat, Uq_mat, Dq_mat, H_mat, X0_mat, U0_mat, D0_mat, R_mat, u_mat,
                                           y_mat, Xout_mat, Uout_mat, Dout_mat, control_flag);
#else
    // Run xfOpenCV kernel:
    xf::cv::KalmanFilter<KF_N, KF_M, KF_C, KF_MTU, KF_MMU, XF_USE_URAM, 0, IN_TYPE, NPPCX, XF_CV_DEPTH_A, XF_CV_DEPTH_B,
                         XF_CV_DEPTH_UQ, XF_CV_DEPTH_DQ, XF_CV_DEPTH_H, XF_CV_DEPTH_X0, XF_CV_DEPTH_U0, XF_CV_DEPTH_D0,
                         XF_CV_DEPTH_R, XF_CV_DEPTH_U, XF_CV_DEPTH_Y, XF_CV_DEPTH_XOUT, XF_CV_DEPTH_UOUT,
                         XF_CV_DEPTH_DOUT>(A_mat, Uq_mat, Dq_mat, H_mat, X0_mat, U0_mat, D0_mat, R_mat, y_mat, Xout_mat,
                                           Uout_mat, Dout_mat, control_flag);

#endif

    obj_outU.xfMat2Array<OUTPUT_PTR_WIDTH, OUT_TYPE, KF_N, KF_N, NPPCX, XF_CV_DEPTH_UOUT>(Uout_mat, out_U);
    obj_outD.xfMat2Array<OUTPUT_PTR_WIDTH, OUT_TYPE, KF_N, 1, NPPCX, XF_CV_DEPTH_DOUT>(Dout_mat, out_D);
    obj_outX.xfMat2Array<OUTPUT_PTR_WIDTH, OUT_TYPE, KF_N, 1, NPPCX, XF_CV_DEPTH_XOUT>(Xout_mat, out_X);
    return;
} // End of kernel
