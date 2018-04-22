#include "vdi_write.h"
using namespace std;

int writeSuperblock(VDIFile* f, BootSector boot, unsigned int vdiMap[], ext2_super_block& superBlock)
{
	unsigned int superblockLocation = translate(1024, f, boot, vdiMap);
	if (lseek(f->file, superblockLocation, SEEK_SET) < 0)
	{
		cout << "Error! Failed to seek to the superblock" << endl;
		return 1;
	}
	if (write(f->file, &superBlock, sizeof(superBlock)) != 1024)
	{
		cout << "Error! superblock was not written correctly!" << endl;
		return 1;
	}
	return 0;
}

int writeGroupDescriptor(VDIFile* f, BootSector boot, unsigned int vdiMap[], unsigned int blockSize,
                         ext2_group_descriptor groupDescriptor[], unsigned int numberBlockGroups)
{
	unsigned int groupStart = 0;
	if (blockSize == 1024) groupStart = blockSize * 2;
	else groupStart = blockSize;

	unsigned int groupLoc = translate(groupStart, f, boot, vdiMap);
	if (lseek(f->file, groupLoc, SEEK_SET) < 0)
	{
		cout << "Error! Failed to seek to the group desc" << endl;
		return 1;
	}
	size_t size = sizeof(ext2_group_descriptor) * numberBlockGroups;
	if (write(f->file, groupDescriptor, size) != size)
	{
		cout << "Error! Failed to write group desc" << endl;
		return 1;
	}
	return 0;
}

int writeBitmap(VDIFile* f, BootSector boot, unsigned int vdiMap[], unsigned char* bitmap, unsigned int blockSize,
                unsigned int blockId)
{
	if (lseek(f->file, translate(blockId * blockSize, f, boot, vdiMap), SEEK_SET) < 0)
	{
		cout << "Error! Failed to seek to bitmap" << endl;
		return 1;
	}
	if (write(f->file, bitmap, blockSize) != blockSize)
	{
		cout << "Error! Failed to write bitmap" << endl;
		return 1;
	}
	return 0;
}

int writeBlock(VDIFile* f, BootSector boot, unsigned int vdiMap[], ext2_inode inode,
               std::vector<unsigned int>& addresses, unsigned int blockNum, unsigned int blockSize, unsigned char* data)
{
	if (blockNum * blockSize >= inode.size) return -1;
	unsigned char* buffer = (unsigned char*)malloc(blockSize);
	int directIndex;
	int singleIndex;
	int doubleIndex;
	int tripleIndex;

	computeIndex(blockNum, blockSize, directIndex, singleIndex, doubleIndex, tripleIndex);
	unsigned int tripleBlockNum = inode.i_block[14];
	unsigned int doubleBlockNum = 0;
	if (tripleIndex != -1)
	{
		if (doubleIndex != -1 && singleIndex != -1 && directIndex == -1)
		{
			if (lseek(f->file, translate(tripleBlockNum * blockSize, f, boot, vdiMap), SEEK_SET) < 0) return -1;
			if (read(f->file, buffer, blockSize) < 0) return -1;
			if (tripleIndex >= (blockSize / 4)) return -1;
			doubleBlockNum = *(((unsigned int *)buffer) + tripleIndex);
		}
		else return -1;
	}
	unsigned int singleBlockNum = 0;
	if (doubleIndex != -1)
	{
		if (singleIndex != -1 && directIndex == -1)
		{
			if (tripleIndex == -1)
			{
				doubleBlockNum = inode.i_block[13];
			}
			if (doubleBlockNum == 0)
			{
				doubleBlockNum = addresses.back();
				addresses.pop_back();
				*(((unsigned int *)buffer) + tripleIndex) = doubleBlockNum;
				if (lseek(f->file, translate(tripleBlockNum * blockSize, f, boot, vdiMap), SEEK_SET) < 0) return -1;
				if (write(f->file, buffer, blockSize) != blockSize) return -1;
			}
			if (lseek(f->file, translate(doubleBlockNum * blockSize, f, boot, vdiMap), SEEK_SET) < 0) return -1;
			if (read(f->file, buffer, blockSize) < 0) return -1;
			if (doubleIndex >= (blockSize / 4)) return -1;
			singleBlockNum = *(((unsigned int *)buffer) + doubleIndex);
		}
		else return -1;
	}

	unsigned int directBlockNum = 0;
	if (singleIndex != -1)
	{
		if (directIndex == -1)
		{
			if (doubleIndex == -1) singleBlockNum = inode.i_block[12];
			if (singleBlockNum == 0)
			{
				singleBlockNum = addresses.back();
				addresses.pop_back();
				*(((unsigned int *)buffer) + doubleIndex) = singleBlockNum;
				if (lseek(f->file, translate(doubleBlockNum * blockSize, f, boot, vdiMap), SEEK_SET) < 0) return -1;
				if (write(f->file, buffer, blockSize) != blockSize) return -1;
			}
			if (lseek(f->file, translate(singleBlockNum * blockSize, f, boot, vdiMap), SEEK_SET) < 0) return -1;
			if (read(f->file, buffer, blockSize) < 0) return -1;
			if (singleIndex >= (blockSize / 4)) return -1;
			directBlockNum = *(((unsigned int *)buffer) + singleIndex);
		}
		else return -1;
	}

	if (directIndex != -1)
	{
		if (directIndex < 12) directBlockNum = inode.i_block[directIndex];
		else return -1;
	}

	if (directBlockNum == 0)
	{
		directBlockNum = addresses.back();
		addresses.pop_back();
		*(((unsigned int *)buffer) + singleIndex) = directBlockNum;
		if (lseek(f->file, translate(singleBlockNum * blockSize, f, boot, vdiMap), SEEK_SET) < 0) return -1;
		if (write(f->file, buffer, blockSize) != blockSize) return -1;
	}

	if (lseek(f->file, translate(directBlockNum * blockSize, f, boot, vdiMap), SEEK_SET) < 0) return -1;
	if (write(f->file, data, blockSize) != blockSize) return -1;
	unsigned int diff = inode.size - (blockNum * blockSize);
	if (diff >= blockSize) return blockSize;
	return diff;
}

