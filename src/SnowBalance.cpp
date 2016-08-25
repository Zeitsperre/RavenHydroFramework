/*----------------------------------------------------------------
  Raven Library Source Code
  Copyright � 2008-2014 the Raven Development Team
------------------------------------------------------------------
	Fully coupled snow balance routines
----------------------------------------------------------------*/
#include "HydroProcessABC.h"
#include "SnowMovers.h"

//////////////////////////////////////////////////////////////////
/// \brief   Implementation of the constructor
/// \details For combined modelling of melt, refreeze, and energy content
/// \param   bal_type Method of balancing energy selected
//
CmvSnowBalance::CmvSnowBalance(snowbal_type bal_type):
                CHydroProcessABC(SNOW_BALANCE)
{
  type =bal_type;

  int iSnow; //used by all snow balance routines
  iSnow   =pModel->GetStateVarIndex(SNOW);
  
  if (type==SNOBAL_SIMPLE_MELT)
  {
    int iPond=pModel->GetStateVarIndex(PONDED_WATER);

    CHydroProcessABC::DynamicSpecifyConnections(1);

    iFrom[0]=iSnow;  iTo[0]=iPond;
  }
  else if (type==SNOBAL_COLD_CONTENT)
  {
    int iSnowLiq,iCC,iAtmosEn,iSW;
    iSnowLiq=pModel->GetStateVarIndex(SNOW_LIQ);
    iCC     =pModel->GetStateVarIndex(COLD_CONTENT); 
    iAtmosEn=pModel->GetStateVarIndex(ENERGY_LOSSES);
    iSW     =pModel->GetStateVarIndex(SURFACE_WATER);
    
    CHydroProcessABC::DynamicSpecifyConnections(5);//nConnections=5
    
    iFrom[0]=iCC;       iTo[0]=iAtmosEn;//rates[0]: COLD_CONTENT->LOST_ENERGY
    iFrom[1]=iSnowLiq;  iTo[1]=iSnow;   //rates[1]: SNOW_LIQ->SNOW
    iFrom[2]=iAtmosEn;  iTo[2]=iCC;     //rates[2]: RADIATIVE_ENERGY->COLD_CONTENT
    iFrom[3]=iSnow;     iTo[3]=iSW;     //rates[3]: SNOW ->SURFACE_WATER
    iFrom[4]=iSnowLiq;  iTo[4]=iSW;     //rates[4]: SNOW_LIQ->SURFACE_WATER
  }
  else if (type==SNOBAL_HBV)
  {
    int iSnowLiq,iSnowDepth,iSoil;
    iSnowLiq  =pModel->GetStateVarIndex(SNOW_LIQ);
    iSnowDepth=pModel->GetStateVarIndex(SNOW_DEPTH);
    iSoil     =pModel->GetStateVarIndex(SOIL,0);

    CHydroProcessABC::DynamicSpecifyConnections(2); //nConnections=2
    
    iFrom[0]=iSnow;       iTo[0]=iSnowLiq;       //rates[0]: SNOW->SNOW_LIQ
    iFrom[1]=iSnowLiq;		iTo[1]=iSoil;          //rates[1]: SNOW_LIQ->?SOIL
  }
  else if (type==SNOBAL_UBCWM)
  {
    int iSnowLiq,iCumMelt,iPonded,iSnowCov,iColdCont;
    iSnowLiq  =pModel->GetStateVarIndex(SNOW_LIQ);
    iPonded   =pModel->GetStateVarIndex(PONDED_WATER);
    iSnowCov  =pModel->GetStateVarIndex(SNOW_COVER);
    iColdCont =pModel->GetStateVarIndex(COLD_CONTENT);
    iCumMelt  =pModel->GetStateVarIndex(CUM_SNOWMELT);

    int iSnowDef;
    iSnowDef  =pModel->GetStateVarIndex(SNOW_DEFICIT);

    CHydroProcessABC::DynamicSpecifyConnections(7); //nConnections=7
    
    iFrom[0]=iSnow;       iTo[0]=iSnowLiq;       //rates[0]: SNOW->SNOW_LIQ
    iFrom[1]=iSnowLiq;		iTo[1]=iPonded;        //rates[1]: SNOW_LIQ->PONDED
    iFrom[2]=iSnow;		    iTo[2]=iPonded;        //rates[2]: SNOW->PONDED
    iFrom[3]=iColdCont;		iTo[3]=iColdCont;      //rates[3]: CC modification
    iFrom[4]=iSnowCov;		iTo[4]=iSnowCov;       //rates[4]: snow cover modification
    iFrom[5]=iCumMelt;    iTo[5]=iCumMelt;       //rates[5]: cumulative melt modification
    iFrom[6]=iSnowDef;    iTo[6]=iSnowDef;       //rates[6]: snow deficit modification
  }
  else if (type==SNOBAL_CEMA_NIEGE)
  {
    int iSnowCov,iPonded;
    iSnowCov  =pModel->GetStateVarIndex(SNOW_COVER);
    iPonded   =pModel->GetStateVarIndex(PONDED_WATER);

    CHydroProcessABC::DynamicSpecifyConnections(2); //nConnections=2

    iFrom[0]=iSnow;		    iTo[0]=iPonded;            //rates[0]: SNOW->PONDED
    iFrom[1]=iSnowCov;		iTo[1]=iSnowCov;           //rates[1]: snow cover modification
  }
  else if(type==SNOBAL_TWO_LAYER)
  {
    int iSnowfall,iPonded,iSLSurf,iSLPack,iCCSurf,iCCPack,iSnowTemp,iCumMelt;
    iSnowfall =pModel->GetStateVarIndex(NEW_SNOW);
    iPonded   =pModel->GetStateVarIndex(PONDED_WATER);
    iSLSurf   =pModel->GetStateVarIndex(SNOW_LIQ,0);
    iSLPack   =pModel->GetStateVarIndex(SNOW_LIQ,1);
    iCCSurf   =pModel->GetStateVarIndex(COLD_CONTENT,0);
    iCCPack   =pModel->GetStateVarIndex(COLD_CONTENT,1);
    iSnowTemp =pModel->GetStateVarIndex(SNOW_TEMP);
    iCumMelt = pModel->GetStateVarIndex(CUM_SNOWMELT);

    CHydroProcessABC::DynamicSpecifyConnections(10); //nConnections=10
    
    iFrom[0]=iSnowfall;   iTo[0]=iSnow;      //rates[0]: SNOWFALL         -> SNOW
    iFrom[1]=iPonded;     iTo[1]=iSLSurf;    //rates[1]: Rain             -> SNOW_LIQ surface
    iFrom[2]=iSnow;	      iTo[2]=iSLSurf;    //rates[2]: SNOW             -> SNOW_LIQ surface  
    iFrom[3]=iSLSurf;	  iTo[3]=iSLPack;    //rates[3]: SNOW_LIQ surface -> pack
    iFrom[4]=iSLPack;	  iTo[4]=iSnow;      //rates[4]: SNOW_LIQ pack    -> SNOW
    iFrom[5]=iSLPack;	  iTo[5]=iPonded;    //rates[5]: SNOW_LIQ pack    -> ponded water
    iFrom[6]=iCCSurf;	  iTo[6]=iCCSurf;    //rates[6]: CC surface layer
    iFrom[7]=iCCPack;	  iTo[7]=iCCPack;    //rates[7]: CC pack layer
    iFrom[8]=iSnowTemp;	  iTo[8]=iSnowTemp;  //rates[8]: Snow Temp
    iFrom[9]=iCumMelt;    iTo[9]=iCumMelt;   //rates[9]: Cumulative Melt
  }
  else{
    ExitGracefully("CmvSnowBalance::Constructor: undefined snow balance type",BAD_DATA);
  }
}
//////////////////////////////////////////////////////////////////
/// \brief   Implementation of the constructor
/// \details For combined modelling of melt, refreeze, and energy content
/// \param   bal_type Method of balancing energy selected
/// \param   iSnowTo index of 'To' state variable (should be ponded water or snow_liq)
//
CmvSnowBalance::CmvSnowBalance(snowbal_type bal_type, int iSnowTo) :
                         CHydroProcessABC(SNOW_BALANCE)
{
  type = bal_type;

  int iSnow; //used by all snow balance routines
  iSnow = pModel->GetStateVarIndex(SNOW);

  if (type == SNOBAL_SIMPLE_MELT)
  {
    sv_type typ=pModel->GetStateVarType(iSnowTo);
    if ((typ != PONDED_WATER) && (typ != SNOW_LIQ)){
      ExitGracefully("CmvSnowBalance Constructor: SNOBAL_SIMPLE_MELT target should be either PONDED_WATER or SNOW_LIQ",BAD_DATA_WARN);
    }
    CHydroProcessABC::DynamicSpecifyConnections(1);
    iFrom[0] = iSnow;  iTo[0] = iSnowTo;
  }
  else{
    ExitGracefully("CmvSnowBalance::Constructor: incorrect constructor for this type.",RUNTIME_ERR);
  }
}
//////////////////////////////////////////////////////////////////
/// \brief Implementation of the default destructor
//
CmvSnowBalance::~CmvSnowBalance(){} 

