#define FUSE_USE_VERSION 31

#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <fuse3/fuse.h>
#include "myext.h"

#define MIN(a, b) ((a) < (b) ? (a) : (b))

const union Block rootBlock = {
  .dirents[0] = {
    .name = "myfile",
    .idxInode = 1
  },
  .dirents[1] = {
    .name = "subdir",
    .idxInode = 2
  }
};

const union Block myBlock = {
  .data = "hello world!\n"
};

const union Block subdirBlock = {
  .dirents[0] = {
    .name = "subfile",
    .idxInode = 3
  }
};

const union Block subfileBlock = {
  .data = "This is subfile\n"
};

const struct Myinode rootInode = {
  .fileType = FILETYPE_DIRECTORY,
  .idxBlock = 0
};

const struct Myinode myInode = {
  .fileType = FILETYPE_REGULAR,
  .idxBlock = 1,
  .size = 13
};

const struct Myinode subdirInode = {
  .fileType = FILETYPE_DIRECTORY,
  .idxBlock = 2
};

const struct Myinode subfileInode = {
  .fileType = FILETYPE_REGULAR,
  .idxBlock = 3,
  .size = 16
};

struct Storage storage = { 
  .inodes = { rootInode, myInode, subdirInode, subfileInode },
  .blocks = { rootBlock, myBlock, subdirBlock, subfileBlock },
};

static void *myext_init(struct fuse_conn_info *conn,
			struct fuse_config *cfg)
{
	cfg->kernel_cache = 1;
	return NULL;
}

struct Myinode *search(struct Myinode *parent, char *segment) {
  if (segment == NULL) return parent;
  if (parent == NULL || parent->fileType != FILETYPE_DIRECTORY) return NULL;
  
  struct Mydirent *dirents = storage.blocks[parent->idxBlock].dirents;

  for (uint64_t i = 0; i < DIRENTS_PER_BLOCK; i++) {
    if (strncmp(segment, dirents[i].name, NAME_BYTES) == 0) {
      struct Myinode* found = &storage.inodes[dirents[i].idxInode];

      switch (found->fileType) {
        case FILETYPE_REGULAR:
          return found;
        case FILETYPE_DIRECTORY:
          return search(found, strtok(NULL, "/"));
        default:
          return NULL;
      }
    }
  }

  return NULL;
}

struct Myinode *lookup(const char *path) {
  char* buf = strndup(path, NAME_BYTES);

  char* segment = strtok(buf, "/");
  struct Myinode* ret = search(&storage.inodes[0], segment);

  free(buf);
  return ret;
}

static int myext_open(const char *path, struct fuse_file_info *fi)
{
  struct Myinode* inode = lookup(path);   
  if (inode == NULL) return -ENOENT;
    
	return 0;
}

static int myext_read(const char *path, char *buf, size_t size, off_t offset,
		      struct fuse_file_info *fi)
{
  struct Myinode* inode = lookup(path);
  if (inode == NULL) return -ENOENT;

  size_t begin = MIN(offset, inode->size);
  size_t end = MIN(offset + size, inode->size);

  memcpy(buf, storage.blocks[inode->idxBlock].data + begin, end - begin);
	return end - begin;
}

static int myext_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
			 off_t offset, struct fuse_file_info *fi,
			 enum fuse_readdir_flags flags)
{
  struct Myinode *dirInode = lookup(path);
  if (dirInode == NULL || dirInode->fileType != FILETYPE_DIRECTORY) return -ENOENT; 

  filler(buf, ".", NULL, 0, 0);
  filler(buf, "..", NULL, 0, 0);

  struct Mydirent *dirents = storage.blocks[dirInode->idxBlock].dirents;
  for (uint64_t i = 0; i < DIRENTS_PER_BLOCK; i++) {
    char *filename = dirents[i].name;

    if (filename == NULL || strlen(filename) == 0) continue;
    filler(buf, filename, NULL, 0, 0); 
  }

	return 0;
}

static int myext_getattr(const char *path, struct stat *stbuf,
			 struct fuse_file_info *fi)
{
	memset(stbuf, 0, sizeof(struct stat));
  
  struct Myinode *inode = lookup(path);
  if (inode == NULL) {
    return -ENOENT;
  }

  stbuf->st_mode = (inode->fileType == FILETYPE_DIRECTORY ? (S_IFDIR | 0755) : (S_IFREG | 0444));
  stbuf->st_size = inode->size;
  
  return 0;
}

static const struct fuse_operations myext_oper = {
	.init     = myext_init,
	.readdir  = myext_readdir,
	.open		  = myext_open,
	.read	    = myext_read,
  .getattr  = myext_getattr
};

int main(int argc, char* argv[]) {
	struct fuse_args args = FUSE_ARGS_INIT(argc, argv);

  int ret = fuse_main(args.argc, args.argv, &myext_oper, NULL);
  return ret;
}
