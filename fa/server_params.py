import json
import socket
import random
import time
from collections import defaultdict
import os
import sys
import importlib

print("\nInitial parameters created and shared...")

sys.path.append(os.path.abspath("/home/ufg/anaconda3/envs/fa_5glena/myproject/shared_memory"))
print()
import shared_read_dr2
# Call the read_shared_dr() function to retrieve delivery_ratios and commands
delivery_ratios = shared_read_dr2.read_shared_dr()
#print(f"Number of DRs: {len(delivery_ratios)}")
#print("List of all DRs: ", delivery_ratios)
#print("First DR: ", delivery_ratios[0])

#print()

import shared_read_cmd2
import shared_write_cmd2
command = "netrun"
def check_command_loop():
    while True:
        commands = shared_read_cmd2.read_shared_cmd()
        #print("Command from memory:", commands)
        if commands != "netend":
            print("Warning: Last network simulation did not successfully complete!\n")           
            shared_write_cmd2.write_shared_cmd(command)
            #print("Command has changed. Exiting the loop.")
            break            
        else:            
            shared_write_cmd2.write_shared_cmd(command)
            break
        time.sleep(5)
    #os.kill(os.getppid(), 9)  # Forcefully closes the terminal

if __name__ == "__main__":
    # Continuously check the shared memory every 5 seconds.
    check_command_loop()
    
    # Continue with the rest of your code after the command is no longer "netrun".
    #print("Continuing with the rest of the code...")
    # ... (rest of your code here)

