#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include <algorithm>
#include <functional>
#include <stdexcept>
#include "stat.h"
#include "rng.h"

#pragma warning(disable : 4996)

#ifdef WIN32
#include <windows.h>
#else
#include <sys/time.h>
#endif 

static double GetTime()
{
#ifdef WIN32
	LARGE_INTEGER t, freq;
	QueryPerformanceCounter(&t);
	QueryPerformanceFrequency(&freq);

	return ((double)t.QuadPart)/((double)freq.QuadPart);
#else
	struct timeval t;
	gettimeofday (&t, (struct timezone *) 0);

	return ((double)t.tv_sec)+((double)t.tv_usec)/1.e+6;
#endif
} 

CStatistics::CStatistics()
{
	m_fMean = 0.;
	m_fMeanSquared = 0.;
	m_msg[0] = '\0';
	m_bSampleArraySorted = true;
}

CStatistics::~CStatistics()
{
}

CStatistics::CStatistics(double *pSamples, int nCount)
{
	m_fMean = 0.;
	m_fMeanSquared = 0.;
	m_msg[0] = '\0';
	m_bSampleArraySorted = true;
	SampleBatch(pSamples, nCount);
}

void CStatistics::Reset()
{
	m_fMean = 0.;
	m_fMeanSquared = 0.;
	m_msg[0] = '\0';
	m_arSamples.clear();
	m_bSampleArraySorted = true;
}


void CStatistics::Sample (double fSample)
{
	int nCount = GetCount();
	m_arSamples.push_back(fSample);
	m_bSampleArraySorted = false;

	if (nCount==0)
	{
		m_fMean = fSample;
		m_fMeanSquared = fSample*fSample;
	}
	else
	{
		double fCount = (double) nCount;
		m_fMean = (fSample + fCount*m_fMean)/(fCount+1.0);
		m_fMeanSquared = (fSample*fSample + fCount*m_fMeanSquared)/(fCount+1.0);
	}
}

void CStatistics::SampleBatch (double *pSamples, int nCount)
{
	assert(nCount>0);
	int nCurCount = GetCount();
	double fCurCount = double(nCurCount);
	double fNewCount = double(nCount);

	m_arSamples.resize(nCurCount + nCount);

	double fMean = 0.;
	double fMeanSq = 0.;
	
	for (int i=0; i<nCount; i++)
	{
		double fSample = pSamples[i];
		m_arSamples[i+nCurCount] = fSample;
		fMean += fSample;
		fMeanSq += fSample*fSample;
	}

	m_fMean = (m_fMean*fCurCount + fMean*fNewCount)/(fCurCount+fNewCount);
	m_fMeanSquared = (m_fMeanSquared*fCurCount + fMeanSq*fNewCount)/(fCurCount+fNewCount);
}

double CStatistics::GetStatistic(TStatistic eStat)
{
	double fRet = 0.;
	switch (eStat)
	{
	case MEAN:
		fRet = GetMean();
		break;
	case STDEV:
		fRet = GetStDev();
		break;
	case VARIANCE:
		fRet = GetVariance();
		break;
	case CVARIANCE:
		fRet = GetC();
		break;
	default:
		throw std::runtime_error("MakeBootstrapStatistic: Unexpected value TStatistic");
	}
	return fRet;
}

double CStatistics::GetMedian() const
{
	int nCount = GetCount();
	if (nCount<1)
		return 0.;
	EnsureArrayIsSorted();
	if (nCount % 2 == 0)
		return 0.5*(m_arSamples[nCount/2-1]+m_arSamples[nCount/2]);
	else
		return m_arSamples[nCount/2];
}

double CStatistics::GetVariance() const
{
	double fCount = double(GetCount());
	if (fCount<3.) return 0.;
	double fVariance = m_fMeanSquared - m_fMean*m_fMean;
	fVariance = fVariance*(fCount/(fCount-1));
	return fVariance;
}

double CStatistics::GetStDev() const
{
	return ::sqrt(GetVariance());
}

double CStatistics::GetMeanStErr() const
{
	double fCount = double(GetCount());
	assert (fCount>2.);

	return sqrt(GetVariance()/fCount);
}

double CStatistics::GetC() const
{
	if (m_fMean<=0.)
		return 0.;
	double fStdDev = GetStDev();
	return fStdDev/m_fMean;
}

double CStatistics::GetNormalityTest() const
{
	double fMedian = GetMedian();
	double fDif = fabs(m_fMean - fMedian);
	double fMax = fabs(m_fMean) > fabs(fMedian) ? fabs(m_fMean) : fabs(fMedian);
	return fDif / fMax;
}

