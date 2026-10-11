"""Compile actual TEST-08/09 helpers against a protocol/ownership model.

Requires Python 3 and g++. Checks startup latches, driver-owned read completion,
selected-slave rejection, release clocks and unsuppressed bad read bytes.
This does not emulate analog signals or establish the physical failure cause.
An optional earlier sketch path permits a negative regression demonstration.
"""
from pathlib import Path
import re
import subprocess
import sys
import tempfile

base = Path(__file__).resolve().parents[2]
paths = [Path(sys.argv[1])] if len(sys.argv) > 1 else [
    base / 'TEST-08_microSD_Hardware/TEST-08_microSD_Hardware.ino',
    base / 'TEST-09_Ethernet_SD_Integration/TEST-09_Ethernet_SD_Integration.ino',
]

def function(source, signature):
    start = source.index(signature)
    i = source.index('{', start) + 1
    depth = 1
    while depth:
        if source[i] == '{': depth += 1
        elif source[i] == '}': depth -= 1
        i += 1
    return source[start:i]

model = r'''
#include <cstdint>
#include <iostream>
#include <stdexcept>
using std::uint8_t; using std::uint16_t; using std::uint32_t;
const int HIGH=1,LOW=0,OUTPUT=1,MSBFIRST=1,SPI_MODE0=0;
const uint8_t ETH_CS=10,SD_CS=4;
const uint16_t ETH_RTR=0x0017,RTR_ADDRESS=0x0017;
const uint32_t ETH_SPI_HZ=ETH_CLOCK,SD_SPI_HZ=4000000;
int pins[16]={},directions[16]={};
bool sdReady=true,busFault=false,driverDeselect=false;
struct CardModel {bool partial=false;void readEnd();} card;
struct SPISettings {
  uint32_t clock;
  SPISettings(uint32_t c,int order,int mode):clock(c){
    if((c!=ETH_SPI_HZ&&c!=SD_SPI_HZ)||order!=MSBFIRST||mode!=SPI_MODE0)
      throw std::runtime_error("invalid SPI settings");
  }
};
struct SpiModel {
  bool active=false,cardDrives=false,corruptOnce=false;
  uint32_t clock=0;unsigned transfers=0;
  uint8_t registers[65536]={},opcode=0;unsigned frame=0;uint16_t address=0;
  void begin(){}
  void beginTransaction(SPISettings s){
    if(active)throw std::runtime_error("configuration overwritten during active transaction");
    if(pins[SD_CS]!=HIGH||pins[ETH_CS]!=HIGH)throw std::runtime_error("configuration changed with selected slave");
    active=true;clock=s.clock;
  }
  void endTransaction(){
    if(!active||pins[ETH_CS]!=HIGH||pins[SD_CS]!=HIGH)throw std::runtime_error("unbalanced transaction/select");
    active=false;
  }
  uint8_t transfer(uint8_t byte){
    if(!active)throw std::runtime_error("clock outside transaction");
    ++transfers;
    if(pins[ETH_CS]==HIGH&&pins[SD_CS]==HIGH){cardDrives=false;return 0xFF;}
    if(pins[SD_CS]==LOW&&pins[ETH_CS]==HIGH){
      if(!card.partial||clock!=SD_SPI_HZ)throw std::runtime_error("SD completion used wrong owner/settings");
      return 0xFF;
    }
    if(pins[ETH_CS]!=LOW||pins[SD_CS]!=HIGH||cardDrives)throw std::runtime_error("shared-bus collision");
    switch(frame++){
      case 0:opcode=byte;return 0xFF;
      case 1:address=uint16_t(byte)<<8;return 0xFF;
      case 2:address|=byte;return 0xFF;
      case 3:
        if(opcode==0x0F){uint8_t value=registers[address];
          if(corruptOnce&&address==ETH_RTR+1){value^=0x80;corruptOnce=false;}return value;}
        if(opcode==0xF0){registers[address]=byte;return 0xFF;}
        throw std::runtime_error("W5100 opcode");
      default:throw std::runtime_error("W5100 frame length");
    }
  }
} SPI;
int digitalRead(int pin){return pins[pin];}
void pinMode(int pin,int mode){
  if(mode==OUTPUT&&pins[pin]!=HIGH)throw std::runtime_error("CS became output while latch was LOW");
  directions[pin]=mode;
}
void digitalWrite(int pin,int value){
  if(pin==SD_CS&&value==HIGH&&pins[pin]==LOW&&directions[pin]==OUTPUT&&!driverDeselect)
    throw std::runtime_error("application forcibly deselected unfinished SD operation");
  if(pin==ETH_CS&&value==LOW){
    if(pins[SD_CS]!=HIGH||SPI.cardDrives)throw std::runtime_error("W5100 selected before SD release");
    SPI.frame=0;
  }
  pins[pin]=value;
}
void CardModel::readEnd(){
  if(!partial)return;
  // Model the driver's public completion contract: remaining data/CRC first.
  for(unsigned i=0;i<4;++i)SPI.transfer(0xFF);
  driverDeselect=true;digitalWrite(SD_CS,HIGH);driverDeselect=false;
  SPI.endTransaction();partial=false;SPI.cardDrives=true;
}
ACTUAL_HELPERS
void require(bool value,const char* label){if(!value)throw std::runtime_error(label);}
int main(){
  try{
    INITIALIZE
    SPI.registers[ETH_RTR]=0x07;SPI.registers[ETH_RTR+1]=0xD0;
    require(readRtr()==0x07D0,"initial RTR");
    WRITE_PROBE
    SPI.cardDrives=true;
    require(readRtr()==0x07D0&&readRtr()==0x07D0,"post-SD release");
    // A legitimate pending partial read must complete through the SD driver.
    SPI.beginTransaction(SPISettings(SD_SPI_HZ,MSBFIRST,SPI_MODE0));
    pins[SD_CS]=LOW;card.partial=true;
    require(readRtr()==0x07D0&&!card.partial&&!SPI.active,"partial read handoff");
    // A selected card with no completable read is an ownership fault.
    pins[SD_CS]=LOW;unsigned before=SPI.transfers;
    require(readRtr()==0xFFFF&&busFault,"selected SD did not produce latched fault");
    require(pins[SD_CS]==LOW&&SPI.transfers==before,"fault was hidden by deselection/clocks");
    pins[SD_CS]=HIGH;
    require(readRtr()==0xFFFF&&SPI.transfers==before,"latched fault was silently recovered");
    busFault=false;pins[ETH_CS]=LOW;before=SPI.transfers;
    require(readRtr()==0xFFFF&&busFault&&SPI.transfers==before,"selected Ethernet owner ignored");
    pins[ETH_CS]=HIGH;busFault=false;
    // Return corruption unchanged; no retry, voting or overwrite with next read.
    SPI.corruptOnce=true;
    require(readRtr()==0x0750&&readRtr()==0x07D0,"bad first read was suppressed");
    require(!SPI.active&&pins[ETH_CS]==HIGH&&pins[SD_CS]==HIGH,"final bus state");
    std::cout<<"SPI ORDER MODEL PASS: latches, partial completion, owner rejection, idle clocks, exact bad byte; no hardware result\n";
    return 0;
  }catch(const std::exception& e){std::cerr<<"SPI ORDER MODEL FAIL: "<<e.what()<<"\n";return 1;}
}
'''
for path in paths:
    source=path.read_text()
    clock=re.search(r'const uint32_t ETH_SPI_HZ = ([0-9]+)UL;',source)
    clock=int(clock.group(1)) if clock else 4000000
    is08='uint8_t ethernetRead(' in source
    signatures=(['bool beginEthernetTransaction('] if 'bool beginEthernetTransaction(' in source else
                ['void beginEthernetTransaction(']) if is08 else ['bool releaseBus(']
    signatures += ['uint8_t ethernetRead(', 'void ethernetWrite(', 'uint16_t readRtr(', 'void writeRtr('] if is08 else ['uint8_t readEthernet(', 'uint16_t readRtr(']
    init='initializeChipSelects();'
    if 'void initializeChipSelects(' in source:signatures.insert(0,'void initializeChipSelects(')
    else:init='pins[ETH_CS]=pins[SD_CS]=HIGH;directions[ETH_CS]=directions[SD_CS]=OUTPUT;'
    helpers='\n'.join(function(source,s) for s in signatures)
    write='writeRtr(0x1234);require(readRtr()==0x1234,"probe");writeRtr(0x07D0);' if is08 else ''
    cpp_source=model.replace('ETH_CLOCK',str(clock)).replace('ACTUAL_HELPERS',helpers).replace('INITIALIZE',init).replace('WRITE_PROBE',write)
    with tempfile.TemporaryDirectory(prefix='spi-order-model-') as directory:
        directory=Path(directory);cpp=directory/'model.cpp';binary=directory/'model'
        cpp.write_text(cpp_source)
        subprocess.run(['g++','-std=c++11','-Wall','-Wextra','-Werror',str(cpp),'-o',str(binary)],check=True,timeout=30)
        result=subprocess.run([str(binary)],timeout=5)
        print(path.name,flush=True)
        if result.returncode:raise SystemExit(result.returncode)
