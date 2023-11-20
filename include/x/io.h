#ifndef _X_IO_H
#define _X_IO_H

#include <stddef.h>
#include <unistd.h>

struct loop;
struct bio;

typedef ssize_t (*__bio_read)(struct bio *, const char *, size_t);
typedef void (*__bio_close)(struct bio *);

struct bio *bio_alloc(struct loop *, int, int, int, __bio_read, __bio_close);
ssize_t bio_write(struct bio *, const char *, size_t);
ssize_t bio_flush(struct bio *);
void bio_free(struct bio *);

#endif  // _X_IO_H
