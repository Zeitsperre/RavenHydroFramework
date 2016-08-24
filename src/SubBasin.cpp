/*----------------------------------------------------------------
  Raven Library Source Code
  Copyright � 2008-2014 the Raven Development Team
----------------------------------------------------------------*/
#include "SubBasin.h"

/*****************************************************************
   Constructor/Destructor
------------------------------------------------------------------
*****************************************************************/

//////////////////////////////////////////////////////////////////
/// \brief Implementation of the subbasin constructor
///
/// \param Identifier [in] Subbasin ID
/// \param Name       [in] Subbasin name
/// \param *pMod      [in] Pointer to surface water model
/// \param down_ID    [in] Index of downstream SB, if < 0, downstream outlet
/// \param *pChan     [in] Channel
/// \param reach_len  [in] Reach length [m]
/// \param Qreference [in] Reference flow [m^3/s] [or AUTO_COMPUTE]
/// \param gaged      [in] If true, hydrographs are generated
//
CSubBasin::CSubBasin( const long  				 Identifier,
											const string         Name,
											const CModelABC			*pMod,
											const long  				 down_ID,     //index of downstream SB, if <0, downstream outlet
											const CChannelXSect *pChan,				//Channel
                      const double         reach_len,   //reach length [m]
                      const double         Qreference,  //reference flow, m3/s [or AUTO_COMPUTE]
											const bool           gaged)       //if true, hydrographs are generated
{
  ExitGracefullyIf(pMod==NULL,
    "CSubBasin:Constructor: NULL model",BAD_DATA);
	ExitGracefullyIf(((Qreference<=0.0) && (Qreference!=AUTO_COMPUTE)),
    "CSubBasin::Constructor: Reference flow must be non-zero and positive (or _AUTO)",BAD_DATA);

  _pModel=pMod;

	_ID=Identifier;
  _name=Name;

  _basin_area		=0.0;
  _drainage_area=0.0;
  _avg_ann_flow =0.0;
  _reach_length =reach_len;
  _is_headwater =true;

  _t_conc				     =AUTO_COMPUTE;
  _t_peak				     =AUTO_COMPUTE;
	_t_lag					   =AUTO_COMPUTE;
  _reservoir_constant=AUTO_COMPUTE;
  _num_reservoirs    =1;

	_nSegments  	     =1;//0;

	ExitGracefullyIf(_nSegments>MAX_RIVER_SEGS,
		"CSubBasin:Constructor: exceeded maximum river segments",BAD_DATA);

  _downstream_ID=down_ID;
  if (_downstream_ID<0){_downstream_ID=DOESNT_EXIST;}//outflow basin
  _gauged       =gaged;

  _pChannel=pChan; //Can be NULL

  _pReservoir=NULL;
  
	_nHydroUnits   =0;
  _pHydroUnits   =NULL;

	//Initialized in Initialize
	_aQout=NULL;
	_aQout=new double [_nSegments];
	ExitGracefullyIf(_aQout==NULL,"CSubBasin Constructor",OUT_OF_MEMORY);
	for (int seg=0;seg<_nSegments;seg++){
		_aQout[seg]=AUTO_COMPUTE; //can be overridden by initial conditions
	}
	_QoutLast=AUTO_COMPUTE; //can be overridden by initial conditions
	_QlatLast=AUTO_COMPUTE; //can be overridden by initial conditions
  _channel_storage=0.0;   //calculated from initial flows
  _rivulet_storage=0.0;   //calculated from initial flows

	//Below are initialized in GenerateCatchmentHydrograph, GenerateRoutingHydrograph  
  _aQlatHist     =NULL;  _nQlatHist     =0;
  _aQinHist      =NULL;  _nQinHist      =0;
  _aUnitHydro    =NULL;
  _aRouteHydro   =NULL;

	//initialized in SetInflowHydrograph
	_pInflowHydro  =NULL;

  _Q_ref=Qreference;
  _c_ref=AUTO_COMPUTE;
  _w_ref=AUTO_COMPUTE;
}

//////////////////////////////////////////////////////////////////
/// \brief Implementation of the destructor
/// \details Point array references to null
//
CSubBasin::~CSubBasin()
{
  if (DESTRUCTOR_DEBUG){cout<<"  DELETING SUBBASIN"<<endl;}
  delete [] _pHydroUnits;_pHydroUnits=NULL; //just deletes pointer array, not hydrounits
	delete [] _aQout;      _aQout      =NULL;
  delete [] _aQlatHist;  _aQlatHist  =NULL;
  delete [] _aQinHist;   _aQinHist   =NULL;
  delete [] _aUnitHydro; _aUnitHydro =NULL;
  delete [] _aRouteHydro;_aRouteHydro=NULL;
  delete _pReservoir;
}
/*****************************************************************
   Accessors
*****************************************************************/

//////////////////////////////////////////////////////////////////
/// \brief Returns subbasin ID
/// \return subbasin ID
//
long  			CSubBasin::GetID               () const {return _ID;            }

//////////////////////////////////////////////////////////////////
/// \brief Returns subbasin name
/// \return subbasin name
//
string      CSubBasin::GetName						 () const {return _name;          }

//////////////////////////////////////////////////////////////////
/// \brief Returns subbasin physical area [km^2]
/// \return subbasin area [km^2]
//
double			CSubBasin::GetBasinArea        () const {return _basin_area;    }//[km2]

//////////////////////////////////////////////////////////////////
/// \brief Returns SB drainage area (includes basin and upstream contributing areas) [km^2]
/// \return SB drainage area [km^2]
//
double			CSubBasin::GetDrainageArea     () const {return _drainage_area; }//[km2]

//////////////////////////////////////////////////////////////////
/// \brief Returns average annual flowrate within reach [m^3/s]
/// \return Average annual flowrate within reach [m^3/s]
//
double			CSubBasin::GetAvgAnnualFlow    () const {return _avg_ann_flow;  }//[m3/s]

//////////////////////////////////////////////////////////////////
/// \brief Returns ID of downstream subbasin
/// \return ID of downstream subbasin (or DOESNT_EXIST if there is none)
//
long			CSubBasin::GetDownstreamID     () const {return _downstream_ID; }

//////////////////////////////////////////////////////////////////
/// \brief Returns reach length [m]
/// \return Reach length [m]
//
double      CSubBasin::GetReachLength      () const {return _reach_length;  }//[m] 

//////////////////////////////////////////////////////////////////
/// \brief Returns True if subbasin is gauged, false otherwise
/// \return Boolean indicating if subbasin is gauged
//
bool				CSubBasin::IsGauged            () const {return _gauged;        }

//////////////////////////////////////////////////////////////////
/// \brief Returns number of river segments used in routing
/// \return Number of river segments used in routing
//
int					CSubBasin::GetNumSegments			 () const {return _nSegments;	 }

//////////////////////////////////////////////////////////////////
/// \brief returns Unit Hydrograph as array pointer
/// \return Unit Hydrograph as array pointer
//
const double        *CSubBasin::GetUnitHydrograph    () const{return _aUnitHydro;}

//////////////////////////////////////////////////////////////////
/// \brief returns routing Hydrograph as array pointer
/// \return routing Hydrograph as array pointer
//
const double        *CSubBasin::GetRoutingHydrograph () const{return _aRouteHydro;}

//////////////////////////////////////////////////////////////////
/// \brief returns number of timesteps stored in unit hydrograph history
/// \return number of timesteps stored in unit hydrograph history
//
int                  CSubBasin::GetLatHistorySize    () const{return _nQlatHist;}

