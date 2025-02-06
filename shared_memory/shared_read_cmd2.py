import mmap
import os

SHM_CMD_NAME = "/shared_cmd"
CMD_SIZE = 256  # Tamanho da memória compartilhada (ajuste conforme necessário)

def read_shared_cmd():
    """
    Lê o valor da memória compartilhada e retorna a string lida.
    """
    shm_path = f'/dev/shm{SHM_CMD_NAME}'
    
    try:
        # Tenta abrir a memória compartilhada existente para leitura.
        fd = os.open(shm_path, os.O_RDONLY)
    except FileNotFoundError as e:
        print(f"Erro ao acessar a memória compartilhada: {e}")
        return None
    except Exception as e:
        print(f"Erro inesperado ao abrir a memória compartilhada: {e}")
        return None

    try:
        # Mapeia a memória compartilhada para leitura.
        with mmap.mmap(fd, CMD_SIZE, mmap.MAP_SHARED, mmap.PROT_READ) as shm:
            shm.seek(0)
            # Lê o conteúdo da memória e corta na primeira ocorrência de '\x00'
            raw_data = shm.read(CMD_SIZE)
            command_str = raw_data.split(b'\x00', 1)[0].decode('utf-8')
            return command_str
    except Exception as e:
        print(f"Erro ao ler a memória compartilhada: {e}")
        return None
    finally:
        os.close(fd)

if __name__ == "__main__":
    command = read_shared_cmd()
    if command is not None:
        #print(f"Comando lido da memória compartilhada: {command}")
        pass

