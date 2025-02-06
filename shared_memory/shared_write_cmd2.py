import mmap
import os

SHM_CMD_NAME = "/shared_cmd"
CMD_SIZE = 256  # Size of the shared memory

def write_shared_cmd(command):
    """
    Writes a command to the shared memory. If the shared memory does not exist,
    it will be created.
    
    Parameters:
        command (str): The command string to write to shared memory.
    """
    shm_path = f'/dev/shm{SHM_CMD_NAME}'
    
    try:
        # Attempt to open the existing shared memory object for read and write.
        fd = os.open(shm_path, os.O_RDWR)
    except FileNotFoundError:
        # If the shared memory doesn't exist, create it.
        print(f"Shared memory not found at {shm_path}. Creating it.")
        fd = os.open(shm_path, os.O_CREAT | os.O_RDWR, 0o600)
        os.ftruncate(fd, CMD_SIZE)
    
    try:
        # Map the shared memory for both reading and writing.
        with mmap.mmap(fd, CMD_SIZE, mmap.MAP_SHARED, mmap.PROT_READ | mmap.PROT_WRITE) as shm:
            shm.seek(0)
            # Encode the command and append a null terminator.
            encoded_command = command.encode('utf-8')
            shm.write(encoded_command + b'\x00')
            # Optionally, pad the rest of the shared memory with null bytes.
            remaining_bytes = CMD_SIZE - (len(encoded_command) + 1)
            if remaining_bytes > 0:
                shm.write(b'\x00' * remaining_bytes)
            #print(f"Command '{command}' written to shared memory.")
    except Exception as e:
        print(f"Error writing to shared memory: {e}")
    finally:
        os.close(fd)

