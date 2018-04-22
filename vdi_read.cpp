#include "vdi_read.h"

using namespace std;

/**
 * Mimcs the UNIX open system call 
 * Opens the vdifile
 */
int vdiOpen(VDIFile* vdi, char* fn)
{
	vdi->file = open(fn, O_RDWR);
	vdi->cursor = 0;
	if (vdi->file < 0) return 1;
	return vdi->file;
}

/**
 * Mimics the UNIX close system call
 * Closes the vdifile
 */
void vdiClose(VDIFile* f)
{
	close(f->file);
}

/**
 * Mimics the UNIX lseek system call
 * Seeks to the given location inside the vdifile
 */
off_t vdiSeek(VDIFile* f, off_t offset, int whence)
{
	off_t location;
	if (whence == SEEK_SET)
	{
		location = lseek(f->file, offset, whence);
		if (location < 0)
		{
			cout << "Error seeking the vdi header" << endl;
			return 1;
		}
		f->cursor = location;
	}
	if (whence == SEEK_CUR)
	{
		location = lseek(f->file, offset, whence);
		if (location < 0)
		{
			cout << "Error seeking the vdi header" << endl;
			return 1;
		}
		f->cursor += offset;
	}
	if (whence == SEEK_END)
	{
		location = lseek(f->file, offset, whence);
		if (location < 0)
		{
			cout << "Error seeking the vdi header" << endl;
			return 1;
		}
		f->cursor = offset + f->fileSize;
	}
	return f->cursor;
}

/**
 * Mimics the UNIX read system call.
 * Reads a vdifile after the cursor has been placed.
 */
ssize_t vdiRead(VDIFile* f, void* buf, ssize_t n)
{
	ssize_t numBytes = read(f->file, buf, n);
	if (numBytes != n)
	{
		cout << "Error header not read correctly" << endl;
		return 1;
	}
	return 0;
}

/**
 * Reads the vdiMap. 
 * Performs lseek and read in the same function. 
 */
int readVdiMap(VDIFile* f, unsigned int vdiMap[])
{
	off_t offset = lseek(f->file, f->header.offsetBlocks, SEEK_SET);
	if (offset < 0)
	{
		cout << "Error! failed to seek vdiMap" << endl;
		return 1;
	}
	int numMap = read(f->file, vdiMap, 4 * (f->header.blocksInHdd));
	if (numMap < 0)
	{
		cout << "Error! failed to read vdiMap" << endl;
		return 1;
	}
	return 0;
}


/**
 * Reads the MBR.
 */
int readMbr(VDIFile* f, BootSector& boot)
{
	off_t offset = lseek(f->file, f->header.offsetData, SEEK_SET);
	if (offset < 0)
	{
		cout << "Error! failed to seek to MBR" << endl;
		return 1;
	}
	int numMbr = read(f->file, &boot, sizeof(boot));
	if (numMbr != sizeof(boot))
	{
		cout << "Error! failed to read MBR" << endl;
		return 1;
	}
	return 0;
}

/**
 * Reads the superblock
 */
int readSuperblock(VDIFile* f, BootSector& boot, unsigned int vdiMap[], ext2_super_block& super)
{
	unsigned int superblockLocation = translate(1024, f, boot, vdiMap);
	if (lseek(f->file, superblockLocation, SEEK_SET) < 0)
	{
		cout << "Error! failed to seek to the superblock!" << endl;
		return 1;
	}
	if (read(f->file, &super, sizeof(super)) != 1024)
	{
		cout << "Error! failed to read superblock correctly!" << endl;
		return 1;
	}
	return 0;
}

/**
 * Helper function to translate the super block location. 
 */
unsigned int translate(unsigned int location, VDIFile* f, BootSector bootSector, unsigned int vdiMap[])
{
	unsigned int part1 = f->header.offsetData;
	unsigned int part2 = bootSector.partitionTable[0].sector_1 * 512 + location;
	unsigned int offset = part2 % f->header.blockSize;
	unsigned int block = part2 / f->header.blockSize;
	unsigned int translation = vdiMap[block];
	unsigned int value = part1 + translation * f->header.blockSize + offset;
	return value;
}