//////////////////////////////////////////////////////////////////
/// \brief returns number of timesteps stored in routing hydrograph history
/// \return number of timesteps stored in routing hydrograph history
//
int                  CSubBasin::GetInflowHistorySize () const{return _nQinHist;}

//////////////////////////////////////////////////////////////////
/// \brief Returns Number of HRUs in SB
/// \return Number of HRUs in SB
//
int				          	CSubBasin::GetNumHRUs					 () const{return _nHydroUnits;   }

//////////////////////////////////////////////////////////////////
/// \brief Returns HRU corresponding to index k
/// \param k [in] Index correspondng to the HRU of interest
/// \return HRU corresponding to index k
//
const CHydroUnit*CSubBasin::GetHRU				 (const int k) const 
{
#ifdef _STRICTCHECK_ 
  ExitGracefullyIf((k<0) && (k>=_nHydroUnits),"CSubBasin:GetHRU::improper index",BAD_DATA);
#endif
  return _pHydroUnits[k];
}
//////////////////////////////////////////////////////////////////
/// \brief Returns pointer to reservoir associated with Subbasin (or NULL)
/// \return  pointer to reservoir associated with Subbasin (or NULL if no reservoir)
//
const CReservoir    *CSubBasin::GetReservoir () const
{
  return _pReservoir;
}
//////////////////////////////////////////////////////////////////
/// \brief Returns area-weighted average value of state variable with index i over all HRUs
/// \param i [in] Index corresponding to a state variable
/// \return area-weighted average value of state variable with index i over all HRUs
//
double    CSubBasin::GetAvgStateVar   (const int i) const
{
  ExitGracefullyIf((i<0) && (i>=_pModel->GetNumStateVars()),
    "CSubBasin:GetAverageStateVar::improper index",BAD_DATA);
  double sum=0.0;
  for (int k=0;k<_nHydroUnits;k++)
  {
    sum+=_pHydroUnits[k]->GetStateVarValue(i)*_pHydroUnits[k]->GetArea();
  }
  return sum/_basin_area;
}

//////////////////////////////////////////////////////////////////
/// \brief Returns area-weighted average value of forcing function with identifier forcing_string over entire subbasin
/// \param &forcing_string [in] Identifier corresponding to a forcing function
/// \return area-weighted average value of forcing function with identifier forcing_string over entire subbasin
//
double CSubBasin::GetAvgForcing (const string &forcing_string) const
{
  double sum=0.0;
  for (int k=0;k<_nHydroUnits;k++)
  {
    sum    +=_pHydroUnits[k]->GetForcing(forcing_string)*_pHydroUnits[k]->GetArea();
  }
  return sum/_basin_area;
}

//////////////////////////////////////////////////////////////////
/// \brief Returns specified inflow to subbasin at time t
/// \param &t [in] Model time at which the inflow to SB is to be determined
/// \return specified inflow to subbasin at time t
//
double CSubBasin::GetSpecifiedInflow(const double &t) const
{
	if (_pInflowHydro==NULL){return 0.0;}	
	return _pInflowHydro->GetValue(t);
}

//////////////////////////////////////////////////////////////////
/// \brief Returns channel storage [m^3]
/// \note Should only be called after _aQinHist has been updated by calling SetInflow
/// \return Channel storage [m^3]
//
double CSubBasin::GetChannelStorage () const
{
  if (_pReservoir!=NULL){return _channel_storage+_pReservoir->GetStorage();}
  return _channel_storage;  
}

//////////////////////////////////////////////////////////////////
/// \brief Returns rivulet storage [m^3]
/// \details Returns uphill tributary storage [m^3] obtained from convoltuion
/// of inflow history with unit hydrograph
/// \note Should only be called after _aQlatHist has been updated by calling SetLateralInflow
///
/// \return Current Rivulet (non-main channel) surface water storage
//
double CSubBasin::GetRivuletStorage () const
{
  return _rivulet_storage;
}

//////////////////////////////////////////////////////////////////
/// \brief Returns Outflow at start of current timestep (during solution) or end of completed timestep [m^3/s]
/// \return Outflow at start of current timestep (during solution) or end of completed timestep [m^3/s]
//
double    CSubBasin::GetOutflowRate   () const 
{
  if (_pReservoir!=NULL){return _pReservoir->GetOutflowRate();}
	return _aQout[_nSegments-1]; //[m3/s](from start of time step until after solver is called)
}
//////////////////////////////////////////////////////////////////
/// \brief Returns Outflow at start of current timestep (during solution) or end of completed timestep [m^3/s]
/// \return Outflow at start of current timestep (during solution) or end of completed timestep [m^3/s]
//
double  CSubBasin::GetReservoirInflow() const
{
  if (_pReservoir==NULL){return 0.0;}
	return _aQout[_nSegments-1]; //[m3/s](from start of time step until after solver is called)
}
//////////////////////////////////////////////////////////////////
/// \brief Returns total volume lost from main reach over timestep [m^3]
/// \note Should be called only at end of completed tstep
/// \return Total volume lost from main reach over timestep [m^3]
//
double CSubBasin::GetIntegratedOutflow(const double &tstep) const//[m3]
{
  if (_pReservoir!=NULL){return _pReservoir->GetIntegratedOutflow(tstep);}
  //used in mass balance to estimate water loss through streamflow
  return 0.5*(_aQout[_nSegments-1]+_QoutLast)*(tstep*SEC_PER_DAY); //integrated
}
//////////////////////////////////////////////////////////////////
/// \brief Returns total volume lost from main reach to reservoir over timestep [m^3]
/// \note Should be called only at end of completed tstep
/// \return Total volume lost from main reach over timestep [m^3]
//
double CSubBasin::GetIntegratedReservoirInflow(const double &tstep) const
{
  if (_pReservoir==NULL){return 0.0;}
  return 0.5*(_aQout[_nSegments-1]+_QoutLast)*(tstep*SEC_PER_DAY); //integrated
}
//////////////////////////////////////////////////////////////////
/// \brief Returns total volume gained by main reach from unmodeled sources over timestep [m^3]
/// \note Should be called only at end of completed tstep
/// \return Total volume gained by main reach from unmodeled sources over timestep [m^3]
//
double CSubBasin::GetIntegratedSpecInflow(const double &t, const double &tstep) const//[m3]
{
  //used in mass balance to estimate water gain from unmodeled upstream sources
  return 0.5*(GetSpecifiedInflow(t)+GetSpecifiedInflow(t+tstep))*(tstep*SEC_PER_DAY); //integrated
}
//////////////////////////////////////////////////////////////////
/// \brief Returns reference discharge [m^3]
/// \return  reference discharge [m^3] or AUTO_COMPUTE if not yet calculated
//
double CSubBasin::GetReferenceFlow() const
{
  return _Q_ref;
}
/*****************************************************************
   Manipulators
*****************************************************************/

//////////////////////////////////////////////////////////////////
/// \brief Adds an HRU to HRU coverage of subbasin
/// \param *pHRU [in] HRU to be added
//
void CSubBasin::AddHRU(CHydroUnit *pHRU)
{
  if (!DynArrayAppend((void**&)(_pHydroUnits),(void*)(pHRU),_nHydroUnits)){
   ExitGracefully("CSubBasin::AddHRU: adding NULL HRU",BAD_DATA);} 
}

