#include <math.h>
#include <memory.h>
#include <vector>
#include <complex>
#include "fft.h"

const double const_PI = (double) 3.141592653589793238462643383279502884;

// Radix-2 Cooley-Tukey Algorithm, 1965 (complex input, complex output)
// *x - Real, *y - Imaginary
// Compute values in place
static short do_fft(short int dir, long m, double *x, double *y)
{
	long n, i, i1, j, k, i2, l, l1, l2;
	double c1, c2, tx, ty, t1, t2, u1, u2, z;

	// Calculate the number of points
	n = 1;
	for (i = 0; i < m; i++)
		n *= 2;

	// Do the bit reversal
	i2 = n >> 1;
	j = 0;
	for (i = 0; i < n - 1; i++)
	{
		if (i < j)
		{
			tx = x[i];
			ty = y[i];
			x[i] = x[j];
			y[i] = y[j];
			x[j] = tx;
			y[j] = ty;
		}
		k = i2;
		while (k <= j)
		{
			j -= k;
			k >>= 1;
		}
		j += k;
	}

	// Compute the FFT
	c1 = -1.0;
	c2 = 0.0;
	l2 = 1;
	for (l = 0; l < m; l++)
	{
		l1 = l2;
		l2 <<= 1;
		u1 = 1.0;
		u2 = 0.0;
		for (j = 0; j < l1; j++)
		{
			for (i = j; i < n; i += l2)
			{
				i1 = i + l1;
				t1 = u1 * x[i1] - u2 * y[i1];
				t2 = u1 * y[i1] + u2 * x[i1];
				x[i1] = x[i] - t1;
				y[i1] = y[i] - t2;
				x[i] += t1;
				y[i] += t2;
			}
			z = u1 * c1 - u2 * c2;
			u2 = u1 * c2 + u2 * c1;
			u1 = z;
		}
		c2 = ::sqrt((1.0 - c1) / 2.0);
		if (dir == 1)
			c2 = -c2;
		c1 = ::sqrt((1.0 + c1) / 2.0);
	}

	// 1/sqrt(n) normalization
	z = 1. / ::sqrt(double(n));
	for (i = 0; i < n; i++)
	{
		x[i] *= z;
		y[i] *= z;
	}

	return 0;
}

static void do_bit_reverse2(int n, double *a)
{
	int j, j1, k, k1, l, m, m2, n2;
	double xr, xi;

	m = n >> 2;
	m2 = m << 1;
	n2 = n - 2;
	k = 0;
	for (j = 0; j <= m2 - 4; j += 4)
	{
		if (j < k)
		{
			xr = a[j];
			xi = a[j + 1];
			a[j] = a[k];
			a[j + 1] = a[k + 1];
			a[k] = xr;
			a[k + 1] = xi;
		}
		else if (j > k)
		{
			j1 = n2 - j;
			k1 = n2 - k;
			xr = a[j1];
			xi = a[j1 + 1];
			a[j1] = a[k1];
			a[j1 + 1] = a[k1 + 1];
			a[k1] = xr;
			a[k1 + 1] = xi;
		}
		k1 = m2 + k;
		xr = a[j + 2];
		xi = a[j + 3];
		a[j + 2] = a[k1];
		a[j + 3] = a[k1 + 1];
		a[k1] = xr;
		a[k1 + 1] = xi;
		l = m;
		while (k >= l)
		{
			k -= l;
			l >>= 1;
		}
		k += l;
	}
}


