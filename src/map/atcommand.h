/****************************************************************************!
*                _           _   _   _                                       *
*               | |__  _ __ / \ | |_| |__   ___ _ __   __ _                  *
*               | '_ \| '__/ _ \| __| '_ \ / _ \ '_ \ / _` |                 *
*               | |_) | | / ___ \ |_| | | |  __/ | | | (_| |                 *
*               |_.__/|_|/_/   \_\__|_| |_|\___|_| |_|\__,_|                 *
*                                                                            *
*                            www.brathena.org                                *
******************************************************************************
* src/map/atcommand.h                                                        *
******************************************************************************
* Copyright (c) brAthena Dev Team                                            *
* Copyright (c) Hercules Dev Team                                            *
* Copyright (c) Athena Dev Teams                                             *
*                                                                            *
* Licenciado sob a licen?a GNU GPL                                           *
* Para mais informa??es leia o arquivo LICENSE na ra?z do emulador           *
*****************************************************************************/

#ifndef MAP_ATCOMMAND_H
#define MAP_ATCOMMAND_H

#include "map/pc_groups.h"
#include "common/cbasetypes.h"
#include "common/conf.h"
#include "common/db.h"

#include <stdarg.h>

/**
 * Declarations
 **/
struct map_session_data;
struct AtCommandInfo;
struct block_list;

/**
 * Defines
 **/
#define ATCOMMAND_LENGTH 50
#define MAX_MSG 4000
#define msg_txt(idx) atcommand->msg(idx)
#define msg_sd(sd,msg_number) atcommand->msgsd((sd),(msg_number))
#define msg_fd(fd,msg_number) atcommand->msgfd((fd),(msg_number))

/**
 * Enumerations
 **/
typedef enum {
	COMMAND_ATCOMMAND = 1,
	COMMAND_CHARCOMMAND = 2,
} AtCommandType;

/**
 * Typedef
 **/
typedef bool (*AtCommandFunc)(const int fd, struct map_session_data* sd, const char* command, const char* message,struct AtCommandInfo *info);
typedef struct AtCommandInfo AtCommandInfo;
typedef struct AliasInfo AliasInfo;

/**
 * Structures
 **/
struct AliasInfo {
	AtCommandInfo *command;
	char alias[ATCOMMAND_LENGTH];
};

struct AtCommandInfo {
	char command[ATCOMMAND_LENGTH];
	AtCommandFunc func;
	char *at_groups;/* quick @commands "can-use" lookup */
	char *char_groups;/* quick @charcommands "can-use" lookup */
	char *help;/* quick access to this @command's help string */
	bool log;/* whether to log this command or not, regardless of group settings */
};

struct atcmd_binding_data {
	char command[ATCOMMAND_LENGTH];
	char npc_event[ATCOMMAND_LENGTH];
	int group_lv;
	int group_lv_char;
	bool log;
};

/**
 * Interface
 **/
struct atcommand_interface {
	unsigned char at_symbol;
	unsigned char char_symbol;
	/* atcommand binding */
	struct atcmd_binding_data** binding;
	int binding_count;
	/* other vars */
	DBMap* db; //name -> AtCommandInfo
	DBMap* alias_db; //alias -> AtCommandInfo
	/**
	 * msg_table[lang_id][msg_id]
	 * Server messages (0-499 reserved for GM commands, 500-999 reserved for others)
	 **/
	char*** msg_table;
	uint8 max_message_table;
	/* */
	void (*init) (bool minimal);
	void (*final) (void);
	/* */
	bool (*exec) (const int fd, struct map_session_data *sd, const char *message, bool player_invoked);
	bool (*create) (char *name, AtCommandFunc func);
	bool (*can_use) (struct map_session_data *sd, const char *command);
	bool (*can_use2) (struct map_session_data *sd, const char *command, AtCommandType type);
	void (*load_groups) (GroupSettings **groups, config_setting_t **commands_, size_t sz);
	AtCommandInfo* (*exists) (const char* name);
	bool (*msg_read) (const char *cfg_name, bool allow_override);
	void (*final_msg) (void);
	/* atcommand binding */
	struct atcmd_binding_data* (*get_bind_byname) (const char* name);
	/* */
	AtCommandInfo* (*get_info_byname) (const char *name); // @help
	const char* (*check_alias) (const char *aliasname); // @help
	void (*get_suggestions) (struct map_session_data* sd, const char *name, bool is_atcmd_cmd); // @help
	void (*config_read) (const char* config_filename);
	/* command-specific subs */
	int (*stopattack) (struct block_list *bl,va_list ap);
	int (*pvpoff_sub) (struct block_list *bl,va_list ap);
	int (*pvpon_sub) (struct block_list *bl,va_list ap);
	int (*atkillmonster_sub) (struct block_list *bl, va_list ap);
	void (*raise_sub) (struct map_session_data* sd);
	void (*get_jail_time) (int jailtime, int* year, int* month, int* day, int* hour, int* minute);
	int (*cleanfloor_sub) (struct block_list *bl, va_list ap);
	int (*mutearea_sub) (struct block_list *bl,va_list ap);
	void (*getring) (struct map_session_data* sd);
	void (*channel_help) (int fd, const char *command, bool can_create);
	/* */
	void (*commands_sub) (struct map_session_data* sd, const int fd, AtCommandType type);
	void (*cmd_db_clear) (void);
	int (*cmd_db_clear_sub) (DBKey key, DBData *data, va_list args);
	void (*doload) (void);
	void (*base_commands) (void);
	bool (*add) (char *name, AtCommandFunc func, bool replace);
	const char* (*msg) (int msg_number);
	void (*expand_message_table) (void);
	const char* (*msgfd) (int fd, int msg_number);
	const char* (*msgsd) (struct map_session_data *sd, int msg_number);
};

struct atcommand_interface *atcommand;

#ifdef BRATHENA_CORE
void atcommand_defaults(void);
#endif // BRATHENA_CORE

/* stay here */
#define ACMD(x) static bool atcommand_ ## x (const int fd, struct map_session_data* sd, const char* command, const char* message, struct AtCommandInfo *info) __attribute__((nonnull (2, 3, 4, 5))); \
    static bool atcommand_ ## x (const int fd, struct map_session_data* sd, const char* command, const char* message, struct AtCommandInfo *info)

#endif /* MAP_ATCOMMAND_H */
