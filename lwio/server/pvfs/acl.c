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
 *        acl.c
 *
 * Abstract:
 *
 *        Likewise Posix File System Driver (PVFS)
 *
 *        Supporting ACL routines
 *
 * Authors: Gerald Carter <gcarter@likewise.com>
 */

#include "pvfs.h"

/***********************************************************************
 **********************************************************************/

NTSTATUS
PvfsGetSecurityDescriptorFileDefault(
    IN PPVFS_CCB pCcb,
    IN SECURITY_INFORMATION SecInfo,
    IN OUT PSECURITY_DESCRIPTOR_RELATIVE pSecDesc,
    IN OUT PULONG pSecDescLen
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;
    BYTE pFullSecDesc[SECURITY_DESCRIPTOR_RELATIVE_MAX_SIZE];
    ULONG FullSecDescLen = SECURITY_DESCRIPTOR_RELATIVE_MAX_SIZE;

    LwRtlZeroMemory(pFullSecDesc, FullSecDescLen);

    ntError = PvfsGetSecurityDescriptorPosix(
                  pCcb,
                  (PSECURITY_DESCRIPTOR_RELATIVE)pFullSecDesc,
                  &FullSecDescLen);
    BAIL_ON_NT_STATUS(ntError);

    ntError = RtlQuerySecurityDescriptorInfo(
                  SecInfo,
                  pSecDesc,
                  pSecDescLen,
                  (PSECURITY_DESCRIPTOR_RELATIVE)pFullSecDesc);
    BAIL_ON_NT_STATUS(ntError);

cleanup:
    return ntError;

error:
    goto cleanup;

}


/***********************************************************************
 **********************************************************************/

NTSTATUS
PvfsGetSecurityDescriptorFile(
    IN PPVFS_CCB pCcb,
    IN SECURITY_INFORMATION SecInfo,
    IN OUT PSECURITY_DESCRIPTOR_RELATIVE pSecDesc,
    IN OUT PULONG pSecDescLen
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;
    PSECURITY_DESCRIPTOR_RELATIVE pFullSecDesc = NULL;
    ULONG FullSecDescLen = 1024;
    BOOLEAN bNoStoredSecDesc = FALSE;

    do
    {
        ntError = PvfsReallocateMemory((PVOID*)&pFullSecDesc, FullSecDescLen);
        BAIL_ON_NT_STATUS(ntError);

        ntError = STATUS_RESOURCE_DATA_NOT_FOUND;

#ifdef HAVE_EA_SUPPORT
        /* Try to get from EA but short circuit future attempts this go
           round if there is nothing there */

        if (!bNoStoredSecDesc) {
            ntError = PvfsGetSecurityDescriptorFileXattr(
                          pCcb,
                          pFullSecDesc,
                          &FullSecDescLen);
        }
#endif

        /* EA security descriptor support not available or present for
           this object */

        if (ntError == STATUS_RESOURCE_DATA_NOT_FOUND)
        {
            bNoStoredSecDesc = TRUE;
            ntError = PvfsGetSecurityDescriptorPosix(
                          pCcb,
                          pFullSecDesc,
                          &FullSecDescLen);
        }

        /* Need more space, else fail */

        if (ntError == STATUS_BUFFER_TOO_SMALL)
        {
            FullSecDescLen *= 2;
        }
        else
        {
            BAIL_ON_NT_STATUS(ntError);
        }


    } while ((ntError != STATUS_SUCCESS) &&
             (FullSecDescLen <= SECURITY_DESCRIPTOR_RELATIVE_MAX_SIZE));
    BAIL_ON_NT_STATUS(ntError);

    ntError = RtlQuerySecurityDescriptorInfo(
                  SecInfo,
                  pSecDesc,
                  pSecDescLen,
                  pFullSecDesc);
    BAIL_ON_NT_STATUS(ntError);

cleanup:
    PVFS_FREE(&pFullSecDesc);

    return ntError;

error:
    goto cleanup;

}


