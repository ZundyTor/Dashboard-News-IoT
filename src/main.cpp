#include <SPI.h>
#include <Wire.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "config.h"

// Crear objeto display
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, 
  OLED_MOSI, OLED_CLK, OLED_DC, OLED_RESET, OLED_CS);

// Estructura para almacenar noticias
struct NewsArticle {
  String title;
  String source;
  String publishedAt;
};

// Variables globales - Noticias
NewsArticle newsArticles[5];
int totalNews = 0;
int currentNewsIndex = 0;

// Variables globales - Tiempos
unsigned long lastNewsUpdate = 0;
unsigned long lastDisplayUpdate = 0;
unsigned long lastCloudUpdate = 0;
unsigned long lastCommandCheck = 0;
String lastUpdateTime = "";

// Variables globales - Estado
bool wifiConnected = false;
int failedAttempts = 0;

// Variables globales - Estadísticas (para la nube)
unsigned long uptimeSeconds = 0;
unsigned long totalNewsRequests = 0;
unsigned long successfulRequests = 0;
unsigned long failedRequests = 0;
unsigned long cloudUpdateCount = 0;
unsigned long cloudSuccessCount = 0;
unsigned long cloudFailCount = 0;
unsigned long commandsReceived = 0;
unsigned long commandsExecuted = 0;

// Variables configurables remotamente
int currentUpdateInterval = NEWS_UPDATE_INTERVAL;
String currentCategory = String(NEWS_CATEGORY);
String currentCountry = String(NEWS_COUNTRY);

// ===================================
// FUNCIONES DE UTILIDAD
// ===================================

String getTimeString() {
  unsigned long totalSeconds = millis() / 1000;
  int hours = (totalSeconds / 3600) % 24;
  int minutes = (totalSeconds / 60) % 60;
  int seconds = totalSeconds % 60;
  
  char timeStr[9];
  sprintf(timeStr, "%02d:%02d:%02d", hours, minutes, seconds);
  return String(timeStr);
}

void drawProgressBar(int x, int y, int width, int height, int progress) {
  display.drawRect(x, y, width, height, SSD1306_WHITE);
  int fillWidth = (width - 2) * progress / 100;
  if (fillWidth > 0) {
    display.fillRect(x + 1, y + 1, fillWidth, height - 2, SSD1306_WHITE);
  }
}

String truncateString(String str, int maxLength) {
  if (str.length() <= maxLength) return str;
  return str.substring(0, maxLength - 3) + "...";
}

int categoryToNumber(String category) {
  if (category == "technology") return 0;
  if (category == "business") return 1;
  if (category == "sports") return 2;
  if (category == "entertainment") return 3;
  if (category == "health") return 4;
  if (category == "science") return 5;
  if (category == "general") return 6;
  return 0;
}

String numberToCategory(int num) {
  switch(num) {
    case 0: return "technology";
    case 1: return "business";
    case 2: return "sports";
    case 3: return "entertainment";
    case 4: return "health";
    case 5: return "science";
    case 6: return "general";
    default: return "technology";
  }
}

// ===================================
// FUNCIONES DE DISPLAY
// ===================================

void displayMessage(String title, String message, bool showProgress = false, int progress = 0) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  
  display.setCursor(0, 0);
  display.println(title);
  display.drawLine(0, 10, 128, 10, SSD1306_WHITE);
  
  display.setCursor(0, 14);
  display.println(message);
  
  if (showProgress) {
    drawProgressBar(10, 45, 108, 8, progress);
  }
  
  display.drawLine(0, 54, 128, 54, SSD1306_WHITE);
  display.setCursor(0, 56);
  display.print("T: ");
  display.print(getTimeString());
  
  display.display();
}

