/*
Project:     WUDESIM ver. 1 BETA
File:        WUDESIMmain.cpp
Author:      Ahmed Abokifa
Date:        10/25/2016
*/

#include <iostream>
#include <ctime>
#include <fstream>  
#include <ctime>


# include "WUDESIM.h"
# include "CLASSES.h"
# include "WUDESIM_CORE.h"
# include "WRITING_FUN.h"
# include "Utilities.h"
# include "epanet2.h"


using namespace std;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////     INITIALIZE WUDESIM  /////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Define network
Network net;

// initialize error
int    error = 0;

// initialize log file
ofstream DE_LOG_FILE;

// initialize bool flags for WUDESIM functions
bool OPEN_EPANET_PROJ_fl        = false;
bool FIND_DEADENDS_fl           = false;
bool RUN_EPANET_SIM_fl          = false;
bool CALC_DEADEND_PROPERTIES_fl = false;
bool OPEN_WUDESIM_PROJ_fl       = false;
bool GENERATE_STOC_DEMAND_fl    = false;
bool RUN_WUDESIM_SIM_fl         = false;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////      FULL SIMULATION    /////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


// Run complete simulation

int DE_RUN_FULL_SIM(const char* EPANET_INP, const char* EPANET_RPT, const char* WUDESIM_INP, const char* WUDESIM_RPT) {
		
	// Start clock
	double start_s = clock(); 

	WRITE_LOG_MSG("****************	FULL WUDESIM Simulation Started!	****************");
	
	error = DE_ENGINE_OPEN_EPANET_PROJ(EPANET_INP, EPANET_RPT); if (error) { goto failed; }

	error = DE_ENGINE_FIND_DEADENDS(); if (error) { goto failed; } else { DE_WRITE_DEADEND_IDS(); }

	error = DE_ENGINE_RUN_EPANET_SIM(); if (error) { goto failed; } else { DE_WRITE_EPANET_REPORT(); }

	error = DE_ENGINE_CALC_DEADEND_PROPERTIES_EPANET(); if (error) { goto failed; } else { DE_WRITE_DEADEND_PROPERTIES(); }

	error = DE_ENGINE_OPEN_WUDESIM_PROJ(WUDESIM_INP, WUDESIM_RPT); if (error) { goto failed; }

	error = DE_ENGINE_GENERATE_STOC_DEMAND(); if (error) { goto failed; } else { DE_WRITE_STOCHASTIC_DEMANDS(); }

	error = DE_ENGINE_RUN_WUDESIM_SIM(); if (error) { goto failed; } else { DE_WRITE_WUDESIM_REPORT(); }
	
	WRITE_LOG_MSG("\n*****************************************************************************");
	WRITE_LOG_MSG("WUDESIM finished successfuly!");
	WRITE_LOG_MSG("WUDESIM execution time: " + toString(clock() - start_s / double(CLOCKS_PER_SEC) * 1000) + " ms");

	return 0;

failed:;
	WRITE_LOG_MSG("WUDESIM did not finish successfuly!"); return 1;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////      ENGINE FUNCTIONS   /////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Open EPANET Project
int DE_ENGINE_OPEN_EPANET_PROJ(const char* EPANET_INP, const char* EPANET_RPT) {

	WRITE_LOG_MSG("\no	Processing EPANET input file  ...");

	// Get file name	
	net.EPANET_INP = EPANET_INP;
	net.EPANET_RPT = EPANET_RPT;

	// Process EPANET input file
	error = OP_EPANET_INP(&net);
	if (error) { WRITE_LOG_MSG("o	Processing EPANET input file was not successful!"); return 1; }
	else { 
		WRITE_LOG_MSG("o	Processing EPANET input file was successful!"); 
		OPEN_EPANET_PROJ_fl = true;
	}

	return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Find dead-end branches in the network
int DE_ENGINE_FIND_DEADENDS() {
	
	WRITE_LOG_MSG("\no	Finding dead-end branches ...");

	// Check whether an EPANET project was successfuly opened
	if (!OPEN_EPANET_PROJ_fl) { WRITE_LOG_MSG("\no	EPANET project was not successfuly opened!"); return 1; }

	// Find dead end branches in the network
	int N_DEbranches = 0;
	N_DEbranches = FIND_DE_BRANCHES(&net);
	if (N_DEbranches == 0) { WRITE_LOG_MSG("o	Found no dead-end branches in the network!"); return 1; }
	else { 
		WRITE_LOG_MSG("o	Found " + toString(N_DEbranches) + " dead-end branches in the network");
		FIND_DEADENDS_fl = true;
	}

	return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Run EPANET simulation
int DE_ENGINE_RUN_EPANET_SIM() {

	WRITE_LOG_MSG("\no	Running EPANET simulation...");
	
	// Check whether dead-ends were discovered
	if (!FIND_DEADENDS_fl) { error = DE_ENGINE_FIND_DEADENDS(); if (error) { return 1; } }

	// Run EPANET simulation and extract results
	error = RUN_EPANET_SIM(&net);
	if (error) { WRITE_LOG_MSG("o	Running EPANET was not successful!"); return 1; }
	else {
		WRITE_LOG_MSG("o	Running EPANET was successful!");
		RUN_EPANET_SIM_fl = true;
	}

	return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Calculate the properties of dead-end branches
int DE_ENGINE_CALC_DEADEND_PROPERTIES_EPANET(){

	WRITE_LOG_MSG("\no	Evaluating properties of dead-end pipes ...");

	// Check whether an EPANET simulation was conducted
	if (!RUN_EPANET_SIM_fl) { WRITE_LOG_MSG("\no	EPANET simulation did not successfuly complete!"); return 1; }

	//   Calculate dead end pipes properties
	error = CALC_DE_PROPERTIES(&net);
	if (error) { WRITE_LOG_MSG("o	Evaluating properties of dead-end pipes was not successful!"); return 1; }
	else { 
		WRITE_LOG_MSG("o	Evaluating properties of dead-end pipes was successful!");
		CALC_DEADEND_PROPERTIES_fl = true;
	}

	return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Open WUDESIM Project
int DE_ENGINE_OPEN_WUDESIM_PROJ(const char* WUDESIM_INP, const char* WUDESIM_RPT) {

	WRITE_LOG_MSG("\no	Processing WUDESIM input file ...");

	// Check whether dead-ends were discovered
	if (!CALC_DEADEND_PROPERTIES_fl) { error = DE_ENGINE_CALC_DEADEND_PROPERTIES_EPANET(); if (error) { return 1; } }

	// Get file names	
	net.WUDESIM_INP = WUDESIM_INP;
	net.WUDESIM_RPT = WUDESIM_RPT;

	// Process WUDESIM input file
	error = OP_WUDESIM_INP(&net);
	if (error) { WRITE_LOG_MSG("o	Processing WUDESIM input file was not successful!"); return 1; }
	else {
		WRITE_LOG_MSG("o	Processing WUDESIM input file was successful!");
		OPEN_WUDESIM_PROJ_fl = true;
	}

	return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Generate stochastic demands for deadend pipes
int DE_ENGINE_GENERATE_STOC_DEMAND() {

	// Check whether WUDESIM project was successfuly opened
	if (!OPEN_WUDESIM_PROJ_fl) { WRITE_LOG_MSG("\no	WUDESIM project did not open successfuly!"); return 1; }

	//  Generate stochastic demands
	if (net.DE_options.Stoc_dem_fl) {

		WRITE_LOG_MSG("\no	Generating stochastic flows for selected dead-end pipes ...");

		error = GEN_STOC_DEM(&net, net.DE_options.simulated_branches);
		if (error) { WRITE_LOG_MSG("o	Stochastic flow generation was not successful!"); return 1; }
		else { 
			WRITE_LOG_MSG("o	Stochastic flow generation was successful!");
			GENERATE_STOC_DEMAND_fl = true;
		}	
	}
	return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Run WUDESIM simulation
int DE_ENGINE_RUN_WUDESIM_SIM() {

	WRITE_LOG_MSG("\no	Starting WUDESIM water quality simulations!");

	// Check whether WUDESIM project was successfuly opened
	if (!OPEN_WUDESIM_PROJ_fl) { WRITE_LOG_MSG("\no	WUDESIM project did not open successfuly!"); return 1; }

	// Check whether stochastic demands were conducted
	if (net.DE_options.Stoc_dem_fl && !GENERATE_STOC_DEMAND_fl) {
		error = DE_ENGINE_GENERATE_STOC_DEMAND(); if (error) { return 1; }
	}

	// Calculate Correction Factors
	CALC_CORR_FACT(&net);	
	
	// Check that a number of branches is selected for simulation
	if (net.DE_options.simulated_branches.empty()) { WRITE_LOG_MSG("	o	No Dead End branches are selected for simulation!"); return 1;}

	error = RUN_WUDESIM_SIM(&net, net.DE_options.simulated_branches);
	if (error) { WRITE_LOG_MSG("o	WUDESIM simulations were not successful!"); return 1; }
	else { 
		WRITE_LOG_MSG("o	WUDESIM simulations were successful!");
		RUN_WUDESIM_SIM_fl = true;
	}

	return 0;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Close WUDESIM and release all the memory
int DE_ENGINE_CLOSE_WUDESIM_PROJ() {

	// destruct the network
	net.~Network();

	// close EPANET
	ENclose();

	// close log file
	DE_LOG_FILE.close();

	// close all functions
	OPEN_EPANET_PROJ_fl        = false;
	FIND_DEADENDS_fl           = false;
	RUN_EPANET_SIM_fl          = false;
	CALC_DEADEND_PROPERTIES_fl = false;
	OPEN_WUDESIM_PROJ_fl       = false;
	GENERATE_STOC_DEMAND_fl    = false;
	RUN_WUDESIM_SIM_fl         = false;

	return 0;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////      WRITING FUNCTIONS   ////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int DE_WRITE_DEADEND_IDS() {

	// Check whether dead-ends were discovered
	if (!FIND_DEADENDS_fl) { error = DE_ENGINE_FIND_DEADENDS(); if (error) { return 1; } }
	write_DE_ids(&net);
	WRITE_LOG_MSG("o	IDs of Dead End Pipes were written to DEB_ID.OUT");
	return 0;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int DE_WRITE_DEADEND_PROPERTIES() {

	// Check whether Dead end properties were calculated
	if (!CALC_DEADEND_PROPERTIES_fl) { error = DE_ENGINE_CALC_DEADEND_PROPERTIES_EPANET(); if (error) { return 1; } }
	write_DE_Properties(&net);
	WRITE_LOG_MSG("o	Properties of Dead End Pipes were written to DEB_PROP.OUT");
	return 0;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int DE_WRITE_STOCHASTIC_DEMANDS() {

	if (net.DE_options.Stoc_dem_fl) {
		// Check whether stochastic demands were generated
		if (!GENERATE_STOC_DEMAND_fl) {
			error = DE_ENGINE_GENERATE_STOC_DEMAND(); if (error) { return 1; }
		}
		write_stoc_dems(&net);
		WRITE_LOG_MSG("o	Stochastic flows were written to DEB_STOC_FL.OUT");
	}
	return 0;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int DE_WRITE_WUDESIM_REPORT() {

	WRITE_LOG_MSG("\no	Writing WUDESIM Report file ...");

	if (!RUN_WUDESIM_SIM_fl) {
		WRITE_LOG_MSG("o	WUDESIM simulation did not finish successfully!"); return 1;
	}
	else{ 
		write_WUDESIM_rpt(&net); 
		WRITE_LOG_MSG("o	WUDESIM Report file was written successfully!");
	}
	return 0;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int DE_WRITE_EPANET_REPORT() {

	WRITE_LOG_MSG("\no	Writing EPANET Report file ...");

	if (!RUN_EPANET_SIM_fl) {
		WRITE_LOG_MSG("o	EPANET simulation did not finish successfully!"); return 1;
	}
	else {
		write_EPANET_rpt(&net);
		WRITE_LOG_MSG("o	EPANET Report file was written successfully!");
	}
	return 0;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////        GET FUNCTIONS     ////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int DE_GET_BRAN_COUNT(const int prop_idx) {
	switch(prop_idx){
	case DE_BRAN_COUNT:            return net.DE_branches.size();
	}
}

int DE_GET_STEP_COUNT(const int prop_idx) {
	switch (prop_idx) {
	case DE_EPANET_STEP_COUNT:     return net.times.N_steps;
	case DE_STOCHASTIC_STEP_COUNT: return net.DE_options.N_steps_WUDESIM;
	}
}

int DE_GET_BRAN_SIZE(const int prop_idx, const int branch_idx) { 
	switch (prop_idx) {	
	case DE_BRAN_SIZE: return net.DE_branches[branch_idx].branch_size;
	}
}

double DE_GET_PIPE_PROPERTY(const int prop_idx, const int branch_idx, const int pipe_idx) {
	switch (prop_idx) {
	case DE_LENGTH:           return net.DE_branches[branch_idx].length[pipe_idx];
	case DE_DIAMETER:         return net.DE_branches[branch_idx].diameter[pipe_idx];
	}
}

double DE_GET_PIPE_RESULT_EPANET(const int prop_idx, const int branch_idx, const int pipe_idx,const int step) {
	switch (prop_idx) {
	case DE_REYNOLDS_EPANET:   return net.DE_branches[branch_idx].Reynolds_EPANET[pipe_idx][step];
	case DE_RES_TIME_EPANET:   return net.DE_branches[branch_idx].Res_time_EPANET[pipe_idx][step];
	case DE_FLOW_EPANET:       return net.DE_branches[branch_idx].pipe_flow_EPANET[pipe_idx][step];
	}	
}

double DE_GET_PIPE_RESULT_WUDESIM(const int prop_idx, const int branch_idx, const int pipe_idx, const int step) {
	switch (prop_idx) {
	case DE_REYNOLDS_WUDESIM:        return net.DE_branches[branch_idx].Reynolds_WUDESIM_WRITE[pipe_idx][step];
	case DE_RES_TIME_WUDESIM:        return net.DE_branches[branch_idx].Res_time_WUDESIM_WRITE[pipe_idx][step];
	case DE_PECLET_WUDESIM:          return net.DE_branches[branch_idx].Peclet_WUDESIM_WRITE[pipe_idx][step];
	}
}

double DE_GET_NODE_RESULT_EPANET(const int prop_idx, const int branch_idx, const int node_idx, const int step) {
	switch (prop_idx) {
	case DE_QUAL_EPANET:       return net.DE_branches[branch_idx].terminal_C_EPANET[node_idx][step];
	case DE_DEMAND_EPANET:     return net.DE_branches[branch_idx].node_demand_EPANET[node_idx][step];
	}
}

double DE_GET_NODE_RESULT_WUDESIM(const int prop_idx, const int branch_idx, const int node_idx, const int step) {
	switch (prop_idx) {
	case DE_QUAL_WUDESIM: return net.DE_branches[branch_idx].terminal_C_WUDESIM_WRITE[node_idx][step];
	}
}

double DE_GET_STOC_FLOW(const int prop_idx, const int branch_idx, const int pipe_node_idx, const int step) {
	switch (prop_idx) {
	case DE_FLOW_STOCHASTIC:    return net.DE_branches[branch_idx].pipe_flow_STOC[pipe_node_idx][step];
	case DE_DEMAND_STOCHASTIC:  return net.DE_branches[branch_idx].node_demand_STOC[pipe_node_idx][step];
	}
}

const char* DE_GET_ID(const int prop_idx, const int branch_idx, const int pipe_node_idx) {
	switch (prop_idx) {
	case DE_PIPE_ID: return net.DE_branches[branch_idx].pipe_id[pipe_node_idx].c_str();
	case DE_NODE_ID: return net.DE_branches[branch_idx].terminal_id[pipe_node_idx].c_str();
	case DE_BRAN_ID: return net.DE_branches[branch_idx].branch_id.c_str();
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// WUDESIM log writing function
void WRITE_LOG_MSG(string output_msg) {

	// open log file if it's not open
	if (!DE_LOG_FILE.is_open()) { DE_LOG_FILE.open("WUDESIM.LOG", ios::out | ios::trunc); }

	cout << output_msg << endl;
	DE_LOG_FILE << output_msg << endl;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////