/****************************************************************
 ***************************************************************/

NTSTATUS
PvfsGetSecurityDescriptorFilename(
    IN PCSTR pszFilename,
    IN SECURITY_INFORMATION SecInfo,
    IN OUT PSECURITY_DESCRIPTOR_RELATIVE pSecDesc,
    IN OUT PULONG pSecDescLen
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;
    PSECURITY_DESCRIPTOR_RELATIVE pFullSecDesc = NULL;
    ULONG FullSecDescLen = 1024;
    BOOLEAN bNoStoredSecDesc = FALSE;

    do
    {
        ntError = PvfsReallocateMemory((PVOID*)&pFullSecDesc, FullSecDescLen);
        BAIL_ON_NT_STATUS(ntError);

        ntError = STATUS_RESOURCE_DATA_NOT_FOUND;

#ifdef HAVE_EA_SUPPORT
        /* Try to get from EA but short circuit future attempts this go
           round if there is nothing there */

        if (!bNoStoredSecDesc) {
            ntError = PvfsGetSecurityDescriptorFilenameXattr(pszFilename,
                                                             pFullSecDesc,
                                                             &FullSecDescLen);
        }
#endif

        /* EA security descriptor support not available or present for
           this object */

        if (ntError == STATUS_RESOURCE_DATA_NOT_FOUND)
        {
            bNoStoredSecDesc = TRUE;
            ntError = PvfsGetSecurityDescriptorFilenamePosix(
                          pszFilename,
                          pFullSecDesc,
                          &FullSecDescLen);
        }

        /* Need more space, else fail */

        if (ntError == STATUS_BUFFER_TOO_SMALL)
        {
            FullSecDescLen *= 2;
        }
        else
        {
            BAIL_ON_NT_STATUS(ntError);
        }

    } while ((ntError != STATUS_SUCCESS) &&
             (FullSecDescLen <= SECURITY_DESCRIPTOR_RELATIVE_MAX_SIZE));
    BAIL_ON_NT_STATUS(ntError);

    ntError = RtlQuerySecurityDescriptorInfo(
                  SecInfo,
                  pSecDesc,
                  pSecDescLen,
                  pFullSecDesc);
    BAIL_ON_NT_STATUS(ntError);

cleanup:
    PVFS_FREE(&pFullSecDesc);

    return ntError;

error:
    goto cleanup;

}

/****************************************************************
 ***************************************************************/

