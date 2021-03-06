
#include <sstream>

static int Dll_GlobalErrorCode = 0;
std::stringstream Dll_GlobalErrstream;

#define MOBIUS_PARTIAL_ERROR(Msg) \
	{Dll_GlobalErrstream << Msg; \
	Dll_GlobalErrorCode = 1;}
	
#define MOBIUS_FATAL_ERROR(Msg) \
	{MOBIUS_PARTIAL_ERROR(Msg) \
	throw 0;}


#include "../mobius.h"


#define CHECK_ERROR_BEGIN \
try {

#define CHECK_ERROR_END \
} catch(int Errcode) { \
}

#if (defined(_WIN32) || defined(_WIN64))
	#define DLLEXPORT extern "C" __declspec(dllexport)
#elif (defined(__unix__) || defined(__linux__) || defined(__unix) || defined(unix))
	#define DLLEXPORT extern "C" __attribute((visibility("default")))
#endif

DLLEXPORT int
DllEncounteredError(char *ErrmsgOut)
{
	std::string ErrStr = Dll_GlobalErrstream.str();
	strcpy(ErrmsgOut, ErrStr.c_str());
	
	int Code = Dll_GlobalErrorCode;
	
	//NOTE: Since IPython does not seem to reload the dll when you restart it (normally), we have to clear this.
	Dll_GlobalErrorCode = 0;
	Dll_GlobalErrstream.clear();
	
	return Code;
}

DLLEXPORT void
DllRunModel(void *DataSetPtr)
{
	CHECK_ERROR_BEGIN
	
	RunModel((mobius_data_set *)DataSetPtr);
	
	CHECK_ERROR_END
}

DLLEXPORT void *
DllCopyDataSet(void *DataSetPtr)
{
	CHECK_ERROR_BEGIN
	
	return (void *)CopyDataSet((mobius_data_set *)DataSetPtr);
	
	CHECK_ERROR_END
	
	return 0;
}

DLLEXPORT void
DllDeleteDataSet(void *DataSetPtr)
{
	CHECK_ERROR_BEGIN
	
	delete (mobius_data_set *)DataSetPtr;
	
	CHECK_ERROR_END
}

DLLEXPORT u64
DllGetTimesteps(void *DataSetPtr)
{
	CHECK_ERROR_BEGIN
	
	return GetTimesteps((mobius_data_set *)DataSetPtr);
	
	CHECK_ERROR_END
	
	return 0;
}

DLLEXPORT u64
DllGetInputTimesteps(void *DataSetPtr)
{
	CHECK_ERROR_BEGIN
	
	mobius_data_set *DataSet = (mobius_data_set *)DataSetPtr;
	return DataSet->InputDataTimesteps;
	
	CHECK_ERROR_END
	
	return 0;
}

DLLEXPORT void
DllGetResultSeries(void *DataSetPtr, char *Name, char **IndexNames, u64 IndexCount, double *WriteTo)
{
	CHECK_ERROR_BEGIN
	
	mobius_data_set *DataSet = (mobius_data_set *)DataSetPtr;
	
	size_t Timesteps = GetTimesteps(DataSet);
	
	GetResultSeries(DataSet, Name, IndexNames, (size_t)IndexCount, WriteTo, Timesteps);
	
	CHECK_ERROR_END
}

DLLEXPORT void
DllGetInputSeries(void *DataSetPtr, char *Name, char **IndexNames, u64 IndexCount, double *WriteTo, bool AlignWithResults)
{
	CHECK_ERROR_BEGIN
	
	mobius_data_set *DataSet = (mobius_data_set *)DataSetPtr;
	
	size_t Timesteps = GetTimesteps(DataSet);
	if(!AlignWithResults)
	{
		Timesteps = DataSet->InputDataTimesteps;
	}
	
	GetInputSeries(DataSet, Name, IndexNames, (size_t)IndexCount, WriteTo, Timesteps, AlignWithResults);
	
	CHECK_ERROR_END
}

