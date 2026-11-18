#include "common.h"
#include <fstream>
#include <iterator>
#include <vector>
void test_yield(){}
#define THINGER_OTA_H
class ThingerOTA{
public:
 ThingerOTA(ThingerClient&){}
 void set_block_size(size_t){}
 bool is_compressed(){return true;}
 void update_firmware_checksum(uint8_t*,size_t){}
 virtual void fill_options(pson&)=0;
 virtual bool begin_ota(const char*,const char*,size_t,pson&,pson&)=0;
 virtual bool write_ota(uint8_t*,size_t,pson&)=0;
 virtual bool end_ota(pson&)=0;
 virtual size_t remaining()=0;
 virtual const char* supported_compression()=0;
};
#include "ThingerESP32OTA.h"
int main(int argc,char** argv){
 Client wire;ThingerClient client(wire,"u","d","c");ThingerESP32OTA ota(client);
 std::ifstream f(argv[1],std::ios::binary);
 std::vector<uint8_t> z{std::istreambuf_iterator<char>(f),std::istreambuf_iterator<char>()};
 size_t n=std::stoul(argv[2]),split=z.size()-4;
 pson opts,state;opts["compressed_size"]=(unsigned)z.size();
 assert(ota.begin_ota("test","1",n,opts,state));
 bool first=ota.write_ota(z.data(),split,state);size_t left=Update.remaining();
 if(n%32768==0) assert(!ota.end_ota(state));
 bool last=ota.write_ota(z.data()+split,4,state);
 bool contents=Update.bytes.size()==n;
 for(size_t i=0;i<Update.bytes.size();i++)contents=contents && Update.bytes[i]==(i%251);
 assert(first && last && contents && ota.end_ota(state));
 std::cout<<"size="<<n<<" data_chunk="<<first<<" remaining="<<left<<" trailer="<<last<<" contents="<<contents<<"\n";
}