// Radix-2 Cooley-Tukey Algorithm, 1965 (complex input, complex output)
// a[2*i] - Real, a[2*i+1] - Imaginary
// This algorithm does not evaluate square root on each cycle, but is less presice than do_fft
// Compute values in place
static void do_complex_dft(int n, double wr, double wi, double *a)
{
	int i, j, k, l, m;
	double wkr, wki, wdr, wdi, ss, xr, xi;

	m = n;
	while (m > 4)
	{
		l = m >> 1;
		wkr = 1;
		wki = 0;
		wdr = 1 - 2 * wi * wi;
		wdi = 2 * wi * wr;
		ss = 2 * wdi;
		wr = wdr;
		wi = wdi;
		for (j = 0; j <= n - m; j += m)
		{
			i = j + l;
			xr = a[j] - a[i];
			xi = a[j + 1] - a[i + 1];
			a[j] += a[i];
			a[j + 1] += a[i + 1];
			a[i] = xr;
			a[i + 1] = xi;
			xr = a[j + 2] - a[i + 2];
			xi = a[j + 3] - a[i + 3];
			a[j + 2] += a[i + 2];
			a[j + 3] += a[i + 3];
			a[i + 2] = wdr * xr - wdi * xi;
			a[i + 3] = wdr * xi + wdi * xr;
		}
		for (k = 4; k <= l - 4; k += 4)
		{
			wkr -= ss * wdi;
			wki += ss * wdr;
			wdr -= ss * wki;
			wdi += ss * wkr;
			for (j = k; j <= n - m + k; j += m)
			{
				i = j + l;
				xr = a[j] - a[i];
				xi = a[j + 1] - a[i + 1];
				a[j] += a[i];
				a[j + 1] += a[i + 1];
				a[i] = wkr * xr - wki * xi;
				a[i + 1] = wkr * xi + wki * xr;
				xr = a[j + 2] - a[i + 2];
				xi = a[j + 3] - a[i + 3];
				a[j + 2] += a[i + 2];
				a[j + 3] += a[i + 3];
				a[i + 2] = wdr * xr - wdi * xi;
				a[i + 3] = wdr * xi + wdi * xr;
			}
		}
		m = l;
	}
	if (m > 2)
	{
		for (j = 0; j <= n - 4; j += 4)
		{
			xr = a[j] - a[j + 2];
			xi = a[j + 1] - a[j + 3];
			a[j] += a[j + 2];
			a[j + 1] += a[j + 3];
			a[j + 2] = xr;
			a[j + 3] = xi;
		}
	}
	if (n > 4)
	{
		do_bit_reverse2(n, a);
	}
}

// Radix-2 Cooley-Tukey Algorithm, 1965 (real input, complex output)
static void do_real_rdft(int n, double wr, double wi, double *a)
{
	int j, k;
	double wkr, wki, wdr, wdi, ss, xr, xi, yr, yi;

	if (n > 4)
	{
		wkr = 0;
		wki = 0;
		wdr = wi * wi;
		wdi = wi * wr;
		ss = 4 * wdi;
		wr = 1 - 2 * wdr;
		wi = 2 * wdi;
		if (wi >= 0)
		{
			do_complex_dft(n, wr, wi, a);
			xi = a[0] - a[1];
			a[0] += a[1];
			a[1] = xi;
		}
		for (k = (n >> 1) - 4; k >= 4; k -= 4)
		{
			j = n - k;
			xr = a[k + 2] - a[j - 2];
			xi = a[k + 3] + a[j - 1];
			yr = wdr * xr - wdi * xi;
			yi = wdr * xi + wdi * xr;
			a[k + 2] -= yr;
			a[k + 3] -= yi;
			a[j - 2] += yr;
			a[j - 1] -= yi;
			wkr += ss * wdi;
			wki += ss * (0.5 - wdr);
			xr = a[k] - a[j];
			xi = a[k + 1] + a[j + 1];
			yr = wkr * xr - wki * xi;
			yi = wkr * xi + wki * xr;
			a[k] -= yr;
			a[k + 1] -= yi;
			a[j] += yr;
			a[j + 1] -= yi;
			wdr += ss * wki;
			wdi += ss * (0.5 - wkr);
		}
		j = n - 2;
		xr = a[2] - a[j];
		xi = a[3] + a[j + 1];
		yr = wdr * xr - wdi * xi;
		yi = wdr * xi + wdi * xr;
		a[2] -= yr;
		a[3] -= yi;
		a[j] += yr;
		a[j + 1] -= yi;
		if (wi < 0)
		{
			a[1] = 0.5 * (a[0] - a[1]);
			a[0] -= a[1];
			do_complex_dft(n, wr, wi, a);
		}
	}
	else 
	{
		if (wi < 0)
		{
			a[1] = 0.5 * (a[0] - a[1]);
			a[0] -= a[1];
		}
		if (n > 2)
		{
			xr = a[0] - a[2];
			xi = a[1] - a[3];
			a[0] += a[2];
			a[1] += a[3];
			a[2] = xr;
			a[3] = xi;
		}
		if (wi >= 0)
		{
			xi = a[0] - a[1];
			a[0] += a[1];
			a[1] = xi;
		}
	}
}

