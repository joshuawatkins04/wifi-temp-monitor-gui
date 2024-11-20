# **iOS Temperature Monitor App**

The iOS Temperature Monitor App allows users to receive real-time temperature data from the ESP32-based sensor on their iPhone or iPad. This app complements the Windows version, providing mobile access to temperature monitoring.

### **Features**

* **Real-Time Updates**: Get continuous, live temperature readings sent from the ESP32 device.  
* **Intuitive Interface**: Displays temperature data in a clean, user-friendly manner.  
* **Mobile Convenience**: Monitor temperatures on-the-go directly from your mobile device.

### **Installation**

* The app can be installed on iOS devices running iOS 11 or later.  
* Compile the app using Xcode. Ensure you have an active Apple Developer account if you plan to install the app on a real device.

### **Requirements**

* **iOS Version**: Requires iOS 11 or later.  
* **ESP32 Connectivity**: The app must be connected to the same WiFi network as the ESP32.

### **Usage**

* **Initial Setup**: After launching the app, enter the WiFi credentials to connect to the ESP32 sensor.  
* **Monitoring**: Once connected, the app will display the current temperature and update regularly.

### **Troubleshooting**

* **Connection Issues**: Make sure both the iOS device and ESP32 are on the same WiFi network.  
* **App Crashing**: Check if the app has the correct permissions to access the network.

### **Development**

The app is developed in Swift, using Xcode for iOS development. Contributions are welcome to enhance the user interface or add new features.