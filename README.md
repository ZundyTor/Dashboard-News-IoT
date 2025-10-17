# 📰 Dashboard de Noticias IoT con ESP32

Sistema IoT que integra un ESP32 con display OLED SSD1306 (SPI), consumo de APIs públicas (NewsAPI) y servicios en la nube (ThingSpeak + TalkBack) para visualizar titulares y controlar parámetros de forma remota. Proyecto desarrollado paso a paso, con pruebas en cada fase y manejo robusto de errores.

---

## 🟦 Tabla de Contenidos

- [📖 Descripción del proyecto y objetivos](#-descripción-del-proyecto-y-objetivos)
- [🔩 Lista de componentes y conexiones](#-lista-de-componentes-y-conexiones)
- [☁️ Servicios en la nube utilizados](#-servicios-en-la-nube-utilizados)
- [🌐 APIs externas integradas](#-apis-externas-integradas)
- [🚦 Instrucciones de instalación paso a paso](#-instrucciones-de-instalación-paso-a-paso)
- [🔐 Configuración de credenciales](#-configuración-de-credenciales)
- [🕹️ Guía de uso del sistema completo](#-guía-de-uso-del-sistema-completo)
- [🧩 Diagrama de arquitectura del sistema](#-diagrama-de-arquitectura-del-sistema)
- [🐞 Problemas encontrados y soluciones](#-problemas-encontrados-y-soluciones)
- [🚀 Mejoras futuras identificadas](#-mejoras-futuras-identificadas)

---

# 📖 Descripción del proyecto y objetivos

**Dashboard de Noticias IoT**: Visualiza titulares en tiempo real en un ESP32 + OLED, consume APIs públicas (NewsAPI), envía telemetría a la nube (ThingSpeak), y permite control remoto (TalkBack).

✅ **Objetivo:** Demostrar integración IoT + Cloud + APIs + control remoto, con código robusto y documentado.

**Metas alcanzadas:**
- Hardware confiable y display interactivo
- Consumo seguro de APIs externas
- Sincronización de datos y control con la nube
- Documentación y visuales para onboarding y troubleshooting

---

# 🔩 Lista de componentes y conexiones

## 🛠️ Hardware

| Componente       | Descripción                      | Cantidad |
|------------------|----------------------------------|----------|
| ESP32 DevKit     | ESP32-D0WD-V3, WiFi, 240 MHz     | 1        |
| OLED SSD1306 SPI | 128x64, 7 pines, monocromático   | 1        |
| Cable USB        | Micro-B                          | 1        |

Librerías (PlatformIO):
- Adafruit SSD1306
- Adafruit GFX Library
- Adafruit BusIO
- ArduinoJson
- WiFi / WiFiClientSecure / HTTPClient (framework Arduino para ESP32)

## 🔗 Conexiones

| OLED Pin | ESP32 GPIO | Descripción         |
|----------|------------|--------------------|
| GND      | GND        | Tierra             |
| VCC      | 3.3V       | Alimentación       |
| SCL      | 18         | Clock SPI (SCLK)   |
| SDA      | 23         | Data SPI (MOSI)    |
| RES      | 4          | Reset              |
| DC       | 2          | Data/Command       |
| CS       | 5          | Chip Select        |

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

# ☁️ Servicios en la nube utilizados

- **ThingSpeak:** (canal con 8 campos):
  - Field1: uptime_seconds
  - Field2: wifi_rssi (dBm)
  - Field3: free_memory (bytes)
  - Field4: total_requests (NewsAPI)
  - Field5: successful_requests
  - Field6: failed_requests
  - Field7: current_category (0=technology, 1=business, 2=sports, 3=entertainment, 4=health, 5=science, 6=general)
  - Field8: device_status (1=OK, 0=Error)
- **ThingSpeak TalkBack:**
  - Cola de comandos remotos (FIFO). Comandos soportados: CATEGORY_*, COUNTRY_*, INTERVAL_*, UPDATE_NOW, STATUS, RESTART.

---

# 🌐 APIs externas integradas

- **NewsAPI**: [https://newsapi.org/](https://newsapi.org/)
  - Endpoint: /v2/top-headlines
  - Parámetros usados: country, category, pageSize=5, apiKey
  - Ejemplo de request: https://newsapi.org/v2/top-headlines?country=us&category=technology&pageSize=5&apiKey=YOUR_API_KEY
  - Límite plan free: 100 requests/día.

- **ThingSpeak Write API (telemetría)**:
  - Endpoint: http://api.thingspeak.com/update
  - Parámetros: api_key, field1..field8
  - Rate limit free: mínimo 15s entre updates.

- **ThingSpeak TalkBack (comandos)**:
  - Endpoint: http://api.thingspeak.com/talkbacks/{TALKBACK_ID}/commands/execute?api_key={TALKBACK_API_KEY}
  - Respuesta HTTP 200 con comando en texto; 404 si la cola está vacía.

---

# 🚦 Instrucciones de instalación paso a paso

**Prerrequisitos:**
- VS Code + PlatformIO
- Drivers USB ESP32 (CP2102/CH340)
- Cuentas en NewsAPI, ThingSpeak

**Pasos:**

1. **Clona el repositorio**  
   ```bash
   git clone https://github.com/ZundyTor/Dashboard-News-IoT.git
   cd Dashboard-News-IoT
   ```

2. **Instala dependencias con PlatformIO**  
   (Se instalan automáticamente al abrir el proyecto)

3. **Copia y configura credenciales**  
   ```bash
   cp include/config.h.example include/config.h
   ```
   Edita `include/config.h` con tus datos (¡no lo subas!).

4. **Conecta el hardware**  
   - Cableado según la tabla de conexiones.

5. **Compila y sube el código**  
   - Project Tasks > Upload o `Ctrl+Alt+U`
   - Mantén presionado BOOT en “Connecting…”

6. **Abre Monitor Serial**  
   - Velocidad: 115200  
   - Verifica logs, display, telemetría y comandos.

---

# 🔐 Configuración de credenciales

**Archivo:** `include/config.h` *(no se sube, plantilla en `config.h.example`)*

```cpp
const char* WIFI_SSID = "TU_WIFI";
const char* WIFI_PASSWORD = "TU_PASSWORD";
const char* NEWS_API_KEY = "TU_API_KEY_NEWSAPI";
const char* THINGSPEAK_WRITE_API_KEY = "TU_KEY";
const unsigned long THINGSPEAK_CHANNEL_ID = 123456;
const unsigned long TALKBACK_ID = 654321;
const char* TALKBACK_API_KEY = "TU_TALKBACK_KEY";
```

**Intervalos recomendados (ms):**
- `NEWS_UPDATE_INTERVAL = 60000`
- `CLOUD_UPDATE_INTERVAL = 60000`
- `COMMAND_CHECK_INTERVAL = 30000`
- `DISPLAY_SCROLL_DELAY = 3000`

⚠️ **Nunca subas tus claves reales.**

---

# 🕹️ Guía de uso del sistema completo

**Flujo:**
1. Conexión WiFi (display: progreso)
2. Descarga de noticias (NewsAPI)
3. Muestra titulares (rotación cada 3s)
4. Envío de telemetría a ThingSpeak cada 60s
5. Verificación de comandos remotos (TalkBack cada 30s)
6. Ejecución automática de comandos (cambio de categoría, país, intervalo, update, restart)

**Display:**
```
┌────────────┬─────────┬─────────────┐
│ CATEGORY   │ [n/N]   │ WiFi Status │
├────────────┴─────────┴─────────────┤
│ Fuente de noticia                  │
│ Titular (4 líneas, word wrap)      │
├────────────────────────────────────┤
│ Up:hh:mm:ss   ● ○ ○ ○              │
└────────────────────────────────────┘
```

**Comandos remotos soportados:**
- `CATEGORY_TECHNOLOGY`, `CATEGORY_BUSINESS`, ...
- `COUNTRY_US`, `COUNTRY_MX`, ...
- `INTERVAL_30`...`INTERVAL_300`
- `UPDATE_NOW`
- `STATUS`
- `RESTART`

**Uso TalkBack:**
- Agrega el comando en ThingSpeak → Apps → TalkBack
- El ESP32 lo ejecuta en ≤30s

---

## 📊 Explicación de los Fields en ThingSpeak

Cada uno de los siguientes campos ("fields") representa una variable clave para el monitoreo y control del sistema IoT. Así puedes interpretar las gráficas o valores en el dashboard de ThingSpeak:

| **Field**              | **Descripción**                                                                                                                                                 | **Ejemplo**  |
|------------------------|-----------------------------------------------------------------------------------------------------------------------------------------------------------------|-------------|
| **uptime_seconds**     | 🕒 **Tiempo total encendido** del ESP32 desde el último reinicio, en segundos. Útil para saber la estabilidad y detectar reinicios no deseados.                  | 3600        |
| **wifi_rssi**          | 📶 **Intensidad de señal WiFi** en dBm. Entre -30 (excelente) y -90 (muy mala). Si baja de -70, es probable que haya desconexiones.                             | -45         |
| **free_memory**        | 💾 **Memoria RAM libre** en bytes. Permite identificar fugas de memoria o cuellos de botella en el software.                                                    | 245000      |
| **total_requests**     | 🔄 **Total de solicitudes** hechas a NewsAPI desde el último reinicio. Permite monitorear frecuencia de uso y posibles excesos respecto al límite diario.        | 50          |
| **successful_requests**| ✅ **Solicitudes exitosas** a NewsAPI. Útil para calcular tasa de éxito y la calidad de la conexión/API.                                                        | 48          |
| **failed_requests**    | ❌ **Solicitudes fallidas** a NewsAPI (errores de red, límites, API key inválida, etc). Ayuda a detectar problemas antes de que afecten al usuario.              | 2           |
| **current_category**   | 🏷️ **Categoría de noticias actual** (valor numérico):<br>0=technology, 1=business, 2=sports, 3=entertainment, 4=health, 5=science, 6=general.                  | 0           |
| **device_status**      | ⚙️ **Estado general del dispositivo**:<br>1=OK (funcionando), 0=Error (sin conexión, sin noticias, etc).                                                        | 1           |

---

### 📈 ¿Cómo interpretar las gráficas del dashboard de ThingSpeak?

- **uptime_seconds**: Si sube continuamente, el sistema está estable. Si ves caídas a 0, hubo un reinicio.
- **wifi_rssi**: Si baja de -70 dBm, la señal es débil; si se mantiene estable, la conexión es confiable.
- **free_memory**: Debe ser relativamente constante; si baja mucho, puede haber fuga de memoria.
- **total_requests / successful_requests / failed_requests**: Te permiten calcular la tasa de éxito y anticipar problemas de red o API.
- **current_category**: Útil para ver cuándo y con qué comandos cambiaste el tipo de noticias.
- **device_status**: Si ves un 0, revisa el ESP32; puede estar sin WiFi o con error de API.

---

> **Tip**: ¡Estas métricas no solo ayudan a monitorear el sistema, sino también a anticipar y diagnosticar problemas antes de que el usuario los perciba!

# 🧩 Diagrama de arquitectura del sistema

```
      Internet
         │
 ┌───────┼───────┐
 │       │       │
 ▼       ▼       ▼
NewsAPI  ThingSpeak  ThingSpeak TalkBack
│        │         │
│        │         │
└───┬────┴─────┬───┘
    │          │
    ▼          ▼
   ESP32 ←→ OLED SSD1306
     │
     ▼
 Serial Monitor / USB
```

---

# 🐞 Problemas encontrados y soluciones

- **Display en blanco**: Inicialmente era por código I2C, se corrigió usando SPI y pines correctos.
- **NewsAPI sin noticias**: Algunas combinaciones país/categoría no devuelven titulares, usar combinaciones populares (ej: us + technology).
- **ThingSpeak responde “0”**: Límite mínimo de 15s entre updates; esperar y verificar fields/API key.
- **Error de subida ESP32**: Usar botón BOOT durante “Connecting…”, cerrar Monitor Serial y revisar puerto.
- **Conflicto Git push**: Usar `git pull origin main --allow-unrelated-histories` antes de push.

---

# 🚀 Mejoras futuras identificadas

- Scroll horizontal suave en display para titulares largos.
- Cache local de noticias y deduplicación.
- OTA (actualización por WiFi).
- TLS robusto (verificación de certificados).
- Dashboard web propio con analítica avanzada.
- Selección de fuentes y filtros por palabra clave.
- Modo ahorro de energía/deep sleep.

---

## Notas finales

- Respeta los límites de NewsAPI (100 req/día) y ThingSpeak (≥15s entre updates).
- Ajusta intervalos en config.h para desarrollo (más rápidos) o producción (más conservadores).
- TalkBack es una cola: agrega comandos cuando desees ejecutar acciones y espera el ciclo de verificación.

---

**Desarrollado por Sebastián Blanco y Daniel Lerzundy**

---
