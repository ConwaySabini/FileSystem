// ============================================================================
// fs.c - user FileSytem API
// ============================================================================

#include "bfs.h"
#include "fs.h"

// ============================================================================
// Close the file currently open on file descriptor 'fd'.
// ============================================================================
i32 fsClose(i32 fd)
{
    i32 inum = bfsFdToInum(fd);
    bfsDerefOFT(inum);
    return 0;
}

// ============================================================================
// Create the file called 'fname'.  Overwrite, if it already exsists.
// On success, return its file descriptor.  On failure, EFNF
// ============================================================================
i32 fsCreate(str fname)
{
    i32 inum = bfsCreateFile(fname);
    if (inum == EFNF)
        return EFNF;
    return bfsInumToFd(inum);
}

// ============================================================================
// Format the BFS disk by initializing the SuperBlock, Inodes, Directory and
// Freelist.  On succes, return 0.  On failure, abort
// ============================================================================
i32 fsFormat()
{
    FILE *fp = fopen(BFSDISK, "w+b");
    if (fp == NULL)
        FATAL(EDISKCREATE);

    i32 ret = bfsInitSuper(fp); // initialize Super block
    if (ret != 0)
    {
        fclose(fp);
        FATAL(ret);
    }

    ret = bfsInitInodes(fp); // initialize Inodes block
    if (ret != 0)
    {
        fclose(fp);
        FATAL(ret);
    }

    ret = bfsInitDir(fp); // initialize Dir block
    if (ret != 0)
    {
        fclose(fp);
        FATAL(ret);
    }

    ret = bfsInitFreeList(); // initialize Freelist
    if (ret != 0)
    {
        fclose(fp);
        FATAL(ret);
    }

    fclose(fp);
    return 0;
}

// ============================================================================
// Mount the BFS disk.  It must already exist
// ============================================================================
i32 fsMount()
{
    FILE *fp = fopen(BFSDISK, "rb");
    if (fp == NULL)
        FATAL(ENODISK); // BFSDISK not found
    fclose(fp);
    return 0;
}

// ============================================================================
// Open the existing file called 'fname'.  On success, return its file
// descriptor.  On failure, return EFNF
// ============================================================================
i32 fsOpen(str fname)
{
    i32 inum = bfsLookupFile(fname); // lookup 'fname' in Directory
    if (inum == EFNF)
        return EFNF;
    return bfsInumToFd(inum);
}

