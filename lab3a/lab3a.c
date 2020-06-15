#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "libext2.h"

/* for handling debug code */
#ifdef DEBUG
#define DB(c) c
#else
#define DB(c)
#endif

/* ret codes */
#define SUCCESS     0
#define BADARGS     1
#define CORRUPT     2

/* ext2 constants */
#define SUPERBLOCK_START 1024

/* program globals */
int imgfd = -1;

/**
 * die
 * Kills the program with specified
 * exit code and prints msg to stderr.
 * Closes imgfd, if open.
 */
void
die(int code, const char *msg)
{
    if (imgfd < 0)
        close(imgfd);

    fprintf(stderr, "%s", msg);
    exit(code);
}

int
main(int argc, char **argv)
{
    /* check that exactly one argument is provided. */
    if (argc != 2)
        die(BADARGS, "lab3a requires a disk image.");

    /* confirm existence of the disk image */
    if ((imgfd = open(argv[1], O_RDONLY)) < 0)
        die(BADARGS, "Specified image does not exist!");

    /* load superblock */
    Superblock sb;
    ZEROOUT(sb);
    if (pread(imgfd, &sb, sizeof(Superblock), SUPERBLOCK_START) != sizeof(Superblock))
        die(CORRUPT, "Provided image is not large enough to accomodate the superblock!");

    /* verify superblock */
    if (sb.s_magic != EXT2_SUPER_MAGIC)
        die(CORRUPT, "File system is not of format EXT2!");

    /** SUPERBLOCK
     * total number of blocks (decimal)
     * total number of i-nodes (decimal)
     * block size (in bytes, decimal)
     * i-node size (in bytes, decimal)
     * blocks per group (decimal)
     * i-nodes per group (decimal)
     * first non-reserved i-node (decimal)
     */
    int blockSize = EXT2_MIN_BLOCK_SIZE << sb.s_log_block_size;
    fprintf(stdout, "SUPERBLOCK,%d,%d,%d,%d,%d,%d,%d\n", sb.s_blocks_count,
                                                         sb.s_inodes_count,
                                                         blockSize,
                                                         sb.s_inode_size,
                                                         sb.s_blocks_per_group,
                                                         sb.s_inodes_per_group,
                                                         sb.s_first_ino);

    /**
     * The block group descriptor table
     * starts on the first block following
     * the superblock.
     */
    int groupCount = sb.s_blocks_count / sb.s_blocks_per_group + 1;
    DB(fprintf(stderr, "Group count is %d\n", groupCount));
    GroupDesc gd;

    for (int k = 0; k < groupCount; k++)
    {
        ZEROOUT(gd);

        /* tries to acquire the first block of the first BGDT. */
        if (pread(imgfd, &gd, sizeof(GroupDesc), absAddr(&sb, k + 1)) != sizeof(GroupDesc))
            die(CORRUPT, "Could not locate block group descriptor table!");

        /** GROUP SUMMARY
         * group number (decimal, starting from zero)
         * total number of blocks in this group (decimal)
         * total number of inodes in this group (decimal)
         * number of free blocks (decimal)
         * number of free i-nodes (decimal)
         * block number of free block bitmap for this group (decimal)
         * block number of free i-node bitmap for this group (decimal)
         * block number of first block of i-nodes in this group (decimal)
         */
        fprintf(stdout, "GROUP,%d,%d,%d,%d,%d,%d,%d,%d\n", k,
                                                           sb.s_blocks_count,
                                                           sb.s_inodes_count,
                                                           gd.bg_free_blocks_count,
                                                           gd.bg_free_inodes_count,
                                                           gd.bg_block_bitmap,
                                                           gd.bg_inode_bitmap,
                                                           gd.bg_inode_table);

        /* summarize the free block bitmap of the group */
        byte blockBitmap[sb.s_blocks_per_group / BITSIZE(byte)];
        byte blockInodemap[sb.s_inodes_per_group / BITSIZE(byte)];


        DB(fprintf(stderr, "We will need %d bits to hold the bitmap, which is %ld bytes.\n", sb.s_blocks_per_group,
                                                                                            sb.s_blocks_per_group / BITSIZE(byte)));
                                                                                            
        if (pread(imgfd, blockBitmap, sizeof(blockBitmap), absAddr(&sb, gd.bg_block_bitmap - 1)) != sizeof(blockBitmap))
            die(CORRUPT, "Failed to read block bitmap!");
        
        if (pread(imgfd, blockInodemap, sizeof(blockInodemap), absAddr(&sb,gd.bg_inode_bitmap - 1)) != sizeof(blockInodemap))
            die(CORRUPT, "Failed to read block inodemap!");

        if (summarizeFreeBlockBitmap(blockBitmap, sb.s_blocks_per_group))
            die(CORRUPT, "Free block bitmap is corrupt!");

        if (summarizeFreeInodeBitmap(blockInodemap, sb.s_inodes_per_group))
            die(CORRUPT, "Free Inode bitmap is corrupt!");
        
        DB(fprintf(stderr, "Will be searching for inode table in block %d\n", gd.bg_inode_table));
        DB(fprintf(stderr, "That would be address %d\n", absAddr(&sb, gd.bg_inode_table - 1)));

        /* inode table summary */
        Inode inodeTable[sb.s_inodes_per_group];
        ZEROOUT(inodeTable);
       
       
        if (pread(imgfd, inodeTable, sizeof(inodeTable), absAddr(&sb, gd.bg_inode_table - 1)) != sizeof(inodeTable))
            die(CORRUPT, "Failed to read inode table!");


        DB(fprintf(stderr, "Inodes per group: %d\n", sb.s_inodes_per_group));
       
        if (summarizeInodes(imgfd, &sb, inodeTable, sb.s_inodes_per_group))
            die(CORRUPT, "Inode table is corrupt!");
    }

    return 0;
}
