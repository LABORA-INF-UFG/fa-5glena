import json
import socket
import time
import tensorflow as tf
from tensorflow.keras import layers, models, regularizers
import numpy as np
from sklearn.metrics import precision_score, recall_score
import matplotlib.pyplot as plt
import os

# Set seeds for reproducibility
seed = 1
np.random.seed(seed)
tf.random.set_seed(seed)

# Check for available GPUs
physical_devices = tf.config.list_physical_devices('GPU')
if physical_devices:
    for device in physical_devices:
        tf.config.experimental.set_memory_growth(device, True)
    device_name = '/GPU:0'
else:
    device_name = '/CPU:0'

# Step 1: Receive num_clients from the server
server_address = ('localhost', 12345)

# Create a socket and connect to the server
client_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
client_socket.connect(server_address)

# Receive num_clients from the server
serialized_num_clients = client_socket.recv(4096)
num_clients = int(serialized_num_clients.decode('utf-8'))

print("\nDEFINED PARAMETERS")
print(f"Number of total clients: {num_clients}")

# Close the connection after receiving num_clients
client_socket.close()

# Load the MNIST dataset
(x_train, y_train), (x_test, y_test) = tf.keras.datasets.mnist.load_data()

# Preprocess (Normalize the data)
x_train = x_train / 255.0
x_test = x_test / 255.0

# Declare and initialize some important constants
num_epochs = 5
size_batch = 32

client_data_size = len(x_train) // num_clients  # Split the dataset into parts for clients

# Shuffle the data before splitting into client datasets (for reproducibility)
indices = np.arange(len(x_train))
np.random.shuffle(indices)
x_train = x_train[indices]
y_train = y_train[indices]

# Distribute the datasets to the clients
client_datasets = [(x_train[i * client_data_size:(i + 1) * client_data_size], 
                    y_train[i * client_data_size:(i + 1) * client_data_size]) for i in range(num_clients)]

# Define a function to create the neural network model
def create_model(input_shape):
    model = models.Sequential()
    model.add(layers.Flatten())
    model.add(layers.Dense(512, activation='relu'))
    #model.add(layers.Dropout(0.1))
    model.add(layers.Dense(256, activation='relu'))
    #model.add(layers.Dropout(0.1))
    model.add(layers.Dense(10, activation='softmax'))
    
    model.compile(optimizer='adam', loss='sparse_categorical_crossentropy', metrics=['accuracy'])
    return model

# Train and evaluate models for each client
client_models = []
test_accuracies = []
test_precisions = []
test_recalls = []

# Calculate the size of the test dataset for each client
test_dataset_size_per_client = len(x_test) // num_clients  # Calculate size per client
remainder = len(x_test) % num_clients  # Calculate remainder

# Distribute test dataset sizes among clients
test_dataset_sizes = [test_dataset_size_per_client + (1 if i < remainder else 0) for i in range(num_clients)]

# Determine the maximum test dataset size
max_test_dataset_size = max(test_dataset_sizes)

# Step 2: Send the maximum test dataset size to the server
# Create a new socket and connect to the server
client_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
client_socket.connect(server_address)

# Send maximum test dataset size to the server
data = {
    'client_id': 1,  # Set your client ID
    'test_dataset_size': max_test_dataset_size
}
serialized_data = json.dumps(data).encode('utf-8')
client_socket.sendall(serialized_data)

# Close the connection after sending the test dataset size
client_socket.close()

print(f"\nCLIENTS MODELS TRAINING")
# Train all client models and print performance metrics
models_folder = 'clients_models'
os.makedirs(models_folder, exist_ok=True)  # Create the folder if it doesn't exist

# Train all client models and print performance metrics
for i, (client_x_train, client_y_train) in enumerate(client_datasets):
    print(f"Client {i + 1} ...")
    client_model = create_model(input_shape=(28, 28))
    client_model.fit(client_x_train, client_y_train, epochs=num_epochs, batch_size=size_batch, verbose=0)
    
    test_loss, test_acc = client_model.evaluate(x_test, y_test, verbose=0)
    y_pred = np.argmax(client_model.predict(x_test), axis=-1)
    precision = precision_score(y_test, y_pred, average='macro')
    recall = recall_score(y_test, y_pred, average='macro')
    
    print(f"Test accuracy: {test_acc}")
    print(f"Test precision: {precision}")
    print(f"Test recall: {recall}")
    print()
    client_models.append(client_model)
    test_accuracies.append(test_acc)
    test_precisions.append(precision)
    test_recalls.append(recall)

# Reconnect to the server to receive the query and send performance data
client_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
client_socket.connect(server_address)

