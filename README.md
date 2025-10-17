# 📰 Dashboard de Noticias IoT con ESP32

Sistema IoT que integra un ESP32 con display OLED SSD1306 (SPI), consumo de APIs públicas (NewsAPI) y servicios en la nube (ThingSpeak + TalkBack) para visualizar titulares y controlar parámetros de forma remota. Proyecto desarrollado paso a paso, con pruebas en cada fase y manejo robusto de errores.

Estado: Operativo
Plataforma: ESP32 (ESP32-D0WD-V3) + PlatformIO (VS Code)
Lenguaje: C++

---

## Descripción del proyecto y objetivos

El proyecto implementa un Dashboard de Noticias en un ESP32 con pantalla OLED. Obtiene titulares desde NewsAPI y los muestra en el display rotando periódicamente. Envía telemetría a ThingSpeak (uptime, RSSI, memoria, estadísticas de requests) y recibe comandos desde TalkBack para cambiar categoría/país, intervalo de actualización, forzar actualización o reiniciar el dispositivo.

Objetivo general:
- Desarrollar un sistema IoT completo que integre ESP32 + OLED y servicios en la nube, demostrando habilidades en IoT, APIs REST y computación en la nube.

Objetivos específicos:
- Implementar dispositivo IoT con ESP32 y display SSD1306 (SPI).
- Integrar servicios web externos (NewsAPI).
- Desplegar servicios en la nube (ThingSpeak) para recepción/envío de datos.
- Configurar comunicación bidireccional: telemetría y control remoto (TalkBack).
- Documentar el proceso completo con diagramas, guías y troubleshooting.

---

## Lista de componentes y conexiones

Hardware:
- ESP32 DevKit (ESP32-D0WD-V3).
- Display OLED SSD1306 128x64 (versión SPI de 7 pines).
- Cable USB (programación/alimentación).

Librerías (PlatformIO):
- Adafruit SSD1306
- Adafruit GFX Library
- Adafruit BusIO
- ArduinoJson
- WiFi / WiFiClientSecure / HTTPClient (framework Arduino para ESP32)

Conexiones (SPI, comprobadas en pruebas):
- GND → GND
- VCC → 3.3V
- SCL (SCLK) → GPIO 18
- SDA (MOSI) → GPIO 23
- RES (Reset) → GPIO 4
- DC (Data/Command) → GPIO 2
- CS (Chip Select) → GPIO 5

Diagrama rápido de conexiones (ASCII):
```
ESP32 DevKit                           Display OLED SSD1306 (SPI 7 pines)
┌─────────────────┐                    ┌──────────────────┐
│             3.3V├────────────────────┤VCC               │
│              GND├────────────────────┤GND               │
│      GPIO 18    ├────────────────────┤SCL (SCLK)        │
│      GPIO 23    ├────────────────────┤SDA (MOSI)        │
│      GPIO 4     ├────────────────────┤RES (Reset)       │
│      GPIO 2     ├────────────────────┤DC (D/C)          │
│      GPIO 5     ├────────────────────┤CS (Chip Select)  │
└─────────────────┘                    └──────────────────┘
```

---

## Servicios en la nube utilizados

- ThingSpeak (canal con 8 campos):
  - Field1: uptime_seconds
  - Field2: wifi_rssi (dBm)
  - Field3: free_memory (bytes)
  - Field4: total_requests (NewsAPI)
  - Field5: successful_requests
  - Field6: failed_requests
  - Field7: current_category (0=technology, 1=business, 2=sports, 3=entertainment, 4=health, 5=science, 6=general)
  - Field8: device_status (1=OK, 0=Error)
- ThingSpeak TalkBack:
  - Cola de comandos remotos (FIFO). Comandos soportados: CATEGORY_*, COUNTRY_*, INTERVAL_*, UPDATE_NOW, STATUS, RESTART.

---

## APIs externas integradas

