/****************************************************************************!
*                _           _   _   _                                       *
*               | |__  _ __ / \ | |_| |__   ___ _ __   __ _                  *
*               | '_ \| '__/ _ \| __| '_ \ / _ \ '_ \ / _` |                 *
*               | |_) | | / ___ \ |_| | | |  __/ | | | (_| |                 *
*               |_.__/|_|/_/   \_\__|_| |_|\___|_| |_|\__,_|                 *
*                                                                            *
*                            www.brathena.org                                *
******************************************************************************
* src/map/storage.h                                                          *
******************************************************************************
* Copyright (c) brAthena Dev Team                                            *
* Copyright (c) Hercules Dev Team                                            *
* Copyright (c) Athena Dev Teams                                             *
*                                                                            *
* Licenciado sob a licen?a GNU GPL                                           *
* Para mais informa??es leia o arquivo LICENSE na ra?z do emulador           *
*****************************************************************************/

#ifndef MAP_STORAGE_H
#define MAP_STORAGE_H

#include "common/cbasetypes.h"
#include "common/db.h"

struct guild_storage;
struct item;
struct map_session_data;

/**
 * Acceptable values for map_session_data.state.storage_flag
 */
enum storage_flag {
	STORAGE_FLAG_CLOSED = 0, // Closed
	STORAGE_FLAG_NORMAL = 1, // Normal Storage open
	STORAGE_FLAG_GUILD  = 2, // Guild Storage open
};

struct storage_interface {
	/* */
	void (*reconnect) (void);
	/* */
	int (*delitem) (struct map_session_data* sd, int n, int amount);
	int (*open) (struct map_session_data *sd);
	int (*add) (struct map_session_data *sd,int index,int amount);
	int (*get) (struct map_session_data *sd,int index,int amount);
	int (*additem) (struct map_session_data* sd, struct item* item_data, int amount);
	int (*addfromcart) (struct map_session_data *sd,int index,int amount);
	int (*gettocart) (struct map_session_data *sd,int index,int amount);
	void (*close) (struct map_session_data *sd);
	void (*pc_quit) (struct map_session_data *sd, int flag);
	int (*comp_item) (const void *i1_, const void *i2_);
	void (*sortitem) (struct item* items, unsigned int size);
	int (*reconnect_sub) (DBKey key, DBData *data, va_list ap);
};

struct guild_storage_interface {
	struct DBMap* db; // int guild_id -> struct guild_storage*
	/* */
	struct guild_storage *(*ensure) (int guild_id);
	/* */
	void (*init) (bool minimal);
	void (*final) (void);
	/* */
	int (*delete) (int guild_id);
	int (*open) (struct map_session_data *sd);
	int (*additem) (struct map_session_data *sd,struct guild_storage *stor,struct item *item_data,int amount);
	int (*delitem) (struct map_session_data *sd,struct guild_storage *stor,int n,int amount);
	int (*add) (struct map_session_data *sd,int index,int amount);
	int (*get) (struct map_session_data *sd,int index,int amount);
	int (*addfromcart) (struct map_session_data *sd,int index,int amount);
	int (*gettocart) (struct map_session_data *sd,int index,int amount);
	int (*close) (struct map_session_data *sd);
	int (*pc_quit) (struct map_session_data *sd,int flag);
	int (*save) (int account_id, int guild_id, int flag);
	int (*saved) (int guild_id); //Ack from char server that guild store was saved.
	DBData (*create) (DBKey key, va_list args);
};

#ifdef BRATHENA_CORE
void storage_defaults(void);
void gstorage_defaults(void);
#endif // BRATHENA_CORE

struct storage_interface *storage;
struct guild_storage_interface *gstorage;

#endif /* MAP_STORAGE_H */