// Fast Discrete Cosine Transform based on FFT (real input, real input)
static void do_real_dct(int n, double wr, double wi, double *a)
{
	int j, k, m;
	double wkr, wki, wdr, wdi, ss, xr;

	if (n > 2)
	{
		wkr = 0.5;
		wki = 0.5;
		wdr = 0.5 * (wr - wi);
		wdi = 0.5 * (wr + wi);
		ss = 2 * wi;
		if (wi < 0)
		{
			xr = a[n - 1];
			for (k = n - 2; k >= 2; k -= 2)
			{
				a[k + 1] = a[k] - a[k - 1];
				a[k] += a[k - 1];
			}
			a[1] = 2 * xr;
			a[0] *= 2;
			do_real_rdft(n, 1 - ss * wi, ss * wr, a);
			xr = wdr;
			wdr = wdi;
			wdi = xr;
			ss = -ss;
		}
		m = n >> 1;
		for (k = 1; k <= m - 3; k += 2)
		{
			j = n - k;
			xr = wdi * a[k] - wdr * a[j];
			a[k] = wdr * a[k] + wdi * a[j];
			a[j] = xr;
			wkr -= ss * wdi;
			wki += ss * wdr;
			xr = wki * a[k + 1] - wkr * a[j - 1];
			a[k + 1] = wkr * a[k + 1] + wki * a[j - 1];
			a[j - 1] = xr;
			wdr -= ss * wki;
			wdi += ss * wkr;
		}
		k = m - 1;
		j = n - k;
		xr = wdi * a[k] - wdr * a[j];
		a[k] = wdr * a[k] + wdi * a[j];
		a[j] = xr;
		a[m] *= wki + ss * wdr;
		if (wi >= 0)
		{
			do_real_rdft(n, 1 - ss * wi, ss * wr, a);
			xr = a[1];
			for (k = 2; k <= n - 2; k += 2)
			{
				a[k - 1] = a[k] - a[k + 1];
				a[k] += a[k + 1];
			}
			a[n - 1] = xr;
		}
	}
	else
	{
		if (wi >= 0)
		{
			xr = 0.5 * (wr + wi) * a[1];
			a[1] = a[0] - xr;
			a[0] += xr;
		}
		else
		{
			xr = a[0] - a[1];
			a[0] += a[1];
			a[1] = 0.5 * (wr - wi) * xr;
		}
	}
}

