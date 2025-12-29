#ifndef FFT_H
#define FFT_H

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <complex.h>
#include <assert.h>


#ifdef _MSC_VER
#		define F_Complex _Fcomplex
#		define cbuild(re, im)	_FCbuild(re, im)
#		define cfromreal(re)	_FCbuild(re, 0)
#		define cfromimag(im)	_FCbuild(0, im)
#		define addcc(a, b)		_FCbuild(crealf(a) + crealf(b), cimagf(a) + cimagf(b))
#		define subcc(a, b)		_FCbuild(crealf(a) - crealf(b), cimagf(a) - cimagf(b))
#		define mulcc					_FCmulcc
#else
#		define F_Complex _Complex float
#		define cbuild(re, im)	((re) + (im) * I)
#		define cfromreal(re)	(re)
#		define cfromimag(im)	(im)
#		define addcc(a, b)		((a) + (b))
#		define subcc(a, b)		((a) - (b))
#		define mulcc(a, b)		((a) * (b))
#endif

/* Cooley–Tukey FFT: https://cp-algorithms.com/algebra/fft.html*/
static void fft_rec(F_Complex in[], F_Complex out[], int _N)
{
  assert((_N & (_N-1)) == 0 && "This fft(Cooley-Tukey FFT) only works for power of two");
  if (_N == 1) {
    out[0] = in[0];
    return;
  }

  F_Complex in_a0[_N/2];
  F_Complex in_a1[_N/2];

  F_Complex out_a0[_N/2];
  F_Complex out_a1[_N/2];

  for (int k = 0; k < _N / 2; k++) {
    in_a0[k] = in[2 * k];
    in_a1[k] = in[2 * k + 1];
  }

  fft_rec(in_a0, out_a0, _N/2);
  fft_rec(in_a1, out_a1, _N/2);

  float angle = -2.0 * M_PI / _N;
  F_Complex w = 1.0f + I * 0.0f;
  F_Complex wn = cexp(I * angle);

  for (int n = 0; n < _N / 2; n++) {
    out[n] = out_a0[n] + w * out_a1[n];
    out[n + _N/2] = out_a0[n] - w * out_a1[n];
    w *= wn;
  }
}

/* Iterative Cooley–Tukey FFT with Bit-Reversal: https://cp-algorithms.com/algebra/fft.html*/
static void fft_bit(F_Complex in[], F_Complex out[], int _N)
{
  assert((_N & (_N-1)) == 0 && "This fft(Cooley-Tukey FFT) only works for power of two");
	for (int i = 0; i < _N; i++) {
		out[i] = in[i];
	}
	for (int i = 1, j = 0; i < _N; i++) {
		int bit = _N >> 1;
		for (; j & bit; bit >>= 1)
			j ^= bit;
		j ^= bit;
		if (i < j) {
			F_Complex temp = out[i];
			out[i] = out[j];
			out[j] = temp;
		}
	}

	for (int len = 2; len <= _N; len <<= 1) {
		float angle = -2.0f * M_PI / len;
		F_Complex wlen = cosf(angle) + sinf(angle) * I;
		for (int i = 0; i < _N; i += len) {
			F_Complex w = 1.0f + 0.0f * I;
			for (int j = 0; j < len / 2; j++) {
				F_Complex u = out[i + j], v = out[i + j + len/2] * w;
				out[i + j] = u + v;
				out[i + j + len/2] = u - v;
				w = w * wlen;
			}
		}
	}
}

#endif // FFT_H
