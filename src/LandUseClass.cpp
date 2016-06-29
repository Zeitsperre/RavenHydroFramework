/*----------------------------------------------------------------
  Raven Library Source Code
  Copyright � 2008-2014 the Raven Development Team
----------------------------------------------------------------*/
#include "Properties.h"
#include "SoilAndLandClasses.h"
/*****************************************************************
   Constructor / Destructor
*****************************************************************/

//////////////////////////////////////////////////////////////////
/// \brief Implementation of the LandUseClass constructor
/// \param name [in] String nickname for land use class
//
CLandUseClass::CLandUseClass(const string name)
{
  tag=name;
  if (!DynArrayAppend((void**&)(pAllLUClasses),(void*)(this),NumLUClasses)){
     ExitGracefully("CLandUseClass::Constructor: creating NULL land use class",BAD_DATA);};
}

//////////////////////////////////////////////////////////////////
/// \brief Implementation of the default destructor
//
CLandUseClass::~CLandUseClass()
{
  if (DESTRUCTOR_DEBUG){cout<<"  DELETING LAND USE CLASS "<<endl;}
}

///////////////////////////////////////////////////////////////////
/// \brief Returns reference to surface properties
/// \return Surface properties associated with land use class
//
const surface_struct *CLandUseClass::GetSurfaceStruct() const{return &S;}

///////////////////////////////////////////////////////////////////
/// \brief Return nick name identifier of land use class
/// \return nick name identifier of land use class
//
string                CLandUseClass::GetTag          () const{return tag;}

/*****************************************************************
   Static Initialization, Accessors, Destructors
*****************************************************************/
CLandUseClass **CLandUseClass::pAllLUClasses=NULL;
int             CLandUseClass::NumLUClasses=0;

//////////////////////////////////////////////////////////////////
/// \brief Return number of land use classes
/// \return Number of land use classes
//
int CLandUseClass::GetNumClasses(){
  return NumLUClasses;
}

//////////////////////////////////////////////////////////////////
/// \brief Summarize LU class information to screen
//
void CLandUseClass::SummarizeToScreen()
{
  cout<<"==================="<<endl;
  cout<<"Land Use Class Summary:"<<NumLUClasses<<" LU/LT classes in database"<<endl;
  for (int c=0; c<NumLUClasses;c++){
    cout<<"-LULT. class \""<<pAllLUClasses[c]->GetTag()<<"\" "<<endl;
    cout<<"    impermeable: "<<pAllLUClasses[c]->GetSurfaceStruct()->impermeable_frac*100<<" %"<<endl;
    cout<<"       forested: "<<pAllLUClasses[c]->GetSurfaceStruct()->forest_coverage*100<<" %"<<endl;
    cout<<"max melt factor: "<<pAllLUClasses[c]->GetSurfaceStruct()->melt_factor <<" mm/d/K"<<endl;
  }
}

//////////////////////////////////////////////////////////////////
/// \brief Write LU class properties to file
/// \param &OUT [out] Output stream to which information is written
//
void CLandUseClass::WriteParamsToFile(ofstream &OUT)
{
  CLandUseClass *s;
  const surface_struct *t;
  OUT<<endl<<"---Land Use Parameters---------------------"<<endl;
  OUT<<"CLASS,";
  OUT<<"IMPERMEABLE_FRAC,FOREST_COVERAGE,ROUGHNESS,FOREST_SPARSENESS,";         
  OUT<<"MELT_FACTOR [mm/d/K],MIN_MELT_FACTOR [mm/d/K],REFREEZE_FACTOR [mm/d/K],HBV_MELT_FOR_CORR,HBV_MELT_ASP_CORR,"; 
  OUT<<"HBV_MELT_GLACIER_CORR[-],HBV_GLACIER_KMIN[-],GLACIER_STORAGE_COEFF[-],HBV_GLACIER_AG[1/mm SWE],CC_DECAY_COEFF[1/d]";
  OUT<<"SCS_CN,PARTITION_COEFF,SCS_IA_FRACTION,MAX_SAT_AREA_FRAC[-],B_EXP[-],";  
  OUT<<"DEP_MAX,ABST_PERCENT,OW_PET_CORR,LAKE_PET_CORR,FOREST_PET_CORR";     

  OUT<<endl;
  for (int c=0; c<NumLUClasses;c++)
  {
    s=pAllLUClasses[c];
    t=s->GetSurfaceStruct();
    OUT<<s->GetTag()<<",";
    OUT<<t->impermeable_frac<<","<<t->forest_coverage<<","<<t->roughness<<","<<t->forest_sparseness<<",";       
    OUT<<t->melt_factor<<","<<t->min_melt_factor<<","<<t->refreeze_factor<<","<<t->HBV_melt_for_corr<<","<<t->HBV_melt_asp_corr<<",";
    OUT<<t->HBV_melt_glacier_corr<<","<<t->HBV_glacier_Kmin<<","<<t->glac_storage_coeff<<","<<t->HBV_glacier_Ag<<","<<t->CC_decay_coeff<<",";
    OUT<<t->SCS_CN<<","<<t->partition_coeff<<","<<t->SCS_Ia_fraction<<","<<t->max_sat_area_frac<<","<<t->b_exp<<","; 
    OUT<<t->dep_max<<","<<t->abst_percent<<","<<t->ow_PET_corr<<","<<t->lake_PET_corr<<","<<t->forest_PET_corr<<",";
    
    OUT<<endl;
  }
}