DLLEXPORT void
DllSetParameterDouble(void *DataSetPtr, char *Name, char **IndexNames, u64 IndexCount, double Val)
{
	CHECK_ERROR_BEGIN
	
	mobius_data_set *DataSet = (mobius_data_set *)DataSetPtr;
	parameter_value Value;
	Value.ValDouble = Val;
	SetParameterValue(DataSet, Name, IndexNames, (size_t)IndexCount, Value, ParameterType_Double);
	
	CHECK_ERROR_END
}

DLLEXPORT void
DllSetParameterUInt(void *DataSetPtr, char *Name, char **IndexNames, u64 IndexCount, u64 Val)
{
	CHECK_ERROR_BEGIN
	
	mobius_data_set *DataSet = (mobius_data_set *)DataSetPtr;
	parameter_value Value;
	Value.ValUInt = Val;
	SetParameterValue(DataSet, Name, IndexNames, (size_t)IndexCount, Value, ParameterType_UInt);
	
	CHECK_ERROR_END
}

DLLEXPORT void
DllSetParameterBool(void *DataSetPtr, char *Name, char **IndexNames, u64 IndexCount, bool Val)
{
	CHECK_ERROR_BEGIN
	
	mobius_data_set *DataSet = (mobius_data_set *)DataSetPtr;
	parameter_value Value;
	Value.ValBool = Val;
	SetParameterValue(DataSet, Name, IndexNames, (size_t)IndexCount, Value, ParameterType_Bool);
	
	CHECK_ERROR_END
}

DLLEXPORT void
DllSetParameterTime(void *DataSetPtr, char *Name, char **IndexNames, u64 IndexCount, char *Val)
{
	CHECK_ERROR_BEGIN
	
	mobius_data_set *DataSet = (mobius_data_set *)DataSetPtr;
	parameter_value Value;
	bool ParseSuccess;
	Value.ValTime = datetime(Val, &ParseSuccess);
	if(!ParseSuccess)
	{
		MOBIUS_FATAL_ERROR("ERROR: Unrecognized date format provided for the value of the parameter " << Name << std::endl)
	}
	SetParameterValue(DataSet, Name, IndexNames, (size_t)IndexCount, Value, ParameterType_Time);
	
	CHECK_ERROR_END
}

DLLEXPORT double
DllGetParameterDouble(void *DataSetPtr, char *Name, char **IndexNames, u64 IndexCount)
{
	CHECK_ERROR_BEGIN
	
	return GetParameterValue((mobius_data_set *)DataSetPtr, Name, IndexNames, (size_t)IndexCount, ParameterType_Double).ValDouble;
	
	CHECK_ERROR_END
	
	return 0.0;
}

DLLEXPORT u64
DllGetParameterUInt(void *DataSetPtr, char *Name, char **IndexNames, u64 IndexCount)
{
	CHECK_ERROR_BEGIN
	
	return GetParameterValue((mobius_data_set *)DataSetPtr, Name, IndexNames, (size_t)IndexCount, ParameterType_UInt).ValUInt;
	
	CHECK_ERROR_END
	
	return 0;
}

DLLEXPORT bool
DllGetParameterBool(void *DataSetPtr, char *Name, char **IndexNames, u64 IndexCount)
{
	CHECK_ERROR_BEGIN
	
	return GetParameterValue((mobius_data_set *)DataSetPtr, Name, IndexNames, (size_t)IndexCount, ParameterType_Bool).ValBool;
	
	CHECK_ERROR_END
	
	return false;
}

DLLEXPORT void
DllGetParameterTime(void *DataSetPtr, const char *Name, char **IndexNames, u64 IndexCount, char *WriteTo)
{
	//IMPORTANT: This assumes that WriteTo is a char* buffer that is long enough (i.e. at least 11-12-ish bytes)
	//IMPORTANT: This is NOT thread safe since datetime::ToString is not thread safe.
	CHECK_ERROR_BEGIN
	
	datetime DateTime = GetParameterValue((mobius_data_set *)DataSetPtr, Name, IndexNames, (size_t)IndexCount, ParameterType_Time).ValTime;
	char *TimeStr = DateTime.ToString();
	strcpy(WriteTo, TimeStr);
	
	CHECK_ERROR_END
}

