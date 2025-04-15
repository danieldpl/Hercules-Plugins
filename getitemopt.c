#include "config/core.h"    // HERCULES_VERSION
#include "common/hercules.h"
#include "common/nullpo.h"
#include "common/showmsg.h"
#include "map/pc.h"          // pc->additem, script->rid2sd
#include "map/clif.h"        // clif->additem
#include "map/map.h"         // map->addflooritem
#include "map/pet.h"         // pet->create_egg
#include "map/itemdb.h"      // itemdb->isstackable, itemdb->search_name, itemdb->exists, itemdb->option_exists
#include "map/script.h"      // script_* functions

#include "plugins/HPMHooking.h"
#include "common/HPMDataCheck.h"

#include <string.h>          // Para memset()

HPExport struct hplugin_info pinfo = {
	"[getitemopt]",
	SERVER_TYPE_MAP,
	"1.0",
	HPM_VERSION,
};


BUILDIN(getitemopt)
{
	int nameid;
	struct item it;
	struct map_session_data *sd;
	struct item_data *item_data;
	struct itemdb_option *ito = NULL;
	int i;

	memset(&it, 0, sizeof(it));

	// Pegando ID do item
	if (script_isstringtype(st, 2)) {
		// Se for string ("nome do item")
		const char *name = script_getstr(st, 2);
		if ((item_data = itemdb->search_name(name)) == NULL) {
			ShowError("buildin_getitemopt: Nonexistent item %s requested.\n", name);
			script_pushint(st, 0);
			return false;
		}
		nameid = item_data->nameid;
	} else {
		// Se for número (ID do item)
		nameid = script_getnum(st, 2);
		if (nameid < 0) nameid = -nameid;
		if (nameid <= 0 || !(item_data = itemdb->exists(nameid))) {
			ShowError("buildin_getitemopt: Nonexistent item ID %d requested.\n", nameid);
			script_pushint(st, 0);
			return false;
		}
	}

	// Preenchendo dados do item
	it.nameid = nameid;
	it.identify = 1;

	sd = script->rid2sd(st); // Jogador atual
	if (sd == NULL)
		return true;

	// Itens empilháveis não podem ter opções
	if (itemdb->isstackable(nameid)) {
		ShowError("buildin_getitemopt: Cannot add options to stackable items (%d).\n", nameid);
		script_pushint(st, 0);
		return false;
	}

	// Configurando opções (até 5)
	for (i = 0; i < MAX_ITEM_OPTIONS; i++) {
		int opt_index = script_getnum(st, 3 + i * 2);    // 3,5,7,9,11
		int opt_value = script_getnum(st, 4 + i * 2);    // 4,6,8,10,12

		if (opt_index == 0)
			continue; // Se não foi passado, ignora

		// Verifica se a opção existe
		if ((ito = itemdb->option_exists(opt_index)) == NULL) {
			ShowError("buildin_getitemopt: Option index %d does not exist!\n", opt_index);
			script_pushint(st, 0);
			return false;
		}

		// Verifica se o valor da opção está no intervalo permitido
		if (opt_value < -INT16_MAX || opt_value > INT16_MAX) {
			ShowError("buildin_getitemopt: Option value %d exceeds limit (%d to %d)!\n", opt_value, -INT16_MAX, INT16_MAX);
			script_pushint(st, 0);
			return false;
		}

		// Define a opção
		it.option[i].index = opt_index;
		it.option[i].value = opt_value;
	}

	// Se não for um ovo de pet
	if (!pet->create_egg(sd, nameid)) {
		int flag = 0;
		if ((flag = pc->additem(sd, &it, 1, LOG_TYPE_SCRIPT))) {
			clif->additem(sd, 0, 0, flag);
		}
		return true;
	} else {
		ShowError("buildin_getitemopt: Item cannot be a pet egg.\n");
		script_pushint(st, 0);
		return false;
	}
}

HPExport void plugin_init(void)
{
	addScriptCommand("getitemopt", "v???????????", getitemopt);
}

HPExport void server_online(void)
{
	ShowInfo("'%s' Plugin by Danieldpl/Hercules. Version '%s'\n", pinfo.name, pinfo.version);
}