/**
 * Reads the group descriptor.
 */
int readGroupDescriptor(VDIFile* f, BootSector bootSector, unsigned int vdiMap[], unsigned int blockSize,
                        ext2_group_descriptor groupDescriptor[], unsigned int blockGroupCount)
{
	unsigned int start = 0;
	if (blockSize == 1024) start = 2 * blockSize;
	else start = 1 * blockSize;
	unsigned int location = translate(start, f, bootSector, vdiMap);
	if (lseek(f->file, location, SEEK_SET) < 0)
	{
		cout << "Error! Cannot seek to group descriptor table!" << endl;
		return 1;
	}
	if (read(f->file, groupDescriptor, sizeof(ext2_group_descriptor) * blockGroupCount) != sizeof(ext2_group_descriptor) *
		blockGroupCount)
	{
		cout << "Error! Cannot read group descriptor!" << endl;
		return 1;
	}
	return 0;
}

/**
 * Reads the bitmap.
 */
unsigned char* readBitmap(unsigned int blockSize, unsigned int blockId, VDIFile* f, BootSector bootSector,
                          unsigned int vdiMap[])
{
	unsigned char* bitmap;
	bitmap = (unsigned char *)malloc(blockSize);
	lseek(f->file, translate(blockId * blockSize, f, bootSector, vdiMap), SEEK_SET);
	read(f->file, bitmap, blockSize);
	return bitmap;
}

/**
 * Reads the inode.
 */
ext2_inode readInode(VDIFile* f, BootSector bootSector, unsigned int vdiMap[], unsigned int inodeCount,
                     unsigned int blockSize, ext2_super_block superBlock, ext2_group_descriptor groupDescriptor[])
{
	ext2_inode inode;
	inodeCount--;
	unsigned int groupCount = inodeCount / superBlock.s_inodes_per_group;
	unsigned int offset1 = inodeCount % superBlock.s_inodes_per_group;
	unsigned int inodesPerBlock = blockSize / sizeof(ext2_inode);
	unsigned int blockNum = groupDescriptor[groupCount].inode_table + (offset1 / inodesPerBlock);
	unsigned int offset2 = inodeCount % inodesPerBlock;
	lseek(f->file, translate((blockNum * blockSize) + offset2 * (sizeof(ext2_inode)), f, bootSector, vdiMap), SEEK_SET);
	read(f->file, &inode, sizeof(ext2_inode));
	return inode;
}

/**
 * Fetches the directory entry.
 */
bool getDirEntry(ext2_dir_entry_2& found, unsigned char* dataBlock, unsigned int sizeDiff, string fname, bool display)
{
	ext2_dir_entry_2 * entry = (ext2_dir_entry_2 *)malloc(sizeof(ext2_dir_entry_2));
	memcpy(entry, dataBlock, sizeof(*entry));
	unsigned int size = 0;
	while (size < sizeDiff)
	{
		char f_name[256];
		memcpy(f_name, entry->name, entry->name_len);
		f_name[entry->name_len] = '\0';
		if (entry->inode != 0)
		{
			if (display) cout << f_name << endl;
			else if ((string)f_name == fname)
			{
				found = *entry;
				free(entry);
				return true;
			}
		}
		else
		{
			size += sizeof(*entry);
			memcpy(entry, dataBlock + size, sizeof(*entry));
			continue;
		}
		size += entry->rec_len;
		memcpy(entry, dataBlock + size, sizeof(*entry));
	}
	free(entry);
	return false;
}

/**
 * Reads a block given its block number, block size, inode.
 */
