/*----------------------------------------------------------------
  Raven Library Source Code
  Copyright � 2008-2014 the Raven Development Team
------------------------------------------------------------------
  Capillary Rise
----------------------------------------------------------------*/

#include "HydroProcessABC.h"
#include "SoilWaterMovers.h"

/*****************************************************************
   CapillaryRise Constructor/Destructor
------------------------------------------------------------------
*****************************************************************/

//////////////////////////////////////////////////////////////////
/// \brief Implementation of the standard constructor
/// \param cr_type [in] Model of capillary rise selected
/// \param In_index [in] Soil storage unit index from which water is lost
/// \param Out_index [in] Soil storage unit index to which water rises
//
CmvCapillaryRise::CmvCapillaryRise(crise_type	cr_type,
									                  int				  In_index,			//soil water storage
									                  int			  	Out_index)		
            :CHydroProcessABC(CAPILLARY_RISE)
{
  ExitGracefullyIf(In_index==DOESNT_EXIST,
    "CmvCapillaryRise Constructor: invalid 'from' compartment specified",BAD_DATA);
  ExitGracefullyIf(Out_index==DOESNT_EXIST,
    "CmvCapillaryRise Constructor: invalid 'to' compartment specified",BAD_DATA);

  type =cr_type;
  CHydroProcessABC::DynamicSpecifyConnections(1);
  iFrom[0]=In_index; iTo[0]=Out_index; 
}

//////////////////////////////////////////////////////////////////
/// \brief Implementation of default destructor
//
CmvCapillaryRise::~CmvCapillaryRise(){} 

//////////////////////////////////////////////////////////////////
/// \brief Verifies iFrom - iTo connectivity
/// \details Ensures that water rises from soil layer/ groundwater
/// to another soil layer or groundwater unit
//
void CmvCapillaryRise::Initialize()
{
  ExitGracefullyIf((pModel->GetStateVarType(iFrom[0])!=SOIL) &&
                   (pModel->GetStateVarType(iFrom[0])!=GROUNDWATER),
    "CmvCapillaryRise::Initialize:CapillaryRise must come from soil or groundwater unit",BAD_DATA); 
  ExitGracefullyIf((pModel->GetStateVarType(iTo[0])!=SOIL) &&
                   (pModel->GetStateVarType(iTo[0])!=GROUNDWATER),
    "CmvCapillaryRise::Initialize:CapillaryRise must go to soil or groundwater unit",BAD_DATA); 
}

//////////////////////////////////////////////////////////////////
/// \brief Returns participating parameter list
///
/// \param *aP [out] array of parameter names needed for capillary rise algorithm
/// \param *aPC [out] Class type (soil, vegetation, landuse or terrain) corresponding to each parameter
/// \param &nP [out] Number of parameters required by capillary rise  algorithm (size of aP[] and aPC[])
//
void CmvCapillaryRise::GetParticipatingParamList(string  *aP , class_type *aPC , int &nP) const
{
  nP=0;
  //only needed for parameters that are not autogenerated
	if (type==CRISE_HBV) 
  { ///< HBV MODEL \cite Bergstroem1992
    nP=1;
    aP [0]="MAX_CAP_RISE_RATE";     aPC [0]=CLASS_SOIL;
  }
  else
  {
    ExitGracefully("CmvCapillaryRise::GetParticipatingParamList: undefined Capillary Rise algorithm",BAD_DATA);
  }
}

//////////////////////////////////////////////////////////////////
/// \brief Sets reference to participating state variables
/// \details User specifies from and to compartments, levels not known before construction
///
/// \param crise_type [in] Model of capillary rise used
/// \param *aSV [out] Reference to state variable types needed by cr algorithm
/// \param *aLev [out] Array of level of multilevel state variables (or DOESNT_EXIST, if single level)
/// \param &nSV [out] Number of state variables required by cr algorithm (size of aSV[] and aLev[] arrays)
//
void CmvCapillaryRise::GetParticipatingStateVarList(crise_type  btype,sv_type *aSV, int *aLev, int &nSV) 
{
  nSV=0;
  //user specified 'from' & 'to' compartment, Levels - not known before construction
}