//////////////////////////////////////////////////////////////////
/// \brief Adds a reservoir to the outlet of the basin
/// \param *pRes [in] pointer to reservoir to be added
//
void CSubBasin::AddReservoir(CReservoir *pRes)
{
	ExitGracefullyIf(_pReservoir!=NULL,
		"CSubBasin::AddReservoir: only one inflow reservoir may be specified per basin",BAD_DATA);
  _pReservoir=pRes;
}


//////////////////////////////////////////////////////////////////
/// \brief Sets basin properties
/// \param label [in] String property identifier
/// \param &value [in] Property set value
/// \return True if procedure was successful
//
bool CSubBasin::SetBasinProperties(const string label, 
                                    const double &value)
{
  string label_n = StringToUppercase(label);
  if      (!label_n.compare("TIME_CONC"     ))  {_t_conc=value;}
  else if (!label_n.compare("TIME_TO_PEAK"  ))  {_t_peak=value;}
  else if (!label_n.compare("TIME_LAG"      ))  {_t_lag=value;}
  else if (!label_n.compare("RES_CONSTANT"  ))  {_reservoir_constant=value;}
  else if (!label_n.compare("NUM_RESERVOIRS"))  {_num_reservoirs=(int)(value);}
  else if (!label_n.compare("Q_REFERENCE"   ))  {_Q_ref=value;}
  else{
    return false;//bad string
  }
  return true;
}
//////////////////////////////////////////////////////////////////
/// \brief Sets basin headwater status (called by model during subbasin initialization)
//
void CSubBasin::SetAsNonHeadwater()
{
  _is_headwater=false;
}
//////////////////////////////////////////////////////////////////
/// \brief Adds inflow hydrograph
/// \param *pInflow Inflow time series to be added
//
void	CSubBasin::AddInflowHydrograph (CTimeSeries *pInflow)
{
	ExitGracefullyIf(_pInflowHydro!=NULL,
		"CSubBasin::AddInflowHydrograph: only one inflow hydrograph may be specified per basin",BAD_DATA);
	_pInflowHydro=pInflow;
}

//////////////////////////////////////////////////////////////////
/// \brief Adds reservoir extraction
/// \param *pOutflow Outflow time series to be added
//
void	CSubBasin::AddReservoirExtract (CTimeSeries *pOutflow)
{

  if (_pReservoir!=NULL){
    _pReservoir->AddExtractionTimeSeries(pOutflow);
  }
  else{
    WriteWarning("Reservoir extraction history specified for basin without reservoir",false);
  }
}


//////////////////////////////////////////////////////////////////
/// \brief sets (usually initial) reservoir flow rate & stage
/// \param Q [in] flow rate [m3/s]
//
void	CSubBasin::SetReservoirFlow(const double &Q)
{
  if (_pReservoir==NULL){
    WriteWarning("CSubBasin::SetReservoirFlow: trying to set flow for non-existent reservoir.",false);return;
  }
  _pReservoir->SetInitialFlow(Q);
}
//////////////////////////////////////////////////////////////////
/// \brief sets reservoir stage (and recalculates flow rate)
/// \param stage [m]
//
void	CSubBasin::SetReservoirStage(const double &stage)
{
  if (_pReservoir==NULL){
    WriteWarning("CSubBasin::SetReservoirStage: trying to set stage for non-existent reservoir.",false);return;
  }
  _pReservoir->UpdateStage(stage);
}
/////////////////////////////////////////////////////////////////
/// \brief Sets channel storage, usually upon read of state file
/// \param &V [in] channel storage [m3]
//
void CSubBasin::SetChannelStorage   (const double &V){
  _channel_storage=V;
}
/////////////////////////////////////////////////////////////////
/// \brief Sets rivulet storage, usually upon read of state file
/// \param &V [in] rivulet storage [m3]
//
void CSubBasin::SetRivuletStorage   (const double &V){
  _rivulet_storage=V;
}
/////////////////////////////////////////////////////////////////
/// \brief Sets qout storage array, usually upon read of state file
/// \param &N [in] size of aQo array (=_nSegments)
/// \param *aQo array (size N) outflow from each reach segment [m3/s] 
/// \param QoLast: most recent outflow from reach [m3/s] 
//
void CSubBasin::SetQoutArray(const int N, const double *aQo, const double QoLast)
{
  if (N!=_nSegments){
    WriteWarning("Number of reach segments in state file and input file are inconsistent. Unable to read in-reach flow initial conditions",false);
  }
  else{
    for (int i=0;i<_nSegments;i++){_aQout[i]=aQo[i];}
    _QoutLast=QoLast;
  }
}
/////////////////////////////////////////////////////////////////
/// \brief Sets Qlat storage array, usually upon read of state file
/// \param &N [in] size of aQl array (=_nQlatHist)
/// \param *aQl array (size N) history of lateral loss rates from HRUs, water still in subbasin [m3/s] 
/// \param QoLast: most recent lateral inflow to reach [m3/s] 
//
void CSubBasin::SetQlatHist(const int N, const double *aQl, const double QlLast)
{
  if (_aQlatHist!=NULL){
    ExitGracefully("CSubBasin::SetQlatHist: should not overwrite existing history array. Improper use.",RUNTIME_ERR);
  }
  _nQlatHist=N;
  _aQlatHist=new double [_nQlatHist];
  for (int i=0;i<_nQlatHist;i++){_aQlatHist[i]=aQl[i];}
  _QlatLast=QlLast;
}
/////////////////////////////////////////////////////////////////
/// \brief Sets Qin storage array, usually upon read of state file
/// \param &N [in] size of aQi array (=_nQinHist)
/// \param *aQi array (size N) recent history of inflow to reach [m3/s] 
/// \param QoLast: most recent lateral inflow to reach [m3/s] 
//
void CSubBasin::SetQinHist          (const int N, const double *aQi)
{
  if (_aQinHist!=NULL){
    ExitGracefully("CSubBasin::SetQinHist: should not overwrite existing history array. Improper use.",RUNTIME_ERR);
  }
  _nQinHist=N;
  _aQinHist=new double [_nQinHist];
  for (int i=0;i<_nQinHist;i++){_aQinHist[i]=aQi[i];}
}
//////////////////////////////////////////////////////////////////
/// \brief Calculates subbasin area as a sum of HRU areas
/// \remark Called in CModel::Initialize
/// \note After CModel::Initialize, GetBasinArea should be used
/// \return subbasin Area [km^2]
//

double    CSubBasin::CalculateBasinArea()
{
  _basin_area=0.0;
  for (int k=0;k<_nHydroUnits;k++)
	{
    ExitGracefullyIf(_pHydroUnits[k]->GetArea()<=0.0,
      "CSubBasin::CalculateBasinArea: one or more HRUs has a negative or zero area",BAD_DATA);
    _basin_area+=_pHydroUnits[k]->GetArea();
  }
  ExitGracefullyIf(_nHydroUnits==0,
    "CSubBasin::CalculateBasinArea: one or more subbasins has zero constituent HRUs", BAD_DATA); 
  ExitGracefullyIf(_basin_area<=0,
    "CSubBasin::CalculateBasinArea: negative or zero subbasin area!", BAD_DATA); 

  return _basin_area;
}

