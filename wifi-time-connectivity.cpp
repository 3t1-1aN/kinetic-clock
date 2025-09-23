#include <NTPClient.h>
#include <WiFiNINA.h>

// Your network credentials
char ssid[] = "YOUR_SSID";
char pass[] = "YOUR_PASSWORD";

// --- NTP Client Setup ---
const long utcOffsetInSeconds = 0; // For UTC time, 0 offset
// The default NTP server is time.nist.gov
NTPClient ntpClient(WiFi.getMode(), "time.nist.gov", utcOffsetInSeconds);

void setup() {
  Serial.begin(9600);
  while (!Serial);

  WiFi.begin(ssid, pass);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi!");

  ntpClient.begin();
}

void loop() {
  ntpClient.update(); // Update the time from the NTP server
  // The time is now available in ntpClient.getFormattedTime()
  Serial.print("Current Time: ");
  Serial.println(ntpClient.getFormattedTime());

  delay(5000); // Update time every 5 seconds
}
