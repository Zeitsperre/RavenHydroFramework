/*----------------------------------------------------------------
  Raven Library Source Code
  Copyright (c) 2008-2021 the Raven Development Team
  ----------------------------------------------------------------*/
#include "RavenInclude.h"
#include "Model.h"
#include "StateVariables.h"
#include "HydroUnits.h"
#include "ParseLib.h"

void SetInitialStateVar(CModel *&pModel,const int SVind,const sv_type typ,const int m,const int k,const double &val);
//////////////////////////////////////////////////////////////////
/// \brief Parses Initial conditions file
/// \details model.rvc: input file that defines HRU and Subbasin initial conditions\n
///
/// \param *&pModel [out] Reference to model object
/// \param &Options [out] Global model options information
/// \return True if operation is successful
//
bool ParseInitialConditionsFile(CModel *&pModel, const optStruct &Options)
{
  int         i,k;              //counters
  long        SBID;             //subbasin ID
  //CHydroUnit *pHRU;           //temporary pointers to HRUs, Subbasins
  CSubBasin  *pSB;
  bool        ended(false);
  bool        in_ifmode_statement=false;

  time_struct tt;
  JulianConvert(0.0,Options.julian_start_day,Options.julian_start_year,Options.calendar,tt);

  ifstream    IC;
  IC.open(Options.rvc_filename.c_str());
  if (IC.fail()){
    cout << "ERROR opening initial conditions file: "<<Options.rvc_filename<<endl; return false;}

  int   Len,line(0),code;
  char *s[MAXINPUTITEMS];
  CParser *pp=new CParser(IC,Options.rvc_filename,line);

  ifstream INPUT2;           //For Secondary input
  CParser *pMainParser=NULL; //for storage of main parser while reading secondary files

  if (Options.noisy){
    cout <<"======================================================"<<endl;
    cout <<"Parsing Initial conditions File " << Options.rvc_filename <<"..."<<endl;
    cout <<"======================================================"<<endl;
  }

  //Initialize everything to zero (required for ensemble simulation)
  //--------------------------------------------------------------------------
  for (int i=0;i<pModel->GetNumStateVars();i++)
  {
    sv_type typ      =pModel->GetStateVarType(i);
    int     layer_ind=pModel->GetStateVarLayer(i);
    for(k=0;k<pModel->GetNumHRUs();k++) {
      SetInitialStateVar(pModel,i,typ,layer_ind,k,0.0);
    }
  }
  for(int p=0;p<pModel->GetNumSubBasins();p++) {
    pModel->GetSubBasin(p)->SetBasinProperties("Q_REFERENCE",AUTO_COMPUTE);
    double *aQo=new double [pModel->GetSubBasin(p)->GetNumSegments()];
    for(int i=0;i<pModel->GetSubBasin(p)->GetNumSegments();i++) {
      aQo[i]=AUTO_COMPUTE;
    }
    //pModel->GetSubBasin(p)->SetQoutArray(pModel->GetSubBasin(p)->GetNumSegments(),aQo,AUTO_COMPUTE); //seems to cause issues
    delete [] aQo;
    pModel->GetSubBasin(p)->SetChannelStorage(0.0);
    pModel->GetSubBasin(p)->SetRivuletStorage(0.0);
    if(pModel->GetSubBasin(p)->GetReservoir()!=NULL) {
      pModel->GetSubBasin(p)->GetReservoir()->SetReservoirStage(0.0,0.0);
    }
  }

  //--Sift through file-----------------------------------------------
  bool end_of_file=pp->Tokenize(s,Len);
  while (!end_of_file)
  {
    if (ended){break;}
    if (Options.noisy){ cout << "reading line " << pp->GetLineNumber() << ": ";}

    /*assign code for switch statement
      ------------------------------------------------------------------
      <100         : ignored/special
      0   thru 100 : All other
      ------------------------------------------------------------------
    */
    string concname="UNKNOWN";
    code=0;
    //---------------------SPECIAL -----------------------------
    if       (Len==0)                                       {code=-1; }//blank line
    else if  (IsComment(s[0],Len))                          {code=-2; }//comment
    else if  (!strcmp(s[0],":End"                         )){code=-4; }//stop reading
    else if  (!strcmp(s[0],":IfModeEquals"                )){code=-5; }
    else if  (in_ifmode_statement)                          {code=-6; }
    else if  (!strcmp(s[0],":RedirectToFile"              )){code=-3; }//redirect to secondary file
    //-------------------MODEL INITIAL CONDITIONS----------------
    else if  (!strcmp(s[0],":BasinInitialConditions"      )){code=1;  }
    else if  (!strcmp(s[0],":HRUInitialConditions"        )){code=2;  }//UNUSED
    else if  (!strcmp(s[0],":EndHRUInitialConditions"     )){code=-2; }//UNUSED
    else if  (!strcmp(s[0],":ALL"                         )){code=3;  }
    else if  (!strcmp(s[0],":UniformInitialConditions"    )){code=3;  }
    else if  (!strcmp(s[0],":HRUStateVariableTable"       )){code=4; concname="";}
    else if  (!strcmp(s[0],":InitialTemperatureTable"     )){code=4; concname="TEMPERATURE";}
    else if  (!strcmp(s[0],":InitialConcentrationTable"   )){code=4; concname=s[1]; }
    else if  (!strcmp(s[0],":EndHRUStateVariableTable"    )){code=-2; }
    else if  (!strcmp(s[0],":EndInitialTemperatureTable"  )){code=-2;}
    else if  (!strcmp(s[0],":EndInitialConcentrationTable")){code=-2;}
    else if  (!strcmp(s[0],":InitialConditions"           )){code=5;  }//OBSOLETE
    else if  (!strcmp(s[0],":EndInitialConditions"        )){code=-2; }//OBSOLETE
    else if  (!strcmp(s[0],":BasinStateVariables"         )){code=6;  }
    else if  (!strcmp(s[0],":EndBasinStateVariables"      )){code=-2; }
    else if  (!strcmp(s[0],":InitialReservoirFlow"        )){code=7;  }
    else if  (!strcmp(s[0],":InitialReservoirStage"       )){code=8;  }
    else if  (!strcmp(s[0],":TimeStamp"                   )){code=10; }
    else if  (!strcmp(s[0],":Nudge"                       )){code=11; }
    else if  (!strcmp(s[0],":BasinTransportVariables"     )){code=12; concname=s[1];}
    else if  (!strcmp(s[0],":EndBasinTransportVariables"  )){code=-2; }
    


    switch(code)
    {
    case(-1):  //----------------------------------------------
    {/*Blank Line*/
      if (Options.noisy) {cout <<""<<endl;}break;
    }
    case(-2):  //----------------------------------------------
    {/*Comment*/
      if (Options.noisy) {cout <<"*"<<endl;} break;
    }
    case(-3):  //----------------------------------------------
    {/*:RedirectToFile*/
      string filename="";
      for (int i=1;i<Len;i++){filename+=s[i]; if(i<Len-1){filename+=' ';}}
      if (Options.noisy) {cout <<"Redirect to file: "<<filename<<endl;}

      filename=CorrectForRelativePath(filename,Options.rvc_filename);

      INPUT2.open(filename.c_str());
      if (INPUT2.fail()){
        string warn=":RedirectToFile: Cannot find file "+filename;
        ExitGracefully(warn.c_str(),BAD_DATA);
      }
      else{
        pMainParser=pp;   //save pointer to primary parser
        pp=new CParser(INPUT2,filename,line);//open new parser
      }
      break;
    }
    case(-4):  //----------------------------------------------
    {/*:End*/
      if (Options.noisy) {cout <<"EOF"<<endl;} ended=true; break;
    }
    case(-5):  //----------------------------------------------
    {/*:IfModeEquals*/
      if(Len>1) {
        if(Options.noisy) { cout <<"Mode statement start..."<<endl; }
        char testmode=s[1][0];
        if(testmode!=Options.run_mode) {
          in_ifmode_statement=true;
        }
      }
      break;
    }
    case(-6):  //----------------------------------------------
    {/*in_ifmode_statement*/
      if(Options.noisy) { cout <<"...Mode statement end"<<endl; }
      if(!strcmp(s[0],":EndIfModeEquals"))
      {
        in_ifmode_statement=false;
      }
      break;
    }
    case(1):  //----------------------------------------------
    { /*
        ":BasinInitialConditions"
        {SBID, {flow x num_segments}} x nSubBasins
        :EndBasinInitialConditions
      */
      if (Options.noisy) {cout <<"Basin Initial Conditions..."<<endl;}
      while ((Len==0) || (strcmp(s[0],":EndBasinInitialConditions")))
      {
        pp->Tokenize(s,Len);
        if      (IsComment(s[0], Len)){}//comment line
        else if (!strcmp(s[0],":Attributes"  )){}//ignored by Raven - needed for GUIs
        else if (!strcmp(s[0],":Units"       )){}//ignored by Raven - needed for GUIs
        else if (!strcmp(s[0],":EndBasinInitialConditions")){}//done
        else
        {
          ExitGracefullyIf(Len<2,
                           "Parse HRU File: incorrect number of terms in SubBasin initial conditions",BAD_DATA);
          SBID=s_to_l(s[0]);
          pSB=NULL;
          pSB=pModel->GetSubBasinByID(SBID);
          if (pSB!=NULL){
            pSB->SetQout(s_to_d(s[1]));
          }
          else          {
            WriteWarning("Subbasin "+to_string(SBID)+" not in model, cannot set initial conditions",Options.noisy);
          }
        }
      }
      break;
    }
    case(2):  //----------------------------------------------
    { /* :HRUInitialConditions
       */
      if (Options.noisy) {cout <<"   Reading HRU Initial Conditions..."<<endl;}
      //does nothing, just an input file placeholder
      break;
    }
    case(3):  //----------------------------------------------
    { /* :UniformInitialConditions [svtype] [svval]
         -sets blanket IC values for a single state variable across all HRUs
         -only needed if IC not equal to zero
      */
      if (Options.noisy) {cout <<"Initial Conditions (Uniform)"<<endl;}
      if (Len>=3)
      {
        int     layer_ind(-1);
        int     SVind;
        sv_type typ;

        typ=CStateVariable::StringToSVType(s[1],layer_ind,false);

        if (typ==UNRECOGNIZED_SVTYPE){
          WriteWarning(":UniformInitialConditions: unrecognized state variable type "+to_string(s[1]),Options.noisy);
          break;
        }

        SVind=pModel->GetStateVarIndex(typ,layer_ind);

        if (SVind!=DOESNT_EXIST){
          for (k=0;k<pModel->GetNumHRUs();k++){
            SetInitialStateVar(pModel,SVind,typ,layer_ind,k,s_to_d(s[2]));
          }
        }
        else{
          WriteWarning("Initial conditions specified for state variable not in model ("+to_string(s[1])+")",Options.noisy);
        }
      }
      else{
        pp->ImproperFormat(s);
      }
      break;
    }
    case(4):  //----------------------------------------------
    { /* :HRUStateVariableTable (formerly :IntialConditionsTable)
           :Attributes {list of state variables}
           :Units {list of units}
           {HRUID, SV1,SV2,SV3} x nHRUs
         :EndHRUStateVariableTable
         - sets unique IC values for different HRUs and one or multiple state variables

         or 

         :InitialTemperatureTable
           :Attributes {list of WATER state variables}
           :Units {list of units}
           {HRUID, T1,T2,T3} x nHRUs
         :EndInitialTemperatureTable

         or 
         :InitialConcentrationTable [constituent name]
           :Attributes {list of WATER state variables}
           :Units {list of units}
           {HRUID, T1,T2,T3} x nHRUs
         :EndInitialConcentrationTable
      */
      if      ((concname==""           ) && (Options.noisy)) {cout <<"   Reading HRU Initial Condition Table..."<<endl;}
      else if ((concname=="TEMPERATURE") && (Options.noisy)) {cout <<"   Reading Temperature Initial Condition Table..."<<endl; }
      else if (                             (Options.noisy)) {cout <<"   Reading Concentration Initial Condition Table for "<<concname<<"..."<<endl; }

      pp->Tokenize(s,Len);
      int c=DOESNT_EXIST;
      if(concname!="") {
        c=pModel->GetTransportModel()->GetConstituentIndex(concname);
        if(c==DOESNT_EXIST) {
          ExitGracefully("Invalid constituent name in :InitialConcentrationTable or :InitialTemperatureTable",BAD_DATA);
        }
      }

      // Read :Attributes header-------------------------------------------------
      if (strcmp(s[0],":Attributes")){
        WriteWarning(":HRUStateVariableTable command: first line must begin with :Attributes",Options.noisy);break;}
      int nSV=Len-1;
      int *SVinds=new int [nSV];
      for (i=0;i<nSV;i++)
      {
        int     layer_ind(DOESNT_EXIST);
        sv_type typ;
        typ=CStateVariable::StringToSVType(s[i+1],layer_ind,false);

        if (typ==UNRECOGNIZED_SVTYPE){
          WriteWarning(":HRUStateVariableTable: unrecognized state variable type "+to_string(s[i+1]),Options.noisy);
          SVinds[i]=DOESNT_EXIST;
        }
        else{
          SVinds[i]=pModel->GetStateVarIndex(typ,layer_ind);
          
          if(c!=DOESNT_EXIST) { //convert water storage index to corresponding constituent index
            int m=pModel->GetTransportModel()->GetLayerIndex(c,SVinds[i]);
            SVinds[i]=DOESNT_EXIST;
            if(m!=DOESNT_EXIST) {  
              SVinds[i]=pModel->GetStateVarIndex(CONSTITUENT,m);
            }
          }

          if (SVinds[i]==DOESNT_EXIST){
            WriteWarning("Initial conditions specified for state variable not in model ("+to_string(s[i+1])+")",Options.noisy);
          }

          if ((typ==ATMOS_PRECIP) || (typ==ATMOSPHERE) || (typ==GLACIER_ICE)){//initial conditions of cumulative precip, evap, and glacier loss ignored, left at zero
            SVinds[i]=DOESNT_EXIST;
          }

          ///  zero out concentrations/temperatures linked to atmos_precip, atmosphere, and glacier ice
          if(typ==CONSTITUENT) {
            i=pModel->GetTransportModel()->GetWaterStorIndexFromLayer(layer_ind);
            sv_type wattyp=pModel->GetStateVarType(i);
            if((wattyp==ATMOS_PRECIP) || (wattyp==ATMOSPHERE) || (wattyp==GLACIER_ICE)) {//initial concentrations conditions of cumulative precip, evap, and glacier loss ignored, left at zero
              SVinds[i]=DOESNT_EXIST;
            }
          } //CODE UNCHECKED
        }
      }
      
      // Read body of Table -----------------------------------------------------
      int parsedHRUs=0;
      int HRUID;
      CHydroUnit *pHRU;
      while ( (Len==0) || 
              ((strcmp(s[0],":EndHRUStateVariableTable")) && 
               (strcmp(s[0],":EndInitialTemperatureTable")) &&
               (strcmp(s[0],":EndInitialConcentrationTable"))) )
      {
        pp->Tokenize(s,Len);

        if      (IsComment(s[0], Len)){}//comment line
        else if (!strcmp(s[0],":Units")){}//ignored by Raven
        else if (!strcmp(s[0],":EndHRUStateVariableTable")){}//end command
        else if (!strcmp(s[0],":EndInitialTemperatureTable")){}//end command
        else if (!strcmp(s[0],":EndInitialConcentrationTable")){}//end command
        else //row in SV table
        {
          ExitGracefullyIf(Len!=(nSV+1),
            "Parse :HRUStateVariableTable: incorrect number of columns in HRU State Variable Table row (.rvc file)",BAD_DATA);

          ExitGracefullyIf(parsedHRUs>=pModel->GetNumHRUs(),
            "Parse: :HRUStateVariableTable: # of rows more than # of HRUs (.rvc file)",BAD_DATA_WARN);

          HRUID=s_to_i(s[0]);
          pHRU=pModel->GetHRUByID(HRUID);
          if(pHRU==NULL){
            string warn="HRU ID ["+to_string(HRUID)+"] in .rvc file not found in model";
            WriteWarning(warn,Options.noisy);
          }
          else{
            for(i=0;i<nSV;i++){
              if(SVinds[i]!=DOESNT_EXIST){
                if(c==DOESNT_EXIST) {
                  pHRU->SetStateVarValue(SVinds[i],s_to_d(s[i+1]));
                }
                else{
                  int      k=pHRU->GetGlobalIndex();
                  double val=s_to_d(s[i+1]);
                  int      m=pModel->GetStateVarLayer(SVinds[i]);
                  //cout<<" i, SVind, c, m:"<<i<<","<<SVinds[i]<<"," <<c<<","<<m<<endl;
                  SetInitialStateVar(pModel,SVinds[i],CONSTITUENT,m,k,val);
                }
              }
            }
            parsedHRUs++;
          }
        }//if Iscomment...
      }//end while 
      if(parsedHRUs!=pModel->GetNumHRUs()){
        WriteWarning("Parse: :HRUStateVariableTable: number of HRUs in .rvc file not equal to that in model",Options.noisy);
      }
      delete [] SVinds;
      break;
    }
    case(5):  //----------------------------------------------
    { /* \todo[clean]: make this command obsolete
        ":InitialConditions" {SV_NAME}
        {v1,v2,v3,v4} x nHRUs
        :EndInitialConditions
      */
      if (Len<2){pp->ImproperFormat(s);}
      else{
        if (Options.noisy) {cout <<"   Reading Initial Conditions for "<<s[1]<<endl;}
        string stateVariable= s[1];
        sv_type SVtype;
        int     SVlayer=0;
        SVtype=CStateVariable::StringToSVType(s[1],SVlayer,false);
        if (SVtype==UNRECOGNIZED_SVTYPE){
          WriteWarning("Unrecognized State Variable type " + string(s[1]) +" in :InitialConditions command",Options.noisy);
          break;
        }
        int nHRUs=pModel->GetNumHRUs();
        double *v=new double [nHRUs];
        int count =0;
        while (count<pModel->GetNumHRUs())
        {
          if(!strcmp(s[0],":EndInitialConditions"))
          { break; }
          else
          {
            pp->Tokenize(s,Len);
            if      (IsComment(s[0], Len)){}//comment line
            else if (!strcmp(s[0],":EndInitialConditions")){}//done
            else
            {
              for (int i=0;i<Len;i++)
              {
                if (count<nHRUs){
                  v[count]=s_to_d(s[i]);
                }
                count++;
              }
            }
          }

          int iSV=pModel->GetStateVarIndex(SVtype,SVlayer);
          if (iSV!=DOESNT_EXIST){
            if ((SVtype!=ATMOS_PRECIP) && (SVtype!=ATMOSPHERE)){//initial conditions of cumulative precip & evap ignored, left at zero
              for (int k=0;k<pModel->GetNumHRUs();k++){
                pModel->GetHydroUnit(k)->SetStateVarValue(iSV,v[k]);
              }
            }
          }
        }/*end while*/
        if (count!=nHRUs)
        {
          WriteWarning("Initial condition count is incorrect for state variable \""+(stateVariable)+"\"",Options.noisy);
          //zero out rest 
          int iSV=pModel->GetStateVarIndex(SVtype,SVlayer);
          if(iSV!=DOESNT_EXIST){
            if((SVtype!=ATMOS_PRECIP) && (SVtype!=ATMOSPHERE)){//initial conditions of cumulative precip & evap ignored, left at zero
              for(int k=count;k<pModel->GetNumHRUs();k++){
                pModel->GetHydroUnit(k)->SetStateVarValue(iSV,0);
              }
            }
          }
          //cout<<"       NumHRUs="<<pModel->GetNumHRUs()<<" values found="<<count<<endl;
        }
        delete [] v;
        int iSV=pModel->GetStateVarIndex(SVtype,SVlayer);
        if (iSV==DOESNT_EXIST){
          WriteWarning("Unused state Variable " + string(s[1]) +" in :InitialConditions command will be ignored",Options.noisy);
        }
      }

      break;
    }
    case(6):  //----------------------------------------------
    { /* :BasinStateVariables
          :BasinIndex ID, name
            :ChannelStorage [val]
            :RivuletStorage [val]
            :Qout [nsegs] [aQout x nsegs] [aQoutLast]
            :Qlat [nQlatHist] [aQlatHist x nQlatHist] [QlatLast]
            :Qin  [nQinHist] [aQinHist x nQinHist]
            {:ResStage [stage] [last stage]}
          :BasinIndex 1D, name
            ...
        :EndBasinStateVariables
      */
      if (Options.noisy) {cout <<"   Reading Basin State Variables"<<endl;}
      bool done=false;
      CSubBasin *pBasin=NULL;
      int SBID=DOESNT_EXIST;
      do
      {
        bool eof=pp->Tokenize(s,Len);
        if (eof){done=true;}
        if      (Len==0){}//do nothing
        else if (IsComment(s[0],Len)){}//do nothing
        else if (!strcmp(s[0],":BasinIndex"))
        {
          SBID=s_to_l(s[1]);
          pBasin=pModel->GetSubBasinByID(SBID);
          ExitGracefullyIf(pBasin==NULL,
                           "ParseInitialConditionsFile: bad basin index in .rvc file",BAD_DATA);
          if (Options.noisy) {cout <<"     Reading Basin "<<pBasin->GetID()<<": "<<pBasin->GetName()<<endl;}
        }
        else if (!strcmp(s[0],":ChannelStorage"))
        {
          if (Len>=2){
            pBasin->SetChannelStorage(s_to_d(s[1]));
          }
        }
        else if (!strcmp(s[0],":RivuletStorage"))
        {
          if (Len>=2){
            pBasin->SetRivuletStorage(s_to_d(s[1]));
          }
        }
        else if (!strcmp(s[0],":Qout"))
        {
          if (Len>2){
            int nsegs=s_to_i(s[1]);
            if (Len>=nsegs+3){
              double *aQout=new double [nsegs+1];
              for (int i=0;i<=nsegs;i++){
                aQout[i]=s_to_d(s[i+2]);
              }
              pBasin->SetQoutArray(nsegs,aQout,aQout[nsegs]);
              delete [] aQout;
            }
          }
        }
        else if (!strcmp(s[0],":Qlat"))
        {
          if (Len>2){
            int histsize=s_to_i(s[1]);
            if (Len>=histsize+3){
              double *aQlat=new double [histsize+1];
              for (int i=0;i<=histsize;i++){
                aQlat[i]=s_to_d(s[i+2]);
              }
              pBasin->SetQlatHist(histsize,aQlat,aQlat[histsize]);
              delete [] aQlat;
            }
          }
        }
        else if (!strcmp(s[0],":Qin"))
        {
          if (Len>2){
            int histsize=s_to_i(s[1]);
            if (Len>=histsize+2){
              double *aQin=new double [histsize];
              for (int i=0;i<histsize;i++){
                aQin[i]=s_to_d(s[i+2]);
              }
              pBasin->SetQinHist(histsize,aQin);
              delete [] aQin;
            }
          }
        }
        else if (!strcmp(s[0],":ResStage"))
        {
          if (Len>=3){
            pBasin->GetReservoir()->SetReservoirStage(s_to_d(s[1]),s_to_d(s[2]));
          }
        }
        else if(!strcmp(s[0],":ResFlow"))
        {
          if(Len>=3) {
            pBasin->GetReservoir()->SetInitialFlow(s_to_d(s[1]),s_to_d(s[2]),tt,Options);
          }
        }
        else if (!strcmp(s[0], ":ControlFlow"))
        {
          if (Len >= 4) {
            pBasin->GetReservoir()->SetControlFlow(s_to_i(s[1]),s_to_d(s[2]),s_to_d(s[3]));
          }
        }
        else if(!strcmp(s[0],":ResDAscale"))
        {
          if(Len>=3) {
            pBasin->GetReservoir()->SetDataAssimFactors(s_to_d(s[1]),s_to_d(s[2]));
          }
        }
        else if (!strcmp(s[0],":EndBasinStateVariables"))
        {
          done=true;
        }
      } while (!done);
      break;
    }
    case(7):  //----------------------------------------------
    {
      //:InitialReservoirFlow [SBID] [flow in m3/s]
      int SBID=s_to_l(s[1]);
      CSubBasin *pBasin=pModel->GetSubBasinByID(SBID);
      if(pBasin==NULL) {
        ExitGracefully("ParseInitialConditionsFile: bad basin index in :InitialReservoirFlow command (.rvc file)",BAD_DATA_WARN);break;
      }
      time_struct tt;
      JulianConvert(0.0,Options.julian_start_day,Options.julian_start_year,Options.calendar,tt);
      pBasin->GetReservoir()->UpdateReservoir(tt,Options); //ensures correct discharge rating curve is used to calculate flow
      pBasin->GetReservoir()->SetInitialFlow(AutoOrDouble(s[2]),AutoOrDouble(s[2]),tt,Options);
      break;
    }
    case(8):  //----------------------------------------------
    {
      //:InitialReservoirStage [SBID] [stage]
      int SBID=s_to_l(s[1]);
      CSubBasin *pBasin=pModel->GetSubBasinByID(SBID);
      if(pBasin==NULL) {
        ExitGracefully("ParseInitialConditionsFile: bad basin index in :InitialReservoirFlow command (.rvc file)",BAD_DATA_WARN); break;
      }
      if(pBasin->GetReservoir()==NULL) {
        ExitGracefully("ParseInitialConditionsFile: bad basin index in :InitialReservoirStage command (.rvc file), no reservoir exists in specified basin",BAD_DATA_WARN);break;
      }
      pBasin->GetReservoir()->SetReservoirStage(s_to_d(s[2]),s_to_d(s[2]));
      break;
    }
    case(10):  //----------------------------------------------
    {
      /*
        ":TimeStamp" [date] [hr]
      */
      //purely QA/QC check
      if (Len<3){break;}
      time_struct tt;
      tt=DateStringToTimeStruct(s[1],s[2],Options.calendar);
      if ((Options.julian_start_day!=tt.julian_day) || (Options.julian_start_year!=tt.year)){
        WriteWarning(":Timestamp command in initial conditions (.rvc) file is not consistent with :StartDate command in model (.rvi) file",Options.noisy);
      }
      break;
    }
    case(11): //------------------------------------------------
    {
      //:Nudge NUDGE_MULTIPLY [state_var] [mutiplier/add-on] [HRU Group]
      // e.g., 
      //:Nudge NUDGE_MULTIPLY SNOW 1.6 Band_1250
      //
      if(Len<5) {
        WriteWarning("Incorrect syntax for :Nudge command",Options.noisy);break;
      }
      if(pModel->GetHRUGroup(s[4])==NULL) {
        WriteWarning("Invalid HRU group in :Nudge command",Options.noisy);break;
      }
      int     SVlayer,iSV,nHRUs;
      sv_type SVtype;
      double  val;
      SVtype  =CStateVariable::StringToSVType(s[2],SVlayer,true);
      iSV     =pModel->GetStateVarIndex(SVtype,SVlayer);
      nHRUs   =pModel->GetNumHRUs();
      
      if(!strcmp(s[1],"NUDGE_MULTIPLY")) {//----------------------------------
        for(int k=0;k<nHRUs;k++) {
          if(pModel->IsInHRUGroup(k,s[4])) {
            val=pModel->GetHydroUnit(k)->GetStateVarValue(iSV);
            val*=s_to_d(s[3]);
            pModel->GetHydroUnit(k)->SetStateVarValue(iSV,val);
          }
        }
      }
      else if(!strcmp(s[1],"NUDGE_ADD")) {//----------------------------------
        for(int k=0;k<nHRUs;k++) {
          if(pModel->IsInHRUGroup(k,s[4])) {
            val=pModel->GetHydroUnit(k)->GetStateVarValue(iSV);
            val+=s_to_d(s[3]);
            pModel->GetHydroUnit(k)->SetStateVarValue(iSV,val);
          }
        }
      }
      break;
    }
    case(12):  //----------------------------------------------
    {
      /*
      :BasinTransportVariables [constit_name]
        :BasinIndex ID
          :ChannelMass [val]
          :RivuletMass [val]
          :Mout [nsegs]     [aMout     x nsegs    ] [aMoutLast]
          :Mlat [nQlatHist] [aMlatHist x nQlatHist] [MlatLast]
          :Min  [nQinHist]  [aMinHist  x nQinHist ]
          {:ResMassOut [Mout_res] [last_Mout_res]}
          {:ResMass [mass] [last mass]}
        :BasinIndex ID
        ...
      :EndBasinTransportVariables
      */
      int c=pModel->GetTransportModel()->GetConstituentIndex(concname);
      if(c==DOESNT_EXIST) {
        WriteWarning("Unrecognized constituent entry in :BasinTransportVariables within .rvc file. Command was ignored.",Options.noisy);
        break;
      }
      CConstituentModel *pConstit=pModel->GetTransportModel()->GetConstituentModel(c);
      

      int p=-1;
      if(Options.noisy) { cout <<"   Reading Basin Transport Variables"<<endl; }
      bool done=false;
      CSubBasin *pBasin=NULL;
      int SBID=DOESNT_EXIST;
      do
      {
        bool eof=pp->Tokenize(s,Len);
        if(eof) { done=true; }
        if(Len==0) {}//do nothing
        else if(IsComment(s[0],Len)) {}//do nothing
        else if(!strcmp(s[0],":BasinIndex"))
        {
          SBID=s_to_l(s[1]);
          pBasin=pModel->GetSubBasinByID(SBID);
          ExitGracefullyIf(pBasin==NULL,
            "ParseInitialConditionsFile: bad basin index in .rvc file",BAD_DATA);
          p++;
          if(Options.noisy) { cout <<"     Reading Transport Vars for Basin "<<pBasin->GetID()<<": "<<pBasin->GetName()<<endl; }
        }
        else if(!strcmp(s[0],":ChannelMass"))
        {
          if(Len>=2) {
            pConstit->SetChannelMass(p,s_to_d(s[1]));
          }
        }
        else if(!strcmp(s[0],":RivuletMass"))
        {
          if(Len>=2) {
            pConstit->SetRivuletMass(p,s_to_d(s[1]));
          }
        }
        else if(!strcmp(s[0],":Mout"))
        {
          if(Len>2) {
            int nsegs=s_to_i(s[1]);
            if(Len>=nsegs+3) {
              double *aMout=new double[nsegs+1];
              for(int i=0;i<=nsegs;i++) {
                aMout[i]=s_to_d(s[i+2]);
              }
              pConstit->SetMoutArray(p,nsegs,aMout,aMout[nsegs]);
              delete[] aMout;
            }
          }
        }
        else if(!strcmp(s[0],":Mlat"))
        {
          if(Len>2) {
            int histsize=s_to_i(s[1]);
            if(Len>=histsize+3) {
              double *aMlat=new double[histsize+1];
              for(int i=0;i<=histsize;i++) {
                aMlat[i]=s_to_d(s[i+2]);
              }
              pConstit->SetMlatHist(p,histsize,aMlat,aMlat[histsize]);
              delete[] aMlat;
            }
          }
        }
        else if(!strcmp(s[0],":Min"))
        {
          if(Len>2) {
            int histsize=s_to_i(s[1]);
            if(Len>=histsize+2) {
              double *aMin=new double[histsize];
              for(int i=0;i<histsize;i++) {
                aMin[i]=s_to_d(s[i+2]);
              }
              pConstit->SetMinHist(p,histsize,aMin);
              delete[] aMin;
            }
          }
        }
        else if(!strcmp(s[0],":ResMass"))
        {
          if(Len>=3) {
            pConstit->SetInitialReservoirMass(p,s_to_d(s[1]),s_to_d(s[2]));
          }
        }
        else if(!strcmp(s[0],":ResMassOut"))
        {
          if(Len>=3) {
            pConstit->SetReservoirMassOutflow(p,s_to_d(s[1]),s_to_d(s[2]));
          }
        }
        else if(!strcmp(s[0],":EndBasinTransportVariables"))
        {
          done=true;
        }
      } while(!done);
      break;
    }
    default://------------------------------------------------
    {
      char firstChar = *(s[0]);
      switch(firstChar)
      {
      case ':':
      {
        if     (!strcmp(s[0],":FileType"    )) {if (Options.noisy){cout<<"Filetype"<<endl;}}//do nothing
        else if(!strcmp(s[0],":Application" )) {if (Options.noisy){cout<<"Application"<<endl;}}//do nothing
        else if(!strcmp(s[0],":Version"     )) {if (Options.noisy){cout<<"Version"<<endl;}}//do nothing
        else if(!strcmp(s[0],":WrittenBy"   )) {if (Options.noisy){cout<<"WrittenBy"<<endl;}}//do nothing
        else if(!strcmp(s[0],":CreationDate")) {if (Options.noisy){cout<<"CreationDate"<<endl;}}//do nothing
        else if(!strcmp(s[0],":SourceFile"  )) {if (Options.noisy){cout<<"SourceFile"<<endl;}}//do nothing
        else
        {
          string warn ="IGNORING unrecognized command: " + string(s[0])+ " in .rvc file";
          WriteWarning(warn,Options.noisy);
        }
      }
      break;
      default:
      {
        string errString = "Unrecognized command in .rvc file:\n   " + string(s[0]);
        ExitGracefully(errString.c_str(),BAD_DATA_WARN);//STRICT
      }
      break;
      }
    }
    }//end switch(code)

    end_of_file=pp->Tokenize(s,Len);

    //return after file redirect, if in secondary file
    if ((end_of_file) && (pMainParser!=NULL))
    {
      INPUT2.clear();
      INPUT2.close();
      delete pp;
      pp=pMainParser;
      pMainParser=NULL;
      end_of_file=pp->Tokenize(s,Len);
    }
  } //end while !end_of_file
  IC.close();

  // check quality of initial state variables
  //======================================================================
  CHydroUnit *pHRU;
  double *v=new double [pModel->GetNumStateVars()];
  for (int k=0;k<pModel->GetNumHRUs();k++)
  {
    pHRU=pModel->GetHydroUnit(k);
    for (int i=0; i<pModel->GetNumStateVars(); i++)
    {
      v[i]=pHRU->GetStateVarValue(i);
    }
    for (int i=0; i<pModel->GetNumStateVars(); i++)
    {
      double maxv=max(pHRU->GetStateVarMax(i,v,Options,true),0.0); //ignores all variable maximum thresholds that are dependent upon model state
      if ((v[i]-maxv)>PRETTY_SMALL)// check for capacity
      {
        string name=CStateVariable::GetStateVarLongName(pModel->GetStateVarType(i),pModel->GetStateVarLayer(i));
        string warn ="maximum state variable limit exceeded in initial conditions for " + name+ " (in HRU "+to_string(pHRU->GetID())+") in .rvc file";
        WriteWarning(warn,Options.noisy);

        if (!Options.keepUBCWMbugs){
          pHRU->SetStateVarValue(i,maxv);
        }
      }
    }
  }
  delete [] v;

  delete pp;
  pp=NULL;
  return true;
}