void displayNewsImproved(NewsArticle article, int index, int total) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  
  // Header
  display.setCursor(0, 0);
  String catShort = currentCategory.substring(0, 4);
  catShort.toUpperCase();
  display.print(catShort);
  display.print(" [");
  display.print(index + 1);
  display.print("/");
  display.print(total);
  display.print("]");
  
  display.setCursor(100, 0);
  if (wifiConnected) {
    display.print("WiFi");
  } else {
    display.print("--");
  }
  
  display.drawLine(0, 9, 128, 9, SSD1306_WHITE);
  
  // Fuente
  display.setCursor(0, 11);
  String source = truncateString(article.source, 21);
  display.print(source);
  
  display.drawLine(0, 19, 128, 19, SSD1306_WHITE);
  
  // Titular con word wrap
  display.setCursor(0, 21);
  String title = article.title;
  int lineHeight = 8;
  int maxLines = 4;
  int currentLine = 0;
  int startPos = 0;
  int maxCharsPerLine = 21;
  
  while (startPos < title.length() && currentLine < maxLines) {
    String line = title.substring(startPos, min((int)title.length(), startPos + maxCharsPerLine));
    
    if (startPos + maxCharsPerLine < title.length()) {
      int lastSpace = line.lastIndexOf(' ');
      if (lastSpace > 0 && lastSpace < line.length()) {
        line = title.substring(startPos, startPos + lastSpace);
        startPos += lastSpace + 1;
      } else {
        startPos += maxCharsPerLine;
      }
    } else {
      startPos = title.length();
    }
    
    display.setCursor(0, 21 + (currentLine * lineHeight));
    display.println(line);
    currentLine++;
  }
  
  // Footer
  display.drawLine(0, 54, 128, 54, SSD1306_WHITE);
  
  display.setCursor(0, 56);
  display.setTextSize(1);
  display.print("Up:");
  display.print(lastUpdateTime);
  
  // Indicador de página
  int dotX = 90;
  for (int i = 0; i < total && i < 5; i++) {
    if (i == index) {
      display.fillCircle(dotX + (i * 8), 59, 2, SSD1306_WHITE);
    } else {
      display.drawCircle(dotX + (i * 8), 59, 2, SSD1306_WHITE);
    }
  }
  
  display.display();
}

void displayError(String error, bool showRetry = false) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  
  display.drawLine(10, 10, 20, 20, SSD1306_WHITE);
  display.drawLine(20, 10, 10, 20, SSD1306_WHITE);
  
  display.setCursor(30, 12);
  display.println("ERROR");
  display.drawLine(0, 24, 128, 24, SSD1306_WHITE);
  
  display.setCursor(0, 28);
  display.println(error);
  
  if (showRetry) {
    display.setCursor(0, 50);
    display.print("Reintento...");
  }
  
  display.display();
}

void displayCommandReceived(String command) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  
  // Icono de comando (flecha hacia abajo)
  display.fillTriangle(20, 10, 15, 20, 25, 20, SSD1306_WHITE);
  
  display.setCursor(35, 12);
  display.println("COMANDO");
  display.drawLine(0, 24, 128, 24, SSD1306_WHITE);
  
  display.setCursor(0, 28);
  display.println("Recibido:");
  display.println();
  display.println(command);
  
  display.display();
  delay(2000);
}

// ===================================
// FUNCIONES DE WiFi
// ===================================

bool connectWiFi() {
  Serial.println("\n=================================");
  Serial.println("Conectando a WiFi...");
  Serial.print("SSID: ");
  Serial.println(WIFI_SSID);
  Serial.println("=================================\n");
  
  displayMessage("WiFi", "Conectando...\n\n" + String(WIFI_SSID), true, 0);
  
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  
  int attempts = 0;
  int maxAttempts = 30;
  
  while (WiFi.status() != WL_CONNECTED && attempts < maxAttempts) {
    delay(500);
    Serial.print(".");
    attempts++;
    
    int progress = (attempts * 100) / maxAttempts;
    displayMessage("WiFi", "Conectando...\n\n" + String(WIFI_SSID), true, progress);
  }
  
  Serial.println();
  
  if (WiFi.status() == WL_CONNECTED) {
    wifiConnected = true;
    failedAttempts = 0;
    
    Serial.println("✓ WiFi conectado!");
    Serial.print("  IP: ");
    Serial.println(WiFi.localIP());
    Serial.print("  RSSI: ");
    Serial.print(WiFi.RSSI());
    Serial.println(" dBm\n");
    
    displayMessage("WiFi", "Conectado!\n\nIP:\n" + WiFi.localIP().toString(), true, 100);
    delay(2000);
    return true;
  } else {
    wifiConnected = false;
    failedAttempts++;
    
    Serial.println("✗ Error: No se pudo conectar");
    displayError("No WiFi", true);
    delay(3000);
    return false;
  }
}

