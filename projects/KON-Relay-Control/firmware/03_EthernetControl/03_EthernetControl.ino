/**
 * APP-01 - W5100 4-Relay Controller
 *
 * Repository status: FROZEN / maintenance-only auxiliary profile.
 * Arduino UNO + W5100 Ethernet Shield, Ethernet only.
 * Relay inputs: D4, D5, D6, D7. W5100 CS: D10.
 * microSD MUST be physically removed because D4 is reused as Relay 1.
 *
 * Control: Web UI, HTTP JSON API, Serial CLI at 115200.
 * Windows PowerShell CLI/GUI use the HTTP API.
 *
 * Bench result: relay module is ACTIVE HIGH. RELAY_ACTIVE_LOW=false.
 */

#include <SPI.h>
#include <Ethernet.h>
#include <string.h>
#include <ctype.h>

const uint8_t ETH_CS = 10;
const uint8_t RELAY_PINS[4] = {4, 5, 6, 7};
const bool RELAY_ACTIVE_LOW = false;

byte mac[] = {0x02, 0x4B, 0x4F, 0x4E, 0x60, 0x01};
EthernetServer server(80);

bool relayState[4] = {false, false, false, false};
bool networkReady = false;
unsigned long requestCount = 0;
unsigned long maintainTick = 0;

char serialLine[32];
uint8_t serialLen = 0;

uint8_t relayLevel(bool on) {
  return RELAY_ACTIVE_LOW ? (on ? LOW : HIGH) : (on ? HIGH : LOW);
}

void setRelay(uint8_t index, bool on) {
  if (index >= 4) return;
  relayState[index] = on;
  digitalWrite(RELAY_PINS[index], relayLevel(on));
}

void toggleRelay(uint8_t index) {
  if (index < 4) setRelay(index, !relayState[index]);
}

void setAll(bool on) {
  for (uint8_t i = 0; i < 4; ++i) setRelay(i, on);
}

void printRelayStatus(Stream &out) {
  out.print(F("RELAYS "));
  for (uint8_t i = 0; i < 4; ++i) {
    out.print(F("R"));
    out.print(i + 1);
    out.print('=');
    out.print(relayState[i] ? F("ON") : F("OFF"));
    if (i != 3) out.print(' ');
  }
  out.println();
}

void printHelp() {
  Serial.println(F("CLI: HELP | STATUS | R1 ON|OFF|TOGGLE ... R4 | ALL ON|OFF"));
}

void normalize(char *s) {
  while (*s) {
    *s = (char)toupper((unsigned char)*s);
    ++s;
  }
}

void handleSerialCommand(char *line) {
  normalize(line);

  if (!strcmp(line, "HELP") || !strcmp(line, "?")) {
    printHelp();
    return;
  }

  if (!strcmp(line, "STATUS")) {
    printRelayStatus(Serial);
    if (networkReady) {
      Serial.print(F("IP "));
      Serial.println(Ethernet.localIP());
    } else {
      Serial.println(F("NETWORK OFF"));
    }
    return;
  }

  if (!strcmp(line, "ALL ON")) {
    setAll(true);
    Serial.println(F("OK ALL ON"));
    printRelayStatus(Serial);
    return;
  }

  if (!strcmp(line, "ALL OFF")) {
    setAll(false);
    Serial.println(F("OK ALL OFF"));
    printRelayStatus(Serial);
    return;
  }

  if (line[0] == 'R' && line[1] >= '1' && line[1] <= '4' && line[2] == ' ') {
    uint8_t index = (uint8_t)(line[1] - '1');
    const char *action = line + 3;

    if (!strcmp(action, "ON")) setRelay(index, true);
    else if (!strcmp(action, "OFF")) setRelay(index, false);
    else if (!strcmp(action, "TOGGLE")) toggleRelay(index);
    else {
      Serial.println(F("ERR ACTION"));
      return;
    }

    Serial.print(F("OK R"));
    Serial.print(index + 1);
    Serial.print(' ');
    Serial.println(relayState[index] ? F("ON") : F("OFF"));
    return;
  }

  Serial.println(F("ERR COMMAND"));
  printHelp();
}

void pollSerial() {
  while (Serial.available()) {
    char c = (char)Serial.read();

    if (c == '\r' || c == '\n') {
      if (serialLen) {
        serialLine[serialLen] = 0;
        handleSerialCommand(serialLine);
        serialLen = 0;
      }
      continue;
    }

    if (c >= 32 && c <= 126 && serialLen < sizeof(serialLine) - 1) {
      serialLine[serialLen++] = c;
    }
  }
}

void httpHeader(EthernetClient &c, const __FlashStringHelper *contentType) {
  c.println(F("HTTP/1.1 200 OK"));
  c.print(F("Content-Type: "));
  c.println(contentType);
  c.println(F("Cache-Control: no-store"));
  c.println(F("Connection: close"));
  c.println();
}