// ============================================================================
// Read 'numb' bytes of data from the cursor in the file currently fsOpen'd on
// File Descriptor 'fd' into 'buf'.  On success, return actual number of bytes
// read (may be less than 'numb' if we hit EOF).  On failure, abort
// ============================================================================
i32 fsRead(i32 fd, i32 numb, void *buf)
{
    i32 inum = bfsFdToInum(fd);
    i32 cursor = bfsTell(fd);
    i32 curs_start = cursor;
    i32 size = bfsGetSize(inum);
    Inode node = {.size = 0, .direct = {[2] = 0}, .indirect = 0};
    Inode *inode = &node;
    i32 eof = 0;
    i32 sum = 0;
    i8 done = 0;
    i32 read = 0;
    i32 block_count = 0;
    i8 bioBuf[BYTESPERBLOCK];
    bfsRefOFT(inum);
    bfsReadInode(inum, inode);

    i32 i = 0;
    for (i = cursor / BYTESPERBLOCK; i < NUMDIRECT; i++)
    {
        if (cursor > BYTESPERBLOCK * NUMDIRECT) // if blocks in indirect break
        {
            break;
        }
        bfsRead(inum, i, (i8 *)bioBuf);
        if (bioBuf[0] == 0) // if buffer starts at 0 keep reading 0s
        {
            read = 1;
        }
        for (i32 l = 0; l < BYTESPERBLOCK; l++)
        {
            if (read == 0 && bioBuf[l] == 0) // if read 0 stop
            {
                eof = l;
                sum += l;
                done = 1;
                break;
            }
            else if (read == 1 && bioBuf[l] != 0) // if read 0 stop
            {
                read = 0;
            }
            if (bioBuf[l] == -1 || sum + l == numb) // if eof stop
            {
                eof = l;
                sum += l;
                done = 1;
                break;
            }
        }
        if (eof == 0)
        {
            if (cursor + BYTESPERBLOCK > size) // if read too far stop
            {
                eof = size - cursor;
                sum += eof;
                done = 1;
            }
            else // increment sum
            {
                eof = BYTESPERBLOCK;
                sum += BYTESPERBLOCK;
            }
        }
        bfsSetCursor(inum, cursor + eof);
        memmove((buf + block_count * BYTESPERBLOCK), (void *)bioBuf, eof);
        block_count++;
        cursor = bfsTell(fd);
        eof = 0;
        if (sum == numb || done == 1)
            break;
    }
    // follows similar logic above so no duplicate comments
    i = 0;
    if (cursor < size && done != 1)
    {
        eof = 0;
        while (cursor < size && done != 1)
        {
            i32 fbn = cursor / BYTESPERBLOCK; // get indirect block
            bfsRead(inum, fbn, (i8 *)bioBuf);
            if (bioBuf[0] == 0)
            {
                read = 1;
            }
            for (i32 l = 0; l < BYTESPERBLOCK; l++)
            {
                if (read == 0 && bioBuf[l] == 0)
                {
                    eof = l;
                    sum += l;
                    done = 1;
                    break;
                }
                else if (read == 1 && bioBuf[l] != 0) // if read 0 stop
                {
                    read = 0;
                }
                if (bioBuf[l] == -1 || sum + l == numb)
                {
                    eof = l;
                    sum += l;
                    done = 1;
                    break;
                }
            }
            if (eof == 0)
            {
                if (cursor + BYTESPERBLOCK > size)
                {
                    eof = size - cursor;
                    sum += eof;
                    done = 1;
                }
                else
                {
                    eof = BYTESPERBLOCK;
                    sum += BYTESPERBLOCK;
                }
            }
            bfsSetCursor(inum, cursor + eof);
            memmove((buf + block_count * BYTESPERBLOCK), (void *)bioBuf, eof);
            block_count++;
            eof = 0;
            cursor = bfsTell(fd);
            if (sum == numb || done == 1)
                break;
            fbn++;
        }
    }
    bfsDerefOFT(inum);
    return sum;
}

// ============================================================================
// Move the cursor for the file currently open on File Descriptor 'fd' to the
// byte-offset 'offset'.  'whence' can be any of:
//
//  SEEK_SET : set cursor to 'offset'
//  SEEK_CUR : add 'offset' to the current cursor
//  SEEK_END : add 'offset' to the size of the file
//
// On success, return 0.  On failure, abort
// ============================================================================
i32 fsSeek(i32 fd, i32 offset, i32 whence)
{

    if (offset < 0)
        FATAL(EBADCURS);

    i32 inum = bfsFdToInum(fd);
    i32 ofte = bfsFindOFTE(inum);

    switch (whence)
    {
    case SEEK_SET:
        g_oft[ofte].curs = offset;
        break;
    case SEEK_CUR:
        g_oft[ofte].curs += offset;
        break;
    case SEEK_END:
    {
        i32 end = fsSize(fd);
        g_oft[ofte].curs = end + offset;
        break;
    }
    default:
        FATAL(EBADWHENCE);
    }
    return 0;
}

// ============================================================================
// Return the cursor position for the file open on File Descriptor 'fd'
// ============================================================================
i32 fsTell(i32 fd)
{
    return bfsTell(fd);
}

// ============================================================================
// Retrieve the current file size in bytes.  This depends on the highest offset
// written to the file, or the highest offset set with the fsSeek function.  On
// success, return the file size.  On failure, abort
// ============================================================================
i32 fsSize(i32 fd)
{
    i32 inum = bfsFdToInum(fd);
    return bfsGetSize(inum);
}

