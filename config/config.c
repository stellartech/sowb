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

/* Need to come back and finish this. */

#include "sowb.h"
#include "user.h"
#include "debug.h"
#include "config.h"
#include "ff.h"

CONFIG_UNION    system_config;

bool config_block_process;
bool config_loaded;
bool config_reload;

/** config_init
 */
void config_init(void) {
    int i, j;
    
    DEBUG_INIT_START;
    
    /*
    for (i = CONFIG_FLASH_PAGE_BASE, j = 0; i < 4096; i++, j++) {
        flash_read_page(i, system_config.buffers[j], true);
    }
    
    config_loaded        = true;
    config_reload        = false;
    config_block_process = false;
    */
    
    DEBUG_INIT_END;
}

/** config_process
 */
void config_process(void) {

    /* For long period operations (e.g. config_save()) that may call
       system _process() functions, block ourselves from re-entering. */
    if (config_block_process) return;
    
}

void config_save(void) {
    int i, j;
    char buffer[FLASH_PAGE_SIZE];
    
    /* Don't re-enter this function from _process(). */
    config_block_process = true;
    
    /* Sector 15 is used as a "scratch area". It allows us to store
       pages from other sectors that we need to "restore" since the
       LPC1768 doesn't have enough space to store entire sectors. */
    while (flash_sector_erase_in_progress()) WHILE_WAITING_DO_PROCESS_FUNCTIONS;
    flash_erase_sector(15);
    while (flash_sector_erase_in_progress()) WHILE_WAITING_DO_PROCESS_FUNCTIONS;
    
    /* We need to make a copy of all the pages below our config area
       before we store our configuration. */
    for (i = 4096 - 256; i < CONFIG_FLASH_PAGE_BASE; i++) {
        flash_read_page(i, buffer, true);
        flash_page_write(3840 + i, buffer);
        while(flash_write_in_progress()) WHILE_WAITING_DO_PROCESS_FUNCTIONS;
    }

    /* Now erase the sector in which our config resides. */
    while (flash_sector_erase_in_progress()) WHILE_WAITING_DO_PROCESS_FUNCTIONS;
    flash_erase_sector(14);
    while (flash_sector_erase_in_progress()) WHILE_WAITING_DO_PROCESS_FUNCTIONS;

        
    for (i = CONFIG_FLASH_PAGE_BASE, j = 0; i < CONFIG_FLASH_PAGE_BASE + CONFIG_FLASH_PAGES; i++, j++) {
        while(flash_write_in_progress() || flash_sector_erase_in_progress()) {
            WHILE_WAITING_DO_PROCESS_FUNCTIONS;
        }
        flash_page_write(i, system_config.buffers[j]);
    }
    
    config_block_process = false;
}

/** config_copy_flash_page
 *
 * Used to copy the raw config struct, page by page
 * to an external memory buffer.
 *
 * @param int page The page to copy.
 * @param char* buffer The buffer to copy the page to.
 */
void config_copy_flash_page(int page, char *buffer) {
    memcpy(buffer, system_config.buffers[page], FLASH_PAGE_SIZE);
}

/** config_get_page
 *
 * Get the base address of a specific page of config data.
 *
 * @param int page The page to get the address of.
 * @return char* The address of the page.
 */
char * config_get_page(int page) {
    return system_config.buffers[page];
}



