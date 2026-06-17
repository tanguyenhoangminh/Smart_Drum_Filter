#define BLYNK_TEMPLATE_ID "TMPL6fDRXmB_P"
#define BLYNK_TEMPLATE_NAME "00F"
#define BLYNK_AUTH_TOKEN "tf3WvLeQhQuhK1UF3vlQBK_PLFQBvrRu"

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <BlynkSimpleEsp8266.h>

// --- Pins ---
#define PUMP_PIN D1
#define MOTOR_PIN D2
#define PUMP_PIN_Ext D4
#define FLOAT_SWITCH D5

// --- Global Objects ---
ESP8266WebServer server(80);
BlynkTimer timer;
// --- States ---
bool isPumpOn = false;
bool isMotorOn = false;
bool isAutoMode = false;
bool inConfigMode = false;
bool isConfigArmed = false;
unsigned long configArmTime = 0;
// --- Cycle Timers (in seconds) ---
long workCycleSec = 20;     // V3
long restCycleSec = 10;     // V5
long breakdownSec = 0;      // V6

unsigned long autoModeStartTime = 0;
bool isResting = false;     // Tracks if we are in the Rest Cycle

unsigned long cycleStartTime = 0;
unsigned long lastSprayToggleTime = 0;
bool sprayStateOn = false;

void loop() {
    server.handleClient();
    MDNS.update();
    timer.run();

    if (WiFi.status() == WL_CONNECTED) {
        Blynk.run();
    }

    // Switch to Config Portal after 5 seconds of being armed
    if (isConfigArmed && !inConfigMode) {
        if (millis() - configArmTime > 5000) {
            inConfigMode = true;
            isConfigArmed = false; 
        }
    }

    // --- AUTO MODE ---
 // --- AUTO MODE ---
    if (isAutoMode) {
        unsigned long now = millis();
        // 1. Breakdown Timer (V6) Check
        if (breakdownSec > 0 && (now - autoModeStartTime >= breakdownSec * 1000UL)) {
            // Time is up! Revert to Manual Mode
            isAutoMode = false;
            setMotor(false);
            setPump(false);
            extPumpLock(true);
            // Sync UI back to manual state
            Blynk.virtualWrite(V2, 0); 
            return; // Exit auto logic
        }

        // 2. Water Level Override (V4)
        // Assuming FLOAT_SWITCH HIGH means LOW WATER based on your previous code
        if (digitalRead(FLOAT_SWITCH)) { 
            setMotor(false);
            setPump(false);
            extPumpLock(false);
            return; // Pause execution until water is NORMAL
        }

        // 3. Work / Rest Cycle Logic
        unsigned long elapsedInCycle = now - cycleStartTime;

        if (!isResting) {
            extPumpLock(true));
            if (elapsedInCycle < workCycleSec * 1000UL) {
                // Motor (V1) stays ON for the entire Work Cycle
                if (!isMotorOn) setMotor(true);
                if (!isPumpOn) setPump(true);
            } else {
                // Work cycle finished, transition to Rest
                isResting = true;
                cycleStartTime = now;
                setMotor(false);
                setPump(false);
                
            }
        } else if(isResting){
            extPumpLock(false);
            if (elapsedInCycle < restCycleSec * 1000UL) {
                // Keep everything OFF
                if (isMotorOn) setMotor(false);
                if (isPumpOn) setPump(false);
            } else {
                // Rest cycle finished, transition back to Work
                isResting = false;
                cycleStartTime = now;
            }
        }
    }
}
void sendFloatStatus() {
    String status = digitalRead(FLOAT_SWITCH) ? "Low" : "Normal";
        if (WiFi.status() == WL_CONNECTED) {
            Blynk.virtualWrite(V4, status);
        }
}
BLYNK_WRITE(V2) { // Mode
    isAutoMode = param.asInt();

    if (isAutoMode) {
        // AUTOMATIC MODE: Disable manual switches, enable timers
        Blynk.setProperty(V0, "isDisabled", true);
        Blynk.setProperty(V1, "isDisabled", true);
        Blynk.setProperty(V3, "isDisabled", false);
        Blynk.setProperty(V5, "isDisabled", false);
        Blynk.setProperty(V6, "isDisabled", false);

        cycleStartTime = millis();
        autoModeStartTime = millis();
        isResting = false;
    } else {
        // MANUAL MODE: Enable manual switches, disable timers
        Blynk.setProperty(V0, "isDisabled", false);
        Blynk.setProperty(V1, "isDisabled", false);
        Blynk.setProperty(V3, "isDisabled", true);
        Blynk.setProperty(V5, "isDisabled", true);
        Blynk.setProperty(V6, "isDisabled", true);

        setMotor(false);
        setPump(false);
    }
}