// ===================================
// FUNCIONES DE NewsAPI
// ===================================

bool fetchNews() {
  totalNewsRequests++;
  
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("✗ WiFi desconectado");
    wifiConnected = false;
    failedRequests++;
    return false;
  }
  
  wifiConnected = true;
  
  Serial.println("\n=================================");
  Serial.println("Obteniendo noticias de NewsAPI");
  Serial.print("Request #");
  Serial.println(totalNewsRequests);
  Serial.println("=================================");
  
  displayMessage("NewsAPI", "Descargando...", true, 30);
  
  String url = "https://newsapi.org/v2/top-headlines?country=";
  url += currentCountry;
  url += "&category=";
  url += currentCategory;
  url += "&pageSize=5";
  url += "&apiKey=";
  url += NEWS_API_KEY;
  
  Serial.print("País: ");
  Serial.println(currentCountry);
  Serial.print("Categoría: ");
  Serial.println(currentCategory);
  Serial.println();
  
  WiFiClientSecure client;
  client.setInsecure();
  client.setTimeout(15000);
  
  HTTPClient http;
  http.begin(client, url);
  http.addHeader("User-Agent", "ESP32-NewsDisplay/1.0");
  http.setTimeout(15000);
  
  displayMessage("NewsAPI", "Descargando...", true, 50);
  
  int httpCode = http.GET();
  
  Serial.print("HTTP Code: ");
  Serial.println(httpCode);
  
  if (httpCode == 200) {
    displayMessage("NewsAPI", "Procesando...", true, 70);
    
    String payload = http.getString();
    Serial.println("✓ Respuesta recibida");
    Serial.print("  Tamaño: ");
    Serial.print(payload.length());
    Serial.println(" bytes");
    
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, payload);
    
    if (error) {
      Serial.print("✗ Error JSON: ");
      Serial.println(error.c_str());
      displayError("Error JSON", true);
      http.end();
      failedRequests++;
      return false;
    }
    
    displayMessage("NewsAPI", "Extrayendo...", true, 90);
    
    JsonArray articles = doc["articles"];
    totalNews = 0;
    
    Serial.println("\nNoticias:");
    Serial.println("---------------------------------");
    
    for (JsonObject article : articles) {
      if (totalNews >= 5) break;
      
      const char* title = article["title"];
      const char* sourceName = article["source"]["name"];
      const char* publishedAt = article["publishedAt"];
      
      if (title != nullptr) {
        newsArticles[totalNews].title = String(title);
        newsArticles[totalNews].source = sourceName != nullptr ? String(sourceName) : "Unknown";
        newsArticles[totalNews].publishedAt = publishedAt != nullptr ? String(publishedAt) : "";
        
        Serial.print(totalNews + 1);
        Serial.print(". [");
        Serial.print(newsArticles[totalNews].source);
        Serial.print("] ");
        Serial.println(newsArticles[totalNews].title);
        
        totalNews++;
      }
    }
    
    Serial.println("---------------------------------");
    Serial.print("Total: ");
    Serial.print(totalNews);
    Serial.println(" noticias\n");
    
    http.end();
    
    if (totalNews > 0) {
      successfulRequests++;
      currentNewsIndex = 0;
      lastUpdateTime = getTimeString();
      failedAttempts = 0;
      
      displayMessage("NewsAPI", "Completo!", true, 100);
      delay(1000);
      
      displayNewsImproved(newsArticles[0], 0, totalNews);
      
      Serial.println("✓ Noticias actualizadas\n");
      return true;
    } else {
      displayError("Sin noticias", true);
      failedRequests++;
      return false;
    }
    
  } else {
    Serial.print("✗ Error HTTP: ");
    Serial.println(httpCode);
    
    String errorMsg = "HTTP " + String(httpCode);
    if (httpCode == 401) errorMsg = "API Key\ninvalida";
    else if (httpCode == 429) errorMsg = "Limite\nexcedido";
    else if (httpCode == -1) errorMsg = "Timeout";
    
    displayError(errorMsg, true);
    http.end();
    failedRequests++;
    return false;
  }
}

