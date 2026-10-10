"""Compile real TEST-09 command/SD functions against declared mocks, not hardware."""
from pathlib import Path
import subprocess
import tempfile

source = (Path(__file__).resolve().parents[1] / 'TEST-09_Ethernet_SD_Integration.ino').read_text()
def function(signature):
    start = source.index(signature)
    opening = source.index('{', start)
    depth = 1
    i = opening + 1
    while depth:
        if source[i] == '{': depth += 1
        elif source[i] == '}': depth -= 1
        i += 1
    return source[start:i]

model = r'''
#include <cstdint>
#include <cstring>
#include <cstdlib>
#include <algorithm>
#include <iostream>
#include <stdexcept>
#include <vector>
using std::uint8_t; using std::uint16_t; using std::uint32_t;
using __FlashStringHelper=char;
#define F(x) x
const int HIGH=1,ETH_CS=10,SD_CS=4,O_WRITE=2,O_READ=1;
const char FILE_NAME[]="T09CHECK.BIN";
uint8_t packet[128];char command[20]={},token[9]={};uint8_t commandLength=0;
bool overflow=false,active=false,used=false;
uint32_t durationMs=0,maxSdMs=0;uint16_t finalCrc=0;
int starts=0,finishes=0,rejections=0,samples=0;
bool cacheWasDiscarded=false,corrupt=false,shortWrite=false,syncFailure=false,closeFailure=false;
uint32_t millis(){static uint32_t tick=0;return ++tick;}
int digitalRead(int){return HIGH;}
void sampleRam(){++samples;}
void prefix(const char*){}
struct SerialMock {
 void print(const char*){} void println(unsigned long){}
 void println(const char* s){if(std::strcmp(s,"COMMAND_REJECTED")==0)++rejections;}
} Serial;
bool prepare(){++starts;active=true;return true;}
void finish(bool requested){if(!requested)throw std::runtime_error("unexpected failed finish");++finishes;active=false;}
struct FileMock {
 bool opened=false;std::vector<uint8_t> disk;bool writing=false;
 bool open(FileMock*,const char* name,int flags){
  if(std::strcmp(name,FILE_NAME)!=0)throw std::runtime_error("wrong filename");
  writing=flags==O_WRITE;opened=true;
  if(writing)cacheWasDiscarded=false;
  else if(!cacheWasDiscarded)throw std::runtime_error("read reused write cache");
  return true;
 }
 bool truncate(int size){if(!writing||size!=0)throw std::runtime_error("bad truncate");disk.clear();return true;}
 unsigned write(uint8_t* data,unsigned n){disk.assign(data,data+n);return shortWrite?n-1:n;}
 bool sync(){return !syncFailure;}
 unsigned fileSize(){return disk.size();}
 bool isOpen(){return opened;}
 bool close(){opened=false;return !closeFailure;}
 int read(uint8_t* data,unsigned n){std::copy(disk.begin(),disk.end(),data);if(corrupt&&n)data[0]^=1;return n;}
 int read(){return -1;}
} root,file;
struct SdVolume {static uint8_t* cacheClear(){cacheWasDiscarded=true;static uint8_t cache;return &cache;}};
ACTUAL_FUNCTIONS
void require(bool condition,const char* label){if(!condition)throw std::runtime_error(label);}
void cmd(const char* input){std::strncpy(command,input,sizeof(command));command[sizeof(command)-1]=0;commandLength=std::strlen(command);handleCommand();}
int main(){
 try{
  cmd("RUN 0123ABCD 30");require(starts==1&&active&&used&&durationMs==30000,"RUN command");
  cmd("STOP DEADBEEF");require(active&&finishes==0&&rejections==1,"wrong session STOP");
  cmd("STOP 0123ABCD");require(!active&&finishes==1,"matching STOP command");
  cmd("RUN 0123ABCD 30");require(starts==1,"second RUN accepted");
  used=false;cmd("RUN 0123ABCD 29");require(starts==1,"short duration accepted");
  cmd("RUN 0123ABCD 601");require(starts==1,"long duration accepted");
  cmd("RUN 0123ABCD xx");require(starts==1,"nonnumeric duration accepted");
  overflow=true;cmd("RUN 0123ABCD 30");require(starts==1,"overflow accepted");overflow=false;
  for(unsigned n: {16,32,64,128}){
   for(unsigned i=0;i<n;++i)packet[i]=uint8_t(i*73+17);
   auto expected=std::vector<uint8_t>(packet,packet+n);
   require(sdRoundtrip(n),"SD roundtrip failed");
   require(cacheWasDiscarded&&std::equal(expected.begin(),expected.end(),packet),"uncached readback");
  }
  corrupt=true;require(!sdRoundtrip(16),"corrupt read accepted");corrupt=false;
  shortWrite=true;require(!sdRoundtrip(16),"short write accepted");shortWrite=false;
  syncFailure=true;require(!sdRoundtrip(16),"sync failure accepted");syncFailure=false;
  closeFailure=true;require(!sdRoundtrip(16),"close failure accepted");closeFailure=false;
  std::cout<<"FIRMWARE MODEL PASS: RUN/STOP session and duration gates, cache discard, 4 sizes, corrupt/short/sync/close failures; no hardware claim\n";
 }catch(const std::exception& e){std::cerr<<e.what()<<"\n";return 1;}
}
'''
helpers='\n'.join(function(s) for s in ['uint16_t crc16(', 'bool sdRoundtrip(', 'void handleCommand('])
with tempfile.TemporaryDirectory(prefix='test09-model-') as directory:
    directory=Path(directory)
    cpp=directory/'model.cpp';binary=directory/'model'
    cpp.write_text(model.replace('ACTUAL_FUNCTIONS',helpers))
    subprocess.run(['g++','-std=c++11','-Wall','-Wextra','-Werror',str(cpp),'-o',str(binary)],check=True,timeout=30)
    subprocess.run([str(binary)],check=True,timeout=5)
