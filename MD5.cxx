#include <algorithm>
#include <memory>

#include <iostream>

#include "MD5.hxx"

namespace rps {
namespace crypto {
namespace md5 {


// s specifies the per-round shift amounts
static const word s[64] = { 7, 12, 17, 22,  7, 12, 17, 22,  7, 12, 17, 22,  7, 12, 17, 22,
                            5,  9, 14, 20,  5,  9, 14, 20,  5,  9, 14, 20,  5,  9, 14, 20,
                            4, 11, 16, 23,  4, 11, 16, 23,  4, 11, 16, 23,  4, 11, 16, 23,
                            6, 10, 15, 21,  6, 10, 15, 21,  6, 10, 15, 21,  6, 10, 15, 21 };


// Use binary integer part of the sines of integers (Radians) as constants:
// for (int i = 0; i < 64; ++i)
//   K[i] = floor(abs(sin(i + 1)) * double(0xFFFFFFFF));
static const word K[64] = { 0xd76aa478, 0xe8c7b756, 0x242070db, 0xc1bdceee,
                            0xf57c0faf, 0x4787c62a, 0xa8304613, 0xfd469501,
                            0x698098d8, 0x8b44f7af, 0xffff5bb1, 0x895cd7be,
                            0x6b901122, 0xfd987193, 0xa679438e, 0x49b40821,
                            0xf61e2562, 0xc040b340, 0x265e5a51, 0xe9b6c7aa,
                            0xd62f105d, 0x02441453, 0xd8a1e681, 0xe7d3fbc8,
                            0x21e1cde6, 0xc33707d6, 0xf4d50d87, 0x455a14ed,
                            0xa9e3e905, 0xfcefa3f8, 0x676f02d9, 0x8d2a4c8a,
                            0xfffa3942, 0x8771f681, 0x6d9d6122, 0xfde5380c,
                            0xa4beea44, 0x4bdecfa9, 0xf6bb4b60, 0xbebfbc70,
                            0x289b7ec6, 0xeaa127fa, 0xd4ef3085, 0x04881d05,
                            0xd9d4d039, 0xe6db99e5, 0x1fa27cf8, 0xc4ac5665,
                            0xf4292244, 0x432aff97, 0xab9423a7, 0xfc93a039,
                            0x655b59c3, 0x8f0ccc92, 0xffeff47d, 0x85845dd1,
                            0x6fa87e4f, 0xfe2ce6e0, 0xa3014314, 0x4e0811a1,
                            0xf7537e82, 0xbd3af235, 0x2ad7d2bb, 0xeb86d391 };


// Auxiliary function for md5::generator
static inline word leftrotate(const word x, const word c)
{
  return (x << c) | (x >> (32 - c));
}


// MD5 generator class. An MD5 digest can be generated by calling @update() with subseqent part of input,
// then retrieving the MD5 digest by calling @result().
class generator
{
public:
  generator()
  {
    clear();
  }

  void update(const byte bytes[], std::size_t size);
  digest result();
  
private:
  void updateWithChunk(const std::array<byte, 64>& chunk);

  void clear()
  {
    a0 = 0x67452301;
    b0 = 0xefcdab89;
    c0 = 0x98badcfe;
    d0 = 0x10325476;

    messageSize = 0;
    lastChunkSize = 0;
    std::fill(&lastChunk[0], &lastChunk[64], 0);
  }

  std::size_t messageSize;
  word a0, b0, c0, d0;
  std::size_t lastChunkSize;
  std::array<byte, 64> lastChunk;
};

} // namespace md5
} // namespace crypto
} // namespace rps


// Updates the generator state with the given array of bytes.
void rps::crypto::md5::generator::update(const rps::crypto::md5::byte *bytes, std::size_t size)
{
  messageSize += size;

  // copy data into lastChunk and update digest with each full (64 byte) chunks
  std::size_t copyLength = 0;
  do
  {
    copyLength = std::min(64 - lastChunkSize, size);
    std::copy(&bytes[0], &bytes[copyLength], &lastChunk[lastChunkSize]);
    lastChunkSize += copyLength;
    bytes += copyLength;
    size -= copyLength;
    
    if (lastChunkSize == 64) {
      updateWithChunk(lastChunk);
      lastChunkSize = 0;
    }
  } while (size > 0);
}


// Updates the MD5 generator state with the given chunk.
void rps::crypto::md5::generator::updateWithChunk(const std::array<rps::crypto::md5::byte, 64>& chunk)
{
  // break chunk into sixteen 32-bit words M[j], 0 ? j ? 15
  const word *M = reinterpret_cast<const word *>(chunk.data());
  
  // Initialize hash value for this chunk:
  word A = a0;
  word B = b0;
  word C = c0;
  word D = d0;

  // Main loop:
  for (unsigned int i = 0; i < 64; ++i)
  {
    word F = 0;
    word g = 0;
  
    if (i < 16)
    {
        F = (B & C) | ((~B) & D);
        g = i;
    }
    else if (i < 32)
    {
        F = (D & B) | ((~D) & C);
        g = (5 * i + 1) % 16;
    }
    else if (i < 48)
    {
        F = B ^ C ^ D;
        g = (3 * i + 5) % 16;
    }
    else // if (i < 64)
    {
        F = C ^ (B | (~D));
        g = (7 * i) % 16;
    }
    word temp = D;
    D = C;
    C = B;
    B = B + leftrotate((A + F + K[i] + M[g]), s[i]);
    A = temp;
  }
  
  // Add this chunk's hash to result so far
  a0 += A;
  b0 += B;
  c0 += C;
  d0 += D;
}


// Finishes the MD5 calculation and returns the digest.
// After returning the MD5 digest, this function clears the generator state.
rps::crypto::md5::digest rps::crypto::md5::generator::result()
{
  // padding with zeros
  // append "0" bit until message length in bits ? 448 (mod 512)
  std::fill(&lastChunk[lastChunkSize], &lastChunk[64], 0);

  // append 1 bit after the end of the message
  // Notice: the input bytes are considered as bits strings,
  // where the first bit is the most significant bit of the byte.[47]

  lastChunk[lastChunkSize] = byte(0x1u << 7);
  
  // if message length (64 bit unsigned integer) does not fit into the remaining part of the chunk
  if (lastChunkSize >= 56)
  {
    // update with chunk and create new chunk filled with 0
    updateWithChunk(lastChunk);
    std::fill(&lastChunk[0], &lastChunk[64], 0);
  }

  // append original length in bits mod (2 pow 64) to message
  *reinterpret_cast<doubleword *>(&lastChunk[56]) = messageSize * 8;
  updateWithChunk(lastChunk);

  // digest[16] := a0 append b0 append c0 append d0 //(Output is in little-endian)
  digest result;
  reinterpret_cast<word *>(result.data())[0] = a0;
  reinterpret_cast<word *>(result.data())[1] = b0;
  reinterpret_cast<word *>(result.data())[2] = c0;
  reinterpret_cast<word *>(result.data())[3] = d0;

  // clear generator state
  clear();
  
  return result;
}


// Generates an MD5 digest for the given array of @bytes with a length of @size.
rps::crypto::md5::digest rps::crypto::md5::generate(const rps::crypto::md5::byte *bytes, std::size_t size)
{
  rps::crypto::md5::generator g;
  g.update(bytes, size);
  return g.result();
}
