/*
 *  Copyright (c) 2010 The WebM project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "blockd.h"

//Stegozoa: this version assumes the block array has 25 * n_mb blocks, instead of 25
void vp8_setup_block_dptrs(MACROBLOCKD *x, int n_mb) {
  int r, c;

  for(int i = 0; i < n_mb; i++) {
      for (r = 0; r < 4; ++r) {
        for (c = 0; c < 4; ++c) {
          x->block[i * 25 + r * 4 + c].predictor = x->predictor + r * 4 * 16 + c * 4;
        }
      }

      for (r = 0; r < 2; ++r) {
        for (c = 0; c < 2; ++c) {
          x->block[i * 25 + 16 + r * 2 + c].predictor =
              x->predictor + 256 + r * 4 * 8 + c * 4;
        }
      }

      for (r = 0; r < 2; ++r) {
        for (c = 0; c < 2; ++c) {
          x->block[i * 25 + 20 + r * 2 + c].predictor =
              x->predictor + 320 + r * 4 * 8 + c * 4;
        }
      }

      for (r = 0; r < 25; ++r) {
        x->block[i * 25 + r].dqcoeff = x->dqcoeff + r * 16;
        x->block[i * 25 + r].qcoeff = (x->qcoeff + i * 400) + r * 16;
        x->block[i * 25 + r].eob = (x->eobs + i * 25) + r;
      }
  }

  memset(x->qcoeff + 400 * 4, 1, 400*sizeof(short));

  for(r = 0; r < 25; ++r)
      for(c = 0; c < 16; ++c)
          printf("QDCT:%d\n", x->block[4 * 25 + r].qcoeff[c];
}

//Stegozoa: this version assumes the block array has 25 * n_mb blocks, instead of 25
void vp8_build_block_doffsets(MACROBLOCKD *x, int n_mb) {
  int block;

  for (int i = 0; i < n_mb; ++i) {
      for (block = 0; block < 16; ++block) /* y blocks */
      {
        x->block[i * 25 + block].offset =
            (block >> 2) * 4 * x->dst.y_stride + (block & 3) * 4;
      }

      for (block = 16; block < 20; ++block) /* U and V blocks */
      {
        x->block[i * 25 + block + 4].offset = x->block[i * 25 + block].offset =
            ((block - 16) >> 1) * 4 * x->dst.uv_stride + (block & 1) * 4;
      }
  }
}
