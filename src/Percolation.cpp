/*----------------------------------------------------------------
  Raven Library Source Code
  Copyright (c) 2008-2021 the Raven Development Team
  ----------------------------------------------------------------
  Percolation
  ----------------------------------------------------------------*/

#include "HydroProcessABC.h"
#include "SoilWaterMovers.h"

/*****************************************************************
   Percolation Constructor/Destructor
------------------------------------------------------------------
*****************************************************************/

//////////////////////////////////////////////////////////////////
/// \brief Implementation of the percolation constructor
///
/// \param p_type [in] Model of percolation selected
/// \param From_index [in] Index of soil storage unit from which percolation occurs
/// \param To_index [in] Index of soil storage unit from to which water is lost
//
CmvPercolation::CmvPercolation(perc_type        p_type,
                               int              From_index,                     //soil water storage
                               int              To_index)
  :CHydroProcessABC(PERCOLATION,From_index,To_index)
{
  ExitGracefullyIf(From_index==DOESNT_EXIST,
                   "CmvPercolation Constructor: invalid 'from' compartment specified",BAD_DATA);
  ExitGracefullyIf(To_index==DOESNT_EXIST,
                   "CmvPercolation Constructor: invalid 'to' compartment specified",BAD_DATA);
  type  = p_type;
}

//////////////////////////////////////////////////////////////////
/// \brief Implementation of the default destructor
//
CmvPercolation::~CmvPercolation(){}


//////////////////////////////////////////////////////////////////
/// \brief Validate iTo/iFrom connectivity of the percolation
//
void   CmvPercolation::Initialize()
{
  sv_type fromType      = pModel->GetStateVarType(iFrom[0]);
  ExitGracefullyIf(fromType!=SOIL,
                   "CmvPercoclation::Initialize: percolation must be from a soil unit",BAD_DATA);
  sv_type toType        = pModel->GetStateVarType(iTo[0]);
  ExitGracefullyIf((toType!=SOIL) && (toType!=GROUNDWATER),
                   "CmvPercoclation::Initialize: percolation must be between two soil or groundwater units",BAD_DATA);
  //check if PERC_POWER_LAW and not using 2-layer model

  //check for existence of params
}

//////////////////////////////////////////////////////////////////
/// \brief Returns participating parameter list
///
/// \param *aP [out] array of parameter names needed for percolation algorithm
/// \param *aPC [out] Class type (soil, vegetation, landuse or terrain) corresponding to each parameter
/// \param &nP [out] Number of parameters required by percolation algorithm (size of aP[] and aPC[])
//
void CmvPercolation::GetParticipatingParamList(string  *aP, class_type *aPC , int &nP) const
{
  //only needed for parameters that are not autogenerated
  if (type==PERC_CONSTANT)
  {
    nP=1;
    aP[0]="MAX_PERC_RATE"; aPC[0]=CLASS_SOIL;
  }
  else if ((type == PERC_GAWSER) || (type == PERC_GAWSER_CONSTRAIN))
  {
    nP=3;
    aP[0]="MAX_PERC_RATE";  aPC[0]=CLASS_SOIL;
    aP[1]="FIELD_CAPACITY"; aPC[1]=CLASS_SOIL;
    aP[2]="POROSITY";       aPC[2]=CLASS_SOIL;
  }
  else if (type==PERC_POWER_LAW)
  {
    nP=3;
    aP[0]="MAX_PERC_RATE"; aPC[0]=CLASS_SOIL;
    aP[1]="PERC_N";        aPC[1]=CLASS_SOIL;
    aP[2]="POROSITY";      aPC[2]=CLASS_SOIL;
  }
  else if (type==PERC_PRMS)
  {
    nP=5;
    aP[0]="MAX_PERC_RATE";  aPC[0]=CLASS_SOIL;
    aP[1]="PERC_N";         aPC[1]=CLASS_SOIL;
    aP[2]="POROSITY";       aPC[2]=CLASS_SOIL;
    aP[3]="FIELD_CAPACITY"; aPC[3]=CLASS_SOIL;
    aP[4]="SAT_WILT";       aPC[4]=CLASS_SOIL;
  }
  else if (type==PERC_SACRAMENTO)
  {
    nP=6;
    aP[0]="SAC_PERC_EXPON";    aPC[0]=CLASS_SOIL;
    aP[1]="SAC_PERC_ALPHA";    aPC[1]=CLASS_SOIL;
    aP[2]="MAX_BASEFLOW_RATE"; aPC[2]=CLASS_SOIL;
    aP[3]="POROSITY";          aPC[3]=CLASS_SOIL;
    aP[4]="FIELD_CAPACITY";    aPC[4]=CLASS_SOIL;
    aP[5]="SAT_WILT";          aPC[5]=CLASS_SOIL;
  }
  else if ((type==PERC_LINEAR) || (type==PERC_LINEAR_ANALYTIC))
  {
    nP=1;
    aP[0]="PERC_COEFF";     aPC[0]=CLASS_SOIL;
  }
  else if (type==PERC_GR4J)
  {
    nP=0;
  }
  else if ((type==PERC_GR4JEXCH) || (type==PERC_GR4JEXCH2))
  {
    nP=1;
    aP[0]="GR4J_X2";     aPC[0]=CLASS_SOIL;
  }
  else if (type==PERC_ASPEN)
  {
    nP=1;
    aP[0]="PERC_ASPEN"; aPC[0]=CLASS_SOIL;
  }
  else
  {
    ExitGracefully("CmvPercolation::GetParticipatingParamList: undefined percolation algorithm",BAD_DATA);
  }
}

