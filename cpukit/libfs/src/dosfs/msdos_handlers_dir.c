/*
 *  Directory Handlers Table for MSDOS filesystem
 *
 *  Copyright (C) 2001 OKTET Ltd., St.-Petersburg, Russia
 *  Author: Eugeny S. Mints <Eugeny.Mints@oktet.ru>
 *
 *  The license and distribution terms for this file may be
 *  found in the file LICENSE in this distribution or at
 *  http://www.rtems.com/license/LICENSE.
 *
 *  @(#) $Id$
 */

#if HAVE_CONFIG_H
#include "config.h"
#endif

#include <rtems/libio.h>
#include "msdos.h"

const rtems_filesystem_file_handlers_r msdos_dir_handlers = {
    msdos_dir_open,
    msdos_dir_close,
    msdos_dir_read,
    rtems_filesystem_default_write,
    rtems_filesystem_default_ioctl,
    msdos_dir_lseek,
    msdos_dir_stat,
    msdos_dir_chmod,
    rtems_filesystem_default_ftruncate,
    rtems_filesystem_default_fpathconf,
    msdos_dir_sync,
    msdos_dir_sync,
    rtems_filesystem_default_fcntl,
    msdos_dir_rmnod
};
