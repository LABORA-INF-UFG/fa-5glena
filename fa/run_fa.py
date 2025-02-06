import subprocess
import platform
import os
import shlex
import time  # Import the time module

def open_terminal_and_run(command):
    current_os = platform.system()

    if current_os == 'Linux':
        # Open a terminal, run the command, and then exit the terminal after execution
        terminal_command = ['gnome-terminal', '--', 'bash', '-c', f"{command}; exit;"]
    elif current_os == 'Darwin':  # macOS
        # Open a terminal, run the command, and close the terminal after execution
        terminal_command = ['osascript', '-e', f'tell app "Terminal" to do script "{command}; exit;"']
    elif current_os == 'Windows':
        # Open a cmd window, run the command, and close the cmd window after execution
        terminal_command = ['cmd', '/c', 'start', 'cmd', '/k', f"{command} && exit"]
    else:
        raise NotImplementedError(f"Unsupported OS: {current_os}")

    process = subprocess.Popen(terminal_command)
    return process

if __name__ == "__main__":
    # Get the current directory where the script is located
    current_directory = os.path.dirname(os.path.abspath(__file__))

    # Construct the commands to run server2_14.py and client2_14.py
    first_command = f"python {shlex.quote(os.path.join(current_directory, 'server2_14_csv.py'))}"
    second_command = f"python {shlex.quote(os.path.join(current_directory, 'client2_14_csv.py'))}"
    
    # Run up to 15 rounds
    rounds = 15
    wait_time_seconds = 330  # Short wait time between iterations (in seconds)

    for roundx in range(1, rounds + 1):
        print(f"\nStarting round {roundx}...")

        # Open the first terminal and run the first command
        process1 = open_terminal_and_run(first_command)
    
        # Open the second terminal and run the second command
        process2 = open_terminal_and_run(second_command)
        
        # Wait for both processes to complete before proceeding to the next round
        process1.wait()
        process2.wait()

        # Optionally, short wait between rounds to ensure smooth transition
        print(f"Waiting for {wait_time_seconds} seconds before starting round {roundx + 1}...")
        time.sleep(wait_time_seconds)  # Short wait time between iterations
        
        print(f"Round {roundx} completed.")
        
    print("\nAll iterations completed.")