//////////////////////////////////////////////////////////////////
/// \brief Destroy all LU classes
//
void CLandUseClass::DestroyAllLUClasses()
{
  if (DESTRUCTOR_DEBUG){cout <<"DESTROYING ALL LULT CLASSES"<<endl;}
  for (int c=0; c<NumLUClasses;c++){
    delete pAllLUClasses[c];
  }
  delete [] pAllLUClasses;
}

//////////////////////////////////////////////////////////////////
/// \brief Returns the LU class corresponding to passed string
/// \details Converts string (e.g., "AGRICULTURAL" in HRU file) to LU class
///  can accept either lultclass index or lultclass tag
///  if string is invalid, returns NULL
/// \param s [in] LU class identifier (tag or index)
/// \return Pointer to LU class corresponding to identifier string s
//
CLandUseClass *CLandUseClass::StringToLUClass(const string s)
{
  string sup=StringToUppercase(s);
  for (int c=0;c<NumLUClasses;c++)
  {
    if (!sup.compare(StringToUppercase(pAllLUClasses[c]->GetTag()))){return pAllLUClasses[c];}
    else if (s_to_i(s.c_str())==(c+1))         {return pAllLUClasses[c];}
  }
  return NULL;
}
//////////////////////////////////////////////////////////////////
/// \brief Returns the land use  class corresponding to the passed index
///  if index is invalid, returns NULL
/// \param c [in] Soil class index 
/// \return Reference to land use class corresponding to index c
//
const CLandUseClass *CLandUseClass::GetLUClass(int c)
{
  if ((c<0) || (c>=NumLUClasses)){return NULL;}
  return pAllLUClasses[c];
}
//////////////////////////////////////////////////////////////////
/// \brief Automatically calculates surface propeties
/// \details  Sets surface properties based upon simple lu/lt parameters 
///  Input [Stmp & Rtmp] has been read from .rvp file - if parameter == 
///  AUTO_COMPLETE, then empirical relationships are used to estimate 
///  parameters 
///
/// \param &Stmp [in] Input LU parameters (read from .rvp file)
/// \param &Sdefault [in] Default LU parameters
//
void CLandUseClass::AutoCalculateLandUseProps(const surface_struct &Stmp,
                                              const surface_struct &Sdefault)
                                              //const surface_struct &needed_params
{
  bool autocalc;
  string warn;
  bool chatty=true;

  //these parameters are required
  S.impermeable_frac=Stmp.impermeable_frac;

  //Forest coverage
  autocalc=SetCalculableValue(S.forest_coverage,Stmp.forest_coverage,Sdefault.forest_coverage);
  if (autocalc)
  {  
    //RELATIONSHIP REQUIRED 
    S.forest_coverage =0.0;
    //no warning -default is no forest cover
  }
  autocalc=SetCalculableValue(S.forest_sparseness ,Stmp.forest_sparseness,Sdefault.forest_sparseness);
  if (autocalc)
  {
    S.forest_sparseness =0.0;  //Default - not sparse
    //no warning -default is no sparseness
  }
  //Standard surface properties
  //----------------------------------------------------------------------------
  //Roughness
  autocalc=SetCalculableValue(S.roughness,Stmp.roughness,Sdefault.roughness);
  if (autocalc)
  {  
    //RELATIONSHIP REQUIRED 
    S.roughness =0.0;  
    //no warning -default is no roughness
  }

  autocalc=SetCalculableValue(S.max_sat_area_frac,Stmp.max_sat_area_frac,Sdefault.max_sat_area_frac);
  if (autocalc)
  {  
    S.max_sat_area_frac =1.0;
    //no warning -default is no max
  }

  //Snow properties
  autocalc=SetCalculableValue(S.melt_factor,Stmp.melt_factor,Sdefault.melt_factor);
  if (autocalc)
  {  
    //RELATIONSHIP REQUIRED? 
    S.melt_factor =5.04;//[mm/K/d]/// \ref GAWSER ??
    warn="The required parameter MELT_FACTOR for land use class "+tag+" was autogenerated with value "+to_string(S.melt_factor);
    if (chatty){WriteWarning(warn,false);}
  }
  autocalc=SetCalculableValue(S.min_melt_factor,Stmp.min_melt_factor,Sdefault.min_melt_factor);
  if (autocalc)
  {  
    S.min_melt_factor =S.melt_factor;//[mm/K/d]
    //no warning 
  }
  autocalc=SetCalculableValue(S.refreeze_factor,Stmp.refreeze_factor,Sdefault.refreeze_factor);
  if (autocalc)
  {  
    S.refreeze_factor =5.04;//[mm/K/d]// \ref GAWSER 
    warn="The required parameter REFREEZE_FACTOR for land use class "+tag+" was autogenerated with value "+to_string(S.refreeze_factor);
    if (chatty){WriteWarning(warn,false);}
  }

  autocalc=SetCalculableValue(S.HBV_melt_for_corr,Stmp.HBV_melt_for_corr,Sdefault.HBV_melt_for_corr);
  if (autocalc)
  {
    S.HBV_melt_for_corr =1.0;  //Default - no forest correction
    //no warning
  }
  autocalc=SetCalculableValue(S.HBV_melt_asp_corr,Stmp.HBV_melt_asp_corr,Sdefault.HBV_melt_asp_corr);
  if (autocalc)
  {
    S.HBV_melt_asp_corr =0.0;  //Default - no aspect correction
    //no warning
  }
  autocalc=SetCalculableValue(S.HBV_melt_glacier_corr,Stmp.HBV_melt_glacier_corr,Sdefault.HBV_melt_glacier_corr);
  if (autocalc)
  {
    S.HBV_melt_glacier_corr =1.0;  //Default - no glacier correction
    //no warning
  }
  autocalc=SetCalculableValue(S.ow_PET_corr,Stmp.ow_PET_corr,Sdefault.ow_PET_corr);
  if (autocalc)
  {
    S.ow_PET_corr =1.0;  //Default - no correction
    //no warning
  }
  autocalc=SetCalculableValue(S.lake_PET_corr,Stmp.lake_PET_corr,Sdefault.lake_PET_corr);
  if (autocalc)
  {
    S.lake_PET_corr =1.0;  //Default - no correction
    //no warning
  }
  autocalc=SetCalculableValue(S.forest_PET_corr,Stmp.forest_PET_corr,Sdefault.forest_PET_corr);
  if (autocalc)
  {
    S.forest_PET_corr =1.0;  //Default - no correction 
    //no warning
  }
  autocalc=SetCalculableValue(S.SCS_Ia_fraction,Stmp.SCS_Ia_fraction,Sdefault.SCS_Ia_fraction);
  if (autocalc)
  {
    S.SCS_Ia_fraction=0.2;  //Default SCS Ia=0.2*S
    warn="The required parameter SCS_IA_FRACTION for land use class "+tag+" was autogenerated with value "+to_string(S.SCS_Ia_fraction);
    if (chatty){WriteWarning(warn,false);}
  }

  autocalc=SetCalculableValue(S.snow_patch_limit,Stmp.snow_patch_limit,Sdefault.snow_patch_limit);
  if (autocalc)
  {
    S.snow_patch_limit=0.0;  //Default snow patch limit = 0.0 mm
    //no warning - default is no patching
  }

  autocalc = SetCalculableValue(S.UBC_icept_factor, Stmp.UBC_icept_factor, Sdefault.UBC_icept_factor);
  if (autocalc){
    S.UBC_icept_factor = 0.0; //Temporary for UBCWM until translator repaired - should not have default
  }

  //Model-specific LULT properties - cannot be autocomputed, must be specified by user
  //----------------------------------------------------------------------------
  bool needed=false;//TMP DEBUG (to be removed- this handled elsewhere in code)

  SetSpecifiedValue(S.partition_coeff,Stmp.partition_coeff,Sdefault.partition_coeff,needed,"PARTITION_COEFF");//(needed_params.partition_coeff>0.0)
  SetSpecifiedValue(S.SCS_CN,Stmp.SCS_CN,Sdefault.SCS_CN,needed,"SCS_CN");
  SetSpecifiedValue(S.b_exp,Stmp.b_exp,Sdefault.b_exp,needed,"B_EXP");
  SetSpecifiedValue(S.dep_max,Stmp.dep_max,Sdefault.dep_max,needed,"DEP_MAX");
  SetSpecifiedValue(S.abst_percent,Stmp.abst_percent,Sdefault.abst_percent,needed,"ABST_PERCENT");
  SetSpecifiedValue(S.HBV_glacier_Kmin,Stmp.HBV_glacier_Kmin,Sdefault.HBV_glacier_Kmin,needed,"HBV_GLACIER_KMIN");
  SetSpecifiedValue(S.glac_storage_coeff,Stmp.glac_storage_coeff,Sdefault.glac_storage_coeff,needed,"GLAC_STORAGE_COEFF");
  SetSpecifiedValue(S.HBV_glacier_Ag,Stmp.HBV_glacier_Ag,Sdefault.HBV_glacier_Ag,needed,"HBV_GLACIER_AG");
  SetSpecifiedValue(S.CC_decay_coeff,Stmp.CC_decay_coeff,Sdefault.CC_decay_coeff,needed,"CC_DECAY_COEFF");
  SetSpecifiedValue(S.GR4J_x4,Stmp.GR4J_x4,Sdefault.GR4J_x4,needed,"GR4J_X4");
  SetSpecifiedValue(S.wind_exposure, Stmp.wind_exposure, Sdefault.wind_exposure, needed,"WIND_EXPOSURE");
}

