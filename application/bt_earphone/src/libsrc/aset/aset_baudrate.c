/**
 *  ***************************************************************************
 *  Copyright (c) 2003-2021 Actions Semiconductor. All rights reserved.
 *
 *  \file       aset_baudrate.c
 *  \brief      ASET baudrate config
 *  \author     zhouxl
 *  \version    1.00
 *  \date       2021-3-26
 *  ***************************************************************************
 *
 *  \History:
 *  Version 1.00
 *       Initial release
 */


#include "stub_inner.h"



bool SetASETBaudrate(usp_handle_t *aset_op, uint32_t baudrate)
{
    return SetUSPBaudrate(aset_op, baudrate);
}

