#include <math.h>
#include <time.h>
#include <assert.h>
#include "rng.h"

//----------------------------------------------- ARC4 ---------------------------------------------------

#define swap_byte(x,y) t = *(x); *(x) = *(y); *(y) = t

void CRng::_init( const char *key, size_t keylen )
{
    int i, j, a;
    unsigned int k;
    unsigned char *m;

    _x = 0;
    _y = 0;
    m = _m;

    for( i = 0; i < 256; i++ )
        m[i] = (unsigned char) i;

    j = k = 0;

    for( i = 0; i < 256; i++, k++ )
    {
        if( k >= keylen ) k = 0;

        a = m[i];
        j = ( j + a + key[k] ) & 0xFF;
        m[i] = m[j];
        m[j] = (unsigned char) a;
    }
}

int CRng::_rng ( char *data, size_t len )
{
    int x, y, a, b;
    size_t i;
    unsigned char *m;

    x = _x;
    y = _y;
    m = _m;

    for( i = 0; i < len; i++ )
    {
        x = ( x + 1 ) & 0xFF; a = m[x];
        y = ( y + a ) & 0xFF; b = m[y];

        m[x] = (unsigned char) b;
        m[y] = (unsigned char) a;

        //data[i] = (unsigned char) ( data[i] ^ m[(unsigned char)( a + b )] );
		data[i] = (unsigned char) ( m[(unsigned char)( a + b )] );
    }

    _x = x;
    _y = y;

    return( 0 );
}

//----------------------------------------------- CRng ---------------------------------------------------

CRng::CRng()
{
}

CRng::~CRng()
{
}

//Seed by system timer
void CRng::Seed()
{
	unsigned int i = (unsigned int)time(NULL);
	Seed((const char *)&i, 4);
}

//Seed by a 32-bit integer
void CRng::Seed(unsigned int seed)
{
	Seed((const char *)&seed, 4);
}

//Seed by a arbitrary buffer
void CRng::Seed(const char *pBuf, int buflen)
{
	_init(pBuf, buflen);
}

double CRng::Get01()
{
	unsigned int i = 0;
	unsigned char c;
	_rng((char *)&c, 1);	i = c;
	_rng((char *)&c, 1);	i |= ((unsigned int)c << 8);
	_rng((char *)&c, 1);	i |= ((unsigned int)c << 16);
	_rng((char *)&c, 1);	i |= ((unsigned int)c << 24);
	return i*(1.0/4294967295.0); 
}

int CRng::GetUniformInterval(int a, int b)
{
	unsigned char c;
	int s = b-a+1;
	unsigned int r_num;

	if (s==0) return a;
	bool bExit = false;
	while (!bExit)
	{
		if (s<65536)
		{
			_rng((char *)&c, 1);
			r_num = c;
			_rng((char *)&c, 1);
			r_num |= ((unsigned int)c << 8);
			// Drop residual numbers to avoid bias from uniform distribution
			if ( (int)r_num > (65536-(65536%s)) )
				continue;
		}
		else
		{
			_rng((char *)&c, 1);
			r_num = c;
			_rng((char *)&c, 1);
			r_num |= ((unsigned int)c << 8);
			_rng((char *)&c, 1);
			r_num |= ((unsigned int)c << 16);
			_rng((char *)&c, 1);
			r_num |= ((unsigned int)c << 24);
			// Drop residual numbers to avoid bias from uniform distribution
			if ( r_num > ( 4294967296-(4294967296%s)) )
				continue;
		}
		bExit = true;
	}

	return (r_num % s) + a;
}

double CRng::GetUniformInterval(double a, double b)
{
	double s = b-a;
	double r = Get01()*s + a;
	return r;
}

double CRng::GetExponential(double mean)
{
	double r = Get01();
	double l = - log(1.-r);
	return double(l)*mean;
}

double CRng::GetTriangular(double a, double b, double c)
{
	double r = Get01();
	double cdf = ((c-a)*(c-a)) / ((b-a)*(c-a));

	if (r<cdf)
		r = a+sqrt(r*(b-a)*(c-a));
	else
		r = b-sqrt((1.-r)*(b-a)*(b-c));
	return r;
}

double CRng::GetNormal(double mean, double std)
{
	int i;
	double r=0.;
	for (i=0; i<12; i++)
		r+=Get01();
	r-=6.;
	return r*std+mean;
}

void CRng::GetNormalBivariate(double &r1, double &r2, double mean1, double std1, double mean2, double std2, double cov12)
{
	double a1 = GetNormal(0., 1.);
	double a2 = GetNormal(0., 1.);
	
	r1 = mean1 + std1*a1;

	double p = cov12/(std1*std2);
	assert(p<=1.);
	double c = sqrt(1./(p*p)-1.);
	
	double add = std2/sqrt(1+c*c)*(a1+c*a2);
	r2 = mean2 + add;
}

double CRng::GetCustomDiscrete(int nCount, double *pfWeights, double *pfValues)
{
	if (nCount<2) return 0.;

	double Wsum = 0.;

	int i;
	for (i=0; i<nCount; i++)
		Wsum += pfWeights[i];		

	double r = GetUniformInterval(0., Wsum);

	i=0;
	while (r>pfWeights[i])
	{
		r -= pfWeights[i];
		i++;
	}

    return pfValues[i];
}

double CRng::GetCustomContinious(int nCount, double *pfWeights, double *pfValues)
{
	if (nCount<2) return 0.;

	double Wsum = 0.;

	int i;
	for (i=0; i<nCount-1; i++)
		Wsum += pfWeights[i];		

	double r = GetUniformInterval(0., Wsum);

	i=0;
	while (r>pfWeights[i])
	{
		r -= pfWeights[i];
		i++;
	}
	double a  = (pfWeights[i]-r)/pfWeights[i];
	r = pfValues[i] + (pfValues[i+1]-pfValues[i])*a;

    return r;
}