//////////////////////////////////////////////////////////////////
/// \brief Initializes SB attributes
/// \details Calculates total subbasin area; initializes _basin_area, _drainage_area, 
/// sets initial flow conditions based on annual avg. rainfall and upstream flows,
/// reserves memory for Qin, Qlat history storage, generates Unit hydrographs
//
/// \note Should be called only after basins are ordered from upstream to 
/// downstream, in order from upstream to down
///
/// \param &Qin_avg [in] Average inflow from upstream [m^3/s]
/// \param &Qlat_avg [in] Average lateral inflow [m^3/s]
/// \param &total_drain_area [in] Total drainage area [km^2]
/// \param &Options [in] Global model options information
//
void CSubBasin::Initialize(const double    &Qin_avg,          //[m3/s] from upstream
                           const double    &Qlat_avg,         //[m3/s] from lateral inflow
                           const double    &total_drain_area, //[km2]  
                           const optStruct &Options)
{
	int seg;

  ExitGracefullyIf(_nHydroUnits==0,
    "CSubBasin::Initialize: a SubBasin with no HRUs has been found", BAD_DATA); 
  ExitGracefullyIf((_pChannel==NULL) && (Options.routing!=ROUTE_NONE) ,
      "CSubBasin::Initialize: channel profile for basin may only be 'NONE' if Routing=ROUTE_NONE",BAD_DATA);
  
  if (_pInflowHydro != NULL){_is_headwater=false;}

  _drainage_area=total_drain_area;  

  //set reference flow in non-headwater basins
  //------------------------------------------------------------------------
  if (_Q_ref==AUTO_COMPUTE)
  {
    if (((Qin_avg + Qlat_avg) <= 0) && (!_is_headwater)){//reference flow only matters for non-headwater basins
      ExitGracefully("CSubBasin::Initialize: negative or zero average flow specified in initialization.",BAD_DATA);
    }
    ResetReferenceFlow(10.0*(Qin_avg+Qlat_avg)); //VERY APPROXIMATE - much better to specify!
  }
  else{
    ResetReferenceFlow(_Q_ref);
  }
  
  // estimate reach length if needed
  //------------------------------------------------------------------------
  if (_reach_length==AUTO_COMPUTE)
  { 
    //_reach_length =0.6581*pow(_basin_area,1.0317)*M_PER_KM;//[m] // \ref B. Annable, personal comm, 2009 (units wrong?)
    _reach_length =pow(_basin_area,0.67)*M_PER_KM;//[m] // \ref from Grand river data, JRC 2010
  }

  //estimate avg annual flow due to rainfall in this basin & upstream flows
  //------------------------------------------------------------------------
	_avg_ann_flow=Qin_avg+Qlat_avg;

  //Set initial conditions for flow history variables (if they weren't set in .rvc file)
  //------------------------------------------------------------------------
  for (seg=0;seg<_nSegments;seg++){
    if (_aQout[seg]==AUTO_COMPUTE){
		  _aQout[seg]=Qin_avg+Qlat_avg*(double)(seg+1)/(double)(_nSegments);
    }
	}
  if (_QoutLast==AUTO_COMPUTE){
	  _QoutLast    =_aQout[_nSegments-1];
  }
  if (_QlatLast==AUTO_COMPUTE){
	  _QlatLast    =Qlat_avg;
  }

  ///< \ref from Williams (1922), as cited in Handbook of Hydrology, eqn. 9.4.3 \cite williams1922
  if (_t_conc==AUTO_COMPUTE){
    //_t_conc=14.6*_reach_length/M_PER_KM*pow(_basin_area,-0.1)*pow( [[AVERAGE VALLEY SLOPE???]],-0.2)/MIN_PER_DAY;
		_t_conc=0.76/24*pow(_basin_area,0.38);// \ref Austrailian Rainfall and runoff
  }
  if (_t_peak==AUTO_COMPUTE){
    _t_peak=0.3*_t_conc;/// \todo [fix hack] better means of determining needed
  }
	if (_t_lag==AUTO_COMPUTE){
		_t_lag=0.0;
	}
  if (_reservoir_constant==AUTO_COMPUTE){
    _reservoir_constant=-log(_t_conc/(1+_t_conc));
  }
  ExitGracefullyIf(_t_conc<_t_peak,
    "CSubBasin::Initialize: time of concentration must be greater than time to peak",BAD_DATA);
  ExitGracefullyIf(_t_peak<=0,
    "CSubBasin::Initialize: time to peak must be greater than zero",BAD_DATA);
  ExitGracefullyIf(_t_conc<=0,
    "CSubBasin::Initialize: time of concentration must be greater than zero",BAD_DATA);

	//Calculate Initial Channel Storage from flowrate
  //------------------------------------------------------------------------
	_channel_storage=0.0;
  if (Options.routing!=ROUTE_NONE)
  {
    for (seg=0;seg<_nSegments;seg++)
    {
		  _channel_storage+=_pChannel->GetArea(_aQout[seg])*(_reach_length/_nSegments); //[m3]
	  }
    //cout<<"    *initial channel_storage:"<<_channel_storage<<" m3, RL: "<<_reach_length/1000<<" km, _aQout:"<<aQout[_nSegments-1]<<"m3/s Area:"<<_pChannel->GetArea(aQout[num_segments-1])<<endl;
  }

  //generate catchment & routing hydrograph weights
  //reserves memory and populates _aQinHist,_aQlatHist,_aRouteHydro
  //------------------------------------------------------------------------
  GenerateCatchmentHydrograph(Qlat_avg,Options);

  GenerateRoutingHydrograph  (Qin_avg, Options);

  //initialize rivulet storage
  //------------------------------------------------------------------------
  double sum(0.0);
  for (int n=0;n<_nQlatHist;n++)
	{
    sum+=(n)*_aUnitHydro[n]; 
	}
  _rivulet_storage=sum*Qlat_avg*(Options.timestep*SEC_PER_DAY);//[m3];

  //initialize reservoir
  //------------------------------------------------------------------------
  if (_pReservoir!=NULL){
    _pReservoir->Initialize(Options);
  }

  if (_pInflowHydro != NULL){
    _pInflowHydro->Initialize(Options.julian_start_day ,Options.julian_start_year,Options.duration,Options.timestep,false);
  }

  //Check Muskingum parameters, if necessary
  //------------------------------------------------------------------------
  if ((Options.routing==ROUTE_MUSKINGUM) || (Options.routing==ROUTE_MUSKINGUM_CUNGE)) 
  {
    double dx					=_reach_length/(double)(_nSegments);
    double K=GetMuskingumK(dx);
    double X=GetMuskingumK(dx);
    if ((Options.timestep<2*K*X) || (Options.timestep>2*K*(1-X)))
    {
      string warn;
      if (Options.timestep<2*K*X){
        warn ="CSubBasin::Initialize: inappropriate global time step for Muskingum routing in subbasin "+_name+" timestep too small, must increase # of reach segments";
      }
      else{
        warn="CSubBasin::Initialize: inappropriate global time step for Muskingum routing in subbasin "+_name+": local timestepping will be used";
      }
      WriteWarning(warn,Options.noisy);
      if (Options.noisy){
        cout<<warn<<endl;
        cout<<"   For stability, the following condition should hold:"<<endl;
        cout<<"   2KX < dt < 2K(1-X): "<< 2*K*X<<" < "<<Options.timestep<< " < " << 2*K*(1-X)<<endl;
        cout<<"   where K="<<K<<" X="<<X<<" dt="<<Options.timestep<<endl<<endl;
      }
    }
  }
}

/////////////////////////////////////////////////////////////////
/// \brief Resets reference flow
/// \details Resets celerity and channel top width at reference flow based on reference flow
/// \param &Qreference [in] Reference flow
//
void CSubBasin::ResetReferenceFlow(const double &Qreference)
{
  _Q_ref=Qreference;
  if ((_Q_ref!=AUTO_COMPUTE) && (_pChannel!=NULL))
  {
    if ((_Q_ref <= 0.0) && (!_is_headwater)){
      cout << "_Q_ref=" << _Q_ref << endl;
      ExitGracefully("CSubBasin::ResetReferenceFlow: invalid (negative or zero) reference flow rate in non-headwater basin", BAD_DATA);
    }
    _c_ref=_pChannel->GetCelerity(_Q_ref);
    _w_ref=_pChannel->GetTopWidth(_Q_ref);
  }
  else
  {
    _c_ref=AUTO_COMPUTE;
    _w_ref=AUTO_COMPUTE;
  }
}

