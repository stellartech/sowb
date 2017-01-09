/****************************************************************************
 *    Copyright 2010 Andy Kirkham, Stellar Technologies Ltd
 *    
 *    This file is part of the Satellite Observers Workbench (SOWB).
 *
 *    SOWB is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation, either version 3 of the License, or
 *    (at your option) any later version.
 *
 *    SOWB is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with SOWB.  If not, see <http://www.gnu.org/licenses/>.
 *
 *    $Id: main.cpp 5 2010-07-12 20:51:11Z ajk $
 *    
 ***************************************************************************/

#ifndef CONFIG_H
#define CONFIG_H

#include "flash.h"

#define CONFIG_MAX_SIZE         4096
#define CONFIG_FLASH_PAGES      CONFIG_MAX_SIZE / FLASH_PAGE_SIZE
#define CONFIG_FLASH_PAGE_BASE  3840 - CONFIG_FLASH_PAGES

typedef struct _config_values {
    int config_struct_version;
} CONFIG_VALUES;

typedef union _config_union {
    char buffers[CONFIG_FLASH_PAGES][FLASH_PAGE_SIZE];
    CONFIG_VALUES   values;
} CONFIG_UNION;

void config_init(void);
void config_process(void);
void config_copy_flash_page(int page, char *buffer);
char * config_get_page(int page);

#endif
