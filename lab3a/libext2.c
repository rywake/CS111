#include "libext2.h"
#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <stdlib.h>

int
absAddr(Superblock *s, int blockN)
{
    return s == NULL ? 0 : (EXT2_MIN_BLOCK_SIZE + (EXT2_MIN_BLOCK_SIZE << s->s_log_block_size) * blockN);
}

int
summarizeFreeBlockBitmap(byte *bitmap, int nBlocks)
{
    for (int k = 0; k < nBlocks; k++)
        if (!((bitmap[k >> 3] >> (k % 8)) & 1))
            fprintf(stdout, "BFREE,%d\n", k + 1);   /* + 1 because we start counting blocks from 1 */

    return 0;
}

int 
summarizeFreeInodeBitmap(byte * inodemap, int nInodes)
{
    for (int k = 0; k < nInodes; k++)
        if (!((inodemap[k >> 3] >> (k % 8)) & 1))
            fprintf(stdout,"IFREE,%d\n", k + 1);
    return 0;
}

int
processDir(int imgfd, Superblock *s, Inode *dir)
{
    Inode tmp;
    DirEnt parentDir, d;

    /* get current directory inode */
    if (pread(imgfd, &parentDir, sizeof(parentDir), absAddr(s, dir->i_block[0] - 1)) != sizeof(parentDir))
        return -1;

    int currentBlock = 0;       /* the current block of the directory file */
    int internalOffset = 0;     /* the offset within the current block */

    /* the first 12 blocks are direct and are what we care about */
    while (currentBlock < dir->i_blocks && currentBlock < 12)
    {
        if (internalOffset >= EXT2_MIN_BLOCK_SIZE)
        {
            currentBlock++;
            internalOffset = 0;
        }

        /* get the inode in the directory */
        if (pread(imgfd, &d, sizeof(d), absAddr(s, dir->i_block[currentBlock] - 1) + internalOffset) != sizeof(d))
            return -1;

        /* advance offset */
        internalOffset += d.rec_len;

        /* catch invalid/corrupt DirEnt */
        
        if (d.name_len > 255)
            return -1;

        /* catch end of directory */
        if (d.inode == 0)
            break;

        /** DIRENT
         *  - parent inode number (decimal) ... the I-node number of the directory that contains this entry
         *  - logical byte offset (decimal) of this entry within the directory
         *  - inode number of the referenced file (decimal)
         *  - entry length (decimal)
         *  - name length (decimal)
         *  - name (string, surrounded by single-quotes). Don't worry about escaping, we promise there will
         *    be no single-quotes or commas in any of the file names. 
         */
        fprintf(stdout,
                "DIRENT,%d,%d,%d,%d,%d,'%.*s'\n",
                parentDir.inode,
                currentBlock * EXT2_MIN_BLOCK_SIZE + internalOffset - d.rec_len,
                d.inode,
                d.rec_len,
                d.name_len,
                d.name_len,
                d.name);
    }

    return 0;
}

int
processIndirect(int imgfd, Superblock *s, __u32 block_num, Inode *indir, int level, int inode_num, __u32 offset)
{
    /* Number of indices in the array. */
    __u32 block_size = EXT2_MIN_BLOCK_SIZE << s->s_log_block_size;
    __u32 num_indexes = block_size/sizeof(__u32);

    /* Original offset is the offset that each block starts with, will be bassed recursively in the offset argument */
    __u32 Indirect_block[num_indexes];
    __u32 original_offset = offset;

    if (pread(imgfd, Indirect_block, sizeof(Indirect_block), absAddr(s,block_num-1)) != sizeof(Indirect_block))
        return -1;
    
    for (int i = 0; i < num_indexes; i++)
    {
        if (Indirect_block[i] == 0)
            continue;

        fprintf(stdout, "INDIRECT,%d,%d,%d,%d,%d\n",
                                                        inode_num,
                                                        level,
                                                        offset + i,
                                                        block_num,
                                                        Indirect_block[i]);
    
        if (level > 1)
            processIndirect(imgfd,s, Indirect_block[i],indir,level-1, inode_num, original_offset);

    }
    return 0;
}