NTSTATUS
PvfsSetSecurityDescriptorFile(
    IN PPVFS_CCB pCcb,
    IN SECURITY_INFORMATION SecInfo,
    IN PSECURITY_DESCRIPTOR_RELATIVE pSecDesc,
    IN ULONG SecDescLen
    )
{
    NTSTATUS ntError = STATUS_ACCESS_DENIED;
    PSECURITY_DESCRIPTOR_RELATIVE pSDCur = NULL;
    ULONG SDCurLen = 1024;
    PSECURITY_DESCRIPTOR_RELATIVE pNewSecDesc = NULL;
    ULONG NewSecDescLen = 0;
    PSECURITY_DESCRIPTOR_RELATIVE pFinalSecDesc = NULL;
    ULONG FinalSecDescLen = 0;
    SECURITY_INFORMATION SecInfoAll = (OWNER_SECURITY_INFORMATION |
                                       GROUP_SECURITY_INFORMATION |
                                       DACL_SECURITY_INFORMATION |
                                       SACL_SECURITY_INFORMATION);

    /* Sanity checks */

    if (SecInfo == 0) {
        ntError = STATUS_INVALID_PARAMETER;
        BAIL_ON_NT_STATUS(ntError);
    }

    if (SecInfo == SecInfoAll)
    {
        /* We already have a fully formed Security Descriptor */

        pFinalSecDesc = pSecDesc;
        FinalSecDescLen = SecDescLen;
    }
    else
    {
        /* Retrieve the existing SD and merge with the incoming one */

        do
        {
            ntError = PvfsReallocateMemory((PVOID*)&pSDCur, SDCurLen);
            BAIL_ON_NT_STATUS(ntError);

            ntError = PvfsGetSecurityDescriptorFile(
                          pCcb,
                          SecInfoAll,
                          pSDCur,
                          &SDCurLen);
            if (ntError == STATUS_BUFFER_TOO_SMALL) {
                SDCurLen *= 2;
            }
        } while ((ntError != STATUS_SUCCESS) && (SDCurLen <= 4096));
        BAIL_ON_NT_STATUS(ntError);

        /* Assume that the new SD is <= the combined size of the current
           SD and the incoming one */

        NewSecDescLen = SDCurLen + SecDescLen;
        ntError = PvfsAllocateMemory((PVOID*)&pNewSecDesc, NewSecDescLen);
        BAIL_ON_NT_STATUS(ntError);

        ntError = RtlSetSecurityDescriptorInfo(
                      SecInfo,
                      pSecDesc,
                      pSDCur,
                      pNewSecDesc,
                      &NewSecDescLen,
                      &gPvfsFileGenericMapping);
        BAIL_ON_NT_STATUS(ntError);

        pFinalSecDesc = pNewSecDesc;
        FinalSecDescLen = NewSecDescLen;

    }


    /* Save the combined SD */

#ifdef HAVE_EA_SUPPORT
    ntError = PvfsSetSecurityDescriptorFileXattr(
                  pCcb,
                  pFinalSecDesc,
                  FinalSecDescLen);
#else
    ntError = PvfsSetSecurityDescriptorPosix(
                  pCcb,
                  pFinalSecDesc,
                  FinalSecDescLen);
#endif

    BAIL_ON_NT_STATUS(ntError);

    PvfsNotifyScheduleFullReport(
        pCcb->pFcb,
        FILE_NOTIFY_CHANGE_SECURITY,
        FILE_ACTION_MODIFIED,
        pCcb->pszFilename);


cleanup:
    PVFS_FREE(&pSDCur);
    PVFS_FREE(&pNewSecDesc);

    return ntError;

error:
    goto cleanup;
}

/****************************************************************
 ATTN: Always use RTL routines when allocating memory for
 PSECURITY_DESCRIPTOR_ABSOLUTE.  Else the Free() here will not
 be symmetic.
 ***************************************************************/

VOID
PvfsFreeAbsoluteSecurityDescriptor(
    IN OUT PSECURITY_DESCRIPTOR_ABSOLUTE *ppSecDesc
    )
{
    NTSTATUS ntError = STATUS_SUCCESS;
    PSID pOwner = NULL;
    PSID pGroup = NULL;
    PACL pDacl = NULL;
    PACL pSacl = NULL;
    BOOLEAN bDefaulted = FALSE;
    BOOLEAN bPresent = FALSE;
    PSECURITY_DESCRIPTOR_ABSOLUTE pSecDesc = NULL;

    if ((ppSecDesc == NULL) || (*ppSecDesc == NULL)) {
        return;
    }

    pSecDesc = *ppSecDesc;

    ntError = RtlGetOwnerSecurityDescriptor(pSecDesc, &pOwner, &bDefaulted);
    ntError = RtlGetGroupSecurityDescriptor(pSecDesc, &pGroup, &bDefaulted);

    ntError = RtlGetDaclSecurityDescriptor(pSecDesc, &bPresent, &pDacl, &bDefaulted);
    ntError = RtlGetSaclSecurityDescriptor(pSecDesc, &bPresent, &pSacl, &bDefaulted);

    RTL_FREE(&pSecDesc);
    RTL_FREE(&pOwner);
    RTL_FREE(&pGroup);
    RTL_FREE(&pDacl);
    RTL_FREE(&pSacl);

    *ppSecDesc = NULL;

    return;
}


/***********************************************************************
 **********************************************************************/

