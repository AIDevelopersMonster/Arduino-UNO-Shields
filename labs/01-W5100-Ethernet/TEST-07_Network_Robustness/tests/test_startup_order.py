"""Compile the actual sketch against a strict driver-lifecycle model.

Register/socket access before successful driver initialization raises an error.
No physical UNO, W5100, card, DHCP server or SPI waveform is simulated/certified.
Run: python tests/test_startup_order.py [--sketch path/to/sketch.ino]
"""
import argparse
import pathlib
import subprocess
import tempfile

STUB = r'''
#pragma once
#include <algorithm>
#include <cstdint>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>
using byte = uint8_t;
struct __FlashStringHelper {};
#define F(s) reinterpret_cast<const __FlashStringHelper *>(s)
#define HIGH 1
#define OUTPUT 1
using std::min;
inline uint32_t tick = 0;
inline bool driver = false, nextChip = true;
inline int lease = 1, maintenance = 0, begins = 0, ioCalls = 0;
inline bool udpBind = true;
inline int pins[16] = {};
inline std::vector<std::string> operations;
inline uint32_t millis() { return tick; }
inline void delay(uint32_t t) { tick += t; }
inline void digitalWrite(uint8_t p, int v) { pins[p] = v; }
inline void pinMode(uint8_t p, int) {
  if ((p == 4 || p == 10) && pins[p] != HIGH)
    throw std::runtime_error("CS driven OUTPUT before inactive latch");
}
inline void io(const std::string &op) {
  if (!driver) throw std::runtime_error("uninitialized driver access: " + op);
  operations.push_back(op); ++ioCalls;
}
struct IPAddress {
  uint8_t b[4] = {};
  IPAddress() = default;
  IPAddress(uint8_t a, uint8_t c, uint8_t d, uint8_t e): b{a,c,d,e} {}
  bool operator==(const IPAddress &r) const {
    return std::equal(b,b+4,r.b);
  }
  bool operator!=(const IPAddress &r) const { return !(*this == r); }
};
inline std::ostream &operator<<(std::ostream &s, const IPAddress &ip) {
  return s << unsigned(ip.b[0]) << '.' << unsigned(ip.b[1]) << '.'
           << unsigned(ip.b[2]) << '.' << unsigned(ip.b[3]);
}
struct SerialModel {
  std::string out = "reset-noise", input;
  void begin(int) {}
  int available() { return !input.empty(); }
  int read() { char c = input.front(); input.erase(0,1); return c; }
  template<class T> size_t print(const T &v) {
    std::ostringstream s; s << v; out += s.str(); return s.str().size();
  }
  size_t print(const __FlashStringHelper *v) {
    auto s = reinterpret_cast<const char *>(v); out += s;
    return std::char_traits<char>::length(s);
  }
  void println() { out += '\n'; }
  template<class T> void println(const T &v) { print(v); println(); }
};
inline SerialModel Serial;
enum EthernetHardwareStatus { EthernetNoHardware, EthernetW5100 };
enum EthernetLinkStatus { Unknown, LinkON, LinkOFF };
inline IPAddress currentIP;
struct EthernetModel {
  void init(uint8_t) {} // Only selects SS, as in Ethernet 2.0.2.
  int begin(byte *, uint32_t, uint32_t) {
    ++begins;
    if (Serial.out.find("name=DHCP_BEGIN") == std::string::npos)
      throw std::runtime_error("begin without prior diagnostic event");
    operations.push_back("begin"); driver = nextChip; tick += 100;
    currentIP = (driver && lease == 1) ? IPAddress(192,0,2,1) : IPAddress();
    return driver ? lease : 0;
  }
  EthernetHardwareStatus hardwareStatus() {
    return driver ? EthernetW5100 : EthernetNoHardware;
  }
  void setLocalIP(IPAddress ip) { io("setLocalIP"); currentIP = ip; }
  void setRetransmissionTimeout(int) { io("RTR"); }
  void setRetransmissionCount(int) { io("RCR"); }
  IPAddress localIP() { io("localIP"); return currentIP; }
  IPAddress subnetMask() { io("mask"); return IPAddress(255,255,255,0); }
  IPAddress gatewayIP() { io("gateway"); return IPAddress(192,0,2,254); }
  IPAddress dnsServerIP() { return IPAddress(192,0,2,254); }
  EthernetLinkStatus linkStatus() { io("link"); return Unknown; }
  int maintain() { io("maintain"); return maintenance; }
};
inline EthernetModel Ethernet;
struct EthernetClient {
  int index = 4;
  EthernetClient() = default;
  explicit EthernetClient(uint8_t i): index(i) {}
  explicit operator bool() const { return index < 4; }
  void setConnectionTimeout(int) {}
  void stop() { if(index < 4) { io("socketDisconnect"); index = 4; } }
  int available() { io("clientAvailable"); return 0; }
  int read() { io("clientRead"); return -1; }
  bool connected() { io("clientConnected"); return false; }
  size_t print(const __FlashStringHelper *) { io("clientPrint"); return 5; }
  size_t write(uint8_t *, size_t n) { io("clientWrite"); return n; }
  size_t write(uint8_t) { io("clientWriteByte"); return 1; }
};
struct EthernetServer {
  bool active = false;
  explicit EthernetServer(uint16_t) {}
  void begin() { io("serverBegin"); active = true; }
  explicit operator bool() const { return active; }
  EthernetClient accept() { io("accept"); return EthernetClient(); }
};
struct EthernetUDP {
  bool active = false;
  void stop() { if(active) { io("udpStop"); active = false; } }
  int begin(uint16_t) { io("udpBegin"); active = udpBind; return active; }
  int parsePacket() { io("parsePacket"); return 0; }
  int read(uint8_t *, int) { io("udpRead"); return 0; }
  IPAddress remoteIP() { io("remoteIP"); return IPAddress(192,0,2,2); }
  uint16_t remotePort() { io("remotePort"); return 1234; }
  int beginPacket(IPAddress, uint16_t) { io("beginPacket"); return 1; }
  size_t write(uint8_t *, size_t n) { io("udpWrite"); return n; }
  int endPacket() { io("endPacket"); return 1; }
};
'''