//////////////////////////////////////////////////////////////////
/// \brief returns participating state variables
/// \note User specifies "From" and "To" compartments - levels are not know before construction
///
/// \param p_type [in] Model of percolation used
/// \param *aSV [out] Array of state variable types needed by percolation algorithm
/// \param *aLev [out] Array of level of multilevel state variables (or DOESNT_EXIST, if single level)
/// \param &nSV [out] Number of state variables required by percolation algorithm (size of aSV[] and aLev[] arrays)
//
void CmvPercolation::GetParticipatingStateVarList(perc_type     p_type,sv_type *aSV, int *aLev, int &nSV)
{
  nSV=0;
}

//////////////////////////////////////////////////////////////////
/// \brief Returns rate of loss of water to lower soil layers [mm/d]
/// \details
//
/// \b IF type == PERC_GAWSER
///             \li Percolation as used in GAWSER
///             \li \math \f$ P=K*(S-S_{fc})/(S_{max}-S_{fc}) \f$
///
/// \b IF type == PERC_POWER_LAW
///             \li power law percolation method
///             \li \math \f$ P=K*(S/S_{max})^c \f$
///             \ref as defined by FUSE, Clark et al., 2007 \cite Clark2008WRR
///   \ref Used in VIC, HBV models \cite Gao2009 \cite Bergstroem1995
///
/// \b IF type == PERC_PRMS
///             \li  PRMS percolation method
///             \li \math \f$ P=K*(S-(S_{fc}-S_{wc}))/(S_{max}-(S_{fc}-S_{wc})) \f$
///             \ref as defined by FUSE, Clark et al., 2007 \cite Clark2008WRR
///
/// \b IF type == PERC_SACRAMENTO
///             \li Sacremento percolation method
///             \li \math \f$ P=L_{Z}S_{B}{S-(S_{fc}-S_{wc})}/{S_{max}-(S_{fc}-S_{wc})} \f$
///             \li  where \math \f$ L_{Z} = 1 + \alpha(1 - (S^{2}/S^{2}_{max}))^{\psi} \f$
///             \ref as defined by FUSE, Clark et al., 2007 \cite Clark2008WRR
///
/// \b IF type == PERC_CONSTANT
///             \li  Constant rate of percolation (a la HBV)
///
/// \b IF type == PERC_GR4J
///             \li  from GR4J model
///
/// \param *state_vars [in] Current array of state variables
/// \param *pHRU [in] Reference to pertinent HRU
/// \param &Options [in] Global model options information
/// \param &tt [in] Current model time structure
/// \param *rates [out] Rate of percolation [mm/d]
//
void   CmvPercolation::GetRatesOfChange( const double                   *state_vars,
                                         const CHydroUnit       *pHRU,
                                         const optStruct        &Options,
                                         const time_struct &tt,
                                         double     *rates) const
{
  if (pHRU->GetHRUType()==HRU_LAKE){return;}//Lakes  (but allowed beneath some glaciers?)

  double stor,max_stor;

  int m   = pModel->GetStateVarLayer(iFrom[0]); //which soil layer
  stor     = state_vars[iFrom[0]];              //soil layer water content [mm]
  max_stor = pHRU->GetSoilCapacity(m);          //maximum storage of 'from' soil layer [mm]
  if (max_stor <= 0.0){return;}                 //handles zero-thickness layers

  //-----------------------------------------------------------------
  if (type==PERC_CONSTANT)
  {
    rates[0]=pHRU->GetSoilProps(m)->max_perc_rate;
  }
  //-----------------------------------------------------------------
  else if (type==PERC_GAWSER)
  {
    //same as PERC_PRMS with sat_wilt=0.0 and perc_n=1.0
    double field_cap,max_rate;//mm/d
    field_cap= pHRU->GetSoilProps(m)->field_capacity*max_stor; //moisture content at fc [mm]
    max_rate = pHRU->GetSoilProps(m)->max_perc_rate;

    rates[0]=max_rate*max(stor-field_cap,0.0)/(max_stor-field_cap);
  }
  //-----------------------------------------------------------------
  else if (type == PERC_GAWSER_CONSTRAIN)
  {
    double field_cap, max_rate;//mm/d
    field_cap = pHRU->GetSoilProps(m)->field_capacity*max_stor; //moisture content at fc [mm]
    max_rate  = pHRU->GetSoilProps(m)->max_perc_rate;

    rates[0] = max_rate*max(stor-field_cap,0.0)/(max_stor-field_cap);

    if (rates[0] > (stor - field_cap)/Options.timestep)
    {
      rates[0] = (stor - field_cap)/Options.timestep; //only percolates when storage>fc
    }
  }
  //-----------------------------------------------------------------
  else if (type==PERC_POWER_LAW)
  {
    double max_rate,n;

    max_rate= pHRU->GetSoilProps(m)->max_perc_rate;
    n       = pHRU->GetSoilProps(m)->perc_n;

    rates[0] = max_rate * pow(stor/max_stor,n);
  }
  //-----------------------------------------------------------------
  else if (type==PERC_LINEAR)
  {
    double perc_coeff;
    perc_coeff= pHRU->GetSoilProps(m)->perc_coeff;

    rates[0] = perc_coeff * stor;
  }
  //-----------------------------------------------------------------
  else if (type==PERC_LINEAR_ANALYTIC)
  {
    double perc_coeff;
    perc_coeff= pHRU->GetSoilProps(m)->perc_coeff;

    rates[0]= stor*(1-exp(-perc_coeff*Options.timestep))/Options.timestep; //Alternate analytical formulation
  }
  //-----------------------------------------------------------------
  else if (type==PERC_PRMS)
  {
    double max_rate,n,tens_stor;
    double free_stor, free_stor_max;                                                    //maximum tension storage in upper soil layer [mm]

    tens_stor = pHRU->GetSoilTensionStorageCapacity(m);                                 // [mm]
    max_rate  = pHRU->GetSoilProps(m)->max_perc_rate;
    n         = pHRU->GetSoilProps(m)->perc_n;

    free_stor                   = max(0.0,stor-tens_stor);      //free water content in soil layer [mm]
    free_stor_max = (max_stor-tens_stor);                       //max free water content in soil layer [mm]

    rates[0] = max_rate * pow(free_stor/free_stor_max,n);
  }
  //-----------------------------------------------------------------
  else if (type==PERC_SACRAMENTO)
  {
    double psi, alpha;
    double free_stor, free_stor_max, lz_perc, max_baseflow;
    double tens_stor;
    double stor2,max_stor2;

    int m2  = pModel->GetStateVarLayer(iTo  [0]);

    stor2                 = state_vars[iTo  [0]];
    tens_stor = pHRU->GetSoilTensionStorageCapacity(m);

    psi                   = pHRU->GetSoilProps(m2)->SAC_perc_expon;
    alpha                 = pHRU->GetSoilProps(m2)->SAC_perc_alpha;

    if (m==0){max_stor2= pHRU->GetSoilCapacity(m2);}
    else     {max_stor2= ALMOST_INF;               }

    free_stor     = max(0.0,stor-tens_stor);      //free water content in soil layer [mm]
    free_stor_max = (max_stor-tens_stor);         //max free water content in soil layer [mm]
    max_baseflow  = pHRU->GetSoilProps(m)->max_baseflow_rate;
    lz_perc       = 1.0 + alpha * pow((1-(stor2/max_stor2)),psi);       //lower zone percolation demand

    rates[0] =  lz_perc * max_baseflow * (free_stor/free_stor_max);
  }
  //-----------------------------------------------------------------
  else if (type==PERC_GR4J)
  {
    rates[0]=stor*(1.0-pow(1.0+pow(4.0/9.0*max(stor/max_stor,0.0),4),-0.25))/Options.timestep;
  }
  //-----------------------------------------------------------------
  else if (type==PERC_GR4JEXCH)
  {
    double x2=pHRU->GetSoilProps(m)->GR4J_x2;
    double x3=pHRU->GetSoilProps(m)->GR4J_x3;
    rates[0]=-x2*pow(max(min(stor/x3,1.0),0.0),3.5); //note - x2 can be negative (exports) or positive (imports/baseflow)
  }
  //-----------------------------------------------------------------
  else if (type==PERC_GR4JEXCH2)
  {  //refers to SOIL[1] to calculate exchange between other compartments
    double x2=pHRU->GetSoilProps(1)->GR4J_x2;
    double x3=pHRU->GetSoilProps(1)->GR4J_x3;
    int iSoil=pModel->GetStateVarIndex(SOIL,1);
    stor=state_vars[iSoil];
    max_stor= pHRU->GetSoilCapacity(1);
    rates[0]=-x2*pow(max(min(stor/x3,1.0),0.0),3.5); //note - x2 can be negative (exports) or positive (imports/baseflow)
  }
  else if (type==PERC_ASPEN)
  {
    //constant percolation but with separate rate to be kept seperate from regular percolation process
    rates[0]=pHRU->GetSoilProps(m)->perc_aspen;
  }
  //-----------------------------------------------------------------
  //-----------------------------------------------------------------
  else
  {
    ExitGracefully("CmvPercolation::GetRatesOfChange: undefined percolation type",BAD_DATA);
  }

}//end percolation routines

