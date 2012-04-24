/*
 * para.h
 *
 * CopyRight (C) 2012 crazyleen <ruishenglin@126.com>
 */

#ifndef PARA_H_
#define PARA_H_

struct para_data {
	char *file;/* config file name */
	struct para_list *para_head;
};

struct para_list {
	char *key;
	char *value;
	char *comment;/* text after last key */
	struct para_list *next;
};

void para_print(struct para_data *conf);

/**
 * para_init - read config file value to list (malloc)
 * @return: 0 on success, -1 on error
 *
 * NOTE: this function must be called first to read config, remember to call para_free
 * 		at the end
 */
int para_init(struct para_data *conf);

/**
 * para_free - free list
 */
void para_free(struct para_data *conf);

/**
 * para_read - read key's value in para_list
 * @return: return pointer point to the key's value, or NULL
 *
 * NOTE: called para_init() before using this para_read(), after all, call para_free()
 */
char *para_read(struct para_data *conf, const char *key) ;

/**
 * para_update - write a new line "*key = *value" to conf list
 * @return: 0 on success, -1 on error
 */
int para_update(struct para_data *conf, const char *key, const char *value, const char *comment);

/**
 * para_write - write struct para_list to file, wite all para.
 * @return: 0 on success, -1 on error
 */
int para_write(struct para_data *conf);

#endif /* PARA_H_ */
