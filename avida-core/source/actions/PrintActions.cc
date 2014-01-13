/*
 *  PrintActions.cc
 *  Avida
 *
 *  Created by David on 5/11/06.
 *  Copyright 1999-2011 Michigan State University. All rights reserved.
 *
 *
 *  This file is part of Avida.
 *
 *  Avida is free software; you can redistribute it and/or modify it under the terms of the GNU Lesser General Public License
 *  as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
 *
 *  Avida is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License along with Avida.
 *  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "PrintActions.h"

#include "avida/core/Feedback.h"
#include "avida/core/InstructionSequence.h"
#include "avida/core/UniverseDriver.h"
#include "avida/data/Manager.h"
#include "avida/data/Package.h"
#include "avida/data/Recorder.h"
#include "avida/output/File.h"
#include "avida/systematics/Arbiter.h"
#include "avida/systematics/Group.h"
#include "avida/systematics/Manager.h"

#include "avida/private/util/GenomeLoader.h"

#include "apto/rng.h"

#include "cAction.h"
#include "cActionLibrary.h"
#include "cCPUTestInfo.h"
#include "cEnvironment.h"
#include "cHardwareBase.h"
#include "cHardwareManager.h"
#include "cInstSet.h"
#include "cOrganism.h"
#include "cPopulation.h"
#include "cPopulationCell.h"
#include "cStats.h"
#include "cWorld.h"
#include "cUserFeedback.h"
#include "cBirthEntry.h"

#include <cmath>
#include <cerrno>
#include <map>
#include <algorithm>

class cBioGroup;

using namespace Avida;


#define STATS_OUT_FILE(METHOD, DEFAULT)                                                   /*  1 */ \
class cAction ## METHOD : public cAction {                                                /*  2 */ \
private:                                                                                  /*  3 */ \
cString m_filename;                                                                     /*  4 */ \
public:                                                                                   /*  5 */ \
cAction ## METHOD(cWorld* world, const cString& args, Feedback&) : cAction(world, args)            /*  6 */ \
{                                                                                       /*  7 */ \
cString largs(args);                                                                  /*  8 */ \
if (largs == "") m_filename = #DEFAULT; else m_filename = largs.PopWord();            /*  9 */ \
}                                                                                       /* 10 */ \
static const cString GetDescription() { return "Arguments: [string fname=\"" #DEFAULT "\"]"; }  /* 11 */ \
void Process(cAvidaContext&) { m_world->GetStats().METHOD(m_filename); }            /* 12 */ \
}                                                                                         /* 13 */ \

STATS_OUT_FILE(PrintAverageData,            average.dat         );
STATS_OUT_FILE(PrintErrorData,              error.dat           );
STATS_OUT_FILE(PrintVarianceData,           variance.dat        );
STATS_OUT_FILE(PrintCountData,              count.dat           );
STATS_OUT_FILE(PrintTotalsData,             totals.dat          );
STATS_OUT_FILE(PrintTasksData,              tasks.dat           );
STATS_OUT_FILE(PrintThreadsData,            threads.dat         );
STATS_OUT_FILE(PrintTasksExeData,           tasks_exe.dat       );
STATS_OUT_FILE(PrintNewTasksData,           newtasks.dat	);
STATS_OUT_FILE(PrintNewReactionData,	    newreactions.dat	);
STATS_OUT_FILE(PrintNewTasksDataPlus,       newtasksplus.dat	);
STATS_OUT_FILE(PrintTasksQualData,          tasks_quality.dat   );
STATS_OUT_FILE(PrintReactionData,           reactions.dat       );
STATS_OUT_FILE(PrintReactionExeData,        reactions_exe.dat   );
STATS_OUT_FILE(PrintCurrentReactionData,    cur_reactions.dat   );
STATS_OUT_FILE(PrintReactionRewardData,     reaction_reward.dat );
STATS_OUT_FILE(PrintCurrentReactionRewardData,     cur_reaction_reward.dat );
STATS_OUT_FILE(PrintTimeData,               time.dat            );
STATS_OUT_FILE(PrintExtendedTimeData,       xtime.dat           );
STATS_OUT_FILE(PrintMutationRateData,       mutation_rates.dat  );
STATS_OUT_FILE(PrintDivideMutData,          divide_mut.dat      );
STATS_OUT_FILE(PrintPreyAverageData,        prey_average.dat   );
STATS_OUT_FILE(PrintPredatorAverageData,    predator_average.dat   );
STATS_OUT_FILE(PrintTopPredatorAverageData,    top_pred_average.dat   );
STATS_OUT_FILE(PrintPreyErrorData,          prey_error.dat   );
STATS_OUT_FILE(PrintPredatorErrorData,      predator_error.dat   );
STATS_OUT_FILE(PrintTopPredatorErrorData,      top_pred_error.dat   );
STATS_OUT_FILE(PrintPreyVarianceData,       prey_variance.dat   );
STATS_OUT_FILE(PrintPredatorVarianceData,   predator_variance.dat   );
STATS_OUT_FILE(PrintTopPredatorVarianceData,   top_pred_variance.dat   );
STATS_OUT_FILE(PrintSenseData,              sense.dat           );
STATS_OUT_FILE(PrintSenseExeData,           sense_exe.dat       );


STATS_OUT_FILE(PrintOrganismLocation,       location.dat);

// @WRE: Added output event for collected visit counts
STATS_OUT_FILE(PrintDynamicMaxMinData,	    maxmin.dat			);
STATS_OUT_FILE(PrintNumOrgsKilledData,      orgs_killed.dat);

// group formation
STATS_OUT_FILE(PrintTargets,                  targets.dat);
STATS_OUT_FILE(PrintMimicDisplays,            mimics.dat);
STATS_OUT_FILE(PrintTopPredTargets,           top_pred_targets.dat);


class cActionPrintResourceData : public cAction
{
private:
  cString m_filename;
public:
  cActionPrintResourceData(cWorld* world, const cString& args, Feedback&) : cAction(world, args)
  {
    cString largs(args);
    if (largs == "") m_filename = "resource.dat"; else m_filename = largs.PopWord();
  }
  static const cString GetDescription() { return "Arguments: [string fname=\"resource.dat\"]"; }
  void Process(cAvidaContext& ctx)
  {
    m_world->GetPopulation().GetResources().UpdateResStats(ctx);
    m_world->GetStats().PrintResourceData(m_filename);
  }
};

class cActionPrintResourceLocData : public cAction
{
private:
  cString m_filename;
public:
  cActionPrintResourceLocData(cWorld* world, const cString& args, Feedback&) : cAction(world, args)
  {
    cString largs(args);
    if (largs == "") m_filename = "resourceloc.dat"; else m_filename = largs.PopWord();
  }
  static const cString GetDescription() { return "Arguments: [string fname=\"resourceloc.dat\"]"; }
  void Process(cAvidaContext& ctx)
  {
    m_world->GetPopulation().GetResources().TriggerDoUpdates(ctx);
    m_world->GetStats().PrintResourceLocData(m_filename, ctx);
  }
};

class cActionPrintResWallLocData : public cAction
{
private:
  cString m_filename;
public:
  cActionPrintResWallLocData(cWorld* world, const cString& args, Feedback&) : cAction(world, args)
  {
    cString largs(args);
    if (largs == "") m_filename = "reswallloc.dat"; else m_filename = largs.PopWord();
  }
  static const cString GetDescription() { return "Arguments: [string fname=\"reswallloc.dat\"]"; }
  void Process(cAvidaContext& ctx)
  {
    m_world->GetPopulation().GetResources().TriggerDoUpdates(ctx);
    m_world->GetStats().PrintResWallLocData(m_filename, ctx);
  }
};


class cActionPrintData : public cAction
{
private:
  cString m_filename;
  cString m_format;
public:
  cActionPrintData(cWorld* world, const cString& args, Feedback&) : cAction(world, args)
  {
    cString largs(args);
    m_filename = largs.PopWord();
    m_format = largs.PopWord();
  }
  
  static const cString GetDescription() { return "Arguments: <cString fname> <cString format>"; }
  
  void Process(cAvidaContext&)
  {
    m_world->GetStats().PrintDataFile(m_filename, m_format, ',');
  }
};

class cActionPrintInstructionData : public cAction, public Data::Recorder
{
private:
  cString m_filename;
  Apto::String m_inst_set;
  Data::DataID m_data_id;
  Data::PackagePtr m_data;
  
public:
  cActionPrintInstructionData(cWorld* world, const cString& args, Feedback&)
  : cAction(world, args), m_inst_set(world->GetHardwareManager().GetDefaultInstSet().GetInstSetName())
  {
    cString largs(args);
    largs.Trim();
    if (largs.GetSize()) m_filename = largs.PopWord();
    else {
      if (m_filename == "") m_filename = "instruction.dat";
    }
    if (largs.GetSize()) m_inst_set = (const char*)largs.PopWord();
    
    if (m_filename == "") m_filename.Set("instruction-%s.dat", (const char*)m_inst_set);
    
    m_data_id = Apto::FormatStr("core.population.inst_exec_counts[%s]", (const char*)m_inst_set);
    
    Data::RecorderPtr thisPtr(this);
    this->AddReference();
    m_world->GetDataManager()->AttachRecorder(thisPtr);
  }
  
  static const cString GetDescription() { return "Arguments: [string fname=\"instruction-${inst_set}.dat\"] [string inst_set]"; }
  
  Data::ConstDataSetPtr RequestedData() const
  {
    Data::DataSetPtr ds(new Data::DataSet);
    ds->Insert(m_data_id);
    return ds;
  }
  
  
  void NotifyData(Update, Data::DataRetrievalFunctor retrieve_data)
  {
    m_data = retrieve_data(m_data_id);
  }
  
  void Process(cAvidaContext&)
  {
    const cInstSet& is = m_world->GetHardwareManager().GetInstSet(m_inst_set);
    Avida::Output::FilePtr df = Avida::Output::File::StaticWithPath(m_world->GetNewWorld(), (const char*)m_filename);
    
    df->WriteComment("Avida instruction execution data");
    df->WriteTimeStamp();
    
    df->Write(m_world->GetStats().GetUpdate(), "Update");
    
    if (m_data) {
      for (int i = 0; i < m_data->NumComponents(); i++) {
        df->Write(m_data->GetComponent(i)->IntValue(), is.GetName(i));
      }
    }
    
    df->Endl();
  }
};

class cActionPrintPreyInstructionData : public cAction
{
private:
  cString m_filename;
  cString m_inst_set;
  
public:
  cActionPrintPreyInstructionData(cWorld* world, const cString& args, Feedback&)
  : cAction(world, args), m_inst_set(world->GetHardwareManager().GetDefaultInstSet().GetInstSetName())
  {
    cString largs(args);
    largs.Trim();
    if (largs.GetSize()) m_filename = largs.PopWord();
    else {
      if (m_filename == "") m_filename = "prey_instruction.dat";
    }
    if (largs.GetSize()) m_inst_set = largs.PopWord();
    
    if (m_filename == "") m_filename.Set("prey_instruction-%s.dat", (const char*)m_inst_set);
  }
  
  static const cString GetDescription() { return "Arguments: [string fname=\"prey_instruction-${inst_set}.dat\"] [string inst_set]"; }
  
  void Process(cAvidaContext&)
  {
    m_world->GetStats().PrintPreyInstructionData(m_filename, m_inst_set);
  }
};

class cActionPrintPredatorInstructionData : public cAction
{
private:
  cString m_filename;
  cString m_inst_set;
  
public:
  cActionPrintPredatorInstructionData(cWorld* world, const cString& args, Feedback&)
  : cAction(world, args), m_inst_set(world->GetHardwareManager().GetDefaultInstSet().GetInstSetName())
  {
    cString largs(args);
    largs.Trim();
    if (largs.GetSize()) m_filename = largs.PopWord();
    else {
      if (m_filename == "") m_filename = "predator_instruction.dat";
    }
    if (largs.GetSize()) m_inst_set = largs.PopWord();
    
    if (m_filename == "") m_filename.Set("predator_instruction-%s.dat", (const char*)m_inst_set);
  }
  
  static const cString GetDescription() { return "Arguments: [string fname=\"predator_instruction-${inst_set}.dat\"] [string inst_set]"; }
  
  void Process(cAvidaContext&)
  {
    m_world->GetStats().PrintPredatorInstructionData(m_filename, m_inst_set);
  }
};

class cActionPrintTopPredatorInstructionData : public cAction
{
private:
  cString m_filename;
  cString m_inst_set;
  
public:
  cActionPrintTopPredatorInstructionData(cWorld* world, const cString& args, Feedback&)
  : cAction(world, args), m_inst_set(world->GetHardwareManager().GetDefaultInstSet().GetInstSetName())
  {
    cString largs(args);
    largs.Trim();
    if (largs.GetSize()) m_filename = largs.PopWord();
    else {
      if (m_filename == "") m_filename = "top_pred_instruction.dat";
    }
    if (largs.GetSize()) m_inst_set = largs.PopWord();
    
    if (m_filename == "") m_filename.Set("top_pred_instruction-%s.dat", (const char*)m_inst_set);
  }
  
  static const cString GetDescription() { return "Arguments: [string fname=\"top_pred_instruction-${inst_set}.dat\"] [string inst_set]"; }
  
  void Process(cAvidaContext& ctx)
  {
    m_world->GetStats().PrintTopPredatorInstructionData(m_filename, m_inst_set);
  }
};

class cActionPrintPreyFromSensorInstructionData : public cAction
{
private:
  cString m_filename;
  cString m_inst_set;
  
public:
  cActionPrintPreyFromSensorInstructionData(cWorld* world, const cString& args, Feedback&)
  : cAction(world, args), m_inst_set(world->GetHardwareManager().GetDefaultInstSet().GetInstSetName())
  {
    cString largs(args);
    largs.Trim();
    if (largs.GetSize()) m_filename = largs.PopWord();
    else {
      if (m_filename == "") m_filename = "prey_from_sensor_exec.dat";
    }
    if (largs.GetSize()) m_inst_set = largs.PopWord();
    
    if (m_filename == "") m_filename.Set("prey_from_sensor_exec-%s.dat", (const char*)m_inst_set);
  }
  