- NewsAPI: [https://newsapi.org/](https://newsapi.org/)
  - Endpoint: /v2/top-headlines
  - Parámetros usados: country, category, pageSize=5, apiKey
  - Ejemplo de request: https://newsapi.org/v2/top-headlines?country=us&category=technology&pageSize=5&apiKey=YOUR_API_KEY
  - Límite plan free: 100 requests/día.

- ThingSpeak Write API (telemetría):
  - Endpoint: http://api.thingspeak.com/update
  - Parámetros: api_key, field1..field8
  - Rate limit free: mínimo 15s entre updates.

- ThingSpeak TalkBack (comandos):
  - Endpoint: http://api.thingspeak.com/talkbacks/{TALKBACK_ID}/commands/execute?api_key={TALKBACK_API_KEY}
  - Respuesta HTTP 200 con comando en texto; 404 si la cola está vacía.

---

## Instrucciones de instalación paso a paso

Prerrequisitos:
- Visual Studio Code + extensión PlatformIO IDE.
- Drivers USB del chip de tu ESP32 (CP2102 o CH340).
- Cuentas en NewsAPI y ThingSpeak (crear canal y TalkBack).

1) Clonar y abrir el proyecto
- Clona el repo en tu equipo y ábrelo con VS Code → PlatformIO carga dependencias la primera vez.

2) Configurar PlatformIO
- Revisa platformio.ini. Debe incluir lib_deps de Adafruit y ArduinoJson y monitor_speed=115200.

3) Crear config.h a partir de la plantilla
- Copia include/config.h.example a include/config.h.
- Completa tus credenciales (WiFi, NewsAPI, ThingSpeak, TalkBack). No subas config.h a GitHub.

4) Cableado y verificación de hardware
- Conecta el OLED SSD1306 SPI con los pines indicados.
- Conecta el ESP32 por USB.

5) Compilar
- PlatformIO: Project Tasks → Build. La primera compilación puede tardar.

6) Subir al ESP32
- Cierra el Monitor Serial si está abierto.
- Click Upload. Cuando veas “Connecting…”, mantén presionado BOOT hasta que empiece “Writing…”, luego suelta.

7) Abrir Monitor Serial
- Velocidad 115200. Verifica conexión WiFi, descarga de noticias y envíos a ThingSpeak.

---

## Configuración de credenciales (sin exponer keys)

Archivo: include/config.h (externalizado, ignorado por Git).

Parámetros:
- WiFi: WIFI_SSID, WIFI_PASSWORD.
- NewsAPI: NEWS_API_KEY, NEWS_COUNTRY (ej. us), NEWS_CATEGORY (ej. technology).
- ThingSpeak:
  - THINGSPEAK_WRITE_API_KEY, THINGSPEAK_READ_API_KEY (opcional para lecturas),
  - THINGSPEAK_CHANNEL_ID (numérico),
  - THINGSPEAK_SERVER="api.thingspeak.com".
- TalkBack:
  - TALKBACK_ID (numérico), TALKBACK_API_KEY.

Intervalos (ms) recomendados:
- NEWS_UPDATE_INTERVAL = 60000
- CLOUD_UPDATE_INTERVAL = 60000
- COMMAND_CHECK_INTERVAL = 30000
- DISPLAY_SCROLL_DELAY = 3000

Seguridad:
- No subas config.h al repo (listado en .gitignore).
- No compartas API keys en issues/commits/screenshots.

---

## Guía de uso del sistema completo

Al iniciar:
- El ESP32 conecta a WiFi (reintento y barra de progreso en display).
- Descarga 5 titulares y los muestra rotando cada 3s.
- Envía telemetría a ThingSpeak cada 60s.
- Verifica TalkBack cada 30s y ejecuta comandos en cola (uno por ciclo).

Indicadores en OLED:
- Header: CATEGORÍA [índice/total] y estado WiFi.
- Fuente de la noticia.
- Titular con word wrap (4 líneas).
- Footer: “Up:hh:mm:ss” + paginación con puntos.