//////////////////////////////////////////////////////////////////
/// \brief Initializes snow balance modelling object
//
void CmvSnowBalance::Initialize(){}

//////////////////////////////////////////////////////////////////
/// \brief Returns participating parameter list
///
/// \param *aP [out] array of parameter names needed for snow balance algorithm
/// \param *aPC [out] Class type (soil, vegetation, landuse or terrain) corresponding to each parameter
/// \param &nP [out] Number of parameters required by snow balance algorithm (size of aP[] and aPC[])
//
void CmvSnowBalance::GetParticipatingParamList(string *aP, class_type *aPC, int &nP) const
{  
  if (type==SNOBAL_SIMPLE_MELT)
  {
    nP=0;
  }
  else if (type==SNOBAL_COLD_CONTENT)
  {
    nP=1;
	  aP[0]="SNOW_SWI";		     aPC[0]=CLASS_GLOBAL; 
  }
  else if (type==SNOBAL_HBV)
  {
    nP=3; 
    aP[0]="REFREEZE_FACTOR"; aPC[0]=CLASS_LANDUSE; 
    aP[1]="MELT_FACTOR";     aPC[1]=CLASS_LANDUSE; 
	  aP[2]="SNOW_SWI";		     aPC[2]=CLASS_GLOBAL; 
  }
  else if (type==SNOBAL_UBCWM)
  {
    nP=3; 
    aP[0]="CC_DECAY_COEFF";   aPC[0]=CLASS_LANDUSE; 
	  aP[1]="SNOW_SWI";		      aPC[1]=CLASS_GLOBAL; 
	  aP[2]="SNOW_PATCH_LIMIT"; aPC[2]=CLASS_LANDUSE;
  }
  else if (type==SNOBAL_CEMA_NIEGE)
  {
    nP=1;
    aP[0]="AVG_ANNUAL_SNOW";  aPC[0]=CLASS_GLOBAL;
  }
  else if (type==SNOBAL_TWO_LAYER)
  {
    nP=2;
    aP[0]="MAX_SWE_SURFACE";	aPC[0]=CLASS_GLOBAL; 
    aP[1]="SNOW_SWI";		      aPC[1]=CLASS_GLOBAL; 
  }
  else
  {
    ExitGracefully("CmvSnowBalance::GetParticipatingParamList: undefined snow balance algorithm",BAD_DATA);
  }
}