  static const cString GetDescription() { return "Arguments: [string fname=\"prey_from_sensor_exec-${inst_set}.dat\"] [string inst_set]"; }
  
  void Process(cAvidaContext& ctx)
  {
    m_world->GetStats().PrintPreyFromSensorInstructionData(m_filename, m_inst_set);
  }
};

class cActionPrintPredatorFromSensorInstructionData : public cAction
{
private:
  cString m_filename;
  cString m_inst_set;
  
public:
  cActionPrintPredatorFromSensorInstructionData(cWorld* world, const cString& args, Feedback&)
  : cAction(world, args), m_inst_set(world->GetHardwareManager().GetDefaultInstSet().GetInstSetName())
  {
    cString largs(args);
    largs.Trim();
    if (largs.GetSize()) m_filename = largs.PopWord();
    else {
      if (m_filename == "") m_filename = "predator_from_sensor_exec.dat";
    }
    if (largs.GetSize()) m_inst_set = largs.PopWord();
    
    if (m_filename == "") m_filename.Set("predator_from_sensor_exec-%s.dat", (const char*)m_inst_set);
  }
  
  static const cString GetDescription() { return "Arguments: [string fname=\"predator_from_sensor_exec-${inst_set}.dat\"] [string inst_set]"; }
  
  void Process(cAvidaContext& ctx)
  {
    m_world->GetStats().PrintPredatorFromSensorInstructionData(m_filename, m_inst_set);
  }
};

class cActionPrintTopPredatorFromSensorInstructionData : public cAction
{
private:
  cString m_filename;
  cString m_inst_set;
  
public:
  cActionPrintTopPredatorFromSensorInstructionData(cWorld* world, const cString& args, Feedback&)
  : cAction(world, args), m_inst_set(world->GetHardwareManager().GetDefaultInstSet().GetInstSetName())
  {
    cString largs(args);
    largs.Trim();
    if (largs.GetSize()) m_filename = largs.PopWord();
    else {
      if (m_filename == "") m_filename = "top_pred_from_sensor_exec.dat";
    }
    if (largs.GetSize()) m_inst_set = largs.PopWord();
    
    if (m_filename == "") m_filename.Set("top_pred_from_sensor_exec-%s.dat", (const char*)m_inst_set);
  }
  
  static const cString GetDescription() { return "Arguments: [string fname=\"top_pred_from_sensor_exec-${inst_set}.dat\"] [string inst_set]"; }
  
  void Process(cAvidaContext& ctx)
  {
    m_world->GetStats().PrintTopPredatorFromSensorInstructionData(m_filename, m_inst_set);
  }
};



class cActionPrintKilledPreyFTData : public cAction
{
private:
  cString m_filename;
  
public:
  cActionPrintKilledPreyFTData(cWorld* world, const cString& args, Feedback&)
  : cAction(world, args)
  {
    cString largs(args);
    largs.Trim();
    if (largs.GetSize()) m_filename = largs.PopWord();
    else {
      if (m_filename == "") m_filename = "killed_prey.dat";
    }
    if (m_filename == "") m_filename.Set("killed_prey.dat");
  }
  
  static const cString GetDescription() { return "Arguments: [string fname=\"killed_prey.dat\"]"; }

  void Process(cAvidaContext& ctx)
  {
    m_world->GetStats().PrintKilledPreyFTData(m_filename);
  }
};



class cActionPrintInstructionAbundanceHistogram : public cAction
{
private:
  cString m_filename;
  cString m_inst_set;
  
public:
  cActionPrintInstructionAbundanceHistogram(cWorld* world, const cString& args, Feedback&)
  : cAction(world, args), m_inst_set(world->GetHardwareManager().GetDefaultInstSet().GetInstSetName())
  {
    cString largs(args);
    largs.Trim();
    if (largs.GetSize()) m_filename = largs.PopWord();
    if (largs.GetSize()) m_inst_set = largs.PopWord();
    else {
      if (m_filename == "") m_filename = "instruction_histogram.dat";
    }
    
    if (m_filename == "") m_filename.Set("instruction_histogram-%s.dat", (const char*)m_inst_set);
  }
  
  static const cString GetDescription() { return "Arguments: [string fname=\"instruction_histogram-${inst_set}.dat\"] [string inst_set]"; }
  void Process(cAvidaContext&)
  {
    cPopulation& population = m_world->GetPopulation();
    
    // ----- number of instructions available?
    const cInstSet& is = m_world->GetHardwareManager().GetInstSet((const char*)m_inst_set);
    
    Apto::Array<int> inst_counts(is.GetSize());
    inst_counts.SetAll(0);
    
    //looping through all CPUs counting up instructions
    const int num_cells = population.GetSize();
    for (int x = 0; x < num_cells; x++) {
      cPopulationCell& cell = population.GetCell(x);
      if (cell.IsOccupied() && cell.GetOrganism()->GetGenome().Properties().Get("instset").StringValue() == is.GetInstSetName()) {
        // access this CPU's code block
        InstMemSpace& cpu_mem = cell.GetOrganism()->GetHardware().GetMemory();
        const int mem_size = cpu_mem.GetSize();
        for (int y = 0; y < mem_size; y++) inst_counts[cpu_mem[y].GetOp()]++;
      }
    }
    
    // ----- output instruction counts
    Avida::Output::FilePtr df = Avida::Output::File::StaticWithPath(m_world->GetNewWorld(), (const char*)m_filename);
    df->Write(m_world->GetStats().GetUpdate(), "Update");
    for (int i = 0; i < is.GetSize(); i++) df->Write(inst_counts[i], is.GetName(i));
    df->Endl();
  }
};


class cActionPrintDepthHistogram : public cAction
{
private:
  cString m_filename;
public:
  cActionPrintDepthHistogram(cWorld* world, const cString& args, Feedback&) : cAction(world, args)
  {
    cString largs(args);
    if (largs == "") m_filename = "depth_histogram.dat"; else m_filename = largs.PopWord();
  }
  
  static const cString GetDescription() { return "Arguments: [string fname=\"depth_histogram.dat\"]"; }
  void Process(cAvidaContext&)
  {
    // Output format:    update  min  max  histogram_values...
    int min = INT_MAX;
    int max = 0;
    
    // Two pass method
    
    // Loop through all genotypes getting min and max values
    Systematics::ManagerPtr classmgr = Systematics::Manager::Of(m_world->GetNewWorld());
    Systematics::Arbiter::IteratorPtr it = classmgr->ArbiterForRole("genotype")->Begin();
    
    while (it->Next()) {
      Systematics::GroupPtr bg = it->Get();
      if (bg->Depth() < min) min = bg->Depth();
      if (bg->Depth() > max) max = bg->Depth();
    }
    assert(max >= min);
    
    // Allocate the array for the bins (& zero)
    Apto::Array<int> n(max - min + 1);
    n.SetAll(0);
    
    // Loop through all genotypes binning the values
    it = classmgr->ArbiterForRole("genotype")->Begin();
    while (it->Next()) {
      n[it->Get()->Depth() - min] += it->Get()->NumUnits();
    }
    
    Avida::Output::FilePtr df = Avida::Output::File::StaticWithPath(m_world->GetNewWorld(), (const char*)m_filename);
    df->Write(m_world->GetStats().GetUpdate(), "Update");
    df->Write(min, "Minimum");
    df->Write(max, "Maximum");
    for (int i = 0; i < n.GetSize(); i++)  df->WriteAnonymous(n[i]);
    df->Endl();
  }
};




class cActionEcho : public cAction
{
private:
  cString m_filename;
public:
  cActionEcho(cWorld* world, const cString& args, Feedback&) : cAction(world, args) { ; }
  
  static const cString GetDescription() { return "Arguments: <cString message>"; }
  
  void Process(cAvidaContext& ctx)
  {
    if (m_args == "") {
      m_args.Set("Echo : Update = %f\t AveGeneration = %f", m_world->GetStats().GetUpdate(),
                 m_world->GetStats().SumGeneration().Average());
    }
    ctx.Driver().Feedback().Notify(m_args);
  }
};


class cActionPrintGenotypeAbundanceHistogram : public cAction
{
private:
  cString m_filename;
  
public:
  cActionPrintGenotypeAbundanceHistogram(cWorld* world, const cString& args, Feedback&) : cAction(world, args)
  {
    cString largs(args);
    if (largs == "") m_filename = "genotype_abundance_histogram.dat"; else m_filename = largs.PopWord();
  }
  
  static const cString GetDescription() { return "Arguments: [string fname=\"genotype_abundance_histogram.dat\"]"; }
  void Process(cAvidaContext&)
  {
    // Allocate array for the histogram & zero it
    Systematics::ManagerPtr classmgr = Systematics::Manager::Of(m_world->GetNewWorld());
    Systematics::Arbiter::IteratorPtr it = classmgr->ArbiterForRole("genotype")->Begin();
    Apto::Array<int> hist(it->Next()->NumUnits());
    hist.SetAll(0);
    
    // Loop through all genotypes binning the values
    do {
      assert(it->Get()->NumUnits() - 1 >= 0);
      assert(it->Get()->NumUnits() - 1 < hist.GetSize());
      hist[it->Get()->NumUnits() - 1]++;
    } while (it->Next());
    
    Avida::Output::FilePtr df = Avida::Output::File::StaticWithPath(m_world->GetNewWorld(), (const char*)m_filename);
    df->Write(m_world->GetStats().GetUpdate(), "Update");
    for (int i = 0; i < hist.GetSize(); i++) df->Write(hist[i],"");
    df->Endl();
  }
};




/*
 Write the currently dominant genotype to disk.
 
 Parameters:
 filename (string)
 The name under which the genotype should be saved. If no
 filename is given, the genotype is saved into the directory
 archive, under the name that the archive has associated with
 this genotype.
 */
class cActionPrintDominantGenotype : public cAction
{
private:
  cString m_filename;
  
public:
  cActionPrintDominantGenotype(cWorld* world, const cString& args, Feedback&) : cAction(world, args), m_filename("")
  {
    cString largs(args);
    if (largs.GetSize()) m_filename = largs.PopWord();
  }
  
  static const cString GetDescription() { return "Arguments: [string fname='']"; }
  
  void Process(cAvidaContext& ctx)
  {
    Systematics::ManagerPtr classmgr = Systematics::Manager::Of(m_world->GetNewWorld());
    Systematics::Arbiter::IteratorPtr it = classmgr->ArbiterForRole("genotype")->Begin();
    Systematics::GroupPtr bg = it->Next();
    if (bg) {
      cString filename(m_filename);
      if (filename == "") filename.Set("archive/%s.org", (const char*)bg->Properties().Get("name").StringValue());
      cTestCPU* testcpu = m_world->GetHardwareManager().CreateTestCPU(ctx);
      testcpu->PrintGenome(ctx, Genome(bg->Properties().Get("genome")), filename, m_world->GetStats().GetUpdate());
      delete testcpu;
    }
  }
};

class cActionPrintDominantGroupGenotypes : public cAction
{
private:
  cString m_filename;
  
public:
  cActionPrintDominantGroupGenotypes(cWorld* world, const cString& args, Feedback&) : cAction(world, args), m_filename("")
  {
    cString largs(args);
    if (largs.GetSize()) m_filename = largs.PopWord();
  }
  
  static const cString GetDescription() { return "Arguments: [string fname='']"; }
  
  void Process(cAvidaContext& ctx)
  {
    Systematics::ManagerPtr classmgr = Systematics::Manager::Of(m_world->GetNewWorld());
    Systematics::Arbiter::IteratorPtr it = classmgr->ArbiterForRole("genotype")->Begin();
    int num_groups = 0;
    map<int,int> groups_formed = m_world->GetPopulation().GetFormedGroups();    
    map <int,int>::iterator itr;    
    for(itr = groups_formed.begin();itr!=groups_formed.end();itr++) {
      double cur_size = itr->second;
      if (cur_size > 0) num_groups++; 
    }
    
    Apto::Array<int, Apto::Smart> birth_groups_checked;
    
    for (int i = 0; i < num_groups; i++) {
      Systematics::GroupPtr bg = it->Next();
      bool already_used = false;
      
      if (!bg) break;

      if (bg && ((bool)Apto::StrAs(bg->Properties().Get("threshold")) || i == 0)) {
        int last_birth_group_id = Apto::StrAs(bg->Properties().Get("last_group_id")); 
        int last_birth_cell = Apto::StrAs(bg->Properties().Get("last_birth_cell"));
        int last_birth_forager_type = Apto::StrAs(bg->Properties().Get("last_forager_type")); 
        if (i != 0) {
          for (int j = 0; j < birth_groups_checked.GetSize(); j++) {
            if (last_birth_group_id == birth_groups_checked[j]) {
              already_used = true;
              i--;
              break;
            }
          }
        }
        if (!already_used) birth_groups_checked.Push(last_birth_group_id);
        if (already_used) continue;
        
        cString filename(m_filename);
        if (filename == "") filename.Set("archive/grp%d_ft%d_%s.org", last_birth_group_id, last_birth_forager_type, (const char*)bg->Properties().Get("name").StringValue());
        else filename = filename.Set(filename + "grp%d_ft%d", last_birth_group_id, last_birth_forager_type); 

        // need a random number generator to pass to testcpu that does not affect any other random number pulls (since this is just for printing the genome)
        Apto::RNG::AvidaRNG rng(0);
        cAvidaContext ctx2(&m_world->GetDriver(), rng);
        cTestCPU* testcpu = m_world->GetHardwareManager().CreateTestCPU(ctx2);
        testcpu->PrintGenome(ctx2, Genome(bg->Properties().Get("genome")), filename, m_world->GetStats().GetUpdate(), true, last_birth_cell, last_birth_group_id, last_birth_forager_type);
        delete testcpu;
      }
    }
  }
};