void SetInitialStateVar(CModel *&pModel,const int SVind,const sv_type typ,const int m,const int k,const double &val) 
{
  if(typ!=CONSTITUENT) //native units, no problem
  {
    pModel->GetHydroUnit(k)->SetStateVarValue(SVind,val);
  }
  else { // specifying mg/L or degrees C, must be converted to mg/m2 or MJ/m2
    string name =pModel->GetTransportModel()->GetConstituentTypeName(m);
    int       c=pModel->GetTransportModel()->GetConstituentIndex(name);

    if(c==DOESNT_EXIST) {
      cout<<name<<" "<<m<<endl;
      ExitGracefully("Constituent in .rvc file does not exist.",BAD_DATA_WARN);
      return;
    }

    int   iStor=pModel->GetTransportModel()->GetWaterStorIndexFromLayer(m);
    double  vol=pModel->GetHydroUnit(k)->GetStateVarValue(iStor); //[mm] Assumes this has already been initialized (this will be true for .rvc file)
    
    if(pModel->GetTransportModel()->GetConstituentModel(c)->GetType()==ENTHALPY) 
    {
      //specified in C, convert to MJ/m2
      double pctfroz=0.0;
      if (val<0.0){pctfroz=1.0;} //treats all 0 degree water as unfrozen
      double energy=ConvertTemperatureToVolumetricEnthalpy(val,pctfroz)*vol/MM_PER_METER; //[C]->[MJ/m3]*[m]=[MJ/m2]
      pModel->GetHydroUnit(k)->SetStateVarValue(SVind,energy);
    }
    else 
    {
      //specified in mg/L, convert to mg/m2
      double mass=val*LITER_PER_M3*vol/MM_PER_METER; //[mg/L]->[mg/L]*[L/m3]*[mm]/[mm/m]=[mg/m2]
      pModel->GetHydroUnit(k)->SetStateVarValue(SVind,mass);
    }
  }
}