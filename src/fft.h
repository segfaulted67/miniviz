#ifndef FFT_H
#define FFT_H

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <complex.h>
#include <assert.h>

static void fft(_Complex float in[], _Complex float out[], int _N)
{
  assert((_N & (_N-1)) == 0 && "This fft(Cooley-Tukey FFT) only works for power of two");
  if (_N == 1) {
    out[0] = in[0];
    return;
  }

  _Complex float in_a0[_N/2];
  _Complex float in_a1[_N/2];

  _Complex float out_a0[_N/2];
  _Complex float out_a1[_N/2];

  for (int k = 0; k < _N / 2; k++) {
    in_a0[k] = in[2 * k];
    in_a1[k] = in[2 * k + 1];
  }

  fft(in_a0, out_a0, _N/2);
  fft(in_a1, out_a1, _N/2);

  float angle = -2.0 * M_PI / _N;
  _Complex float w = 1.0f + I * 0.0f;
  _Complex float wn = cexp(I * angle);

  for (int n = 0; n < _N / 2; n++) {
    out[n] = out_a0[n] + w * out_a1[n];
    out[n + _N/2] = out_a0[n] - w * out_a1[n];
    w *= wn;
  }
}

#endif // FFT_H
