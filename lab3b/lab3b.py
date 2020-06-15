#!/usr/local/cs/bin/python3
#
# lab3b
#
# Parse a CSV dump of an EXT2 filesystem for any inconsistencies
# and output any that are found in human-readable form.
#
# Takes a path to a CSV file as argument.
#
# Return codes:
# * SUCCESS = 0
# * Bad parameters = 1
# * System call failure = 1
# * Inconsistencies found = 2
#

import os
import sys
import csv
import math


# CONSTANTS
SUCCESS = 0
BADPARAMS = 1
SYSCALLFAIL = 1
INCONSISTENCIES_FOUND = 2


# GLOBALS
consistent = True


"""die
kill the program with a message and the specified code
"""
def die(code: int, msg: str):
    if msg is not None:
        print(msg, file=sys.stderr)
    exit(code)


"""report
print the message to stdout and set consistent to False.
"""
def report(msg: str):
    global consistent
    consistent = False
    print(msg)


"""RefList
Holds a dictionary mapping keys to lists or sets of
reference information.
"""
class RefList:
    def __init__(self):
        self.blocks = dict()

    def __repr__(self):
        return f'{self.blocks}'

    def __iter__(self):
        return self.blocks.__iter__()

    def __getitem__(self, k):
        return self.blocks.get(k)

    # add
    # associate 'ref_info' to a given key.
    # if the key is not present already, it is
    # created. Treats references associated to
    # key as though they were a set.
    def add(self, key, ref_info):
        b = self.blocks.get(key)

        if b is None:
            b = { ref_info }
        else:
            b.add(ref_info)

        self.blocks.update({key: b})

    # append
    # associate 'ref_info' to a given key.
    # if the key is not present already, it is
    # created. Treats references associated
    # to given key as though they were a list.
    def append(self, key, ref_info):
        b = self.blocks.get(key)
        
        if b is None:
            b = [ ref_info ]
        else:
            b.append(ref_info)
        
        self.blocks.update({key: b})


"""Inode
Basic inode type. Holds number, file type, size, and
referenced blocks.
"""
class Inode:
    def __init__(self,
                 number = 0,
                 file_type = '?',
                 links = 0,
                 size = 0,
                 blocks = set()):
        self.number = number         # inode number
        self.file_type = file_type   # file type
        self.links = links           # link count
        self.size = size             # size of the file
        self.blocks = blocks         # blocks

    # simply hashes the object with its inode number.
    def __hash__(self):
        return self.number

    def __repr__(self):
        return f'Inode({self.number}, {self.file_type}, {self.links}, {self.size}, {self.blocks})'

#Class to track if there are any duplicate blocks in the list
class Duplicate:
    def __init__ (self, inode_num, block_num, indirect_level, offset):
        self.inode_num = inode_num
        self.block_num = block_num
        self.indirect_level = str(indirect_level)
        self.offset = offset

    def __hash__(self):
        return self.block_num
    
    def __repr__(self):
        return f'Duplicates({self.inode_num},{self.block_num},{self.indirect_level},{self.offset})'

"""Superblock
Superblock of a ext2 fs.
"""
class Superblock:
    def __init__(self,
                 total_blocks: int,
                 total_inodes: int,
                 block_size: int,
                 i_node_size: int,
                 blocks_per_group: int,
                 i_nodes_per_group: int,
                 first_nonreserved_inode: int):
        self.total_blocks = total_blocks
        self.total_inodes = total_inodes
        self.block_size = block_size
        self.inode_size = i_node_size
        self.blocks_per_group = blocks_per_group
        self.i_nodes_per_group = i_nodes_per_group
        self.first_nonreserved_inode = first_nonreserved_inode

    def __repr__(self):
        return f'Superblock({self.total_blocks},{self.total_inodes},{self.block_size},{self.inode_size},{self.blocks_per_group},{self.i_nodes_per_group},{self.first_nonreserved_inode})'


"""Indirect_Block
Holding information about an indirect block.
"""
class Indirect_Block:
    def __init__(self,
                 inode_num: int,
                 level_indirection: int,
                 logical_block_offset: int,
                 block_num: int,
                 reference_block_num: int):
        self.inode_num = inode_num
        self.level_indirection = level_indirection
        self.logical_block_offset = logical_block_offset
        self.block_num = block_num
        self.reference_block_num = reference_block_num


"""DirEnt
Stores basic information about a directory entry.
"""
class DirEnt:
    def __init__(self,
                 parent_inode_num,
                 logical_offset,
                 inode_num,
                 entry_len,
                 name_len,
                 entry_name):
        self.parent_inode_num = parent_inode_num
        self.logical_offset = logical_offset
        self.inode_num = inode_num
        self.entry_len = entry_len
        self.name_len = name_len
        self.entry_name = entry_name

    def __repr__(self):
        return f'({self.parent_inode_num},{self.logical_offset},{self.inode_num},{self.entry_name})'


"""block_valid
returns whether the block number is valid
in a filesystem with superblock sb.
"""
def block_valid(sb_total_blocks: int, block_num: int) -> bool:
    if block_num < 0 or block_num > sb_total_blocks:
        return False
    else:
        return True
    

