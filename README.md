# fa-5glena-integration
Federated Analytics for B5G/6G networks

## Title: Channel-Aware Federated Analytics (FA) in B5G/6G Networks: Dynamic Power Allocation with NS-3 5G-LENA

The project involves developing an integrated framework for simulating FA in real 5G network conditions. The framework applies two algorithms: channel-aware power allocation algorithm to efficiently allocate transmission power for user equipments (UEs) based on distance and channel conditions, and synchronous FA-5GLENA integrated algorithm to integrate the FA with NS-3 5G-LENA and aggregate results for optimized performance within 5G network conditions. 

### General Project Configuration
1. Download and install Anaconda from the site:  https://docs.anaconda.com/anaconda/install/
2. Create environment named "fa_5glena" with python 3.8.20.
3. Inside environment "fa_5glena" create a root directory called "myproject"
4. Inside the root directory "myproject" create subdirectories:		<br />
      "FA" - where all FA server and client related files will be placed.	<br />
      "shared_memory" - where all shared memory related files will be placed.	<br />
      "sim_results" - where network and FA simulation results will be stored.	<br />
   After installation of 5G-LENA, a 4th subdirectory named "ns-3-dev" will be generated in this directory.
5. Find other installation steps in each directory's configuration file. See in 5glena, fa, shared_memory directories.
6. The file "run_project.py" is placed in the root directory "myproject".

This project was created in Anaconda, following the structure: "anaconda3/envs/fa_5glena/myproject", where
"myproject" is the root dirctory that contains subdirectories: FA, ns-3-dev, shared_memory and sim_results.

### Steps to Run the Project
1. Open the Linux terminal (we used gnome-terminal).
2. Navegate to the "shared_memory" subdirectory.
3. Run the following commands to create and load the shared memories:	<br />
	g++ -std=c++17 -o shared_cmd.so shared_cmd.cc -lrt	<br />
	g++ -std=c++17 -o shared_dr.so shared_dr.cc -lrt	<br />
	./shared_cmd.so		<br />
	./shared_dr.so		<br />
4. Navegate to "myproject" root directory
5. Run the command "python3 run_project.py" to run the project.

Note: At the end of the simulation, the results are stored in "sim_results" subdirectory.

### Validation
The "OptmTx_UMi_algorithm.cc" code runs optimized transmission power allocation algorithm. To run random transmission power allocation scenario, substitute the function "CalculateMinimumTxPowerForUE()" function by another code that reads transmission power from "random_txpower_dbm.csv" file found in the repository. To run maximum uniform transmission power allocation, substititute the function "CalculateMinimumTxPowerForUE()" with code that allocates maximum transmission power for all user equipments (UEs).

