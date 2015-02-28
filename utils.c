#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "utils.h"
#include "shalloc.h"

unsigned long get_pow(unsigned long value) {
	unsigned long pow=0;
	unsigned long tmp=1;
	while (tmp < value) {
		tmp *= 2;
		pow++;
	}

	return pow;
}

int list_empty(struct avail_head *head) {
	return (head->next == (struct block *) head) && (head->prev == (struct block *) head);
}

void dump_data(void *ptr, unsigned long size) {
	printf("ptr = %p, ", ptr);
	printf("size = %lu\n", size);
	unsigned long i=0;
	int j, k;
	while (i < size) {
		printf("%08x  ", (unsigned int) i);
		for (j=0; j<8 && i<size; j++) {
			for (k=0; k<2 && i<size; k++) {
				printf("%02x", ((char *)(ptr))[i++]&0xff);
			}
			printf(" ");
		}
		printf("\n");
	}
}

void print_block_info(struct block *ptr) {
	struct block *b = ptr;
	printf("pos = %p, ", ptr);
	printf("state = %u, k_size = %lu\n", b->state, b->k_size);
	printf("data =\n");
	LOG("CHECK pow=%u, bsize=%lu\n", pow2(b->k_size), B_SIZE);
	dump_data(B_DATA(ptr), B_DATA_SIZE(ptr));
	printf("\n");
}

char *read_line(int fd) {
	size_t buf_len = 16;
	size_t line_len = buf_len + 1;
	char BUF[buf_len];
	ssize_t readed;
	char *line;
	if ((line = (char *) shcalloc(1, line_len)) == NULL) {
		return NULL;
	}
//TODO promazat
	//int i=0;
	while ((readed = read(fd, &BUF, buf_len)) > 0) {
		//printf("OTOCKA %d: ", i++);
		//printf("line = %s\n", line);

		strncat(line, BUF, readed);
		char *pos;
		if ((pos = strchr(line, '\n')) == NULL) { 
			/* New-line not found, read more bytes */
			line_len += buf_len;
			if ((line = (char *) reshcalloc(line, line_len)) == NULL) {
				return NULL;
			}
			line[line_len - buf_len] = '\0';
		} else {
		//	printf("line pred upravou: >>%s<<\n", line);
			line_len = strlen(line);
			memcpy(pos, "\0", 1);
		//	printf("line po uprave: >>%s<<\n", line);
		//	printf("RETURN: line=>>%s<<, line_len=%lu, real_len=%lu\n\n", line, line_len, strlen(line));
			/* File descriptor must be returned back */
			if (lseek(fd, strlen(line) - line_len + 1, SEEK_CUR) == (off_t) -1) {
				perror("lseek");
				return NULL;
			}
			return line;
		}
	}
	/* End of file, line must be complete (possibly empty) */
	if (readed == 0 && line[0] != '\0') {
		return line;
	}
	/* Other type of error or nothing was readed */
	shee(line);
	return NULL;
}