"""block_reserved
returns whether the block number provided
is reserved in a filesystem with superblock
sb.
"""
def block_reserved(first_data_block: int, block_num: int) -> bool:
    if block_num < first_data_block and block_num > 0:
        return True
    else:
        return False


"""check_file
the main routine for the script. Expects that
file_path exists. Assumes complete control of
the script.
"""
def check_file(file_path: str):
    global consistent

    # sets of entry types
    superblock = None
    blocks = RefList()   # map of block number -> referencing inode numbers
    inodes = dict()      # dict of all inode information
    indirect_block_refs = set()
    duplicates = dict()

    # free lists
    free_blocks = set()
    free_inodes = set()

    # dict mapping inode number to names of directory entries
    inode_refs = RefList()  # map of inode number -> referencing directory entries.

    # dir_entries
    dir_entries = list()

    # group information
    group_exists = False
    first_data_block = 0
    blocks_in_group = 0
    inodes_in_group = 0
    first_inode_blocknum = 0

    # set up our data
    with open(file_path, "r", newline='') as file:
        reader = csv.reader(file)

        # hopefully this doesn't happen
        if reader is None:
            die(SYSCALLFAIL, "how did this happen")

        try:
            for r in reader:
                # handle lines that are too short or empty
                if len(r) < 2:
                    continue

                row_type = r[0]
                row_contents = r[1:]

                # get the superblock
                if row_type == 'SUPERBLOCK':
                    if len(row_contents) < 7:
                        die(BADPARAMS, "Poorly formatted file! Proper superblock information is required.")

                    superblock = Superblock(*[ int(i) for i in row_contents[:7] ])
                
                # get the group descriptor table
                if row_type == 'GROUP':
                    if len(row_contents) < 8:
                        die(BADPARAMS, "Poorly formatted file! Proper group descriptor table information is required.")

                    blocks_in_group = int(row_contents[1])
                    inodes_in_group = int(row_contents[2])
                    first_inode_blocknum = int(row_contents[7])
                    group_exists = True

                # catch bitmap entries
                if row_type == 'BFREE':
                    free_blocks.add(int(row_contents[0]))

                # catch inode free entries
                if row_type == 'IFREE':
                    free_inodes.add(int(row_contents[0]))

                # catch inodes
                if row_type == 'INODE':
                    new_node = Inode(int(row_contents[0]), row_contents[1], row_contents[5], int(row_contents[9]), [ int(i) for i in row_contents[11:] ])
                    for b in new_node.blocks:
                        blocks.add(b, hash(new_node))
                    inodes.update({new_node.number: new_node})

                # catch directory entries
                if row_type == 'DIRENT':
                    if len(row_contents) < 6:
                        die(BADPARAMS, "Poorly formatted file! Directory entry missing information.")

                    inode_number = int(row_contents[2])
                    entry_name = row_contents[5]
                    referenced_file_inode = int(row_contents[2])
                    inode_refs.append(inode_number, (entry_name, referenced_file_inode))
                    dir_entries.append(DirEnt(int(row_contents[0]), int(row_contents[1]), int(row_contents[2]), int(row_contents[3]), int(row_contents[4]), row_contents[5]))

                # catch indirect block entries
                if row_type == 'INDIRECT':
                    if len(row_contents) < 5:
                        die(BADPARAMS, "Poorly formatted file! Indirect entry missing information.")

                    indirect_block_refs.add(int(row_contents[4]))
                    if row_contents[1] == 1:
                        ind_level = 'INDIRECT BLOCK'
                    elif row_contents[1] == 2:
                        ind_level = 'DOUBLE INDIRECT BLOCK'
                    else:
                        ind_level = 'TRIPLE INDIRECT BLOCK'
                    ind_node = Duplicate(int(row_contents[0]),int(row_contents[4]), ind_level, int(row_contents[2]))
                    my_block = int(row_contents[4])
                    duplicates.setdefault(my_block,[]).append(ind_node)
        except:
            die(SYSCALLFAIL, "Failed to parse file properly! Are you sure it's CSV?")

    if superblock is None or not group_exists:
        die(SUCCESS, "Superblock and/or group entry are missing!")

    # Block number where the actual data blocks (not metadata) begin
    first_data_block = int(math.ceil(first_inode_blocknum + (superblock.inode_size*inodes_in_group)/superblock.block_size))
    

    #####################
    # BLOCK CONSISTENCY #
    #####################

    # if there exist inodes missing from both the free list and the inode
    # list, then report them as allocated.
    if len(inodes) != 0:
        for unreported_inode in filter(lambda i: i not in free_inodes and i not in inodes, range(max(inodes), inodes_in_group + 1)):
            report(f'UNALLOCATED INODE {unreported_inode} NOT ON FREELIST')

    # scan through all inodes to check for consistency
    for key in inodes:
        node = inodes[key]

        # the inode should not be on the free list if it has blocks
        if len(node.blocks) != 0 and node.number in free_inodes:
            report(f'ALLOCATED INODE {node.number} ON FREELIST')

        # if the inode has type 0 or is inode 0, then it should be on the free list
        if (node.file_type == 0 or node.number == 0) and node.number not in free_inodes:
            report(f'UNALLOCATED INODE {node.number} NOT ON FREELIST')

        # for each block it references
        for iteration, block in enumerate(node.blocks):
            # ignore if block number is 0
            if block == 0:
                continue
            
            # Block type, direct, indirect...
            if iteration < 12:
                block_type = 'BLOCK'
                offset = 0
            elif iteration == 12:
                block_type = 'INDIRECT BLOCK'
                offset = 12
            elif iteration == 13:
                block_type = 'DOUBLE INDIRECT BLOCK'
                offset = 256+12
            else:
                block_type = 'TRIPLE INDIRECT BLOCK'
                offset = 256*257+12

            duplicates.setdefault(block,[]).append(Duplicate(key, block, block_type, offset))

            # Check if block is Valid
            if not block_valid(superblock.total_blocks, block):
                report(f'INVALID {block_type} {block} IN INODE {key} AT OFFSET {offset}')

            # Check if block is Reserved
            if block_reserved(first_data_block, block):
                report(f'RESERVED {block_type} {block} IN INODE {key} AT OFFSET {offset}')

            # it should be off the free list
            if block in free_blocks:
                report(f'ALLOCATED BLOCK {block} ON FREELIST')

    # for each block in all blocks that *should* be unallocated:
    for block in set(range(first_data_block, blocks_in_group)).difference(blocks):
        # if unallocated, it should be on the free list.
        if block not in free_blocks and block not in indirect_block_refs:
            report(f'UNREFERENCED BLOCK {block}')

    # handle checking for duplicates
    for blocks in duplicates:
        if len(duplicates[blocks]) > 1:
            for values in duplicates[blocks]:
                report(f'DUPLICATE {values.indirect_level} {values.block_num} IN INODE {values.inode_num} AT OFFSET {values.offset}')

    #########################
    # DIRECTORY CONSISTENCY #
    #########################

    # for each directory
    for directory in filter(lambda i: inodes[i].file_type == 'd', inodes):
        # get the dirent for our current inode
        reference_entry = None
        for d in dir_entries:
            # TODO: kind of a sketchy workaround using entry_name.
            # would prefer something more concrete.
            if d.inode_num == directory and d.entry_name != "'.'" and d.entry_name != "'..'":
                reference_entry = d

        # for each directory entry for our current directory's members
        for member in [ m for m in dir_entries if m.parent_inode_num == directory ]:
            # if the current dir, it should be proper.
            if member.entry_name == "'.'":
                # check that it is aimed at us
                if member.inode_num != directory:
                    report(f'DIRECTORY INODE {directory} NAME {member.entry_name} LINK TO INODE {member.inode_num} SHOULD BE {directory}')

            # if it is the parent dir, it should be proper.
            elif member.entry_name == "'..'":
                # check that it is aimed at the parent.
                # if there is no reference entry, we should be at the root
                if reference_entry is None:
                    if member.inode_num != directory:
                        report(f'DIRECTORY INODE {directory} NAME {member.entry_name} LINK TO INODE {member.inode_num} SHOULD BE {directory}')
                elif member.inode_num != reference_entry.parent_inode_num:
                    report(f'DIRECTORY INODE {directory} NAME {member.entry_name} LINK TO INODE {member.inode_num} SHOULD BE {reference_entry.parent_inode_num}')

            else:
                # every entry should be valid.
                if member.inode_num < 1 or member.inode_num > inodes_in_group:
                    report(f'DIRECTORY INODE {directory} NAME {member.entry_name} INVALID INODE {member.inode_num}')

                # every entry should be allocated.
                if member.inode_num in free_inodes:
                    report(f'DIRECTORY INODE {directory} NAME {member.entry_name} UNALLOCATED INODE {member.inode_num}')

    ##############
    # LINK COUNT #
    ##############

    # for each inode:
    for inode_num in inodes:
        # get the inode:
        i = inodes[inode_num]

        # if unreferenced, its link count should be zero.
        if inode_num not in inode_refs and int(i.links) != 0:
            report(f'INODE {inode_num} HAS 0 LINKS BUT LINKCOUNT IS {i.links}')
        else:
            # otherwise, its reference count should match.
            if int(i.links) != len(inode_refs[inode_num]):
                report(f'INODE {inode_num} HAS {len(inode_refs[inode_num])} LINKS BUT LINKCOUNT IS {i.links}')

    # if we found any inconsistencies, then exit the program as such.
    if not consistent:
        die(INCONSISTENCIES_FOUND, None)
    die(SUCCESS, None)


# arg parsing and sanitization routine
if __name__ == "__main__":
    # check len of args.
    if len(sys.argv) != 2:
        die(BADPARAMS, "This script requires a file to run on!")

    # fail if the file doesn't exist.
    if not os.path.exists(sys.argv[1]):
        die(BADPARAMS, "File must exist!")

    check_file(sys.argv[1])
