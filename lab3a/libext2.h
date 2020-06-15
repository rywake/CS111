#ifndef LIBEXT2_H
#define LIBEXT2_H

/* used for memset macros */
#include <string.h>

#include "ext2_fs.h"

/* Extended constants for inode modes */
#define EXT2_S_IFLNK	0xA000
#define EXT2_S_IFREG	0x8000
#define EXT2_S_IFDIR	0x4000

#define EXT2_FT_DIR 2

/* zero out the contents of a struct in memory */
#define ZEROOUT(s) (memset(&s, 0, sizeof(s)))

/* size in bits */
#define BITSIZE(s) (sizeof(s) << 3)

typedef struct ext2_super_block Superblock;
typedef struct ext2_group_desc GroupDesc;
typedef struct ext2_inode Inode;
typedef struct ext2_dir_entry DirEnt;
typedef char byte;

/**
 * absAddr
 * 
 * Given a Superblock and the desired block number
 * return the absolute address of the block.
 * 
 * STARTS COUNTING FROM THE SUPERBLOCK AS BLOCK ZERO.
 * THIS CONFLICTS WITH THE PREFERRED EXT2 COUNTING
 * SYSTEM, WHICH COUNTS THE SUPERBLOCK AS BLOCK
 * NUMBER ONE.
 */
int absAddr(Superblock *s, int blockN);

/**
 *
 * 
 * Scan the free block bitmap for each group.
 * For each free block, produce a new-line
 * terminated line, with two comma-separated
 * fields (with no white space).
 *  - BFREE
 *  - number of the free block (decimal)
 * 
 * Returns 0 on success, nonzero otherwise (corruption, etc).
 */
int summarizeFreeBlockBitmap(byte *bitmap, int nBlocks);

/**
 * 
 * 
 * Returns 0 on success, nonzero otherwise.
 */
int summarizeFreeInodeBitmap(byte * inodemap, int nInodes);

/**
 * 
 * 
 * Given the image file descriptor (assumed to
 * be open), and the Inode table of said image,
 * scan all inodes and conduct steps in accordance
 * with the subheadings "directory entries" and
 * "indirect block references" in the spec.
 * 
 */
int processDir(int imgfd, Superblock *s, Inode *dir);

/* Processes the indirect blocks that are found
 * when accesing the i_blocks in the inode table
 */
int processIndirect(int imgfd, Superblock *s, __u32 block_num, Inode *indir, int level, int inode_num, __u32 offset);

/**
 * 
 * 
 * Scan the I-nodes for each group. For each
 * allocated (non-zero mode and non-zero link
 * count) I-node, summarize the I-node as
 * specified in the spec under "I-node summary".
 * If the I-node is a directory, then proceed to
 * summarize each directory entry and its indirect
 * block references as specified in the subheadings
 * in the spec "directory entries" and "indirect
 * block references", respectively.
 * 
 * Returns 0 on success, nonzero otherwise.
 */
int summarizeInodes(int imgfd, Superblock *s, Inode *table, int n);

#endif