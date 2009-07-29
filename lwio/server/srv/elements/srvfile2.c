/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

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
 *        srvfile2.c
 *
 * Abstract:
 *
 *        Likewise IO (LWIO) - SRV
 *
 *        Elements
 *
 *        File Object (Version 2)
 *
 * Authors: Sriram Nambakam (snambakam@likewise.com)
 */

#include "includes.h"

static
VOID
SrvFile2Free(
    PLWIO_SRV_FILE_2 pFile
    );

NTSTATUS
SrvFile2Create(
    ULONG64                 ullFid,
    PWSTR                   pwszFilename,
    PIO_FILE_HANDLE         phFile,
    PIO_FILE_NAME*          ppFilename,
    ACCESS_MASK             desiredAccess,
    LONG64                  allocationSize,
    FILE_ATTRIBUTES         fileAttributes,
    FILE_SHARE_FLAGS        shareAccess,
    FILE_CREATE_DISPOSITION createDisposition,
    FILE_CREATE_OPTIONS     createOptions,
    PLWIO_SRV_FILE_2*       ppFile
    )
{
    NTSTATUS ntStatus = 0;
    PLWIO_SRV_FILE_2 pFile = NULL;

    LWIO_LOG_DEBUG("Creating file [fid:%lu]", ullFid);

    ntStatus = SrvAllocateMemory(
                    sizeof(LWIO_SRV_FILE_2),
                    (PVOID*)&pFile);
    BAIL_ON_NT_STATUS(ntStatus);

    pFile->refcount = 1;

    pthread_rwlock_init(&pFile->mutex, NULL);
    pFile->pMutex = &pFile->mutex;

    ntStatus = SrvAllocateStringW(pwszFilename, &pFile->pwszFilename);
    BAIL_ON_NT_STATUS(ntStatus);

    uuid_generate(pFile->GUID);

    pFile->ullFid = ullFid;
    pFile->hFile = *phFile;
    *phFile = NULL;
    pFile->pFilename = *ppFilename;
    *ppFilename = NULL;
    pFile->desiredAccess = desiredAccess;
    pFile->allocationSize = allocationSize;
    pFile->fileAttributes = fileAttributes;
    pFile->shareAccess = shareAccess;
    pFile->createDisposition = createDisposition;
    pFile->createOptions = createOptions;

    LWIO_LOG_DEBUG("Associating file [object:0x%x][fid:%lu]",
                    pFile,
                    ullFid);

    *ppFile = pFile;

cleanup:

    return ntStatus;

error:

    *ppFile = NULL;

    if (pFile)
    {
        SrvFile2Release(pFile);
    }

    goto cleanup;
}

VOID
SrvFile2Release(
    PLWIO_SRV_FILE_2 pFile
    )
{
    LWIO_LOG_DEBUG("Releasing file [fid:%lu]", pFile->ullFid);

    if (InterlockedDecrement(&pFile->refcount) == 0)
    {
        SrvFile2Free(pFile);
    }
}

static
VOID
SrvFile2Free(
    PLWIO_SRV_FILE_2 pFile
    )
{
    LWIO_LOG_DEBUG("Freeing file [object:0x%x][fid:%lu]",
                    pFile,
                    pFile->ullFid);

    if (pFile->pMutex)
    {
        pthread_rwlock_destroy(&pFile->mutex);
        pFile->pMutex = NULL;
    }

    if (pFile->pFilename)
    {
        if (pFile->pFilename->FileName)
        {
            SrvFreeMemory (pFile->pFilename->FileName);
        }

        SrvFreeMemory(pFile->pFilename);
    }

    if (pFile->hFile)
    {
        IoCloseFile(pFile->hFile);
    }

    if (pFile->pwszFilename)
    {
        SrvFreeMemory(pFile->pwszFilename);
    }

    if (pFile->searchSpace.pwszSearchPattern)
    {
        SrvFreeMemory(pFile->searchSpace.pwszSearchPattern);
    }

    if (pFile->searchSpace.pFileInfo)
    {
        SrvFreeMemory(pFile->searchSpace.pFileInfo);
    }

    SrvFreeMemory(pFile);
}