void httpRedirect(EthernetClient &c) {
  c.println(F("HTTP/1.1 303 See Other"));
  c.println(F("Location: /"));
  c.println(F("Cache-Control: no-store"));
  c.println(F("Connection: close"));
  c.println();
}

void httpError(EthernetClient &c, uint16_t code) {
  if (code == 405) c.println(F("HTTP/1.1 405 Method Not Allowed"));
  else c.println(F("HTTP/1.1 404 Not Found"));
  c.println(F("Content-Type: text/plain; charset=utf-8"));
  c.println(F("Connection: close"));
  c.println();
  c.println(code == 405 ? F("Method Not Allowed") : F("Not Found"));
}

void jsonStatus(EthernetClient &c) {
  httpHeader(c, F("application/json; charset=utf-8"));
  c.print(F("{\"relay\":["));
  for (uint8_t i = 0; i < 4; ++i) {
    c.print(relayState[i] ? F("true") : F("false"));
    if (i != 3) c.print(',');
  }
  c.print(F("],\"active_low\":"));
  c.print(RELAY_ACTIVE_LOW ? F("true") : F("false"));
  c.print(F(",\"uptime_s\":"));
  c.print(millis() / 1000UL);
  c.print(F(",\"requests\":"));
  c.print(requestCount);
  c.print(F(",\"ip\":\""));
  c.print(Ethernet.localIP());
  c.println(F("\"}"));
}

void relayCard(EthernetClient &c, uint8_t index) {
  c.print(F("<div class='card'><h2>Relay "));
  c.print(index + 1);
  c.print(F(" <span class='"));
  c.print(relayState[index] ? F("on") : F("off"));
  c.print(F("'>"));
  c.print(relayState[index] ? F("ON") : F("OFF"));
  c.println(F("</span></h2>"));

  c.print(F("<a class='bon' href='/r"));
  c.print(index + 1);
  c.println(F("/on'>ON</a>"));

  c.print(F("<a class='boff' href='/r"));
  c.print(index + 1);
  c.println(F("/off'>OFF</a>"));

  c.print(F("<a class='btog' href='/r"));
  c.print(index + 1);
  c.println(F("/toggle'>TOGGLE</a></div>"));
}

void dashboard(EthernetClient &c) {
  httpHeader(c, F("text/html; charset=utf-8"));
  c.println(F("<!doctype html><html><head><meta charset='utf-8'>"));
  c.println(F("<meta name='viewport' content='width=device-width,initial-scale=1'>"));
  c.println(F("<title>KON 4-Relay Controller</title>"));
  c.println(F("<style>body{font-family:Arial;background:#111827;color:#f3f4f6;max-width:760px;margin:20px auto;padding:12px}"));
  c.println(F(".grid{display:grid;grid-template-columns:repeat(2,1fr);gap:12px}.card{background:#1f2937;padding:16px;border-radius:12px}"));
  c.println(F("a{display:inline-block;text-decoration:none;color:white;padding:11px 16px;margin:4px;border-radius:8px;font-weight:bold}"));
  c.println(F(".bon{background:#15803d}.boff{background:#b91c1c}.btog{background:#1d4ed8}.on{color:#4ade80}.off{color:#f87171}"));
  c.println(F(".all{background:#374151;padding:14px;border-radius:12px;margin-top:14px}@media(max-width:560px){.grid{grid-template-columns:1fr}}</style>"));
  c.println(F("</head><body><h1>Arduino UNO + W5100</h1><h2>KON 4-Relay Controller</h2><div class='grid'>"));

  for (uint8_t i = 0; i < 4; ++i) relayCard(c, i);

  c.println(F("</div><div class='all'><h2>All relays</h2>"));
  c.println(F("<a class='bon' href='/all/on'>ALL ON</a><a class='boff' href='/all/off'>ALL OFF</a>"));
  c.println(F("<a class='btog' href='/'>REFRESH</a><a class='btog' href='/api/status'>JSON</a></div>"));
  c.print(F("<p>IP: "));
  c.print(Ethernet.localIP());
  c.print(F(" | Uptime: "));
  c.print(millis() / 1000UL);
  c.print(F(" s | Requests: "));
  c.print(requestCount);
  c.println(F("</p>"));
  c.print(F("<p>Relay logic: "));
  c.print(RELAY_ACTIVE_LOW ? F("ACTIVE LOW") : F("ACTIVE HIGH"));
  c.println(F(". microSD must be removed. Trusted LAN only.</p></body></html>"));
}

