import sys
import os
import time
import subprocess
import importlib

# Add the build directory to sys.path (Make sure the shared library is placed here)
sys.path.append(os.path.abspath('./build'))

# Function to write data to shared memory and then recompile the writer code twice
def write_and_recompile():
    # Recompile the C++ writer code after writing data
    try:
        subprocess.run(['./compile_writer.sh'], check=True)
    except subprocess.CalledProcessError as e:
        print(f"Error during compilation: {e}")
        exit(1)

    try:
        # Write data to shared memory
        import shared_memory_writer
        shared_memory_writer.create_shared_memory()
    except Exception as e:
        print(f"Error calling create_shared_memory: {e}")
        exit(1)

    # Reload the module to ensure the latest version is used
    try:
        importlib.reload(shared_memory_writer)
    except Exception as e:
        print(f"Error reloading module: {e}")
        exit(1)

# Function to read the updated data from shared memory
def read_shared_memory():
    # Wait for the writer code to update shared memory and sync it properly
    time.sleep(1)  # Give it a moment to ensure data is written and synced

    try:
        # Read data from shared memory
        import shared_memory_writer
        vec = shared_memory_writer.read_shared_memory()
        
        # Separate delivery_ratios and commands
        delivery_ratios, commands = vec
        
        # Print the total number of elements in each vector
        print(f"Total delivery_ratios: {len(delivery_ratios)}")
                
        # Print the delivery_ratios vector and access a specific value
        print("Delivery Ratios:", delivery_ratios)
        if len(delivery_ratios) > 0:
            # Example: Find the index of a specific delivery_ratio value and print it
            specific_value = delivery_ratios[0]  # Use the first value as an example
            for index, value in enumerate(delivery_ratios):
                if value == specific_value:
                    print(f"Value at {index}: {value}")  # index + 1 for 1-based indexing
        
        print()
        
        # Print the commands vector and access a specific value
        print(f"Total commands: {len(commands)}")
        print("Commands:", commands)
        if len(commands) > 0:
            # Example: Find the index of a specific command value and print it
            specific_value = commands[0]  # Use the first command as an example
            for index, value in enumerate(commands):
                if value == specific_value:
                    print(f"Command at {index}: {value}")  # index + 1 for 1-based indexing
        print()
        
    except Exception as e:
        print(f"Error calling read_shared_memory: {e}")
        exit(1)

# Run the process: write data, recompile twice, and then read the updated data
write_and_recompile()
read_shared_memory()

