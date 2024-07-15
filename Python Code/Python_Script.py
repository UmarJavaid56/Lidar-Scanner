import serial  # Import serial library to handle serial communication
import math  # Import math for mathematical operations
import open3d as o3d  # Import Open3D for 3D data processing and visualization
import numpy as np  # Import numpy for numerical operations and handling arrays

# Set up serial connection using specified COM port and baud rate
serial_port = serial.Serial("COM8", 115200)
print("Opening:", serial_port.name)  # Print the name of the opened serial port

# Define the number of vertical layers to scan
vertical_layers = 3
# Define the number of samples to be taken per full rotation
rotation_samples = 64

point_list = []  # Initialize an empty list to store the point data
file = open("./pointcloud.xyz", "w")  # Open a file in write mode to save point data

# Loop through each vertical layer of the scan
for layer in range(vertical_layers):
    print(f"Measurement {layer+1} of {vertical_layers}")  # Print the current layer of measurement

    # Loop through each sample in a full rotation
    for sample in range(rotation_samples):
        distance = 0  # Initialize distance to zero
        serial_port.write(b'u')  # Send a byte to trigger the measurement
        while True:
            received_char = serial_port.read()  # Read one byte from the serial port
            decoded_char = received_char.decode()  # Decode the byte to a string
            if decoded_char == ",":  # Check if the end of a measurement is signaled by a comma
                break
            else:
                # Concatenate strings to form the complete distance reading
                distance = int(str(distance) + str(decoded_char))
                # If the distance exceeds 4 meters, cap it (sensor's maximum range)
                if distance > 4000:
                    distance = 4000  

        # Calculate x, y coordinates from the polar coordinates (distance and angle)
        angle_degrees = sample * (360.0 / rotation_samples)
        x = round(distance * math.cos((math.pi / 180.0) * angle_degrees), 1)
        y = round(distance * math.sin((math.pi / 180.0) * angle_degrees), 1)
        
        # Append the Cartesian coordinates and the current layer height to the point list
        point_list.append([x, y, layer * 100])
        # Print out the current measurement
        print(f"Measurement {layer * rotation_samples + sample + 1}: {point_list[-1]}")
    
print("Closing:", serial_port.name)  # Notify that the serial port is being closed

# Write each point to the file in .xyz format
for point_index in range(rotation_samples * vertical_layers):
    point_str = '%s %s %s\n' % (point_list[point_index][0], point_list[point_index][1], point_list[point_index][2])
    file.write(point_str)

serial_port.close()  # Close the serial port
file.close()  # Close the file

### VISUALIZATION IN OPEN3D

# Load the saved point cloud from the file
point_cloud = o3d.io.read_point_cloud("./pointcloud.xyz", format='xyz')
connection_lines = []  # Initialize a list to store lines for visualization

# Create lines between consecutive points in each layer
for layer in range(vertical_layers):
    layer_offset = rotation_samples * layer
    for i in range(rotation_samples):
        if i < rotation_samples - 1:
            connection_lines.append([i + layer_offset, i + 1 + layer_offset])
        else:
            connection_lines.append([i + layer_offset, layer_offset])

# Create lines between corresponding points in consecutive layers
for layer in range(vertical_layers - 1):
    layer_start = layer * rotation_samples
    next_layer_start = (layer + 1) * rotation_samples
    for i in range(rotation_samples):
        connection_lines.append([layer_start + i, next_layer_start + i])

point_array = np.asarray(point_cloud.points)  # Convert point cloud data to a numpy array
print(point_array)  # Print the numpy array of points

# Create a line set from the points and lines for visualization
line_set = o3d.geometry.LineSet(
    points=o3d.utility.Vector3dVector(point_array),
    lines=o3d.utility.Vector2iVector(connection_lines),
)

# Draw the geometries in a visualization window
o3d.visualization.draw_geometries([line_set], width=1280, height=720)
