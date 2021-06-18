/*----------------------------------------------------------------
Raven Library Source Code
Copyright (c) 2008-2021 the Raven Development Team
----------------------------------------------------------------
CIsotopeModel routines related to isotope transport
----------------------------------------------------------------*/
#include "RavenInclude.h"
#include "IsotopeTransport.h"

string FilenamePrepare(string filebase,const optStruct& Options); //Defined in StandardOutput.cpp

//////////////////////////////////////////////////////////////////
/// \brief isotope model constructor
//
CIsotopeModel::CIsotopeModel(CModel* pMod,CTransportModel* pTMod,string name,const int c, const iso_type iso)
  :CConstituentModel(pMod,pTMod,name,ISOTOPE,false,c)
{
  _isotope=iso;
}

//////////////////////////////////////////////////////////////////
/// \brief isotope model destructor
//
CIsotopeModel::~CIsotopeModel(){}

//////////////////////////////////////////////////////////////////
/// \brief initializes isotope transport model
//
void CIsotopeModel::Initialize(const optStruct& Options)
{
  // Initialize base class members
  //--------------------------------------------------------------------
  CConstituentModel::Initialize(Options);
}
//////////////////////////////////////////////////////////////////
/// \brief returns COMPOSITION of isotope in [o/oo] (rather than concentration in mg/mg)
/// \param M [in] mass [mg/m2] 
/// \param V [in] volume [mm]
//
double CIsotopeModel::GetConcentration(const double& mass,const double& vol)
{
  double C=CConstituentModel::CalculateConcentration(mass,vol);
  return ConcToComposition(C);
}
//////////////////////////////////////////////////////////////////
/// \brief returns advection correction factor for isotope
/// handles enrichment factor for evaporating waters 
/// \param pHRU [in] pointer to HRU of interest
/// \param iFromWater [in] index of "from" water storage state variable
/// \param iToWater [in] index of "to" water storage state variable
/// \param Cs [in] isotopic concentration of source water [mg/mg]
/// 
double CIsotopeModel::GetAdvectionCorrection(const CHydroUnit* pHRU,const int iFromWater,const int iToWater,const double& Cs) const
{  
  sv_type fromType=_pModel->GetStateVarType(iFromWater);
  sv_type toType  =_pModel->GetStateVarType(iToWater);

  if(toType==ATMOSPHERE) // handles all enrichment due to evaporation
  {
    //From Stadnyk-Falcone, PhD Thesis, University of Waterloo 2008
    double dE;     // evaporative composition [o/oo]
    double dL;     // lake/wetland composition [o/oo]
    double dP;     // meteoric water composition [o/oo]
    double dA;     // atmospheric composition [o/oo]
    double dStar;  // limiting steady state composition of source water (not used here) [o/oo]

    double h=pHRU->GetForcingFunctions()->rel_humidity;
    double T=pHRU->GetForcingFunctions()->temp_ave+ZERO_CELSIUS;
    
    //TMP DEBUG - this should not be this hard. Should have a _pTransModel->GetConcentration(k,c,ATMOS_PRECIP,m=0);
    int iAtmPrecip=_pModel->GetStateVarIndex(ATMOS_PRECIP);
    int m         =_pTransModel->GetLayerIndex(_constit_index,iAtmPrecip);
    int i         =_pModel->GetStateVarIndex(CONSTITUENT,m);
    dP=_pTransModel->GetConcentration(pHRU->GetGlobalIndex(),i); //[o/oo]

    //dP=ConcToComposition(dP);//[mg/mg]->[o/oo]
    dL=ConcToComposition(Cs);//[mg/mg]->[o/oo]
    

    double eta=1.0;        //turbulence parameter = 1 for soil, 0.5 for open water, 0.6 for wetlands
    double hprime=1.0;     //1.0 for small water bodies, 0.88 for large lakes/reservoirs 
    double theta;          //advection correction
    if      (fromType==DEPRESSION   ) {eta=0.6;}
    else if (fromType==SOIL         ) {eta=1.0;}
    else if (fromType==SURFACE_WATER) {eta=0.5;}
    //else if (fromType==SNOW         )  //sublimation?
    //else if (fromType==CANOPY_SNOW  )  //sublimation?

    theta=(1-hprime)/(1-h); 
  
    //from Horita and Wesolowski,  Liquid-vapor fractionation of oxygen and hydrogen isotopes of water from the freezing to the critical temperature,
    //Geochimica et Cosmochimica Acta, 58(16), 1994,
    double alpha_star,ep_star,tmp;
    double beta;
    double CK0;
    if     (_isotope==ISO_O18) {
      tmp=-7.685+6.7123*(1e3/T)-1.6664*(1e6/T/T)+0.35401*(1e9/T/T/T); //eqn 6 of H&W
      CK0=28.6/TO_PER_MILLE;   //[-] Vogt, 1976
    }
    else if(_isotope==ISO_H2) {
      tmp=1158.8*(T*T*T/1e9)-1620.1*(T*T/1e6)+794.84*(T/1e3)+2.9992*(1e9/T/T/T)-161.04;  //eqn 5 of H&W
      CK0=25.0/TO_PER_MILLE;
    }
    alpha_star=exp(0.001*tmp); //[-] 
    ep_star   =alpha_star-1.0; //[-]
    beta      =eta*theta*CK0;  //[-]

    dA   =((  dP/TO_PER_MILLE-ep_star)/alpha_star)*TO_PER_MILLE;//[o/oo]
    
    //conversion - all compositions in o/oo must be converted to 
    dStar=(h*dA/TO_PER_MILLE+beta*(1-h)+ep_star/alpha_star)/(h - beta*(1-h) - ep_star/alpha_star)*TO_PER_MILLE; //[o/oo]

    dE   =((dL/TO_PER_MILLE-ep_star)/alpha_star - h*dA/TO_PER_MILLE - beta*(1-h)) / ((1-h)*(1+beta))*TO_PER_MILLE; //[o/oo]

    double C_E=CompositionToConc(dE);
    double C_L=CompositionToConc(dL);
    return min(C_E/C_L,1.0);
  }
  return 1.0;
}
//////////////////////////////////////////////////////////////////
/// \brief Write transport output file headers in .tb0 format
/// \details Called prior to simulation (but after initialization) from CModel::Initialize()
/// \param &Options [in] Global model options information
//
void CIsotopeModel::WriteEnsimOutputFileHeaders(const optStruct& Options)
{
  this->WriteOutputFileHeaders(Options); //Ensim format not supported by isotope model
}

//////////////////////////////////////////////////////////////////
/// \brief Writes minor transport output to ensim formatted file at the end of each timestep (or multiple thereof)
/// \note only thing this modifies should be output streams; called from CModel::WriteMinorOutput()
/// \param &Options [in] Global model options information
/// \param &tt [in] Local (model) time at the end of the pertinent time step
//
void CIsotopeModel::WriteEnsimMinorOutput(const optStruct& Options,const time_struct& tt)
{
  this->WriteMinorOutput(Options,tt);//Ensim format not supported by isotope model
}

//R-values are O18/O16 ratio
//concentrations are 18O/(16O+18O) =%18O by mass (<1)
double CIsotopeModel::RvalToConcentration(const double &Rv) const
{
  return Rv/(Rv+1);
}
double CIsotopeModel::ConcentrationToRval(const double &conc) const
{
  return conc/(1-conc);
}
double CIsotopeModel::ConcToComposition(const double &conc) const
{
  return (conc/(1-conc)/RV_VMOW-1.0)*TO_PER_MILLE; //only valid for O18!
}
double CIsotopeModel::CompositionToConc(const double& d) const
{
  double R=((d/TO_PER_MILLE)+1.0)*RV_VMOW; //only valid for O18!
  return RvalToConcentration(R);
}