bool parseRelayAction(const char *path, uint8_t &index, uint8_t &action, bool &apiMode) {
  apiMode = false;
  const char *p = path;

  if (!strncmp(p, "/api/", 5)) {
    apiMode = true;
    p += 4;
  }

  if (p[0] != '/' || p[1] != 'r' || p[2] < '1' || p[2] > '4' || p[3] != '/') return false;

  index = (uint8_t)(p[2] - '1');
  const char *a = p + 4;

  if (!strcmp(a, "on")) action = 1;
  else if (!strcmp(a, "off")) action = 2;
  else if (!strcmp(a, "toggle")) action = 3;
  else return false;

  return true;
}

bool parseAllAction(const char *path, uint8_t &action, bool &apiMode) {
  apiMode = false;
  const char *p = path;

  if (!strncmp(p, "/api/", 5)) {
    apiMode = true;
    p += 4;
  }

  if (!strcmp(p, "/all/on")) {
    action = 1;
    return true;
  }

  if (!strcmp(p, "/all/off")) {
    action = 2;
    return true;
  }

  return false;
}

void applyAction(uint8_t index, uint8_t action) {
  if (action == 1) setRelay(index, true);
  else if (action == 2) setRelay(index, false);
  else if (action == 3) toggleRelay(index);
}

void handleHttp() {
  EthernetClient c = server.available();
  if (!c) return;

  char method[5] = {0};
  char path[32] = {0};
  uint8_t mi = 0, pi = 0, stage = 0;
  bool complete = false;
  unsigned long start = millis();

  while (c.connected() && millis() - start < 1500UL && !complete) {
    if (!c.available()) continue;

    char ch = (char)c.read();

    if (ch == '\n') {
      complete = true;
      break;
    }

    if (ch == '\r') continue;

    if (stage == 0) {
      if (ch == ' ') {
        stage = 1;
        continue;
      }
      if (mi < sizeof(method) - 1) method[mi++] = ch;
    } else if (stage == 1) {
      if (ch == ' ') {
        stage = 2;
        continue;
      }
      if (pi < sizeof(path) - 1) path[pi++] = ch;
    }
  }

  ++requestCount;

  if (!complete || strcmp(method, "GET")) {
    httpError(c, 405);
    delay(2);
    c.stop();
    return;
  }

  if (!strcmp(path, "/")) {
    dashboard(c);
  } else if (!strcmp(path, "/api") || !strcmp(path, "/api/status")) {
    jsonStatus(c);
  } else {
    uint8_t index = 0, action = 0;
    bool apiMode = false;

    if (parseRelayAction(path, index, action, apiMode)) {
      applyAction(index, action);
      if (apiMode) jsonStatus(c);
      else httpRedirect(c);

      Serial.print(F("HTTP R"));
      Serial.print(index + 1);
      Serial.print(' ');
      Serial.println(relayState[index] ? F("ON") : F("OFF"));
    } else if (parseAllAction(path, action, apiMode)) {
      setAll(action == 1);
      if (apiMode) jsonStatus(c);
      else httpRedirect(c);

      Serial.print(F("HTTP ALL "));
      Serial.println(action == 1 ? F("ON") : F("OFF"));
    } else {
      httpError(c, 404);
    }
  }

  delay(2);
  c.stop();
}

void setup() {
  Serial.begin(115200);

  for (uint8_t i = 0; i < 4; ++i) {
    digitalWrite(RELAY_PINS[i], relayLevel(false));
    pinMode(RELAY_PINS[i], OUTPUT);
    setRelay(i, false);
  }

  pinMode(ETH_CS, OUTPUT);
  digitalWrite(ETH_CS, HIGH);

  delay(500);

  Serial.println(F("KON APP-01 / W5100 4-RELAY CONTROLLER"));
  printRelayStatus(Serial);
  Serial.print(F("RELAY LOGIC: "));
  Serial.println(RELAY_ACTIVE_LOW ? F("ACTIVE LOW") : F("ACTIVE HIGH"));
  printHelp();

  Ethernet.init(ETH_CS);
  Serial.println(F("DHCP request..."));

  if (Ethernet.begin(mac, 10000, 2000) == 0) {
    Serial.println(F("DHCP FAIL / WEB DISABLED / SERIAL CLI AVAILABLE"));
    return;
  }

  networkReady = true;
  server.begin();

  Serial.print(F("IP: "));
  Serial.println(Ethernet.localIP());
  Serial.print(F("WEB: http://"));
  Serial.print(Ethernet.localIP());
  Serial.println('/');
  Serial.println(F("READY"));
}

void loop() {
  pollSerial();

  if (!networkReady) return;

  if (millis() - maintainTick >= 30000UL) {
    maintainTick = millis();
    int rc = Ethernet.maintain();

    if (rc == 1 || rc == 3) Serial.println(F("DHCP MAINTAIN FAIL"));
    if (rc == 2 || rc == 4) {
      Serial.print(F("NEW IP: "));
      Serial.println(Ethernet.localIP());
    }
  }

  handleHttp();
}