class cActionPrintDominantForagerGenotypes : public cAction
{
private:
  cString m_filename;
  
public:
  cActionPrintDominantForagerGenotypes(cWorld* world, const cString& args, Feedback&) : cAction(world, args), m_filename("")
  {
    cString largs(args);
    if (largs.GetSize()) m_filename = largs.PopWord();
  }
  
  static const cString GetDescription() { return "Arguments: [string fname='']"; }
  
  void Process(cAvidaContext& ctx)
  {
    Systematics::ManagerPtr classmgr = Systematics::Manager::Of(m_world->GetNewWorld());
    Systematics::Arbiter::IteratorPtr it = classmgr->ArbiterForRole("genotype")->Begin();
    int num_fts = 1;
    if (m_world->GetConfig().PRED_PREY_SWITCH.Get() == -2 || m_world->GetConfig().PRED_PREY_SWITCH.Get() > -1) num_fts = 3;
    else num_fts = 1;  // account for -1's
    std::set<int> fts_avail = m_world->GetEnvironment().GetTargetIDs();
    set <int>::iterator itr;    
    for(itr = fts_avail.begin();itr!=fts_avail.end();itr++) if (*itr != -1 && *itr != -2 && *itr != -3) num_fts++;
    
    Apto::Array<int, Apto::Smart> birth_forage_types_checked;
    
    for (int i = 0; i < num_fts; i++) {
      bool already_used = false;
      Systematics::GroupPtr bg = it->Next();
      
      if (!bg) break;
      
      if (bg && ((bool)Apto::StrAs(bg->Properties().Get("threshold")) || i == 0)) {
        int last_birth_group_id = Apto::StrAs(bg->Properties().Get("last_group_id")); 
        int last_birth_cell = Apto::StrAs(bg->Properties().Get("last_birth_cell"));
        int last_birth_forager_type = Apto::StrAs(bg->Properties().Get("last_forager_type")); 
        if (i != 0) {
          for (int j = 0; j < birth_forage_types_checked.GetSize(); j++) {
            if (last_birth_forager_type == birth_forage_types_checked[j]) { 
              already_used = true; 
              i--;
              break; 
            }
          }
        }
        if (!already_used) birth_forage_types_checked.Push(last_birth_forager_type);
        if (already_used) continue;
        
        
        cString filename(m_filename);
        if (filename == "") filename.Set("archive/ft%d_grp%d_%s.org", last_birth_forager_type, last_birth_group_id, (const char*)bg->Properties().Get("name").StringValue());
        else filename = filename.Set(filename + ".ft%d_grp%d", last_birth_forager_type, last_birth_group_id); 

        // need a random number generator to pass to testcpu that does not affect any other random number pulls (since this is just for printing the genome)
        Apto::RNG::AvidaRNG rng(0);
        cAvidaContext ctx2(&m_world->GetDriver(), rng);
        cTestCPU* testcpu = m_world->GetHardwareManager().CreateTestCPU(ctx2);
        testcpu->PrintGenome(ctx2, Genome(bg->Properties().Get("genome")), filename, m_world->GetStats().GetUpdate(), true, last_birth_cell, last_birth_group_id, last_birth_forager_type);
        delete testcpu;
      }
    }
  }
};


/*
 This function prints out fitness data. The main point is that it
 calculates the average fitness from info from the testCPU + the actual
 merit of the organisms, and assigns zero fitness to those organisms
 that will never reproduce.
 
 The function also determines the maximum fitness genotype, and can
 produce fitness histograms.
 
 Parameters
 datafn (cString)
 Where the fitness data should be written.
 histofn (cString)
 Where the fitness histogram should be written.
 histotestfn (cString)
 Where the fitness histogram as determined exclusively from the test-CPU should be written.
 save_max_f_genotype (bool)
 Whether the genotype with the maximum fitness should be saved into the classmgr.
 print_fitness_histo (bool)
 Determines whether fitness histograms should be written.
 hist_fmax (double)
 The maximum fitness value to be taken into account for the fitness histograms.
 hist_fstep (double)
 The width of the individual bins in the fitness histograms.
 */
class cActionPrintDetailedFitnessData : public cAction
{
private:
  int m_save_max;
  int m_print_fitness_histo;
  double m_hist_fmax;
  double m_hist_fstep;
  cString m_filenames[3];
  
public:
  cActionPrintDetailedFitnessData(cWorld* world, const cString& args, Feedback&)
  : cAction(world, args), m_save_max(0), m_print_fitness_histo(0), m_hist_fmax(1.0), m_hist_fstep(0.1)
  {
    cString largs(args);
    if (largs.GetSize()) m_save_max = largs.PopWord().AsInt();
    if (largs.GetSize()) m_print_fitness_histo = largs.PopWord().AsInt();
    if (largs.GetSize()) m_hist_fmax = largs.PopWord().AsDouble();
    if (largs.GetSize()) m_hist_fstep = largs.PopWord().AsDouble();
    if (!largs.GetSize()) m_filenames[0] = "fitness.dat"; else m_filenames[0] = largs.PopWord();
    if (!largs.GetSize()) m_filenames[1] = "fitness_histos.dat"; else m_filenames[1] = largs.PopWord();
    if (!largs.GetSize()) m_filenames[2] = "fitness_histos_testCPU.dat"; else m_filenames[2] = largs.PopWord();
  }
  
  static const cString GetDescription() { return "Arguments: [int save_max_f_genotype=0] [int print_fitness_histo=0] [double hist_fmax=1] [double hist_fstep=0.1] [string datafn=\"fitness.dat\"] [string histofn=\"fitness_histos.dat\"] [string histotestfn=\"fitness_histos_testCPU.dat\"]"; }
  
  void Process(cAvidaContext& ctx)
  {
    cPopulation& pop = m_world->GetPopulation();
    const int update = m_world->GetStats().GetUpdate();
    const double generation = m_world->GetStats().SumGeneration().Average();
    
    // the histogram variables
    Apto::Array<int> histo;
    Apto::Array<int> histo_testCPU;
    int bins = 0;
    
    if (m_print_fitness_histo) {
      bins = static_cast<int>(m_hist_fmax / m_hist_fstep) + 1;
      histo.Resize(bins, 0);
      histo_testCPU.Resize(bins, 0 );
    }
    
    int n = 0;
    int nhist_tot = 0;
    int nhist_tot_testCPU = 0;
    double fave = 0;
    double fave_testCPU = 0;
    double max_fitness = -1; // we set this to -1, so that even 0 is larger...
    Systematics::GroupPtr max_f_genotype;
    
    cTestCPU* testcpu = m_world->GetHardwareManager().CreateTestCPU(ctx);
    
    for (int i = 0; i < pop.GetSize(); i++) {
      if (pop.GetCell(i).IsOccupied() == false) continue;  // One use organisms.
      
      cOrganism* organism = pop.GetCell(i).GetOrganism();
      Systematics::GroupPtr genotype = organism->SystematicsGroup("genotype");
      
      cCPUTestInfo test_info;
      testcpu->TestGenome(ctx, test_info, Genome(genotype->Properties().Get("genome")));
      // We calculate the fitness based on the current merit,
      // but with the true gestation time. Also, we set the fitness
      // to zero if the creature is not viable.
      const double f = (test_info.IsViable()) ? organism->GetPhenotype().GetMerit().CalcFitness(test_info.GetTestPhenotype().GetGestationTime()) : 0;
      const double f_testCPU = test_info.GetColonyFitness();
      
      // Get the maximum fitness in the population
      // Here, we want to count only organisms that can truly replicate,
      // to avoid complications
      if (f_testCPU > max_fitness && test_info.GetTestPhenotype().CopyTrue()) {
        max_fitness = f_testCPU;
        max_f_genotype = genotype;
      }
      
      fave += f;
      fave_testCPU += f_testCPU;
      n += 1;
      
      
      // histogram
      if (m_print_fitness_histo && f < m_hist_fmax) {
        histo[static_cast<int>(f / m_hist_fstep)] += 1;
        nhist_tot += 1;
      }
      
      if (m_print_fitness_histo && f_testCPU < m_hist_fmax) {
        histo_testCPU[static_cast<int>(f_testCPU / m_hist_fstep)] += 1;
        nhist_tot_testCPU += 1;
      }
    }
    
    
    // determine the name of the maximum fitness genotype
    cString max_f_name;
    if ((bool)Apto::StrAs(max_f_genotype->Properties().Get("threshold")))
      max_f_name = max_f_genotype->Properties().Get("name").StringValue();
    else {
      // we put the current update into the name, so that it becomes unique.
      Genome gen(max_f_genotype->Properties().Get("genome"));
      InstructionSequencePtr seq;
      seq.DynamicCastFrom(gen.Representation());
      max_f_name.Set("%03d-no_name-u%i", seq->GetSize(), update);
    }
    
    Avida::Output::FilePtr df = Avida::Output::File::StaticWithPath(m_world->GetNewWorld(), (const char*)m_filenames[0]);
    df->Write(update, "Update");
    df->Write(generation, "Generation");
    df->Write(fave / static_cast<double>(n), "Average Fitness");
    df->Write(fave_testCPU / static_cast<double>(n), "Average Test Fitness");
    df->Write(n, "Organism Total");
    df->Write(max_fitness, "Maximum Fitness");
    df->Write(max_f_name, "Maxfit genotype name");
    df->Endl();
    
    if (m_save_max) {
      cString filename;
      filename.Set("archive/%s", static_cast<const char*>(max_f_name));
      testcpu->PrintGenome(ctx, Genome(max_f_genotype->Properties().Get("genome")), filename);
    }
    
    delete testcpu;
    
    if (m_print_fitness_histo) {
      Avida::Output::FilePtr hdf = Avida::Output::File::StaticWithPath(m_world->GetNewWorld(), (const char*)m_filenames[1]);
      hdf->Write(update, "Update");
      hdf->Write(generation, "Generation");
      hdf->Write(fave / static_cast<double>(n), "Average Fitness");
      
      // now output the fitness histo
      for (int i = 0; i < histo.GetSize(); i++)
        hdf->WriteAnonymous(static_cast<double>(histo[i]) / static_cast<double>(nhist_tot));
      hdf->Endl();
      
      
      Avida::Output::FilePtr tdf = Avida::Output::File::StaticWithPath(m_world->GetNewWorld(), (const char*)m_filenames[2]);
      tdf->Write(update, "Update");
      tdf->Write(generation, "Generation");
      tdf->Write(fave / static_cast<double>(n), "Average Fitness");
      
      // now output the fitness histo
      for (int i = 0; i < histo_testCPU.GetSize(); i++)
        tdf->WriteAnonymous(static_cast<double>(histo_testCPU[i]) / static_cast<double>(nhist_tot_testCPU));
      tdf->Endl();
    }
  }
};





/*
 @MRR May 2007 [BETA]
 This function prints out fitness data. The main point is that it
 calculates the average fitness from info from the testCPU + the actual
 merit of the organisms, and assigns zero fitness to those organisms
 that will never reproduce.
 
 The function also determines the maximum fitness genotype, and can
 produce fitness histograms.
 
 This version of the DetailedFitnessData prints the information as a log histogram.
 
 THIS FUNCTION CONTAINS STATIC METHODS USED IN OTHER PRINT ACTION CLASSES.
 MOVEMENT OF THIS FUNCTION TO A LOWER POINT IN THE FILE MAY CAUSE CONFLICTS.
 
 Parameters:
 filename   (cString)     Where the fitness histogram should be written.
 fit_mode   (cString)     Either {Current, Actual, TestCPU}, where
 Current is the current value in the grid.  [Default]
 Actual uses the current merit, but the true gestation time.
 TestCPU determined.
 hist_fmin  (double)      The minimum fitness value for the fitness histogram.  [Default: -3]
 hist_fmax  (double)      The maximum fitness value for the fitness histogram.  [Default: 12]
 hist_fstep (double)      The width of the individual bins in the histogram.    [Default: 0.5]
 */
class cActionPrintLogFitnessHistogram : public cAction
{
private:
  
  double m_hist_fmin;
  double m_hist_fstep;
  double m_hist_fmax;
  cString m_mode;
  cString m_filename;
  
public:
  cActionPrintLogFitnessHistogram(cWorld* world, const cString& args, Feedback&)
  : cAction(world, args)
  {
    cString largs(args);
    m_filename   = (largs.GetSize()) ? largs.PopWord()           : "fitness_log_hist.dat";
    m_mode       = (largs.GetSize()) ? largs.PopWord().ToUpper() : "CURRENT";
    m_hist_fmin  = (largs.GetSize()) ? largs.PopWord().AsDouble(): -3.0;
    m_hist_fstep = (largs.GetSize()) ? largs.PopWord().AsDouble(): 0.5;
    m_hist_fmax  = (largs.GetSize()) ? largs.PopWord().AsDouble(): 12;
  }
  
  static const cString GetDescription() { return  "Parameters: <filename> <mode> <min> <step> <max>";}
  