// Fast Discrete Sine Transform based on FFT (real input, real input)
static void ro_real_dst(int n, double wr, double wi, double *a)
{
	int j, k, m;
	double wkr, wki, wdr, wdi, ss, xr;

	if (n > 2)
	{
		wkr = 0.5;
		wki = 0.5;
		wdr = 0.5 * (wr - wi);
		wdi = 0.5 * (wr + wi);
		ss = 2 * wi;
		if (wi < 0)
		{
			xr = a[n - 1];
			for (k = n - 2; k >= 2; k -= 2)
			{
				a[k + 1] = a[k] + a[k - 1];
				a[k] -= a[k - 1];
			}
			a[1] = -2 * xr;
			a[0] *= 2;
			do_real_rdft(n, 1 - ss * wi, ss * wr, a);
			xr = wdr;
			wdr = -wdi;
			wdi = xr;
			wkr = -wkr;
		}
		m = n >> 1;
		for (k = 1; k <= m - 3; k += 2)
		{
			j = n - k;
			xr = wdi * a[j] - wdr * a[k];
			a[k] = wdr * a[j] + wdi * a[k];
			a[j] = xr;
			wkr -= ss * wdi;
			wki += ss * wdr;
			xr = wki * a[j - 1] - wkr * a[k + 1];
			a[k + 1] = wkr * a[j - 1] + wki * a[k + 1];
			a[j - 1] = xr;
			wdr -= ss * wki;
			wdi += ss * wkr;
		}
		k = m - 1;
		j = n - k;
		xr = wdi * a[j] - wdr * a[k];
		a[k] = wdr * a[j] + wdi * a[k];
		a[j] = xr;
		a[m] *= wki + ss * wdr;
		if (wi >= 0)
		{
			do_real_rdft(n, 1 - ss * wi, ss * wr, a);
			xr = a[1];
			for (k = 2; k <= n - 2; k += 2)
			{
				a[k - 1] = a[k + 1] - a[k];
				a[k] += a[k + 1];
			}
			a[n - 1] = -xr;
		}
	}
	else
	{
		if (wi >= 0)
		{
			xr = 0.5 * (wr + wi) * a[1];
			a[1] = xr - a[0];
			a[0] += xr;
		}
		else
		{
			xr = a[0] + a[1];
			a[0] -= a[1];
			a[1] = 0.5 * (wr - wi) * xr;
		}
	}
}

// Discrete Cosine Transform (slow) (real input, real input)
static int do_real_dct_slow(bool bInverse, int n, double *in, double *out)
{
	if (n < 2) return -1;

	if (bInverse)
	{
		// Slow DCT Type III
		int i, k;
		double x0 = -0.5*in[0];
		double sqN = 1. / ::sqrt(double(n));
		for (i = 0; i < n; i++)
		{
			double v = 0.;
			for (k = 0; k < n; k++)
			{
				v += in[k] * cos(const_PI * (double(i) + 0.5) * (double(k) / double(n)));
			}
			out[i] = 2.*(x0 + v)*sqN;
		}
		return 0;
	}
	else
	{
		// Slow DCT Type II
		int i, k;
		double sqN = 1. / ::sqrt(double(n));
		for (i = 0; i < n; i++)
		{
			double v = 0.;
			for (k = 0; k < n; k++)
			{
				v += in[k] * cos(const_PI * double(i) * ((double(k) + 0.5) / double(n)));
			}
			out[i] = v*sqN;
		}
		return 0;
	}
}

// Fast Fourier transform (real input). Output is always multiple of 2
int FFT(size_t nInCnt, double *pInVal, std::vector<std::complex<double> > &arOutput, bool bInverse, bool bOrthNorm/* = true*/)
{
	if (nInCnt < 2) return -1;

	size_t N = 1;
	int pw = 0;
	while (N < nInCnt)
	{
		N <<= 1;
		pw++;
	}

	std::vector<double> arTmpOutput;
	arTmpOutput.resize(2 * N);
	arOutput.resize(N);
	size_t i;
	for (i = 0; i < nInCnt; i++)
		arTmpOutput[2 * i] = pInVal[i];

	double w1 = cos(const_PI / double(N) );
	double w2 = bInverse ? sin(const_PI / double(N) ) : -sin(const_PI / double(N) );

	do_complex_dft( int(2 * N), w1, w2, &arTmpOutput[0]);

	double norm = 1.;
	if (bOrthNorm)
		norm = ::sqrt(1. / N);
	else if (!bInverse)
		norm = 1. / N;
	for (i = 0; i < N; i++)
		arOutput[i] = std::complex<double>(arTmpOutput[2 * i] * norm, arTmpOutput[2 * i + 1] * norm);

	return 0;
}

