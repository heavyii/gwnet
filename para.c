/*
 * para.c
 *
 * CopyRight (C) 2012 crazyleen <ruishenglin@126.com>
 */

#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <ctype.h>
#include "para.h"

#define LINE_SIZE  1024

void para_print(struct para_data *conf) {
	struct para_list *p = conf->para_head;
	printf("=======para list =======\n");
	printf("file: %s\n", conf->file);
	while (p != NULL) {
		printf("%s = %s\n", p->key, p->value);
		p = p->next;
	}
	printf("=======para end ========\n");
}

/**
 * para_add - add a node to list (malloc)
 * @return: 0 on success, -1 on error
 */
static int para_add(struct para_list **para_head_pp, const char *key,
		const char *value, const char *comment) {
	struct para_list *head = *para_head_pp;
	struct para_list *p = NULL;
	struct para_list *add = (struct para_list *) malloc(
			sizeof(struct para_list));
	if (add == NULL) {
		perror("malloc");
		return -1;
	}
	bzero(add, sizeof(struct para_list));
	add->comment = NULL;
	add->next = NULL;

	add->key = (char *) malloc(strlen(key) + 1);
	if (add == NULL) {
		perror("malloc");
		free(add);
		return -1;
	}
	strcpy(add->key, key);

	add->value = (char *) malloc(strlen(value) + 1);
	if (add == NULL) {
		perror("malloc");
		free(add->key);
		free(add);
		return -1;
	}
	strcpy(add->value, value);

	//write new value
	if (comment != NULL) {
		add->comment = (char *) malloc(strlen(comment) + 1);
		if (add->comment == NULL) {
			perror("malloc");
		}
		strcpy(add->comment, comment);
	}

	//if no header
	if (*para_head_pp == NULL) {
		*para_head_pp = add;
	} else {
		//point to the end
		p = head;
		while (p->next != NULL) {
			p = p->next;
		}
		p->next = add;
	}

	return 0;
}

/**
 * para_free - free list
 */
void para_free(struct para_data *conf) {
	struct para_list *p = conf->para_head;
	struct para_list *next = NULL;
	while (p != NULL) {
		next = p->next;
		if (p->key != NULL) {
			free(p->key);
			p->key = NULL;
		}
		if (p->value != NULL) {
			free(p->value);
			p->value = NULL;
		}
		if (p->comment != NULL) {
			free(p->comment);
			p->comment = NULL;
		}
		free(p);

		p = next;
	}

	conf->para_head = NULL;
}

/**
 * para_write - write struct para_list to file, wite all para.
 * @return: 0 on success, -1 on error
 */
int para_write(struct para_data *conf) {
	int ret = 0;
	int i = 0;
	char *mark = NULL;
	char *tempfile = NULL;
	struct para_list *q = NULL;
	char line[LINE_SIZE];
	char line_last[LINE_SIZE];
	FILE *fp = NULL;
	FILE *tmpfp = NULL;

	//check para
	if (conf == NULL || conf->file == NULL || conf->para_head == NULL) {
		printf("%s NULL para\n", __FUNCTION__);
		return -1;
	}

	//get tempfile name
	tempfile = (char *) malloc(strlen(conf->file) + 5);
	if (tempfile == NULL) {
		perror("malloc");
		return -1;
	}
	strcpy(tempfile, conf->file);
	strcat(tempfile, ".tmp");

	// amount of keys
	i = 0;
	for (q = conf->para_head; q != NULL; q = q->next) {
		i++;
	}

	//mark for written
	mark = (char *) malloc(i + 1);
	if (mark == NULL) {
		perror("malloc");
		return -1;
	}
	bzero(mark, i + 1);

	tmpfp = fopen(tempfile, "w");
	if (NULL == tmpfp) {
		printf("can't open '%s' as config file: %s\n", tempfile,
				strerror(errno));
		free(mark);
		mark = NULL;
		return -1;
	}

	fp = fopen(conf->file, "r");
	if (NULL == fp) {
		printf("can't open '%s' as config file: %s\n", conf->file,
				strerror(errno));
		goto no_config_file;
	}
	while (NULL != fgets(line, sizeof(line), fp)) {
		char *p = NULL;
		int keylen = 0;
		int valuelen = 0;

		char keyname[LINE_SIZE] = "";
		char keyvalue[LINE_SIZE] = "";

		sscanf(line, "%s = %s", keyname, keyvalue);
		if (strlen(keyname) == 0 || strlen(keyvalue) == 0) {
			//may be a comment line
			p = strchr(line, '#');
			if (p != NULL) {
				if(strstr(line_last, p) == NULL){
					fprintf(tmpfp, "%s", p);
				}
			}

			//save old line
			strcpy(line_last, line);
			continue;
		}

		//save key value
		for (i = 0, q = conf->para_head; q != NULL; q = q->next, i++) {
			if (strcasecmp(q->key, keyname) == 0) {
				if (q->comment != NULL) {
					if(strstr(line_last, q->comment) == NULL){
						fprintf(tmpfp, "#%s\n", q->comment);
					}
				}
				fprintf(tmpfp, "%s = %s\n\n", q->key, q->value);

				//mark it
				mark[i] = 'W';
				break;
			}//if (strcasecmp(q->key, keyname) == 0)
		} //for

		//save old line
		strcpy(line_last, line);
	} //while

	no_config_file:
	//save key value
	for (i = 0, q = conf->para_head; q != NULL; q = q->next, i++) {
		//if not mark, write it to file
		if (mark[i] != 'W') {
			if (q->comment != NULL) {
				fprintf(tmpfp, "#%s\n", q->comment);
			}
			fprintf(tmpfp, "%s = %s\n\n", q->key, q->value);

			mark[i] = 'W';
		}
	}

	//free
	if (mark != NULL) {
		free(mark);
		mark = NULL;
	}
	fclose(tmpfp);
	if(fp != NULL){
		fclose(fp);
	}

	//replace config file
	remove(conf->file);
	rename(tempfile, conf->file);
	return ret > 0 ? 0 : -1;
}