DLLEXPORT void
DllGetParameterDoubleMinMax(void *DataSetPtr, char *Name, double *MinOut, double *MaxOut)
{
	CHECK_ERROR_BEGIN
	//TODO: We don't check that the requested parameter was of type double!
	mobius_data_set *DataSet = (mobius_data_set *)DataSetPtr;
	entity_handle Handle = GetParameterHandle(DataSet->Model, Name);
	const parameter_spec &Spec = DataSet->Model->ParameterSpecs[Handle];
	if(Spec.Type != ParameterType_Double)
	{
		MOBIUS_FATAL_ERROR("ERROR: Requested the min and max values of " << Name << " using DllGetParameterDoubleMinMax, but it is not of type double." << std::endl);
	}
	*MinOut = Spec.Min.ValDouble;
	*MaxOut = Spec.Max.ValDouble;
	
	CHECK_ERROR_END
}

DLLEXPORT void
DllGetParameterUIntMinMax(void *DataSetPtr, char *Name, u64 *MinOut, u64 *MaxOut)
{
	CHECK_ERROR_BEGIN
	
	//TODO: We don't check that the requested parameter was of type uint!
	mobius_data_set *DataSet = (mobius_data_set *)DataSetPtr;
	entity_handle Handle = GetParameterHandle(DataSet->Model, Name);
	const parameter_spec &Spec = DataSet->Model->ParameterSpecs[Handle];
	if(Spec.Type != ParameterType_UInt)
	{
		MOBIUS_FATAL_ERROR("ERROR: Requested the min and max values of " << Name << " using DllGetParameterUIntMinMax, but it is not of type uint." << std::endl);
	}
	*MinOut = Spec.Min.ValUInt;
	*MaxOut = Spec.Max.ValUInt;
	
	CHECK_ERROR_END
}

DLLEXPORT const char *
DllGetParameterDescription(void *DataSetPtr, char *Name)
{
	CHECK_ERROR_BEGIN
	
	mobius_data_set *DataSet = (mobius_data_set *)DataSetPtr;
	entity_handle Handle = GetParameterHandle(DataSet->Model, Name);
	const parameter_spec &Spec = DataSet->Model->ParameterSpecs[Handle];
	return Spec.Description;
	
	CHECK_ERROR_END
	
	return 0;
}

DLLEXPORT const char *
DllGetParameterUnit(void *DataSetPtr, char *Name)
{
	CHECK_ERROR_BEGIN
	
	mobius_data_set *DataSet = (mobius_data_set *)DataSetPtr;
	entity_handle Handle = GetParameterHandle(DataSet->Model, Name);
	const parameter_spec &Spec = DataSet->Model->ParameterSpecs[Handle];
	return GetName(DataSet->Model, Spec.Unit);
	
	CHECK_ERROR_END
	
	return 0;
}

DLLEXPORT const char *
DllGetResultUnit(void *DataSetPtr, char *Name)
{
	CHECK_ERROR_BEGIN
	
	mobius_data_set *DataSet = (mobius_data_set *)DataSetPtr;
	equation_h Equation = GetEquationHandle(DataSet->Model, Name);
	const equation_spec &Spec = DataSet->Model->EquationSpecs[Equation.Handle];
	return GetName(DataSet->Model, Spec.Unit);
	
	CHECK_ERROR_END
	
	return 0;
}

DLLEXPORT void
DllGetInputStartDate(void *DataSetPtr, char *WriteTo)
{
	CHECK_ERROR_BEGIN
	//IMPORTANT: This assumes that WriteTo is a char* buffer that is long enough (i.e. at least 11-12-ish bytes)
	//IMPORTANT: This is NOT thread safe since TimeString is not thread safe.
	mobius_data_set *DataSet = (mobius_data_set *)DataSetPtr;
	
	char *TimeStr = GetInputStartDate(DataSet).ToString();
	strcpy(WriteTo, TimeStr);
	
	CHECK_ERROR_END
}

DLLEXPORT void
DllWriteParametersToFile(void *DataSetPtr, char *Filename)
{
	CHECK_ERROR_BEGIN
	
	WriteParametersToFile((mobius_data_set *)DataSetPtr, (const char *)Filename);
	
	CHECK_ERROR_END
}

