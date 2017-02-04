#ifndef _RNG_H_INCLUDED_2014_04_01
#define _RNG_H_INCLUDED_2014_04_01

#include <time.h>

template <typename RngEngine = CRngEngineArc4Ex>
class CRandom
{
public:
	CRandom();
	~CRandom();

	//Seed by system timer
	void Seed();
	//Seed by a 32-bit integer
	void Seed(unsigned int seed);
	//Seed by a arbitrary buffer
	void Seed(const char *pBuf, int buflen);

	double Get01();
	int GetUniformInterval(int a, int b);
	double GetUniformInterval(double a, double b);
	double GetExponential(double mean);
	double GetTriangular(double low, double high, double mode);
	double GetNormal(double mean, double std);
	void GetNormalBivariate(double &r1, double &r2, double mean1, double std1, double mean2, double std2, double cov12);
	double GetCustomDiscrete(int nCount, double *pfWeights, double *pfValues);
	double GetCustomContinious(int nCount, double *pfWeights, double *pfValues);

protected:

	RngEngine engine;
};

class CRngEngineArc4
{
public:
	CRngEngineArc4();

	void Init(const char *pBuf, int buflen);
	int Generate(char *data, size_t len);

private:
	int x_;
	int y_;
	unsigned char m_[256];
};

class CRngEngineArc4Ex
{
public:
	CRngEngineArc4Ex();

	void Init(const char *pBuf, int buflen);
	int Generate(char *data, size_t len);

private:
	int x_;
	int y_;
	unsigned char m_[256];
};

typedef CRandom<CRngEngineArc4Ex> CRng;

//----------------------------------------------- CRng ---------------------------------------------------

template <class RngEngine>
CRandom<RngEngine>::CRandom()
{
}

template <class RngEngine>
CRandom<RngEngine>::~CRandom()
{
}

//Seed by system timer
template <class RngEngine>
void CRandom<RngEngine>::Seed()
{
	unsigned int i = (unsigned int)time(NULL);
	engine.Init((const char *)&i, 4);
}

//Seed by a 32-bit integer
template <class RngEngine>
void CRandom<RngEngine>::Seed(unsigned int seed)
{
	engine.Init((const char *)&seed, 4);
}

//Seed by a arbitrary buffer
template <class RngEngine>
void CRandom<RngEngine>::Seed(const char *pBuf, int buflen)
{
	engine.Init(pBuf, buflen);
}

template <class RngEngine>
double CRandom<RngEngine>::Get01()
{
	unsigned int i = 0;
	unsigned char c;
	engine.Generate((char *)&c, 1);	i = c;
	engine.Generate((char *)&c, 1);	i |= ((unsigned int)c << 8);
	engine.Generate((char *)&c, 1);	i |= ((unsigned int)c << 16);
	engine.Generate((char *)&c, 1);	i |= ((unsigned int)c << 24);
	return i*(1.0 / 4294967295.0);
}

template <class RngEngine>
int CRandom<RngEngine>::GetUniformInterval(int a, int b)
{
	unsigned char c;
	int s = b - a + 1;
	unsigned int r_num;

	if (s == 0) return a;
	bool bExit = false;
	while (!bExit)
	{
		if (s < 65536)
		{
			engine.Generate((char *)&c, 1);
			r_num = c;
			engine.Generate((char *)&c, 1);
			r_num |= ((unsigned int)c << 8);
			// Drop residual numbers to avoid bias from uniform distribution
			if ((int)r_num > (65536 - (65536 % s)))
				continue;
		}
		else
		{
			engine.Generate((char *)&c, 1);
			r_num = c;
			engine.Generate((char *)&c, 1);
			r_num |= ((unsigned int)c << 8);
			engine.Generate((char *)&c, 1);
			r_num |= ((unsigned int)c << 16);
			engine.Generate((char *)&c, 1);
			r_num |= ((unsigned int)c << 24);
			// Drop residual numbers to avoid bias from uniform distribution
			if (r_num > (4294967296 - (4294967296 % s)))
				continue;
		}
		bExit = true;
	}

	return (r_num % s) + a;
}

template <class RngEngine>
double CRandom<RngEngine>::GetUniformInterval(double a, double b)
{
	double s = b - a;
	double r = Get01()*s + a;
	return r;
}

template <class RngEngine>
double CRandom<RngEngine>::GetExponential(double mean)
{
	double r = Get01();
	double l = -log(1. - r);
	return double(l)*mean;
}

template <class RngEngine>
double CRandom<RngEngine>::GetTriangular(double a, double b, double c)
{
	double r = Get01();
	double cdf = ((c - a)*(c - a)) / ((b - a)*(c - a));

	if (r < cdf)
		r = a + sqrt(r*(b - a)*(c - a));
	else
		r = b - sqrt((1. - r)*(b - a)*(b - c));
	return r;
}

template <class RngEngine>
double CRandom<RngEngine>::GetNormal(double mean, double std)
{
	int i;
	double r = 0.;
	for (i = 0; i < 12; i++)
		r += Get01();
	r -= 6.;
	return r*std + mean;
}

template <class RngEngine>
void CRandom<RngEngine>::GetNormalBivariate(double &r1, double &r2, double mean1, double std1, double mean2, double std2, double cov12)
{
	double a1 = GetNormal(0., 1.);
	double a2 = GetNormal(0., 1.);

	r1 = mean1 + std1*a1;

	double p = cov12 / (std1*std2);
	assert(p <= 1.);
	double c = sqrt(1. / (p*p) - 1.);

	double add = std2 / sqrt(1 + c*c)*(a1 + c*a2);
	r2 = mean2 + add;
}

template <class RngEngine>
double CRandom<RngEngine>::GetCustomDiscrete(int nCount, double *pfWeights, double *pfValues)
{
	if (nCount < 2) return 0.;

	double Wsum = 0.;

	int i;
	for (i = 0; i<nCount; i++)
		Wsum += pfWeights[i];

	double r = GetUniformInterval(0., Wsum);

	i = 0;
	while (r>pfWeights[i])
	{
		r -= pfWeights[i];
		i++;
	}

	return pfValues[i];
}

template <class RngEngine>
double CRandom<RngEngine>::GetCustomContinious(int nCount, double *pfWeights, double *pfValues)
{
	if (nCount < 2) return 0.;

	double Wsum = 0.;

	int i;
	for (i = 0; i<nCount - 1; i++)
		Wsum += pfWeights[i];

	double r = GetUniformInterval(0., Wsum);

	i = 0;
	while (r>pfWeights[i])
	{
		r -= pfWeights[i];
		i++;
	}
	double a = (pfWeights[i] - r) / pfWeights[i];
	r = pfValues[i] + (pfValues[i + 1] - pfValues[i])*a;

	return r;
}

#endif //_RNG_H_INCLUDED_2014_04_01