Comandos TalkBack soportados:
- CATEGORY_TECHNOLOGY, CATEGORY_BUSINESS, CATEGORY_SPORTS, CATEGORY_ENTERTAINMENT, CATEGORY_HEALTH, CATEGORY_SCIENCE, CATEGORY_GENERAL
- COUNTRY_US, COUNTRY_MX, COUNTRY_CO, COUNTRY_AR, COUNTRY_ES, …
- INTERVAL_30 … INTERVAL_300 (en segundos)
- UPDATE_NOW (descargar noticias de inmediato)
- STATUS (enviar telemetría de inmediato)
- RESTART (reiniciar ESP32)

Notas:
- TalkBack usa una cola FIFO: los comandos se consumen y eliminan al ejecutarse.
- Latencia típica de ejecución: hasta 30s (según COMMAND_CHECK_INTERVAL).
- Respeta el rate limit de ThingSpeak (≥15s entre updates).

---

## Diagrama de arquitectura del sistema

```
                 Internet
                    │
  ┌─────────────────┼───────────────────┐
  │                 │                   │
  ▼                 ▼                   ▼
NewsAPI        ThingSpeak Channel   ThingSpeak TalkBack
(top-headlines   (Telemetría)         (Comandos remotos)
  HTTPS GET)        HTTP GET             HTTP GET
      │                 │                   │
      └──────────┬──────┴───────┬──────────┘
                 │              │
                 ▼              ▼
            ESP32 (WiFi)  ←→  OLED SSD1306 (SPI)
            main.cpp
            - WiFi manager
            - NewsAPI client (HTTPS)
            - ThingSpeak client (HTTP)
            - TalkBack handler
            - Display controller
            - JSON parser
            - Error handling
            - Stats tracking
```

---

## Problemas encontrados y soluciones

1) Display no mostraba nada pese a “OK” en serial
- Causa: código inicial era para I2C, pero el display es SPI (7 pines).
- Solución: usar constructor SPI de Adafruit_SSD1306 y mapear pines: MOSI=23, CLK=18, DC=2, CS=5, RESET=4.

2) NewsAPI devolvía 200 con payload muy pequeño y 0 artículos (co + technology)
- Causa: combinación país/categoría sin resultados.
- Solución: usar us + technology (o mx + general, u otras combinaciones más pobladas).

3) ThingSpeak respondía “0”
- Causa: rate limit (<15s entre updates) o canal mal configurado.
- Solución: esperar ≥20s antes de nuevo update, confirmar Fields habilitados y Write API Key correcta.

4) Error al subir “Failed to connect to ESP32”
- Causa: modo bootloader no activado automáticamente.
- Solución: presionar BOOT durante “Connecting…”, soltar al iniciar “Writing…”. Cerrar Monitor Serial y verificar puerto.

5) Conflicto Git al subir al repo (“fetch first”)
- Causa: el remoto ya tenía commits (README/License inicial).
- Solución: git pull origin main --allow-unrelated-histories y luego git push.

---

## Mejoras futuras identificadas

- UI del display: scroll horizontal suave para titulares largos y soporte multilenguaje.
- Cache local y deduplicación de noticias para ahorrar Requests.
- OTA (Over-The-Air) para actualizar firmware sin cable.
- TLS robusto con verificación de certificados (evitar setInsecure).
- Backend propio opcional (AWS/Firebase) para analítica avanzada y control granular.
- Modo ahorro de energía y deep sleep (si se alimenta por batería).
- Web dashboard estático (Vercel/Netlify) con visualización y emisión de comandos.
- Selección de fuentes/sources y filtros por palabra clave.

---

## Notas finales

- Respeta los límites de NewsAPI (100 req/día) y ThingSpeak (≥15s entre updates).
- Ajusta intervalos en config.h para desarrollo (más rápidos) o producción (más conservadores).
- TalkBack es una cola: agrega comandos cuando desees ejecutar acciones y espera el ciclo de verificación.
