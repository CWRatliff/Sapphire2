#include "pch.h"
#include "str_ing.h"
//==========================================================
char* str_cat(char* dest, const char* src) {
	char* save = dest;

	for (; *dest; ++dest);
	while (*dest++ = *src++);
	return(save);
}
//==========================================================
int str_cpy(char* dest, const char* src) {
	int	cnt = 0;

	for (;; dest++, src++) {
		*dest = *src;
		cnt++;
		if (*src == '\0')
			break;
	}
	return cnt;
}
//==========================================================
int str_icpy(char* dest, const char* src) {
	int	cnt = 0;
	int sch;

	for (;; dest++, src++) {
		sch = *src;
		*dest = tolower(sch);
		cnt++;
		if (sch == '\0')
			break;
	}
	return cnt;
}
//=========================================================
int str_ncpy(char* dest, const char* src, int len) {
	int		cnt = 0;

	for (; len; dest++, src++) {
		*dest = *src;
		len--;
		cnt++;
		if (*src == '\0')
			break;
	}
	return cnt;
}
//=========================================================
int str_nicpy(char* dest, const char* src, int len) {
	int		cnt = 0;
	int		sch;

	for (; len; dest++, src++) {
		sch = *src;
		*dest = tolower(sch);
		len--;
		cnt++;
		if (sch == '\0')
			break;
	}
	return cnt;
}
//==========================================================
int str_cmp(const char* s1, const char* s2) {

	while (1) {
		char c1 = *s1++;
		char c2 = *s2++;
		int diff = c1 - c2;
		if (0 == diff) {
			if (c1 == '\0')
				break;
		}
		else
			return diff;
	}
	return 0;
}
//==========================================================
int str_icmp(const char* s1, const char* s2) {

	while (1) {
		char c1 = *s1++;
		char c2 = *s2++;
		int diff = tolower(c1) - tolower(c2);
		if (0 == diff) {
			if (c1 == '\0')
				break;
		}
		else
			return diff;
	}
	return 0;
}
//==========================================================
int str_nicmp(const char* s1, const char* s2, int len) {
	int diff;
	char	c1;
	char	c2;

	while (len) {
		c1 = *s1++;
		c2 = *s2++;
		diff = tolower(c1) - tolower(c2);
		if (diff != 0)
			return (diff);
		if (c1 == '\0' || c2 == '\0')
			break;
	}
	return 0;
}
//==========================================================
int str_len(const char* src) {
	int		cnt = 0;

	for (; *src; ++src)
		cnt++;
	return(cnt);
}
//=================================================================
// name validater for user, table, index and field names
// check name length
// 1st char alpha
// other chars alphanumeric
// good TRUE, bad FALSE
int isname(const char* str) {
	int	 len;
	const char* p;

	len = str_len(str);
	if (len <= 0 || len > MAXNAME)
		return FALSE;
	p = str;
	if (!isalpha(*p++))
		return FALSE;
	for (--len; len > 0; len--)
		if (!isalnum(*p++))
			return FALSE;
	return TRUE;
}