// Fast Fourier transform (complex input). Output is always multiple of 2
int FFT(size_t nInCnt, const std::complex<double> *pInVal, std::vector<std::complex<double> > &arOutput, bool bInverse, bool bOrthNorm/* = true*/)
{
	if (nInCnt < 2) return -1;

	size_t N = 1;
	int pw = 0;
	while (N < nInCnt)
	{
		N <<= 1;
		pw++;
	}

	std::vector<double> arTmpOutput;
	arTmpOutput.resize(2 * N);
	arOutput.resize(N);
	size_t i;
	for (i = 0; i < nInCnt; i++)
	{
		arTmpOutput[2 * i] = pInVal[i].real();
		arTmpOutput[2 * i + 1] = pInVal[i].imag();
	}

	double w1 = cos(const_PI / double(N) );
	double w2 = bInverse ? sin(const_PI / double(N) ) : -sin(const_PI / double(N) );

	do_complex_dft(int(2 * N), w1, w2, &arTmpOutput[0]);

	double norm = 1.;
	if (bOrthNorm)
		norm = ::sqrt(1. / N);
	else if (!bInverse)
		norm = 1. / N;
	for (i = 0; i < N; i++)
		arOutput[i] = std::complex<double>(arTmpOutput[2 * i] * norm, arTmpOutput[2 * i + 1] * norm);

	return 0;
}

// Forward Transform: 2 * n -> n + 1 elements (i.e. complex conjurgate half is ignored)
// Inverse Transform: n + 1 -> 2 * n elements
int RFFT(const v_double &f, v_complex &F, bool bOrthNorm /*= true*/)
{
	size_t n = F.size();
	if (n < 4) return -1;

	size_t N = 1;
	int pw = 0;
	while (N < n)
	{
		N <<= 1;
		pw++;
	}

	std::vector<double> arTmpOutput;
	arTmpOutput.resize(2 * N);
	F.resize(N/2+1);
	size_t i;
	for (i = 0; i < n; i++)
		arTmpOutput[2 * i] = f[i];

	double w1 =  cos(const_PI / double(N));
	double w2 = -sin(const_PI / double(N));

	do_complex_dft(int(2 * N), w1, w2, &arTmpOutput[0]);

	double norm = bOrthNorm ? ::sqrt(1. / N) : 1. / N;;
		
	for (i = 0; i < N/2+1; i++)
		F[i] = std::complex<double>(arTmpOutput[2 * i] * norm, arTmpOutput[2 * i + 1] * norm);

	return 0;
}

// Inverse Transform: n + 1 -> 2 * n elements
int IRFFT(const v_complex &F, v_double &f, bool bOrthNorm/* = true*/)
{
	size_t n = F.size();
	if (n < 3) return -1;

	size_t N = F.size() - 1;
	if ( (N & (N-1)) !=0 ) return -2;  // Must be 2^m+1 elements	
	N <<= 1;

	std::vector<double> arTmpOutput;
	arTmpOutput.resize(2 * N);
	f.resize(N);
	size_t i;
	for (i = 0; i < n; i++)
	{
		arTmpOutput[2 * i]     = F[i].real();
		arTmpOutput[2 * i + 1] = F[i].imag();
	}
	for (i = 1; i < n; i++)
	{
		arTmpOutput[2 * N - 2 * i]     =  F[i].real();
		arTmpOutput[2 * N - 2 * i + 1] = -F[i].imag();		
	}

	double w1 = cos(const_PI / double(N));
	double w2 = sin(const_PI / double(N));

	do_complex_dft(int(2 * N), w1, w2, &arTmpOutput[0]);

	double norm = bOrthNorm ? ::sqrt(1. / N) : 1.;

	if (bOrthNorm) norm = ::sqrt(1. / N);
	for (i = 0; i < N; i++)
		f[i] = arTmpOutput[2 * i] * norm;

	return 0;
}

