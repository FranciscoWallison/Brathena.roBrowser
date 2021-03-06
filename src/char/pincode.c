/****************************************************************************!
*                _           _   _   _                                       *
*               | |__  _ __ / \ | |_| |__   ___ _ __   __ _                  *
*               | '_ \| '__/ _ \| __| '_ \ / _ \ '_ \ / _` |                 *
*               | |_) | | / ___ \ |_| | | |  __/ | | | (_| |                 *
*               |_.__/|_|/_/   \_\__|_| |_|\___|_| |_|\__,_|                 *
*                                                                            *
*                            www.brathena.org                                *
******************************************************************************
* src/char/pincode.c                                                         *
******************************************************************************
* Copyright (c) brAthena Dev Team                                            *
* Copyright (c) Hercules Dev Team                                            *
* Copyright (c) Athena Dev Teams                                             *
*                                                                            *
* Licenciado sob a licenca GNU GPL                                           *
* Para mais informa??es leia o arquivo LICENSE na ra?z do emulador           *
*****************************************************************************/

#define BRATHENA_CORE

#include "pincode.h"

#include "char/char.h"
#include "common/cbasetypes.h"
#include "common/db.h"
#include "common/mmo.h"
#include "common/nullpo.h"
#include "common/random.h"
#include "common/showmsg.h"
#include "common/socket.h"
#include "common/strlib.h"

#include <stdlib.h>

struct pincode_interface pincode_s;
struct pincode_interface *pincode;

void pincode_handle (int fd, struct char_session_data* sd) {
	struct online_char_data* character;

	nullpo_retv(sd);
	character = (struct online_char_data*)idb_get(chr->online_char_db, sd->account_id);
	if( character && character->pincode_enable > pincode->charselect
		// Caso esteja habilitada a configura??o de verifica??o dos dias que a senha
		// N?o tenha sido alterada, verifica o pincode do jogador, caso esteja ativa e dentro do prazo
		// N?o verifica o pincode e deixa tudo ok. [CarlosHenrq]
		&& (pincode->enabled_lastpass == 0 || (pincode->enabled_lastpass > 0 && pincode->enabled_lastpass < sd->pincode_lastpass))){
		character->pincode_enable = pincode->charselect * 2;
	}else{
		pincode->sendstate( fd, sd, PINCODE_OK );
		return;
	}

	if( strlen(sd->pincode) == 4 ){
		if( pincode->changetime && time(NULL) > (sd->pincode_change+pincode->changetime) ){ // User hasn't changed his PIN code for a long time
			pincode->sendstate( fd, sd, PINCODE_EXPIRED );
		} else { // Ask user for his PIN code
			pincode->sendstate( fd, sd, PINCODE_ASK );
		}
	} else // No PIN code has been set yet
		pincode->sendstate( fd, sd, PINCODE_NOTSET );

	if( character )
		character->pincode_enable = -1;
}

void pincode_check(int fd, struct char_session_data* sd) {
	char pin[5] = "\0\0\0\0";

	nullpo_retv(sd);
	safestrncpy(pin, (char*)RFIFOP(fd, 6), sizeof(pin));
	pincode->decrypt(sd->pincode_seed, pin);
	if( pincode->compare( fd, sd, pin ) ){
		struct online_char_data* character;
		if( (character = (struct online_char_data*)idb_get(chr->online_char_db, sd->account_id)) )
			character->pincode_enable = pincode->charselect * 2;
		pincode->sendstate( fd, sd, PINCODE_OK );
	}
}

int pincode_compare(int fd, struct char_session_data* sd, char* pin) {
	nullpo_ret(sd);
	nullpo_ret(pin);
	if( strcmp( sd->pincode, pin ) == 0 ){
		sd->pincode_try = 0;
		return 1;
	} else {
		pincode->sendstate( fd, sd, PINCODE_WRONG );
		if( pincode->maxtry && ++sd->pincode_try >= pincode->maxtry ){
			pincode->error( sd->account_id );
		}
		return 0;
	}
}

void pincode_change(int fd, struct char_session_data* sd) {
	char oldpin[5] = "\0\0\0\0", newpin[5] = "\0\0\0\0";

	nullpo_retv(sd);
	safestrncpy(oldpin, (char*)RFIFOP(fd,6), sizeof(oldpin));
	pincode->decrypt(sd->pincode_seed,oldpin);
	if( !pincode->compare( fd, sd, oldpin ) )
		return;

	safestrncpy(newpin, (char*)RFIFOP(fd,10), sizeof(newpin));
	pincode->decrypt(sd->pincode_seed,newpin);
	pincode->update( sd->account_id, newpin );
	safestrncpy(sd->pincode, newpin, sizeof(sd->pincode));
	pincode->sendstate( fd, sd, PINCODE_ASK );
}

