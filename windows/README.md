# **Temperature Monitor GUI**

A simple Windows-based GUI application for monitoring temperature data over UDP, designed to integrate with a microcontroller device such as an Arduino or ESP32. This application also supports automatic startup integration by creating a shortcut in the Windows Startup folder.

---

## **Features**

* **Temperature Monitoring**: Receives temperature data from a microcontroller device over UDP.  
* **Customizable GUI**: Displays real-time temperature data with a user-friendly interface.  
* **Startup Integration**: Automatically ensures the application runs at system startup by creating a shortcut in the Windows Startup folder.  
* **UDP Configuration**: Configurable IP and port settings for seamless communication with the microcontroller device.

---

## **How It Works**

1. **UDP Communication**:  
   * The application initializes a UDP connection to the specified IP and port.  
   * Data is received and displayed in real time.  
2. **Startup Integration**:  
   * On launch, the program checks for the presence of a batch file (`windows_autostart.bat`) in a specific directory.  
   * If the batch file exists, it executes the file to ensure a shortcut to the application is created in the Windows Startup folder.  
3. **GUI Display**:  
   * The program uses the Windows API to create a graphical window for displaying the temperature data.  
   * Fonts and graphical elements are dynamically managed.

---

## **Getting Started**

### **Prerequisites**

* Windows operating system  
* A compiled `.exe` file of the application  
* The batch file (`windows_autostart.bat`) for managing startup integration

### **Installation**

1. **Clone the Repository** (or copy the source files):  
   git clone https://github.com/joshuawatkins04/wifi-temp-monitor-gui.git  
2. **Compile the Source Code**:  
   * Use a C compiler (e.g., GCC or MSVC) to compile the project.  
   * In the terminal, navigate to the main project directory.  
   * Ensure the necessary header files (`serial.h`, `window.h`, `resource.h`) are in the same directory as the source code.  
   * Use the following command to compile:  
     gcc src/main.c src/udp.c src/window.c src/server.c src/mongoose.c assets/icon.o 
     -o TemperatureMonitor -lgdi32 -lws2_32 -mwindows -lpthread
3. **Run the Application**:  
   * Make sure the compiled `.exe` file is in the location specified in the `windows_autostart.bat` file.  
   * Double-click the `.exe` to launch the application.

---

## **Usage**

1. On launch, the program will:  
   * Check and create a Windows Startup shortcut.  
   * Establish a UDP connection with the microcontroller device.  
   * Open the GUI to display real-time temperature data.  
2. **Modify IP and Port**:

Change the IP and port directly in the source code (`main.c`):  
const char \*ip \= "192.168.0.37";

* int port \= 5005;  
  * Recompile the program.  
3. **Batch File Customization**:  
   * Update the batch file to reflect the correct path to your `.exe`:  
     set EXE\_PATH="C:\\Path\\To\\TemperatureMonitor.exe"

---

## **Troubleshooting**

* **Console Not Displaying Logs**:  
  * Ensure a console window is attached using the `AllocConsole()` function in `main.c`.  
* **Startup Shortcut Not Created**:  
  * Check the file path in the batch file.  
  * Verify the application has permission to modify the Startup folder.

---

## **Contributing**

Contributions are welcome\! Please open an issue or submit a pull request.

## **Author**

Joshua Watkins