DLLEXPORT void
DllSetInputSeries(void *DataSetPtr, char *Name, char **IndexNames, u64 IndexCount, double *InputData, u64 InputDataLength, bool AlignWithResults)
{
	CHECK_ERROR_BEGIN
	
	SetInputSeries((mobius_data_set *)DataSetPtr, Name, IndexNames, (size_t)IndexCount, InputData, (size_t)InputDataLength, AlignWithResults);
	
	CHECK_ERROR_END
}


//TODO: Some of the following should be wrapped into accessors in mobius_data_set.cpp, because we are creating too many dependencies on implementation details here:

DLLEXPORT u64
DllGetIndexSetsCount(void *DataSetPtr)
{
	mobius_data_set *DataSet = (mobius_data_set *)DataSetPtr;
	return DataSet->Model->FirstUnusedIndexSetHandle - 1;
}

DLLEXPORT void
DllGetIndexSets(void *DataSetPtr, const char **NamesOut)
{
	CHECK_ERROR_BEGIN
	
	mobius_data_set *DataSet = (mobius_data_set *)DataSetPtr;
	const mobius_model *Model = DataSet->Model;
	for(entity_handle IndexSetHandle = 1; IndexSetHandle < Model->FirstUnusedIndexSetHandle; ++IndexSetHandle)
	{
		NamesOut[IndexSetHandle - 1] = GetName(Model, index_set_h {IndexSetHandle});
	}
	
	CHECK_ERROR_END
}

DLLEXPORT u64
DllGetIndexCount(void *DataSetPtr, char *IndexSetName)
{
	CHECK_ERROR_BEGIN
	
	mobius_data_set *DataSet = (mobius_data_set *)DataSetPtr;
	index_set_h IndexSet = GetIndexSetHandle(DataSet->Model, IndexSetName);
	return DataSet->IndexCounts[IndexSet.Handle];
	
	CHECK_ERROR_END
	
	return 0;
}

DLLEXPORT void
DllGetIndexes(void *DataSetPtr, char *IndexSetName, const char **NamesOut)
{
	CHECK_ERROR_BEGIN
	
	mobius_data_set *DataSet = (mobius_data_set *)DataSetPtr;
	index_set_h IndexSet = GetIndexSetHandle(DataSet->Model, IndexSetName);
	for(size_t IdxIdx = 0; IdxIdx < DataSet->IndexCounts[IndexSet.Handle]; ++IdxIdx)
	{
		NamesOut[IdxIdx] = DataSet->IndexNames[IndexSet.Handle][IdxIdx];
	}
	
	CHECK_ERROR_END
}

DLLEXPORT u64
DllGetParameterIndexSetsCount(void *DataSetPtr, char *ParameterName)
{
	CHECK_ERROR_BEGIN
	
	mobius_data_set *DataSet = (mobius_data_set *)DataSetPtr;
	entity_handle ParameterHandle = GetParameterHandle(DataSet->Model, ParameterName);
	size_t UnitIndex = DataSet->ParameterStorageStructure.UnitForHandle[ParameterHandle];
	return DataSet->ParameterStorageStructure.Units[UnitIndex].IndexSets.size();
	
	CHECK_ERROR_END
	
	return 0;
}

DLLEXPORT void
DllGetParameterIndexSets(void *DataSetPtr, char *ParameterName, const char **NamesOut)
{
	CHECK_ERROR_BEGIN
	
	mobius_data_set *DataSet = (mobius_data_set *)DataSetPtr;
	entity_handle ParameterHandle = GetParameterHandle(DataSet->Model, ParameterName);
	size_t UnitIndex = DataSet->ParameterStorageStructure.UnitForHandle[ParameterHandle];
	size_t Idx = 0;
	for(index_set_h IndexSet : DataSet->ParameterStorageStructure.Units[UnitIndex].IndexSets)
	{
		NamesOut[Idx] = GetName(DataSet->Model, IndexSet);
		++Idx;
	}
	
	CHECK_ERROR_END
}