//////////////////////////////////////////////////////////////////
/// \brief Sets reference to participating state variables
///
/// \param bal_type [in] Snow energy/mass balance model selected
/// \param *aSV [out] Array of state variable types needed by energy balance algorithm
/// \param *aLev [out] Array of level of multilevel state variables (or DOESNT_EXIST, if single level)
/// \param &nSV [out] Number of state variables required by energy balance algorithm (size of aSV[] and aLev[] arrays)
//
void CmvSnowBalance::GetParticipatingStateVarList(snowbal_type bal_type,
																								  sv_type *aSV, int *aLev, int &nSV) 
{
	if (bal_type==SNOBAL_SIMPLE_MELT)
	{
    nSV=1;
    aSV[0]=SNOW;          aLev[0]=DOESNT_EXIST;
    ////user specified 'to' compartment (ponded water or snow_liq)
  }
  else if (bal_type==SNOBAL_COLD_CONTENT)
	{
    nSV=5;
    aSV[0]=SNOW_LIQ;      aLev[0]=DOESNT_EXIST;
    aSV[1]=COLD_CONTENT;  aLev[1]=DOESNT_EXIST;
    aSV[2]=ENERGY_LOSSES; aLev[2]=DOESNT_EXIST;
    aSV[3]=SURFACE_WATER; aLev[3]=DOESNT_EXIST;
    aSV[4]=SNOW;          aLev[4]=DOESNT_EXIST;
  }
  else if (bal_type==SNOBAL_UBCWM)
  {
    nSV=8;
    aSV[0]=SNOW;          aLev[0]=DOESNT_EXIST;
    aSV[1]=SNOW_LIQ;      aLev[1]=DOESNT_EXIST;
    aSV[2]=PONDED_WATER;  aLev[2]=DOESNT_EXIST;
    aSV[3]=COLD_CONTENT;  aLev[3]=DOESNT_EXIST;
    aSV[4]=SNOW_COVER;    aLev[4]=DOESNT_EXIST;
    aSV[5]=CUM_SNOWMELT;  aLev[5]=DOESNT_EXIST;
    aSV[6]=SNOW_ALBEDO;   aLev[6]=DOESNT_EXIST;//Used, but not modified
    aSV[7]=SNOW_DEFICIT;  aLev[7]=DOESNT_EXIST;
  }
  else if (bal_type==SNOBAL_CEMA_NIEGE)
  {
    nSV=3;
    aSV[0]=SNOW;          aLev[0]=DOESNT_EXIST;
    aSV[1]=PONDED_WATER;  aLev[1]=DOESNT_EXIST;
    aSV[2]=SNOW_COVER;    aLev[2]=DOESNT_EXIST;
  }
    else if (bal_type==SNOBAL_TWO_LAYER)
  {
    nSV=9;
    aSV[0]=NEW_SNOW;      aLev[0]=DOESNT_EXIST;
    aSV[1]=SNOW;          aLev[1]=DOESNT_EXIST;
    aSV[2]=SNOW_LIQ;      aLev[2]=0;
    aSV[3]=SNOW_LIQ;      aLev[3]=1;
    aSV[4]=COLD_CONTENT;  aLev[4]=0;
    aSV[5]=COLD_CONTENT;  aLev[5]=1;
    aSV[6]=PONDED_WATER;  aLev[6]=DOESNT_EXIST;
    aSV[7]=SNOW_TEMP;     aLev[7]=DOESNT_EXIST;
    aSV[8]=CUM_SNOWMELT;  aLev[8] = DOESNT_EXIST;
  }
  else{
    nSV=0;
  }
}

