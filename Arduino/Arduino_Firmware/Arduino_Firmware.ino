/*
 * HC-SR04 Ultrasonic Sensor Data Acquisition Firmware
 * Platform: Arduino Uno R3
 * Purpose: High-quality distance data collection for NN training
 */

#define TRIG_PIN 9
#define ECHO_PIN 10

// Measurement parameters
#define NUM_SAMPLES 7
#define MAX_DISTANCE_CM 1050
#define SOUND_SPEED_CM_PER_US 0.0343 / 2.0  // round-trip correction

// Timing
#define MEASUREMENT_INTERVAL_MS 100  // 10 Hz sampling

unsigned long lastMeasurementTime = 0;

// Function to trigger ultrasonic pulse
void triggerUltrasonic() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(5);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
}

// Simple insertion sort for small arrays
void sortArray(float *arr, int size) {
  for (int i = 1; i < size; i++) {
    float key = arr[i];
    int j = i - 1;
    while (j >= 0 && arr[j] > key) {
      arr[j + 1] = arr[j];
      j--;
    }
    arr[j + 1] = key;
  }
}

// Acquire a single distance measurement (cm)
// Returns -1 if invalid
float getDistanceCm() {
  triggerUltrasonic();

  unsigned long duration = pulseIn(
    ECHO_PIN,
    HIGH,
    (unsigned long)(MAX_DISTANCE_CM / SOUND_SPEED_CM_PER_US)
  );

  if (duration == 0) {
    return -1.0; // No echo received
  }

  return duration * SOUND_SPEED_CM_PER_US;
}

void setup() {
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  Serial.begin(115200);
  while (!Serial);

  // CSV header
  Serial.println("timestamp_ms, raw_valid_samples, filtered_distance_cm");
}

void loop() {
  unsigned long currentTime = millis();

  if (currentTime - lastMeasurementTime >= MEASUREMENT_INTERVAL_MS) {
    lastMeasurementTime = currentTime;

    float samples[NUM_SAMPLES];
    int validCount = 0;

    // Collect samples
    for (int i = 0; i < NUM_SAMPLES; i++) {
      float d = getDistanceCm();
      if (d > 0 && d <= MAX_DISTANCE_CM) {
        samples[validCount++] = d;
      }
      delay(10);  // small gap to avoid echo overlap
    }

    float filteredDistance = -1.0;

    if (validCount >= 3) {
      // Sort valid samples
      sortArray(samples, validCount);

      // Median-based mean (discard extremes if possible)
      int start = (validCount > 4) ? 1 : 0;
      int end   = (validCount > 4) ? validCount - 1 : validCount;

      float sum = 0.0;
      int count = 0;

      for (int i = start; i < end; i++) {
        sum += samples[i];
        count++;
      }

      filteredDistance = sum / count;
    }

    // Output CSV row
    Serial.print(currentTime);
    Serial.print(", ");
    Serial.print(validCount);
    Serial.print(", ");

    if (filteredDistance < 0) {
      Serial.println("NaN");
    } else {
      Serial.println(filteredDistance, 2);
    }
  }
}
