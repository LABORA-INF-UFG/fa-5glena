# fa-5glena-integration
Federated Analytics for B5G/6G networks

# Title: Channel-Aware Federated Analytics (FA) in B5G/6G Networks: Dynamic Power Allocation with NS-3 5G-LENA

The project involves developing an integrated framework for simulating FA in real 5G network conditions. The framework applies two algorithms: channel-aware power allocation algorithm to efficiently allocate transmission power for user equipments (UEs) based on distance and channel conditions, and synchronous FA-5GLENA integrated algorithm to integrate the FA with NS-3 5G-LENA and aggregate results for optimized performance within 5G network conditions. 


Configuration
1. Download and install Anaconda from the site:  https://docs.anaconda.com/anaconda/install/
2. Create environment named "fa_5glena" with python 3.8.20.
3. Inside environment "fa_5glena" create a root directory called "myproject"
4. Inside the root directory "myproject" create subdirectories:
      "FA" - where all FA server and client related files will be placed.
      "shared_memory" - where all shared memory related files will be placed
      "sim_results" - where network and FA simulation results will be stored.
   After installation of 5G-LENA, a 4th subdirectory named "ns-3-dev" will be generated in this directory.
5. Find other installation steps in each directory's configuration file. See in 5glena, fa, shared_memory directories.

This project was created in anaconda, following the structure: anaconda3/envs/fa_5glena/myproject
myproject as the root dirctory contains subdirectories: FA, ns-3-dev, shared_memory and sim_results

## Steps to run the project
1. Open the linux terminal (We ran using gnome-terminal)
2. Navegate to the "shared_memory" directory
3. Run the following commands to create and load the shared memories:
2	g++ -std=c++17 -o shared_cmd.so shared_cmd.cc -lrt
	g++ -std=c++17 -o shared_dr.so shared_dr.cc -lrt
	./shared_cmd.so
	./shared_dr.so
4. Navegate to "myproject" root directory
5. Run the command "python3 run_project.py" to run the project.

Note: At the end, the simulation results are stored in "sim_results" directory subdirectory