//////////////////////////////////////////////////////////////////
/// \brief Sets default Surface properties
/// \details Initializes all surface properties to DEFAULT_VALUE
///  if is_template==true, initializes instead to NOT_SPECIFIED or AUTO_CALCULATE
/// \param &S [out] Surface properties class
/// \param is_template [in] True if the default value being set is for the template class
//
void CLandUseClass::InitializeSurfaceProperties(surface_struct &S, bool is_template)
{
  //required parameters 
  S.impermeable_frac =0.0;

  //Autocalculable parameters
  S.forest_coverage  =DefaultParameterValue(is_template,true);
  S.forest_sparseness=DefaultParameterValue(is_template,true);
  S.roughness        =DefaultParameterValue(is_template,true);
  S.melt_factor      =DefaultParameterValue(is_template,true);
  S.min_melt_factor  =DefaultParameterValue(is_template,true);
  S.refreeze_factor  =DefaultParameterValue(is_template,true);
  S.HBV_melt_asp_corr=DefaultParameterValue(is_template,true);
  S.HBV_melt_for_corr=DefaultParameterValue(is_template,true);
  S.HBV_melt_glacier_corr=DefaultParameterValue(is_template,true);//1.64
  S.max_sat_area_frac=DefaultParameterValue(is_template,true);//0.250;    //default [-]
  S.ow_PET_corr      =DefaultParameterValue(is_template,true);//1.0;      //[-]
  S.lake_PET_corr    =DefaultParameterValue(is_template,true);//1.0;      //[-]
  S.forest_PET_corr  =DefaultParameterValue(is_template,true);//1.0;      //[-]
  S.SCS_Ia_fraction  =DefaultParameterValue(is_template,true);//0.2
  S.snow_patch_limit = DefaultParameterValue(is_template,true);//0.0

  //User-specified parameters
  S.partition_coeff   =DefaultParameterValue(is_template,false);//0.4;//needs reasonable defaults
  S.SCS_CN            =DefaultParameterValue(is_template,false);//50
  S.b_exp             =DefaultParameterValue(is_template,false);//0.071;    //default [-]
  S.dep_max           =DefaultParameterValue(is_template,false);//6.29;     //[mm]
  S.abst_percent      =DefaultParameterValue(is_template,false);//0.1;    
  S.HBV_glacier_Kmin  =DefaultParameterValue(is_template,false);//0.05
  S.glac_storage_coeff=DefaultParameterValue(is_template,false);//0.10
  S.HBV_glacier_Ag    =DefaultParameterValue(is_template,false);//0.05 /mm
  S.CC_decay_coeff    =DefaultParameterValue(is_template,false);//0.0? [1/d]
  S.GR4J_x4           =DefaultParameterValue(is_template,false);//1.7 [d]
  S.UBC_icept_factor  =DefaultParameterValue(is_template,false);//0 [0..1]
  S.wind_exposure     =DefaultParameterValue(is_template,false);//
}
//////////////////////////////////////////////////////////////////
/// \brief Sets the value of the surface property corresponding to param_name
/// \param &S [out] Surface properties class
/// \param param_name [in] Parameter identifier
/// \param value [in] Value of parameter to be set
//
void  CLandUseClass::SetSurfaceProperty(string       &param_name, 
                                        const double &value)
{
  SetSurfaceProperty(S,param_name,value);
}
//////////////////////////////////////////////////////////////////
/// \brief Sets the value of the surface property corresponding to param_name
/// \param &S [out] Surface properties class
/// \param param_name [in] Parameter identifier
/// \param value [in] Value of parameter to be set
//
void  CLandUseClass::SetSurfaceProperty(surface_struct &S, 
                                        string       param_name, 
                                        const double value)
{
  string name;
  name = StringToUppercase(param_name);

  if      (!name.compare("IMPERMEABLE_FRAC"       )){S.impermeable_frac=value;}
  else if (!name.compare("FOREST_COVERAGE"        )){S.forest_coverage=value;}
  else if (!name.compare("ROUGHNESS"              )){S.roughness=value;}

  else if (!name.compare("FOREST_SPARSENESS"      )){S.forest_sparseness=value;}
  else if (!name.compare("MELT_FACTOR"            )){S.melt_factor=value;}
  else if (!name.compare("MIN_MELT_FACTOR"        )){S.min_melt_factor=value;}
  else if (!name.compare("REFREEZE_FACTOR"        )){S.refreeze_factor=value;}
  else if (!name.compare("HBV_MELT_ASP_CORR"      )){S.HBV_melt_asp_corr=value;}
  else if (!name.compare("HBV_MELT_FOR_CORR"      )){S.HBV_melt_for_corr=value;}
  else if (!name.compare("MAX_SAT_AREA_FRAC"      )){S.max_sat_area_frac=value;}
  else if (!name.compare("HBV_MELT_GLACIER_CORR"  )){S.HBV_melt_glacier_corr=value;}
  else if (!name.compare("HBV_GLACIER_KMIN"       )){S.HBV_glacier_Kmin=value;}
  else if (!name.compare("GLAC_STORAGE_COEFF"     )){S.glac_storage_coeff=value;}
  else if (!name.compare("HBV_GLACIER_AG"         )){S.HBV_glacier_Ag=value;}
  else if (!name.compare("CC_DECAY_COEFF"         )){S.CC_decay_coeff=value;}
  else if (!name.compare("PARTITION_COEFF"        )){S.partition_coeff=value;}
  else if (!name.compare("SCS_CN"                 )){S.SCS_CN=value;}
  else if (!name.compare("SCS_IA_FRACTION"        )){S.SCS_Ia_fraction=value;}
  else if (!name.compare("B_EXP"                  )){S.b_exp=value;}
  else if (!name.compare("VIC_B_EXP"              )){S.b_exp=value;}
  else if (!name.compare("DEP_MAX"                )){S.dep_max =value;}
  else if (!name.compare("ABST_PERCENT"           )){S.abst_percent =value;}
  else if (!name.compare("OW_PET_CORR"            )){S.ow_PET_corr=value;}
  else if (!name.compare("LAKE_PET_CORR"          )){S.lake_PET_corr=value;}
  else if (!name.compare("FOREST_PET_CORR"        )){S.forest_PET_corr=value;}
  else if (!name.compare("SNOW_PATCH_LIMIT"       )){S.snow_patch_limit=value;}
  else if (!name.compare("GR4J_X4"                )){S.GR4J_x4=value;}
  else if (!name.compare("UBC_ICEPT_FACTOR"       )){S.UBC_icept_factor=value;}
  else if (!name.compare("WIND_EXPOSURE"          )){S.wind_exposure=value;}

  else{
    WriteWarning("Trying to set value of unrecognized/invalid land use/land type parameter "+ name,false);
  }
}
//////////////////////////////////////////////////////////////////
/// \brief gets surface property corresponding to param_name
/// \param param_name [in] Parameter identifier
/// \returns value of parameter
//
double CLandUseClass::GetSurfaceProperty(string param_name) const
{
  return GetSurfaceProperty(S,param_name);
}