// ===================================
// FUNCIONES DE THINGSPEAK
// ===================================

bool sendToCloud() {
  cloudUpdateCount++;
  
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("✗ Cloud: WiFi desconectado");
    cloudFailCount++;
    return false;
  }
  
  Serial.println("\n=================================");
  Serial.println("Enviando datos a ThingSpeak");
  Serial.print("Update #");
  Serial.println(cloudUpdateCount);
  Serial.println("=================================");
  
  uptimeSeconds = millis() / 1000;
  int rssi = WiFi.RSSI();
  int freeMemory = ESP.getFreeHeap();
  int categoryNum = categoryToNumber(currentCategory);
  int deviceStatus = (totalNews > 0) ? 1 : 0;
  
  Serial.println("Datos:");
  Serial.print("  Uptime: ");
  Serial.print(uptimeSeconds);
  Serial.println("s");
  Serial.print("  RSSI: ");
  Serial.print(rssi);
  Serial.println(" dBm");
  Serial.print("  Memoria libre: ");
  Serial.print(freeMemory);
  Serial.println(" bytes");
  Serial.print("  Total requests: ");
  Serial.println(totalNewsRequests);
  Serial.print("  Exitosos: ");
  Serial.println(successfulRequests);
  Serial.print("  Fallidos: ");
  Serial.println(failedRequests);
  Serial.print("  Categoría: ");
  Serial.print(currentCategory);
  Serial.print(" (");
  Serial.print(categoryNum);
  Serial.println(")");
  Serial.print("  Status: ");
  Serial.println(deviceStatus);
  Serial.print("  Comandos recibidos: ");
  Serial.println(commandsReceived);
  Serial.print("  Comandos ejecutados: ");
  Serial.println(commandsExecuted);
  Serial.println();
  
  HTTPClient http;
  
  String url = "http://";
  url += THINGSPEAK_SERVER;
  url += "/update?api_key=";
  url += THINGSPEAK_WRITE_API_KEY;
  url += "&field1=" + String(uptimeSeconds);
  url += "&field2=" + String(rssi);
  url += "&field3=" + String(freeMemory);
  url += "&field4=" + String(totalNewsRequests);
  url += "&field5=" + String(successfulRequests);
  url += "&field6=" + String(failedRequests);
  url += "&field7=" + String(categoryNum);
  url += "&field8=" + String(deviceStatus);
  
  http.begin(url);
  http.addHeader("User-Agent", "ESP32-IoT");
  
  int httpCode = http.GET();
  
  Serial.print("HTTP Code: ");
  Serial.println(httpCode);
  
  if (httpCode == 200) {
    String response = http.getString();
    int entryNumber = response.toInt();
    
    Serial.print("Respuesta: ");
    Serial.println(response);
    
    if (entryNumber > 0) {
      cloudSuccessCount++;
      Serial.println("✓ Datos enviados a la nube!");
      Serial.print("  Entry #");
      Serial.println(entryNumber);
      Serial.println();
      http.end();
      return true;
    } else {
      cloudFailCount++;
      Serial.println("✗ ThingSpeak rechazó datos (rate limit?)");
      http.end();
      return false;
    }
  } else {
    cloudFailCount++;
    Serial.print("✗ Error HTTP: ");
    Serial.println(httpCode);
    http.end();
    return false;
  }
}