  //Given a min:step:max and bin number, return a string reprsenting the range of fitness values.
  //This function may be called from other classes.
  static cString GetHistogramBinLabel(int k, double min, double step, double max)
  {
    int num_bins = static_cast<int>(ceil( (max - min) / step)) + 3;
    cString retval;
    
    if (k == 0)
      retval = "Inviable";
    else if (k == 1)
      retval = cString("[<") + cStringUtil::Convert(min) + ", " + cStringUtil::Convert(min) + cString(")");
    else if (k < num_bins - 1)
      retval = cString("(") + cStringUtil::Convert(min+step*(k-2))
      + cString(", ") + cStringUtil::Convert(min+step*(k-1)) +
      + cString("]");
    else
      retval = cString("[>") + cStringUtil::Convert(max) + cString("]");
    return retval;
  }
  
  
  //This function may get called by outside classes to generate a histogram of log10 fitnesses;
  //max may be updated by this function if the range is not evenly divisible by the step
  static Apto::Array<int> MakeHistogram(const Apto::Array<cOrganism*>& orgs, const Apto::Array<Systematics::GroupPtr>& gens,
                                   double min, double step, double& max, const cString& mode, cWorld* world,
                                   cAvidaContext& ctx)
  {
    //Set up histogram; extra columns prepended (non-viable, < m_hist_fmin) and appended ( > f_hist_fmax)
    //If the bin size is not a multiple of the step size, the last bin is expanded to make it a multiple.
    //All bins are [min, max)
    Apto::Array<int> histogram;
    int num_bins = static_cast<int>(ceil( (max - min) / step)) + 3;
    max  = min + (num_bins - 3) * step;
    histogram.Resize(num_bins, 0);
    cTestCPU* testcpu = world->GetHardwareManager().CreateTestCPU(ctx);
    
    
    // We calculate the fitness based on the current merit,
    // but with the true gestation time. Also, we set the fitness
    // to zero if the creature is not viable.
    for (int i = 0; i < gens.GetSize(); i++) {
      cCPUTestInfo test_info;
      double fitness = 0.0;
      if (mode == "TEST_CPU" || mode == "ACTUAL"){
        test_info.UseManualInputs(orgs[i]->GetOrgInterface().GetInputs());
        testcpu->TestGenome(ctx, test_info, Genome(gens[i]->Properties().Get("genome")));
      }
      
      if (mode == "TEST_CPU"){
        fitness = test_info.GetColonyFitness();
      }
      else if (mode == "CURRENT"){
        fitness = orgs[i]->GetPhenotype().GetFitness();
      }
      else if (mode == "ACTUAL"){
        fitness = (test_info.IsViable()) ?
        orgs[i]->GetPhenotype().GetMerit().CalcFitness(test_info.GetTestPhenotype().GetGestationTime()) : 0.0;
      } else {
        ctx.Driver().Feedback().Error("PrintLogFitnessHistogram::MakeHistogram: Invalid fitness mode requested.");
        ctx.Driver().Abort(Avida::INVALID_CONFIG);
      }
      //Update the histogram
      int update_bin = (fitness == 0) ? 0 :
      static_cast<int>((log10(fitness) - min) / step);
      
      // Bin 0   Inviable
      //     1   Below Range
      //     2   [min, min+step)
      // #bin-1  [max-step, max)
      // num_bin Above Range
      
      if (fitness == 0)
        update_bin = 0;
      else if (log10(fitness) < min)
        update_bin = 1;
      else if (log10(fitness) > max)
        update_bin = num_bins - 1;
      else
        update_bin = static_cast<int>(log10(fitness) - min / step) + 2;
      
      histogram[update_bin]++;
    }
    delete testcpu;
    return histogram;
  }
  
  void Process(cAvidaContext& ctx)
  {
    //Verify input parameters
    if ( (m_mode != "ACTUAL" && m_mode != "CURRENT" && m_mode != "TESTCPU") ||
        m_hist_fmin > m_hist_fmax)
    {
      cerr << "cActionPrintFitnessHistogram: Please check arguments.  Abort.\n";
      cerr << "Parameters: " << m_filename << ", " << m_mode << ", " << m_hist_fmin << ":" << m_hist_fstep << ":" << m_hist_fmax << endl;
      return;
    }
    cerr << "Parameters: " << m_filename << ", " << m_mode << ", " << m_hist_fmin << ":" << m_hist_fstep << ":" << m_hist_fmax << endl;
    
    
    //Gather data objects
    cPopulation& pop        = m_world->GetPopulation();
    const int    update     = m_world->GetStats().GetUpdate();
    const double generation = m_world->GetStats().SumGeneration().Average();
    Apto::Array<cOrganism*> orgs;
    Apto::Array<Systematics::GroupPtr> gens;
    
    for (int i = 0; i < pop.GetSize(); i++)
    {
      if (pop.GetCell(i).IsOccupied() == false) continue;  //Skip unoccupied cells
      cOrganism* organism = pop.GetCell(i).GetOrganism();
      Systematics::GroupPtr genotype = organism->SystematicsGroup("genotype");
      orgs.Push(organism);
      gens.Push(genotype);
    }
    
    Apto::Array<int> histogram = MakeHistogram(orgs, gens, m_hist_fmin, m_hist_fstep, m_hist_fmax, m_mode, m_world, ctx);
    
    
    //Output histogram
    Avida::Output::FilePtr hdf = Avida::Output::File::StaticWithPath(m_world->GetNewWorld(), (const char*)m_filename);
    hdf->Write(update, "Update");
    hdf->Write(generation, "Generation");
    
    for (int k = 0; k < histogram.GetSize(); k++)
      hdf->Write(histogram[k], GetHistogramBinLabel(k, m_hist_fmin, m_hist_fstep, m_hist_fmax));
    hdf->Endl();
  }
};



/*
 @MRR May 2007  [BETA]
 
 This function requires Avida be in run mode.
 
 This function will print histograms of the relative fitness of
 organisms as compared to the parent.
 
 STATIC METHODS IN THIS CLASS ARE CALLED BY OTHER ACTIONS.
 MOVING THIS CLASS MAY BREAK DEPENDENCIES.
 
 Parameters:
 filename  (cString)        Name of the output file
 fit_mode (cString)        Either {Current, Actual, TestCPU}, where
 Current is the current value in the grid. [Default]
 Actual uses the current merit, but the true gestation time.
 that have reproduced.
 TestCPU determined.
 hist_fmin  (double)      The minimum fitness value for the fitness histogram.  [Default: 0.50]
 hist_fmax  (double)      The maximum fitness value for the fitness histogram.  [Default: 0.02]
 hist_fstep (double)      The width of the individual bins in the histogram.    [Default: 1.50]
 
 The file will be formatted:
 <update>  [ <min, min, min+step, ..., max-step, max, >max], each bin [min,max)
 */
class cActionPrintRelativeFitnessHistogram : public cAction
{
private:
  double m_hist_fmin;
  double m_hist_fstep;
  double m_hist_fmax;
  cString m_mode;
  cString m_filename;
  
public:
  cActionPrintRelativeFitnessHistogram(cWorld* world, const cString& args, Feedback&) : cAction(world, args)
  {
    cString largs(args);
    m_filename   = (largs.GetSize()) ? largs.PopWord()           : "rel_fitness_hist.dat";
    m_mode       = (largs.GetSize()) ? largs.PopWord().ToUpper() : "CURRENT";
    m_hist_fmin  = (largs.GetSize()) ? largs.PopWord().AsDouble(): 0;
    m_hist_fstep = (largs.GetSize()) ? largs.PopWord().AsDouble(): 0.1;
    m_hist_fmax  = (largs.GetSize()) ? largs.PopWord().AsDouble(): 2;
  }
  
  static const cString GetDescription() { return "Arguments: [filename] [fit_mode] [hist_min] [hist_step] [hist_max]"; }
  
  
  static cString GetHistogramBinLabel(int k, double min, double step, double max)
  {
    int num_bins = static_cast<int>(ceil( (max - min) / step)) + 2;
    cString retval;
    
    if (k == 0)
      retval = "Inviable";
    else if (k == 1)
      retval = cString("[<") + cStringUtil::Convert(min) + ", " + cStringUtil::Convert(min) + cString(")");
    else if (k < num_bins - 1)
      retval = cString("(") + cStringUtil::Convert(min+step*(k-2))
      + cString(", ") + cStringUtil::Convert(min+step*(k-1)) +
      + cString("]");
    else
      retval = cString("[>") + cStringUtil::Convert(max) + cString("]");
    
    return retval;
  }
  
  static Apto::Array<int> MakeHistogram(const Apto::Array<cOrganism*>& orgs, const Apto::Array<Systematics::GroupPtr>& gens,
                                   double min, double step, double& max, const cString& mode, cWorld* world,
                                   cAvidaContext& ctx)
  {
    //Set up histogram; extra columns prepended (non-viable, < m_hist_fmin) and appended ( > f_hist_fmax)
    //If the bin size is not a multiple of the step size, the last bin is expanded to make it a multiple.
    //All bins are [min, max)
    Apto::Array<int> histogram;
    int num_bins = static_cast<int>(ceil( (max - min) / step)) + 3;
    max  = min + (num_bins - 3) * step;
    histogram.Resize(num_bins, 0);
    cTestCPU* testcpu = world->GetHardwareManager().CreateTestCPU(ctx);
    
    
    // We calculate the fitness based on the current merit,
    // but with the true gestation time. Also, we set the fitness
    // to zero if the creature is not viable.
    for (int i = 0; i < gens.GetSize(); i++){
      cCPUTestInfo test_info;
      double fitness = 0.0;
      double parent_fitness = 1.0;
      if (gens[i]->Properties().Get("parents").StringValue() != "") {
        cStringList parents((const char*)gens[i]->Properties().Get("parents").StringValue(), ',');
        
        Systematics::GroupPtr pbg = Systematics::Manager::Of(world->GetNewWorld())->ArbiterForRole("genotype")->Group(parents.Pop().AsInt());
        parent_fitness = Apto::StrAs(pbg->Properties().Get("fitness"));
      }
      
      if (mode == "TEST_CPU" || mode == "ACTUAL"){
        test_info.UseManualInputs( orgs[i]->GetOrgInterface().GetInputs() );
        testcpu->TestGenome(ctx, test_info, Genome(gens[i]->Properties().Get("genome")));
      }
      
      if (mode == "TEST_CPU"){
        fitness = test_info.GetColonyFitness();
      }
      else if (mode == "CURRENT"){
        fitness = orgs[i]->GetPhenotype().GetFitness();
      }
      else if (mode == "ACTUAL"){
        fitness = (test_info.IsViable()) ?
        orgs[i]->GetPhenotype().GetMerit().CalcFitness(test_info.GetTestPhenotype().GetGestationTime()) : 0.0;
      } else {
        ctx.Driver().Feedback().Error("MakeHistogram: Invalid fitness mode requested.");
        ctx.Driver().Abort(Avida::INVALID_CONFIG);
      }
      
      //Update the histogram
      if (parent_fitness <= 0.0) {
        ctx.Driver().Feedback().Error(cString("PrintRelativeFitness::MakeHistogram reports a parent fitness is zero.") + gens[i]->Properties().Get("parents").StringValue());
        ctx.Driver().Abort(Avida::INTERNAL_ERROR);
      }
      
      int update_bin = 0;
      double rfitness = fitness/parent_fitness;
      
      if (fitness == 0.0)
        update_bin = 0;
      else if (rfitness < min)
        update_bin = 1;
      else if (rfitness > max)
        update_bin = num_bins - 1;
      else
        update_bin = static_cast<int>( ((fitness/parent_fitness) - min) / step) + 2;
      
      histogram[update_bin]++;
    }
    delete testcpu;
    return histogram;
  }
  
  
  
  void Process(cAvidaContext& ctx)
  {
    //Gather data objects
    cPopulation& pop        = m_world->GetPopulation();
    const int    update     = m_world->GetStats().GetUpdate();
    const double generation = m_world->GetStats().SumGeneration().Average();
    Apto::Array<cOrganism*> orgs;
    Apto::Array<Systematics::GroupPtr> gens;
    
    for (int i = 0; i < pop.GetSize(); i++)
    {
      if (pop.GetCell(i).IsOccupied() == false) continue;  //Skip unoccupied cells
      cOrganism* organism = pop.GetCell(i).GetOrganism();
      Systematics::GroupPtr genotype = organism->SystematicsGroup("genotype");
      orgs.Push(organism);
      gens.Push(genotype);
    }
    
    Apto::Array<int> histogram = MakeHistogram(orgs, gens, m_hist_fmin, m_hist_fstep, m_hist_fmax, m_mode, m_world, ctx);
    
    
    //Output histogram
    Avida::Output::FilePtr hdf = Avida::Output::File::StaticWithPath(m_world->GetNewWorld(), (const char*)m_filename);
    hdf->Write(update, "Update");
    hdf->Write(generation, "Generation");
    
    for (int k = 0; k < histogram.GetSize(); k++)
      hdf->Write(histogram[k], GetHistogramBinLabel(k, m_hist_fmin, m_hist_fstep, m_hist_fmax));
    hdf->Endl();
  }
};









/*
 This function goes through all genotypes currently present in the soup,
 and writes into an output file the average Hamming distance between the
 creatures in the population and a given reference genome.
 
 Parameters
 ref_creature_file (cString)
 Filename for the reference genome
 fname (cString)
 Name of file to create, defaults to 'genetic_distance.dat'
 */
class cActionPrintGeneticDistanceData : public cAction
{
private:
  Genome m_reference;
  InstructionSequencePtr m_r_seq;
  cString m_filename;
  
public:
  cActionPrintGeneticDistanceData(cWorld* world, const cString& args, Feedback& feedback)
  : cAction(world, args), m_filename("genetic_distance.dat")
  {
    cString creature_file;
    cString largs(args);
    
    // Load the genome of the reference creature
    creature_file = largs.PopWord();
    GenomePtr genome(Util::LoadGenomeDetailFile(creature_file, m_world->GetWorkingDir(), world->GetHardwareManager(), feedback));
    m_reference = *genome;
    m_r_seq.DynamicCastFrom(m_reference.Representation());
    
    if (largs.GetSize()) m_filename = largs.PopWord();
  }
  
  static const cString GetDescription() { return "Arguments: <string ref_creature_file> [string fname='genetic_distance.dat']"; }
  
