#include <math.h>
#include <assert.h>
#include <memory.h>
#include "rng.h"

//----------------------------------------------- ARC4 ---------------------------------------------------

#define swap_byte(x,y) t = *(x); *(x) = *(y); *(y) = t

CRngEngineArc4::CRngEngineArc4()
	: x_(0)
	, y_(0)
{
	memset(m_, 0, 256);
}

void CRngEngineArc4::Init(const char *pBuf, int buflen)
{
	int i, j, a;
	unsigned int k;
	unsigned char *m;

	x_ = 0;
	y_ = 0;
	m = m_;

	for (i = 0; i < 256; i++)
		m[i] = (unsigned char)i;

	j = k = 0;

	for (i = 0; i < 256; i++, k++)
	{
		if ((int)k >= buflen) k = 0;

		a = m[i];
		j = (j + a + pBuf[k]) & 0xFF;
		m[i] = m[j];
		m[j] = (unsigned char)a;
	}
	return;
}

int CRngEngineArc4::Generate(char *data, size_t len)
{
	int x, y, a, b;
	size_t i;
	unsigned char *m;

	x = x_;
	y = y_;
	m = m_;

	for (i = 0; i < len; i++)
	{
		x = (x + 1) & 0xFF; a = m[x];
		y = (y + a) & 0xFF; b = m[y];

		m[x] = (unsigned char)b;
		m[y] = (unsigned char)a;

		data[i] = (unsigned char)(m[(unsigned char)(a + b)]);
	}

	x_ = x;
	y_ = y;

	return 0;
}

CRngEngineArc4Ex::CRngEngineArc4Ex()
	: x_(0)
	, y_(0)
{
	memset(m_, 0, 256);
}

void CRngEngineArc4Ex::Init(const char *pBuf, int buflen)
{
	if (buflen < 1)
		return;
	unsigned char exbuf[256];
	int i, j, a;
	unsigned int k;
	unsigned char *m;

	x_ = 0;
	y_ = 0;
	m = m_;
	const unsigned char *inBuf = (const unsigned char *)pBuf;
	if (buflen < 16)
	{
		inBuf = exbuf;
		buflen = 256;
		unsigned int s = pBuf[0];
		if (buflen > 1)
			s += (pBuf[1] << 8);
		if (buflen > 2)
			s += (pBuf[2] << 16);
		if (buflen > 3)
			s += (pBuf[3] << 24);
		for (i = 0; i < 256; i++)
		{
			s = (214013 * s + 2531011);
			exbuf[i] = (s >> 16) & 0xFF;
		}
	}

	for (i = 0; i < 256; i++)
		m[i] = (unsigned char)i;

	j = k = 0;

	for (i = 0; i < 256; i++, k++)
	{
		if ((int)k >= buflen) k = 0;

		a = m[i];
		j = (j + a + inBuf[k]) & 0xFF;
		m[i] = m[j];
		m[j] = (unsigned char)a;
	}

	// Skipping first 256 bytes
	Generate((char *)exbuf, 256);
	return;
}

int CRngEngineArc4Ex::Generate(char *data, size_t len)
{
	int x, y, a, b;
	size_t i;
	unsigned char *m;

	x = x_;
	y = y_;
	m = m_;

	for (i = 0; i < len; i++)
	{
		x = (x + 1) & 0xFF; a = m[x];
		y = (y + a) & 0xFF; b = m[y];

		m[x] = (unsigned char)b;
		m[y] = (unsigned char)a;

		data[i] = (unsigned char)(m[(unsigned char)(a + b)]);
	}

	x_ = x;
	y_ = y;

	return 0;
}