int
summarizeInodes(int imgfd, Superblock *s, Inode *table, int n)
{
    char type = '?';

    /* all timestruct stuff */
    time_t longtime;
    struct tm *t;

    int cm = 0;
    int cd = 0;
    int cy = 0;
    int chr = 0;
    int cmin = 0;
    int csec = 0;
    int mm = 0;
    int md = 0;
    int my = 0;
    int mhr = 0;
    int mmin = 0;
    int msec = 0;
    int am = 0;
    int ad = 0;
    int ay = 0;
    int ahr = 0;
    int amin = 0;
    int asec = 0;

    /* scan each inode in the table */
    for (int k = 0; k < n; k++)
    {
        /* skip invalid inodes */
        if (table[k].i_mode == 0 && table[k].i_links_count == 0)
            continue;

        /* acquire type */
        switch (table[k].i_mode & 0xF000)
        {
        case EXT2_S_IFREG:
            type = 'f';
            break;
        case EXT2_S_IFLNK:
            type = 's';
            break;
        case EXT2_S_IFDIR:
            type = 'd';
            break;
        default:
            type = '?';
        }
        
        /* change our ctime, time, atime to tm structs */
        longtime = table[k].i_ctime;
        t = gmtime(&longtime);
        cm = t->tm_mon + 1;
        cd = t->tm_mday;
        cy = (1900 + t->tm_year) % 100;
        chr = t->tm_hour;
        cmin = t->tm_min;
        csec = t->tm_sec;
        
        longtime = table[k].i_mtime;
        t = gmtime(&longtime);
        mm = t->tm_mon + 1;
        md = t->tm_mday;
        my = (1900 + t->tm_year) % 100;
        mhr = t->tm_hour;
        mmin = t->tm_min;
        msec = t->tm_sec;

        longtime = table[k].i_atime;
        t = gmtime(&longtime);
        am = t->tm_mon + 1;
        ad = t->tm_mday;
        ay = (1900 + t->tm_year) % 100;
        ahr = t->tm_hour;
        amin = t->tm_min;
        asec = t->tm_sec;

        /** INODE
         * inode number (decimal)
         * file type ('f' for file, 'd' for directory, 's' for symbolic link, '?" for anything else)
         * mode (low order 12-bits, octal ... suggested format "%o")
         * owner (decimal)
         * group (decimal)
         * link count (decimal)
         * time of last I-node change (mm/dd/yy hh:mm:ss, GMT)
         * modification time (mm/dd/yy hh:mm:ss, GMT)
         * time of last access (mm/dd/yy hh:mm:ss, GMT)
         * file size (decimal)
         * number of (512 byte) blocks of disk space (decimal) taken up by this file
         */
        fprintf(stdout,
                "INODE,%d,%c,%o,%d,%d,%d,%.2d/%.2d/%.2d %.2d:%.2d:%.2d,%.2d/%.2d/%.2d %.2d:%.2d:%.2d,%.2d/%.2d/%.2d %.2d:%.2d:%.2d,%d,%d",
                k + 1,
                type,
                table[k].i_mode & 0xFFF,
                table[k].i_uid,
                table[k].i_gid,
                table[k].i_links_count,
                
                cm,
                cd,
                cy,
                chr,
                cmin,
                csec,

                mm,
                md,
                my,
                mhr,
                mmin,
                msec,

                am,
                ad,
                ay,
                ahr,
                amin,
                asec,

                table[k].i_size,
                table[k].i_blocks);
        
        /*
         * For ordinary files (type 'f') and directories (type 'd') the next fifteen
         * fields are block addresses (decimal, 12 direct, one indirect, one double
         * indirect, one triple indirect).
         * Symbolic links may be a little more complicated. If the file length is
         * less than or equal to the size of the block pointers (60 bytes) the file
         * will contain zero data blocks, and the name is stored in the space
         * normally occupied by the block pointers. If this is the case, the fifteen
         * block pointers should not be printed. If, however, the file length is
         * greater than 60 bytes, print out the fifteen block nunmbes as for ordinary
         * files and directories.
         */
        if (type == 'f' || type == 'd' || (type == 's' && table[k].i_size > 60))
            fprintf(stdout,
                    ",%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d\n",
                    table[k].i_block[0],
                    table[k].i_block[1],
                    table[k].i_block[2],
                    table[k].i_block[3],
                    table[k].i_block[4],
                    table[k].i_block[5],
                    table[k].i_block[6],
                    table[k].i_block[7],
                    table[k].i_block[8],
                    table[k].i_block[9],
                    table[k].i_block[10],
                    table[k].i_block[11],
                    table[k].i_block[12],
                    table[k].i_block[13],
                    table[k].i_block[14]);
        else
            fprintf(stdout, "\n");

        /* if we've encountered a directory, then process its inodes */
        if (type == 'd' && processDir(imgfd, s, &table[k]))
            return -1;


        __u32 block_size = EXT2_MIN_BLOCK_SIZE << s->s_log_block_size;
        __u32 offset = block_size/sizeof(__u32);
        __u32 original_offset = 12;

        if (table[k].i_block[12] != 0)
            {
                if (processIndirect(imgfd, s, (table+k)->i_block[12], table + k, 1, k+1, original_offset))
                    return -1;
            }
         if (table[k].i_block[13] != 0)
            {
                 if (processIndirect(imgfd, s, (table+k)->i_block[13], table + k, 2, k+1, offset + original_offset))
                      return -1;
            }
          if (table[k].i_block[14] != 0)
            {
                if (processIndirect(imgfd, s, (table+k)->i_block[14], table + k, 3, k+1, offset*offset + offset + original_offset))
                    return -1;
            }
    }

    return 0;
}