  void Process(cAvidaContext&)
  {
    double hamming_m1 = 0;
    double hamming_m2 = 0;
    int count = 0;
    int dom_dist = 0;
    
    // get the info for the dominant genotype
    Systematics::ManagerPtr classmgr = Systematics::Manager::Of(m_world->GetNewWorld());
    Systematics::Arbiter::IteratorPtr it = classmgr->ArbiterForRole("genotype")->Begin();
    it->Next();
    Genome best_genome(it->Get()->Properties().Get("genome"));
    InstructionSequencePtr best_seq;
    best_seq.DynamicCastFrom(best_genome.Representation());
    dom_dist = InstructionSequence::FindHammingDistance(*m_r_seq, *best_seq);
    hamming_m1 += dom_dist;
    hamming_m2 += dom_dist*dom_dist;
    count += it->Get()->NumUnits();
    // now cycle over the remaining genotypes
    while ((it->Next())) {
      Genome cur_gen(it->Get()->Properties().Get("genome"));
      InstructionSequencePtr cur_seq;
      cur_seq.DynamicCastFrom(cur_gen.Representation());
      int dist = InstructionSequence::FindHammingDistance(*m_r_seq, *cur_seq);
      hamming_m1 += dist;
      hamming_m2 += dist*dist;
      count += it->Get()->NumUnits();
    }
    
    hamming_m1 /= static_cast<double>(count);
    hamming_m2 /= static_cast<double>(count);
    
    double hamming_best = InstructionSequence::FindHammingDistance(*m_r_seq, *best_seq);
    
    Avida::Output::FilePtr df = Avida::Output::File::StaticWithPath(m_world->GetNewWorld(), (const char*)m_filename);
    df->Write(m_world->GetStats().GetUpdate(), "Update");
    df->Write(hamming_m1, "Average Hamming Distance");
    df->Write(sqrt((hamming_m2 - hamming_m1*hamming_m1) / static_cast<double>(count)), "Standard Error");
    df->Write(hamming_best, "Best Genotype Hamming Distance");
    df->Endl();
  }
};

/*
 This action goes through all genotypes currently present in the population,
 and writes into an output file the names of the genotypes, the fitness as
 determined in the test cpu, and the genetic distance to a reference genome.
 */
class cActionPrintPopulationDistanceData : public cAction
{
private:
  cString m_creature;
  cString m_filename;
  int m_save_genotypes;
  
public:
  cActionPrintPopulationDistanceData(cWorld* world, const cString& args, Feedback&)
  : cAction(world, args), m_filename(""), m_save_genotypes(0)
  {
    cString largs(args);
    if (largs.GetSize()) m_creature = largs.PopWord();
    if (largs.GetSize()) m_filename = largs.PopWord();
    if (largs.GetSize()) m_save_genotypes = largs.PopWord().AsInt();
  }
  
  static const cString GetDescription() { return "Arguments: <string creature> [string fname=\"\"] [int save_genotypes=0]"; }
  
  void Process(cAvidaContext& ctx)
  {
    cString filename(m_filename);
    if (filename == "") filename.Set("pop_distance-%d.dat", m_world->GetStats().GetUpdate());
    Avida::Output::FilePtr df = Avida::Output::File::CreateWithPath(m_world->GetNewWorld(), (const char*)m_filename);
    
    double sum_fitness = 0;
    int sum_num_organisms = 0;
    
    // load the reference genome
    GenomePtr reference_genome;
    cUserFeedback feedback;
    reference_genome = Util::LoadGenomeDetailFile(m_creature, m_world->GetWorkingDir(), m_world->GetHardwareManager(), feedback);
    for (int i = 0; i < feedback.GetNumMessages(); i++) {
      switch (feedback.GetMessageType(i)) {
        case cUserFeedback::UF_ERROR:    cerr << "error: "; break;
        case cUserFeedback::UF_WARNING:  cerr << "warning: "; break;
        default: break;
      };
      cerr << feedback.GetMessage(i) << endl;
    }
    if (!reference_genome) return;
    
    InstructionSequencePtr r_seq;
    r_seq.DynamicCastFrom(reference_genome->Representation());
    
    // cycle over all genotypes
    Systematics::ManagerPtr classmgr = Systematics::Manager::Of(m_world->GetNewWorld());
    Systematics::Arbiter::IteratorPtr it = classmgr->ArbiterForRole("genotype")->Begin();
    while ((it->Next())) {
      Systematics::GroupPtr bg = it->Get();
      const Genome genome(bg->Properties().Get("genome"));
      ConstInstructionSequencePtr seq;
      seq.DynamicCastFrom(genome.Representation());
      const int num_orgs = bg->NumUnits();
      
      // now output
      
      sum_fitness += (double)Apto::StrAs(bg->Properties().Get("fitness")) * num_orgs;
      sum_num_organisms += num_orgs;
      
      df->Write(bg->Properties().Get("name").StringValue(), "Genotype Name");
      df->Write((double)Apto::StrAs(bg->Properties().Get("fitness")), "Fitness");
      df->Write(num_orgs, "Abundance");
      df->Write(InstructionSequence::FindHammingDistance(*r_seq, *seq), "Hamming distance to reference");
      df->Write(InstructionSequence::FindEditDistance(*r_seq, *seq), "Levenstein distance to reference");
      df->Write(genome.AsString(), "Genome");
      
      // save into archive
      if (m_save_genotypes) {
        cTestCPU* testcpu = m_world->GetHardwareManager().CreateTestCPU(ctx);
        testcpu->PrintGenome(ctx, genome, cStringUtil::Stringf("archive/%s.org", (const char*)(bg->Properties().Get("name").StringValue())));
        delete testcpu;
      }
      
      df->Endl();
    }
    df->WriteRaw(cStringUtil::Stringf("# ave fitness from Test CPU's: %d\n", sum_fitness / sum_num_organisms));
    
  }
};


class cActionTestDominant : public cAction
{
private:
  cString m_filename;
  
public:
  cActionTestDominant(cWorld* world, const cString& args, Feedback&) : cAction(world, args), m_filename("dom-test.dat")
  {
    cString largs(args);
    if (largs.GetSize()) m_filename = largs.PopWord();
  }
  static const cString GetDescription() { return "Arguments: [string fname='dom-test.dat']"; }
  void Process(cAvidaContext& ctx)
  {
    Systematics::ManagerPtr classmgr = Systematics::Manager::Of(m_world->GetNewWorld());
    Systematics::Arbiter::IteratorPtr it = classmgr->ArbiterForRole("genotype")->Begin();
    Systematics::GroupPtr bg = it->Next();
    Genome genome(bg->Properties().Get("genome"));
    InstructionSequencePtr seq;
    seq.DynamicCastFrom(genome.Representation());
    
    cTestCPU* testcpu = m_world->GetHardwareManager().CreateTestCPU(ctx);
    cCPUTestInfo test_info;
    testcpu->TestGenome(ctx, test_info, genome);
    delete testcpu;
    
    cPhenotype& colony_phenotype = test_info.GetColonyOrganism()->GetPhenotype();
    
    Avida::Output::FilePtr df = Avida::Output::File::StaticWithPath(m_world->GetNewWorld(), (const char*)m_filename);
    df->Write(m_world->GetStats().GetUpdate(), "Update");
    df->Write(colony_phenotype.GetMerit().GetDouble(), "Merit");
    df->Write(colony_phenotype.GetGestationTime(), "Gestation Time");
    df->Write(colony_phenotype.GetFitness(), "Fitness");
    df->Write(1.0 / (0.1 + colony_phenotype.GetGestationTime()), "Reproduction Rate");
    df->Write(seq->GetSize(), "Genome Length");
    df->Write(colony_phenotype.GetCopiedSize(), "Copied Size");
    df->Write(colony_phenotype.GetExecutedSize(), "Executed Size");
    df->Endl();
  }
};


class cActionPrintTaskSnapshot : public cAction
{
private:
  cString m_filename;
  
public:
  cActionPrintTaskSnapshot(cWorld* world, const cString& args, Feedback&) : cAction(world, args), m_filename("")
  {
    cString largs(args);
    if (largs.GetSize()) m_filename = largs.PopWord();
  }
  static const cString GetDescription() { return "Arguments: [string fname='']"; }
  void Process(cAvidaContext& ctx)
  {
    cString filename(m_filename);
    if (filename == "") filename.Set("tasks_%d.dat", m_world->GetStats().GetUpdate());
    Avida::Output::FilePtr df = Avida::Output::File::CreateWithPath(m_world->GetNewWorld(), (const char*)m_filename);
    
    cPopulation& pop = m_world->GetPopulation();
    cTestCPU* testcpu = m_world->GetHardwareManager().CreateTestCPU(ctx);
    
    for (int i = 0; i < pop.GetSize(); i++) {
      if (pop.GetCell(i).IsOccupied() == false) continue;
      cOrganism* organism = pop.GetCell(i).GetOrganism();
      
      // create a test-cpu for the current creature
      cCPUTestInfo test_info;
      testcpu->TestGenome(ctx, test_info, organism->GetGenome());
      cPhenotype& test_phenotype = test_info.GetTestPhenotype();
      cPhenotype& phenotype = organism->GetPhenotype();
      
      int num_tasks = m_world->GetEnvironment().GetNumTasks();
      int sum_tasks_all = 0;
      int sum_tasks_rewarded = 0;
      int divide_sum_tasks_all = 0;
      int divide_sum_tasks_rewarded = 0;
      int parent_sum_tasks_all = 0;
      int parent_sum_tasks_rewarded = 0;
      
      for (int j = 0; j < num_tasks; j++) {
        // get the number of bonuses for this task
        int bonuses = 1; //phenotype.GetTaskLib().GetTaskNumBonus(j);
        int task_count = ( phenotype.GetCurTaskCount()[j] == 0 ) ? 0 : 1;
        int divide_tasks_count = (test_phenotype.GetLastTaskCount()[j] == 0)?0:1;
        int parent_task_count = (phenotype.GetLastTaskCount()[j] == 0) ? 0 : 1;
        
        // If only one bonus, this task is not rewarded, as last bonus is + 0.
        if (bonuses > 1) {
          sum_tasks_rewarded += task_count;
          divide_sum_tasks_rewarded += divide_tasks_count;
          parent_sum_tasks_rewarded += parent_task_count;
        }
        sum_tasks_all += task_count;
        divide_sum_tasks_all += divide_tasks_count;
        parent_sum_tasks_all += parent_task_count;
      }
      
      df->Write(i, "Cell Number");
      df->Write(sum_tasks_rewarded, "Number of Tasks Rewarded");
      df->Write(sum_tasks_all, "Total Number of Tasks Done");
      df->Write(divide_sum_tasks_rewarded, "Number of Rewarded Tasks on Divide");
      df->Write(divide_sum_tasks_all, "Number of Total Tasks on Divide");
      df->Write(parent_sum_tasks_rewarded, "Parent Number of Tasks Rewared");
      df->Write(parent_sum_tasks_all, "Parent Total Number of Tasks Done");
      df->Write(test_info.GetColonyFitness(), "Genotype Fitness");
      df->Write(organism->SystematicsGroup("genotype")->ID(), "Genotype ID");
      df->Endl();
    }
    
    delete testcpu;
  }
};

class cActionPrintAveNumTasks : public cAction
{
private:
  cString m_filename;
  
public:
  cActionPrintAveNumTasks(cWorld* world, const cString& args, Feedback&) : cAction(world, args), m_filename("ave_num_tasks.dat")
  {
    cString largs(args);
    if (largs.GetSize()) m_filename = largs.PopWord();
  }
  static const cString GetDescription() { return "Arguments: [string fname='']"; }
  void Process(cAvidaContext&)
  {
    Avida::Output::FilePtr df = Avida::Output::File::StaticWithPath(m_world->GetNewWorld(), (const char*)m_filename);
    cPopulation& pop = m_world->GetPopulation();
    
    int ave_tot_tasks = 0;
    int num_task_orgs = 0;
    for (int i = 0; i < pop.GetSize(); i++) {
      if (pop.GetCell(i).IsOccupied() == false) continue;
      
      cPhenotype& phenotype = pop.GetCell(i).GetOrganism()->GetPhenotype();
      int num_tasks = m_world->GetEnvironment().GetNumTasks();
      
      int sum_tasks = 0;
      for (int j = 0; j < num_tasks; j++)
        sum_tasks += ( phenotype.GetLastTaskCount()[j] == 0 ) ? 0 : 1;
      if (sum_tasks>0) {
        ave_tot_tasks += sum_tasks;
        num_task_orgs++;
      }
    }
    double pop_ave = -1;
    if (num_task_orgs>0)
      pop_ave = ave_tot_tasks/double(num_task_orgs);
    
    df->WriteComment("Avida num tasks data");
    df->WriteTimeStamp();
    df->WriteComment("First column gives the current update, 2nd column gives the average number of tasks performed");
    df->WriteComment("by each organism in the current population that performs at least one task ");
    
    df->Write(m_world->GetStats().GetUpdate(), "Update");
    df->Write(pop_ave, "Ave num tasks done by single org that is doing at least one task");
    df->Endl();
  }
};


class cActionPrintViableTasksData : public cAction
{
private:
  cString m_filename;
  
public:
  cActionPrintViableTasksData(cWorld* world, const cString& args, Feedback&) : cAction(world, args), m_filename("viable_tasks.dat")
  {
    cString largs(args);
    if (largs.GetSize()) m_filename = largs.PopWord();
  }
  static const cString GetDescription() { return "Arguments: [string fname='viable_tasks.dat']"; }
  void Process(cAvidaContext& ctx)
  {
    Avida::Output::FilePtr df = Avida::Output::File::StaticWithPath(m_world->GetNewWorld(), (const char*)m_filename);
    cPopulation& pop = m_world->GetPopulation();
    const int num_tasks = m_world->GetEnvironment().GetNumTasks();
    
    Apto::Array<int> tasks(num_tasks);
    tasks.SetAll(0);
    
    for (int i = 0; i < pop.GetSize(); i++) {
      if (!pop.GetCell(i).IsOccupied()) continue;
      if (pop.GetCell(i).GetOrganism()->GetTestFitness(ctx) > 0.0) {
        cPhenotype& phenotype = pop.GetCell(i).GetOrganism()->GetPhenotype();
        for (int j = 0; j < num_tasks; j++) if (phenotype.GetCurTaskCount()[j] > 0) tasks[j]++;
      }
    }
    
    df->WriteComment("Avida viable tasks data");
    df->WriteTimeStamp();
    df->WriteComment("First column gives the current update, next columns give the number");
    df->WriteComment("of organisms that have the particular task as a component of their merit");
    
    df->Write(m_world->GetStats().GetUpdate(), "Update");
    for(int i = 0; i < tasks.GetSize(); i++) {
      df->WriteAnonymous(tasks[i]);
    }
    df->Endl();
  }
};