//////////////////////////////////////////////////////////////////
/// \brief Returns rates of change in all state variables modeled during snow balance calculations
/// \details for combined modeling of melt, refreeze, and energy content \n
/// if type==SNOBAL_SIMPLE_MELT:
///		<ul> <li> simple application of potential melt to SWE; no other variables tracked </ul> 
/// if type==SNOBAL_COLD_CONTENT:
///		<ul> <li> adapted from Brook90 SNOENRGY and SNOPACK routines </ul> \cite Federer2010
/// if type==SNOBAL_HBV-EC:
///		<ul> <li> adapted from HBV-Environment Canada </ul>
/// if type==SNOWBAL_UBC:
///		<ul> <li> adapted from the UBC Watershed Model (c) Michael Quick </ul>
///
/// \param *state_var [in] Array of current state variables in HRU
/// \param *pHRU [in] Reference to pertinent HRU
/// \param &Options [in] Global model options information
/// \param &tt [in] Current model time
/// \param *rates [out] Rate of loss of water from snow melt/refreeze combined [mm/d]
//
void CmvSnowBalance::GetRatesOfChange(const double		 *state_var, 
																      const CHydroUnit *pHRU, 
																      const optStruct	 &Options,
																      const time_struct &tt,
																			      double     *rates) const
{
  if (pHRU->GetType()==HRU_LAKE){return;}

  //------------------------------------------------------------
	if (type==SNOBAL_SIMPLE_MELT)
  {
    rates[0]=max(pHRU->GetForcingFunctions()->potential_melt,0.0);
  }
  //------------------------------------------------------------
	else if (type==SNOBAL_CEMA_NIEGE)
  {
    double pot_melt,snow_cov,SWE;
    double avg_annual_snow=CGlobalParams::GetParams()->avg_annual_snow;
    double snotemp=pHRU->GetSnowTemperature();
    pot_melt=0.0;
    if (snotemp==FREEZING_TEMP){
      pot_melt=max(pHRU->GetForcingFunctions()->potential_melt,0.0);//positive
    }
    SWE=state_var[iFrom[0]];

    snow_cov=min(SWE/avg_annual_snow,1.0); //way too small for Canadian basins!

    rates[0]=(0.9*snow_cov+0.1)*min(SWE/Options.timestep,pot_melt);                       //melt
    rates[1]=(snow_cov-state_var[iFrom[1]])/Options.timestep;   //change in snow cover
  }
  //------------------------------------------------------------
  else if (type==SNOBAL_COLD_CONTENT)
	{
    ColdContentBalance(state_var,pHRU,Options,tt,rates);
  }
  else if(type==SNOBAL_TWO_LAYER)
  {
      TwoLayerBalance(state_var,pHRU,Options,tt,rates);
  }
	//------------------------------------------------------------
	else if (type==SNOBAL_HBV)
	{
    double Ka    =pHRU->GetSurfaceProps()->refreeze_factor;//[mm/d/K]
		double Ta		 =pHRU->GetForcingFunctions()->temp_daily_ave;
    double tstep =Options.timestep;
	
		double melt,refreeze, liq_cap;

		double S		=state_var[iFrom[0]];//snow, SWE mm
    double SL		=state_var[iFrom[1]];//liquid snow, mm
    double SD  	=state_var[iFrom[2]];//snow depth, mm 

    melt    =max(pHRU->GetForcingFunctions()->potential_melt,0.0); //positive
		refreeze=Ka*min(Ta-FREEZING_TEMP,0.0);//negatively valued

		liq_cap=CalculateSnowLiquidCapacity(S,SD,Options);

		rates[0] =max(-SL/tstep,refreeze);
		rates[0]+=max(melt,(liq_cap-SL)/tstep);     //melt 
		rates[1] =max(melt-(liq_cap-SL)/tstep,0.0); //overflow to soil
	}
	//------------------------------------------------------------
  else if (type == SNOBAL_UBCWM)
  {
    double SWE = state_var[iFrom[0]]; //snow as SWE [mm]
    double Sliq = state_var[iFrom[1]]; //liquid snow [mm]
    double CC = state_var[iFrom[3]]; //cold content [mm]
    double coverage = state_var[iFrom[4]]; //snow coverage
    double cum_melt = state_var[iFrom[5]]; //cumulative melt [mm]

    double pot_melt;                     //potential melt [mm]
    double snowmelt = 0;                   //total snowmelt [mm]  
    double tstep = Options.timestep;

    //reset cumulative melt to zero in October
    if ((pHRU->GetCentroid().latitude < 0) && (tt.month == 4) && (tt.day_of_month == 1)){
      cum_melt = 0.0;
    }
    else if ((tt.month == 10) && (tt.day_of_month == 1)){
      cum_melt = 0.0;
    }

    pot_melt = pHRU->GetForcingFunctions()->potential_melt*Options.timestep;

    //Reduce/increase cold content (always leads to pot_melt>=0, CC>=0)
    if (CC > pot_melt){
      CC -= pot_melt;
      pot_melt = 0;
    }
    else{
      pot_melt -= CC;
      CC = 0.0;
    }


    double snowpatch_limit = pHRU->GetSurfaceProps()->snow_patch_limit;

    if (snowpatch_limit > 0.0)
    {
      double snowmeltpatch = 0;
      double AF1, AF2, WSN;

      //if melting will expose ground, melt away snow up to the point of patching and reduce the potential melt
      if ((SWE > snowpatch_limit) && (SWE - pot_melt <= snowpatch_limit)){
        snowmeltpatch = SWE - snowpatch_limit; //calculate snowmelt above snow patch limit (snowmeltpatch) 
        SWE = snowpatch_limit;
        pot_melt = pot_melt - snowmeltpatch; //reduce the potential melt amt of snow melted above the patch limit.
      }

      //if ground is already exposed (or on the cusp of being exposed) 
      if (SWE <= snowpatch_limit)
      {
        //redistribute snow to a wedge
        WSN = 2.0 * sqrt(SWE * snowpatch_limit);
        lowerswap(pot_melt, WSN);

        if (SWE > 0)
        {
          AF1 = WSN / (2 * snowpatch_limit); //AF1 is area fraction of snowcover
          AF2 = (WSN - pot_melt) / (2 * snowpatch_limit);

          //so the snowmelt already allows for snow fraction
          snowmelt = (WSN * AF1) / 2 - (WSN - pot_melt) / 2 * AF2 + snowmeltpatch;
          WSN -= pot_melt;
          upperswap(WSN, 0.0);

          //reaccumulation of snow
          if (WSN < 2 * snowpatch_limit)
          {
            SWE = (WSN * WSN * 0.5) / (2 * snowpatch_limit);
          }
        }
        else{
          snowmelt = 0;
        }
      }
      else
      {
        snowmelt = max(min(pot_melt, SWE), 0.0); //assumes daily ts for now
        SWE -= snowmelt;
      }
    }
    else //snowpatch_limit = 0
    {
      snowmelt = max(min(pot_melt, SWE), 0.0);//assumes daily ts for now
      SWE -= snowmelt;
    }

    double loss = 0.0;
    double transfer = 0;//amount transferred from frozen to liquid snow
    double orig_SWE = state_var[iFrom[0]];
    bool   oldversion = false;

    double snow_d=state_var[iFrom[6]];//JRC : new

    if (oldversion)
    {
      if ((snowmelt > 0) && (SWE > REAL_SMALL)){ loss = snowmelt*(Sliq / orig_SWE); }
      Sliq -= loss;//loss of water holding capacity

      if (SWE < REAL_SMALL){
        loss += Sliq;
        Sliq = 0;
        CC = 0;
        transfer = 0.0;
      }
      else{
        double snDef = max(CalculateSnowLiquidCapacity(SWE, 0.0, Options) - Sliq, 0.0);

        if (snowmelt > snDef)
        {
          Sliq += snDef;
          snowmelt -= snDef;
          transfer = snDef;
          snDef = 0.0;
        }
        else{
          snDef -= (snowmelt);
          Sliq += (snowmelt);
          transfer = (snowmelt);
          snowmelt = 0.0;
          //loss    =0.0;

        }
      }
    }
    else 
    {
      if (SWE < REAL_SMALL){ 
        snow_d=0.0;
        CC = 0;
      }
      else
      {
        if (snowmelt > snow_d){
          snowmelt-=snow_d;
          SWE     +=snow_d;
          snow_d=0;
        }
        else{
          snow_d-=snowmelt;
          SWE   +=snowmelt;
          snowmelt=0;
        }
      }
    }

    //cum_melt+=-(SWE-state_var[iFrom[2]]-transfer);
    cum_melt+=-(SWE-state_var[iFrom[0]]);
    //Calculate snow coverage (this should be compared to AF1 which is the area fraction of snow cover, and non-linear)
	  if (snowpatch_limit!=0.0){
      coverage=SWE/snowpatch_limit;
      upperswap(coverage,0.0);
      lowerswap(coverage,1.0);
    }
    else{
      if (SWE>0.0){coverage=1.0;}
      else        {coverage=0.0;}
    }
    
    //decay of cold content
    CC *=exp(-pHRU->GetSurfaceProps()->CC_decay_coeff*tstep);
    
    rates[0]=transfer/tstep;                       //rates[0]: SNOW->SNOW_LIQ
    rates[1]=loss/tstep;                           //rates[1]: SNOW_LIQ->PONDED  (net melt)
    rates[2]=-(SWE     -state_var[iFrom[2]]+transfer)/tstep; //rates[2]: SNOW->PONDED  (net melt)
    rates[3]=(CC      -state_var[iFrom[3]])/tstep; //rates[3]: CC modification
    rates[4]=(coverage-state_var[iFrom[4]])/tstep; //rates[4]: snow cover modification
    rates[5]=(cum_melt-state_var[iFrom[5]])/tstep; //rates[5]: cumulative melt modification

    if (!oldversion){
      rates[0] = 0.0;                                    //don't care about liquid content
      rates[1] = 0.0;
      rates[2] = snowmelt / tstep;                       //rates[2]: SNOW->PONDED
      rates[6] = (snow_d - state_var[iFrom[6]]) / tstep; //rates[6]: snow deficit modification
    }

    rates[0]*=pHRU->GetForcingFunctions()->subdaily_corr;
    rates[1]*=pHRU->GetForcingFunctions()->subdaily_corr;
    rates[2]*=pHRU->GetForcingFunctions()->subdaily_corr;
    rates[4]*=pHRU->GetForcingFunctions()->subdaily_corr;
    rates[5]*=pHRU->GetForcingFunctions()->subdaily_corr;
    rates[6]*=pHRU->GetForcingFunctions()->subdaily_corr;
    
  }//end UBC Melt
  
};
/*****************************************************************
   Cold Content Balance :Get Rates of Change
------------------------------------------------------------------
adapted from Brook90 SNOENRGY and SNOPACK routines
*****************************************************************/
//////////////////////////////////////////////////////////////////
/// \docminor This function needs better documentation
/// \brief Balances cold content from snow melt/refreeze
/// \ref Adapted from Brook90 SNOENRGY and SNOPACK routines \cite Federer2010
/// 
/// \param *state_vars [in] Array of current state variables in HRU 
/// \param *pHRU [in] Reference to pertinent HRU
/// \param &Options [in] Global model options information
/// \param &tt [in] Current model time
/// \param *rates [out] Rate of change in state variables due to snow balance calculations
//
void CmvSnowBalance::ColdContentBalance(const double		 *state_vars, 
																        const CHydroUnit *pHRU, 
																        const optStruct	 &Options,
																        const time_struct &tt,
																			        double     *rates) const
{
  const double CCFAC   =0.3;//[MJ/m2-d-K]
  const double MELT_FAC=1.5;//[MJ/m2-d-K],
  const double LAIMLT  =0.2;
  const double SAIMLT  =0.5;
  const double MAXLQF  =0.05;//Liquid water holding capacity
  
  double Ta				 =pHRU->GetForcingFunctions()->temp_daily_ave;
  double day_length=pHRU->GetForcingFunctions()->day_length;

	double CC				 =state_vars[iFrom[0]];//cold content MJ-mm/kg
  double SL				 =state_vars[iFrom[1]];//liquid snow, mm
  double S				 =state_vars[iFrom[3]];//snow, SWE mm
  
  double LAI       =pHRU->GetVegVarProps()->LAI; 
  double SAI       =pHRU->GetVegVarProps()->SAI; 
  double tstep		 =Options.timestep;

  double incoming_snow_en;//incoming energy flux
	double pot_melt;				//incoming energy flux, in terms of water flux rate [mm/d]
	double CC_air;					//air temp, expressed in terms of cold content [MJ-mm/kg]
	double liq_snow_cap;    //maximum liquid snow capacity [mm]
	double rainthru=0.0;		//TMP DEBUG
  double Tsnow;						//[C]
  double instant_refreeze;
  double refreeze,cooling,warming,melt_to_liq,melt_to_SW,shrinking_poro,eq_en_avail;

  if (S==0){return;}

	//calculate instant equilibrium of CC and liquid water
  if ((CC>0) && (SL>0))
  {  
    instant_refreeze=threshMin(SL/tstep,CC/LH_FUSION/tstep,0.0);//mm/day
    rates[1]+=instant_refreeze;            //SnowLiq->Snow
    rates[0]-=instant_refreeze*LH_FUSION;  //ColdCon->LostEn
    SL-=rates[1]*tstep;
  }

	Tsnow=FREEZING_TEMP-CC/S/SPH_ICE; //JRC: problematic if S is small
	//[C]=              [MJ-mm/kg]*[kg-K/MJ]*[1/mm]

  if (Ta <= FREEZING_TEMP)
  { //  snow warms or cools proportional to snow-air temperature difference
    incoming_snow_en=CCFAC*2.0*day_length*(Ta-Tsnow);
		//[MJ/m2/d]     =[MJ/m2-d-K]         *[K]
  }
  else
  { //energy input proportional to TA, modified by cover and daylength
    incoming_snow_en=MELT_FAC*2.0*day_length*(Ta-FREEZING_TEMP)*exp(-SAIMLT*SAI)*exp(-LAIMLT*LAI);
		//[MJ/m2/d]     =[MJ/m2-d-K]            *[K]
  }
  //incoming_snow_en*=30; //why is this necc to be consistent with hybrid??

	incoming_snow_en+=rainthru*threshPositive(Ta-FREEZING_TEMP)*SPH_WATER*DENSITY_WATER/MM_PER_METER;
	//[MJ/m2/d]      =[mm/d]  *[K]                             *[MJ/kg/K] *[kg/m3]      *[m/mm]      

 	pot_melt    =incoming_snow_en*(1.0/DENSITY_WATER/LH_FUSION*MM_PER_METER); 
  //[mm/d]    =[MJ/m2/d]       *[m3/kg]      *[kg/MJ]   *[mm/m]

  CC_air      =(FREEZING_TEMP-Ta)*SPH_ICE*S;
	//MJ-mm/kg  =[K]               *[MJ/kg/K]*[mm]

  liq_snow_cap=CalculateSnowLiquidCapacity(S,0.0,Options);
  
  if (pot_melt<=0) //negative energy balance - snowpack cooling
  {
    refreeze=threshMin(SL/tstep,-pot_melt,0.0);             //cold used to refreeze liquid
    rates[1]+=refreeze;//SnowLiq->Snow [mm/d]
    pot_melt+=refreeze;

    cooling=threshMin(-pot_melt,0.0 ,0.0)*LH_FUSION;        //if any cold remaining, increases cold content
    cooling=threshMin(cooling,(CC_air-CC)/tstep,0.0);       //dont exceed maximum cold content (minimum temp Ta)
    rates[2]+=cooling;//Atm->CC [MJ-mm/kg-d]
  }
  else         //positive energy balance - snowpack warming
  { 
    if ((pot_melt*LH_FUSION<CC/tstep) || (Ta<FREEZING_TEMP)) 
    {
      //energy merely reduces cold content, but not below equiv. air temp
      warming=pot_melt*LH_FUSION;
      warming=threshMin(warming,-(CC-CC_air)/tstep,0.0);//is negative sign correct here?

      rates[0]-=warming;//ColdCon<-LostEn
    }
    else
    {
      eq_en_avail=pot_melt;

      warming=CC/tstep;
      rates[0]+=warming; //ColdCon->LostEn//it's warm: all cold content lost
      eq_en_avail-=warming/LH_FUSION;

      melt_to_liq=threshMin(eq_en_avail,(liq_snow_cap-SL)/tstep,0.0);
      rates[1]-=melt_to_liq;//SnowLiq<-Snow (thus negative)
      eq_en_avail-=melt_to_liq;
      
      melt_to_SW=threshMin(eq_en_avail,S/tstep-melt_to_liq,0.0);
      rates[3]+=melt_to_SW;//snow->surface water
      
      shrinking_poro=melt_to_SW*MAXLQF;
      rates[4]+=shrinking_poro;//snow_liq->surface water
    }
  }
  /*cout<<"iFrom[0]: "<<iFrom[0]<<endl;
	cout.precision(3);
  cout<<"Snow: "<<S<<endl;
  cout<<"Snow Liq:"<<SL<<endl;
  cout<<"Cold Content:"<<CC<<endl;
  cout<<"Day Length: "<<day_length<<endl;
	cout<<"Air Temp: "<<Ta<<endl;
	cout<<"Snow Temp: "<<Tsnow<<endl;
	cout<<"liq_snow_cap: "<<liq_snow_cap<<endl;
  cout<<"incoming_snow_en: "<<incoming_snow_en<<"[MJ/m2/d]"<<endl;
  cout<<"pot_melt: "<<pot_melt<<" mm/d"<<endl;
	if (pot_melt<0.0){cout<<"COOLING"<<endl;}
	else             {cout<<"WARMING"<<endl;}
  cout<<"CC_air: "<<CC_air<<endl;
  cout <<"rates[0]:"<<rates[0]<<endl;
  cout <<"rates[1]:"<<rates[1]<<endl;
  cout <<"rates[2]:"<<rates[2]<<endl;
  cout <<"rates[3]:"<<rates[3]<<endl;
  cout <<"rates[4]:"<<rates[4]<<endl<<endl;
  ExitGracefullyIf(S-(rates[4]-rates[1])*tstep<0,
    "Snowmelt Bad",RUNTIME_ERR);*/
}

