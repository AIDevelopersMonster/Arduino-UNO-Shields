"""Compile the sketch's real SPI helpers against a shared-bus peripheral model.

Development only: requires Python 3 and g++. It does not emulate a physical card.
An optional source path allows demonstrating that the earlier firmware fails
when SD keeps MISO driven until an idle byte is clocked with both CS high.
"""
from pathlib import Path
import re
import subprocess
import sys
import tempfile

source = Path(sys.argv[1]) if len(sys.argv) > 1 else Path(__file__).resolve().parents[1] / "TEST-08_microSD_Hardware.ino"
sketch = source.read_text()
start = sketch.index("void beginEthernetTransaction()") if "void beginEthernetTransaction()" in sketch else sketch.index("uint8_t ethernetRead(")
helpers = sketch[start:sketch.index("uint8_t expectedByte(")]
clock_match = re.search(r"const uint32_t ETH_SPI_HZ = ([0-9]+)UL;", sketch)
clock = int(clock_match.group(1)) if clock_match else 4000000
model = r'''
#include <cstdint>
#include <iostream>
#include <stdexcept>
using std::uint8_t; using std::uint16_t; using std::uint32_t;
const int HIGH=1, LOW=0, MSBFIRST=1, SPI_MODE0=0;
const uint8_t ETH_CS=10, SD_CS=4;
const uint16_t ETH_RTR=0x0017;
const uint32_t ETH_SPI_HZ=MODEL_CLOCK;
int pins[16];
struct SPISettings {
  SPISettings(int clock, int order, int mode) {
    if(clock!=MODEL_CLOCK || order!=MSBFIRST || mode!=SPI_MODE0) throw std::runtime_error("settings");
  }
};
struct SpiModel {
  bool active=false, cardDrives=false;
  uint8_t registers[65536] = {};
  uint8_t opcode=0; unsigned frame=0; uint16_t address=0;
  void beginTransaction(SPISettings){ if(active) throw std::runtime_error("nested transaction"); active=true; }
  void endTransaction(){ if(!active || pins[ETH_CS]!=HIGH) throw std::runtime_error("unbalanced transaction"); active=false; }
  uint8_t transfer(uint8_t byte) {
    if(!active) throw std::runtime_error("clock outside transaction");
    if(pins[ETH_CS]==HIGH && pins[SD_CS]==HIGH){cardDrives=false;return 0xFF;}
    if(pins[ETH_CS]!=LOW || pins[SD_CS]!=HIGH || cardDrives) throw std::runtime_error("shared-bus collision");
    switch(frame++) {
      case 0: opcode=byte;return 0xFF;
      case 1: address=uint16_t(byte)<<8;return 0xFF;
      case 2: address|=byte;return 0xFF;
      case 3:
        if(opcode==0x0F)return registers[address];
        if(opcode==0xF0){registers[address]=byte;return 0xFF;}
        throw std::runtime_error("W5100 opcode");
      default:throw std::runtime_error("W5100 frame length");
    }
  }
} SPI;
void digitalWrite(int pin,int value){
  if(pin==ETH_CS && value==LOW){
    if(pins[SD_CS]!=HIGH || SPI.cardDrives) throw std::runtime_error("SD has not released MISO before W5100 select");
    SPI.frame=0;
  }
  pins[pin]=value;
}
SKETCH_HELPERS
int main(){
  try {
    pins[ETH_CS]=pins[SD_CS]=HIGH;
    SPI.registers[ETH_RTR]=0x07;SPI.registers[ETH_RTR+1]=0xD0;
    if(readRtr()!=0x07D0)throw std::runtime_error("initial RTR");
    writeRtr(0x1234);
    if(readRtr()!=0x1234)throw std::runtime_error("write/read probe");
    writeRtr(0x07D0);
    SPI.cardDrives=true; // SD operation finished; CS is high, clocks stopped.
    if(readRtr()!=0x07D0 || readRtr()!=0x07D0)throw std::runtime_error("post-SD RTR");
    SPI.cardDrives=true;
    writeRtr(0x1234);
    if(readRtr()!=0x1234)throw std::runtime_error("post-SD write");
    writeRtr(0x07D0);
    if(SPI.active || pins[ETH_CS]!=HIGH || pins[SD_CS]!=HIGH)throw std::runtime_error("final bus state");
    std::cout<<"SPI MODEL PASS: actual sketch helpers release shared bus; no hardware result\n";
    return 0;
  }catch(const std::exception& error){std::cerr<<"SPI MODEL FAIL: "<<error.what()<<"\n";return 1;}
}
'''
with tempfile.TemporaryDirectory(prefix="test08-spi-model-") as directory:
    directory=Path(directory)
    cpp=directory/"spi_model.cpp";binary=directory/"spi_model"
    cpp.write_text(model.replace("SKETCH_HELPERS",helpers).replace("MODEL_CLOCK",str(clock)))
    subprocess.run(["g++","-std=c++11","-Wall","-Wextra","-Werror",str(cpp),"-o",str(binary)],check=True,timeout=30)
    raise SystemExit(subprocess.run([str(binary)],timeout=5).returncode)