BLYNK_WRITE(V3) { workCycleSec = max(1, param.asInt()); } // Work Cycle
BLYNK_WRITE(V5) { restCycleSec = max(1, param.asInt()); } // Rest Cycle
BLYNK_WRITE(V6) { breakdownSec = max(0, param.asInt()); } // Breakdown Countdown

void initialise() {
    pinMode(PUMP_PIN, INPUT_PULLUP);
    delay(50);
    pinMode(PUMP_PIN, OUTPUT);
    digitalWrite(PUMP_PIN, LOW);
    pinMode(MOTOR_PIN, INPUT_PULLUP);
    delay(50);
    pinMode(MOTOR_PIN, OUTPUT);
    digitalWrite(MOTOR_PIN, LOW);
}

void setMotor(bool state) {
    digitalWrite(MOTOR_PIN, state ? HIGH : LOW);
    isMotorOn = state;
    if (WiFi.status() == WL_CONNECTED) {
        Blynk.virtualWrite(V1, state);
    }
}

void setPump(bool state) {
    digitalWrite(PUMP_PIN, state ? HIGH : LOW);
    isPumpOn = state;
    if (WiFi.status() == WL_CONNECTED) {
        Blynk.virtualWrite(V0, state);
    }
}

void extPumpLock(bool state){
    digitalWrite(PUMP_PIN_Ext, state ? LOW : HIGH);
    }
}
// =====================================================
// HTML PAGES
// =====================================================

String getConfigHTML() {
    // 1. Start building the HTML
    String html = "<!DOCTYPE html><html lang=\"en\"><head><meta charset=\"UTF-16\"><meta name=\"viewport\" content=\"width=device-width, initial-scale=1, user-scalable=no\"/><title>Drum Filter System - </title>";    html += "<style>body{text-align:center;font-family:verdana;max-width:400px;margin:0 auto;padding:20px;}div,input{padding:5px;font-size:1em;}input{width:90%;margin:5px 0;border:1px solid #ccc;border-radius:4px;box-sizing:border-box;}button{border:0;border-radius:0.3rem;background-color:#1fa3ec;color:#fff;line-height:2.4rem;font-size:1.2rem;width:100%;cursor:pointer;margin-top:15px;}button:hover{background-color:#0e70a4;}.network{display:flex;justify-content:space-between;align-items:center;padding:10px;border-bottom:1px solid #eee;}.network a{text-decoration:none;color:#1fa3ec;font-weight:bold;cursor:pointer;}.q{color:#888;font-size:0.9em;}h1{margin-bottom:5px;}</style></head>";
    html += "<body><h1>WiFiManager</h1><h3>Network Setup</h3><hr/>";

    // 2. Scan for networks
    int n = WiFi.scanNetworks();
    
    html += "<div style=\"text-align:left;\">";
    if (n == 0) {
        html += "<p style='text-align:center;'>No networks found. Please refresh.</p>";
    } else {
        // 3. Loop through results and populate the list
        for (int i = 0; i < n; ++i) {
            html += "<div class=\"network\"><a href=\"#\" onclick=\"selectSSID('";
            html += WiFi.SSID(i);
            html += "')\">";
            html += WiFi.SSID(i);
            html += "</a> <span class=\"q\">";
            html += String(WiFi.RSSI(i));
            html += " dBm ";
            html += (WiFi.encryptionType(i) == ENC_TYPE_NONE) ? "🔓" : "🔒";
            html += "</span></div>";
        }
    }
    html += "</div><br/>";

    // 4. Append the rest of the form
    html += "<form method=\"POST\" action=\"/save\">";
    html += "<label for=\"ssid\"><b>SSID</b></label><input id=\"ssid\" name=\"ssid\" length=\"32\" placeholder=\"Network Name\" value=\"\">";
    html += "<label for=\"password\"><b>Password</b></label><input id=\"password\" name=\"password\" length=\"64\" type=\"password\" placeholder=\"Password\">";
    html += "<button type=\"submit\">Save</button></form><br/>";
    html += "<button style=\"background-color:#d9534f;\" onclick=\"window.location.href='/cancel_config'\">Cancel & Return</button>";
    html += "<script>function selectSSID(ssid){document.getElementById('ssid').value=ssid;}</script></body></html>";

    return html;
}

