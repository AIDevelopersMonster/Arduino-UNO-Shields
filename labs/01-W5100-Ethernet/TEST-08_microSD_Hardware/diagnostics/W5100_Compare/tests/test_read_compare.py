"""Compile actual comparison helpers against the existing SPI ownership model.

Python 3/g++ required. The LIB peripheral is a frame-compatible mock, not
the real Ethernet driver; the Arduino build separately compiles that driver.
No analog signal, waveform or physical root cause is modeled here.
"""
from pathlib import Path
import ast
import subprocess
import tempfile

here = Path(__file__).resolve().parent
source = (here.parent / 'W5100_Compare.ino').read_text()
shared = here.parents[2] / 'tests/test_spi_release.py'
tree = ast.parse(shared.read_text())
model = next(ast.literal_eval(node.value) for node in tree.body
             if isinstance(node, ast.Assign) and any(isinstance(t, ast.Name) and t.id == 'model' for t in node.targets))

def function(signature):
    start = source.index(signature)
    end = source.index('{', start) + 1
    depth = 1
    while depth:
        if source[end] == '{': depth += 1
        elif source[end] == '}': depth -= 1
        end += 1
    return source[start:end]

model = model[:model.index('ACTUAL_HELPERS')]
model = model.replace('ETH_CLOCK', '4000000')
model = model.replace('#include <iostream>', '#include <iostream>\n#include <sstream>')
model = model.replace('int pins[16]', 'uint8_t SPCR=0x50,SPSR=0;\nint pins[16]')
model = model.replace('active=true;clock=s.clock;', 'active=true;clock=s.clock;SPCR=0x50;SPSR=0;')
model += r'''
struct __FlashStringHelper {};
#define F(s) reinterpret_cast<const __FlashStringHelper *>(s)
struct SerialModel {
  std::ostringstream output;
  void print(const __FlashStringHelper *s){output<<reinterpret_cast<const char *>(s);}
  void print(uint8_t v){output<<unsigned(v);}
  void print(uint16_t v,int){output<<std::uppercase<<std::hex<<v<<std::dec;}
  template<class T> void print(T v){output<<v;}
  template<class T> void println(T v){print(v);output<<'\n';}
  void println(){output<<'\n';}
} Serial;
const int HEX=16;
char token[]="A1B2C3D4";
uint16_t baselineRtr=0;
uint8_t rtrSamples=0,compareFailures=0;
// Model only the W5100 0F/16-bit-address/8-bit-data driver contract.
struct DriverModel {
  uint16_t readRTR(){
    uint8_t bytes[2];
    for(uint16_t i=0;i<2;i++){
      digitalWrite(ETH_CS,LOW);
      SPI.transfer(0x0F);SPI.transfer(0);SPI.transfer(uint8_t(0x17+i));
      bytes[i]=SPI.transfer(0);digitalWrite(ETH_CS,HIGH);
    }
    return (uint16_t(bytes[0])<<8)|bytes[1];
  }
} W5100;
'''
for signature in ['void initializeChipSelects(', 'void prefix(', 'void hex16(',
                  'bool beginEthernetTransaction(', 'uint8_t rawRead(',
                  'uint16_t rawRtr(', 'uint16_t libraryRtr(', 'bool comparePhase(']:
    model += '\n' + function(signature) + '\n'
model += r'''
void require(bool value,const char* message){if(!value)throw std::runtime_error(message);}
void resetObservations(){baselineRtr=0;rtrSamples=compareFailures=0;Serial.output.str("");}
int main(){
  try{
    initializeChipSelects();SPI.registers[ETH_RTR]=7;SPI.registers[ETH_RTR+1]=0xD0;
    require(comparePhase(F("BEFORE"),true),"complete comparison failed");
    require(baselineRtr==0x07D0&&rtrSamples==12&&compareFailures==0,"baseline/count");
    require(comparePhase(F("AFTER_IO"),false)&&comparePhase(F("AFTER_REMOUNT"),false),"later phase");
    require(rtrSamples==36&&compareFailures==0,"total count");
    resetObservations();SPI.corruptOnce=true;
    require(!comparePhase(F("BEFORE"),true),"first bad raw read hidden");
    require(compareFailures==1&&rtrSamples==12&&baselineRtr==0x07D0,"bad-first/good-later accounting");
    require(Serial.output.str().find("method=RAW value=0750")!=std::string::npos,"actual bad value not logged");
    require(comparePhase(F("AFTER_IO"),false)&&compareFailures==1,"earlier failure was discarded");
    resetObservations();pins[SD_CS]=LOW;unsigned before=SPI.transfers;
    require(!comparePhase(F("BEFORE"),true)&&busFault&&rtrSamples==0,"owner fault did not abort");
    require(pins[SD_CS]==LOW&&SPI.transfers==before,"owner forcibly deselected or clocked");
    pins[SD_CS]=HIGH;busFault=false;resetObservations();SPI.registers[ETH_RTR]=SPI.registers[ETH_RTR+1]=0xFF;
    require(!comparePhase(F("BEFORE"),true)&&compareFailures==12,"FFFF accepted as baseline");
    require(!SPI.active&&pins[SD_CS]==HIGH&&pins[ETH_CS]==HIGH,"bus left active");
    std::cout<<"READ COMPARE MODEL PASS: frames/ownership, 36 observations, bad-first retention, FFFF rejection; no hardware result\n";
  }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}
'''
with tempfile.TemporaryDirectory(prefix='w5100-compare-model-') as directory:
    directory = Path(directory)
    cpp, binary = directory/'model.cpp', directory/'model'
    cpp.write_text(model)
    subprocess.run(['g++','-std=c++11','-Wall','-Wextra','-Werror',str(cpp),'-o',str(binary)],check=True,timeout=30)
    subprocess.run([str(binary)],check=True,timeout=5)