NTSTATUS
PvfsCreateFileSecurity(
    PACCESS_TOKEN pUserToken,
    PPVFS_CCB pCcb,
    PSECURITY_DESCRIPTOR_RELATIVE pSecurityDescriptor,
    BOOLEAN bIsDirectory
    )
{
    NTSTATUS ntError = STATUS_SUCCESS;
    PSTR pszParentPath = NULL;
    PSTR pszBaseFilename = NULL;
    PSECURITY_DESCRIPTOR_RELATIVE pFinalSecDesc = NULL;
    ULONG FinalSecDescLength = 0;
    BYTE ParentSecDescBuffer[SECURITY_DESCRIPTOR_RELATIVE_MAX_SIZE];
    ULONG ParentSecDescLength = SECURITY_DESCRIPTOR_RELATIVE_MAX_SIZE;
    SECURITY_INFORMATION SecInfoAll = (OWNER_SECURITY_INFORMATION |
                                       GROUP_SECURITY_INFORMATION |
                                       DACL_SECURITY_INFORMATION  |
                                       SACL_SECURITY_INFORMATION);
    BYTE DefaultSecDescBuffer[SECURITY_DESCRIPTOR_RELATIVE_MAX_SIZE];
    ULONG DefaultSecDescLength = SECURITY_DESCRIPTOR_RELATIVE_MAX_SIZE;

    LwRtlZeroMemory(ParentSecDescBuffer, ParentSecDescLength);
    LwRtlZeroMemory(DefaultSecDescBuffer, DefaultSecDescLength);

    ntError = PvfsFileSplitPath(
                  &pszParentPath,
                  &pszBaseFilename,
                  pCcb->pszFilename);
    BAIL_ON_NT_STATUS(ntError);

    ntError = PvfsGetSecurityDescriptorFilename(
                  pszParentPath,
                  SecInfoAll,
                  (PSECURITY_DESCRIPTOR_RELATIVE)ParentSecDescBuffer,
                  &ParentSecDescLength);
    BAIL_ON_NT_STATUS(ntError);

    ntError = RtlCreatePrivateObjectSecurityEx(
                  (PSECURITY_DESCRIPTOR_RELATIVE)ParentSecDescBuffer,
                  pSecurityDescriptor,
                  &pFinalSecDesc,
                  &FinalSecDescLength,
                  NULL,
                  bIsDirectory,
                  SEF_DACL_AUTO_INHERIT|SEF_SACL_AUTO_INHERIT,
                  pUserToken,
                  &gPvfsFileGenericMapping);
    BAIL_ON_NT_STATUS(ntError);

    ntError = PvfsSetSecurityDescriptorFile(
                  pCcb,
                  SecInfoAll,
                  pFinalSecDesc,
                  FinalSecDescLength);
    BAIL_ON_NT_STATUS(ntError);

cleanup:
    RtlCStringFree(&pszParentPath);
    RtlCStringFree(&pszBaseFilename);

    return ntError;

error:
    goto cleanup;
}



/****************************************************************
 ***************************************************************/

#if 0  /* UNUSED but useful to cut-n-paste */

