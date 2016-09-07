#pragma once

#include <vector>
#include <complex>

typedef std::vector<std::complex<double> > v_complex;
typedef std::vector<double> v_double;

// Fast Fourier Transform. Output is always multiple of 2
int FFT(size_t nInCnt, double *pInVal, std::vector<std::complex<double> > &arOutput, bool bInverse, bool bOrthNorm = true);
int FFT(size_t nInCnt, const std::complex<double> *pInVal, std::vector<std::complex<double> > &arOutput, bool bInverse, bool bOrthNorm = true);

// Fast Fourier Transform for real input data.
// Forward Transform: 2 * n -> n + 1 elements (i.e. complex conjurgate half is ignored)
int RFFT(const v_double &f, v_complex &F, bool bOrthNorm = true);
// Inverse Transform: n + 1 -> 2 * n elements
int IRFFT(const v_complex &F, v_double &f, bool bOrthNorm = true);
// Forward Transform: 2 * n -> n + 1 elements (i.e. complex conjurgate half is ignored)
int RFFT2(const v_double &f, v_complex &F);
// Inverse Transform: n + 1 -> 2 * n elements
int IRFFT2(const v_complex &F, v_double &f);

// Fast cosine transform. Type DCT-II, inverse is DCT-III. Output is always multiple of 2
int FDCT(size_t nInCnt, const double *pInVal, std::vector<double> &arOutput, bool bInverse, bool bOrthNorm = true);

// Fast sine transform. Output is always multiple of 2
int FDST(size_t nInCnt, const double *pInVal, std::vector<double> &arOutput, bool bInverse, bool bOrthNorm = true);

// Run self-tests
int run_FFT_selftest();
