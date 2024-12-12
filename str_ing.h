#pragma once
#define MAXNAME 32
int isname(const char* str);
char* str_cat(char* d, const char* s);
int str_cpy(char* d, const char* s);
int str_icpy(char* d, const char* s);
int str_ncpy(char* d, const char* s, int len);
int str_cmp(const char* s1, const char* s2);
int str_icmp(const char* s1, const char* s2);
int str_nicmp(const char* s1, const char* s2, int len);
int str_len(const char* src);