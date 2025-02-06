import mmap
import struct
import os

def read_shared_dr():
    # Constants
    SHM_DR_NAME = '/shared_dr'  # Name of the shared memory
    DR_COUNT = 20               # Number of double elements
    DR_SIZE = DR_COUNT * 8      # Size in bytes (since double = 8 bytes)

    # Open the shared memory object
    fd = os.open(f'/dev/shm{SHM_DR_NAME}', os.O_RDONLY)
    
    try:
        # Memory-map the file and read the data from shared memory
        with mmap.mmap(fd, DR_SIZE, mmap.MAP_SHARED, mmap.PROT_READ) as shm:
            data = shm.read(DR_SIZE)  # Read DR_SIZE (160 bytes) for 20 doubles
    finally:
        # Clean up: close the file descriptor
        os.close(fd)
    
    # Convert the binary data into a tuple of doubles (8 bytes each)
    delivery_ratios = struct.unpack(f'{DR_COUNT}d', data)  # 'd' is for double (8 bytes)
    return delivery_ratios

# Example usage:
if __name__ == "__main__":
    dr_values = read_shared_dr()
    # Format the output as a vector with square brackets and comma-separated values.
    vector_str = "[" + ", ".join(str(val) for val in dr_values) + "]"
    #print(vector_str)