String getDashboardHTML() {
    String water = digitalRead(FLOAT_SWITCH) ? "Low Water" : "Sufficient";
    String pumpBadge = isPumpOn ? "<span class='badge badge-on'>ON</span>" : "<span class='badge badge-off'>OFF</span>";
    String motorBadge = isMotorOn ? "<span class='badge badge-on'>ON</span>" : "<span class='badge badge-off'>OFF</span>";

    String html = "<!DOCTYPE html><html><head>";
    html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
    
    if (isConfigArmed) {
        html += "<meta http-equiv='refresh' content='5'>"; 
    }
    
    html += "<style>";
    html += "body { font-family: 'Segoe UI', Tahoma, sans-serif; background-color: #f4f7f6; color: #333; margin: 0; padding: 20px; display: flex; justify-content: center; }";
    html += ".card { background: white; border-radius: 16px; padding: 30px; box-shadow: 0 8px 24px rgba(0,0,0,0.1); max-width: 400px; width: 100%; text-align: center; }";
    html += "h2 { color: #0056b3; margin-top: 0; margin-bottom: 20px; }";
    html += ".status-box { background: #e9ecef; padding: 15px; border-radius: 12px; margin-bottom: 20px; text-align: left; }";
    html += ".status-box p { margin: 10px 0; font-weight: 500; display: flex; justify-content: space-between; align-items: center; }";
    html += ".badge { padding: 6px 12px; border-radius: 20px; font-size: 0.85em; font-weight: bold; }";
    html += ".badge-on { background: #d4edda; color: #155724; }";
    html += ".badge-off { background: #f8d7da; color: #721c24; }";
    html += ".btn-group { display: flex; justify-content: space-between; margin-bottom: 15px; gap: 10px; }";
    html += ".btn { flex: 1; padding: 12px; text-decoration: none; color: white; border-radius: 8px; font-weight: bold; transition: 0.2s; box-sizing: border-box; }";
    html += ".btn-on { background-color: #28a745; } .btn-on:hover { background-color: #218838; }";
    html += ".btn-off { background-color: #dc3545; } .btn-off:hover { background-color: #c82333; }";
    html += ".btn-action { display: block; width: 100%; padding: 14px; margin-top: 15px; text-decoration: none; color: white; border-radius: 8px; font-weight: bold; box-sizing: border-box; transition: 0.2s;}";
    html += ".btn-connect { background-color: #17a2b8; } .btn-connect:hover { background-color: #138496; }";
    html += ".btn-global { background-color: #6c757d; } .btn-global:hover { background-color: #5a6268; }";
    html += ".btn-cancel { background-color: #dc3545; animation: pulse 1s infinite; }"; 
    html += "@keyframes pulse { 0% { opacity: 1; } 50% { opacity: 0.7; } 100% { opacity: 1; } }";
    html += "</style></head><body>";

    html += "<div class='card'>";
    html += "<h2>Drum Filter Control</h2>";
    
    if (isConfigArmed) {
        html += "<div style='background-color:#fff3cd; color:#856404; padding:15px; border-radius:8px; margin-bottom:20px; border: 1px solid #ffeeba;'>";
        html += "<b>Loading...</b><br>Opening portal in 5 seconds...<br><i>Click Cancel below to abort.</i>";
        html += "</div>";
    }
    
    html += "<div class='status-box'>";
    html += "<p>Spray" + pumpBadge + "</p>";
    html += "<p>Filtration" + motorBadge + "</p>";
    html += "<p>Water Level <span><strong>" + water + "</strong></span></p>";
    html += "</div>";

    html += "<div class='btn-group'>";
    html += "<a href='/spray_on' class='btn btn-on'>Spray ON</a>";
    html += "<a href='/spray_off' class='btn btn-off'>Spray OFF</a>";
    html += "</div>";

    html += "<div class='btn-group'>";
    html += "<a href='/motor_on' class='btn btn-on'>Motor ON</a>";
    html += "<a href='/motor_off' class='btn btn-off'>Motor OFF</a>";
    html += "</div>";

    html += "<a href='/connect' class='btn-action btn-connect'>Connect & Refresh</a>";

    if (isConfigArmed) {
        html += "<a href='/global' class='btn-action btn-cancel'>Cancel Config</a>";
    } else {
        html += "<a href='/global' class='btn-action btn-global'>Global Wi-Fi Config</a>";
    }

    html += "</div></body></html>";
    return html;
}