static NTSTATUS
CreateDefaultSecDesc(
    IN OUT PSECURITY_DESCRIPTOR_RELATIVE pSecDescRelative,
    IN OUT PULONG pSecDescLen,
    BOOLEAN bIsDirectory
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;
    PSECURITY_DESCRIPTOR_ABSOLUTE pSecDesc = NULL;
    PACL pDacl = NULL;
    PSID pSid = NULL;

    ntError= RTL_ALLOCATE(&pSecDesc,
                          VOID,
                          SECURITY_DESCRIPTOR_ABSOLUTE_MIN_SIZE);
    BAIL_ON_NT_STATUS(ntError);

    ntError = RtlCreateSecurityDescriptorAbsolute(pSecDesc,
                                                  SECURITY_DESCRIPTOR_REVISION);
    BAIL_ON_NT_STATUS(ntError);

    /* Owner: Administrators */

    ntError = RtlAllocateSidFromCString(&pSid, "S-1-5-32-544");
    BAIL_ON_NT_STATUS(ntError);

    ntError = RtlSetOwnerSecurityDescriptor(pSecDesc,
                                            pSid,
                                            FALSE);
    BAIL_ON_NT_STATUS(ntError);

    pSid = NULL;

    /* Group: Power Users */

    ntError = RtlAllocateSidFromCString(&pSid, "S-1-5-32-547");
    BAIL_ON_NT_STATUS(ntError);

    ntError = RtlSetGroupSecurityDescriptor(pSecDesc,
                                            pSid,
                                            FALSE);
    BAIL_ON_NT_STATUS(ntError);

    pSid = NULL;

    /* DACL */

    if (bIsDirectory) {
        ntError = BuildDefaultDaclDirectory(&pDacl);
    } else {
        ntError = BuildDefaultDaclFile(&pDacl);
    }
    BAIL_ON_NT_STATUS(ntError);

    ntError = RtlSetDaclSecurityDescriptor(pSecDesc,
                                           TRUE,
                                           pDacl,
                                           FALSE);
    BAIL_ON_NT_STATUS(ntError);

    pDacl = NULL;

    /* We don't do SACLs currently */

    /* All done */

    ntError = RtlAbsoluteToSelfRelativeSD(pSecDesc,
                                          pSecDescRelative,
                                          pSecDescLen);
    BAIL_ON_NT_STATUS(ntError);

cleanup:
    RTL_FREE(&pDacl);
    RTL_FREE(&pSid);

    PvfsFreeAbsoluteSecurityDescriptor(&pSecDesc);

    return ntError;

error:
    goto cleanup;
}

/****************************************************************
 ***************************************************************/

static NTSTATUS
BuildDefaultDaclFile(
    PACL *ppDacl
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;
    DWORD dwSizeDacl = 0;
    PSID pAdministratorsSid = NULL;
    PSID pUsersSid = NULL;
    PSID pEveryoneSid = NULL;
    DWORD dwSidCount = 0;
    PACL pDacl = NULL;

    /* Build SIDs */

    ntError = RtlAllocateSidFromCString(&pAdministratorsSid, "S-1-5-32-544");
    BAIL_ON_NT_STATUS(ntError);
    dwSidCount++;

    ntError = RtlAllocateSidFromCString(&pUsersSid, "S-1-5-32-545");
    BAIL_ON_NT_STATUS(ntError);
    dwSidCount++;

    ntError = RtlAllocateSidFromCString(&pEveryoneSid, "S-1-1-0");
    BAIL_ON_NT_STATUS(ntError);
    dwSidCount++;

    dwSizeDacl = ACL_HEADER_SIZE +
        dwSidCount * sizeof(ACCESS_ALLOWED_ACE) +
        RtlLengthSid(pAdministratorsSid) +
        RtlLengthSid(pUsersSid) +
        RtlLengthSid(pEveryoneSid) -
        dwSidCount * sizeof(ULONG);

    ntError= RTL_ALLOCATE(&pDacl, VOID, dwSizeDacl);
    BAIL_ON_NT_STATUS(ntError);

    ntError = RtlCreateAcl(pDacl, dwSizeDacl, ACL_REVISION);
    BAIL_ON_NT_STATUS(ntError);

    ntError = RtlAddAccessAllowedAceEx(pDacl,
                                       ACL_REVISION,
                                       0,
                                       FILE_ALL_ACCESS,
                                       pAdministratorsSid);
    BAIL_ON_NT_STATUS(ntError);

    ntError = RtlAddAccessAllowedAceEx(pDacl,
                                       ACL_REVISION,
                                       0,
                                       FILE_GENERIC_WRITE | FILE_GENERIC_READ,
                                       pUsersSid);
    BAIL_ON_NT_STATUS(ntError);

    ntError = RtlAddAccessAllowedAceEx(pDacl,
                                       ACL_REVISION,
                                       0,
                                       FILE_ALL_ACCESS,
                                       pEveryoneSid);
    BAIL_ON_NT_STATUS(ntError);

    *ppDacl = pDacl;
    pDacl = NULL;
    ntError = STATUS_SUCCESS;

cleanup:
    /* Use RtlFree for SIDs */

    RTL_FREE(&pAdministratorsSid);
    RTL_FREE(&pUsersSid);
    RTL_FREE(&pEveryoneSid);

    RTL_FREE(&pDacl);

    return ntError;

error:
    goto cleanup;
}