// ===================================
// FUNCIONES DE CONTROL REMOTO
// ===================================

void executeCommand(String command) {
  Serial.println("\n╔════════════════════════════════════╗");
  Serial.println("║       COMANDO RECIBIDO             ║");
  Serial.println("╚════════════════════════════════════╝");
  Serial.print("Comando: ");
  Serial.println(command);
  Serial.println();
  
  commandsExecuted++;
  displayCommandReceived(command);
  
  // Cambiar categoría
  if (command.startsWith("CATEGORY_")) {
    String newCategory = command.substring(9);
    newCategory.toLowerCase();
    
    if (newCategory == "technology" || newCategory == "business" || 
        newCategory == "sports" || newCategory == "entertainment" || 
        newCategory == "health" || newCategory == "science" || 
        newCategory == "general") {
      
      currentCategory = newCategory;
      Serial.print("✓ Categoría cambiada a: ");
      Serial.println(currentCategory);
      
      displayMessage("Comando", "Categoria:\n\n" + currentCategory, false);
      delay(2000);
      
      // Actualizar noticias inmediatamente
      fetchNews();
    } else {
      Serial.println("✗ Categoría inválida");
    }
  }
  
  // Cambiar país
  else if (command.startsWith("COUNTRY_")) {
    String newCountry = command.substring(8);
    newCountry.toLowerCase();
    
    currentCountry = newCountry;
    Serial.print("✓ País cambiado a: ");
    Serial.println(currentCountry);
    
    displayMessage("Comando", "Pais:\n\n" + currentCountry, false);
    delay(2000);
    
    // Actualizar noticias inmediatamente
    fetchNews();
  }
  
  // Cambiar intervalo de actualización
  else if (command.startsWith("INTERVAL_")) {
    int newInterval = command.substring(9).toInt();
    
    if (newInterval >= 30 && newInterval <= 300) {
      currentUpdateInterval = newInterval * 1000; // Convertir a milisegundos
      Serial.print("✓ Intervalo cambiado a: ");
      Serial.print(newInterval);
      Serial.println(" segundos");
      
      displayMessage("Comando", "Intervalo:\n\n" + String(newInterval) + "s", false);
      delay(2000);
    } else {
      Serial.println("✗ Intervalo inválido (30-300s)");
    }
  }
  
  // Actualizar noticias ahora
  else if (command == "UPDATE_NOW") {
    Serial.println("✓ Actualizando noticias...");
    displayMessage("Comando", "Actualizando\nnoticias...", false);
    delay(1500);
    fetchNews();
  }
  
  // Reiniciar dispositivo
  else if (command == "RESTART") {
    Serial.println("✓ Reiniciando dispositivo...");
    displayMessage("Comando", "Reiniciando\nESP32...", false);
    delay(2000);
    ESP.restart();
  }
  
  // Enviar reporte de estado
  else if (command == "STATUS") {
    Serial.println("✓ Enviando reporte de estado...");
    displayMessage("Comando", "Enviando\nestado...", false);
    delay(1500);
    sendToCloud();
  }
  
  // Comando desconocido
  else {
    Serial.print("⚠ Comando no reconocido: ");
    Serial.println(command);
  }
  
  Serial.println();
}