//////////////////////////////////////////////////////////////////
/// \brief Generates routing (channel) unit hydrograph for subbasin
/// \details generates channel transfer function, reserves memory for and 
/// populates _aQinHist,_aRouteHydro
/// \remark Called once for each basin from CSubBasin::initialize
///
/// \param &Qin_avg [in] estimated Average inflow [m^3/s]
/// \param &Options [in & out] Global model options information
//
void CSubBasin::GenerateRoutingHydrograph(const double &Qin_avg,
                                          const optStruct &Options)
{
  int n;
  double tstep=Options.timestep;
  double travel_time; //duration for wave to traverse reach
  int OldnQinHist=_nQinHist;

  travel_time=_reach_length/_c_ref/SEC_PER_DAY; //[day]
  
  if      (Options.routing==ROUTE_PLUG_FLOW)
  {
    _nQinHist=(int)(ceil(travel_time/tstep))+2;
  }
  else if (Options.routing==ROUTE_DIFFUSIVE_WAVE)
  {
    _nQinHist=(int)(ceil(2*travel_time/tstep))+2;//really a function of reach length and diffusivity
  }
  else if ((Options.routing==ROUTE_HYDROLOGIC))
  {
    _nQinHist=2;
  }
  else
  {
    //_nQinHist=(int)(ceil(2*travel_time/tstep))+2;
    _nQinHist=20;
  }

  bool bad_initcond=((_nQinHist!=OldnQinHist) && (OldnQinHist!=0));

  if (bad_initcond){
    cout<<_nQinHist<<" "<<OldnQinHist<<endl;
    WriteWarning("CSubBasin::GenerateRoutingHydrograph: size of inflow history array differs between initial conditions file and calculated size. Initial conditions will be overwritten",Options.noisy);
    delete [] _aQinHist;
  }

  //reserve memory, initialize
  if ((_aQinHist==NULL) || (bad_initcond)) //may have been previously read from file
  {
    _aQinHist   =new double [_nQinHist+1 ];
    for (n=0;n<_nQinHist;n++){_aQinHist[n]=Qin_avg; }
  }

  _aRouteHydro=new double [_nQinHist+1 ];
  for (n=0;n<_nQinHist;n++){_aRouteHydro[n]=0.0;}

  double sum,t;
  //---------------------------------------------------------------
  if (Options.routing==ROUTE_PLUG_FLOW)
  {
    double ts;
    for (n=0;n<_nQinHist-1;n++)
    {
      ts=(travel_time-n*tstep)/tstep;
			if ((ts>=0.0) && (ts<1.0)){
        _aRouteHydro[n  ]=1-ts;
        _aRouteHydro[n+1]=ts;
      }
    }
  }
  //---------------------------------------------------------------
  else if (Options.routing==ROUTE_DIFFUSIVE_WAVE)
  {
    ExitGracefullyIf(_nSegments>1,
			"ROUTE_DIFFUSIVE_WAVE only valid for single-segment rivers",BAD_DATA);
    sum=0.0;
		double cc=_c_ref*SEC_PER_DAY; //[m2/day]
    double diffusivity=_pChannel->GetDiffusivity(_Q_ref);
		diffusivity*=SEC_PER_DAY;//convert to m2/d
    for (n=0;n<_nQinHist;n++)
    {
      t=(n-1)*tstep;
      _aRouteHydro[n  ]=ADRCumDist(t+tstep,_reach_length,cc,diffusivity)-sum;
      sum+=_aRouteHydro[n];
    }
    _aRouteHydro[_nQinHist-1]=0.0;//must truncate infinite distrib.
  }
  //---------------------------------------------------------------
  else
  {
    for (n=0;n<_nQinHist;n++)
    {
      _aRouteHydro[n  ]=1/(double)(_nQinHist);
    }
  }

  //correct to ensure that sum _aRouteHydro[m]=1.0
  sum=0.0;
	for (n=0;n<_nQinHist;n++){sum+=_aRouteHydro[n];}
  ExitGracefullyIf(sum==0.0,"CSubBasin::GenerateRoutingHydrograph: bad routing hydrograph constructed",RUNTIME_ERR);
  for (n=0;n<_nQinHist;n++){_aRouteHydro[n]/=sum;}
}

//////////////////////////////////////////////////////////////////
/// \brief Generates catchment hydrograph for subbasin
/// \details generates catchment hydrographs, reserves memory for and 
/// populates aQiHist,_aUnitHydro
/// \remark Called once for each basin from CSubBasin::initialize
///
/// \param &Qlat_avg [in] Estimate of (steady state) inflow [m^3/s]
/// \param &Options [in & out] Global model options information
//
void CSubBasin::GenerateCatchmentHydrograph(const double    &Qlat_avg,
                                            const optStruct &Options)
{
  int n;
  double tstep=Options.timestep;
	double t;
 
  int OldnQlatHist=_nQlatHist;

  if ((Options.catchment_routing==ROUTE_TRI_CONVOLUTION) ||
      (Options.catchment_routing==ROUTE_GAMMA_CONVOLUTION))
  {
    _nQlatHist=(int)(ceil((_t_conc)/tstep))+3;
  }
  else if (Options.catchment_routing==ROUTE_DELAYED_FIRST_ORDER)
  {
    _nQlatHist=2;
  }
  else if (Options.catchment_routing==ROUTE_DUMP)
  {
    _nQlatHist=3;
  }
  else if (Options.catchment_routing==ROUTE_RESERVOIR_SERIES)
  {
    _nQlatHist=(int)(ceil((4.0/_reservoir_constant)/tstep))*_num_reservoirs+2; //approximate
  }

  //add additional history required for lag
  _nQlatHist+=(int)(ceil(_t_lag/tstep));

  bool bad_initcond=((OldnQlatHist!=_nQlatHist) && (OldnQlatHist!=0));

  if (bad_initcond){
    WriteWarning("CSubBasin::GenerateCatchmentHydrograph: size of lateral inflow history array differs between initial conditions file and calculated size. Initial conditions will be overwritten",Options.noisy);
    delete [] _aQlatHist;
  }

  //reserve memory, initialize
  if ((_aQlatHist==NULL) || (bad_initcond)){ //may have been read from file
    _aQlatHist  =new double [_nQlatHist];
    for (n=0;n<_nQlatHist;n++){_aQlatHist [n]=Qlat_avg;}//set to initial (steady-state) conditions
  }

  _aUnitHydro =new double [_nQlatHist];
  for (n=0;n<_nQlatHist;n++){_aUnitHydro[n]=0.0;}

  //generate unit hydrograph
	double sum;
  //---------------------------------------------------------------
	if (Options.catchment_routing==ROUTE_GAMMA_CONVOLUTION)
	{
		const double GAMMA_SHAPE=3.0;/// \ref fixed as in Clark et. al 2007
		sum=0;
		for (n=0;n<_nQlatHist;n++)
		{
			t=n*tstep-_t_lag;
			_aUnitHydro[n]=GammaCumDist((t+tstep)/_t_peak*GAMMA_SHAPE,GAMMA_SHAPE)-sum;
      ExitGracefullyIf(!_finite(_aUnitHydro[n]),
        "GenerateCatchmentHydrograph: issues with gamma distribution. Time to peak may be too small relative to timestep",RUNTIME_ERR);
      
			sum+=_aUnitHydro[n];
		}
    _aUnitHydro[_nQlatHist-1]=0.0;//must truncate infinite distribution
  }
  //---------------------------------------------------------------
	else if (Options.catchment_routing==ROUTE_TRI_CONVOLUTION)
	{
		sum=0.0;
		for (n=0;n<_nQlatHist;n++)
		{
			t=n*tstep-_t_lag;
			_aUnitHydro[n]=TriCumDist(t+tstep,_t_conc,_t_peak)-sum;
			sum+=_aUnitHydro[n];
		}
	}
  //---------------------------------------------------------------
	else if (Options.catchment_routing==ROUTE_RESERVOIR_SERIES)
	{
		sum=0.0;
		for (n=0;n<_nQlatHist;n++)
		{
			t=n*tstep-_t_lag;
			_aUnitHydro[n]=NashCumDist(t+tstep,_reservoir_constant,_num_reservoirs)-sum;
			sum+=_aUnitHydro[n];
    }
  }
  //---------------------------------------------------------------
	else if (Options.catchment_routing==ROUTE_DUMP)
	{ 
		for (n=0;n<_nQlatHist;n++){_aUnitHydro[n]=0.0;}
		_aUnitHydro[0]=1.0;
	}
  //---------------------------------------------------------------

	//correct to ensure that area under _aUnitHydro[n]=1.0
  //---------------------------------------------------------------
	sum=0.0;
  for (n=0;n<_nQlatHist;n++){sum+=_aUnitHydro[n];}
	ExitGracefullyIf(sum==0.0,"CSubBasin::GenerateCatchmentHydrograph: bad unit hydrograph constructed",RUNTIME_ERR);
	for (n=0;n<_nQlatHist;n++){_aUnitHydro[n]/=sum;}
}

