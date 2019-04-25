


//NOTE: Just starting to sketch out some parts of the model. It is far from finished.

//This particular implementation follows L.A.Jackson-Blake &al 2016


inline double
EPC0Computation(double SorptionCoefficient, double SolidPhosphorousMass, double SoilMass, double FreundlichIsothermConstant, double TDPConcentration)
{
	if(SorptionCoefficient > 0.0 && SolidPhosphorousMass > 0.0 && TDPConcentration > 0.0)
	{
		return pow(1e6 * SolidPhosphorousMass / (SorptionCoefficient * SoilMass), FreundlichIsothermConstant);
	}
	return TDPConcentration;
}

inline double
SorptionComputation(double SorptionScalingFactor, double TDPConcentration, double EPC0, double FreundlichIsothermConstant, double WaterVolume)
{
	double inviso = 1.0 / FreundlichIsothermConstant;
	return 1e-3 * SorptionScalingFactor * (pow(TDPConcentration, inviso) - pow(EPC0, inviso)) * WaterVolume;
}




static void
AddINCAPModel(mobius_model *Model)
{
	auto Dimensionless = RegisterUnit(Model);
	auto GCPerM2       = RegisterUnit(Model, "gC / m^2");
	auto GCPerM2PerDay = RegisterUnit(Model, "gC / m^2 / day");
	auto KgPerKm2      = RegisterUnit(Model, "kg/km^2");
	auto KgPerHaPerDay = RegisterUnit(Model, "kg/Ha/day");
	auto KgPerHaPerYear = RegisterUnit(Model, "kg/Ha/year");
	auto KgPerKm2PerDay = RegisterUnit(Model, "kg/km^2/day");
	auto KgPerDay       = RegisterUnit(Model, "kg/day");
	auto PerDay         = RegisterUnit(Model, "1/day");
	auto KgPerKg       = RegisterUnit(Model, "kg/kg");
	auto KgPerHa       = RegisterUnit(Model, "kg/Ha");
	auto JulianDay     = RegisterUnit(Model, "Julian day");
	auto Days          = RegisterUnit(Model, "days");
	auto M             = RegisterUnit(Model, "m");
	auto MPerDay       = RegisterUnit(Model, "m/day");
	auto DegreesCelsius     = RegisterUnit(Model, "°C");
	auto MgPerL        = RegisterUnit(Model, "mg/l");
	auto M3PerKm2      = RegisterUnit(Model, "m^3/km^2");
	auto TonnesPerM2   = RegisterUnit(Model, "10^3 kg/m^2");
	auto M2PerGCPerDay = RegisterUnit(Model, "m^2 /gC/day");
	auto SPerM3PerDay  = RegisterUnit(Model, "s/m^3/day");
	auto SPerMPerGCPerDay = RegisterUnit(Model, "s/m/gC/day");
	auto Kg            = RegisterUnit(Model, "kg");
	auto PerKg         = RegisterUnit(Model, "1/kg");
	
	auto IncaSolver = RegisterSolver(Model, "Inca solver", 0.1, IncaDascru);
	
	auto Soils = GetIndexSetHandle(Model, "Soils");
	
	auto DirectRunoff = RequireIndex(Model, Soils, "Direct runoff");
	auto Soilwater    = RequireIndex(Model, Soils, "Soil water");
	auto Groundwater  = RequireIndex(Model, Soils, "Groundwater");
	
	//NOTE: These are from PERSiST:
	auto WaterDepth3           = GetEquationHandle(Model, "Water depth 3"); //NOTE: This is right before percolation and runoff is subtracted.
	auto WaterDepth            = GetEquationHandle(Model, "Water depth");   //NOTE: This is after everything is subtracted.
	auto RunoffToReach         = GetEquationHandle(Model, "Runoff to reach");
	auto SaturationExcessInput = GetEquationHandle(Model, "Saturation excess input");
	auto PercolationInput      = GetEquationHandle(Model, "Percolation input");
	auto PercolationOut        = GetEquationHandle(Model, "Percolation out");
	
	
	auto SoilwaterVolume             = RegisterEquation(Model, "Soil water volume", M3PerKm2);
	auto GroundwaterVolume           = RegisterEquation(Model, "Groundwater volume", M3PerKm2);
	auto DirectRunoffToReachFraction = RegisterEquation(Model, "Direct runoff to reach fraction", PerDay);
	auto DirectRunoffToSoilFraction  = RegisterEquation(Model, "Direct runoff to soil fraction", PerDay);
	auto SoilToDirectRunoffFraction  = RegisterEquation(Model, "Soil to direct runoff fraction", PerDay);
	auto SoilToGroundwaterFraction   = RegisterEquation(Model, "Soil to groundwater fraction", PerDay);
	auto SoilToReachFraction         = RegisterEquation(Model, "Soil to reach fraction", PerDay);
	auto GroundwaterToReachFraction  = RegisterEquation(Model, "Groundwater to reach fraction", PerDay);
	
	EQUATION(Model, SoilwaterVolume,
		return RESULT(WaterDepth, Soilwater) * 1000.0;
	)
	
	EQUATION(Model, GroundwaterVolume,
		return RESULT(WaterDepth, Groundwater) * 1000.0;
	)
	
	EQUATION(Model, DirectRunoffToReachFraction,
		return SafeDivide(RESULT(RunoffToReach, DirectRunoff), RESULT(WaterDepth3, DirectRunoff));
	)
	
	EQUATION(Model, DirectRunoffToSoilFraction,
		return SafeDivide(RESULT(PercolationInput, Soilwater), RESULT(WaterDepth3, DirectRunoff));
	)
	
	EQUATION(Model, SoilToDirectRunoffFraction,
		return SafeDivide(RESULT(SaturationExcessInput, DirectRunoff), RESULT(WaterDepth3, Soilwater));
	)
	
	EQUATION(Model, SoilToReachFraction,
		return SafeDivide(RESULT(RunoffToReach, Soilwater), RESULT(WaterDepth3, Soilwater));
	)
	
	EQUATION(Model, SoilToGroundwaterFraction,
		return SafeDivide(RESULT(PercolationInput, Groundwater), RESULT(WaterDepth3, Soilwater));
	)

	EQUATION(Model, GroundwaterToReachFraction,
		return SafeDivide(RESULT(RunoffToReach, Groundwater), RESULT(WaterDepth3, Groundwater));
	)
	
	
	auto Land = GetParameterGroupHandle(Model, "Landscape units");
	
	
	//TODO: As always, find good default/min/max values and descriptions for parameters.
	
	auto LiquidManurePTimeseries     = RegisterInput(Model, "Liquid manure P", KgPerHaPerDay);
	auto LiquidFertilizerPTimeseries = RegisterInput(Model, "Liquid fertilizer P", KgPerHaPerDay);
	auto SolidManurePTimeseries      = RegisterInput(Model, "Solid manure P", KgPerHaPerDay);
	auto SolidFertilizerPTimeseries  = RegisterInput(Model, "Solid fertilizer P", KgPerHaPerDay);
	auto PlantResiduePTimeseries     = RegisterInput(Model, "Plant residue P", KgPerHaPerDay);
	auto PhosphorousDryDepositionTimeseries = RegisterInput(Model, "P dry deposition", KgPerHaPerDay);
	auto PhosphorousWetDepositionTimeseries = RegisterInput(Model, "P wet deposition", KgPerHaPerDay);
	
	auto LiquidManureP = RegisterParameterDouble(Model, Land, "Liquid manure P inputs", KgPerHaPerDay, 0.0, 0.0, 100.0, "Inputs of phosphorous to soil through liquid manure. Only used if the timeseries of the same name is not provided.");
	auto LiquidFertilizerP = RegisterParameterDouble(Model, Land, "Liquid fertilizer P inputs", KgPerHaPerDay, 0.0, 0.0, 100.0, "Inputs of phosphorous to soil through liquid fertilizer. Only used if the timeseries of the same name is not provided.");
	auto SolidManureP  = RegisterParameterDouble(Model, Land, "Solid manure P inputs", KgPerHaPerDay, 0.0);
	auto SolidFertilizerP = RegisterParameterDouble(Model, Land, "Solid fertilizer P inputs", KgPerHaPerDay, 0.0);
	auto PlantResidueP = RegisterParameterDouble(Model, Land, "Plant residue P inputs", KgPerHaPerDay, 0.0);
	auto PhosphorousDryDeposition = RegisterParameterDouble(Model, Land, "P dry deposition", KgPerHaPerDay, 0.0);
	auto PhosphorousWetDepositionPar = RegisterParameterDouble(Model, Land, "P wet deposition", KgPerHaPerDay, 0.0); //TODO: Should be computed differently!
	
	auto SoilSorptionScalingFactor = RegisterParameterDouble(Model, Land, "Soil sorption scaling factor", PerDay, 1.0);
	auto SoilPSorptionCoefficient = RegisterParameterDouble(Model, Land, "Soil P sorption coefficient", KgPerKg, 1.0);
	auto SoilFreundlichIsothermConstant = RegisterParameterDouble(Model, Land, "Soil Freundlich isotherm constant", Dimensionless, 1.0);
	auto FertilizerAdditionStartDay = RegisterParameterUInt(Model, Land, "Fertilizer addition start day", JulianDay, 20, 0, 365);
	auto FertilizerAdditionPeriod   = RegisterParameterUInt(Model, Land, "Fertilizer addition period", Days, 20, 0, 365);
	auto GrowingSeasonStart = RegisterParameterUInt(Model, Land, "Growing season start day", JulianDay, 20, 0, 365);
	auto GrowingSeasonPeriod = RegisterParameterUInt(Model, Land, "Growing season period", Days, 20, 0, 365);
	auto PlantGrowthCurveAmplitude = RegisterParameterDouble(Model, Land, "Plant growth curve amplitude", Dimensionless, 1.0, 0.0, 1.0);
	auto PlantGrowthCurveOffset    = RegisterParameterDouble(Model, Land, "Plant growth curve offset", Dimensionless, 0.0, 0.0, 1.0);
	auto PlantPUptakeFactor = RegisterParameterDouble(Model, Land, "Plant P uptake factor", MPerDay, 1.0);
	auto MaxDailyPUptake = RegisterParameterDouble(Model, Land, "Maximum daily plant P uptake", KgPerHaPerDay, 100.0);
	auto MaxAnnualPUptake = RegisterParameterDouble(Model, Land, "Maximum annual plant P uptake", KgPerHaPerYear, 100.0);
	auto MaxSoilLabilePContent = RegisterParameterDouble(Model, Land, "Maximum soil labile P content", KgPerKg, 1.0, 0.0, 1.0);
	auto WeatheringFactor = RegisterParameterDouble(Model, Land, "Weathering factor", PerDay, 1.0);
	auto ChemicalImmobilisationFactor = RegisterParameterDouble(Model, Land, "Chemical immobilisation factor", PerDay, 1.0);
	auto ChangeInRateWithA10DegreeChangeInTemperature = RegisterParameterDouble(Model, Land, "Change in rate with a 10 degree change in temperature", Dimensionless, 1.0, 0.0, 5.0);
	auto TemperatureAtWhichResponseIs1 = RegisterParameterDouble(Model, Land, "Temperature at which response is 1", DegreesCelsius, 20.0);
	auto LowerTemperatureThresholdImmobilisation = RegisterParameterDouble(Model, Land, "Lower temperature threshold for immobilisation", DegreesCelsius, 0.0);
	
	//TODO: Groundwater parameters should maybe not be per landscape unit.
	auto GroundwaterSorptionScalingFactor = RegisterParameterDouble(Model, Land, "Groundwater sorption scaling factor", PerDay, 1.0);
	auto GroundwaterPSorptionCoefficient = RegisterParameterDouble(Model, Land, "Groundwater P sorption coefficient", KgPerKg, 1.0);
	auto GroundwaterFreundlichIsothermConstant = RegisterParameterDouble(Model, Land, "Groundwater Freundlich isotherm constant", Dimensionless, 1.0);
	auto AquiferMass = RegisterParameterDouble(Model, Land, "Aquifer mass", TonnesPerM2, 1.0);
	auto AquiferMatrixPSaturation = RegisterParameterDouble(Model, Land, "Aquifer matrix P saturation", MgPerL, 20.0);
	
	auto PhosphorousWetDeposition      = RegisterEquation(Model, "Fertilizer wet deposition", KgPerHaPerDay);
	auto LiquidPInputsToSoil           = RegisterEquation(Model, "Liquid P inputs to soil", KgPerHaPerDay);
	auto SolidPInputsToSoil            = RegisterEquation(Model, "Solid P inputs to soil", KgPerHaPerDay);
	auto TemperatureFactor             = RegisterEquation(Model, "Temperature factor", Dimensionless);
	auto SoilMoistureFactor            = RegisterEquation(Model, "Soil moisture factor", Dimensionless);
	auto SeasonalPlantGrowthIndex      = RegisterEquation(Model, "Seasonal plant growth index", Dimensionless);
	
	auto SoilwaterEPC0                 = RegisterEquation(Model, "SoilwaterEPC0", MgPerL);
	SetSolver(Model, SoilwaterEPC0, IncaSolver);
	auto SoilPSorptionDesorption       = RegisterEquation(Model, "Soil P sorption/desorption", KgPerKm2PerDay);
	SetSolver(Model, SoilPSorptionDesorption, IncaSolver);
	auto PlantPUptakeFromSoilwater     = RegisterEquation(Model, "Plant P uptake from soilwater", KgPerKm2PerDay);
	SetSolver(Model, PlantPUptakeFromSoilwater, IncaSolver);
	auto ChemicalImmobilisation        = RegisterEquation(Model, "Chemical immobilisation", KgPerKm2PerDay);
	SetSolver(Model, ChemicalImmobilisation, IncaSolver);
	auto SoilwaterTDPMass              = RegisterEquationODE(Model, "Soil water TDP mass", KgPerKm2);
	SetSolver(Model, SoilwaterTDPMass, IncaSolver);
	//SetInitialValue
	auto SoilLabilePMass               = RegisterEquationODE(Model, "Soil labile P mass", KgPerKm2);
	SetSolver(Model, SoilLabilePMass, IncaSolver);
	//SetInitialValue
	auto SoilInactivePMass             = RegisterEquationODE(Model, "Soil inactive P mass", KgPerKm2);
	SetSolver(Model, SoilInactivePMass, IncaSolver);
	//SetInitialValue
	auto SoilwaterTDPConcentration     = RegisterEquation(Model, "Soil water TDP concentration", MgPerL);
	SetSolver(Model, SoilwaterTDPConcentration, IncaSolver);
	auto DirectRunoffTDPMass           = RegisterEquationODE(Model, "Direct runoff TDP mass", KgPerKm2);
	SetSolver(Model, DirectRunoffTDPMass, IncaSolver);
	//SetInitialValue
	auto GroundwaterEPC0               = RegisterEquation(Model, "Groundwater EPC0", MgPerL);
	SetSolver(Model, GroundwaterEPC0, IncaSolver);
	auto GroundwaterPSorptionDesorption = RegisterEquation(Model, "Groundwater P sorption/desorption", KgPerKm2PerDay);
	SetSolver(Model, GroundwaterPSorptionDesorption, IncaSolver);
	auto GroundwaterTDPMass            = RegisterEquationODE(Model, "Groundwater TDP mass", KgPerKm2);
	SetSolver(Model, GroundwaterTDPMass, IncaSolver);
	//SetInitialValue
	auto PMassInTheAquiferMatrix       = RegisterEquationODE(Model, "P mass in the aquifer matrix", KgPerKm2);
	SetSolver(Model, PMassInTheAquiferMatrix, IncaSolver);
	//SetInitialValue
	auto GroundwaterTDPConcentration   = RegisterEquation(Model, "Groundwater TDP concentration", KgPerKm2);
	SetSolver(Model, GroundwaterTDPConcentration, IncaSolver);
	
	auto SoilPControl                  = RegisterEquation(Model, "Soil P control", Dimensionless);
	auto GroundwaterPControl           = RegisterEquation(Model, "Groundwater P control", Dimensionless);
	auto AccumulatedAnnualPUptake      = RegisterEquation(Model, "Accumulated annual P uptake", KgPerHa);
	
	auto SoilMassInTheOAHorizon  = GetEquationHandle(Model, "Soil mass in the O/A horizon"); //From IncaSed.h
	auto SedimentDeliveryToReach = GetEquationHandle(Model, "Sediment delivery to reach"); //From IncaSed.h
	auto SoilTemperature         = GetEquationHandle(Model, "Soil temperature"); //From SoilTemperature.h
	
	auto AirTemperature = GetInputHandle(Model, "Air temperature");
	
	
	EQUATION(Model, PhosphorousWetDeposition,
		//TODO: If there is no timeseries, it should be computed in terms of annual averages instead.
		return IF_INPUT_ELSE_PARAMETER(PhosphorousWetDepositionTimeseries, PhosphorousWetDepositionPar);
	)
	
	EQUATION(Model, LiquidPInputsToSoil,
		double Pliqin = RESULT(PhosphorousWetDeposition);
		double Pliqmanure = IF_INPUT_ELSE_PARAMETER(LiquidManurePTimeseries, LiquidManureP);
		double Pliqfert   = IF_INPUT_ELSE_PARAMETER(LiquidFertilizerPTimeseries, LiquidFertilizerP);
		
		u64 Fstart  = PARAMETER(FertilizerAdditionStartDay);
		u64 Fperiod = PARAMETER(FertilizerAdditionPeriod);
		u64 Day = CURRENT_DAY_OF_YEAR();
		
		if(Day >= Fstart && Day <= Fstart + Fperiod)
		{
			Pliqin += Pliqmanure + Pliqfert;
		}
		
		return Pliqin;
	)
	
	EQUATION(Model, SolidPInputsToSoil,
		double Psolidmanure = IF_INPUT_ELSE_PARAMETER(SolidManurePTimeseries, SolidManureP);
		double Psolidfert   = IF_INPUT_ELSE_PARAMETER(SolidFertilizerPTimeseries, SolidFertilizerP);
		double Presidue     = IF_INPUT_ELSE_PARAMETER(PlantResiduePTimeseries, PlantResidueP);
		double Pdep         = IF_INPUT_ELSE_PARAMETER(PhosphorousDryDepositionTimeseries, PhosphorousDryDeposition) / (double)DAYS_THIS_YEAR();
		
		double Psolidin = Presidue + Pdep;
		
		u64 Fstart  = PARAMETER(FertilizerAdditionStartDay);
		u64 Fperiod = PARAMETER(FertilizerAdditionPeriod);
		u64 Day = CURRENT_DAY_OF_YEAR();
		
		if(Day >= Fstart && Day <= Fstart + Fperiod)
		{
			Psolidin += Psolidmanure + Psolidfert;
		}
		
		return Psolidin;
	)
	
	EQUATION(Model, SoilwaterEPC0,
		return 
			EPC0Computation(
				PARAMETER(SoilPSorptionCoefficient), 
				RESULT(SoilLabilePMass), 
				RESULT(SoilMassInTheOAHorizon), 
				PARAMETER(SoilFreundlichIsothermConstant), 
				RESULT(SoilwaterTDPConcentration)
				);
	)
	
	EQUATION(Model, SoilPSorptionDesorption,
		return
			SorptionComputation(
				PARAMETER(SoilSorptionScalingFactor),
				RESULT(SoilwaterTDPConcentration),
				RESULT(SoilwaterEPC0),
				PARAMETER(SoilFreundlichIsothermConstant),
				RESULT(SoilwaterVolume)
				);
	)
	
	EQUATION(Model, SoilMoistureFactor,
		return 1.0;
		//TODO!!!
	)
	
	EQUATION(Model, TemperatureFactor,
		return pow(PARAMETER(ChangeInRateWithA10DegreeChangeInTemperature), (RESULT(SoilTemperature) - PARAMETER(TemperatureAtWhichResponseIs1)) / 10.0);
	)
	
	EQUATION(Model, SeasonalPlantGrowthIndex,
		double Day = (double)CURRENT_DAY_OF_YEAR();
		double DayOffset = (double)PARAMETER(GrowingSeasonStart);
		double DayCount  = (double)DAYS_THIS_YEAR();
		return PARAMETER(PlantGrowthCurveOffset) + PARAMETER(PlantGrowthCurveAmplitude)*sin(2.0*Pi*(Day - DayOffset) / DayCount);
	)
	
	EQUATION(Model, PlantPUptakeFromSoilwater,
		double tdpconc = SafeDivide(RESULT(SoilwaterTDPMass), RESULT(SoilwaterVolume));
		double uptake = 1e6 * PARAMETER(PlantPUptakeFactor) * RESULT(TemperatureFactor) * RESULT(SoilMoistureFactor) * tdpconc;
		uptake = Min(uptake, 100.0 * PARAMETER(MaxDailyPUptake));
		if(RESULT(AccumulatedAnnualPUptake) > PARAMETER(MaxAnnualPUptake))
		{
			uptake = 0.0;
		}
		return uptake;
	)
	
	EQUATION(Model, AccumulatedAnnualPUptake,
		double acc = LAST_RESULT(AccumulatedAnnualPUptake) + LAST_RESULT(PlantPUptakeFromSoilwater) / 100.0;
		if(CURRENT_DAY_OF_YEAR() == 1) acc = 0.0;
		return acc;
	)
	
	EQUATION(Model, SoilwaterTDPMass,
		return
			  100.0 * RESULT(LiquidPInputsToSoil) 
			- RESULT(SoilPSorptionDesorption) 
			- RESULT(PlantPUptakeFromSoilwater)
			
			- RESULT(SoilwaterTDPMass) * (RESULT(SoilToDirectRunoffFraction) + RESULT(SoilToGroundwaterFraction) + RESULT(SoilToReachFraction))
			+ RESULT(DirectRunoffTDPMass) * RESULT(DirectRunoffToSoilFraction);
	)
	
	EQUATION(Model, ChemicalImmobilisation,
		double Ctemp = RESULT(TemperatureFactor);
		if(INPUT(AirTemperature) <= PARAMETER(LowerTemperatureThresholdImmobilisation)) Ctemp = 0.0;
		return PARAMETER(ChemicalImmobilisationFactor) * Ctemp * RESULT(SoilLabilePMass);
	)
	
	EQUATION(Model, SoilLabilePMass,
		return
			  100.0 * RESULT(SolidPInputsToSoil)
			+ RESULT(SoilPSorptionDesorption)
			+ PARAMETER(WeatheringFactor) * RESULT(TemperatureFactor) * RESULT(SoilInactivePMass)
			- RESULT(SedimentDeliveryToReach) * SafeDivide(RESULT(SoilLabilePMass), RESULT(SoilMassInTheOAHorizon))
			- RESULT(ChemicalImmobilisation);
			
	)
	
	EQUATION(Model, SoilInactivePMass,
		return
			  RESULT(ChemicalImmobilisation)
			- PARAMETER(WeatheringFactor) * RESULT(TemperatureFactor) * RESULT(SoilInactivePMass)
			- RESULT(SedimentDeliveryToReach) * SafeDivide(RESULT(SoilInactivePMass), RESULT(SoilMassInTheOAHorizon));
	)
	
	EQUATION(Model, SoilPControl,
		
		//TODO: Make sure this is executed at the right place!!
	
		double Csatlabile = PARAMETER(MaxSoilLabilePContent);
		double Msoil      = RESULT(SoilMassInTheOAHorizon);
		double Plab       = RESULT(SoilLabilePMass);
		double tdp = LAST_RESULT(SoilwaterTDPMass) + Plab - Csatlabile*Msoil;
		
		if(SafeDivide(Plab, Msoil) >= Csatlabile)
		{
			SET_RESULT(SoilwaterTDPMass, tdp);
			SET_RESULT(SoilLabilePMass, Csatlabile*Msoil);
		}
		
		return 0.0;
	)
	
	
	EQUATION(Model, SoilwaterTDPConcentration,
		return 1000.0 * SafeDivide(RESULT(SoilwaterTDPMass), RESULT(SoilwaterVolume));
	)
	
	
	
	
	EQUATION(Model, GroundwaterEPC0,
		return 
			EPC0Computation(
				PARAMETER(GroundwaterPSorptionCoefficient), 
				RESULT(PMassInTheAquiferMatrix), 
				1e9*PARAMETER(AquiferMass), 
				PARAMETER(GroundwaterFreundlichIsothermConstant), 
				RESULT(GroundwaterTDPConcentration)
				);
	)
	
	EQUATION(Model, GroundwaterPSorptionDesorption,
		return
			SorptionComputation(
				PARAMETER(GroundwaterSorptionScalingFactor),
				RESULT(GroundwaterTDPConcentration),
				RESULT(GroundwaterEPC0),
				PARAMETER(GroundwaterFreundlichIsothermConstant),
				RESULT(GroundwaterVolume)
				);
	)
	
	EQUATION(Model, GroundwaterTDPMass,
		return
			- RESULT(GroundwaterPSorptionDesorption)
			
			- RESULT(GroundwaterTDPMass) * RESULT(GroundwaterToReachFraction)
			+ RESULT(SoilwaterTDPMass) * RESULT(SoilToGroundwaterFraction);
	)
	
	EQUATION(Model, PMassInTheAquiferMatrix,
		return RESULT(GroundwaterPSorptionDesorption);
	)
	
	EQUATION(Model, GroundwaterPControl,
		//TODO: Make sure this is executed at the right place.
	
		double Maquifer = 1e9*PARAMETER(AquiferMass);
		double Csataquifer = PARAMETER(AquiferMatrixPSaturation);
		double Psorbed = RESULT(PMassInTheAquiferMatrix);
		
		double tdp = LAST_RESULT(GroundwaterTDPMass) + Psorbed - Csataquifer*Maquifer;
		
		if(Psorbed/Maquifer >= Csataquifer)
		{
			SET_RESULT(GroundwaterTDPMass, tdp);
			SET_RESULT(PMassInTheAquiferMatrix, Csataquifer*Maquifer);
		}
		
		return 0.0;
	)
	
	EQUATION(Model, GroundwaterTDPConcentration,
		return 1000.0 * SafeDivide(RESULT(GroundwaterTDPMass), RESULT(GroundwaterVolume));
	)
	
	
	
	EQUATION(Model, DirectRunoffTDPMass,
		return 
			  RESULT(SoilwaterTDPMass) * RESULT(SoilToDirectRunoffFraction)
			- RESULT(DirectRunoffTDPMass) * (RESULT(DirectRunoffToSoilFraction) + RESULT(DirectRunoffToReachFraction));
	)
	
	
	
	
	auto Reaches = GetParameterGroupHandle(Model, "Reaches");
	
	auto LandscapeUnits = GetIndexSetHandle(Model, "Landscape units");
	auto Reach =          GetIndexSetHandle(Model, "Reaches");
	
	auto PPEnrichmentFactor = RegisterParameterDouble(Model, Reaches, "PP enrichment factor", Dimensionless, 1.0);
	auto ProportionOfPInEpiphytes = RegisterParameterDouble(Model, Reaches, "Proportion of P in epiphytes", KgPerKg, 0.2);
	auto ProportionOfPInMacrophytes = RegisterParameterDouble(Model, Reaches, "Proportion of P in macrophytes", KgPerKg, 0.2);
	auto EpiphyteGrowthRateCoefficient = RegisterParameterDouble(Model, Reaches, "Epiphyte growth rate coefficient", M2PerGCPerDay, 1.0);
	auto EpiphyteTemperatureDependency = RegisterParameterDouble(Model, Reaches, "Epiphyte temperature dependency", Dimensionless, 1.0);
	auto HalfSaturationOfPForEpiphyteGrowth = RegisterParameterDouble(Model, Reaches, "Half saturation of P for epiphyte growth", MgPerL, 1.0);
	auto EpiphyteDeathRateCoefficent = RegisterParameterDouble(Model, Reaches, "Epiphyte death rate coefficient", SPerM3PerDay, 1.0);
	auto MacrophyteGrowthRateCoefficient = RegisterParameterDouble(Model, Reaches, "Macrophyte growth rate coefficient", PerDay, 1.0);
	auto MacrophyteTemperatureDependency = RegisterParameterDouble(Model, Reaches, "Macrophyte temperature dependency", Dimensionless, 1.0);
	auto SelfShadingForMacrophytes = RegisterParameterDouble(Model, Reaches, "Self-shading for macrophytes", GCPerM2, 1.0);
	auto HalfSaturationOfPForMacrophyteGrowth = RegisterParameterDouble(Model, Reaches, "Half saturation of P for macrophyte growth", MgPerL, 1.0);
	auto MacrophyteDeathRateCoefficient = RegisterParameterDouble(Model, Reaches, "Macrophyte death rate coefficient", SPerMPerGCPerDay, 1.0);
	auto WaterColumnPSorptionCoefficient = RegisterParameterDouble(Model, Reaches, "Water column P sorption coefficient", PerKg, 0.1);
	auto WaterColumnFreundlichIsothermConstant = RegisterParameterDouble(Model, Reaches, "Water column Freundlich isotherm constant", Dimensionless, 1.0);
	auto WaterColumnSorptionScalingFactor = RegisterParameterDouble(Model, Reaches, "Water column sorption scaling factor", PerDay, 1.0);
	auto WaterColumnStreamBedTDPExchangeFraction = RegisterParameterDouble(Model, Reaches, "Water column stream bed TDP exchange fraction", Dimensionless, 0.1);
	auto PorewaterPSorptionCoefficient = RegisterParameterDouble(Model, Reaches, "Pore water P sorption coefficient", PerKg, 0.1);
	auto PorewaterSorptionScalingFactor = RegisterParameterDouble(Model, Reaches, "Pore water sorption scaling factor", PerDay, 1.0);
	auto PorewaterFreundlichIsothermConstant = RegisterParameterDouble(Model, Reaches, "Pore water Freundlich isotherm constant", Dimensionless, 1.0);
	auto BedSedimentDepth = RegisterParameterDouble(Model, Reaches, "Bed sediment depth", M, 1.0);
	auto BedSedimentPorosity = RegisterParameterDouble(Model, Reaches, "Bed sediment porosity", Dimensionless, 0.3);
	
	auto RatioOfHydrolysablePToDOC = RegisterParameterDouble(Model, Reaches, "Ration of hydrolysable P to DOC", KgPerKg, 1.0);
	auto RegressionBetweenTDPAndSRPConstant = RegisterParameterDouble(Model, Reaches, "Regression between TDP and SRP (constant)", Kg, 0.0);
	auto RegressionBetweenTDPAndSRPGradient = RegisterParameterDouble(Model, Reaches, "Regression between TDP and SRP (gradient)", KgPerKg, 0.0);
	
	
	auto DOCConcentration = RegisterInput(Model, "DOC concentration", MgPerL);
	
	
	auto SolarRadiation = GetEquationHandle(Model, "Solar radiation"); //NOTE: From SolarRadiation.h This one is not computed exactly as specified in published INCA-P, but similar.
	auto WaterTemperature = GetEquationHandle(Model, "Water temperature"); //NOTE: From WaterTemperature.h
	auto ReachFlow        = GetEquationHandle(Model, "Reach flow"); //NOTE: From Persist.h
	auto ReachVolume      = GetEquationHandle(Model, "Reach volume"); //From Persist.h
	auto TotalSuspendedSedimentMass = GetEquationHandle(Model, "Total suspended sediment mass"); //From IncaSed.h
	auto TotalBedSedimentMass = GetEquationHandle(Model, "Total bed sediment mass"); //From IncaSed.h
	auto TotalEntrainment = GetEquationHandle(Model, "Total sediment entrainment"); //From IncaSed.h
	auto TotalDeposition = GetEquationHandle(Model, "Total sediment deposition"); //From IncaSed.h
	
	auto SubcatchmentArea = GetParameterDoubleHandle(Model, "Terrestrial catchment area"); //From Persist.h
	auto Percent          = GetParameterDoubleHandle(Model, "%"); //From Persist.h
	auto ReachLength      = GetParameterDoubleHandle(Model, "Reach length"); //From Persist.h
	auto ReachWidth       = GetParameterDoubleHandle(Model, "Reach width"); //From Persist.h
	
	
	auto ReachPSolver = RegisterSolver(Model, "Reach P solver", 0.1, IncaDascru);
	
	auto ReachPPInputFromLand = RegisterEquation(Model, "Reach PP input from land", KgPerKm2PerDay);
	auto AreaScaledReachPPInputFromLand = RegisterEquation(Model, "Area scaled reach PP input from land", KgPerDay);
	auto TotalReachPPInputFromLand = RegisterEquationCumulative(Model, "Total reach PP input from land", AreaScaledReachPPInputFromLand, LandscapeUnits);
	auto ReachTDPInputFromLand = RegisterEquation(Model, "Reach TDP input from land", KgPerKm2PerDay);
	auto AreaScaledReachTDPInputFromLand = RegisterEquation(Model, "Area scaled reach TDP input from land", KgPerDay);
	auto TotalReachTDPInputFromLand = RegisterEquationCumulative(Model, "Total reach TDP input from land", AreaScaledReachTDPInputFromLand, LandscapeUnits);
	auto WaterColumnTDPInput = RegisterEquation(Model, "Water column TDP input", KgPerDay);
	auto WaterColumnPPInput  = RegisterEquation(Model, "Water column PP input", KgPerDay);
	
	
	auto WaterColumnTDPOutput = RegisterEquation(Model, "Water column TDP output", KgPerDay);
	SetSolver(Model, WaterColumnTDPOutput, ReachPSolver);
	auto WaterColumnPPOutput = RegisterEquation(Model, "Water column PP output", KgPerDay);
	SetSolver(Model, WaterColumnPPOutput, ReachPSolver);
	auto ReachTDPUptakeByEpiphytes = RegisterEquation(Model, "Reach TDP uptake by epiphytes", KgPerDay);
	SetSolver(Model, ReachTDPUptakeByEpiphytes, ReachPSolver);
	auto WaterColumnEPC0 = RegisterEquation(Model, "Water column EPC0", MgPerL);
	SetSolver(Model, WaterColumnEPC0, ReachPSolver);
	auto WaterColumnPSorptionDesorption = RegisterEquation(Model, "Water column sorption/desorption", KgPerDay);
	SetSolver(Model, WaterColumnPSorptionDesorption, ReachPSolver);
	auto WaterColumnTDPMass = RegisterEquationODE(Model, "Water column TDP mass", Kg);
	SetSolver(Model, WaterColumnTDPMass, ReachPSolver);
	//SetInitialValue
	auto WaterColumnPPMass = RegisterEquationODE(Model, "Water column PP mass", Kg);
	SetSolver(Model, WaterColumnPPMass, ReachPSolver);
	//SetInitialValue
	auto WaterColumnPorewaterTDPExchange = RegisterEquation(Model, "Water column pore water TDP exchange", KgPerDay);
	SetSolver(Model, WaterColumnPorewaterTDPExchange, ReachPSolver);
	auto ReachPPEntrainment = RegisterEquation(Model, "Reach PP entrainment", KgPerDay);
	SetSolver(Model, ReachPPEntrainment, ReachPSolver);
	auto ReachPPDeposition  = RegisterEquation(Model, "Reach PP deposition", KgPerDay);
	SetSolver(Model, ReachPPDeposition, ReachPSolver);
	auto WaterColumnTDPConcentration = RegisterEquation(Model, "Water column TDP concentration", MgPerL);
	SetSolver(Model, WaterColumnTDPConcentration, ReachPSolver);
	auto WaterColumnPPConcentration = RegisterEquation(Model, "Water column PP concentration", MgPerL);
	SetSolver(Model, WaterColumnPPConcentration, ReachPSolver);
	auto PorewaterEPC0 = RegisterEquation(Model, "Pore water EPC0", MgPerL);
	SetSolver(Model, PorewaterEPC0, ReachPSolver);
	auto StreamBedPSorptionDesorption = RegisterEquation(Model, "Stream bed P sorption/desorption", KgPerDay);
	SetSolver(Model, StreamBedPSorptionDesorption, ReachPSolver);
	auto PorewaterTDPUptakeByMacrophytes = RegisterEquation(Model, "Pore water TDP uptake by macrophytes", KgPerDay);
	SetSolver(Model, PorewaterTDPUptakeByMacrophytes, ReachPSolver);
	auto PorewaterTDPMass = RegisterEquationODE(Model, "Pore water TDP mass", Kg);
	SetSolver(Model, PorewaterTDPMass, ReachPSolver);
	//SetInitialValue
	auto PorewaterTDPConcentration = RegisterEquation(Model, "Pore water TDP concentration", MgPerL);
	SetSolver(Model, PorewaterTDPConcentration, ReachPSolver);
	auto BedPPMass = RegisterEquationODE(Model, "Stream bed PP mass", Kg);
	SetSolver(Model, BedPPMass, ReachPSolver);
	//SetInitialValue
	
	
	auto MacrophyteGrowthRate = RegisterEquation(Model, "Macrophyte growth rate", GCPerM2PerDay);
	SetSolver(Model, MacrophyteGrowthRate, ReachPSolver);
	auto MacrophyteDeathRate  = RegisterEquation(Model, "Macrophyte death rate", GCPerM2PerDay);
	SetSolver(Model, MacrophyteDeathRate, ReachPSolver);
	auto MacrophyteBiomass       = RegisterEquationODE(Model, "Macrophyte biomass", GCPerM2);
	SetSolver(Model, MacrophyteBiomass, ReachPSolver);
	//SetInitialValue
	auto EpiphyteGrowthRate = RegisterEquation(Model, "Epiphyte growth rate", GCPerM2PerDay);
	SetSolver(Model, EpiphyteGrowthRate, ReachPSolver);
	auto EpiphyteDeathRate  = RegisterEquation(Model, "Epiphyte death rate", GCPerM2PerDay);
	SetSolver(Model, EpiphyteDeathRate, ReachPSolver);
	auto EpiphyteBiomass    = RegisterEquationODE(Model, "Epiphyte biomass", GCPerM2);
	SetSolver(Model, EpiphyteBiomass, ReachPSolver);
	//SetInitialValue
	
	
	
	
	auto WaterColumnSRPConcentration = RegisterEquation(Model, "Water column SRP concentration", MgPerL);
	auto WaterColumnPControl = RegisterEquation(Model, "Water column P control", Dimensionless);
	auto PorewaterPControl   = RegisterEquation(Model, "Pore water P control", Dimensionless);
	
	
	EQUATION(Model, ReachPPInputFromLand,
		return
			  PARAMETER(PPEnrichmentFactor)
			* RESULT(SedimentDeliveryToReach)
			* SafeDivide(RESULT(SoilLabilePMass) + RESULT(SoilInactivePMass), RESULT(SoilMassInTheOAHorizon));
	)
	
	EQUATION(Model, AreaScaledReachPPInputFromLand,
		return RESULT(ReachPPInputFromLand) * PARAMETER(SubcatchmentArea) * PARAMETER(Percent) / 100.0;
	)
	
	EQUATION(Model, ReachTDPInputFromLand,
		return
			  RESULT(DirectRunoffTDPMass) * RESULT(DirectRunoffToReachFraction)
			+ RESULT(SoilwaterTDPMass) * RESULT(SoilToReachFraction)
			+ RESULT(GroundwaterTDPMass) * RESULT(GroundwaterToReachFraction);
	)
	
	EQUATION(Model, AreaScaledReachTDPInputFromLand,
		return RESULT(ReachTDPInputFromLand) * PARAMETER(SubcatchmentArea) * PARAMETER(Percent) / 100.0;
	)
	
	EQUATION(Model, WaterColumnTDPInput,
		double upstreamtdp = 0.0;
		
		FOREACH_INPUT(Reach,
			upstreamtdp += RESULT(WaterColumnTDPOutput, *Input);
		)
		
		//TODO: effluent inputs
		
		return upstreamtdp + RESULT(TotalReachTDPInputFromLand);
	)
	
	EQUATION(Model, WaterColumnPPInput,
		double upstreampp = 0.0;
		
		FOREACH_INPUT(Reach,
			upstreampp += RESULT(WaterColumnPPOutput, *Input);
		)
		
		//TODO: effluent inputs
		
		return upstreampp + RESULT(TotalReachPPInputFromLand);
	)
	
	EQUATION(Model, WaterColumnTDPOutput,
		return 86400.0 * RESULT(ReachFlow) * SafeDivide(RESULT(WaterColumnTDPMass), RESULT(ReachVolume));
	)
	
	EQUATION(Model, WaterColumnPPOutput,
		return 86400.0 * RESULT(ReachFlow) * SafeDivide(RESULT(WaterColumnPPMass), RESULT(ReachVolume));
	)
	
	EQUATION(Model, ReachTDPUptakeByEpiphytes,
		return 1e-3 * PARAMETER(ProportionOfPInEpiphytes) * RESULT(EpiphyteGrowthRate) * PARAMETER(ReachLength) * PARAMETER(ReachWidth);
	)
	
	EQUATION(Model, WaterColumnEPC0,
		return 
			EPC0Computation(
				PARAMETER(WaterColumnPSorptionCoefficient), 
				RESULT(WaterColumnPPMass), 
				RESULT(TotalSuspendedSedimentMass), 
				PARAMETER(WaterColumnFreundlichIsothermConstant), 
				RESULT(WaterColumnTDPConcentration)
				);
	)
	
	EQUATION(Model, WaterColumnPSorptionDesorption,
		return
			SorptionComputation(
				PARAMETER(WaterColumnSorptionScalingFactor),
				RESULT(WaterColumnTDPConcentration),
				RESULT(WaterColumnEPC0),
				PARAMETER(WaterColumnFreundlichIsothermConstant),
				RESULT(ReachVolume)
				);
	)
	
	EQUATION(Model, WaterColumnPorewaterTDPExchange,
		return 1e-3 * PARAMETER(WaterColumnStreamBedTDPExchangeFraction) * RESULT(ReachVolume) * (RESULT(PorewaterTDPConcentration) - RESULT(WaterColumnTDPConcentration));
	)
	
	EQUATION(Model, ReachPPEntrainment,
		return RESULT(TotalEntrainment) * SafeDivide(RESULT(BedPPMass), RESULT(TotalBedSedimentMass)) * PARAMETER(ReachLength) * PARAMETER(ReachWidth);
	)
	
	EQUATION(Model, ReachPPDeposition,
		return RESULT(TotalDeposition) * SafeDivide(RESULT(WaterColumnPPMass), RESULT(TotalSuspendedSedimentMass)) * PARAMETER(ReachLength) * PARAMETER(ReachWidth);
	)
	
	EQUATION(Model, WaterColumnTDPMass,
		//TODO: abstraction
		return
			  RESULT(WaterColumnTDPInput)
			- RESULT(WaterColumnTDPOutput)
			- RESULT(ReachTDPUptakeByEpiphytes)
			- RESULT(WaterColumnPSorptionDesorption)
			+ RESULT(WaterColumnPorewaterTDPExchange);
	)
	
	EQUATION(Model, WaterColumnPPMass,
		//TODO: abstraction
		return
			  RESULT(WaterColumnPPInput)
			- RESULT(WaterColumnPPOutput)
			+ RESULT(ReachPPEntrainment)
			- RESULT(ReachPPDeposition)
			+ RESULT(WaterColumnPSorptionDesorption);
	)
	
	EQUATION(Model, WaterColumnPControl,
		return 0.0;
		//TODO
	)
	
	EQUATION(Model, WaterColumnTDPConcentration,
		return 1e3 * SafeDivide(RESULT(WaterColumnTDPMass), RESULT(ReachVolume));
	)
	
	EQUATION(Model, WaterColumnPPConcentration,
		return 1e3 * SafeDivide(RESULT(WaterColumnPPMass), RESULT(ReachVolume));
	)
	
	EQUATION(Model, WaterColumnSRPConcentration,
		double mdoc = PARAMETER(RatioOfHydrolysablePToDOC);
		double msrp = PARAMETER(RegressionBetweenTDPAndSRPGradient);
		double csrp = PARAMETER(RegressionBetweenTDPAndSRPConstant);
		if(INPUT_WAS_PROVIDED(DOCConcentration))
		{
			return RESULT(WaterColumnTDPConcentration) - mdoc * INPUT(DOCConcentration);
		}
		return msrp * RESULT(WaterColumnTDPConcentration) + csrp;
	)
	
	EQUATION(Model, PorewaterEPC0,
		return 
			EPC0Computation(
				PARAMETER(PorewaterPSorptionCoefficient), 
				RESULT(BedPPMass), 
				RESULT(TotalBedSedimentMass), 
				PARAMETER(PorewaterFreundlichIsothermConstant),
				RESULT(PorewaterTDPConcentration)
				);
	)
	
	EQUATION(Model, StreamBedPSorptionDesorption,
		double porewatervolume = PARAMETER(ReachLength) * PARAMETER(ReachWidth) * PARAMETER(BedSedimentDepth) * PARAMETER(BedSedimentPorosity);
		return
			SorptionComputation(
				PARAMETER(PorewaterSorptionScalingFactor),
				RESULT(PorewaterTDPConcentration),
				RESULT(PorewaterEPC0),
				PARAMETER(PorewaterFreundlichIsothermConstant),
				porewatervolume
				);
	)
	
	EQUATION(Model, PorewaterTDPUptakeByMacrophytes,
		return 1e-3 * PARAMETER(ProportionOfPInMacrophytes) * RESULT(MacrophyteGrowthRate) * PARAMETER(ReachLength) * PARAMETER(ReachWidth);
	)
	
	EQUATION(Model, PorewaterTDPMass,
		return
			- RESULT(WaterColumnPorewaterTDPExchange)
			- RESULT(StreamBedPSorptionDesorption)
			- RESULT(PorewaterTDPUptakeByMacrophytes);
	)
	
	EQUATION(Model, BedPPMass,
		return
			  RESULT(StreamBedPSorptionDesorption)
			+ RESULT(ReachPPDeposition)
			- RESULT(ReachPPEntrainment);
	)
	
	EQUATION(Model, PorewaterPControl,
		//TODO
		return 0.0;
	)
	
	EQUATION(Model, PorewaterTDPConcentration,
		double porewatervolume = PARAMETER(ReachLength) * PARAMETER(ReachWidth) * PARAMETER(BedSedimentDepth) * PARAMETER(BedSedimentPorosity);
		return 1e3 * SafeDivide(RESULT(PorewaterTDPMass), porewatervolume);
	)
	
	
	EQUATION(Model, MacrophyteGrowthRate,
		double TDPpw = RESULT(PorewaterTDPConcentration);
		return
			  PARAMETER(MacrophyteGrowthRateCoefficient)
			* pow(PARAMETER(MacrophyteTemperatureDependency), RESULT(WaterTemperature) - 20.0)
			* RESULT(MacrophyteBiomass)
			* RESULT(SolarRadiation)
			* PARAMETER(SelfShadingForMacrophytes)
			* TDPpw / 
				(
				  (PARAMETER(HalfSaturationOfPForMacrophyteGrowth) + TDPpw)
				* (PARAMETER(SelfShadingForMacrophytes) + RESULT(MacrophyteBiomass))
				);
	)
	
	EQUATION(Model, MacrophyteDeathRate,
		return PARAMETER(MacrophyteDeathRateCoefficient) * RESULT(MacrophyteBiomass) * RESULT(EpiphyteBiomass) * RESULT(ReachFlow);
	)
	
	EQUATION(Model, MacrophyteBiomass,
		return RESULT(MacrophyteGrowthRate) - RESULT(MacrophyteDeathRate);
	)
	
	EQUATION(Model, EpiphyteGrowthRate,
		double TDPwc = RESULT(WaterColumnTDPConcentration);
		return
			  PARAMETER(EpiphyteGrowthRateCoefficient)
			* pow(PARAMETER(EpiphyteTemperatureDependency), RESULT(WaterTemperature) - 20.0) 
			* RESULT(EpiphyteBiomass)
			* RESULT(MacrophyteBiomass)
			* RESULT(SolarRadiation)
			* TDPwc / (PARAMETER(HalfSaturationOfPForEpiphyteGrowth) + TDPwc);
	)
	
	EQUATION(Model, EpiphyteDeathRate,
		return PARAMETER(EpiphyteDeathRateCoefficent) * RESULT(EpiphyteBiomass) * RESULT(ReachFlow);
	)
	
	EQUATION(Model, EpiphyteBiomass,
		return RESULT(EpiphyteGrowthRate) - RESULT(EpiphyteDeathRate);
	)
}