/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

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
 *        defs.h
 *
 * Abstract:
 *
 *        Likewise IO (LWIO)
 *
 *        Reference Statistics Logging Module (SRV)
 *
 *        Defines
 *
 * Authors: Sriram Nambakam (snambakam@likewise.com)
 *
 */

#define LWIO_SRV_STAT_NTTIME_EPOCH_DIFFERENCE_SECS  (11644473600LL)

#define LWIO_SRV_STAT_FACTOR_MICROSECS_TO_HUNDREDS_OF_NANOSECS (10LL)

#define LWIO_SRV_STAT_FACTOR_MILLISECS_TO_HUNDREDS_OF_NANOSECS \
            (1000LL * LWIO_SRV_STAT_FACTOR_MICROSECS_TO_HUNDREDS_OF_NANOSECS)

#define LWIO_SRV_STAT_FACTOR_SECS_TO_HUNDREDS_OF_NANOSECS \
            (1000LL * LWIO_SRV_STAT_FACTOR_MILLISECS_TO_HUNDREDS_OF_NANOSECS)

#define BAIL_ON_NT_STATUS(ntStatus)                \
    if ((ntStatus)) {                              \
       goto error;                                 \
    }

#define BAIL_ON_INVALID_POINTER(p)                 \
        if (NULL == p) {                           \
           ntStatus = STATUS_INVALID_PARAMETER;    \
           BAIL_ON_NT_STATUS(ntStatus);            \
        }
