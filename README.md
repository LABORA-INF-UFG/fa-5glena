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