// Forward Transform: 2 * n -> n + 1 elements (i.e. complex conjurgate half is ignored)
// Inverse Transform: n + 1 -> 2 * n elements
int RFFT2(const v_double &f, v_complex &F)
{
	size_t n = F.size();
	if (n < 4) return -1;

	size_t N = 1;
	int pw = 0;
	while (N < n)
	{
		N <<= 1;
		pw++;
	}

	std::vector<double> arReal;
	std::vector<double> arImag;
	arReal.resize(N);
	arImag.resize(N);
	F.resize(N / 2 + 1);
	size_t i;
	for (i = 0; i < n; i++)
		arReal[i] = f[i];

	do_fft(1, pw, &arReal[0], &arImag[0]);

	for (i = 0; i < N / 2 + 1; i++)
		F[i] = std::complex<double>(arReal[i], arImag[i]);

	return 0;
}

// Inverse Transform: n + 1 -> 2 * n elements
int IRFFT2(const v_complex &F, v_double &f)
{
	size_t n = F.size();
	if (n < 3) return -1;

	size_t N = F.size() - 1;
	if ((N & (N - 1)) != 0) return -2;  // Must be 2^m+1 elements	
	N <<= 1;

	size_t nn = 1;
	int pw = 0;
	while (nn < N)
	{
		nn <<= 1;
		pw++;
	}


	std::vector<double> arReal;
	std::vector<double> arImag;
	arReal.resize(N);
	arImag.resize(N);
	f.resize(N);
	size_t i;
	for (i = 0; i < n; i++)
	{
		arReal[i] = F[i].real();
		arImag[i] = F[i].imag();
	}
	for (i = 1; i < n; i++)
	{
		arReal[N - i] = F[i].real();
		arImag[N - i] = -F[i].imag();
	}

	double w1 = cos(const_PI / double(N));
	double w2 = -sin(const_PI / double(N));

	do_fft(-1, pw, &arReal[0], &arImag[0]);

	for (i = 0; i < N; i++)
		f[i] = arReal[i];

	return 0;
}



// Fast discrete cosine transform based on FFT (interface)
int FDCT(size_t nInCnt, const double *pInVal, std::vector<double> &arOutput, bool bInverse, bool bOrthNorm/*=true*/)
{
	if (nInCnt < 2) return -1;

	size_t N = 1;
	int pw = 0;
	while (N < nInCnt)
	{
		N <<= 1;
		pw++;
	}

	arOutput.resize(N);
	size_t i;
	for (i = 0; i < nInCnt; i++)
	{
		if (!bInverse)
			arOutput[i] = pInVal[i];
		else
		{
			if (bOrthNorm)
				arOutput[i] = 2.*pInVal[i];
			else
				arOutput[i] = 0.5*pInVal[i];
		}
	}

	double N_1 = 1. / double(N);
	double N2 = double(N) * 2.;
	double w1 = cos(const_PI / N2);
	double w2 = bInverse ? sin(const_PI / N2) : -sin(const_PI / N2);

	if (bInverse) arOutput[0] *= 0.5;

	do_real_dct(int(N), w1, w2, &arOutput[0]);

	if (bOrthNorm)
	{
		double norm = sqrt(N_1);
		for (i = 0; i < N; i++)
		{
			arOutput[i] *= norm;
		}
	}
	else
	{

		for (i = 0; i < N; i++)
		{
			if (bInverse)
				arOutput[i] *= 2. * N_1;
			else
				arOutput[i] *= 2;
		}
	}

	return 0;
}

