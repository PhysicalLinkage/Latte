#include <sys/time.h>
#include <openssl/cmac.h>
#include <openssl/hmac.h>

#define LOOPMAX 100000

static unsigned char ckey[] = 
{
    0x2b,0x7e,0x15,0x16, 0x28,0xae,0xd2,0xa6,
    0xab,0xf7,0x15,0x88, 0x09,0xcf,0x4f,0x3c,
    0x2b,0x7e,0x15,0x16, 0x28,0xae,0xd2,0xa6,
    0xab,0xf7,0x15,0x88, 0x09,0xcf,0x4f,0x3c,
};

static unsigned char hkey[] = {0x2b,0x7e,0x15,0x16, 0x28,0xae,0xd2,0xa6,
                               0xab,0xf7,0x15,0x88, 0x09,0xcf,0x4f,0x3c,
                               0x00,0x00,0x00,0x00};

static unsigned char data[] = {0x6b,0xc1,0xbe,0xe2, 0x2e,0x40,0x9f,0x96,
                               0xe9,0x3d,0x7e,0x11, 0x73,0x93,0x17,0x2a};

static size_t ckeylen = sizeof(ckey);
static size_t hkeylen = sizeof(hkey);
static size_t datalen = sizeof(data);

void print(unsigned char* out, unsigned int len)
{
    for (int i = 0; i < len; ++i)
    {
        printf("%x", out[i]);
    }
    printf("\n");
}

void calc_cmac() {
  int           i;
  CMAC_CTX      *ctx = CMAC_CTX_new();
  size_t        len;
  unsigned char out[16 + 1] = {0};
  struct timeval s,e;
  double et;
  bool ok = true;

  gettimeofday(&s, NULL);
  for ( i = 0 ; i < LOOPMAX ; i++){
    if (CMAC_Init(ctx, ckey, ckeylen, EVP_aes_256_cbc(), NULL)
    &&  CMAC_Update(ctx, data, datalen)
    &&  CMAC_Final(ctx, out, &len))
    {

    }

  }
  gettimeofday(&e, NULL);

  et = (e.tv_sec - s.tv_sec) + (e.tv_usec - s.tv_usec)*1.0E-6;
  printf("elapsed time for %d times [CMAC] calc = %lf\n", LOOPMAX, et );
  printf("%lu\n", len);
  //print(out, len);
  printf("cmac size = %lu\n", len);
  
  CMAC_CTX_cleanup(ctx); /* Remove key from memory */
}

void calc_hmac() {
  int           i;
  HMAC_CTX      ctx;
  unsigned int  len;
  unsigned char out[20 + 1] = {0};
  struct timeval s,e;
  double et;

  gettimeofday(&s, NULL);
  for (i=0;i < LOOPMAX; ++i) {
    HMAC_Init(&ctx, hkey, hkeylen, EVP_sha1(  ));
    HMAC_Update(&ctx, data, datalen);
    HMAC_Final(&ctx, out, &len);
  }
  gettimeofday(&e, NULL);

  et = (e.tv_sec - s.tv_sec) + (e.tv_usec - s.tv_usec)*1.0E-6;
  printf("elapsed time for %d times [HMAC] calc = %lf\n",LOOPMAX,et);
  print(out, len);
  printf("hmac size = %lu\n", len);

  HMAC_cleanup(&ctx); /* Remove key from memory */
}

#include <openssl/md5.h>

int CMAC_TEST_TEST (){
  calc_cmac();
  uint8_t *message = (unsigned char*)"Sample Message";
  unsigned char digest[MD5_DIGEST_LENGTH];

	MD5_CTX sha_ctx;
	MD5_Init(&sha_ctx); // コンテキストを初期化
	MD5_Update(&sha_ctx, message, sizeof(message)); // message を入力にする
	MD5_Final(digest, &sha_ctx);

    printf("md5\n\n");
    print(digest, MD5_DIGEST_LENGTH);
  return 0;
}

