/****************************************************************************!
*                _           _   _   _                                       *
*               | |__  _ __ / \ | |_| |__   ___ _ __   __ _                  *
*               | '_ \| '__/ _ \| __| '_ \ / _ \ '_ \ / _` |                 *
*               | |_) | | / ___ \ |_| | | |  __/ | | | (_| |                 *
*               |_.__/|_|/_/   \_\__|_| |_|\___|_| |_|\__,_|                 *
*                                                                            *
*                            www.brathena.org                                *
******************************************************************************
* src/map/searchstore.h                                                      *
******************************************************************************
* Copyright (c) brAthena Dev Team                                            *
* Copyright (c) Hercules Dev Team                                            *
* Copyright (c) Athena Dev Teams                                             *
*                                                                            *
* Licenciado sob a licen?a GNU GPL                                           *
* Para mais informa??es leia o arquivo LICENSE na ra?z do emulador           *
*****************************************************************************/

#ifndef MAP_SEARCHSTORE_H
#define MAP_SEARCHSTORE_H

#include "map/map.h" // MESSAGE_SIZE
#include "common/cbasetypes.h"
#include "common/mmo.h" // MAX_SLOTS

#include <time.h>

/**
 * Defines
 **/
#define SEARCHSTORE_RESULTS_PER_PAGE 10

/**
 * Enumerations
 **/
enum e_searchstore_searchtype {
	SEARCHTYPE_VENDING      = 0,
	SEARCHTYPE_BUYING_STORE = 1,
};

enum e_searchstore_effecttype {
	EFFECTTYPE_NORMAL = 0,
	EFFECTTYPE_CASH   = 1,
	EFFECTTYPE_MAX
};
/// failure constants for clif functions
enum e_searchstore_failure {
	SSI_FAILED_NOTHING_SEARCH_ITEM         = 0,  // "No matching stores were found."
	SSI_FAILED_OVER_MAXCOUNT               = 1,  // "There are too many results. Please enter more detailed search term."
	SSI_FAILED_SEARCH_CNT                  = 2,  // "You cannot search anymore."
	SSI_FAILED_LIMIT_SEARCH_TIME           = 3,  // "You cannot search yet."
	SSI_FAILED_SSILIST_CLICK_TO_OPEN_STORE = 4,  // "No sale (purchase) information available."
};

/**
 * Structures
 **/
/// information about the search being performed
struct s_search_store_search {
	struct map_session_data* search_sd;  // sd of the searching player
	const unsigned short* itemlist;
	const unsigned short* cardlist;
	unsigned int item_count;
	unsigned int card_count;
	unsigned int min_price;
	unsigned int max_price;
};

struct s_search_store_info_item {
	unsigned int store_id;
	int account_id;
	char store_name[MESSAGE_SIZE];
	unsigned short nameid;
	unsigned short amount;
	unsigned int price;
	short card[MAX_SLOTS];
	unsigned char refine;
};

struct s_search_store_info {
	unsigned int count;
	struct s_search_store_info_item* items;
	unsigned int pages;  // amount of pages already sent to client
	unsigned int uses;
	int remote_id;
	time_t nextquerytime;
	unsigned short effect;  // 0 = Normal (display coords), 1 = Cash (remote open store)
	unsigned char type;  // 0 = Vending, 1 = Buying Store
	bool open;
};

/// type for shop search function
typedef bool (*searchstore_search_t)(struct map_session_data* sd, unsigned short nameid);
typedef bool (*searchstore_searchall_t)(struct map_session_data* sd, const struct s_search_store_search* s);

/**
 * Interface
 **/
struct searchstore_interface {
	bool (*open) (struct map_session_data* sd, unsigned int uses, unsigned short effect);
	void (*query) (struct map_session_data* sd, unsigned char type, unsigned int min_price, unsigned int max_price, const unsigned short* itemlist, unsigned int item_count, const unsigned short* cardlist, unsigned int card_count);
	bool (*querynext) (struct map_session_data* sd);
	void (*next) (struct map_session_data* sd);
	void (*clear) (struct map_session_data* sd);
	void (*close) (struct map_session_data* sd);
	void (*click) (struct map_session_data* sd, int account_id, int store_id, unsigned short nameid);
	bool (*queryremote) (struct map_session_data* sd, int account_id);
	void (*clearremote) (struct map_session_data* sd);
	bool (*result) (struct map_session_data* sd, unsigned int store_id, int account_id, const char* store_name, unsigned short nameid, unsigned short amount, unsigned int price, const short* card, unsigned char refine);
};

struct searchstore_interface *searchstore;

#ifdef BRATHENA_CORE
void searchstore_defaults(void);
#endif // BRATHENA_CORE

#endif /* MAP_SEARCHSTORE_H */