/**
 * para_update - write a new line "*key = *value" to conf list
 * @return: 0 on success, -1 on error
 */
int para_update(struct para_data *conf, const char *key, const char *value,
		const char *comment) {
	int ret;
	char *vp = NULL;
	struct para_list *p = conf->para_head;

	while (p != NULL) {
		//find key
		if (strcasecmp(p->key, key) == 0) {
			//free old value
			if (p->value != NULL) {
				free(p->value);
				p->value = NULL;
			}

			//write new value
			p->value = (char *) malloc(strlen(value) + 1);
			if (p->value == NULL) {
				perror("malloc");
				return -1;
			}
			strcpy(p->value, value);

			if (comment != NULL) {
				if (p->comment != NULL) {
					free(p->comment);
					p->comment = NULL;
				}

				p->comment = (char *) malloc(strlen(comment) + 1);
				if (p->comment == NULL) {
					perror("malloc");
				} else {
					strcpy(p->comment, comment);
				}
			}
			return 0;
		}
		p = p->next;
	}

	//no this key in list
	para_add(&conf->para_head, key, value, comment);
	return ret;
}

/**
 * para_read - read key's value in para_list
 * @return: return pointer point to the key's value, or NULL
 *
 * NOTE: called para_init() before using this para_read(), after all, call para_free()
 */
char *para_read(struct para_data *conf, const char *key) {
	struct para_list *p = conf->para_head;

	while (p != NULL) {
		//find key
		if (strcasecmp(p->key, key) == 0) {
			return p->value;
		}

		p = p->next;
	}

	return NULL;
}

/**
 * para_init - read config file value to list (malloc)
 * @return: 0 on success, -1 on error
 *
 * NOTE: this function must be called first to read config, remember to call para_free
 * 		at the end
 */
int para_init(struct para_data *conf) {
	char line[LINE_SIZE];
	FILE *fp = NULL;

	if (conf == NULL || conf->file == NULL) {
		printf("%s: para null", __FUNCTION__);
		return -1;
	}

	//no para data
	conf->para_head = NULL;

	fp = fopen(conf->file, "r");
	if (NULL == fp) {
		printf("can't open '%s': %s\n", conf->file, strerror(errno));
		return -1;
	}

	//fine key
	while (NULL != fgets(line, sizeof(line), fp)) {
		char *p = NULL;
		int keylen = 0;
		int valuelen = 0;

		char keyname[LINE_SIZE] = "";
		char keyvalue[LINE_SIZE] = "";

		sscanf(line, "%s = %s", keyname, keyvalue);
		if (strlen(keyname) == 0 || strlen(keyvalue) == 0) {
			continue;
		}

		//add one para
		para_add(&conf->para_head, keyname, keyvalue, NULL);
	}

	fclose(fp);
	return 0;
}

#if 0
/**
 * demo main - show you how to use this module
 */
int main(int argc, char **argv) {
	struct para_data conf;
	bzero(&conf, sizeof conf);
	conf.file = "config.conf";

	para_init(&conf);
	para_update(&conf, "ifname", "eth0", "net card name");
	para_update(&conf, "username", "crazyleen", NULL);
	para_update(&conf, "password", "123456", NULL);
	para_update(&conf, "e-mail", "ruishenglin@126.com",
			"bugs report to this email");
	para_write(&conf);

	para_print(&conf);

	printf("author's email is: %s\n", para_read(&conf, "e-mail"));
	para_free(&conf);
}
#endif