DLLEXPORT u64
DllGetResultIndexSetsCount(void *DataSetPtr, char *ResultName)
{
	CHECK_ERROR_BEGIN
	
	mobius_data_set *DataSet = (mobius_data_set *)DataSetPtr;
	equation_h Equation = GetEquationHandle(DataSet->Model, ResultName);
	size_t UnitIndex = DataSet->ResultStorageStructure.UnitForHandle[Equation.Handle];
	return DataSet->ResultStorageStructure.Units[UnitIndex].IndexSets.size();
	
	CHECK_ERROR_END
	
	return 0;
}

DLLEXPORT void
DllGetResultIndexSets(void *DataSetPtr, char *ResultName, const char **NamesOut)
{
	CHECK_ERROR_BEGIN
	
	mobius_data_set *DataSet = (mobius_data_set *)DataSetPtr;
	equation_h Equation = GetEquationHandle(DataSet->Model, ResultName);
	size_t UnitIndex = DataSet->ResultStorageStructure.UnitForHandle[Equation.Handle];
	size_t Idx = 0;
	for(index_set_h IndexSet : DataSet->ResultStorageStructure.Units[UnitIndex].IndexSets)
	{
		NamesOut[Idx] = GetName(DataSet->Model, IndexSet);
		++Idx;
	}
	
	CHECK_ERROR_END
}

DLLEXPORT u64
DllGetInputIndexSetsCount(void *DataSetPtr, char *InputName)
{
	CHECK_ERROR_BEGIN
	
	mobius_data_set *DataSet = (mobius_data_set *)DataSetPtr;
	input_h Input = GetInputHandle(DataSet->Model, InputName);
	size_t UnitIndex = DataSet->ResultStorageStructure.UnitForHandle[Input.Handle];
	return DataSet->InputStorageStructure.Units[UnitIndex].IndexSets.size();
	
	CHECK_ERROR_END
	
	return 0;
}

DLLEXPORT void
DllGetInputIndexSets(void *DataSetPtr, char *InputName, const char **NamesOut)
{
	CHECK_ERROR_BEGIN
	
	mobius_data_set *DataSet = (mobius_data_set *)DataSetPtr;
	input_h Input = GetInputHandle(DataSet->Model, InputName);
	size_t UnitIndex = DataSet->ResultStorageStructure.UnitForHandle[Input.Handle];
	size_t Idx = 0;
	for(index_set_h IndexSet : DataSet->InputStorageStructure.Units[UnitIndex].IndexSets)
	{
		NamesOut[Idx] = GetName(DataSet->Model, IndexSet);
		++Idx;
	}
	
	CHECK_ERROR_END
}

DLLEXPORT u64
DllGetParameterGroupIndexSetsCount(void *DataSetPtr, char *ParameterGroupName)
{
	CHECK_ERROR_BEGIN
	
	mobius_data_set *DataSet = (mobius_data_set *)DataSetPtr;
	parameter_group_h ParameterGroup = GetParameterGroupHandle(DataSet->Model, ParameterGroupName);
	u64 Count = 0;
	
	while(IsValid(ParameterGroup))
	{
		const parameter_group_spec &Spec = DataSet->Model->ParameterGroupSpecs[ParameterGroup.Handle];
		if(IsValid(Spec.IndexSet)) ++Count;
		ParameterGroup = Spec.ParentGroup;
	}
	
	return Count;
	
	CHECK_ERROR_END
	
	return 0;
}

DLLEXPORT void
DllGetParameterGroupIndexSets(void *DataSetPtr, char *ParameterGroupName, const char **NamesOut)
{
	CHECK_ERROR_BEGIN
	
	mobius_data_set *DataSet = (mobius_data_set *)DataSetPtr;
	parameter_group_h ParameterGroup = GetParameterGroupHandle(DataSet->Model, ParameterGroupName);
	
	size_t Idx = 0;
	while(IsValid(ParameterGroup))
	{
		const parameter_group_spec &Spec = DataSet->Model->ParameterGroupSpecs[ParameterGroup.Handle];
		if(IsValid(Spec.IndexSet))
		{
			NamesOut[Idx] = GetName(DataSet->Model, Spec.IndexSet);
			++Idx;
		}
		ParameterGroup = Spec.ParentGroup;
	}
	
	CHECK_ERROR_END
}

