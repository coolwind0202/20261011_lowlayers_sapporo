#include <stdint.h>

enum FileType {
  FILETYPE_REGULAR,
  FILETYPE_DIRECTORY
};

#define NAME_BYTES 24

struct Mydirent {
  char name[NAME_BYTES];
  uint64_t idxInode;
};


struct Myinode {
  uint64_t idxBlock;
  enum FileType fileType;
  uint64_t size;
};

#define BLOCK_BYTES 512
#define DIRENTS_PER_BLOCK 16
union Block {
  char data[BLOCK_BYTES];   // 1ブロックは512バイト
  struct Mydirent dirents[DIRENTS_PER_BLOCK]; // sizeof(Mydirent) == 32 なので 16ブロック
};

#define N_INODES 200
#define N_INODE_BITMAP 25
#define N_BLOCKS 200

struct Storage {
  char inodeBitmap[N_INODE_BITMAP];
  struct Myinode inodes[N_INODES];
  union Block blocks[N_BLOCKS];
};