void pincode_setnew(int fd, struct char_session_data* sd) {
	char newpin[5] = "\0\0\0\0";

	nullpo_retv(sd);
	safestrncpy(newpin, (char*)RFIFOP(fd,6), sizeof(newpin));
	pincode->decrypt(sd->pincode_seed,newpin);
	pincode->update( sd->account_id, newpin );
	safestrncpy(sd->pincode, newpin, sizeof(sd->pincode));
	pincode->sendstate( fd, sd, PINCODE_ASK );
}

// 0 = pin is correct
// 1 = ask for pin - client sends 0x8b8
// 2 = create new pin - client sends 0x8ba
// 3 = pin must be changed - client 0x8be
// 4 = create new pin ?? - client sends 0x8ba
// 5 = client shows msgstr(1896)
// 6 = client shows msgstr(1897) Unable to use your KSSN number
// 7 = char select window shows a button - client sends 0x8c5
// 8 = pincode was incorrect
void pincode_sendstate(int fd, struct char_session_data* sd, uint16 state) {
	nullpo_retv(sd);
	WFIFOHEAD(fd, 12);
	WFIFOW(fd, 0) = 0x8b9;
	WFIFOL(fd, 2) = sd->pincode_seed = rnd() % 0xFFFF;
	WFIFOL(fd, 6) = sd->account_id;
	WFIFOW(fd,10) = state;
	WFIFOSET(fd,12);
}

void pincode_notifyLoginPinUpdate(int account_id, char* pin) {
	nullpo_retv(pin);
	Assert_retv(chr->login_fd != -1);
	WFIFOHEAD(chr->login_fd,11);
	WFIFOW(chr->login_fd,0) = 0x2738;
	WFIFOL(chr->login_fd,2) = account_id;
	safestrncpy( (char*)WFIFOP(chr->login_fd,6), pin, 5 );
	WFIFOSET(chr->login_fd,11);
}

void pincode_notifyLoginPinError(int account_id) {
	WFIFOHEAD(chr->login_fd,6);
	WFIFOW(chr->login_fd,0) = 0x2739;
	WFIFOL(chr->login_fd,2) = account_id;
	WFIFOSET(chr->login_fd,6);
}

void pincode_decrypt(unsigned int userSeed, char* pin) {
	int i;
	char tab[10] = {0,1,2,3,4,5,6,7,8,9};

	nullpo_retv(pin);
	for (i = 1; i < 10; i++) {
		int pos;
		userSeed = pincode->baseSeed + userSeed * pincode->multiplier;
		pos = userSeed % (i + 1);
		if( i != pos ){
			tab[i] ^= tab[pos];
			tab[pos] ^= tab[i];
			tab[i] ^= tab[pos];
		}
	}

	for( i = 0; i < 4; i++ ){
		pin[i] = tab[pin[i] - '0'];
	}

	sprintf(pin, "%d%d%d%d", pin[0], pin[1], pin[2], pin[3]);
}

bool pincode_config_read(char *w1, char *w2) {

	nullpo_ret(w1);
	nullpo_ret(w2);
	while ( true ) {
		if ( strcmpi(w1, "pincode_enabled") == 0 ) {
			pincode->enabled = atoi(w2);
#if PACKETVER < 20110309
			if( pincode->enabled ) {
				ShowWarning("pincode_enabled: Requer PACKETVER 20110309 ou superior. Desabilitando...\n");
				pincode->enabled = 0;
			}
#endif
		} else if ( strcmpi(w1, "pincode_changetime") == 0 ) {
			pincode->changetime = atoi(w2)*60;
		} else if ( strcmpi(w1, "pincode_maxtry") == 0 ) {
			pincode->maxtry = atoi(w2);
			if( pincode->maxtry > 3 ) {
				ShowWarning("pincode_maxtry muito grande (%d); maximo permitido: 3! setando para 3...\n", pincode->maxtry);
				pincode->maxtry = 3;
			}
		} else if ( strcmpi(w1, "pincode_charselect") == 0 ) {
			pincode->charselect = atoi(w2);
		}
		// Verifica se ir? utilizar o n?mero de dias para verificar o pincode. [CarlosHenrq]
		else if(!strcmpi(w1, "pincode_lastpass"))
		{
			pincode->enabled_lastpass = atoi(w2);
		} else {
			return false;
		}
		break;
	}

	return true;
}

void pincode_defaults(void) {
	pincode = &pincode_s;

	pincode->enabled = PINCODE_OK;
	pincode->changetime = 0;
	pincode->maxtry = 3;
	pincode->charselect = 0;
	pincode->multiplier = 0x3498;
	pincode->baseSeed = 0x881234;

	pincode->handle = pincode_handle;
	pincode->decrypt = pincode_decrypt;
	pincode->error = pincode_notifyLoginPinError;
	pincode->update = pincode_notifyLoginPinUpdate;
	pincode->sendstate = pincode_sendstate;
	pincode->setnew = pincode_setnew;
	pincode->change = pincode_change;
	pincode->compare = pincode_compare;
	pincode->check = pincode_check;
	pincode->config_read = pincode_config_read;

}
