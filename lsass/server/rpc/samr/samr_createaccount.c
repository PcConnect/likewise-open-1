#include "includes.h"


static
DWORD
SamrSrvBuildHomedirPath(
    PCWSTR  pwszSamAccountName,
    PCWSTR  pwszDomainName,
    PWSTR  *ppHomedirPath
    );


NTSTATUS
SamrSrvCreateAccount(
    /* [in] */ handle_t hBinding,
    /* [in] */ DOMAIN_HANDLE hDomain,
    /* [in] */ UnicodeStringEx *account_name,
    /* [in  */ DWORD dwObjectClass,
    /* [in] */ UINT32 account_flags,
    /* [in] */ UINT32 access_mask,
    /* [out] */ ACCOUNT_HANDLE *hAccount,
    /* [out] */ UINT32 *access_granted,
    /* [out] */ UINT32 *rid
    )
{
    const wchar_t wszAccountDnFmt[] = L"CN=%ws,%ws";
    NTSTATUS ntStatus = STATUS_SUCCESS;
    DWORD dwError = 0;
    PDOMAIN_CONTEXT pDomCtx = NULL;
    PACCOUNT_CONTEXT pAccCtx = NULL;
    HANDLE hDirectory = NULL;
    WCHAR wszAttrObjectClass[] = DS_ATTR_OBJECT_CLASS;
    WCHAR wszAttrSamAccountName[] = DS_ATTR_SAM_ACCOUNT_NAME;
    WCHAR wszAttrCommonName[] = DS_ATTR_COMMON_NAME;
    WCHAR wszAttrAccountFlags[] = DS_ATTR_ACCOUNT_FLAGS;
    WCHAR wszAttrNetBIOSName[] = DS_ATTR_NETBIOS_NAME;
    WCHAR wszAttrHomedir[] = DS_ATTR_HOME_DIR;
    WCHAR wszAttrShell[] = DS_ATTR_SHELL;

    enum AttrValueIndex {
        ATTR_VAL_IDX_OBJECT_CLASS = 0,
        ATTR_VAL_IDX_SAM_ACCOUNT_NAME,
        ATTR_VAL_IDX_COMMON_NAME,
        ATTR_VAL_IDX_ACCOUNT_FLAGS,
        ATTR_VAL_IDX_NETBIOS_NAME,
        ATTR_VAL_IDX_HOME_DIR,
        ATTR_VAL_IDX_SHELL,
        ATTR_VAL_IDX_SENTINEL
    };

    ATTRIBUTE_VALUE AttrValues[] = {
        {   /* ATTR_VAL_IDX_OBJECT_CLASS */
            .Type = DIRECTORY_ATTR_TYPE_INTEGER,
            .data.ulValue = 0
        },
        {   /* ATTR_VAL_IDX_SAM_ACCOUNT_NAME */
            .Type = DIRECTORY_ATTR_TYPE_UNICODE_STRING,
            .data.pwszStringValue = NULL
        },
        {   /* ATTR_VAL_IDX_COMMON_NAME */
            .Type = DIRECTORY_ATTR_TYPE_UNICODE_STRING,
            .data.pwszStringValue = NULL
        },
        {   /* ATTR_VAL_IDX_ACCOUNT_FLAGS */
            .Type = DIRECTORY_ATTR_TYPE_INTEGER,
            .data.ulValue = 0
        },
        {   /* ATTR_VAL_IDX_NETBIOS_NAME */
            .Type = DIRECTORY_ATTR_TYPE_UNICODE_STRING,
            .data.pwszStringValue = NULL
        },
        {   /* ATTR_VAL_IDX_HOME_DIR */
            .Type = DIRECTORY_ATTR_TYPE_UNICODE_STRING,
            .data.pszStringValue = NULL
        },
        {   /* ATTR_VAL_IDX_SHELL */
            .Type = DIRECTORY_ATTR_TYPE_ANSI_STRING,
            .data.pszStringValue = NULL
        }
    };

    DIRECTORY_MOD ModObjectClass = {
        DIR_MOD_FLAGS_ADD,
        wszAttrObjectClass,
        1,
        &AttrValues[ATTR_VAL_IDX_OBJECT_CLASS]
    };

    DIRECTORY_MOD ModSamAccountName = {
        DIR_MOD_FLAGS_ADD,
        wszAttrSamAccountName,
        1,
        &AttrValues[ATTR_VAL_IDX_SAM_ACCOUNT_NAME]
    };

    DIRECTORY_MOD ModCommonName = {
        DIR_MOD_FLAGS_ADD,
        wszAttrCommonName,
        1,
        &AttrValues[ATTR_VAL_IDX_COMMON_NAME]
    };

    DIRECTORY_MOD ModAccountFlags = {
        DIR_MOD_FLAGS_ADD,
        wszAttrAccountFlags,
        1,
        &AttrValues[ATTR_VAL_IDX_ACCOUNT_FLAGS]
    };

    DIRECTORY_MOD ModNetBIOSName = {
        DIR_MOD_FLAGS_ADD,
        wszAttrNetBIOSName,
        1,
        &AttrValues[ATTR_VAL_IDX_NETBIOS_NAME]
    };

    DIRECTORY_MOD ModHomeDir = {
        DIR_MOD_FLAGS_ADD,
        wszAttrHomedir,
        1,
        &AttrValues[ATTR_VAL_IDX_HOME_DIR]
    };

    DIRECTORY_MOD ModShell = {
        DIR_MOD_FLAGS_ADD,
        wszAttrShell,
        1,
        &AttrValues[ATTR_VAL_IDX_SHELL]
    };

    DIRECTORY_MOD Mods[ATTR_VAL_IDX_SENTINEL + 1];

    PWSTR pwszAccountName = NULL;
    PWSTR pwszName = NULL;
    PWSTR pwszParentDn = NULL;
    PWSTR pwszAccountDn = NULL;
    PSTR pszShell = NULL;
    PWSTR pwszHomedirPath = NULL;
    DWORD dwAccountDnLen = 0;
    size_t sCommonNameLen = 0;
    size_t sParentDnLen = 0;
    DWORD i = 0;
    UnicodeString AccountName = {0};
    Ids Rids = {0};
    Ids Types = {0};
    DWORD dwRid = 0;
    DWORD dwAccountType = 0;

    memset(&Mods, 0, sizeof(Mods));

    pDomCtx = (PDOMAIN_CONTEXT)hDomain;

    if (pDomCtx == NULL || pDomCtx->Type != SamrContextDomain) {
        ntStatus = STATUS_INVALID_HANDLE;
        BAIL_ON_NTSTATUS_ERROR(ntStatus);
    }

    hDirectory   = pDomCtx->pConnCtx->hDirectory;
    pwszParentDn = pDomCtx->pwszDn;

    ntStatus = SamrSrvGetFromUnicodeStringEx(&pwszAccountName,
                                             account_name);
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

    dwError = LwAllocateWc16String(&pwszName,
                                   pwszAccountName);
    BAIL_ON_LSA_ERROR(dwError);

    /*
     * Check if such account name already exists.
     */

    ntStatus = SamrSrvInitUnicodeString(&AccountName,
                                        pwszAccountName);
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

    ntStatus = SamrSrvLookupNames(hBinding,
                                  hDomain,
                                  1,
                                  &AccountName,
                                  &Rids,
                                  &Types);
    if (ntStatus == STATUS_SUCCESS)
    {
        /* Account already exists - return error code */
        ntStatus = STATUS_USER_EXISTS;
    }
    else if (ntStatus == STATUS_NONE_MAPPED)
    {
        /* Account doesn't exists - proceed with creating it */
        ntStatus = STATUS_SUCCESS;
    }
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

    if (Rids.ids)
    {
        SamrSrvFreeMemory(Rids.ids);
    }

    if (Types.ids)
    {
        SamrSrvFreeMemory(Types.ids);
    }

    SamrSrvFreeUnicodeString(&AccountName);


    dwError = LwWc16sLen(pwszAccountName, &sCommonNameLen);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwWc16sLen(pwszParentDn, &sParentDnLen);
    BAIL_ON_LSA_ERROR(dwError);

    dwAccountDnLen = sCommonNameLen +
                     sParentDnLen +
                     (sizeof(wszAccountDnFmt)/sizeof(wszAccountDnFmt[0]));

    dwError = LwAllocateMemory(sizeof(WCHAR) * dwAccountDnLen,
                               OUT_PPVOID(&pwszAccountDn));
    BAIL_ON_LSA_ERROR(dwError);

    sw16printfw(pwszAccountDn, dwAccountDnLen, wszAccountDnFmt,
                pwszAccountName,
                pwszParentDn);

    dwError = SamrSrvConfigGetDefaultLoginShell(&pszShell);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = SamrSrvBuildHomedirPath(pwszAccountName,
                                      pDomCtx->pwszDomainName,
                                      &pwszHomedirPath);
    BAIL_ON_LSA_ERROR(dwError);

    AttrValues[ATTR_VAL_IDX_OBJECT_CLASS].data.ulValue
        = dwObjectClass;
    AttrValues[ATTR_VAL_IDX_SAM_ACCOUNT_NAME].data.pwszStringValue
        = pwszAccountName;
    AttrValues[ATTR_VAL_IDX_COMMON_NAME].data.pwszStringValue
        = pwszAccountName;
    AttrValues[ATTR_VAL_IDX_ACCOUNT_FLAGS].data.ulValue
        = (account_flags | ACB_DISABLED);
    AttrValues[ATTR_VAL_IDX_NETBIOS_NAME].data.pwszStringValue
        = pDomCtx->pwszDomainName;
    AttrValues[ATTR_VAL_IDX_HOME_DIR].data.pwszStringValue
        = pwszHomedirPath;
    AttrValues[ATTR_VAL_IDX_SHELL].data.pszStringValue
        = pszShell;

    Mods[i++] = ModObjectClass;
    Mods[i++] = ModSamAccountName;
    Mods[i++] = ModCommonName;
    Mods[i++] = ModNetBIOSName;

    if (dwObjectClass == DS_OBJECT_CLASS_USER)
    {
        Mods[i++] = ModAccountFlags;
        Mods[i++] = ModHomeDir;
        Mods[i++] = ModShell;
    }

    dwError = DirectoryAddObject(hDirectory,
                                 pwszAccountDn,
                                 Mods);
    BAIL_ON_LSA_ERROR(dwError);

    ntStatus = SamrSrvInitUnicodeString(&AccountName,
                                        pwszAccountName);
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

    ntStatus = SamrSrvLookupNames(hBinding,
                                  hDomain,
                                  1,
                                  &AccountName,
                                  &Rids,
                                  &Types);
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

    dwRid         = Rids.ids[0];
    dwAccountType = Types.ids[0];

    dwError = LwAllocateMemory(sizeof(*pAccCtx),
                               (void**)&pAccCtx);
    BAIL_ON_LSA_ERROR(dwError);

    pAccCtx->Type          = SamrContextAccount;
    pAccCtx->refcount      = 1;
    pAccCtx->pwszDn        = pwszAccountDn;
    pAccCtx->pwszName      = pwszName;
    pAccCtx->dwRid         = dwRid;
    pAccCtx->dwAccountType = dwAccountType;
    pAccCtx->pDomCtx       = pDomCtx;

    /* Increase ref count because DCE/RPC runtime is about to use this
       pointer as well */
    InterlockedIncrement(&pAccCtx->refcount);

    *hAccount = (ACCOUNT_HANDLE)pAccCtx;
    *rid      = dwRid;

    /* TODO: this has be changed accordingly when security descriptors
       are enabled */
    *access_granted = MAXIMUM_ALLOWED;

cleanup:
    SamrSrvFreeUnicodeString(&AccountName);

    if (pwszAccountName)
    {
        SamrSrvFreeMemory(pwszAccountName);
    }

    if (Rids.ids)
    {
        SamrSrvFreeMemory(Rids.ids);
    }

    if (Types.ids)
    {
        SamrSrvFreeMemory(Types.ids);
    }

    if (ntStatus == STATUS_SUCCESS &&
        dwError != ERROR_SUCCESS)
    {
        ntStatus = LwWin32ErrorToNtStatus(dwError);
    }

    return ntStatus;

error:
    LW_SAFE_FREE_MEMORY(pAccCtx);

    *hAccount       = NULL;
    *access_granted = 0;
    *rid            = 0;
    goto cleanup;
}