//////////////////////////////////////////////////////////////////
/// \brief Sets inflow into primary channel and updates flow history
/// \remark Called *before* RouteWater routine in Solver.cpp, updating the 
/// input flow rates to the valid values so that _aQinHist[0] 
/// the *current* inflow rate needed for RouteWater and the history is properly stored
/// \note This water will eventually reach the reach outflow, but possibly 
/// after a lag or convolution (due to channel/rivulet storage within 
/// the basin)
///
/// \param &Qin [in] New flow [m^3/s]
//
void CSubBasin::SetInflow    (const double &Qin)//[m3/s]
{
  for (int n=_nQinHist-1;n>0;n--){
    _aQinHist[n]=_aQinHist[n-1];
  }
  _aQinHist[0]=Qin;
}
//////////////////////////////////////////////////////////////////
/// \brief updates flow algorithms based upon the time
/// \notes can later support quite generic temporal changes to treatment of flow phenom (e.g., changes to unit hydrograph)
/// \param tt [in] current model time
/// \param Options [in] model options structure
//
void  CSubBasin::UpdateFlowRules(const time_struct &tt, const optStruct &Options)
{
  if (_pReservoir != NULL){ _pReservoir->UpdateFlowRules(tt,Options); }
}
//////////////////////////////////////////////////////////////////
/// \brief Sets lateral inflow into primary channel and updates flow history
/// \remark Called *before* RouteWater routine in Solver.cpp, updating the 
/// input flow rate to the valid values so that _aQlatHist[0] are the 
/// *current* inflow rates needed for RouteWater and the history is properly stored
///
/// \param &Qlat [in] New lateral inflow [m^3/s]
//
void CSubBasin::SetLateralInflow    (const double &Qlat)//[m3/s]
{ 
  for (int n=_nQlatHist-1;n>0;n--){
    _aQlatHist[n]=_aQlatHist[n-1];
  }
  _aQlatHist[0]=Qlat;
}
//////////////////////////////////////////////////////////////////
/// \brief Sets outflow from primary channel and updates flow history
/// \details Also recalculates channel storage (uglier than desired), resets _QlatLast and _QoutLast
/// \remark Called *after* RouteWater routine is called in Solver.cpp, to reset
/// the outflow rates for this basin
///
/// \param *aQo [in] Array of new outflows [m^3/s]
/// \param &res_ht [in] new reservoir stage [m]
/// \param &Options [in] Global model options information
/// \param initialize Flag to indicate if flows are to only be initialized
//
void CSubBasin::UpdateOutflows   (const double *aQo,   //[m3/s]
                                  const double &res_ht, //[m]
                                  const optStruct &Options, bool initialize)
{
  double tstep=Options.timestep;

  //Update flows
  //------------------------------------------------------
	_QoutLast=_aQout[_nSegments-1];
	for (int seg=0;seg<_nSegments;seg++){
		_aQout[seg]=aQo[seg];
	}
  //_aQout[num_segments-1] is now the new outflow from the channel 

  if (_pReservoir!=NULL){_pReservoir->UpdateStage(res_ht);}

	if (initialize){return;}//entering initial conditions

	//Update channel storage
	//------------------------------------------------------
  double dt=tstep*SEC_PER_DAY;
	double dV=0.0;
  double Qlat_new(0.0);
  for (int n=0;n<_nQlatHist;n++){
		Qlat_new+=_aUnitHydro[n]*_aQlatHist[n];
	}	
  
	//volume change from linearly varying upstream inflow over this time step 
	dV+=0.5*(_aQinHist[0]+_aQinHist[1])*dt; 

	//volume change from linearly varying downstream outflow over this time step 
	dV-=0.5*(_aQout[_nSegments-1]+_QoutLast)*dt; 
	
	//volume change from lateral inflows over previous time step (corrects for the fact that _aQout is channel flow plus that added from lateral inflow)
	dV+=0.5*(Qlat_new+_QlatLast)*dt; 
  
	_channel_storage+=dV;//[m3]
  
	//Update rivulet storage
	//------------------------------------------------------
  dV=0.0;

  //inflow from ground surface (integrated over this time step)
  dV+=_aQlatHist[0]*dt;

  //outflow to channel (integrated over this time step)
  dV-=0.5*(Qlat_new+_QlatLast)*dt;

  _rivulet_storage+=dV;//[m3]

  /* KEEP HERE FOR TESTING
  cout.precision(4);
  cout<<"   UH: ";
  for (int i=0;i<_nQlatHist;i++){cout<<_aUnitHydro[i]<<"  ";}cout<<endl;
  cout<<"   Ql: ";
  for (int i=0;i<_nQlatHist;i++){cout<<_aQlatHist[i]<<"  ";}cout<<endl;
  cout<<"   dV: "<<dV/dt<<" ("<<_aQlatHist[0]<<" - "<< 0.5*(Qlat_new+_QlatLast)<<") [Qlatlast="<<QlatLast<<"]"<<endl;*/

  _QlatLast=Qlat_new;
}