//////////////////////////////////////////////////////////////////
/// \brief Corrects rates of change (*rates) returned from RatesOfChange function
/// \details Ensures that the rate of flow cannot drain "from" compartment over timestep.
/// Presumes overfilling of "to" compartment is handled using cascade
///
/// \param *state_vars [in] current array of state variable values
/// \param *pHRU [in] Reference to pertinent HRU
/// \param &Options [in] Global model options information
/// \param &tt [in] Current model time strucure
/// \param *rates [out] Rate of percolation [mm/d]
//
void   CmvPercolation::ApplyConstraints(const double             *state_vars,
                                        const CHydroUnit *pHRU,
                                        const optStruct  &Options,
                                        const time_struct &tt,
                                        double     *rates) const
{
  if (pHRU->GetHRUType()==HRU_LAKE){return;}//Lakes

  double min_stor=g_min_storage;

  //cant remove more than is there
  rates[0]=threshMin(rates[0],max(state_vars[iFrom[0]]-min_stor,0.0)/Options.timestep,0.0);

  //exceedance of max "to" compartment
  //water flow simply slows (or stops) so that receptor will not overfill during tstep
  double room;
  room=threshMax(pHRU->GetStateVarMax(iTo[0],state_vars,Options)-state_vars[iTo[0]],0.0,0.0);
  rates[0]=threshMin(rates[0],room/Options.timestep,0.0);
}