int writeInode(VDIFile* f, BootSector boot, unsigned int vdiMap[], ext2_inode inode, unsigned int inodeNum,
                unsigned int blockSize, ext2_super_block superBlock, ext2_group_descriptor groupDescriptor[])
{
	inodeNum--;
	unsigned int groupNum = inodeNum / superBlock.s_inodes_per_group;
	unsigned int offset1 = inodeNum % superBlock.s_inodes_per_group;
	unsigned int inodesPerBlock = blockSize / sizeof(ext2_inode);
	unsigned int blockNum = groupDescriptor[groupNum].inode_table + (offset1 / inodesPerBlock);
	unsigned int offset2 = inodeNum % inodesPerBlock;

	if (lseek(f->file, translate(blockNum * blockSize + offset2 * sizeof(ext2_inode), f, boot, vdiMap),
	          SEEK_SET) < 0) return 1;
	if (write(f->file, &inode, sizeof(ext2_inode)) != sizeof(ext2_inode)) return 1;

	return 0;
}

bool isBlockFree(unsigned char* bitmap, unsigned int blockSize, unsigned int blockOffset, unsigned int& address)
{
	for (int i = 0; i < blockSize; i++)
	{
		unsigned char current_byte = *(bitmap + i);

		for (int j = 0; j < 8; j++)
		{
			if (!((current_byte >> j) & 0x1))
			{
				address = blockOffset + i * 8 + j;
				if (blockSize == 1024) address++;
				*(bitmap + i) = ((1 << j) | current_byte);
				return true;
			}
		}
	}
	return false;
}

bool isInodeFree(unsigned char* bitmap, unsigned int blockSize, unsigned int inodeBlockOffset,
                   unsigned int& address)
{
	for (int i = 0; i < blockSize; i++)
	{
		unsigned char currentByte = *(bitmap + i);
		for (int j = 0; j < blockSize; j++)
		{
			if (!((currentByte >> j) & 0x1))
			{
				address = (inodeBlockOffset + j + i * 8) + 1;
				*(bitmap + i) = ((1 << j) | currentByte);
				return true;
			}
		}
	}
	return false;
}