//////////////////////////////////////////////////////////////////
/// \brief Returns Muskingum parameter, K
/// \details Calculates K based upon routing step size, dx, and the channel rating curve properties
/// \param [in] &dx Routing step size [m]
/// \return Muskingum parameter K [d]
//
double  CSubBasin::GetMuskingumK(const double &dx) const
{
  return dx/(_c_ref*SEC_PER_DAY);//[m]/([m/s]*[s/d])
}
//////////////////////////////////////////////////////////////////
/// \brief Returns Muskingum parameter, X
/// \details Calculates X based upon routing step size, dx, and the channel rating curve properties
/// \param [in] &dx Routing step size [m]
/// \return Muskingum parameter X [m]
/// \notes calculated based upon analogy to diffusion equation
//
double  CSubBasin::GetMuskingumX(const double &dx) const
{
  ///< X ranges from 0 to .5 \cite Maidment1993
  ExitGracefullyIf(_pChannel==NULL,"CSubBasin::GetMuskingumX",BAD_DATA);
  double bedslope=_pChannel->GetBedslope();
	
  return max(0.0,0.5*(1.0-_Q_ref/bedslope/_w_ref/_c_ref/dx));//[m3/s]/([m]*[m/s]*[m])
}

//////////////////////////////////////////////////////////////////
/// \brief Creates aQout_new [m^3/s], an array of point measurements for outflow at downstream end of each river segment
/// \details Array represents measurements at the end of the current timestep. if (catchment_routing==distributed), 
/// lateral flow Qlat (e.g., runoff, interflow, or baseflow) is routed as part of channel routing routine otherwise, 
/// lateral flow is all routed to most downstream outflow segment . Assumes uniform time step
///
/// \param [out] *aQout_new Array of outflows at downstream end of each segment at end of current timestep [m^3/s]
/// \param &Options [in] Global model options information
//
void CSubBasin::RouteWater(//const double &PET,           //[mm/day]
                             double *aQout_new,//[m3/s][size:_nSegments]
                             double &res_ht, //[m]
                             const optStruct &Options,
                             const time_struct &tt) const
{
	int    seg;
  double tstep;       //[d] time step
	double dx;					//[m]
	double Qlat_new;    //[m3/s] flow at end of timestep to reach from catchment storage
  double seg_fraction;//[0..1] % of reach length assoc with segment

  tstep       =Options.timestep;
	dx					=_reach_length/(double)(_nSegments);
  seg_fraction=1.0/(double)(_nSegments);

  //==============================================================
  // route from catchment
  //==============================================================
	Qlat_new=0.0;
  for (int n=0;n<_nQlatHist;n++){
    Qlat_new+=_aUnitHydro[n]*_aQlatHist[n];
  }
	
  double Qlat_last=0;
  for (int n=0;n<_nQlatHist-1;n++){
    Qlat_last+=_aUnitHydro[n]*_aQlatHist[n+1];
  }
  routing_method route_method;
  if (_is_headwater){route_method=ROUTE_NONE;}
  else              {route_method=Options.routing;}

  //==============================================================
  // route in channel
  //==============================================================
  if ((route_method==ROUTE_MUSKINGUM) || 
      (route_method==ROUTE_MUSKINGUM_CUNGE))
  {
    double K,X,c1,c2,c3,c4,denom,cunge;
    K=GetMuskingumK(dx);
		X=GetMuskingumX(dx);
		
		//may need local time-stepping (2*KX<dt<2*K(1-X))
    //Local time stepping works great if dt is too large, but no fix (other than adding more segments) if dt is too small!
    double dt,Qin,Qin_new;
		dt=min(K,tstep);
		//dt=tstep;

    static double aQoutStored[100];//100=max num segments
		for (seg=0;seg<_nSegments;seg++){aQoutStored[seg]=_aQout[seg];}
		//cout<<"check: "<< 2*K*X<<" < "<<dt<< " < " << 2*K*(1-X)<<" K="<<K<<" X="<<X<<" dt="<<dt<<endl;
		for (double t=0;t<tstep;t+=dt)//Local time-stepping
		{
			if (dt>(tstep-t)){dt=tstep-t;}
			cunge=0;
			if ((route_method==ROUTE_MUSKINGUM_CUNGE) && 
          (!Options.distrib_lat_inflow)){cunge=1;} 

			//Standard Muskingum/Muskingum Cunge
			denom=(2*K*(1.0-X)+dt);
			c1 = ( dt-2*K*(  X)) / denom;
			c2 = ( dt+2*K*(  X)) / denom;
			c3 = (-dt+2*K*(1-X)) / denom;
			c4 = ( dt          ) / denom;
		
      Qin    =_aQinHist[1]+((t   )/tstep)*(_aQinHist[0]-_aQinHist[1]);
		  Qin_new=_aQinHist[1]+((t+dt)/tstep)*(_aQinHist[0]-_aQinHist[1]);

			for (seg=0;seg<_nSegments;seg++)//move downstream
			{
				aQout_new[seg] = c1*Qin_new + c2*Qin + c3*_aQout[seg] + cunge*c4*(Qlat_new*seg_fraction); 
				if (Options.distrib_lat_inflow==true){
				  aQout_new[seg]+= (Qlat_new*seg_fraction);// more appropriate than musk-cunge?
        }
				Qin    =_aQout    [seg];
				Qin_new=aQout_new[seg];
        
        _aQout[seg]=aQout_new[seg];//only matters for dt<tstep
			}
			
		} //Local time-stepping
		for (seg=0;seg<_nSegments;seg++){_aQout[seg]=aQoutStored[seg];}
  }
  //==============================================================
  else if (route_method==ROUTE_STORAGECOEFF) 
  {
		///< Variable storage routing method \ref (Williams, 1969/ SWAT) \cite williams1969flood
		///< As interpreted from Williams, 1969/Kim and Lee, HP 2010 \cite williams1969flood
    double ttime;					//[d] reach segment travel time 
    double storage_coeff;
		double area;					//[m3] volume of water in reach at end of timestep
		double c1,c2,c3;

		double Qin_new=_aQinHist[0];
		double Qin    =_aQinHist[1];
		for (seg=0;seg<_nSegments;seg++)
		{
			area=_pChannel->GetArea(_aQout[seg]);

      ttime=GetMuskingumK(dx)*seg_fraction;// K=representative travel time for the reach segment (else storagecoeff changes and mass flows not preserved)
      storage_coeff = min(1.0 / (ttime/tstep + 0.5),1.0);

			c1=storage_coeff/2;
			c2=storage_coeff/2;
			c3=(1.0-storage_coeff);

      //new outflow proportional to old outflow without correction for /// \todo [funct]:properly handle impact of (Options.distrib_lat_inflow==true)
      double corr=1.0;
      if (Options.distrib_lat_inflow){corr=seg_fraction;}
			aQout_new[seg] = c1*Qin + c2*Qin_new + c3*(_aQout[seg]-corr*_QlatLast);
			if (Options.distrib_lat_inflow==true){
			  aQout_new[seg]+= Qlat_new*seg_fraction;
      }
			Qin    =_aQout    [seg];
			Qin_new=aQout_new[seg];
		}
  }
  //==============================================================
  else if (route_method==ROUTE_HYDROLOGIC) 
  { ///< basic hydrologic routing using Newton's algorithm
    ///<  ONE SEGMENT ONLY FOR NOW

    ///dV(Q)/dt=(Q_in(n)+Q_in(n+1))/2-(Q_out(n)+Q_out(n+1)(h))/2 -> solve for >Qout_new
    /// rewritten as f(Q)-gamma=0 for Newton's method solution

    const double ROUTE_MAXITER=20;
    const double ROUTE_TOLERANCE=0.0001;//[m3/s]

    double Qout_old =_aQout   [_nSegments-1]-Qlat_last;
    double Qin_new  =_aQinHist[0];
		double Qin_old  =_aQinHist[1];
    double V_old=_pChannel->GetArea(Qout_old)*_reach_length; 

    int    iter=0;
    double change=0;
    double gamma=V_old+(Qin_old+Qin_new-Qout_old)/2.0*(tstep*SEC_PER_DAY);//[m3]

    double dQ=0.1; //[m3/s]
    double f,dfdQ;
    double Q_guess=Qout_old;
    double relax=1.0;//0.99;

    //double Qg[ROUTE_MAXITER],ff[ROUTE_MAXITER]; //for debugging; retain
    do //Newton's method with discrete approximation of df/dQ
    {
      f   =(_pChannel->GetArea(Q_guess   )*_reach_length+(Q_guess   )/2.0*(tstep*SEC_PER_DAY));
      dfdQ=(_pChannel->GetArea(Q_guess+dQ)*_reach_length+(Q_guess+dQ)/2.0*(tstep*SEC_PER_DAY)-f)/dQ;
      change=-(f-gamma)/dfdQ;//[m3/s]
      if (dfdQ==0.0){change=1e-7;}

      //Qg[iter]=Q_guess; ff[iter]=f-gamma;//for debugging; retain

      Q_guess+=relax*change; //relaxation required for some tough cases
      if (Q_guess<0){Q_guess=0; change=0.0;}
      
      //if (Q_guess < 0){ cout << iter << ":" << Q_guess << " " << f << " " << dfdQ << " " << change << endl; }
      iter++;
      if (iter > 3){ relax = 0.9; }
      if (iter > 10){ relax = 0.7; }
    } while ((iter<ROUTE_MAXITER) && (fabs(change)>ROUTE_TOLERANCE));
    //the above is really fast for most trivial changes in flow, since f is locally linear, 1 to 3 iterations sufficient

    aQout_new[_nSegments-1]=Q_guess;
    if (Q_guess<0){ cout << "Negative flow in basin "<<to_string(this->_ID)<<" Qoutold: "<<Qout_old<<" Qin: "<<Qin_new<<" "<<Qin_old<<endl; }
    if (Options.distrib_lat_inflow){
      aQout_new[_nSegments-1]+=(seg_fraction)*Qlat_new;
    }
    if (iter==ROUTE_MAXITER){
      string warn="CSubBasin::RouteWater did not converge after "+to_string(ROUTE_MAXITER)+"  iterations for basin "+to_string(_ID)+ " flow: " +to_string(Q_guess)+" stage: "+to_string(_pChannel->GetStageElev(Q_guess));
      WriteWarning(warn,false);
      /*for (int i = 0; i < 20; i++){
        string warn = to_string(Qg[i]) + " " + to_string(ff[i]);
        WriteWarning(warn,false);
      }*/
    }
  }
  //==============================================================
  else if ((route_method==ROUTE_PLUG_FLOW) 
        || (route_method==ROUTE_DIFFUSIVE_WAVE))
  {	//Simple convolution - segmentation unused
		aQout_new[_nSegments-1]=0.0;
    for (int n=0;n<_nQinHist;n++){
      aQout_new[_nSegments-1]+=_aRouteHydro[n]*_aQinHist[n];
    }
    if (Options.distrib_lat_inflow){
      aQout_new[_nSegments-1]+=(1-seg_fraction)*Qlat_new;
    }
  }
  //==============================================================
  else if (route_method==ROUTE_NONE)
  {//In channel routing skipped, inflow directly routed out - segmentation unused
		for (seg=0;seg<_nSegments;seg++){
			aQout_new[seg]=0.0;
		}
    aQout_new[_nSegments-1]=_aQinHist[0];
    if (Options.distrib_lat_inflow){
      aQout_new[_nSegments-1]+=(1-seg_fraction)*Qlat_new;
    }
  }
  //==============================================================
  else{
    ExitGracefully("Unrecognized routing method",STUB);
  }

	if (Options.distrib_lat_inflow==false)
  {//all fluxes from catchment are routed directly to basin outlet
		aQout_new[_nSegments-1]+=Qlat_new;
  }
	else{//only last segments worth is routed directly to basin outlet
		aQout_new[_nSegments-1]+=Qlat_new*seg_fraction;
	}

  //Reservoir Routing
  //-----------------------------------------------------------------
  if (_pReservoir!=NULL)
  {
    res_ht=_pReservoir->RouteWater(_aQout[_nSegments-1],aQout_new[_nSegments-1],tstep,tt);
  }
  else
  {
    res_ht=0.0;
  }

  /*if (aQout_new[_nSegments-1]<-REAL_SMALL){
    cout<<"Lateral inflow:" <<Qlat_new<<endl;
    cout<<"Outflow: "<<aQout_new[_nSegments-1]<<endl;
    ExitGracefully("RouteWater: negative flow!",RUNTIME_ERR);
  }*/

  return;
}