//////////////////////////////////////////////////////////////////
/// \brief Balances cold content from snow melt/refreeze
///
/// \note Precipitation should come before snow balance for this to work as intended.
///
/// \param *state_vars [in] Array of current state variables in HRU 
/// \param *pHRU [in] Reference to pertinent HRU
/// \param &Options [in] Global model options information
/// \param &tt [in] Current model time
/// \param *rates [out] Rate of change in state variables due to snow balance calculations
//
void CmvSnowBalance::TwoLayerBalance(const double   *state_vars,
    const CHydroUnit *pHRU,
    const optStruct	 &Options,
    const time_struct &tt,
    double     *rates) const
{

    //initialize storage varaibles  
    //------------------------------------------------------------------------
    double newSnow = state_vars[iFrom[0]]; //NEW SNOW [mm]
    double rainthru = state_vars[iFrom[1]]; //PONDEDWATER [mm]
    double Swe = state_vars[iFrom[2]]; // total snow water equivalent [mm]
    double SweSurf; // Water equivalent of snow surface layer [mm]
    double SwePack; // Water equivalent of snow pack layer [mm]
    double SlwcSurf = state_vars[iFrom[3]]; //liquid snow in snow surface layer [mm]
    double SlwcPack = state_vars[iFrom[4]]; //liquid snow in snow pack layer [mm]

    double CcSurf = state_vars[iFrom[6]]; //cold content of snow surface layer [MJ/m2]
    double CcPack = state_vars[iFrom[7]]; //cold content of snow pack layer [MJ/m2]
    double snowT = state_vars[iFrom[8]];  //snow surface temperature [C]
    double cum_melt = state_vars[iFrom[9]]; //cumulative melt [mm]

    if (Swe <= REAL_SMALL && newSnow <= REAL_SMALL) { return; } //no snow to balance

    //parameters
    //------------------------------------------------------------------------
    double MAXLIQ = CGlobalParams::GetParams()->snow_SWI;       // maximum liquid water fraction of snow, dimensionless
    double MAXSWESURF = CGlobalParams::GetParams()->max_SWE_surface;  // maximum swe of surface layer of snowpack

    // forcings
    //------------------------------------------------------------------------
    double Mf = pHRU->GetForcingFunctions()->potential_melt*Options.timestep;
    double Ta = pHRU->GetForcingFunctions()->temp_ave;

    //Rates
    //------------------------------------------------------------------------
    double meltSurf = 0.0;    // melt rate from snow surface layer
    double surfToPack = 0.0;  // liquid percolating from surface to pack 
    double freezePack = 0.0;  // refreeze in the pack layer
    double SnowOutflow = 0.0; // outflow from the snow

    //INTERNAL
    //------------------------------------------------------------------------
    double posMf;               //positive meltfactor
    double ccSnowFall;

    //reset cumulative melt to zero in October
    //------------------------------------------------------------------------
    if ((pHRU->GetCentroid().latitude < 0) && (tt.month == 4) && (tt.day_of_month == 1)) {
        cum_melt = 0.0;
    }
    else if ((tt.month == 10) && (tt.day_of_month == 1)) {
        cum_melt = 0.0;
    }

    //reconstruct two layer snowpack
    //------------------------------------------------------------------------
    if ( (Swe+SlwcSurf) > MAXSWESURF)
    {
        SweSurf = MAXSWESURF-SlwcSurf;
    }
    else
    {
        SweSurf = Swe;
    }
    SwePack = Swe - SweSurf;

    //add snow throughfall to snowPack and compute its cold content
    //this also initializes the cold content with the first snow fall
    //------------------------------------------------------------------------
    ccSnowFall = HCP_ICE*MJ_PER_J / MM_PER_METER * newSnow * max(-Ta, 0.0);
    //[MJ/m2] = [J/m3/K]*[MJ/J]  * [m/mm]      *  [mm]  *  [K]

    //distribute fresh snowfall
    //------------------------------------------------------------------------
    if (newSnow < (MAXSWESURF - SweSurf - SlwcSurf))// suface layer is below max
    {
        SweSurf += newSnow;
        CcSurf += ccSnowFall;
    }
    else
    {//snow surface layer is at max
        double deltaswePack = SweSurf + SlwcSurf + newSnow - MAXSWESURF;
        double deltaccPack=0;
        if (SweSurf > 0) {
             deltaccPack = (deltaswePack/SweSurf) * CcSurf;//give pack the proportional coldconent of surface layer
        }
        SwePack += deltaswePack;
        SweSurf = MAXSWESURF - SlwcSurf;
        CcPack += deltaccPack;
        CcSurf += ccSnowFall - deltaccPack;
    }

    //add rainfall to surface snow liquid
    //------------------------------------------------------------------------
    SlwcSurf += rainthru;

    //add CC to Mf to get total energy available for melt/freeze
    //------------------------------------------------------------------------
    Mf -= CcSurf / LH_FUSION;

    //snowpack cooling or warming
    //engergy exchange occurs only between snow surface layer and atmosphere
    //snow surface layer and snow pack layer only exchange liquid water
    //------------------------------------------------------------------------
    //snowpack cooling or warming
    //------------------------------------------------------------------------
    if (Mf <= 0.0)
    {    //   snow cooling     
        posMf = -Mf;
        if (posMf < SlwcSurf)//refreeze part of liquid water
        {
            meltSurf += Mf;   // note that Mf is negative 
            CcSurf = 0.0;
        }
        else// refreeze all liquid water and increase cold content
        {
            posMf -= SlwcSurf;
            meltSurf -= SlwcSurf;
            CcSurf = posMf * LH_FUSION; // leftover energy is new CC

            if (SweSurf < 50.0) //dont cool below gruTa when Swe is below 10 mm (need this for stable run, otherwise CC and Tsnow start to fluctuate...)
            {
                CcSurf = min(CcSurf, -Ta * SweSurf * HCP_ICE*MJ_PER_J/MM_PER_METER);
                CcSurf = max(CcSurf, 0.0);
            }
        }
    }
    else // snow warming (_Mf > 0)
    {
        CcSurf = 0.0; // can't have CC when snow is melting

        if (SweSurf < Mf) // All snow melts
        {
            meltSurf += SweSurf;
        }
        else
        {
            meltSurf += Mf;
        }
    }

    //Calculate how much liquid stays in surface layer, and pass the rest to the pack layer
    //------------------------------------------------------------------------		
    SweSurf -= meltSurf; //new SWE after melt/freeze
    SlwcSurf += meltSurf; //new Slwc after melt/freeze
    if (SlwcSurf >= MAXLIQ * SweSurf)
    {
        surfToPack = SlwcSurf - MAXLIQ * SweSurf;
    }
    else
    {
        surfToPack = 0;
    }

    //snow pack layer
    //------------------------------------------------------------------------
    SlwcPack += surfToPack;//add the surface layer outflow to pack liquid water 

    //re-freeze liquid water in snowpack layer
    //------------------------------------------------------------------------
    if (CcPack > SlwcPack * LH_FUSION) //snow pack layer cold content not fully satisfied
    { 
        CcPack -= SlwcPack * LH_FUSION;        //freeze all water
        freezePack += SlwcPack * LH_FUSION;
    }
    else //eliminate cold content
    {
        freezePack += CcPack / LH_FUSION;      //freeze only part of water
        CcPack = 0.0;
    }

    //update snow pack layer liquid water content
    //------------------------------------------------------------------------
    SwePack += freezePack; //new SWE after melt/freeze
    SlwcPack -= freezePack; //new Slwc after melt/freeze
    if (SlwcPack > SwePack * MAXLIQ)
    {
        SnowOutflow = SlwcPack - SwePack * MAXLIQ;
    }
    else
    {
        SnowOutflow = 0;
    }

    //Snow Temp - assumes an isothermal snow surface layer
    snowT = -CcSurf / (HCP_ICE*MJ_PER_J* max(1.0, SweSurf)/MM_PER_METER);
    snowT *= 0.2; // There's no physical basis for this, but snowT was always way too low. 
                  // This correction puts snowT close to the air temperature.

//    if (snowT < -30) { snowT = -30; }

    cum_melt += meltSurf;

    rates[0] = newSnow / Options.timestep;     // SNOWFALL -> SNOW
    rates[1] = rainthru / Options.timestep;    // PONDEDWATER -> SNOW_LIQ surface
    rates[2] = meltSurf / Options.timestep;    // SNOW -> SNOW_LIQ surface
    rates[3] = surfToPack / Options.timestep;  // SNOW_LIQ surface -> SNOW_LIQ pack
    rates[4] = freezePack / Options.timestep;  // SNOW_LIQ pack -> SNOW
    rates[5] = SnowOutflow / Options.timestep; // SNOW_LIQ pack -> ponded water
    rates[6] = (CcSurf - state_vars[iFrom[6]]) / Options.timestep;   //CC pack layer
    rates[7] = (CcPack - state_vars[iFrom[7]]) / Options.timestep;   //CC pack layer
    rates[8] = (snowT - state_vars[iFrom[8]]) / Options.timestep;   //SNOW_TEMP
    rates[9] = (cum_melt - state_vars[iFrom[9]]) / Options.timestep; //Cumulative Melt
}