/****************************************************************
 ***************************************************************/

static NTSTATUS
BuildDefaultDaclDirectory(
    PACL *ppDacl
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;
    DWORD dwSizeDacl = 0;
    PSID pAdministratorsSid = NULL;
    PSID pUsersSid = NULL;
    PSID pEveryoneSid = NULL;
    PSID pCreatorOwnerSid = NULL;
    DWORD dwSidCount = 0;
    PACL pDacl = NULL;

    /* Build SIDs */

    ntError = RtlAllocateSidFromCString(&pAdministratorsSid, "S-1-5-32-544");
    BAIL_ON_NT_STATUS(ntError);
    dwSidCount++;

    ntError = RtlAllocateSidFromCString(&pUsersSid, "S-1-5-32-545");
    BAIL_ON_NT_STATUS(ntError);
    dwSidCount++;

    ntError = RtlAllocateSidFromCString(&pEveryoneSid, "S-1-1-0");
    BAIL_ON_NT_STATUS(ntError);
    dwSidCount++;

    ntError = RtlAllocateSidFromCString(&pCreatorOwnerSid, "S-1-3-0");
    BAIL_ON_NT_STATUS(ntError);
    dwSidCount++;

    dwSizeDacl = ACL_HEADER_SIZE +
        dwSidCount * sizeof(ACCESS_ALLOWED_ACE) +
        RtlLengthSid(pAdministratorsSid) +
        RtlLengthSid(pUsersSid) +
        RtlLengthSid(pEveryoneSid) +
        RtlLengthSid(pCreatorOwnerSid) -
        dwSidCount * sizeof(ULONG);

    ntError= RTL_ALLOCATE(&pDacl, VOID, dwSizeDacl);
    BAIL_ON_NT_STATUS(ntError);

    ntError = RtlCreateAcl(pDacl, dwSizeDacl, ACL_REVISION);
    BAIL_ON_NT_STATUS(ntError);

    ntError = RtlAddAccessAllowedAceEx(pDacl,
                                       ACL_REVISION,
                                       OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE,
                                       FILE_ALL_ACCESS,
                                       pAdministratorsSid);
    BAIL_ON_NT_STATUS(ntError);

    ntError = RtlAddAccessAllowedAceEx(pDacl,
                                       ACL_REVISION,
                                       OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE,
                                       FILE_GENERIC_WRITE | FILE_GENERIC_READ,
                                       pUsersSid);
    BAIL_ON_NT_STATUS(ntError);

    ntError = RtlAddAccessAllowedAceEx(pDacl,
                                       ACL_REVISION,
                                       0,
                                       FILE_ALL_ACCESS,
                                       pEveryoneSid);
    BAIL_ON_NT_STATUS(ntError);

    ntError = RtlAddAccessAllowedAceEx(pDacl,
                                       ACL_REVISION,
                                       OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE | INHERIT_ONLY_ACE,
                                       FILE_ALL_ACCESS,
                                       pCreatorOwnerSid);
    BAIL_ON_NT_STATUS(ntError);

    *ppDacl = pDacl;
    pDacl = NULL;
    ntError = STATUS_SUCCESS;

cleanup:
    /* Use RtlFree for SIDs */

    RTL_FREE(&pAdministratorsSid);
    RTL_FREE(&pUsersSid);
    RTL_FREE(&pEveryoneSid);
    RTL_FREE(&pCreatorOwnerSid);

    RTL_FREE(&pDacl);

    return ntError;

error:
    goto cleanup;
}


#endif   /* UNUSED */


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/