//////////////////////////////////////////////////////////////////
/// \brief Returns water loss [m^3/d] due to transmission losses, evaporation losses
//
/// \param &reach_volume [in] Representative reach volume overt tsep [m^3]
/// \param &PET [in] Potential evapotranspiration [mm/d]
/// \param &Options [in] Global model options information
/// \return Double to represent water loss over time stemp [m^3/d]
//
double CSubBasin::ChannelLosses( const double &reach_volume,//[m3]-representative reach volume over tstep
                                 const double &PET,
                                 const optStruct &Options) const
{
  double Tloss;       //[m3/day] Transmission losses during timestep
  double Eloss;       //[m3/day] Evaporation losses during timestep
	double perim;				//[m]
	double top_width;		//[m]

  ExitGracefullyIf(_pChannel==NULL,"CSubBasin::ChannelLosses: NULL channel",BAD_DATA);

  perim				=_pChannel->GetWettedPerim(_aQout[_nSegments-1]);
	top_width		=_pChannel->GetTopWidth   (_aQout[_nSegments-1]);

  // calculate transmission loss rate from channel
  //--------------------------------------------------------------
  Tloss = 0.0;//_pChannel->GetChannelConductivity() * _reach_length * perim; /// \todo [add funct]: link channel conductivity to channel structure

  // calculate evaporation loss rate from channel
  //--------------------------------------------------------------
  Eloss = PET * _reach_length * top_width;

  //returns water loss over timestep [m3/day]
  return threshMin(Eloss + Tloss,reach_volume/Options.timestep,0.0);

}

/////////////////////////////////////////////////////////////////
/// \brief Write minor output 
/// \note currently unused
/// \param &tt [in] time_structure during which minor output is to be written
//
void CSubBasin::WriteMinorOutput(const time_struct &tt) const
{
}

/////////////////////////////////////////////////////////////////
/// \brief Write subbasin state variable data to solution file
/// \param &OUT [out] Output file stream to which data will be written
//
void CSubBasin::WriteToSolutionFile (ofstream &OUT) const
{
  OUT<<_name<<endl;
  OUT<<"    :ChannelStorage, "<<_channel_storage<<endl;
  OUT<<"    :RivuletStorage, "<<_rivulet_storage<<endl;
  OUT<<"    :Qout,"<<_nSegments  <<",";for (int i=0;i<_nSegments;i++){OUT<<_aQout    [i]<<",";}OUT<<_QoutLast<<endl;
  OUT<<"    :Qlat,"<<_nQlatHist  <<",";for (int i=0;i<_nQlatHist;i++){OUT<<_aQlatHist[i]<<",";}OUT<<_QlatLast<<endl;
  OUT<<"    :Qin ,"<<_nQinHist   <<",";for (int i=0;i<_nQinHist; i++){OUT<<_aQinHist [i]<<",";}OUT<<endl;
  if (_pReservoir!=NULL){
    _pReservoir->WriteToSolutionFile(OUT);
  }
}
