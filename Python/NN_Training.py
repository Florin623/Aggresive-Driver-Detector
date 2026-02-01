import numpy as np
import pandas as pd
import tensorflow as tf
from tensorflow.keras import layers, models
from sklearn.model_selection import train_test_split
from sklearn.preprocessing import StandardScaler
from sklearn.metrics import mean_absolute_error, mean_squared_error
import matplotlib.pyplot as plt
import random
import os

# ===============================
# Reproducibility configuration
# ===============================
SEED = 42
np.random.seed(SEED)
tf.random.set_seed(SEED)
random.seed(SEED)

# ===============================
# Load dataset
# ===============================
# CSV format assumed:
# timestamp_ms,distance_cm,valid_samples,ground_truth
data = pd.read_csv("hc_sr04_log.csv")

# ===============================
# Input feature construction
# ===============================
# Temporal window size (number of previous samples)
WINDOW_SIZE = 3

# Lists to hold constructed samples
X = []
y = []

# Build temporal input vectors
for i in range(WINDOW_SIZE, len(data)):
    # Current + previous filtered distances
    distances = data.loc[i-WINDOW_SIZE:i, "distance_cm"].values
    
    # Measurement quality at current timestep
    quality = data.loc[i, "valid_samples"]
    
    # Concatenate into one input vector
    features = np.concatenate([distances, [quality]])
    X.append(features)
    
    # Ground truth target
    y.append(data.loc[i, "ground_truth"])

X = np.array(X)
y = np.array(y)

# ===============================
# Dataset split
# ===============================
# Fixed splits to isolate optimization variance
X_train, X_temp, y_train, y_temp = train_test_split(
    X, y, test_size=0.3, random_state=SEED
)

X_val, X_test, y_val, y_test = train_test_split(
    X_temp, y_temp, test_size=0.5, random_state=SEED
)

# ===============================
# Normalization
# ===============================
# Normalize input features using training statistics only
scaler = StandardScaler()
X_train = scaler.fit_transform(X_train)
X_val   = scaler.transform(X_val)
X_test  = scaler.transform(X_test)

# ===============================
# Model definition
# ===============================
model = models.Sequential([
    layers.Input(shape=(X_train.shape[1],)),
    
    # Hidden layers kept small for embedded deployment
    layers.Dense(32, activation='relu'),
    layers.Dense(16, activation='relu'),
    
    # Linear output for regression
    layers.Dense(1, activation='linear')
])

# ===============================
# Compilation
# ===============================
model.compile(
    optimizer=tf.keras.optimizers.Adam(learning_rate=0.001),
    loss='mse',
    metrics=['mae']
)

# ===============================
# Training
# ===============================
early_stopping = tf.keras.callbacks.EarlyStopping(
    monitor='val_loss',
    patience=10,
    restore_best_weights=True
)

history = model.fit(
    X_train, y_train,
    validation_data=(X_val, y_val),
    epochs=200,
    batch_size=32,
    callbacks=[early_stopping],
    verbose=1
)

# ===============================
# Evaluation
# ===============================
y_pred = model.predict(X_test).flatten()

mae  = mean_absolute_error(y_test, y_pred)
rmse = np.sqrt(mean_squared_error(y_test, y_pred))

print(f"Test MAE:  {mae:.2f} cm")
print(f"Test RMSE: {rmse:.2f} cm")

# ===============================
# Baseline comparison
# ===============================
baseline_pred = X_test[:, -2]  # last distance in temporal window
baseline_mae  = mean_absolute_error(y_test, baseline_pred)
baseline_rmse = np.sqrt(mean_squared_error(y_test, baseline_pred))

print(f"Baseline MAE:  {baseline_mae:.2f} cm")
print(f"Baseline RMSE: {baseline_rmse:.2f} cm")

# ===============================
# Visualization
# ===============================
plt.figure(figsize=(10, 5))
plt.plot(y_test, label="Ground Truth")
plt.plot(y_pred, label="NN Prediction")
plt.legend()
plt.xlabel("Sample Index")
plt.ylabel("Distance [cm]")
plt.title("Neural Network Distance Estimation")
plt.show()

# ===============================
# Model export
# ===============================
model.export("ultrasonic_distance_model")
np.save("input_scaler_mean.npy", scaler.mean_)
np.save("input_scaler_scale.npy", scaler.scale_)

