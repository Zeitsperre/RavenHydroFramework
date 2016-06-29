/*----------------------------------------------------------------
  Raven Library Source Code
  Copyright � 2008-2014 the Raven Development Team
----------------------------------------------------------------*/

#ifndef RADIATION_H
#define RADIATION_H

#include "RavenInclude.h"
#include "HydroUnits.h"

///////////////////////////////////////////////////////////////////
/// \brief Utility class for radiation calculations
//
class CRadiation
{  
  private:/*------------------------------------------------------*/
    static double RadCorr                   (const double declin,		  //declination, radians
								                             const double solar_noon, //time of solar noon, days
								                             const double lat_eq,     //equivalent latitude, radians
								                             const double t_sunrise,  //time of sunrise, days before solar noon
								                             const double t_sunset,   //time of sunrise, days after solar noon,
								                             const double t_sol,			//time of day w.r.t. solar noon, days (not adjusted for slope)
								                             const bool   avg_daily);	//

    static double CalcScatteringTransmissivity(const double dew_pt,//dew point temp, [C]
																		         const double opt_air_mass);//[-]  
    static double CalcDiffScatteringTransmissivity(const double dew_pt,//dew point temp, [C]
									            							 const double opt_air_mass);//[-]
    static double MassFunct                 (const double latrad, const double declin, const double cos_omt,
                                             const double c1,const double c2, const double c3,const double t);
    static double UBC_SolarRadiation        (const double lat,   //[rad]
                                             const double elev,   //[masl]
                                             const double day_angle,
                                             const double day_length,
                                                   double &ET_radia );  //[MJ/m2/d]
	
  public:/*------------------------------------------------------*/

    static double DayAngle                  (const double&day,
                                             const int    year);
    static double DayLength                 (const double lat,    //latitude (in radians)
                                             const double declin); //solar declination (in radians)
    static double SolarDeclination          (const double day_angle);
    static double SolarZenithAngle          (const double lat_eq,//equivalent latitude, radians 
                                             const double declin,//solar declination, radians
                                             const double t);    //absolute time, days
    static double EccentricityCorr          (const double day_angle);

    static double EstimateLongwaveRadiation (const optStruct    &Options,
                                             const force_struct *F,
                                             const CHydroUnit   *pHRU);
    static double CalcETRadiation           (const double latrad,//latitude in radians
                                             const double lateq,//equivalent latitude for slope
                                             const double declin,//declination in radians
                                             const double ecc,   //eccentricity correction [-]
                                             const double slope, //in radians
                                             const double solar_noon, //effective solar noon correction for slope [days]
                                             const double day_length, // length of day (not corrected for slope) [days]
                                             const double t_sol, //time of day w.r.t. solar noon [days] [-0.5..0.5]
                                             const bool   avg_daily);//true if avg_daily is to be calculated
    static double ClearSkySolarRadiation    (const double julian_day,  
                                             const double latrad,   //[rad]
                                             const double lateq, //[rad]
                                             const double slope, //[rad]
                                             const double day_angle,
                                             const double day_length,
                                             const double solar_noon,//[days]
                                             const double dew_pt,//dew point temp, [C]
                                                   double &ET_radia, //ET radiation [MJ/m2/d]
                                             const bool   avg_daily);  //true if average daily is to be computed
    static double EstimateShortwaveRadiation(const optStruct &Options,
                                             const force_struct *F,
                                             const CHydroUnit *pHRU,
                                             const time_struct  &tt,
                                                   double       &ET_rad);

	  static double SWCloudCoverCorrection    (const optStruct &Options,
                                             const force_struct *F);

    static double SWCanopyCorrection        (const optStruct &Options,
											                       const CHydroUnit *pHRU);

   

    static double OpticalAirMass            (const double latrad,//in radians
											                       const double declin,//in radians
                                             const double day_length, // [days]
											                       const double t_sol, //time of day w.r.t. solar noon [days] [-0.5..0.5]
											                       const bool   avg_daily);//in radians

};
#endif