DLLEXPORT u64
DllGetAllParameterGroupsCount(void *DataSetPtr, const char *ParentGroupName)
{
	CHECK_ERROR_BEGIN
	
	mobius_data_set *DataSet = (mobius_data_set *)DataSetPtr;
	const mobius_model *Model = DataSet->Model;
	
	parameter_group_h ParentGroup = {0};
	if(ParentGroupName && strlen(ParentGroupName) > 0)
	{
		ParentGroup = GetParameterGroupHandle(Model, ParentGroupName);
	}
	
	u64 Count = 0;
	
	for(entity_handle ParameterGroupHandle = 1; ParameterGroupHandle < Model->FirstUnusedParameterGroupHandle; ++ParameterGroupHandle)
	{
		const parameter_group_spec &Spec = Model->ParameterGroupSpecs[ParameterGroupHandle];
		if(!IsValid(ParentGroup) || ParentGroup == Spec.ParentGroup) ++Count;
	}
	
	return Count;
	
	CHECK_ERROR_END
	
	return 0;
}

DLLEXPORT void
DllGetAllParameterGroups(void *DataSetPtr, const char **NamesOut, const char *ParentGroupName)
{
	CHECK_ERROR_BEGIN
	
	mobius_data_set *DataSet = (mobius_data_set *)DataSetPtr;
	const mobius_model *Model = DataSet->Model;
	
	parameter_group_h ParentGroup = {0};
	if(ParentGroupName && strlen(ParentGroupName) > 0)
	{
		ParentGroup = GetParameterGroupHandle(Model, ParentGroupName);
	}
	
	size_t Idx = 0;
	for(entity_handle ParameterGroupHandle = 1; ParameterGroupHandle < Model->FirstUnusedParameterGroupHandle; ++ParameterGroupHandle)
	{
		const parameter_group_spec &Spec = Model->ParameterGroupSpecs[ParameterGroupHandle];
		if(!IsValid(ParentGroup) || ParentGroup == Spec.ParentGroup)
		{
			NamesOut[Idx] = Spec.Name;
			++Idx;
		}
	}
	
	CHECK_ERROR_END

}

DLLEXPORT u64
DllGetAllParametersCount(void *DataSetPtr, const char *GroupName)
{
	CHECK_ERROR_BEGIN
	
	mobius_data_set *DataSet = (mobius_data_set *)DataSetPtr;
	const mobius_model *Model = DataSet->Model;
	
	parameter_group_h Group = {0};
	if(GroupName && strlen(GroupName) > 0)
	{
		Group = GetParameterGroupHandle(Model, GroupName);
	}
	
	u64 Count = 0;
	for(entity_handle ParameterHandle = 1; ParameterHandle < Model->FirstUnusedParameterHandle; ++ParameterHandle)
	{
		const parameter_spec &Spec = Model->ParameterSpecs[ParameterHandle];
		if(!IsValid(Group) || Group == Spec.Group) ++Count;
	}
	
	return Count;
	
	CHECK_ERROR_END
	
	return 0;
}

DLLEXPORT void
DllGetAllParameters(void *DataSetPtr, const char **NamesOut, const char **TypesOut, const char *GroupName)
{
	CHECK_ERROR_BEGIN
	
	mobius_data_set *DataSet = (mobius_data_set *)DataSetPtr;
	const mobius_model *Model = DataSet->Model;
	
	parameter_group_h Group = {0};
	if(GroupName && strlen(GroupName) > 0)
	{
		Group = GetParameterGroupHandle(Model, GroupName);
	}
	
	size_t Idx = 0;
	for(entity_handle ParameterHandle = 1; ParameterHandle < Model->FirstUnusedParameterHandle; ++ParameterHandle)
	{
		const parameter_spec &Spec = DataSet->Model->ParameterSpecs[ParameterHandle];
		if(!IsValid(Group) || Group == Spec.Group)
		{
			NamesOut[Idx] = Spec.Name;
			TypesOut[Idx] = GetParameterTypeName(Spec.Type);
			
			++Idx;
		}
	}
	
	CHECK_ERROR_END
}

DLLEXPORT u64
DllGetAllResultsCount(void *DataSetPtr)
{
	CHECK_ERROR_BEGIN
	
	mobius_data_set *DataSet = (mobius_data_set *)DataSetPtr;
	return (u64)(DataSet->Model->FirstUnusedEquationHandle - 1);
	
	CHECK_ERROR_END
	
	return 0;
}

