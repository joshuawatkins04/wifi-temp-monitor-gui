#include <stdio.h>
#include <direct.h>
#include <errno.h>
#include <time.h>

typedef struct
{
  time_t timestamp;
  float temperature;
  float humidity;
} LogEntry;

void createDirectory(const char *folderName)
{
  if (_mkdir(folderName) == -1)
  {
    if (errno != EEXIST)
    {
      perror("Failed to create logs directory");
    }
  }
  else
  {
    printf("Created logs directory: %s\n", folderName);
  }
}

void createLog(float temperature, float humidity)
{
  createDirectory("logs");

  time_t currentTime = time(NULL);
  struct tm *timeinfo = localtime(&currentTime);
  char buffer[40];
  strftime(buffer, sizeof(buffer), "temperature_log_%Y-%m-%d.bin", timeinfo);

  char filePath[256];
  snprintf(filePath, sizeof(filePath), "logs/%s", buffer);

  FILE *file = fopen(filePath, "ab");
  if (!file)
  {
    perror("Failed to open binary log file");
    return;
  }

  LogEntry entry = {currentTime, temperature, humidity};
  fwrite(&entry, sizeof(LogEntry), 1, file);
  fclose(file);

  printf("Logged data to %s: Temperature=%.2f, Humidity=%.2f\n", filePath, temperature, humidity);
}