class cActionDumpFitnessGrid : public cAction
{
private:
  cString m_filename;
  
public:
  cActionDumpFitnessGrid(cWorld* world, const cString& args, Feedback&) : cAction(world, args), m_filename("")
  {
    cString largs(args);
    if (largs.GetSize()) m_filename = largs.PopWord();
  }
  static const cString GetDescription() { return "Arguments: [string fname='']"; }
  void Process(cAvidaContext&)
  {
    cString filename(m_filename);
    if (filename == "") filename.Set("grid_fitness-%d.dat", m_world->GetStats().GetUpdate());
    Avida::Output::FilePtr df = Avida::Output::File::CreateWithPath(m_world->GetNewWorld(), (const char*)filename);
    ofstream& fp = df->OFStream();
    
    for (int i = 0; i < m_world->GetPopulation().GetWorldX(); i++) {
      for (int j = 0; j < m_world->GetPopulation().GetWorldY(); j++) {
        cPopulationCell& cell = m_world->GetPopulation().GetCell(j * m_world->GetPopulation().GetWorldX() + i);
        double fitness = (cell.IsOccupied()) ? cell.GetOrganism()->GetPhenotype().GetFitness() : 0.0;
        fp << fitness << " ";
      }
      fp << endl;
    }
  }
};


class cActionDumpClassificationIDGrid : public cAction
{
private:
  cString m_filename;
  cString m_role;
  
public:
  cActionDumpClassificationIDGrid(cWorld* world, const cString& args, Feedback&) : cAction(world, args), m_filename(""), m_role("genotype")
  {
    cString largs(args);
    if (largs.GetSize()) m_filename = largs.PopWord();
    if (largs.GetSize()) m_role = largs.PopWord();
  }
  static const cString GetDescription() { return "Arguments: [string fname_prefix='']"; }
  void Process(cAvidaContext&)
  {
    cString filename(m_filename);
    if (filename == "") filename = "grid_class_id";
    filename.Set("%s-%d.dat", (const char*)filename, m_world->GetStats().GetUpdate());
    Avida::Output::FilePtr df = Avida::Output::File::CreateWithPath(m_world->GetNewWorld(), (const char*)filename);
    ofstream& fp = df->OFStream();
    
    for (int j = 0; j < m_world->GetPopulation().GetWorldY(); j++) {
      for (int i = 0; i < m_world->GetPopulation().GetWorldX(); i++) {
        cPopulationCell& cell = m_world->GetPopulation().GetCell(j * m_world->GetPopulation().GetWorldX() + i);
        int id = (cell.IsOccupied() && cell.GetOrganism()->SystematicsGroup((const char*)m_role)) ? cell.GetOrganism()->SystematicsGroup((const char*)m_role)->ID() : -1;
        fp << id << " ";
      }
      fp << endl;
    }
  }
};

class cActionDumpGenotypeColorGrid : public cAction
{
private:
  int m_num_colors;
  int m_threshold;
  cString m_filename;
  Apto::Array<int> m_genotype_chart;
  
public:
  cActionDumpGenotypeColorGrid(cWorld* world, const cString& args, Feedback&)
  : cAction(world, args), m_num_colors(12), m_threshold(10), m_filename(""), m_genotype_chart(0)
  {
    cString largs(args);
    if (largs.GetSize()) m_num_colors = largs.PopWord().AsInt();
    if (largs.GetSize()) m_threshold = largs.PopWord().AsInt();
    if (largs.GetSize()) m_filename = largs.PopWord();
    
    m_genotype_chart.Resize(m_num_colors, 0);
  }
  
  static const cString GetDescription() { return "Arguments: [int num_colors=12] [string fname='']"; }
  
  void Process(cAvidaContext&)
  {
    // Update current entries in the color chart
    for (int i = 0; i < m_num_colors; i++) {
      if (m_genotype_chart[i] && FindPos(m_genotype_chart[i]) < 0) m_genotype_chart[i] = 0;
    }
    
    // Add new entries where possible
    Systematics::ManagerPtr classmgr = Systematics::Manager::Of(m_world->GetNewWorld());
    Systematics::Arbiter::IteratorPtr it = classmgr->ArbiterForRole("genotype")->Begin();
    for (int i = 0; (it->Next()) && i < m_threshold; i++) {
      if (!isInChart(it->Get()->ID())) {
        // Add to the genotype chart
        for (int j = 0; j < m_num_colors; j++) {
          if (m_genotype_chart[j] == 0) {
            m_genotype_chart[j] = it->Get()->ID();
            break;
          }
        }
      }
    }
    
    cString filename(m_filename);
    if (filename == "") filename.Set("grid_genotype_color-%d.dat", m_world->GetStats().GetUpdate());
    Avida::Output::FilePtr df = Avida::Output::File::CreateWithPath(m_world->GetNewWorld(), (const char*)filename);
    ofstream& fp = df->OFStream();
    
    for (int j = 0; j < m_world->GetPopulation().GetWorldY(); j++) {
      for (int i = 0; i < m_world->GetPopulation().GetWorldX(); i++) {
        cPopulationCell& cell = m_world->GetPopulation().GetCell(j * m_world->GetPopulation().GetWorldX() + i);
        Systematics::GroupPtr bg = (cell.IsOccupied()) ? cell.GetOrganism()->SystematicsGroup("genotype") : Systematics::GroupPtr(NULL);
        if (bg) {
          int color = 0;
          for (; color < m_num_colors; color++) if (m_genotype_chart[color] == bg->ID()) break;
          if (color == m_num_colors && (bool)Apto::StrAs(bg->Properties().Get("threshold"))) color++;
          fp << color << " ";
        } else {
          fp << "-1 ";
        }
      }
      fp << endl;
    }
  }
  
private:
  int FindPos(int gid)
  {
    Systematics::ManagerPtr classmgr = Systematics::Manager::Of(m_world->GetNewWorld());
    Systematics::Arbiter::IteratorPtr it = classmgr->ArbiterForRole("genotype")->Begin();
    int i = 0;
    while ((it->Next()) && i < m_num_colors) {
      if (gid == it->Get()->ID()) return i;
      i++;
    }
    
    return -1;
  }
  
  inline bool isInChart(int gid)
  {
    for (int i = 0; i < m_num_colors; i++) {
      if (m_genotype_chart[i] == gid) return true;
    }
    return false;
  }
};


class cActionDumpPhenotypeIDGrid : public cAction
{
private:
  cString m_filename;
  
public:
  cActionDumpPhenotypeIDGrid(cWorld* world, const cString& args, Feedback&) : cAction(world, args), m_filename("")
  {
    cString largs(args);
    if (largs.GetSize()) m_filename = largs.PopWord();
  }
  static const cString GetDescription() { return "Arguments: [string fname='']"; }
  void Process(cAvidaContext&)
  {
    cString filename(m_filename);
    if (filename == "") filename.Set("grid_phenotype_id.%d.dat", m_world->GetStats().GetUpdate());
    Avida::Output::FilePtr df = Avida::Output::File::CreateWithPath(m_world->GetNewWorld(), (const char*)filename);
    ofstream& fp = df->OFStream();
    
    for (int j = 0; j < m_world->GetPopulation().GetWorldY(); j++) {
      for (int i = 0; i < m_world->GetPopulation().GetWorldX(); i++) {
        cPopulationCell& cell = m_world->GetPopulation().GetCell(j * m_world->GetPopulation().GetWorldX() + i);
        int id = (cell.IsOccupied()) ? cell.GetOrganism()->GetPhenotype().CalcID() : -1;
        fp << id << " ";
      }
      fp << endl;
    }
  }
};

class cActionDumpIDGrid : public cAction
{
private:
  cString m_filename;
  
public:
  cActionDumpIDGrid(cWorld* world, const cString& args, Feedback&) : cAction(world, args), m_filename("")
  {
    cString largs(args);
    if (largs.GetSize()) m_filename = largs.PopWord();  
  }
  static const cString GetDescription() { return "Arguments: [string fname='']"; }
  void Process(cAvidaContext&)
  {
    cString filename(m_filename);
    if (filename == "") filename.Set("id_grid.%d.dat", m_world->GetStats().GetUpdate());
    Avida::Output::FilePtr df = Avida::Output::File::CreateWithPath(m_world->GetNewWorld(), (const char*)filename);
    ofstream& fp = df->OFStream();
    
    for (int j = 0; j < m_world->GetPopulation().GetWorldY(); j++) {
      for (int i = 0; i < m_world->GetPopulation().GetWorldX(); i++) {
        cPopulationCell& cell = m_world->GetPopulation().GetCell(j * m_world->GetPopulation().GetWorldX() + i);
        int id = (cell.IsOccupied()) ? cell.GetOrganism()->GetID() : -1;
        fp << id << " ";
      }
      fp << endl;
    }
  }
};


class cActionDumpTargetGrid : public cAction
{
private:
  cString m_filename;
  
public:
  cActionDumpTargetGrid(cWorld* world, const cString& args, Feedback&) : cAction(world, args), m_filename("")
  {
    cString largs(args);
    if (largs.GetSize()) m_filename = largs.PopWord();  
  }
  static const cString GetDescription() { return "Arguments: [string fname='']"; }
  void Process(cAvidaContext&)
  {
    const int worldx = m_world->GetPopulation().GetWorldX();
    cString filename(m_filename);
    
    if (m_world->GetConfig().USE_AVATARS.Get()) {
      if (filename == "") filename.Set("grid_dumps/avatar_grid.%d.dat", m_world->GetStats().GetUpdate());
      Avida::Output::FilePtr df = Avida::Output::File::CreateWithPath(m_world->GetNewWorld(), (const char*)filename);
      ofstream& fp = df->OFStream();
      
      for (int j = 0; j < m_world->GetPopulation().GetWorldY(); j++) {
        for (int i = 0; i < worldx; i++) {
          cPopulationCell& cell = m_world->GetPopulation().GetCell(j * worldx + i);
          int target = -99;
          if (cell.HasAV()) {
            if (cell.HasPredAV()) target = cell.GetRandPredAV()->GetForageTarget();
            else target = cell.GetRandPreyAV()->GetForageTarget();
          } 
          fp << target << " ";
        }
        fp << endl;
      }
    }    
    
    else {
      if (filename == "") filename.Set("grid_dumps/target_grid.%d.dat", m_world->GetStats().GetUpdate());
      Avida::Output::FilePtr df = Avida::Output::File::CreateWithPath(m_world->GetNewWorld(), (const char*)filename);
      ofstream& fp = df->OFStream();
      
      for (int j = 0; j < m_world->GetPopulation().GetWorldY(); j++) {
        for (int i = 0; i < worldx; i++) {
          cPopulationCell& cell = m_world->GetPopulation().GetCell(j * worldx + i);
          int target = -99;
          if (cell.IsOccupied()) target = cell.GetOrganism()->GetForageTarget();
          fp << target << " ";
        }
        fp << endl;
      }
    }
  }
};

//DumpMaxResGrid intended for creating single output file of spatial resources, recording the max value (of any resource) when resources overlap
class cActionDumpMaxResGrid : public cAction
{
private:
  cString m_filename;
  
public:
  cActionDumpMaxResGrid(cWorld* world, const cString& args, Feedback&) : cAction(world, args), m_filename("")
  {
    cString largs(args);
    if (largs.GetSize()) m_filename = largs.PopWord();  
  }
  static const cString GetDescription() { return "Arguments: [string fname='']"; }
  void Process(cAvidaContext& ctx)
  {
    cString filename(m_filename);
    if (filename == "") filename.Set("grid_dumps/max_res_grid.%d.dat", m_world->GetStats().GetUpdate());
    Avida::Output::FilePtr df = Avida::Output::File::CreateWithPath(m_world->GetNewWorld(), (const char*)filename);
    ofstream& fp = df->OFStream();
    
    for (int j = 0; j < m_world->GetPopulation().GetWorldY(); j++) {
      for (int i = 0; i < m_world->GetPopulation().GetWorldX(); i++) {
        const Apto::Array<double> res_count = m_world->GetPopulation().GetResources().GetCellResources(j * m_world->GetPopulation().GetWorldX() + i, ctx);
        double max_resource = 0.0;    
        // get the resource library
        const cResourceDefLib& resource_lib = m_world->GetEnvironment().GetResDefLib();
        // if more than one resource is available, return the resource with the most available in this spot 
        // (note that, with global resources, the GLOBAL total will evaluated)
        // we build regular resources on top of any hills, but replace any regular resources or hills with any walls or dens 
        double topo_height = 0.0;
        for (int h = 0; h < res_count.GetSize(); h++) {
          int hab_type = resource_lib.GetResDef(h)->GetHabitat();
          if ((res_count[h] > max_resource) && (hab_type != 1) && (hab_type !=2)) max_resource = res_count[h];
          else if ((hab_type == 1 || hab_type == 4 || hab_type == 5) && res_count[h] > 0) topo_height = resource_lib.GetResDef(h)->GetPlateau();
          // allow walls to trump everything else
          else if (hab_type == 2 && res_count[h] > 0) { 
            topo_height = resource_lib.GetResDef(h)->GetPlateau();
            max_resource = 0.0;
            break;
          }
        }
        max_resource = max_resource + topo_height;
        fp << max_resource << " ";
      }
      fp << endl;
    }
  }
};



