/*
 * slowtail.c
 *
 * Tail a file between program invocations by writing an offset file
 * that contains the file's inode and the last file offset read.
 *
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>

#define MAX_BUF       4096
#define MAX_FILE_NAME 1024
#define OFFSET_EXT    ".offset"

/*
 * Read the offset file in filename and set inode and offset.
 */
int read_offset(char *filename, ino_t *inode, off_t *offset)
{
	FILE *f;
	char buf[MAX_BUF], *p;
	struct stat fs;

	/* Zero everything first so we can bail on errors */
	*inode = 0;
	*offset = 0;

	/* If the offset file doesn't exist don't worry */
	if (stat(filename, &fs) < 0)
		return 0;

	if ((f = fopen(filename, "r")) == NULL)
		return errno;

	if (fgets(buf, MAX_BUF, f) == NULL)
		return errno;

	p = strchr(buf, ' ');
	*inode = (ino_t)strtol(buf, NULL, 10);
	*offset = (off_t)strtol(p+1, NULL, 10);

	fclose(f);

	return 0;
}

/*
 * Write an offset file to filename using given inode and offset.
 */
int write_offset(char *filename, ino_t inode, off_t offset)
{
	FILE *f;

	if ((f = fopen(filename, "w")) == NULL)
		return errno;

	fprintf(f, "%u %ld\n", (unsigned int)inode, (long int)offset);

	fclose(f);

	if (ferror(f) != 0)
		return errno;

	return 0;
}

/*
 * Fetch logs starting from new_offset and then set new_inode and
 * new_offset appropriately.
 */
int print_logs(char *filename, ino_t *new_inode, off_t *new_offset)
{
	FILE *f;
	struct stat fs;
	char buf[MAX_BUF];
	ino_t inode;
	off_t offset;

	inode = *new_inode;
	offset = *new_offset;

	if (stat(filename, &fs) != 0)
		return errno;

	if (inode != fs.st_ino) {
		/* The file was rotated */
		inode = fs.st_ino;
		offset = 0;
	}

	if (offset > fs.st_size) {
		/* The file was truncated */
		offset = 0;
	}

	if ((f = fopen(filename, "r")) == NULL)
		return errno;

	fseek(f, offset, SEEK_SET);
	while ((fgets(buf, MAX_BUF, f)) != NULL)
		printf("%s", buf);
	fflush(stdout);
	fclose(f);

	if (ferror(f) != 0)
		return errno;

	*new_inode = fs.st_ino;
	*new_offset = fs.st_size;

	return 0;
}

void usage(char *progname)
{
	fprintf(stderr, "usage: %s <filename>\n", progname);
}

int main(int argc, char **argv)
{
	char offset_file[MAX_FILE_NAME];
	char *file_name, *prog;
	ino_t inode;
	off_t offset;

	prog = argv[0];

	if (argc != 2) {
		fprintf(stderr, "no filename specified\n");
		usage(prog);
		return 1;
	}

	file_name = argv[1];

	if (strlen(OFFSET_EXT) + strlen(file_name) + 1 > MAX_FILE_NAME) {
		fprintf(stderr, "%s: filename too long (max is %ld)\n",
			prog, MAX_FILE_NAME - strlen(OFFSET_EXT) - 1);
		return 1;
	}
	strncat(offset_file, file_name,
		sizeof(offset_file) - strlen(offset_file) - 1);
	strncat(offset_file, OFFSET_EXT,
		sizeof(offset_file) - strlen(offset_file) - 1);

	if (read_offset(offset_file, &inode, &offset) != 0) {
		fprintf(stderr, "%s: can't open offset file: %s\n",
			prog, strerror(errno));
		return 1;
	}

	if (print_logs(file_name, &inode, &offset) != 0) {
		fprintf(stderr, "%s: can't read '%s': %s\n",
			prog, file_name, strerror(errno));
		return 1;
	}

	if (write_offset(offset_file, inode, offset) != 0) {
		fprintf(stderr, "%s: can't write offset file: %s\n",
			prog, strerror(errno));
		return 1;
	}

	return 0;
}
