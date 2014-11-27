#!/usr/bin/python
import struct


class XTEA():
    
    def __init__(self):
        self.DELTA = 0x9E3779B9L
        self.MASK = 0xffffffffL

    def strdecode(self, key, msg):
        z = msg.decode('hex')
        return self.xtea_decrypt(key, z, 32)
        
    def xtea_encrypt(self, key,block,n=32):
        
        v0,v1 = struct.unpack("2L",block)
        k = struct.unpack("4L",key)
        sum = 0L
        for round in range(n):
            v0 = (v0 + (((v1<<4 ^ v1>>5) + v1) ^ (sum + k[sum & 3]))) & self.MASK
            sum = (sum + self.DELTA) & self.MASK
            v1 = (v1 + (((v0<<4 ^ v0>>5) + v0) ^ (sum + k[sum>>11 & 3]))) & self.MASK
        return struct.pack("2L",v0,v1)

    def xtea_decrypt(self, key,block,n=32):
       
        v0,v1 = struct.unpack("2L",block)
        k = struct.unpack("4L",key)
       
        sum = (self.DELTA * n) & self.MASK
        for round in range(n):
            v1 = (v1 - (((v0<<4 ^ v0>>5) + v0) ^ (sum + k[sum>>11 & 3]))) & self.MASK
            sum = (sum - self.DELTA) & self.MASK
            v0 = (v0 - (((v1<<4 ^ v1>>5) + v1) ^ (sum + k[sum & 3]))) & self.MASK
        return struct.pack("2L",v0,v1)




if __name__ == "__main__":
    x = XTEA()
    z = x.xtea_encrypt('0123456789012345','ABCDEFGH')
    z.encode('hex')
    
    z = 'ea0c3d7c1c22557f'.decode('hex')
    x = x.xtea_decrypt('0123456789012345',z)
    print x