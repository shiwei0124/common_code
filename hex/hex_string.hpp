#include <string>

static std::string ToBinaryString(const unsigned char* buf,int len)
{
    int output_len = len*8;
    std::string output;
    const char* m[] = {"0","1"};
    
    for(int i = output_len - 1,j = 0; i >=0 ; --i,++j)
    {
        output.append(m[(buf[j/8] >> (i % 8)) & 0x01],1);
    }
    return output;
}

static std::string ToHexString(const unsigned char* buf,int len,std::string tok = "")
{
    std::string output;
    char temp[8];
    for (int i = 0; i < len; ++i)
    {
        sprintf(temp,"%02x",buf[i]);
        output.append(temp,2);
        output.append(tok);
    }
    
    return output;
}