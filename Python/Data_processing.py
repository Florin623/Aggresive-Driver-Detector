import pandas as pd
import glob
import os

# 1. Define the folder where your CSV files are stored
input_folder = 'sensor_data' # Change this to your folder path
output_file = 'master_training_data.csv'

# 2. Get a list of all CSV files
all_files = glob.glob(os.path.join(input_folder, "*.csv"))

master_list = []

print(f"Found {len(all_files)} files. Starting merge...")

for filename in all_files:
    # Read the CSV
    # We use skipinitialspace=True because your format has spaces after commas
    df = pd.read_csv(filename, skipinitialspace=True)
    
    # Standardize column names
    # Your header has a trailing '30' or '35', we will rename them to something clean
    df.columns = ['timestamp_ms', 'distance_cm', 'valid_samples', 'ground_truth']
    
    # Ensure data types are correct
    df['timestamp_ms'] = df['timestamp_ms'].astype(int)
    df['distance_cm'] = df['distance_cm'].astype(float)
    df['valid_samples'] = df['valid_samples'].astype(int)
    df['ground_truth'] = df['ground_truth'].astype(int)
    
    master_list.append(df)
    print(f"Added {filename} - Samples: {len(df)}")

# 3. Concatenate all dataframes
full_dataset = pd.concat(master_list, axis=0, ignore_index=True)

# 4. Sort by ground truth and then by timestamp
full_dataset = full_dataset.sort_values(by=['ground_truth', 'timestamp_ms'])

# 5. Save the master file
full_dataset.to_csv(output_file, index=False)

print("-" * 30)
print(f"Success! Master file saved as: {output_file}")
print(f"Total samples collected: {len(full_dataset)}")