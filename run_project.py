import subprocess
import sys

# Command to run initial parameters (Python)
server_param = "python3 /home/ufg/anaconda3/envs/fa_5glena/myproject/FA/server_params.py"

# Command to run the ns-3 5g-lena simulation for network simulation (C++)
network_run = "/home/ufg/anaconda3/envs/fa_5glena/myproject/ns-3-dev/ns3 run scratch/OptmTx_UMi_algorithm.cc"

# Command to select clients, train, predict, send responses do server and aggragate results (Python)
fa_run = "python3 /home/ufg/anaconda3/envs/fa_5glena/myproject/FA/run_fa.py"

# Open terminal and execute commands sequentially, with 'read' to keep terminal open (Python)
command = f'gnome-terminal -- bash -c "{server_param} && {network_run} && {fa_run} && echo \'Press any key to exit...\'; read"'

# Run the command to open terminal and execute the sequence (Python)
subprocess.run(command, shell=True)