double CStatistics::GetPercentile(float fPercentile)
{
	int nCount = GetCount();
	if (nCount<1 || fPercentile<0. || fPercentile>100.)
		return 0.;
	int nIndex = int ( float(GetCount())*(fPercentile/100.) + 0.5f );
	assert(nIndex>=0 && nIndex<nCount);
	EnsureArrayIsSorted();
	return m_arSamples[nIndex];
}

void CStatistics::EnsureArrayIsSorted() const
{
	if (m_arSamples.size()>0 && !m_bSampleArraySorted)
	{
		std::sort(m_arSamples.begin(), m_arSamples.end());
		m_bSampleArraySorted = true;
	}
}

double CStatistics::GetMinValue()
{
	int nCount = GetCount();
	if (nCount==0)
		return 0.;
	EnsureArrayIsSorted();
	return m_arSamples[0];
}

double CStatistics::GetMaxValue()
{
	int nCount = GetCount();
	if (nCount==0)
		return 0.;
	EnsureArrayIsSorted();
	return m_arSamples[nCount-1];
}

void CStatistics::PrintHistogramm(int nWidth/*=60*/, int nIntervals/*=10*/, FILE *file/*=stdout*/)
{
	int nCount = (int) m_arSamples.size();
	if (nIntervals<2 || nIntervals*2>nCount)
		return;	
	EnsureArrayIsSorted();

	double fMin = GetMinValue();
	double fMax = GetMaxValue();
	if (fMax-fMin<10e-10)
		return;

	double fWidth = double(nWidth);
	std::vector<double> arHeights(nIntervals, 0.);

	double fStep = (fMax-fMin)/double(nIntervals);
	int i,j,nSample=0, nMaxCnt = 0;
	for (i=0; i<nIntervals; i++)
	{
		int cnt=0;
		double fBigger = fMin + double(i+1)*fStep;
		while (nSample<nCount && m_arSamples[nSample]<=fBigger)
		{
			nSample++;
			cnt++;
		}
		if (i==nIntervals-1 && nSample<nCount)
			cnt += nSample-nCount;
		arHeights[i] = double(cnt);
		if (cnt>nMaxCnt) nMaxCnt = cnt;
		if (nSample>=nCount)
			break;
	}

	for (i=0; i<nIntervals; i++)
	{
		int cnt=0;
		double fI0 = fMin + double(i)*fStep;
		double fI1 = fMin + double(i+1)*fStep;
		fprintf(file, "%10g ..%10g |%3d|", fI0, fI1, int(arHeights[i]));

		int nStars = int((arHeights[i]/double(nMaxCnt))*fWidth+0.5);
		for (j=0; j<nStars; j++)
			fprintf(file, "*");
		fprintf(file, "\n");
	}
}

const char *CStatistics::GetStatictics()
{
	strcpy(m_msg,"");
	int nCount = GetCount();
	double fCount = (double) nCount;
	if (nCount<1) return m_msg;

	double fStDev = GetStDev();
	double fSumMean = m_fMean*fCount;
	double fSumStDev = fStDev*::sqrt(double(fCount));
	double i1 = fSumMean - 2.*fSumStDev;
	double i2 = fSumMean + 2.*fSumStDev;
	sprintf(m_msg, "Count=%d, Min=%g, Max=%g\nAvg=%g, Med.=%g Std=%g, C=%g\nAvg.Total=%g, Std.Total=%g\n95%%Conf.: %g - %g\n", 
		nCount, GetMinValue(), GetMaxValue(), GetMean(), GetMedian(), fStDev, GetC(), fSumMean, fSumStDev, i1, i2);
	return m_msg;
}

int CStatistics::MakeBootstrapSample(const CStatistics &oStat, CRng &rng)
{
	assert(oStat.GetCount()>3);

	Reset();

	for (int i=0; i<oStat.GetCount(); i++)
	{
		int nRanomSampleIndex = rng.GetUniformInterval(0, oStat.GetCount()-1);
		Sample(oStat.GetSample(nRanomSampleIndex));
	}

	return 0;
}