DLLEXPORT void
DllGetAllResults(void *DataSetPtr, const char **NamesOut, const char **TypesOut)
{
	CHECK_ERROR_BEGIN
	
	mobius_data_set *DataSet = (mobius_data_set *)DataSetPtr;
	for(size_t Idx = 0; Idx < DataSet->Model->FirstUnusedEquationHandle - 1; ++Idx)
	{
		entity_handle Handle = Idx + 1;
		const equation_spec &Spec = DataSet->Model->EquationSpecs[Handle];
		NamesOut[Idx] = Spec.Name;
		TypesOut[Idx] = GetEquationTypeName(Spec.Type);
	}
	
	CHECK_ERROR_END
}

DLLEXPORT u64
DllGetAllInputsCount(void *DataSetPtr)
{
	CHECK_ERROR_BEGIN
	
	mobius_data_set *DataSet = (mobius_data_set *)DataSetPtr;
	return (u64)(DataSet->Model->FirstUnusedInputHandle - 1);
	
	CHECK_ERROR_END
	
	return 0;
}

DLLEXPORT void
DllGetAllInputs(void *DataSetPtr, const char **NamesOut, const char **TypesOut)
{
	CHECK_ERROR_BEGIN
	
	mobius_data_set *DataSet = (mobius_data_set *)DataSetPtr;
	for(size_t Idx = 0; Idx < DataSet->Model->FirstUnusedInputHandle - 1; ++Idx)
	{
		entity_handle Handle = Idx + 1;
		const input_spec &Spec = DataSet->Model->InputSpecs[Handle];
		NamesOut[Idx] = Spec.Name;
		TypesOut[Idx] = Spec.IsAdditional ? "additional" : "required";
	}
	
	CHECK_ERROR_END
}

DLLEXPORT bool
DllInputWasProvided(void *DataSetPtr, const char *Name, char **IndexNames, u64 IndexCount)
{
	CHECK_ERROR_BEGIN
	
	return InputSeriesWasProvided((mobius_data_set *)DataSetPtr, Name, IndexNames, (size_t)IndexCount);
	
	CHECK_ERROR_END
	
	return false;
}

DLLEXPORT u64
DllGetBranchInputsCount(void *DataSetPtr, const char *IndexSetName, const char *IndexName)
{
	CHECK_ERROR_BEGIN
	
	mobius_data_set *DataSet = (mobius_data_set *)DataSetPtr;
	index_set_h IndexSet = GetIndexSetHandle(DataSet->Model, IndexSetName);
	
	const index_set_spec &Spec = DataSet->Model->IndexSetSpecs[IndexSet.Handle];
	
	if(Spec.Type != IndexSetType_Branched)
	{
		MOBIUS_FATAL_ERROR("Tried to read branch inputs from the index set " << IndexSetName << ", but that is not a branched index set." << std::endl);
	}
	
	index_t Index = GetIndex(DataSet, IndexSet, IndexName);
	
	return (u64)DataSet->BranchInputs[IndexSet.Handle][Index].Count;
	
	CHECK_ERROR_END
	
	return 0;
}

DLLEXPORT void
DllGetBranchInputs(void *DataSetPtr, const char *IndexSetName, const char *IndexName, const char **BranchInputsOut)
{
	CHECK_ERROR_BEGIN
	
	mobius_data_set *DataSet = (mobius_data_set *)DataSetPtr;
	index_set_h IndexSet = GetIndexSetHandle(DataSet->Model, IndexSetName);
	
	index_t Index = GetIndex(DataSet, IndexSet, IndexName);
	
	size_t Count = DataSet->BranchInputs[IndexSet.Handle][Index].Count;
	for(size_t Idx = 0; Idx < Count; ++Idx)
	{
		index_t IdxIdx = DataSet->BranchInputs[IndexSet.Handle][Index].Inputs[Idx];
		BranchInputsOut[Idx] = DataSet->IndexNames[IndexSet.Handle][IdxIdx];
	}
	
	CHECK_ERROR_END
}

