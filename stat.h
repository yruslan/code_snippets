#ifndef _STAT_H_INCLUDED_2014_03_21
#define _STAT_H_INCLUDED_2014_03_21

#include <stdio.h>
#include <vector>

typedef void (*FnBenchKernel)(void);

class CRng;

class CStatistics
{
public:
	enum TStatistic { MEAN=0, MEDIAN, STDEV, VARIANCE, CVARIANCE };

public:
	CStatistics();
	CStatistics(double *pSamples, int nCount);
	virtual ~CStatistics();

	void Reset();

	void Sample (double sample);                      // Account for another sample
	void SampleBatch (double *pSamples, int nCount);  // Account for many samples at time

	operator const double * () {return &m_arSamples[0];};
	double GetSample(int n) const { return m_arSamples[n];};
	double GetStatistic(TStatistic eStat);
	double GetMean() const {return m_fMean;};
	double GetMeanStErr() const;
	double GetStDev() const;
	double GetVariance() const;
	double GetMedian() const;

	// Get coefficient of variance (0 -> deterministic, 1 -> memoryless
	double GetC() const;

	// Get normality test coefficient. Must be close to 0 for normal distribution (<0.01).
	double GetNormalityTest() const;

	double GetPercentile(float fPercentile);

	double GetMinValue();
	double GetMaxValue();
	int GetCount() const {return (int)m_arSamples.size(); };

	void PrintHistogramm(int nWidth=60, int nIntervals=10, FILE *file=stdout);
	const char *GetStatictics();

	//// Bootstrap

	// Generate a single bootstrap sample
	int MakeBootstrapSample(const CStatistics &oStat, CRng &rng);
	
	// Generate a series of bootatrap samples for a specified statistic 
	int MakeBootstrapStatistic(TStatistic eStat, const CStatistics &oStat, int nGenSamples=1000, CRng *pRNG=NULL);

	//// Microbenchmark

	int RunMicrobenchmark(FnBenchKernel fnKernel, int nSampleSize=30, double fWarmupSeconds=3., int nNumOfOperations=0 );

	//// Statistical tests
	
	// T-test. Compare means (nested experiment). Returns significance of the difference.
	// Close to 1 mean the effect is real
	// Note. Number of samples in both sets should be >3 in each group
	// Returns -1 on error
	float Test_CompareMeans(const CStatistics &st);

	// T-test as the previous one. But in this tests outliers are automatically
	// removed increasing consistancy of the test.
	float Test_CompareMeans_Outliers(const CStatistics &st);

	//
	// The number of samples in each group must match
	float Test_TTestCrossed(const CStatistics &st);

	static float CDF_Student(float t, int df);
	static float CDF_StdNormal(float x);

private:
	void EnsureArrayIsSorted() const;

	mutable bool m_bSampleArraySorted;
	mutable std::vector<double> m_arSamples;
	double m_fMean;
	double m_fMeanSquared;
	char m_msg[255];
};

int ResampleTableFunction (const std::vector<std::pair<double, double> > &arInput, int nCount,
					      std::vector<std::pair<double, double> > &arOutput, bool bUseAbsErr=false);

typedef double(fnUnaryFunction)(double);
int ResampleFunction (fnUnaryFunction &fn, double fStart, double fStop, int nInitialSamples,
					  int nNumSamples, std::vector<std::pair<double, double> > &arOutput, bool bUseAbsErr=false);


#endif //_STAT_H_INCLUDED_2014_03_21