///////////////////////////////////////////////////////////////////////////
/// \brief Returns land surface property value corresponding to param_name from structure provided
/// \param surface_struct [in] land surface structure
/// \param param_name [in] Parameter name
/// \return LULT property corresponding to parameter name
//
double CLandUseClass::GetSurfaceProperty(const surface_struct &S, string param_name)
{
  string name;
  name = StringToUppercase(param_name);

  if      (!name.compare("IMPERMEABLE_FRAC"       )){return S.impermeable_frac;}
  else if (!name.compare("FOREST_COVERAGE"        )){return S.forest_coverage;}
  else if (!name.compare("ROUGHNESS"              )){return S.roughness;}

  else if (!name.compare("FOREST_SPARSENESS"      )){return S.forest_sparseness;}
  else if (!name.compare("MELT_FACTOR"            )){return S.melt_factor;}
  else if (!name.compare("MIN_MELT_FACTOR"        )){return S.min_melt_factor;}
  else if (!name.compare("REFREEZE_FACTOR"        )){return S.refreeze_factor;}
  else if (!name.compare("HBV_MELT_ASP_CORR"      )){return S.HBV_melt_asp_corr;}
  else if (!name.compare("HBV_MELT_FOR_CORR"      )){return S.HBV_melt_for_corr;}
  else if (!name.compare("MAX_SAT_AREA_FRAC"      )){return S.max_sat_area_frac;}
  else if (!name.compare("HBV_MELT_GLACIER_CORR"  )){return S.HBV_melt_glacier_corr;}
  else if (!name.compare("HBV_GLACIER_KMIN"       )){return S.HBV_glacier_Kmin;}
  else if (!name.compare("GLAC_STORAGE_COEFF"     )){return S.glac_storage_coeff;}
  else if (!name.compare("HBV_GLACIER_AG"         )){return S.HBV_glacier_Ag;}
  else if (!name.compare("CC_DECAY_COEFF"         )){return S.CC_decay_coeff;}
  else if (!name.compare("PARTITION_COEFF"        )){return S.partition_coeff;}
  else if (!name.compare("SCS_IA_FRACTION"        )){return S.SCS_Ia_fraction;}
  else if (!name.compare("SCS_CN"                 )){return S.SCS_CN;}
  else if (!name.compare("B_EXP"                  )){return S.b_exp;}
  else if (!name.compare("VIC_B_EXP"              )){return S.b_exp;}
  else if (!name.compare("DEP_MAX"                )){return S.dep_max ;}
  else if (!name.compare("ABST_PERCENT"           )){return S.abst_percent;}
  else if (!name.compare("OW_PET_CORR"            )){return S.ow_PET_corr;}
  else if (!name.compare("LAKE_PET_CORR"          )){return S.lake_PET_corr;}
  else if (!name.compare("FOREST_PET_CORR"        )){return S.forest_PET_corr;}
  else if (!name.compare("SNOW_PATCH_LIMIT"       )){return S.snow_patch_limit;}
  else if (!name.compare("GR4J_X4"                )){return S.GR4J_x4;}
  else if (!name.compare("UBC_ICEPT_FACTOR"       )){return S.UBC_icept_factor;}
  else if (!name.compare("WIND_EXPOSURE"          )){return S.wind_exposure;}
  else{
    string msg="CLandUseClass::GetSurfaceProperty: Unrecognized/invalid LU/LT parameter name in .rvp file: "+name;
    ExitGracefully(msg.c_str(),BAD_DATA);
    return 0.0;
  }

}