int CStatistics::MakeBootstrapStatistic(TStatistic eStat, const CStatistics &oStat, int nGenSamples/*=1000*/, CRng *_pRNG/*=NULL*/)
{
	CRng rng0;
	CRng *pRnd;

	assert(nGenSamples>10);
	if (nGenSamples<3) return -1;
	Reset();

	// Setting up the a random number generator
	if (_pRNG!=NULL)
	{
		// Use specified PRNG
		pRnd = _pRNG;
	}
	else
	{
		// Use default PRNG
		rng0.Seed();
		pRnd = &rng0;		
	}

	// Generate samples
	CStatistics oGen;
	for (int i=0; i<nGenSamples; i++)
	{
		oGen.MakeBootstrapSample(oStat, *pRnd);
		Sample(oGen.GetStatistic(eStat));
	}

	return 0;
}


int CStatistics::RunMicrobenchmark(FnBenchKernel fnKernel, int nSampleSize/*=30*/, double fWarmupSeconds/*=3.*/, int nNumOfOperations/*=0*/ )
{
	if (nSampleSize<2 || fnKernel==NULL)
		return -1;

	Reset();

	int n = nSampleSize;
	double fTime0 = GetTime();
	double fTime1 = GetTime();

	if (fWarmupSeconds>0.)
	{
		while (fTime1-fTime0<fWarmupSeconds)
		{
			fnKernel();
			fTime1 = GetTime();
		}
	}
	
	std::vector<double> arSample(n);
	int i;
	for (i=0; i<n; i++)
	{
		fTime0 = GetTime();
		fnKernel();
		fTime1 = GetTime();
		Sample(fTime1-fTime0);
	}
	return 0;
}

float CStatistics::Test_CompareMeans(const CStatistics &st)
{
	if (GetCount()<3. || st.GetCount()<3.)
		return 0.;

	double fN1 = double(GetCount());
	double fN2 = double(st.GetCount());
	double fVariance1 = GetVariance();
	double fVariance2 = st.GetVariance();
	float fStat = (float) ( fabs(GetMean()-st.GetMean()) / sqrt(fVariance1/fN1 + fVariance2/fN2) );
	int df = GetCount()+st.GetCount()-2;

	float fP0 = 0.f;
	if (df < 40)
	{
		if (fStat>50.) return 0.;
		fP0 = CDF_Student(fStat, df);
	}
	else
	{
		if (fStat>8.) return 0.;
		fP0 = CDF_StdNormal(fStat);
	}

	//printf("df=%d, fP0=%g, t=%g\n", df, fP0, fStat);
			
	return (1.f-fP0)*2.f;
}

float CStatistics::Test_CompareMedians(const CStatistics &st)
{
	if (GetCount() < 3. || st.GetCount() < 3.)
		return 0.;

	double fN1 = double(GetCount());
	double fN2 = double(st.GetCount());
	double fVariance1 = GetVariance();
	double fVariance2 = st.GetVariance();
	float fStat = (float)(fabs(GetMedian() - st.GetMedian()) / sqrt(fVariance1 / fN1 + fVariance2 / fN2));
	int df = GetCount() + st.GetCount() - 2;

	float fP0 = 0.f;
	if (df < 40)
	{
		if (fStat > 50.) return 0.;
		fP0 = CDF_Student(fStat, df);
	}
	else
	{
		if (fStat > 8.) return 0.;
		fP0 = CDF_StdNormal(fStat);
	}

	//printf("df=%d, fP0=%g, t=%g\n", df, fP0, fStat);

	return (1.f - fP0)*2.f;
}

float CStatistics::Test_CompareMeans_Outliers(const CStatistics &st)
{
	if (GetCount()<5 || st.GetCount()<5)
		return Test_CompareMeans(st);

	CStatistics st1, st2;
	double fThreshold1 = 2.*GetStDev();
	double fThreshold2 = 2.*st.GetStDev();
	double fMean1 = GetMean();
	double fMean2 = st.GetMean();

	int i;
	for (i=0; i<GetCount(); i++)
	{
		double val = GetSample(i);
		if (fabs(val-fMean1)<fThreshold1)
			st1.Sample(val);
	}
	for (i=0; i<st.GetCount(); i++)
	{
		double val = st.GetSample(i);
		if (fabs(val-fMean2)<fThreshold2)
			st2.Sample(val);
	}

	if (st1.GetCount()<5 || st2.GetCount()<5 || float(st1.GetCount())<float(GetCount())*0.9f ||
		float(st2.GetCount())<float(st.GetCount())*0.9f)
		return Test_CompareMeans(st);

	//printf("st1=%d, st2=%d\n", st1.GetCount(), st2.GetCount());
	
	return st1.Test_CompareMeans(st2);
}