static
DWORD
SamrSrvBuildHomedirPath(
    PCWSTR pwszSamAccountName,
    PCWSTR pwszDomainName,
    PWSTR *ppwszHomedirPath
    )
{
    DWORD dwError = ERROR_SUCCESS;
    PWSTR pwszSamAccount = NULL;
    PWSTR pwszDomain = NULL;
    PSTR pszHomedirTemplate = NULL;
    PWSTR pwszHomedirTemplate = NULL;
    size_t sHomedirTemplateLen = 0;
    PWSTR pwszTemplateCursor = NULL;
    PSTR pszHomedirPrefix = NULL;
    PWSTR pwszHomedirPrefix = NULL;
    size_t sHomedirPrefixLen = 0;
    PSTR pszHostName = NULL;
    PWSTR pwszHostName = NULL;
    size_t sHostNameLen = 0;
    size_t sSamAccountNameLen = 0;
    size_t sDomainNameLen = 0;
    PWSTR pwszHomedirPath = NULL;
    DWORD dwHomedirPathLenAllowed = 0;
    DWORD dwOffset = 0;
    DWORD dwLenRemaining = 0;
    PWSTR pwszInsert = NULL;
    size_t sInsertLen = 0;

    dwError = LwAllocateWc16String(&pwszSamAccount,
                                   pwszSamAccountName);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwAllocateWc16String(&pwszDomain,
                                   pwszDomainName);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = SamrSrvConfigGetHomedirTemplate(&pszHomedirTemplate);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwMbsToWc16s(pszHomedirTemplate,
                           &pwszHomedirTemplate);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwWc16sLen(pwszHomedirTemplate,
                         &sHomedirTemplateLen);
    BAIL_ON_LSA_ERROR(dwError);

    if (strstr(pszHomedirTemplate, "%H"))
    {
        dwError = SamrSrvConfigGetHomedirPrefix(&pszHomedirPrefix);
        BAIL_ON_LSA_ERROR(dwError);

        if (pszHomedirPrefix)
        {
            dwError = LwMbsToWc16s(pszHomedirPrefix,
                                   &pwszHomedirPrefix);
            BAIL_ON_LSA_ERROR(dwError);

            dwError = LwWc16sLen(pwszHomedirPrefix,
                                 &sHomedirPrefixLen);
            BAIL_ON_LSA_ERROR(dwError);
        }
    }

    if (strstr(pszHomedirTemplate, "%L"))
    {
        dwError = LsaDnsGetHostInfo(&pszHostName);
        BAIL_ON_LSA_ERROR(dwError);

        if (pszHostName)
        {
            dwError = LwMbsToWc16s(pszHostName,
                                   &pwszHostName);
            BAIL_ON_LSA_ERROR(dwError);

            dwError = LwWc16sLen(pwszHostName,
                                 &sHostNameLen);
            BAIL_ON_LSA_ERROR(dwError);
        }
    }

    dwError = LwWc16sLen(pwszSamAccount,
                         &sSamAccountNameLen);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwWc16sLen(pwszDomain,
                         &sDomainNameLen);
    BAIL_ON_LSA_ERROR(dwError);

    dwHomedirPathLenAllowed = (sHomedirTemplateLen +
                               sHomedirPrefixLen +
                               sHostNameLen +
                               sSamAccountNameLen +
                               sDomainNameLen +
                               1);

    dwError = LwAllocateMemory(sizeof(WCHAR) * dwHomedirPathLenAllowed,
                               OUT_PPVOID(&pwszHomedirPath));
    BAIL_ON_LSA_ERROR(dwError);

    pwszTemplateCursor = pwszHomedirTemplate;
    dwLenRemaining = dwHomedirPathLenAllowed - dwOffset - 1;

    while (pwszTemplateCursor[0] &&
           dwLenRemaining > 0)
    {
        if (pwszTemplateCursor[0] == (WCHAR)('%'))
        {
            switch (pwszTemplateCursor[1])
            {
            case (WCHAR)('D'):
                pwszInsert = pwszDomain;
                sInsertLen = sDomainNameLen;

                dwError = LwWc16sToUpper(pwszInsert);
                BAIL_ON_LSA_ERROR(dwError);
                break;

            case (WCHAR)('U'):
                pwszInsert = pwszSamAccount;
                sInsertLen = sSamAccountNameLen;

                dwError = LwWc16sToLower(pwszInsert);
                BAIL_ON_LSA_ERROR(dwError);
                break;

            case (WCHAR)('H'):
                pwszInsert = pwszHomedirPrefix;
                sInsertLen = sHomedirPrefixLen;
                break;

            case (WCHAR)('L'):
                pwszInsert = pwszHostName;
                sInsertLen = sHostNameLen;

            default:
                dwError = LW_ERROR_INVALID_HOMEDIR_TEMPLATE;
                BAIL_ON_LSA_ERROR(dwError);
            }

            pwszTemplateCursor += 2;
        }
        else
        {
            PCWSTR pwszEnd = pwszTemplateCursor;
            while (pwszEnd[0] &&
                   pwszEnd[0] != (WCHAR)('%'))
            {
                pwszEnd++;
            }

            if (!pwszEnd)
            {
                dwError = LwWc16sLen(pwszTemplateCursor,
                                     &sInsertLen);
                BAIL_ON_LSA_ERROR(dwError);
            }
            else
            {
                sInsertLen = pwszEnd - pwszTemplateCursor;
            }

            pwszInsert = pwszTemplateCursor;
            pwszTemplateCursor += sInsertLen;
        }

        memcpy(pwszHomedirPath + dwOffset,
               pwszInsert,
               sizeof(WCHAR) * sInsertLen);

        dwOffset += sInsertLen;
        dwLenRemaining = dwHomedirPathLenAllowed - dwOffset - 1;
    }

    LSA_ASSERT(dwOffset < dwHomedirPathLenAllowed);

    pwszHomedirPath[dwOffset] = 0;
    dwOffset++;

    *ppwszHomedirPath = pwszHomedirPath;

cleanup:
    LW_SAFE_FREE_MEMORY(pwszSamAccount);
    LW_SAFE_FREE_MEMORY(pwszDomain);
    LW_SAFE_FREE_MEMORY(pszHomedirTemplate);
    LW_SAFE_FREE_MEMORY(pwszHomedirTemplate);
    LW_SAFE_FREE_MEMORY(pszHomedirPrefix);
    LW_SAFE_FREE_MEMORY(pwszHomedirPrefix);
    LW_SAFE_FREE_MEMORY(pszHostName);
    LW_SAFE_FREE_MEMORY(pwszHostName);

    return dwError;

error:
    LW_SAFE_FREE_MEMORY(pwszHomedirPath);

    *ppwszHomedirPath = NULL;
    goto cleanup;
}


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
