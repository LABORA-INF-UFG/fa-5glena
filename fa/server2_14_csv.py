import csv
import json
import socket
import random
import time
from collections import defaultdict
import os

import shared_memory_reader

# Call the read_shared_memory() function to retrieve delivery_ratios and commands
delivery_ratios, commands = shared_memory_reader.read_shared_memory()

print(f"Number of DRs: {len(delivery_ratios)}")
print("List of all DRs: ", delivery_ratios)
print("First DR: ", delivery_ratios[0])

print()

print(f"Number of CMD: {len(commands)}")
print("List of all CMD: ", commands)
print("First CMD: ", commands[0])

# Record the start time
exec_start_time = time.time()

# Define the server address
server_address = ('localhost', 12345)

# Create a TCP/IP socket
server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

# Bind the socket to the server address
server_socket.bind(server_address)

# Listen for incoming connections
server_socket.listen(5)

print("\nServer up and listening...\n")

# Step 1: Read clients from the imported delivery_ratios vector
clients = []
view_clients = []

# Use delivery_ratios to select clients
for i, dr in enumerate(delivery_ratios):
    if float(dr) >= 90:  # Only select clients with delivery ratio >= 90
        clients.append(i+1)  # Assuming index corresponds to client ID
    else:
        clients.append(-1)

valid_clients = [client for client in clients if client != -1]
num_selected = len(valid_clients)

# Count all clients, including those with -1 assigned (total number of clients)
num_clients = len(clients)

# Ensure the list has exactly view_client clients (the total count of clients)
view_client = num_clients

# Print results
#print(f"Number of clients sampled: {num_clients}")


# Step 2: Send num_clients to the client
client_socket, client_address = server_socket.accept()

# Send num_clients to the client
serialized_num_clients = json.dumps(view_client).encode('utf-8')
client_socket.sendall(serialized_num_clients)

# Close the connection after sending num_clients
client_socket.close()

max_test_dataset_size = 0  # Initialize the maximum test dataset size

# Step 3: Receive dataset size from client side (Wait for a connection)
client_socket, client_address = server_socket.accept()

print("DEFINED PARAMETERS")
# Receive dataset size from the client
serialized_data = client_socket.recv(4096)
if serialized_data:
    # Deserialize received data
    received_data = json.loads(serialized_data.decode('utf-8'))
    test_dataset_size = received_data['test_dataset_size']
    client_id = received_data['client_id']

    # Update the maximum test dataset size
    max_test_dataset_size = test_dataset_size

    print(f"Maximum test dataset size: {max_test_dataset_size}")

# Close the client connection after receiving test dataset size
client_socket.close()

# Generate a fixed image index using the maximum test dataset size
index = 193
print(f"Query to clients: Predict image at index {index}?")
print(f"Number of total clients: {view_client}")
print(f"Sampled Clients: {clients}")

if index >= max_test_dataset_size or index < 0:
    print("\nError: Index out of range of test dataset size!")

# Initialize lists to store received data from clients
received_data_list = []
transmission_times = []
data_sizes = []
throughputs = []

# Initialize reliability scores (priors)
client_reliability = defaultdict(lambda: [1, 1])  # Beta distribution parameters (alpha, beta)

# Function to update reliability based on received data
def update_reliability(client_id, confidence):
    alpha, beta = client_reliability[client_id]
    reliability_weight = confidence  # Use confidence as reliability weight
    alpha += reliability_weight
    beta += (1 - reliability_weight)
    client_reliability[client_id] = [alpha, beta]

# Step 4: Accept connections from multiple clients to receive performance data
for _ in range(view_client):
    # Wait for a connection
    client_socket, client_address = server_socket.accept()

    # Define the query data to be sent to the client
    query_data = {
        'image_index': index,
        'client_ids': clients
    }

    # Serialize query data to JSON
    serialized_query_data = json.dumps(query_data).encode('utf-8')

    # Send query data to the client
    client_socket.sendall(serialized_query_data)

    # Receive data from the client
    while True:
        start_time = time.time()  # Start measuring transmission time
        serialized_data = client_socket.recv(4096)
        end_time = time.time()  # End measuring transmission time

        if not serialized_data:
            break
        
        # Deserialize received data
        received_data = json.loads(serialized_data.decode('utf-8'))

        # Append received data to the list
        received_data_list.append(received_data)

        # Extract transmission metrics from received data
        transmission_time = received_data['transmission_time']
        data_size = received_data['data_size']
        throughput = received_data['throughput']
        
        # Append transmission metrics to lists
        transmission_times.append(transmission_time)
        data_sizes.append(data_size)
        throughputs.append(throughput)

        # Process received data
        print(f"\nReceived data from client {received_data['client_id']}:")
        print(f"Test accuracy: {received_data['test_accuracy']}")
        print(f"Test precision: {received_data['test_precision']}")
        print(f"Test recall: {received_data['test_recall']}")
        print(f"Predicted class: {received_data['predicted_class']}")
        
        # Update reliability based on the confidence of the prediction
        update_reliability(received_data['client_id'], received_data['confidence'])
        
        # Send acknowledgment to the client
        acknowledgment = "Data received successfully."
        client_socket.sendall(acknowledgment.encode('utf-8'))

    # Close the client connection
    client_socket.close()
    
    # Break the loop if all clients have sent their data
    if len(received_data_list) == view_client:
        break

