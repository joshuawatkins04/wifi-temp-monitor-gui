import argparse
import struct
from datetime import datetime

import matplotlib.dates as mdates
import matplotlib.pyplot as plt
import numpy as np

entry_format = "qff"
entry_size = struct.calcsize(entry_format)

def read_binary_log(filename):
  data = []
  try:
    with open(filename, "rb") as file:
      while chunk := file.read(entry_size):
        timestamp, temperature, humidity = struct.unpack(entry_format, chunk)
        data.append((datetime.fromtimestamp(timestamp), temperature, humidity))
  except FileNotFoundError:
    print(f"Error: File '{filename}' not found.")
    return
  except Exception as e:
    print(f"Error reading file: {e}")
    return []
  return data

def plot_data(data):
  if not data:
    print("No data to plot.")
    return

  timestamps = [entry[0] for entry in data]
  temperatures = [entry[1] for entry in data]
  humidities = [entry[2] for entry in data]

  plt.figure(figsize=(10, 6))

  # Temperature Graph
  plt.subplot(2, 1, 1)
  plt.plot(timestamps, temperatures, label="Temperature (°C)", color="red")
  plt.ylabel("Temperature (°C)")
  plt.legend()
  plt.grid(True)

  plt.gca().xaxis.set_major_formatter(mdates.DateFormatter("%H:%M"))
  plt.gca().xaxis.set_major_locator(mdates.MinuteLocator(interval=30))

  # Humidity Graph
  plt.subplot(2, 1, 2)
  plt.plot(timestamps, humidities, label="Humidity (%)", color="blue")
  plt.ylabel("Humidity (%)")
  plt.legend()
  plt.grid(True)

  plt.gca().xaxis.set_major_formatter(mdates.DateFormatter("%H:%M"))
  plt.gca().xaxis.set_major_locator(mdates.MinuteLocator(interval=30))

  plt.xlabel("Timestamp")
  plt.tight_layout()
  plt.show()

def calculate_statistics(data):
  if not data:
    print("No data available for analysis")
    return
  
  temperatures = [entry[1] for entry in data]
  humidities = [entry[2] for entry in data]

  temperature_mean = np.mean(temperatures)
  temperature_median = np.median(temperatures)
  temperature_min = np.min(temperatures)
  temperature_max = np.max(temperatures)

  humidity_mean = np.mean(humidities)
  humidity_median = np.median(humidities)
  humidity_min = np.min(humidities)
  humidity_max = np.max(humidities)

  print("\n--- Temperature Statistics ---")
  print(f"Mean: {temperature_mean:.2f} °C")
  print(f"Median: {temperature_median:.2f} °C")
  print(f"Min: {temperature_min:.2f} °C")
  print(f"Max: {temperature_max:.2f} °C")

  print("\n--- Humidity Statistics ---")
  print(f"Mean: {humidity_mean:.2f} %")
  print(f"Median: {humidity_median:.2f} %")
  print(f"Min: {humidity_min:.2f} %")
  print(f"Max: {humidity_max:.2f} %\n")

if __name__ == "__main__":
  parser = argparse.ArgumentParser(description="Analyse and plot temperature and humidity data")
  parser.add_argument("logfile", type=str, help="Path to the binary log file")

  args = parser.parse_args()
  log_file_path = args.logfile
  data = read_binary_log(log_file_path)
  
  calculate_statistics(data)
  plot_data(data)