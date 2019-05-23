/**
 * sha1-use.h
 * sha1的库引用于网络, 非本人所写
 * 该头文件使用了sha1库进行编码
 * 编码字符串为标准字符串
 */
#include <sha1.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

static bool SHA1_encode(const char *src, char *dest, unsigned int destlen)
{
    SHA1Context sha;
    uint8_t Message_Digest[SHA1HashSize];
	SHA1Reset(&sha);
	SHA1Input(&sha, (const uint8_t *)src, strlen(src));
	if (SHA1Result(&sha, Message_Digest))
    {
#ifdef DEBUG
		fprintf(stderr, "SHA1 ERROR: Could not compute message digest");
#endif //DEBUG
		return false;
	}
    else
    {
        char i;
        for(i = 0; i < SHA1HashSize; i++)
        {
            *dest = Message_Digest[i];
            dest++;
        }
        *dest = '\0';
    }
    return true;
}