# Aggregate received data using Bayesian Aggregation
if received_data_list:
    class_probabilities = defaultdict(float)
    total_weight = 0.0
    total_accuracy = 0.0
    total_precision = 0.0
    total_recall = 0.0
    total_transmission_time = 0.0
    total_throughput = 0.0

    for data in received_data_list:
        client_id = data['client_id']
        predicted_class = data['predicted_class']
        confidence = data['confidence']
        alpha, beta = client_reliability[client_id]
        reliability = alpha / (alpha + beta)

        # Weight of the client based on dataset size
        weight = data['weight_dataset']
        total_weight += weight

        class_probabilities[predicted_class] += reliability * confidence * weight

        # Aggregate accuracy, precision, recall
        total_accuracy += data['test_accuracy'] * weight
        total_precision += data['test_precision'] * weight
        total_recall += data['test_recall'] * weight

        # Aggregate transmission metrics
        total_transmission_time = sum(transmission_times)
        avg_throughput = sum(throughputs) / len(received_data_list)

    # Calculate weighted averages
    if total_weight > 0:
        avg_accuracy = total_accuracy / total_weight
        avg_precision = total_precision / total_weight
        avg_recall = total_recall / total_weight
        
        for class_label in class_probabilities:
            class_probabilities[class_label] /= total_weight

        # Determine the final predicted class
        final_predicted_class = max(class_probabilities, key=class_probabilities.get)

        print("\nAggregated results:")
        print(f"Expected number of responses: {len(received_data_list)}")
        print(f"Number of received responses: {num_selected}")
        print(f"Receiption ratio: {num_selected/view_client*100:.6f} %")
        print(f"Final predicted class: {final_predicted_class}")
        print(f"Average accuracy: {avg_accuracy:.6f}")
        print(f"Average precision: {avg_precision:.6f}")
        print(f"Average recall: {avg_recall:.6f}")

        # Write aggregated results to a CSV file
        csv_target_file = 'fa_results/fa_randPower.csv'

        # Initialize current_round
        current_round = 1

        # Check if the file exists to determine the starting round number
        if os.path.exists(csv_target_file) and os.path.getsize(csv_target_file) > 0:
            try:
                with open(csv_target_file, 'r', newline='') as csvfile:
                    reader = csv.DictReader(csvfile)
                    rows = list(reader)
                    if rows:
                        last_row = rows[-1]
                        current_round = int(last_row['round']) + 1
                    else:
                        current_round = 1
            except IOError as e:
                print(f"Error reading CSV file: {e}")

        try:
            with open(csv_target_file, 'a', newline='') as csvfile:
                fieldnames = [
                    'round', 'total_clients', 'final_prediction',
                    'average_accuracy', 'average_precision', 'average_recall'
                ]
                writer = csv.DictWriter(csvfile, fieldnames=fieldnames)

                # Check if file is empty and write header
                if os.path.getsize(csv_target_file) == 0:
                    writer.writeheader()

                # Write new row with incremented Round value
                writer.writerow({
                    'round': current_round,
                    'total_clients': len(received_data_list),
                    'final_prediction': final_predicted_class,
                    'average_accuracy': f"{avg_accuracy:.6f}",
                    'average_precision': f"{avg_precision:.6f}",
                    'average_recall': f"{avg_recall:.6f}",
                })
            
        except IOError as e:
            print(f"Error writing to CSV file: {e}")

        finally:
            print(f"\nTerminating connection...")
            time.sleep(5)  # Print the message for 5 seconds 

            server_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
            server_socket.shutdown(socket.SHUT_RDWR)
            server_socket.close()

            # Record the end time
            exec_end_time = time.time()
            execution_time = exec_end_time - exec_start_time            
            print(f"\nExecution time: {execution_time:.6f} seconds\n")
            time.sleep(5)  # Print the message for 5 seconds 
            exit