//////////////////////////////////////////////////////////////////
/// \brief Returns rate of loss of water from soil or aquifer to another soil layer [mm/d]
/// \details if type==CRISE_HBV, flow is linearly and inversely proportional to saturation of recieving unit \cite Bergstroem1992
///
/// \param *storage [in] Reference to soil storage from which water rises
/// \param *pHRU [in] Reference to pertinent HRU
/// \param &Options [in] Global model options information
/// \param &tt [in] Specified point at time at which this accessing takes place
/// \param *rates [out] Rate of loss of water from soil to another soil layer [mm/day]
//
void   CmvCapillaryRise::GetRatesOfChange( const double      *storage, 
                                           const CHydroUnit  *pHRU, 
                                           const optStruct &Options,
                                           const time_struct &tt,
                                                 double     *rates) const
{
  if (pHRU->GetType()!=HRU_STANDARD){return;}//Lakes & glaciers

  const soil_struct *pSoil=NULL;

  double stor,max_stor=0;
	int    m;
  
  //--Obtain critical parameters for calculation----------------------
  stor = storage[iTo[0]];   //soil layer/groundwater water content [mm]
 
  sv_type toType  = pModel->GetStateVarType(iTo[0]);
  
  pSoil=NULL;
  max_stor=0.0;
  if      (toType==SOIL)       {
    m				 = pModel->GetStateVarLayer(iTo[0]); //which soil layer
    pSoil    = pHRU->GetSoilProps(m); 
    max_stor = pHRU->GetSoilCapacity(m);  //maximum storage of soil layer [mm]
  }
  else if (toType==GROUNDWATER){
    m				 = pModel->GetStateVarLayer(iTo[0]); //which aquifer layer
    pSoil    = pHRU->GetAquiferProps(m); 
    max_stor = pHRU->GetAquiferCapacity(m);  //maximum storage of soil layer [mm]
  }
  
  stor=min(max(stor,0.0),max_stor); //correct for potentially invalid storage

  //--Calculate Rate of water loss from soil/GW reservoir------------
	//-----------------------------------------------------------------
	if (type==CRISE_HBV)
  { //HBV MODEL 
    double max_rate;
    max_rate = pSoil->max_cap_rise_rate;

    rates[0]=max_rate*(1.0-stor/max_stor);
  }

	//-----------------------------------------------------------------
  else
  {
    ExitGracefully("CmvCapillaryRise::GetRatesOfChange: undefined Capillary Rise type",BAD_DATA);
  }

}

//////////////////////////////////////////////////////////////////
/// \brief Corrects rates of change (*rates) returned from RatesOfChange function 
/// \details Ensures that the rate of flow cannot drain "from" compartment over timestep.
/// Presumes overfilling of "to" compartment is handled using cascade
///
/// \param *storage [in] state variable array for this HRU
/// \param *pHRU [in] Reference to pertinent HRU
/// \param &Options [in] Global model options information
/// \param &tt [in] Specified point at time at which this accessing takes place
/// \param *rates [out] Rate of loss from soil to other soil layer [mm/day]
//
void   CmvCapillaryRise::ApplyConstraints( const double     *storage, 
                                      const CHydroUnit *pHRU, 
                                      const optStruct  &Options,
                                      const time_struct &tt,
                                            double     *rates) const
{
  if (pHRU->GetType()!=HRU_STANDARD){return;}//Lakes & glaciers

  //cant remove more than is there
  rates[0]=threshMin(rates[0],storage[iFrom[0]]/Options.timestep,0.0);

  //exceedance of max "to" compartment
	//water flow simply slows (or stops) so that receptor will not overfill during tstep
	rates[0]=threshMin(rates[0],
			                (pHRU->GetStateVarMax(iTo[0],storage,Options)-storage[iTo[0]])/Options.timestep,0.0);
}