// =====================================================
// SERVER ROUTES
// =====================================================
void setupServer() {
    server.on("/", []() { 
        if (inConfigMode) {
            server.send(200, "text/html", getConfigHTML()); // Now calls the dynamic function
        } else {
            server.send(200, "text/html", getDashboardHTML()); 
        }
    });

    server.on("/spray_on", [](){ setPump(true); server.sendHeader("Location", "/"); server.send(303); });
    server.on("/spray_off", [](){ setPump(false); server.sendHeader("Location", "/"); server.send(303); });

    server.on("/motor_on", [](){ setMotor(true); server.sendHeader("Location", "/"); server.send(303); });
    server.on("/motor_off", [](){ setMotor(false); server.sendHeader("Location", "/"); server.send(303); });

    server.on("/connect", [](){ 
        server.sendHeader("Location", "/"); 
        server.send(303); 
        ESP.restart();
        delay(10000); 
    });

    server.on("/global", []() {
        if (!inConfigMode) {
            isConfigArmed = !isConfigArmed; 
            if (isConfigArmed) {
                configArmTime = millis(); 
            }
        }
        server.sendHeader("Location", "/"); 
        server.send(303);
    });

    server.on("/cancel_config", []() {
        inConfigMode = false;
        isConfigArmed = false;
        server.sendHeader("Location", "/"); 
        server.send(303);
    });

    server.on("/save", HTTP_POST, []() {
        String ssid = server.arg("ssid");
        String pass = server.arg("password");

        Serial.println("Received WiFi Credentials:");
        Serial.println("SSID: " + ssid);
        Serial.println("PASS: " + pass);

        String responseHTML = "<!DOCTYPE html><html><head><meta name='viewport' content='width=device-width, initial-scale=1'/><meta http-equiv='refresh' content='3;url=/'><style>body{text-align:center;font-family:verdana;margin-top:50px;}</style></head><body><h2>Credentials Saved!</h2><p>Attempting to connect... returning to dashboard.</p></body></html>";
        server.send(200, "text/html", responseHTML);

        inConfigMode = false; 
        isConfigArmed = false;

        WiFi.begin(ssid, pass);
    });

    server.begin();
}

// =====================================================
// SETUP
// =====================================================
void setup() {
    Serial.begin(9600);
    pinMode(PUMP_PIN, OUTPUT);
    pinMode(FLOAT_SWITCH, INPUT_PULLUP);
    digitalWrite(PUMP_PIN, LOW);

    initialise(); 

    // Setup WiFi as Access Point & Station
    WiFi.mode(WIFI_AP_STA);
    WiFi.softAP("Drum Filter System Configuration", "V1im3n3t43r3o3");
    
    if (MDNS.begin("drumfilter")) {
        Serial.println("MDNS responder started");
    }

    Blynk.config(BLYNK_AUTH_TOKEN);

    timer.setInterval(2000L, sendFloatStatus);
    setupServer();
}
