#ifndef _RNG_H_INCLUDED_2014_04_01
#define _RNG_H_INCLUDED_2014_04_01

class CRng
{
public:
	CRng();
	~CRng();

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
	double GetTriangular(double a, double b, double c);
	double GetNormal(double mean, double std);
	void GetNormalBivariate(double &r1, double &r2, double mean1, double std1, double mean2, double std2, double cov12);
	double GetCustomDiscrete(int nCount, double *pfWeights, double *pfValues);
	double GetCustomContinious(int nCount, double *pfWeights, double *pfValues);

protected:
	void _init( const char *key, size_t len );
	int _rng ( char *data, size_t len );

	//RNG state
    int _x;
    int _y;
    unsigned char _m[256];
};

#endif //_RNG_H_INCLUDED_2014_04_01
