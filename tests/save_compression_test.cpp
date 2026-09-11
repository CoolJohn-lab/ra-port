#include <assert.h>
#include <string.h>
#include <vector>
#include "lzopipe.h"

class Capture : public Pipe {
public:
    std::vector<unsigned char> data;
    virtual int Put(void const *bytes, int count) {
        if (bytes && count>0) {
            unsigned char const *p=(unsigned char const *)bytes;
            data.insert(data.end(),p,p+count);
        }
        return count;
    }
};
int main()
{
    // Incompressible enough to exercise the whole pointer dictionary on LP64,
    // including complete blocks, accumulated chunks, and the final short block.
    std::vector<unsigned char> input(45000);
    unsigned int random=0x19faab12;
    for (size_t i=0;i<input.size();++i) {
        random=random*1664525u+1013904223u;
        input[i]=(unsigned char)(random>>24);
    }
    Capture encoded;
    LZOPipe compress(LZOPipe::COMPRESS);
    compress.Put_To(encoded);
    compress.Put(&input[0],301);
    compress.Put(&input[301],(int)input.size()-301);
    compress.Flush();
    Capture decoded;
    LZOPipe decompress(LZOPipe::DECOMPRESS);
    decompress.Put_To(decoded);
    decompress.Put(&encoded.data[0],(int)encoded.data.size());
    decompress.Flush();
    assert(decoded.data==input);
    return 0;
}