# Receive the query data from the server
serialized_query_data = client_socket.recv(4096)
query_data = json.loads(serialized_query_data.decode('utf-8'))

image_index = query_data['image_index']
sampled_clients = query_data['client_ids']

# Display the image at the specified index
plt.imshow(x_test[image_index], cmap='gray')
plt.title(f"Image at index {image_index}")
plt.axis('off')
plt.show(block=False)  # Show the plot without blocking the execution
time.sleep(0)  # Wait for 0 seconds
plt.close()  # Close the plot

print()
print("RESULTS PREDICTION AND TRANSMISSION TO SERVER")
# Print prediction results for each client in the sampled_clients and send performance data
for i in range(num_clients):
    if i + 1 in sampled_clients:
        client_model = client_models[i]
        client_x_train, client_y_train = client_datasets[i]
        
        # Check if the image index is within the client's dataset size
        if image_index < test_dataset_sizes[i]:
            # Test the model with the specified image from the test set
            x = x_test[image_index]
            x = x.reshape(1, 28, 28)  # Reshape to the expected input shape

            start_time = time.time()  # Start measuring transmission time
            y = client_model.predict(x)
            end_time = time.time()  # End measuring transmission time

            predicted_class = np.argmax(y)
            confidence = float(np.max(y))  # Extract the confidence score for the predicted class            

            # Print prediction results for each client
            print(f"Client {i + 1}...")
            print(f"Dataset size: {test_dataset_sizes[i]}")
            print(f"Prediction (image index {image_index}): {predicted_class}")
            print(f"Confidence score: {confidence}")            
        else:
            # Set predicted class to -1 as an error
            predicted_class = -1
            confidence = 0.0  # Set confidence score to 0 as an error
            print(f"Client {i + 1}...")
            print(f"Dataset size: {test_dataset_sizes[i]}")
            print(f"Prediction (image index {image_index}): {predicted_class}")
            print(f"Confidence score: {confidence}")
 
        # Calculate performance metrics only for sampled clients
        test_acc = test_accuracies[i]
        test_prec = test_precisions[i]
        test_rec = test_recalls[i]

        # Measure transmission time and throughput
        transmission_time = end_time - start_time
        data_size = x.nbytes / 1024  # Data size in KB
        throughput = data_size / transmission_time  # throughput in KB/s
        print(f"Transmission time: {transmission_time:.6f} seconds")
        #print(f"Data size: {data_size:.6f} KB")
        #print(f"Throughput: {throughput:.6f} KB/s")
        
        # Send performance data to the server along with transmission metrics
        data = {
            'client_id': i + 1,
            'weight_dataset' : test_dataset_sizes[i],
            'test_accuracy': test_acc,
            'test_precision': test_prec,
            'test_recall': test_rec,
            'predicted_class': int(predicted_class),
            'confidence': confidence,  # Include confidence score            
            'transmission_time': transmission_time,
            'data_size': data_size,
            'throughput': throughput
        }
        serialized_data = json.dumps(data).encode('utf-8')
        client_socket.sendall(serialized_data)

        # Receive acknowledgment from the server
        acknowledgment = client_socket.recv(4096).decode('utf-8')
        print(f"Server acknowledgment: {acknowledgment}")
        print()

    else:
        # Set test metrics to 0 for clients not in sampled_clients
        test_acc = 0
        test_prec = 0
        test_rec = 0

        # Set predicted class to -1 for clients not in sampled_clients
        predicted_class = -1
        confidence = 0.60  # Set confidence score to 0 for clients not in sampled_clients

        # Set transmission metrics to 0 for clients not in sampled_clients
        transmission_time = 0
        data_size = 0
        throughput = 0

        # Send performance data to the server for non-sampled clients
        data = {
            'client_id': i + 1,
            'weight_dataset' : test_dataset_sizes[i],			#'weight_dataset': 0,
            'test_accuracy': test_acc,
            'test_precision': test_prec,
            'test_recall': test_rec,
            'predicted_class': int(predicted_class),
            'confidence': confidence,  # Include confidence score            
            'transmission_time': transmission_time,
            'data_size': data_size,
            'throughput': throughput
        }
        serialized_data = json.dumps(data).encode('utf-8')
        client_socket.sendall(serialized_data)

        # Receive acknowledgment from the server
        acknowledgment = client_socket.recv(4096).decode('utf-8')
        print(f"\nServer acknowledgment: {acknowledgment}")
        
    time.sleep(2)  # 2-second delay

print(f"Terminating connection...")
time.sleep(5)  # Print the message for 5 seconds
print()

# Close the client connection
client_socket.close()