bool checkCommands() {
  if (WiFi.status() != WL_CONNECTED) {
    return false;
  }
  
  Serial.println("\n⟳ Verificando comandos remotos...");
  
  HTTPClient http;
  
  // Construir URL para obtener el último comando
  String url = "http://";
  url += THINGSPEAK_SERVER;
  url += "/talkbacks/";
  url += String(TALKBACK_ID);
  url += "/commands/execute";
  url += "?api_key=";
  url += TALKBACK_API_KEY;
  
  http.begin(url);
  http.addHeader("User-Agent", "ESP32-IoT");
  
  int httpCode = http.GET();
  
  Serial.print("  HTTP Code: ");
  Serial.println(httpCode);
  
  if (httpCode == 200) {
    String command = http.getString();
    command.trim();
    
    if (command.length() > 0) {
      commandsReceived++;
      Serial.print("  ✓ Comando recibido: ");
      Serial.println(command);
      http.end();
      
      // Ejecutar comando
      executeCommand(command);
      return true;
    } else {
      Serial.println("  (Sin comandos pendientes)");
      http.end();
      return true;
    }
  } else if (httpCode == 404) {
    Serial.println("  (Cola de comandos vacía)");
    http.end();
    return true;
  } else {
    Serial.print("  ✗ Error HTTP: ");
    Serial.println(httpCode);
    http.end();
    return false;
  }
}

// ===================================
// SETUP
// ===================================

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("\n\n");
  Serial.println("╔═══════════════════════════════════════╗");
  Serial.println("║   Dashboard de Noticias IoT - FINAL  ║");
  Serial.println("║      Con Control Remoto Completo     ║");
  Serial.println("╚═══════════════════════════════════════╝");
  Serial.println();
  Serial.print("Chip: ");
  Serial.println(ESP.getChipModel());
  Serial.print("Frecuencia: ");
  Serial.print(ESP.getCpuFreqMHz());
  Serial.println(" MHz");
  Serial.print("Memoria libre: ");
  Serial.print(ESP.getFreeHeap());
  Serial.println(" bytes");
  Serial.print("ThingSpeak Channel: ");
  Serial.println(THINGSPEAK_CHANNEL_ID);
  Serial.print("TalkBack ID: ");
  Serial.println(TALKBACK_ID);
  Serial.println();
  
  // Inicializar display
  Serial.print("Inicializando display... ");
  if(!display.begin(SSD1306_SWITCHCAPVCC)) {
    Serial.println("FALLO");
    while(1);
  }
  Serial.println("OK");
  
  display.clearDisplay();
  displayMessage("IoT Dashboard", "Iniciando...\n\nControl Remoto", true, 50);
  delay(2000);
  
  // Conectar WiFi
  int wifiAttempts = 0;
  while (!connectWiFi() && wifiAttempts < 3) {
    wifiAttempts++;
    Serial.print("Reintento WiFi ");
    Serial.print(wifiAttempts);
    Serial.println("/3");
    delay(5000);
  }
  
  if (!wifiConnected) {
    Serial.println("✗ Sin WiFi");
    displayError("Sin WiFi");
    while(1) delay(1000);
  }
  
  // Obtener noticias iniciales
  if (fetchNews()) {
    Serial.println("✓ Sistema inicializado\n");
  } else {
    Serial.println("⚠ No se obtuvieron noticias iniciales\n");
  }
  
  // Enviar datos iniciales a la nube
  Serial.println("Enviando datos iniciales a ThingSpeak...");
  delay(2000);
  sendToCloud();
  
  // Verificar comandos iniciales
  Serial.println("Verificando comandos iniciales...");
  checkCommands();
  
  lastNewsUpdate = millis();
  lastDisplayUpdate = millis();
  lastCloudUpdate = millis();
  lastCommandCheck = millis();
  
  Serial.println("\n✓ Sistema operativo completamente\n");
  Serial.println("╔════════════════════════════════════════╗");
  Serial.println("║        COMANDOS DISPONIBLES:           ║");
  Serial.println("╠════════════════════════════════════════╣");
  Serial.println("║  CATEGORY_TECHNOLOGY                   ║");
  Serial.println("║  CATEGORY_BUSINESS                     ║");
  Serial.println("║  CATEGORY_SPORTS                       ║");
  Serial.println("║  COUNTRY_US                            ║");
  Serial.println("║  COUNTRY_MX                            ║");
  Serial.println("║  COUNTRY_CO                            ║");
  Serial.println("║  INTERVAL_30  (30-300 segundos)        ║");
  Serial.println("║  UPDATE_NOW                            ║");
  Serial.println("║  STATUS                                ║");
  Serial.println("║  RESTART                               ║");
  Serial.println("╚════════════════════════════════════════╝");
  Serial.println();
}