float CStatistics::CDF_StdNormal(float x)
{
    // constants
    double a1 =  0.254829592;
    double a2 = -0.284496736;
    double a3 =  1.421413741;
    double a4 = -1.453152027;
    double a5 =  1.061405429;
    double p  =  0.3275911;

    // Save the sign of x
    int sign = 1;
    if (x < 0)
        sign = -1;
    double x1 = fabs(x)/sqrt(2.0);

    // A&S formula 7.1.26
    double t = 1.0/(1.0 + p*x1);
    double y = 1.0 - (((((a5*t + a4)*t) + a3)*t + a2)*t + a1)*t*exp(-x1*x1);

    return float(0.5*(1.0 + sign*y));
}

// ALGORITHM AS 3  APPL. STATIST. (1968) VOL.17, P.189
float CStatistics::CDF_Student(float t, int df)
{
	const float PI = 3.1415926535f;
	const float G1 = 1.f/PI;
	float A, B, C, F, S, FK;
	int   IM2,IOE,K,KS;

	if (df < 1) return -1.f;
	F = 1.f*df;
	A = (float) (t / sqrt(F));
	B = F / (F + t * t);
	IM2 = df - 2;
	IOE = df % 2;
	S = 1.f;
	C = 1.f;
	F = 1.f;
	KS = 2 + IOE;
	FK = (float) KS;
	if (IM2 < 2) goto e20;
    K=KS;
    
	while (K <= IM2)
	{
		C = (float) (C * B * (FK - 1.f) / FK);
		S = S + C;
		if (S == F) goto e20;
		F = S;
		FK += 2.f;
		K += 2;
	}
e20:
	if (IOE == 1) goto e30;
    return (float) (0.5 + 0.5 * A * sqrt(B) * S);
e30:
	if (df == 1) S = 0.f;
	return (float) (0.5f + (A * B * S + atan(A)) * G1);
}

int ResampleTableFunction (const std::vector<std::pair<double, double> > &arInput, int nCount,
					  std::vector<std::pair<double, double> > &arOutput, bool bUseAbsErr/*=false*/)
{
	int nCurCount = (int) arInput.size();
	arOutput = arInput;

	while (nCurCount>nCount)
	{
		// Remove one point
		int nPoint = -1;
		double fMaxErr = 0.;
		for (int i=1; i<(int)arOutput.size()-1; i++)
		{
			// Calc error without this point
			double fErr=0.;
			for (int j0=1,j1=1; j0<(int)arInput.size()-1; j0++)
			{
				while (arOutput[j1].first<arInput[j0].first) j1++;

				double X0 = arOutput[j1-1].first;
				double Y0 = arOutput[j1-1].second;
				if (j1==i) j1++;
				if (arOutput[j1].first == arInput[j0].first) continue;
				double X1 = arOutput[j1].first;
				double Y1 = arOutput[j1].second;
				double X = arInput[j0].first;
				double Y_ = ((Y1-Y0)/(X1-X0)) * (X-X0) + Y0;
				double Y = arInput[j0].second;
				double fErrAbs = (Y_ - Y)*(Y_ - Y);
				if (!bUseAbsErr && fabs(Y)<1e-8) Y=1e-8;
				if (bUseAbsErr || Y == 0.)
					fErr += fErrAbs;
				else
					fErr += fErrAbs/fabs(Y);
			}

			if (nPoint<0 || fErr<fMaxErr)
			{
				nPoint = i;
				fMaxErr = fErr;
				if (fMaxErr==0.) break;
			}
		}
		if (nPoint<0)
			return -1;

		arOutput.erase(arOutput.begin()+nPoint);
		nCurCount--;
		printf("%d (%g, pt=%d)\n", nCurCount, fMaxErr, nPoint);
	}
	return 0;
}

int ResampleFunction (fnUnaryFunction &fn, double fStart, double fStop, int nInitialSamples,
					  int nNumSamples, std::vector<std::pair<double, double> > &arOutput, bool bUseAbsErr/*=false*/)
{
	assert(fStop > fStart);
	assert(nInitialSamples > 3);
	assert(nNumSamples > 2);

	double fStep = (fStop-fStart)/double(nInitialSamples-1);

	if (nInitialSamples<nNumSamples || nInitialSamples<5 || nNumSamples<3) return -1;

	std::vector<std::pair<double, double> > arInput;

	arInput.reserve(nInitialSamples);
	double fX = 0.;
	int j=0;
	while (fX<=fStop)
	{
		fX = fStart + fStep*double(j);
		double fY = fn(fX);
		arInput.push_back( std::make_pair(fX, fY) );
		j++;
	}

	return ResampleTableFunction(arInput, nNumSamples, arOutput, bUseAbsErr);
}
