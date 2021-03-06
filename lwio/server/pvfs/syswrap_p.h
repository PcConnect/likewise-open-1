/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil; tab-width: 4 -*-
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * Editor Settings: expandtabs and use 4 spaces for indentation */

/*
 * Copyright Likewise Software
 * All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.  You should have received a copy of the GNU General
 * Public License along with this program.  If not, see
 * <http://www.gnu.org/licenses/>.
 *
 * LIKEWISE SOFTWARE MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING
 * TERMS AS WELL.  IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT
 * WITH LIKEWISE SOFTWARE, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE
 * TERMS OF THAT SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE GNU
 * GENERAL PUBLIC LICENSE, NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU
 * HAVE QUESTIONS, OR WISH TO REQUEST A COPY OF THE ALTERNATE LICENSING
 * TERMS OFFERED BY LIKEWISE SOFTWARE, PLEASE CONTACT LIKEWISE SOFTWARE AT
 * license@likewisesoftware.com
 */

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        syswrap_p.h
 *
 * Abstract:
 *
 *        Likewise Posix File System Driver (PVFS)
 *
 *        syscall wrappers
 *
 * Authors: Gerald Carter <gcarter@likewise.com>
 */


#ifndef _PVFS_SYSWRAP_P_H
#define _PVFS_SYSWRAP_P_H

#include "config.h"

#include <sys/types.h>
#include <unistd.h>
#include <time.h>
#include <sys/stat.h>
#include <fcntl.h>


/* Syscall wrappers */

NTSTATUS
PvfsSysStat(
	PCSTR pszFilename,
	PPVFS_STAT pStat
	);

NTSTATUS
PvfsSysFstat(
	int fd,
	PPVFS_STAT pStat
	);

NTSTATUS
PvfsSysClose(
    int fd
    );

NTSTATUS
PvfsSysOpenDir(
    IN PCSTR pszDirname,
    OUT OPTIONAL DIR **ppDir
    );

NTSTATUS
PvfsSysReadDir(
    DIR *pDir,
    struct dirent *pDirEntry,
    struct dirent **ppDirEntry
    );

NTSTATUS
PvfsSysCloseDir(
    DIR *pDir
    );

NTSTATUS
PvfsSysLseek(
    int fd,
    off_t offset,
    int whence,
    off_t *pNewOffset
    );

NTSTATUS
PvfsSysFtruncate(
    int fd,
    off_t offset
    );

NTSTATUS
PvfsSysFstatFs(
    PPVFS_CCB pCcb,
    PPVFS_STATFS pStatFs
    );

NTSTATUS
PvfsSysRead(
    PPVFS_CCB pCcb,
    PVOID pBuffer,
    ULONG pBufLen,
    PULONG64 pOffset,
    PULONG pBytesRead
    );

NTSTATUS
PvfsSysWrite(
    PPVFS_CCB pCcb,
    PVOID pBuffer,
    ULONG pBufLen,
    PULONG64 pOffset,
    PULONG pBytesWritten
    );


NTSTATUS
PvfsSysFchmod(
    PPVFS_CCB pCcb,
    mode_t Mode
    );

NTSTATUS
PvfsSysFsync(
    PPVFS_CCB pCcb
    );

NTSTATUS
PvfsSysNanoSleep(
    const struct timespec *pRequestedTime,
    struct timespec *pRemainingTime
    );

NTSTATUS
PvfsSysPipe(
    int PipeFds[2]
    );

NTSTATUS
PvfsSysSetNonBlocking(
    int Fd
    );

#ifdef HAVE_SPLICE
NTSTATUS
PvfsSysSplice(
    int FromFd,
    PLONG64 pFromOffset,
    int ToFd,
    PLONG64 pToOffset,
    ULONG Length,
    unsigned int Flags,
    PULONG pBytesSpliced
    );
#endif


//
// New PVFS_FILE_NAME interfaces
//

NTSTATUS
PvfsSysStatByFileName(
    IN PPVFS_FILE_NAME FileName,
	IN OUT PPVFS_STAT Stat
	);

NTSTATUS
PvfsSysRenameByFileName(
    IN PPVFS_FILE_NAME OriginalFileName,
    IN PPVFS_FILE_NAME NewFielName
    );

NTSTATUS
PvfsSysOpenByFileName(
    OUT int *pFd,
    OUT PBOOLEAN pbCreateOwnerFile,
    IN PPVFS_FILE_NAME pFileName,
    IN int iFlags,
    IN mode_t Mode
    );

NTSTATUS
PvfsSysRemoveByFileName(
    IN PPVFS_FILE_NAME FileName
    );

NTSTATUS
PvfsSysMkDirByFileName(
    IN PPVFS_FILE_NAME DirectoryName,
    mode_t mode
    );

NTSTATUS
PvfsSysChownByFileName(
    IN PPVFS_FILE_NAME pFileName,
    uid_t uid,
    gid_t gid
    );

NTSTATUS
PvfsSysLinkByFileName(
    IN PPVFS_FILE_NAME pOldname,
    IN PPVFS_FILE_NAME pNewname
    );

//
// Syscall interfaces on base file objects (not the stream itself)
//

NTSTATUS
PvfsSysStatByFcb(
    IN PPVFS_FCB pFcb,
    IN OUT PPVFS_STAT pStat
    );

NTSTATUS
PvfsSysUtimeByFcb(
    IN PPVFS_FCB pFcb,
    IN LONG64 LastWriteTime,
    IN LONG64 LastAccessTime
    );


#endif     /* _PVFS_SYSWRAP_P_H */