// Fast discrete cosine transform based on 4N FFT method (interface)
int FDCT2(bool bInverse, int n, double *in, double *out)
{
	if (n < 2) return -1;

	int i;
	int N = 1;
	int pw = 0;
	while (N < n)
	{
		N <<= 1;
		pw++;
	}

	int N_fft = N * 4;

	std::vector<double> fft_re(N_fft), fft_im(N_fft);

	if (bInverse)
	{
		for (i = 0; i < n; i++)
		{
			fft_re[i] = in[i];
			fft_re[2*N - i] = -in[i];
		}
		for (i = 1; i < n; i++)
		{
			fft_re[2*N + i] = - in[i];
			fft_re[4*N - i] = in[i];
		}
	}
	else
	{
		for (i = 0; i < n; i++)
		{
			fft_re[i * 2 + 1] = in[i];
			fft_re[N_fft - (i * 2) - 1] = in[i];
		}
	}

	if (bInverse)
		do_fft(-1, pw + 2, &fft_re[0], &fft_im[0]);
	else
		do_fft(1, pw + 2, &fft_re[0], &fft_im[0]);

	if (bInverse)
	{
		for (i = 0; i < n; i++)
		{
			out[i] = fft_re[i*2+1];
		}
	}
	else
	{
		for (i = 0; i < n; i++)
		{
			out[i] = fft_re[i];
		}
	}
	return 0;
}

// Fast discrete sine transform (interface)
int FDST(size_t nInCnt, const double *pInVal, std::vector<double> &arOutput, bool bInverse, bool bOrthNorm/*=true*/)
{
	if (nInCnt < 2) return -1;

	size_t N = 1;
	int pw = 0;
	while (N < nInCnt)
	{
		N <<= 1;
		pw++;
	}

	arOutput.resize(N);
	size_t i;
	for (i = 0; i < nInCnt; i++)
	{
		arOutput[i] = pInVal[i];
	}

	double N_1 = 1. / double(N);
	double N2 = double(N) * 2.;
	double w1 = cos(const_PI / N2);
	double w2 = bInverse ? -sin(const_PI / N2) : sin(const_PI / N2);

	ro_real_dst(int(N), w1, w2, &arOutput[0]);

	if (bInverse) arOutput[0] *= 0.5;

	if (bOrthNorm)
	{
		double norm = sqrt(2.*N_1);

		for (i = 0; i < N; i++)
		{
			arOutput[i] *= norm;
		}
	}
	else
	{
		if (bInverse)
		{
			for (i = 0; i < N; i++)
			{
				arOutput[i] *= 2. * N_1;
			}
		}
	}

	return 0;
}

static int nTestNum = 0;
static int nTestErrNum = 0;

