#define MOBIUS_TIMESTEP_VERBOSITY 0
//NOTE: the g++ compiler flag ffast-math will make it so that isnan does not work correctly, so don't use that flag.
#define MOBIUS_TEST_FOR_NAN 0
#define MOBIUS_EQUATION_PROFILING 0
#define MOBIUS_PRINT_TIMING_INFO 0


#include "../../mobius.h"

#include "../../Modules/UnitConversions.h"
#include "../../Modules/SimplyC.h"
#include "../../Modules/SimplyQ_noGW.h"
#include "../../Modules/SoilTemperature_simply.h"

#include "../../incaview_compatibility.h"

int main(int argc, char **argv)
{
	incaview_commandline_arguments Args;
	ParseIncaviewCommandline(argc, argv, &Args);
	
	mobius_model *Model = BeginModelDefinition("SimplyC", "0.1");
	
	auto Days 	      = RegisterUnit(Model, "days");
	auto System = RegisterParameterGroup(Model, "System");
	RegisterParameterUInt(Model, System, "Timesteps", Days, 100);
	RegisterParameterDate(Model, System, "Start date", "1991-1-1");
	
	AddSimplyHydrologyModule(Model);
	AddSoilTemperatureModel(Model);
	AddSimplyCModel(Model);
	
	EnsureModelComplianceWithIncaviewCommandline(Model, &Args);
	
	EndModelDefinition(Model);
	
	mobius_data_set *DataSet = GenerateDataSet(Model);
	
	RunDatasetAsSpecifiedByIncaviewCommandline(DataSet, &Args);
}