int readBlock(ext2_inode inode, unsigned int blockNum, unsigned int blockSize, VDIFile* f, BootSector bootSector,
              unsigned int vdiMap[], unsigned char* buf)
{
	if (blockNum * blockSize >= inode.size) return -1;
	int direct, index_1, index_2, index_3;
	computeIndex(blockNum, blockSize, direct, index_1, index_2, index_3);
	unsigned int direct_block_num = 0;
	unsigned int blockNum1 = 0;
	unsigned int blockNum2 = 0;
	unsigned int blockNum3 = inode.i_block[14];
	bool hole = false;
	if (index_3 != -1)
	{
		if (blockNum3 == 0) hole = true;
		else
		{
			if (index_1 != -1 && index_2 != -1 && direct == -1)
			{
				if (lseek(f->file, translate(blockNum3 * blockSize, f, bootSector, vdiMap), SEEK_SET) < 0) return -1;
				if (read(f->file, buf, blockSize) < 0) return -1;
				if (index_3 >= (blockSize / 4)) return -1;
				blockNum2 = *(((unsigned int *)buf) + index_3);
			}
			else return -1;
		}
	}
	if (!hole && index_2 != -1)
	{
		if (index_1 != -1 && direct == -1)
		{
			if (index_3 == -1) blockNum2 = inode.i_block[13];

			if (blockNum2 == 0) hole = true;

			else
			{
				if (lseek(f->file, translate(blockNum2 * blockSize, f, bootSector, vdiMap), SEEK_SET) < 0) return -1;
				if (read(f->file, buf, blockSize) < 0) return -1;
				if (index_2 >= (blockSize / 4)) return -1;
				blockNum1 = *(((unsigned int *)buf) + index_2);
			}
		}
		else return -1;
	}
	if (!hole && index_1 != -1)
	{
		if (direct == -1)
		{
			if (index_2 == -1) blockNum1 = inode.i_block[12];
			if (blockNum1 == 0) hole = true;
			else
			{
				if (lseek(f->file, translate(blockNum1 * blockSize, f, bootSector, vdiMap), SEEK_SET) < 0) return -1;
				if (read(f->file, buf, blockSize) < 0) return -1;
				if (index_1 >= (blockSize / 4))return -1;
				direct_block_num = *(((unsigned int *)buf) + index_1);
			}
		}
		else return -1;
	}
	if (!hole && direct != -1)
	{
		if (direct < 12) direct_block_num = inode.i_block[direct];
		else return -1;
	}
	if (hole) memset(buf, 0, blockSize);
	else
	{
		if (lseek(f->file, translate(direct_block_num * blockSize, f, bootSector, vdiMap), SEEK_SET) < 0) return -1;
		if (read(f->file, buf, blockSize) < 0) return -1;
	}
	unsigned int difference = inode.size - (blockNum * blockSize);
	if (difference >= blockSize) return blockSize;
	return difference;
}

/**
 * Helper function to compute the indices. 
 */
void computeIndex(unsigned int blockNum, unsigned int blockSize, int& directNumber, int& indirectIndex, int& index2,
                  int& index3)
{
	if (blockNum <= 11)
	{
		directNumber = blockNum;
		indirectIndex = -1;
		index2 = -1;
		index3 = -1;
		return;
	}
	unsigned int blocksRem = blockNum - 12;
	if (blocksRem < blockSize / 4)
	{
		directNumber = -1;
		index2 = -1;
		index3 = -1;
		indirectIndex = blocksRem;
		return;
	}
	blocksRem -= blockSize / 4;
	if (blocksRem < (blockSize / 4) * (blockSize / 4))
	{
		directNumber = -1;
		index3 = -1;
		index2 = blocksRem / (blockSize / 4);
		indirectIndex = blocksRem - (index2) * (blockSize / 4);
		return;
	}
	blocksRem -= (blockSize / 4) * (blockSize / 4);
	if (blocksRem < (blockSize / 4) * (blockSize / 4) * (blockSize / 4))
	{
		directNumber = -1;
		index3 = blocksRem / ((blockSize / 4) * (blockSize / 4));
		index2 = (blocksRem - (index3) * (blockSize / 4) * (blockSize / 4)) / (blockSize / 4);
		indirectIndex = (blocksRem - (index3) * (blockSize / 4) * (blockSize / 4)) - (index2) * (blockSize / 4);
		return;
	}
}

  