#define TEST(a) { nTestNum ++; \
	if (!(a)) {printf("Test %d FAILED! (%s)\n", nTestNum, #a); nTestErrNum++; } }
#define EXPECT(a,b) { nTestNum ++; \
	if (!(fabs(double(a)-double(b))<0.0001)) {printf("Test %d FAILED! (%s==%g, expexted %g)\n", nTestNum, #a, double(a), double(b)); nTestErrNum++; } }
#define FAIL(a) { nTestNum ++; \
	if (!(a)) {printf("Test %d FAILED! (%s)\n", nTestNum, #a); return nTestNum; } }
#define EXPECT_FAIL(a,b) { nTestNum ++; \
	if (!(fabs(double(a)-double(b))<0.0001)) {printf("Test %d FAILED! (%s==%g, expexted %g)\n", nTestNum, #a, double(a), double(b)); return nTestNum; } }

#define USE_N 8

int run_FFT_selftest()
{
	int i;
	double x[USE_N], sx[USE_N], xx[USE_N], dct_slow[USE_N], idct_slow[USE_N];// , s[USE_N], y[USE_N];
	std::vector<double> s, y, t2, t3, f;

	x[0] = 4.;
	x[1] = 3.;
	x[2] = 5.;
	x[3] = 10.;
	FDCT2(false, 4, x, sx);
	FDCT2(true, 4, sx, xx);
	FDCT(4, x, t2, false, true);
	FDCT(4, &t2[0], t3, true, true);
	do_real_dct_slow(false, 4, x, dct_slow);
	do_real_dct_slow(true, 4, dct_slow, idct_slow);

	// Check DCT (slow version)
	//TEST(fabs(dct_slow[0] - 11) < 0.0001);
	EXPECT(dct_slow[0], 11.);
	EXPECT(dct_slow[1], -3.15432);
	EXPECT(dct_slow[2], 2.12132);
	EXPECT(dct_slow[3], -0.224171);
	EXPECT(idct_slow[0], 4);
	EXPECT(idct_slow[1], 3);
	EXPECT(idct_slow[2], 5);
	EXPECT(idct_slow[3], 10);

	// Check Fast DCT
	for (i = 0; i < 4; i++) EXPECT(dct_slow[i], sx[i]);
	for (i = 0; i < 4; i++) EXPECT(idct_slow[i], xx[i]);
	for (i = 0; i < 4; i++) EXPECT(dct_slow[i], t2[i]);
	for (i = 0; i < 4; i++) EXPECT(idct_slow[i], t3[i]);

	printf("Cosine transform1\n");
	for (i = 0; i < 4; i++)
	{
		printf("%d. %7.3g \t=> %7.3g\t=> %7.3g\t=>%7.3g\t=>%g\n", i, x[i], sx[i], t2[i], t3[i], t3[i] / x[i]);
	}

	printf("Cosine transform2\n");
	for (i = 0; i < 4; i++)
	{
		if (fabs(xx[i]) < 1e-13) xx[i] = 0.;
		printf("%d. %5g \t=> %7.3g\t=> %g\n", i, x[i], sx[i], xx[i]);
	}


	memset(x, 0, USE_N * sizeof(double));
	for (i = 0; i < USE_N; i++)
		x[i] = 1.;
	x[1] = 4.;

	printf("Cosine transform\n");
	FDCT(USE_N, x, s, false, true);
	FDCT(s.size(), &s[0], y, true, true);
	for (i = 0; i < USE_N; i++)
	{
		if (fabs(y[i]) < 1e-13) y[i] = 0.;
		printf("%d. %g \t=> %g \t=> %g\n", i, x[i], s[i], y[i]);
	}

	printf("Sine transform\n");
	FDST(USE_N, x, s, false, false);
	FDST(s.size(), &s[0], y, true, false);
	for (i = 0; i < USE_N; i++)
	{
		if (fabs(y[i]) < 1e-13) y[i] = 0.;
		printf("%d. %g \t=> %g \t=> %g\n", i, x[i], s[i], y[i]);
	}

	printf("\nFFT\n");

	std::vector<std::complex<double> > cX(USE_N), cF, cIF;
	for (i = 0; i < USE_N; i++)
	{
		cX[i] = 1.;
		x[i] = 1.;
	}
	x[1] = 4.;
	cX[1] = 4.;
	
	FFT(USE_N, x, cF, false, true);
	FFT(USE_N, &cF[0], cIF, true, true);

	for (i = 0; i < USE_N; i++)
	{
		//if (fabs(cIF[i]) < 1e-13) cIF[i] = 0.;
		printf("%d. %g \t=> (%g + j%g) \t\t=> %g\n", i, cX[i].real(), cF[i].real(), cF[i].imag(), cIF[i].real());
	}

	printf("\nRFFT\n");
	f.resize(USE_N);
	for (i = 0; i < USE_N; i++)
	{
		cX[i] = 1.;
		f[i] = 1.;
	}
	f[1] = 4.;
	cX[1] = 4.;
	RFFT2(f, cF);

	/*for (i=1; i<(int)cF.size(); i++)
	{
		double mod = abs ( cF[i] );
		double ph = arg ( cF[i] ) + 3.14159265/4.;
		cF[i] = std::polar(mod, ph);
	}*/
	IRFFT2(cF, y);

	printf("SRC vs TRG\n");
	for (i = 0; i < USE_N; i++)
	{
		printf("%d. %g \t=> %g\n", i, f[i], y[i]);
	}
	printf("Spectrum\n");
	for (i = 0; i < (int) cF.size(); i++)
	{
		printf("%d. (%5g ; j%-5g)\n", i, cF[i].real(), cF[i].imag());
	}


	return 0;
}