// ===================================
// LOOP
// ===================================

void loop() {
  unsigned long currentMillis = millis();
  uptimeSeconds = currentMillis / 1000;
  
  // Actualizar noticias (intervalo configurable)
  if (currentMillis - lastNewsUpdate >= currentUpdateInterval) {
    lastNewsUpdate = currentMillis;
    Serial.println("\n⟳ Actualización automática de noticias");
    fetchNews();
  }
  
  // Rotar titulares cada 3 segundos
  if (totalNews > 1 && currentMillis - lastDisplayUpdate >= DISPLAY_SCROLL_DELAY) {
    lastDisplayUpdate = currentMillis;
    currentNewsIndex = (currentNewsIndex + 1) % totalNews;
    displayNewsImproved(newsArticles[currentNewsIndex], currentNewsIndex, totalNews);
  }
  
  // Enviar datos a la nube cada 60 segundos
  if (currentMillis - lastCloudUpdate >= CLOUD_UPDATE_INTERVAL) {
    lastCloudUpdate = currentMillis;
    Serial.println("\n⟳ Actualización automática de cloud");
    sendToCloud();
  }
  
  // Verificar comandos remotos cada 30 segundos
  if (currentMillis - lastCommandCheck >= COMMAND_CHECK_INTERVAL) {
    lastCommandCheck = currentMillis;
    checkCommands();
  }
  
  // Verificar WiFi
  if (WiFi.status() != WL_CONNECTED) {
    if (wifiConnected) {
      Serial.println("⚠ WiFi perdido - Reconectando...");
      wifiConnected = false;
      displayError("WiFi perdido", false);
      delay(2000);
      connectWiFi();
    }
  } else {
    wifiConnected = true;
  }
  
  // Reporte de estadísticas cada 5 minutos
  static unsigned long lastStatsReport = 0;
  if (currentMillis - lastStatsReport >= 300000) {
    lastStatsReport = currentMillis;
    
    Serial.println("\n========== ESTADÍSTICAS ==========");
    Serial.print("Uptime: ");
    Serial.print(uptimeSeconds / 3600);
    Serial.print("h ");
    Serial.print((uptimeSeconds % 3600) / 60);
    Serial.print("m ");
    Serial.print(uptimeSeconds % 60);
    Serial.println("s");
    Serial.print("Config: ");
    Serial.print(currentCountry);
    Serial.print(" / ");
    Serial.println(currentCategory);
    Serial.print("Intervalo: ");
    Serial.print(currentUpdateInterval / 1000);
    Serial.println("s");
    Serial.print("Noticias - Total: ");
    Serial.print(totalNewsRequests);
    Serial.print(" | OK: ");
    Serial.print(successfulRequests);
    Serial.print(" | Fail: ");
    Serial.println(failedRequests);
    Serial.print("Cloud - Total: ");
    Serial.print(cloudUpdateCount);
    Serial.print(" | OK: ");
    Serial.print(cloudSuccessCount);
    Serial.print(" | Fail: ");
    Serial.println(cloudFailCount);
    Serial.print("Comandos - Recibidos: ");
    Serial.print(commandsReceived);
    Serial.print(" | Ejecutados: ");
    Serial.println(commandsExecuted);
    Serial.print("Memoria: ");
    Serial.print(ESP.getFreeHeap());
    Serial.println(" bytes");
    Serial.print("RSSI: ");
    Serial.print(WiFi.RSSI());
    Serial.println(" dBm");
    Serial.println("==================================\n");
  }
  
  delay(100);
}