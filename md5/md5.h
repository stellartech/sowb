
#ifndef MD5_H
#define MD5_H

#ifdef __cplusplus
extern "C" {
#endif

void md5(const char *s, char *md5str);
void md5_short(const char *s, char *md5str, int day);

#ifdef __cplusplus
}
#endif

#endif