class cActionDumpGenomeLengthGrid : public cAction
{
private:
  cString m_filename;
  
public:
  cActionDumpGenomeLengthGrid(cWorld* world, const cString& args, Feedback&) : cAction(world, args), m_filename("")
  {
    cString largs(args);
    if (largs.GetSize()) m_filename = largs.PopWord();
  }
  static const cString GetDescription() { return "Arguments: [string fname='']"; }
  void Process(cAvidaContext&)
  {
    cString filename(m_filename);
    if (filename == "") filename.Set("grid_genome_length.%d.dat", m_world->GetStats().GetUpdate());
    Avida::Output::FilePtr df = Avida::Output::File::CreateWithPath(m_world->GetNewWorld(), (const char*)filename);
    ofstream& fp = df->OFStream();
    
    cPopulation* pop = &m_world->GetPopulation();
    
    for (int i = 0; i < pop->GetWorldX(); i++) {
      for (int j = 0; j < pop->GetWorldY(); j++) {
        int genome_length= 0;
        int cell_num = j * pop->GetWorldX() + i;
        if (pop->GetCell(cell_num).IsOccupied() == true)
        {
          cOrganism* organism = pop->GetCell(cell_num).GetOrganism();
          ConstInstructionSequencePtr seq;
          seq.DynamicCastFrom(organism->GetGenome().Representation());
          genome_length = seq->GetSize();
        }
        else { genome_length = -1; }
        fp << genome_length << " ";
      }
      fp << endl;
    }
  }
};


class cActionDumpTaskGrid : public cAction
{
private:
  cString m_filename;
  
public:
  cActionDumpTaskGrid(cWorld* world, const cString& args, Feedback&) : cAction(world, args), m_filename("")
  {
    cString largs(args);
    if (largs.GetSize()) m_filename = largs.PopWord();
  }
  static const cString GetDescription() { return "Arguments: [string fname='']"; }
  void Process(cAvidaContext& ctx)
  {
    cString filename(m_filename);
    if (filename == "") filename.Set("grid_task.%d.dat", m_world->GetStats().GetUpdate());
    Avida::Output::FilePtr df = Avida::Output::File::CreateWithPath(m_world->GetNewWorld(), (const char*)filename);
    ofstream& fp = df->OFStream();
    
    cPopulation* pop = &m_world->GetPopulation();
    cTestCPU* testcpu = m_world->GetHardwareManager().CreateTestCPU(ctx);
    
    const int num_tasks = m_world->GetEnvironment().GetNumTasks();
    
    for (int i = 0; i < pop->GetWorldX(); i++) {
      for (int j = 0; j < pop->GetWorldY(); j++) {
        int task_sum = 0;
        int cell_num = j * pop->GetWorldX() + i;
        if (pop->GetCell(cell_num).IsOccupied() == true) {
          cOrganism* organism = pop->GetCell(cell_num).GetOrganism();
          cCPUTestInfo test_info;
          testcpu->TestGenome(ctx, test_info, organism->GetGenome());
          cPhenotype& test_phenotype = test_info.GetTestPhenotype();
          for (int k = 0; k < num_tasks; k++) {
            if (test_phenotype.GetLastTaskCount()[k] > 0) task_sum += static_cast<int>(pow(2.0, k));
          }
        }
        fp << task_sum << " ";
      }
      fp << endl;
    }
    
    delete testcpu;
  }
};



/* Dumps the task grid of the last task performed by each organism. */
class cActionDumpLastTaskGrid : public cAction
{
private:
  cString m_filename;
  
public:
  cActionDumpLastTaskGrid(cWorld* world, const cString& args, Feedback&) : cAction(world, args), m_filename("")
  {
    cString largs(args);
    if (largs.GetSize()) m_filename = largs.PopWord();  
  }
  static const cString GetDescription() { return "Arguments: [string fname='']"; }
  void Process(cAvidaContext&)
  {
    cString filename(m_filename);
    if (filename == "") filename.Set("grid_last_task.%d.dat", m_world->GetStats().GetUpdate());
    Avida::Output::FilePtr df = Avida::Output::File::CreateWithPath(m_world->GetNewWorld(), (const char*)filename);
    ofstream& fp = df->OFStream();
    
    cPopulation* pop = &m_world->GetPopulation();
    int task_id;      
    for (int i = 0; i < pop->GetWorldX(); i++) {
      for (int j = 0; j < pop->GetWorldY(); j++) {
        int cell_num = i * pop->GetWorldX() + j;
        if (pop->GetCell(cell_num).IsOccupied() == true) {
          cOrganism* organism = pop->GetCell(cell_num).GetOrganism();
          task_id = organism->GetPhenotype().GetLastTaskID();
        } else {
          task_id = -1;
        }
        fp << m_world->GetStats().GetUpdate() << " " << cell_num << " "  << task_id << endl;
      }
    }
  }
};





//Dump the reaction grid from the last gestation cycle, so skip the
//test cpu, and just use what the phenotype has.
class cActionDumpReactionGrid : public cAction
{
private:
  cString m_filename;
  
public:
  cActionDumpReactionGrid(cWorld* world, const cString& args, Feedback&) : cAction(world, args), m_filename("")
  {
    cString largs(args);
    if (largs.GetSize()) m_filename = largs.PopWord();
  }
  static const cString GetDescription() { return "Arguments: [string fname='']"; }
  void Process(cAvidaContext&)
  {
    cString filename(m_filename);
    if (filename == "") filename.Set("grid_reactions.%d.dat", m_world->GetStats().GetUpdate());
    Avida::Output::FilePtr df = Avida::Output::File::CreateWithPath(m_world->GetNewWorld(), (const char*)filename);
    ofstream& fp = df->OFStream();
    
    cPopulation* pop = &m_world->GetPopulation();
    
    const int num_tasks = m_world->GetEnvironment().GetNumTasks();
    
    for (int i = 0; i < pop->GetWorldX(); i++) {
      for (int j = 0; j < pop->GetWorldY(); j++) {
        int task_sum = 0;
        int cell_num = j * pop->GetWorldX() + i;
        if (pop->GetCell(cell_num).IsOccupied() == true) {
          cOrganism* organism = pop->GetCell(cell_num).GetOrganism();
          
          cPhenotype& test_phenotype = organism->GetPhenotype();
          for (int k = 0; k < num_tasks; k++) {
            if (test_phenotype.GetLastReactionCount()[k] > 0) task_sum += static_cast<int>(pow(2.0, k));
          }
        }
        else {task_sum = -1;}
        fp << task_sum << " ";
      }
      fp << endl;
    }
  }
};

class cActionDumpGenotypeGrid : public cAction
{
private:
  cString m_filename;
  
public:
  cActionDumpGenotypeGrid(cWorld* world, const cString& args, Feedback&) : cAction(world, args), m_filename("")
  {
    cString largs(args);
    if (largs.GetSize()) m_filename = largs.PopWord();
  }
  static const cString GetDescription() { return "Arguments: [string fname='']"; }
  void Process(cAvidaContext&)
  {
    cString filename(m_filename);
    if (filename == "") filename.Set("grid_genome.%d.dat", m_world->GetStats().GetUpdate());
    Avida::Output::FilePtr df = Avida::Output::File::CreateWithPath(m_world->GetNewWorld(), (const char*)filename);
    ofstream& fp = df->OFStream();
    
    cPopulation* pop = &m_world->GetPopulation();
    
    for (int i = 0; i < pop->GetWorldX(); i++) {
      for (int j = 0; j < pop->GetWorldY(); j++) {
        cString genome_seq("");
        int cell_num = j * pop->GetWorldX() + i;
        if (pop->GetCell(cell_num).IsOccupied() == true)
        {
          cOrganism* organism = pop->GetCell(cell_num).GetOrganism();
          ConstInstructionSequencePtr seq;
          seq.DynamicCastFrom(organism->GetGenome().Representation());
          genome_seq = seq->AsString();
        }
        else { genome_seq = "-1"; }
        fp << genome_seq << " ";
      }
      fp << endl;
    }
  }
};



class cActionPrintOrgLocData : public cAction
{
private:
  cString m_filename;
  
public:
  cActionPrintOrgLocData(cWorld* world, const cString& args, Feedback&) : cAction(world, args), m_filename("")
  {
    /*Print organism locations + other org data (for movies). */
    cString largs(args);
    if (largs.GetSize()) m_filename = largs.PopWord();  
  }
  static const cString GetDescription() { return "Arguments: [string fname='']"; }
  void Process(cAvidaContext& ctx)
  {
    cString filename(m_filename);
    if (filename == "") filename.Set("grid_dumps/org_loc.%d.dat", m_world->GetStats().GetUpdate());
    Avida::Output::FilePtr df = Avida::Output::File::CreateWithPath(m_world->GetNewWorld(), (const char*)filename);
    ofstream& fp = df->OFStream();
    
    bool use_av = m_world->GetConfig().USE_AVATARS.Get();
    if (!use_av) fp << "# org_id,org_cellx,org_celly,org_forage_target,org_group_id,org_facing" << endl;
    else fp << "# org_id,org_cellx,org_celly,org_forage_target,org_group_id,org_facing,av_cellx,av_celly,av_facing" << endl;
    
    const int worldx = m_world->GetConfig().WORLD_X.Get();
    
    const Apto::Array <cOrganism*, Apto::Smart> live_orgs = m_world->GetPopulation().GetLiveOrgList();
    for (int i = 0; i < live_orgs.GetSize(); i++) {  
      cOrganism* org = live_orgs[i];
      const int id = org->GetID();
      const int loc = org->GetCellID();
      const int locx = loc % worldx;
      const int locy = loc / worldx;
      const int ft = org->GetForageTarget();
      const int faced_dir = org->GetFacedDir();
      
      fp << id << "," << locx << "," << locy << "," << ft << "," <<  -1 << "," <<  faced_dir;
      if (use_av) {
        const int avloc = org->GetOrgInterface().GetAVCellID();
        const int avlocx = avloc % worldx;
        const int avlocy = avloc / worldx;
        const int avfaced_dir = org->GetOrgInterface().GetAVFacing();
        
        fp << "," << avlocx << "," << avlocy << "," << avfaced_dir;
      }
      fp << endl;
    }
  }
};

class cActionPrintPreyFlockingData : public cAction
{
private:
  cString m_filename;
  
public:
  cActionPrintPreyFlockingData(cWorld* world, const cString& args, Feedback&) : cAction(world, args), m_filename("")
  {
    /*Print data on prey neighborhood */
    cString largs(args);
    if (largs.GetSize()) m_filename = largs.PopWord();  
  }
  static const cString GetDescription() { return "Arguments: [string fname='']"; }
  void Process(cAvidaContext& ctx)
  {
    cString filename(m_filename);
    if (filename == "") filename.Set("grid_dumps/prey_flocking.%d.dat", m_world->GetStats().GetUpdate());    
    Avida::Output::FilePtr df = Avida::Output::File::CreateWithPath(m_world->GetNewWorld(), (const char*)filename);
    ofstream& fp = df->OFStream();
    
    bool use_av = m_world->GetConfig().USE_AVATARS.Get();
    if (!use_av) fp << "# org_id,org_cellx,org_celly,num_prey_neighbors,num_prey_this_cell" << endl;
    else fp << "# org_id,org_av_cellx,org_av_celly,num_neighbor_cells_with_prey,num_prey_this_cell" << endl;
    
    const int worldx = m_world->GetConfig().WORLD_X.Get();
    
    Apto::Array<int> neighborhood;
    const Apto::Array <cOrganism*, Apto::Smart> live_orgs = m_world->GetPopulation().GetLiveOrgList();
    for (int i = 0; i < live_orgs.GetSize(); i++) {
      cOrganism* org = live_orgs[i];
      if (!org->IsPreyFT()) continue;
      const int id = org->GetID();
      int num_neighbors = 0;
      neighborhood.Resize(0);
      if (!use_av) {
        const int loc = org->GetCellID();
        const int locx = loc % worldx;
        const int locy = loc / worldx;
        org->GetOrgInterface().GetNeighborhoodCellIDs(neighborhood);
        for (int j = 0; j < neighborhood.GetSize(); j++) {
          if (m_world->GetPopulation().GetCell(neighborhood[j]).IsOccupied()) {
            if (m_world->GetPopulation().GetCell(neighborhood[j]).GetOrganism()->IsPreyFT()) {
              num_neighbors++;
            }
          }
        }
        fp << id << "," << locx << "," << locy << "," << num_neighbors << "," << "1";
      }
      else {
        const int avloc = org->GetOrgInterface().GetAVCellID();
        const int num_prey_this_cell = m_world->GetPopulation().GetCell(avloc).GetNumPreyAV();
        const int avlocx = avloc % worldx;
        const int avlocy = avloc / worldx;
        org->GetOrgInterface().GetAVNeighborhoodCellIDs(neighborhood);
        for (int j = 0; j < neighborhood.GetSize(); j++) {
          if (m_world->GetPopulation().GetCell(neighborhood[j]).HasPreyAV()) num_neighbors++;
        }
        fp << id << "," << avlocx << "," << avlocy << "," << num_neighbors << "," << num_prey_this_cell;
      }
      fp << endl;
    }
  }
};



