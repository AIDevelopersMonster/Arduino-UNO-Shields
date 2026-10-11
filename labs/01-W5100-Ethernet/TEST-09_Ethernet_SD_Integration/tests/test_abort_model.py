"""Exercise the actual TEST-09 finish function with failing owners and closes.
Development model only. Requires Python 3 and g++; no physical hardware claim.
"""
from pathlib import Path
import subprocess
import tempfile

source=(Path(__file__).resolve().parents[1]/'TEST-09_Ethernet_SD_Integration.ino').read_text()
start=source.index('void finish(bool requested)')
i=source.index('{',start)+1;depth=1
while depth:
    if source[i]=='{':depth+=1
    elif source[i]=='}':depth-=1
    i+=1
function=source[start:i]
model=r'''
#include <cstdint>
#include <sstream>
#include <stdexcept>
#include <iostream>
using std::uint8_t;using std::uint16_t;using std::uint32_t;
#define F(x) x
bool active=true,busFault=false,ownsFile=true,ethernetReady=true,udpBound=true;
uint8_t errors=0;uint16_t originalRtr=0x07D0,finalCrc=0;
uint16_t received=24,verified=24,sent=24;
uint32_t bytes=1440,maxSdMs=0,started=1;
int16_t minFree=530,initialFree=560;
const char FILE_NAME[]="T09CHECK.BIN";
int fileCloses=0,rootCloses=0,removes=0,releaseCalls=0,reads=0,stops=0;
bool failFileClose=false,failRootClose=false,failRelease=false;
struct SerialModel{std::ostringstream stream;
 template<class T>void print(T value){stream<<value;}
 template<class T>void println(T value){stream<<value<<'\n';}
}Serial;
void prefix(const char* kind){Serial.print(kind);Serial.print(" token=0123ABCD");}
void hex16(uint16_t value){Serial.stream<<std::hex<<value<<std::dec;}
int16_t freeRam(){return 560;}
void sampleRam(){}
uint32_t millis(){return 30001;}
const int O_READ=1;
struct FileModel{bool opened=true,rootRole=false;
 bool isOpen(){return opened;}
 bool close(){opened=false;if(rootRole){++rootCloses;return !failRootClose;}++fileCloses;return !failFileClose;}
 bool open(FileModel*,const char*,int){return false;}
}file,root;
struct SdFile{static bool remove(FileModel*,const char*){++removes;return true;}};
bool releaseBus(){++releaseCalls;if(failRelease){busFault=true;return false;}return true;}
uint16_t readRtr(){++reads;return 0x07D0;}
struct UdpModel{void stop(){++stops;}}udp;
ACTUAL_FINISH
void require(bool value,const char* name){if(!value)throw std::runtime_error(name);}
void reset(){
 active=true;busFault=false;ownsFile=true;ethernetReady=true;udpBound=true;errors=0;
 file.opened=root.opened=true;root.rootRole=true;file.rootRole=false;
 failFileClose=failRootClose=failRelease=false;
 fileCloses=rootCloses=removes=releaseCalls=reads=stops=0;Serial.stream.str("");
}
bool passed(){return Serial.stream.str().find(" status=PASS ")!=std::string::npos;}
int main(){try{
 reset();finish(true);require(passed()&&removes==1&&reads==2&&stops==1,"normal finish");
 reset();busFault=true;finish(true);
 require(!passed()&&fileCloses==0&&rootCloses==0&&removes==0&&releaseCalls==0&&reads==0&&stops==0,"owner fault performed new bus IO");
 reset();failFileClose=true;finish(true);require(!passed()&&errors&&removes==0,"failed file close accepted");
 reset();failRootClose=true;finish(true);require(!passed()&&errors,"failed root close accepted");
 reset();failRelease=true;finish(true);require(!passed()&&busFault&&reads==0&&stops==0,"failed handoff performed Ethernet IO");
 reset();ethernetReady=false;udpBound=false;file.opened=root.opened=false;errors=1;finish(false);
 require(!passed()&&releaseCalls==0&&reads==0&&stops==0,"early init failure entered Ethernet");
 std::cout<<"ABORT MODEL PASS: actual finish preserves owner faults, close failures and early-init stop; no hardware result\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
'''
with tempfile.TemporaryDirectory(prefix='test09-abort-model-') as directory:
    directory=Path(directory);cpp=directory/'abort.cpp';binary=directory/'abort'
    cpp.write_text(model.replace('ACTUAL_FINISH',function))
    subprocess.run(['g++','-std=c++11','-Wall','-Wextra','-Werror',str(cpp),'-o',str(binary)],check=True,timeout=30)
    subprocess.run([str(binary)],check=True,timeout=5)
