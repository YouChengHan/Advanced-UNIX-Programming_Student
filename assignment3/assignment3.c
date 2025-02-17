#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

typedef struct {
	char *buf;     // pointer points to the memory
	size_t size;   // size of the buffer
	size_t len;    // length of the data
	size_t off;    // offset of current data
} fmem_cookie;

static int fmem_read(void *cookie, char *buf, int size);
static int fmem_write(void *cookie, const char *buf, int size);
static fpos_t fmem_seek(void *cookie, fpos_t offset, int whence);
static int fmem_close(void *cookie);

FILE *fmem_fmemopen(void *buf, size_t size, const char *mode) {
	
	fmem_cookie *ck=malloc(sizeof(fmem_cookie));
	if(!ck) {
		perror("cookie malloc");
		return NULL;
	}
	memset(ck, 0, sizeof(fmem_cookie));

	ck->buf=buf;
	ck->buf=malloc(size);
	if(!ck->buf) {
		free(ck);
		perror("buffer malloc");
		return NULL;
	}
	ck->size=size;
	ck->len=0;
	ck->off=0;

	// only "w+" mode is implemented.
	// other modes should be handled here if needed.

	return funopen(ck, fmem_read, fmem_write, fmem_seek, fmem_close);
}

static int fmem_read(void *cookie, char *buf, int size) {

	fmem_cookie *ck=cookie;
	
	size=size>ck->len-ck->off?ck->len-ck->off:size;
	memcpy(buf, ck->buf+ck->off, size);
	ck->off+=size;

	return size;
}

static int fmem_write(void *cookie, const char *buf, int size) {

	fmem_cookie *ck=cookie;

	size=size>ck->size-ck->off?ck->size-ck->off:size;
	memcpy(ck->buf+ck->off, buf, size);
	ck->off+=size;
	if(ck->off>ck->len) {
		ck->len=ck->off;
		ck->buf[ck->off]='\0';
	}
	//ck->len=ck->off>ck->len?ck->off:ck->len;
	//ck->buf[ck->off]='\0';

	return size;
}

static fpos_t fmem_seek(void *cookie, fpos_t offset, int whence) {

	fmem_cookie *ck=cookie;

	switch(whence) {
		case SEEK_SET:
			if(offset>ck->size||offset<0) {
				errno=EINVAL;
				return -1;
			}
			ck->off=offset;
			break;
		case SEEK_CUR:
			if(ck->off+offset>ck->size||ck->off+offset<0) {
				errno=EINVAL;
				return -1;
			}
			ck->off+=offset;
			break;
		case SEEK_END:
			if(ck->len+offset<0||offset>0) {
				errno=EINVAL;
				return -1;
			}
			ck->off=ck->len+offset;
			break;
		default:
			errno=EINVAL;
			return -1;
	}

	return (fpos_t)ck->off;
}

static int fmem_close(void *cookie) {

	fmem_cookie *ck=cookie;

	free(ck->buf);
	free(ck);

	return 0;
}

int main() {

	char *fs, buf[256]="";
	FILE *fp=fmem_fmemopen(fs, 256, "w+");

	fprintf(fp, "hello, world");                 // write "hello, world" in the file stream
	fseek(fp, 7, SEEK_SET);                      // seek the position of "world"
	fread(buf, strlen("world"), 1, fp);          // read "world" from file stream
	printf("%s\n", buf);
	fseek(fp, 0, SEEK_SET);
	fread(buf, strlen("hello, world"), 1, fp);   // read whole sentence from file stream
	printf("%s\n", buf);

	fclose(fp);
	return 0;
}