//////////////////////////////////////////////////////////////////
/// \brief Corrects rates of change (*rates) returned from RatesOfChange function 
/// \note All constraints contained in routine itself- this routine is blank
///
/// \param *state_vars [in] Array of current state variables in HRU 
/// \param *pHRU [in] Reference to pertinent HRU
/// \param &Options [in] Global model options information
/// \param &tt [in] Current model time
/// \param *rates [out] Rate of change in state variables due to snow balance calculations
//
void  CmvSnowBalance::ApplyConstraints( const double		 *state_vars, 
					                              const CHydroUnit *pHRU, 
					                              const optStruct	 &Options,
					                              const time_struct &tt,
                                              double     *rates) const
{
  if (pHRU->GetType()==HRU_LAKE){return;}

  if (type==SNOBAL_SIMPLE_MELT)
  {
    if (rates[0]<0.0){rates[0]=0.0;}//positivity constraint

    //cant remove more than is there
    rates[0]=threshMin(rates[0],max(state_vars[iFrom[0]]/Options.timestep,0.0),0.0);
  }
  if (type == SNOBAL_UBCWM)
  {
      //snow cover should never be negative
      // subdaily correction sometimes puts it out of this range
      rates[4] = threshMax(rates[4], -state_vars[iFrom[4]] / Options.timestep, 0.0);
      rates[4] = threshMin(rates[4], (1-state_vars[iFrom[4]]) / Options.timestep, 0.0);
  }

  return;//most constraints contained in routine itself
}