MAIN = r'''
int __heap_start = 0;
void *__brkval = nullptr;
void check(bool value, const char *message) {
  if (!value) throw std::runtime_error(message);
}
int main(int argc, char **argv) {
  try {
    std::string name = argv[1];
    // Direct call isolates the old pre-init socket bug from the old CS setup.
    if (name == "direct-initial") {
      acquireDhcp(); check(ready && begins == 1, "first DHCP acquisition");
    } else {
      setup();
      check(Serial.out.find("reset-noise\nEVT ms=") == 0, "BOOT boundary");
      if (name == "no-chip") {
        nextChip = false; loop();
        check(!ready && begins == 1 && ioCalls == 0, "no-chip must not access registers");
        check(Serial.out.find("reason=NO_HARDWARE") != std::string::npos, "missing no-chip reason");
        stats(); check(ioCalls == 0, "STAT accessed absent chip");
        nextChip = true; tick = retryTick + RETRY_MS; loop();
        check(ready && begins == 2, "recovery after absent chip");
      } else if (name == "no-lease") {
        lease = 0; loop();
        check(!ready && begins == 1 && dhcpFail == 1, "DHCP failure missing");
        tick = retryTick + RETRY_MS - 1; loop();
        check(begins == 1, "retry before deadline");
        lease = 1; tick = retryTick + RETRY_MS; loop();
        check(ready && begins == 2, "DHCP retry/recovery missing");
      } else if (name == "query-first") {
        Serial.input = "?"; loop();
        check(ready && begins == 1, "initial query blocked acquisition");
        check(Serial.out.find("ready=0 ip=0.0.0.0") != std::string::npos, "initial STAT IP");
      } else if (name == "manual-first") {
        Serial.input = "D"; loop();
        check(!ready && begins == 0 && ioCalls == 0, "manual command before initialization");
        tick = retryTick + RETRY_MS; loop();
        check(ready && begins == 1, "scheduled manual retry");
      } else {
        loop(); check(ready && begins == 1, "normal startup");
        if (name == "renew-failure") {
          maintenance = 1; tick = maintainTick + MAINTAIN_MS; loop();
          check(!ready && renewFail == 1, "maintenance failure not scheduled");
          maintenance = 0; tick = retryTick + RETRY_MS; loop();
          check(ready && begins == 2 && serviceRestarts == 2, "renew failure recovery");
        } else if (name == "changed-ip") {
          maintenance = 2; currentIP = IPAddress(192,0,2,3);
          tick = maintainTick + MAINTAIN_MS; loop();
          check(ready && renewOK == 1 && serviceRestarts == 2, "address-change rebind");
          check(boundIP == currentIP, "old bound address");
        }
      }
    }
    check(!operations.empty() && operations.front() == "begin", "hardware operation before first begin");
    std::cout << "MODEL PASS / " << name << '\n';
    return 0;
  } catch (const std::exception &e) {
    std::cerr << "MODEL FAIL / " << argv[1] << " / " << e.what() << '\n';
    return 1;
  }
}
'''

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--sketch", type=pathlib.Path,
                        default=pathlib.Path(__file__).resolve().parents[1] /
                        "TEST-07_Network_Robustness.ino")
    parser.add_argument("--cxx", default="g++")
    args = parser.parse_args()
    cases = ("direct-initial", "normal", "no-chip", "no-lease", "query-first",
             "manual-first", "renew-failure", "changed-ip")
    with tempfile.TemporaryDirectory(prefix="test07-startup-") as folder:
        root = pathlib.Path(folder)
        for header in ("SPI.h", "Ethernet.h", "EthernetUdp.h"):
            (root / header).write_text('#include "model.h"\n')
        (root / "model.h").write_text(STUB)
        source = '#include <iostream>\n#include "model.h"\n' + args.sketch.read_text() + MAIN
        (root / "test.cpp").write_text(source)
        binary = root / "test"
        # AVR uses 16-bit pointers/int; the native RAM sampler is not meaningful.
        compile_result = subprocess.run([args.cxx, "-std=c++17", "-fpermissive",
                                         "-I", str(root), str(root / "test.cpp"),
                                         "-o", str(binary)], capture_output=True, text=True)
        if compile_result.returncode:
            raise RuntimeError(compile_result.stderr)
        for case in cases:
            subprocess.run([str(binary), case], check=True)
    print("8 lifecycle models passed; hardware and peak SRAM remain unverified.")

if __name__ == "__main__":
    main()