// ============================================================================
// Write 'numb' bytes of data from 'buf' into the file currently fsOpen'd on
// filedescriptor 'fd'.  The write starts at the current file offset for the
// destination file.  On success, return 0.  On failure, abort
// ============================================================================
i32 fsWrite(i32 fd, i32 numb, void *buf) //TODO write inode
{
    i32 inum = bfsFdToInum(fd);
    i32 cursor = bfsTell(fd);
    Inode inode;
    i32 fsize = bfsGetSize(inum);
    i8 bioBuf[BYTESPERBLOCK];
    i32 inside = 0;
    i32 outside = 0;
    i32 overlap = 0;
    i32 size = fsize;

    bfsRefOFT(inum);
    bfsReadInode(inum, &inode);

    i32 write_size = cursor + numb;

    if (write_size < fsize)
    {
        inside = 1;
    }
    else if (cursor < fsize)
    {
        overlap = 1;
    }
    else
    {
        outside = 1;
    }

    //created a buffer
    i32 buf_rem = write_size % BYTESPERBLOCK;
    i32 bytes = write_size + buf_rem;
    i32 blocks = 0;
    if (numb % BYTESPERBLOCK == 0)
    {
        blocks = (numb / BYTESPERBLOCK);
    }
    else
    {
        blocks = (numb / BYTESPERBLOCK) + 1;
    }
    i8 *buffer_p = malloc(bytes);
    i8 buffer = *buffer_p;

    i32 curs_block = cursor / BYTESPERBLOCK;
    i32 start_dbn = 0;
    i32 final_fbn = (cursor + numb) / BYTESPERBLOCK;

    i32 i = 0;
    if (inside == 1 || overlap == 1)
    {
        start_dbn = bfsFbnToDbn(inum, curs_block);
        bfsRead(inum, curs_block, (i8 *)bioBuf); // read first block
        i32 curs_remain = cursor % BYTESPERBLOCK;
        memmove((void *)buffer_p, (void *)bioBuf, curs_remain);
        memmove((void *)(buffer_p + curs_remain), buf, numb);

        if (numb < BYTESPERBLOCK) // if we only read first block
        {
            i32 offset = numb + curs_remain;
            memmove((void *)(buffer_p + offset), (void *)(bioBuf + offset), BYTESPERBLOCK - offset);
        }
        else if (inside == 1) // get final block and move leftover data
        {
            bfsRead(inum, final_fbn, (i8 *)bioBuf);
            i32 offset = numb + curs_remain;
            i32 remainder = offset % BYTESPERBLOCK;
            memmove(((void *)(buffer_p + offset)), (void *)(bioBuf + remainder), BYTESPERBLOCK - remainder);
        }
        else if (overlap == 1)
        {
            size = write_size;
            i32 extendo = fsize + numb;
            i32 eblocks = (extendo / BYTESPERBLOCK);
            bfsExtend(inum, eblocks);
        }

        i32 fbn = curs_block;
        i32 dbn = start_dbn;

        for (i = 0; i < blocks; i++)
        {
            if (dbn > BLOCKSPERDISK - 1)
            {
                break;
            }
            bioWrite(dbn, (void *)(buffer_p + (i * BYTESPERBLOCK)));
            fbn++;
            dbn = bfsFbnToDbn(inum, fbn);
        }
        bfsSetCursor(inum, write_size);
        cursor = write_size;
    }
    else if (outside)
    {
        memmove((void *)buffer_p, buf, numb);
        size = fsize + numb;
        i32 extendo = fsize + numb;
        i32 eblocks = (extendo / BYTESPERBLOCK);
        bfsExtend(inum, eblocks);

        i32 fbn = curs_block;
        i32 dbn = bfsFbnToDbn(inum, fbn);
        for (i = 0; i < blocks; i++)
        {
            if (dbn > BLOCKSPERDISK - 1)
            {
                break;
            }
            bioWrite(dbn, (void *)(buffer_p + (i * BYTESPERBLOCK)));
            fbn++;
            dbn = bfsFbnToDbn(inum, fbn);
        }
        bfsSetCursor(inum, write_size);
        cursor = write_size;
    }
    bfsSetSize(inum, size);
    bfsDerefOFT(inum);
    return 0;
}
