/*----------------------------------------------------------------
  Raven Library Source Code
  Copyright � 2008-2015 the Raven Development Team
----------------------------------------------------------------*/
#ifndef _DIAGNOSTICS_H
#define _DIAGNOSTICS_H

#include "RavenInclude.h"
#include "TimeSeries.h"

enum diag_type {
	DIAG_NASH_SUTCLIFFE,
	DIAG_RMSE,
	DIAG_PCT_BIAS,
	DIAG_ABSERR,
	DIAG_ABSMAX,
	DIAG_PDIFF,
	DIAG_TMVOL,
	DIAG_RCOEF,
	DIAG_NSC,
	DIAG_RSR,
  DIAG_R2,
  DIAG_CUMUL_FLOW,
  DIAG_LOG_NASH,
  DIAG_KLING_GUPTA,
  DIAG_NASH_SUTCLIFFE_DER,
  DIAG_RMSE_DER,
  DIAG_KLING_GUPTA_DER,
};

///////////////////////////////////////////////////////////////////
/// \brief Data abstraction for time series comparison diagnostics
class CDiagnostic
{ 
  private:/*------------------------------------------------------*/

		diag_type     _type;    ///< output file stream

  public:/*------------------------------------------------------*/

    CDiagnostic(const diag_type  typ);
		~CDiagnostic();

		string GetName() const;

		double CalculateDiagnostic(CTimeSeriesABC *pTSmod, CTimeSeriesABC *pTSObs, CTimeSeriesABC *pTSWeights, const optStruct &Options) const;
};
#endif