class cActionSetVerbose : public cAction
{
private:
  cString m_verbose;
  
public:
  cActionSetVerbose(cWorld* world, const cString& args, Feedback&) : cAction(world, args), m_verbose("")
  {
    cString largs(args);
    if (largs.GetSize()) m_verbose = largs.PopWord();
    m_verbose.ToUpper();
  }
  static const cString GetDescription() { return "Arguments: [string verbosity='']"; }
  void Process(cAvidaContext&)
  {
    // If no arguments are given, assume a basic toggle.
    // Otherwise, read in the argument to decide the new mode.
    if (m_verbose.GetSize() == 0 && m_world->GetVerbosity() <= VERBOSE_NORMAL) {
      m_world->SetVerbosity(VERBOSE_ON);
    } else if (m_verbose.GetSize() == 0 && m_world->GetVerbosity() >= VERBOSE_ON) {
      m_world->SetVerbosity(VERBOSE_NORMAL);
    } else if (m_verbose == "SILENT") m_world->SetVerbosity(VERBOSE_SILENT);
    else if (m_verbose == "NORMAL") m_world->SetVerbosity(VERBOSE_NORMAL);
    else if (m_verbose == "QUIET") m_world->SetVerbosity(VERBOSE_NORMAL);
    else if (m_verbose == "OFF") m_world->SetVerbosity(VERBOSE_NORMAL);
    else if (m_verbose == "ON") m_world->SetVerbosity(VERBOSE_ON);
    else if (m_verbose == "DETAILS") m_world->SetVerbosity(VERBOSE_DETAILS);
    else if (m_verbose == "HIGH") m_world->SetVerbosity(VERBOSE_DETAILS);
    else m_world->SetVerbosity(VERBOSE_NORMAL);
    
    // Print out new verbose level (nothing for silent!)
    if (m_world->GetVerbosity() == VERBOSE_NORMAL) {
      cout << "Verbose NORMAL: Using standard log messages..." << endl;
    } else if (m_world->GetVerbosity() == VERBOSE_ON) {
      cout << "Verbose ON: Using verbose log messages..." << endl;
    } else if (m_world->GetVerbosity() == VERBOSE_DETAILS) {
      cout << "Verbose DETAILS: Using detailed log messages..." << endl;
    }
    
  }
};


class cActionPrintDebug : public cAction
{
public:
  cActionPrintDebug(cWorld* world, const cString& args, Feedback&) : cAction(world, args) { ; }
  
  static const cString GetDescription() { return "No Arguments"; }
  void Process(cAvidaContext&)
  {
  }
};



class cActionPrintSoloTaskSnapshot : public cAction
{
private:
  cString m_filename;
public:
  cActionPrintSoloTaskSnapshot(cWorld* world, const cString& args, Feedback&) : cAction(world, args)
  {
    cString largs(args);
    if (largs == "") m_filename = "tasks-snap.dat"; else m_filename = largs.PopWord();
  }
  static const cString GetDescription() { return "Arguments: [string fname=\"tasks-snap.dat\"]"; }
  void Process(cAvidaContext& ctx)
  {
    m_world->GetStats().PrintSoloTaskSnapshot(m_filename, ctx);
  }
};



//Prints data about all the 'offspring' waiting in the birth chamber
class cActionPrintBirthChamber : public cAction
{
private:
  cString m_filename;
  int m_hw_type;
  
public:
  cActionPrintBirthChamber(cWorld* world, const cString& args, Feedback&) : cAction(world, args), m_filename(""), m_hw_type(0)
  {
    cString largs(args);
    largs.Trim();
    if (largs.GetSize()) m_filename = largs.PopWord();
    if (largs.GetSize()) m_hw_type = largs.PopWord().AsInt();
    else m_hw_type = 0;
  }
  
  static const cString GetDescription() { return "Arguments: [string fname=\"birth_chamber/bc-XXXX.dat\"] [int hwtype=0]"; }
  
  void Process(cAvidaContext&)
  {
    cString filename(m_filename);
    if (filename == "") filename.Set( "birth_chamber/bc-%s.dat", (const char*)cStringUtil::Convert(m_world->GetStats().GetUpdate()));
    m_world->GetPopulation().GetBirthChamber(m_hw_type).PrintBirthChamber(filename, m_hw_type);
  }
};


//Prints data about all the 'offspring' waiting in the birth chamber
class cActionPrintDominantData : public cAction
{
private:
  cString m_filename;
  
public:
  cActionPrintDominantData(cWorld* world, const cString& args, Feedback&) : cAction(world, args), m_filename("dominant.dat")
  {
    cString largs(args);
    largs.Trim();
    if (largs.GetSize()) m_filename = largs.PopWord();
  }
  
  static const cString GetDescription() { return "Arguments: [string fname=\"dominant.dat\"]"; }
  
  void Process(cAvidaContext&)
  {
    Avida::Output::FilePtr df = Avida::Output::File::StaticWithPath(m_world->GetNewWorld(), (const char*)m_filename);
    
    df->WriteComment("Avida Dominant Data");
    df->WriteTimeStamp();
    
    df->Write(m_world->GetStats().GetUpdate(),     "Update");
    
    Systematics::ManagerPtr classmgr = Systematics::Manager::Of(m_world->GetNewWorld());
    Systematics::Arbiter::IteratorPtr it = classmgr->ArbiterForRole("genotype")->Begin();
    Systematics::GroupPtr bg = it->Next();
    if (!bg) return;
    
    df->Write(bg->Properties().Get("ave_metabolic_rate").DoubleValue(),       "Average Merit of the Dominant Genotype");
    df->Write(bg->Properties().Get("ave_gestation_time").DoubleValue(),   "Average Gestation Time of the Dominant Genotype");
    df->Write(bg->Properties().Get("ave_fitness").DoubleValue(),     "Average Fitness of the Dominant Genotype");
    df->Write(bg->Properties().Get("ave_repro_rate").DoubleValue(),  "Repro Rate?");
    
    Genome gen(bg->Properties().Get("genome"));
    InstructionSequencePtr seq;
    seq.DynamicCastFrom(gen.Representation());
    df->Write(seq->GetSize(),        "Size of Dominant Genotype");
    df->Write(bg->Properties().Get("ave_copy_size").DoubleValue(), "Copied Size of Dominant Genotype");
    df->Write(bg->Properties().Get("ave_exe_size").DoubleValue(), "Executed Size of Dominant Genotype");
    df->Write(bg->NumUnits(),   "Abundance of Dominant Genotype");
    df->Write(bg->Properties().Get("last_births").IntValue(),      "Number of Births");
    df->Write(bg->Properties().Get("last_breed_true").IntValue(),  "Number of Dominant Breed True?");
    df->Write(bg->Depth(),  "Dominant Gene Depth");
    df->Write(bg->Properties().Get("last_breed_in").IntValue(),    "Dominant Breed In");
    df->Write(bg->Properties().Get("max_fitness").DoubleValue(),     "Max Fitness?");
    df->Write(bg->ID(), "Genotype ID of Dominant Genotype");
    df->Write(bg->Properties().Get("name").StringValue(),        "Name of the Dominant Genotype");
    df->Endl();    
  }
};






void RegisterPrintActions(cActionLibrary* action_lib)
{
  action_lib->Register<cActionPrintDebug>("PrintDebug");
  
  
  // Stats Out Files
  action_lib->Register<cActionPrintAverageData>("PrintAverageData");
  action_lib->Register<cActionPrintErrorData>("PrintErrorData");
  action_lib->Register<cActionPrintVarianceData>("PrintVarianceData");
  action_lib->Register<cActionPrintCountData>("PrintCountData");
  action_lib->Register<cActionPrintTotalsData>("PrintTotalsData");
  action_lib->Register<cActionPrintThreadsData>("PrintThreadsData");
  action_lib->Register<cActionPrintTasksData>("PrintTasksData");
  action_lib->Register<cActionPrintSoloTaskSnapshot>("PrintSoloTaskSnapshot");
  action_lib->Register<cActionPrintTasksExeData>("PrintTasksExeData");
  action_lib->Register<cActionPrintNewTasksData>("PrintNewTasksData");
  action_lib->Register<cActionPrintNewReactionData>("PrintNewReactionData");
  action_lib->Register<cActionPrintNewTasksDataPlus>("PrintNewTasksDataPlus");
  action_lib->Register<cActionPrintTasksQualData>("PrintTasksQualData");
  action_lib->Register<cActionPrintResourceData>("PrintResourceData");
  action_lib->Register<cActionPrintResourceLocData>("PrintResourceLocData");
  action_lib->Register<cActionPrintResWallLocData>("PrintResWallLocData");
  action_lib->Register<cActionPrintReactionData>("PrintReactionData");
  action_lib->Register<cActionPrintReactionExeData>("PrintReactionExeData");
  action_lib->Register<cActionPrintCurrentReactionData>("PrintCurrentReactionData");
  action_lib->Register<cActionPrintReactionRewardData>("PrintReactionRewardData");
  action_lib->Register<cActionPrintCurrentReactionRewardData>("PrintCurrentReactionRewardData");
  action_lib->Register<cActionPrintTimeData>("PrintTimeData");
  action_lib->Register<cActionPrintExtendedTimeData>("PrintExtendedTimeData");
  action_lib->Register<cActionPrintMutationRateData>("PrintMutationRateData");
  action_lib->Register<cActionPrintDivideMutData>("PrintDivideMutData");
  
  action_lib->Register<cActionPrintPreyAverageData>("PrintPreyAverageData");
  action_lib->Register<cActionPrintPredatorAverageData>("PrintPredatorAverageData");
  action_lib->Register<cActionPrintTopPredatorAverageData>("PrintTopPredatorAverageData");
  action_lib->Register<cActionPrintPreyErrorData>("PrintPreyErrorData");
  action_lib->Register<cActionPrintPredatorErrorData>("PrintPredatorErrorData");
  action_lib->Register<cActionPrintTopPredatorErrorData>("PrintTopPredatorErrorData");
  action_lib->Register<cActionPrintPreyVarianceData>("PrintPreyVarianceData");
  action_lib->Register<cActionPrintPredatorVarianceData>("PrintPredatorVarianceData");
  action_lib->Register<cActionPrintTopPredatorVarianceData>("PrintTopPredatorVarianceData");
  action_lib->Register<cActionPrintPreyInstructionData>("PrintPreyInstructionData");
  action_lib->Register<cActionPrintPredatorInstructionData>("PrintPredatorInstructionData");
  action_lib->Register<cActionPrintTopPredatorInstructionData>("PrintTopPredatorInstructionData");
  action_lib->Register<cActionPrintPreyFromSensorInstructionData>("PrintPreyFromSensorInstructionData");
  action_lib->Register<cActionPrintPredatorFromSensorInstructionData>("PrintPredatorFromSensorInstructionData");
  action_lib->Register<cActionPrintTopPredatorFromSensorInstructionData>("PrintTopPredatorFromSensorInstructionData");
  action_lib->Register<cActionPrintKilledPreyFTData>("PrintKilledPreyFTData");
  
  action_lib->Register<cActionPrintSenseData>("PrintSenseData");
  action_lib->Register<cActionPrintSenseExeData>("PrintSenseExeData");
  action_lib->Register<cActionPrintInstructionData>("PrintInstructionData");
  action_lib->Register<cActionPrintDynamicMaxMinData>("PrintDynamicMaxMinData");
  
  
  action_lib->Register<cActionPrintOrganismLocation>("PrintOrganismLocation");
  action_lib->Register<cActionPrintOrgLocData>("PrintOrgLocData");
  action_lib->Register<cActionPrintPreyFlockingData>("PrintPreyFlockingData");

  
  // Processed Data
  action_lib->Register<cActionPrintData>("PrintData");
  action_lib->Register<cActionPrintInstructionAbundanceHistogram>("PrintInstructionAbundanceHistogram");
  action_lib->Register<cActionPrintDepthHistogram>("PrintDepthHistogram");
  action_lib->Register<cActionEcho>("Echo");
  action_lib->Register<cActionPrintGenotypeAbundanceHistogram>("PrintGenotypeAbundanceHistogram");
  action_lib->Register<cActionPrintDominantGenotype>("PrintDominantGenotype");
  action_lib->Register<cActionPrintDominantGroupGenotypes>("PrintDominantGroupGenotypes");
  action_lib->Register<cActionPrintDominantForagerGenotypes>("PrintDominantForagerGenotypes");
  action_lib->Register<cActionPrintDetailedFitnessData>("PrintDetailedFitnessData");
  action_lib->Register<cActionPrintLogFitnessHistogram>("PrintLogFitnessHistogram");
  action_lib->Register<cActionPrintRelativeFitnessHistogram>("PrintRelativeFitnessHistogram");
  action_lib->Register<cActionPrintGeneticDistanceData>("PrintGeneticDistanceData");
  action_lib->Register<cActionPrintPopulationDistanceData>("PrintPopulationDistanceData");
  
  action_lib->Register<cActionTestDominant>("TestDominant");
  action_lib->Register<cActionPrintTaskSnapshot>("PrintTaskSnapshot");
  action_lib->Register<cActionPrintViableTasksData>("PrintViableTasksData");
  action_lib->Register<cActionPrintAveNumTasks>("PrintAveNumTasks");
  
  // Grid Information Dumps
  action_lib->Register<cActionDumpClassificationIDGrid>("DumpClassificationIDGrid");
  action_lib->Register<cActionDumpFitnessGrid>("DumpFitnessGrid");
  action_lib->Register<cActionDumpGenotypeColorGrid>("DumpGenotypeColorGrid");
  action_lib->Register<cActionDumpPhenotypeIDGrid>("DumpPhenotypeIDGrid");
  action_lib->Register<cActionDumpIDGrid>("DumpIDGrid");
  action_lib->Register<cActionDumpTargetGrid>("DumpTargetGrid");
  action_lib->Register<cActionDumpMaxResGrid>("DumpMaxResGrid");
  action_lib->Register<cActionDumpTaskGrid>("DumpTaskGrid");
  action_lib->Register<cActionDumpLastTaskGrid>("DumpLastTaskGrid");
  
  action_lib->Register<cActionDumpReactionGrid>("DumpReactionGrid");
  action_lib->Register<cActionDumpGenomeLengthGrid>("DumpGenomeLengthGrid");
  action_lib->Register<cActionDumpGenotypeGrid>("DumpGenotypeGrid");
  
  action_lib->Register<cActionPrintNumOrgsKilledData>("PrintNumOrgsKilledData");
  
  action_lib->Register<cActionPrintTargets>("PrintTargets");
  action_lib->Register<cActionPrintMimicDisplays>("PrintMimicDisplays");
  action_lib->Register<cActionPrintTopPredTargets>("PrintTopPredTargets");
  
  action_lib->Register<cActionSetVerbose>("SetVerbose");
  action_lib->Register<cActionSetVerbose>("VERBOSE");
  
  action_lib->Register<cActionPrintBirthChamber>("PrintBirthChamber");
  
  
  action_lib->Register<cActionPrintDominantData>("PrintDominantData");
  
}
