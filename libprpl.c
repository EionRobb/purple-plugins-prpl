/*
 *   Plugin loader protocol plugin for libpurple
 *   Copyright (C) 2019  Eion Robb
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef __GNUC__
#include <unistd.h>
#endif
#include <errno.h>

#include <purple.h>

#ifdef ENABLE_NLS
#      define GETTEXT_PACKAGE "purple-plugins"
#      include <glib/gi18n-lib.h>
#	ifdef _WIN32
#		ifdef LOCALEDIR
#			unset LOCALEDIR
#		endif
#		define LOCALEDIR  wpurple_locale_dir()
#	endif
#else
#      define _(a) (a)
#      define N_(a) (a)
#endif

#define PLUGIN_ID "prpl-eionrobb-plugins-as-a-prpl-wat"
#ifndef PLUGIN_VERSION
#define PLUGIN_VERSION "0.1"
#endif
#define PLUGIN_WEBSITE "https://github.com/EionRobb/"


void
plugins_login(PurpleAccount *account)
{
	GList *plugins;
	PurpleGroup *group = purple_find_group("Plugins");
	PurpleConnection *pc = purple_account_get_connection(account);

	if (!group) {
		group = purple_group_new("Plugins");
		purple_blist_add_group(group, NULL);
	}
	
	for (plugins = purple_plugins_get_all(); plugins; plugins = plugins->next) {
		PurplePlugin *plugin = plugins->data;
		PurplePluginInfo *info = plugin->info;
		
		//Bitlbee already loads protocol plugins
		if (info->type == PURPLE_PLUGIN_PROTOCOL)
			continue;
		
		PurpleBuddy *buddy = purple_buddy_new(account, info->id, info->name);
		purple_blist_add_buddy(buddy, NULL, group, NULL);
		
		if (purple_plugin_is_loaded(plugin)) {
			purple_prpl_got_user_status(account, info->id, "loaded", NULL);
		}
	}
	
	purple_plugins_load_saved("PluginAsAPrpl");
	
	purple_connection_set_state(pc, PURPLE_CONNECTED);
}

static void
plugins_close(PurpleConnection *pc)
{
	purple_plugins_save_loaded("PluginAsAPrpl");
}

static int
plugins_send_im(PurpleConnection *pc,
#if PURPLE_VERSION_CHECK(3, 0, 0)
				PurpleMessage *msg)
{
	const gchar *who = purple_message_get_recipient(msg);
	const gchar *message = purple_message_get_contents(msg);
#else
				const gchar *who, const gchar *message, PurpleMessageFlags flags)
{
#endif

	PurplePlugin *plugin = purple_plugins_find_with_id(who);
	
	if (plugin == NULL)
		return -1;

	if (purple_strequal(message, "load")) {
		purple_plugin_load(plugin);
		
	} else if (purple_strequal(message, "unload")) {
		purple_plugin_unload(plugin);
		
	} else if (purple_strequal(message, "info")) {
		//todo
		
	}

	return 1;
}


static GList *
plugins_status_types(PurpleAccount *account)
{
	GList *types = NULL;
	PurpleStatusType *status;
	
	status = purple_status_type_new_full(PURPLE_STATUS_AVAILABLE, "loaded", _("Online"), TRUE, TRUE, FALSE);
	types = g_list_append(types, status);

	status = purple_status_type_new_full(PURPLE_STATUS_OFFLINE, "unloaded", _("Offline"), TRUE, TRUE, FALSE);
	types = g_list_append(types, status);

	return types;
}

static const char *
plugins_list_icon(PurpleAccount *account, PurpleBuddy *buddy)
{
	return "wat";
}

static gboolean
plugin_load(PurplePlugin *plugin, GError **error)
{
	return TRUE;
}

static gboolean
plugin_unload(PurplePlugin *plugin, GError **error)
{
	purple_signals_disconnect_by_handle(plugin);

	return TRUE;
}

/* Purple2 Plugin Load Functions */
#if !PURPLE_VERSION_CHECK(3, 0, 0)
static gboolean
libpurple2_plugin_load(PurplePlugin *plugin)
{
	return plugin_load(plugin, NULL);
}

static gboolean
libpurple2_plugin_unload(PurplePlugin *plugin)
{
	return plugin_unload(plugin, NULL);
}

static void
plugin_init(PurplePlugin *plugin)
{
	PurplePluginInfo *info;
	PurplePluginProtocolInfo *prpl_info = g_new0(PurplePluginProtocolInfo, 1);

	info = plugin->info;

	if (info == NULL) {
		plugin->info = info = g_new0(PurplePluginInfo, 1);
	}

	info->extra_info = prpl_info;
#if PURPLE_MINOR_VERSION >= 5
	prpl_info->struct_size = sizeof(PurplePluginProtocolInfo);
#endif

	prpl_info->options = OPT_PROTO_NO_PASSWORD;
	prpl_info->list_icon = plugins_list_icon;
	prpl_info->status_types = plugins_status_types;
	prpl_info->login = plugins_login;
	prpl_info->close = plugins_close;
	prpl_info->send_im = plugins_send_im;
}

static PurplePluginInfo info = {
	PURPLE_PLUGIN_MAGIC,
	/*	PURPLE_MAJOR_VERSION,
		PURPLE_MINOR_VERSION,
	*/
	2, 1,
	PURPLE_PLUGIN_PROTOCOL,			/* type */
	NULL,							/* ui_requirement */
	0,								/* flags */
	NULL,							/* dependencies */
	PURPLE_PRIORITY_DEFAULT,		/* priority */
	PLUGIN_ID,						/* id */
	"Purple Plugins Loader Prpl",	/* name */
	PLUGIN_VERSION,					/* version */
	"",								/* summary */
	"",								/* description */
	"Eion Robb <eion@robbmob.com>", /* author */
	PLUGIN_WEBSITE,			/* homepage */
	libpurple2_plugin_load,			/* load */
	libpurple2_plugin_unload,		/* unload */
	NULL,							/* destroy */
	NULL,							/* ui_info */
	NULL,							/* extra_info */
	NULL,							/* prefs_info */
	NULL,							/* actions */
	NULL,							/* padding */
	NULL,
	NULL,
	NULL
};

PURPLE_INIT_PLUGIN(plugins, plugin_init, info);

#else
/* Purple 3 plugin load functions */

G_MODULE_EXPORT GType plugins_protocol_get_type(void);
#define PLUGINS_TYPE_PROTOCOL (plugins_protocol_get_type())
#define PLUGINS_PROTOCOL(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj), PLUGINS_TYPE_PROTOCOL, PluginsProtocol))
#define PLUGINS_PROTOCOL_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST((klass), PLUGINS_TYPE_PROTOCOL, PluginsProtocolClass))
#define PLUGINS_IS_PROTOCOL(obj) (G_TYPE_CHECK_INSTANCE_TYPE((obj), PLUGINS_TYPE_PROTOCOL))
#define PLUGINS_IS_PROTOCOL_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass), PLUGINS_TYPE_PROTOCOL))
#define PLUGINS_PROTOCOL_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS((obj), PLUGINS_TYPE_PROTOCOL, PluginsProtocolClass))

typedef struct _PluginsProtocol {
	PurpleProtocol parent;
} PluginsProtocol;

typedef struct _PluginsProtocolClass {
	PurpleProtocolClass parent_class;
} PluginsProtocolClass;

static void
plugins_protocol_init(PurpleProtocol *prpl_info)
{
	PurpleProtocol *info = prpl_info;

	info->id = PLUGIN_ID;
	info->name = "Purple Plugins Loader Prpl";
}

static void
plugins_protocol_class_init(PurpleProtocolClass *prpl_info)
{
	prpl_info->login = plugins_login;
	prpl_info->close = plugins_close;
	prpl_info->status_types = plugins_status_types;
	prpl_info->list_icon = plugins_list_icon;
}

static void
plugins_protocol_im_iface_init(PurpleProtocolIMIface *prpl_info)
{
	prpl_info->send = plugins_send_im;
}

static PurpleProtocol *plugins_protocol;

PURPLE_DEFINE_TYPE_EXTENDED(
	PluginsProtocol, plugins_protocol, PURPLE_TYPE_PROTOCOL, 0,

	PURPLE_IMPLEMENT_INTERFACE_STATIC(PURPLE_TYPE_PROTOCOL_IM_IFACE,
									  plugins_protocol_im_iface_init)

);

static gboolean
libpurple3_plugin_load(PurplePlugin *plugin, GError **error)
{
	plugins_protocol_register_type(plugin);
	plugins_protocol = purple_protocols_add(PLUGINS_TYPE_PROTOCOL, error);

	if (!plugins_protocol) {
		return FALSE;
	}

	return plugin_load(plugin, error);
}

static gboolean
libpurple3_plugin_unload(PurplePlugin *plugin, GError **error)
{
	if (!plugin_unload(plugin, error)) {
		return FALSE;
	}

	if (!purple_protocols_remove(plugins_protocol, error)) {
		return FALSE;
	}

	return TRUE;
}

static PurplePluginInfo *
plugin_query(GError **error)
{
#ifdef ENABLE_NLS
	bindtextdomain(GETTEXT_PACKAGE, LOCALEDIR);
	bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
#endif
	
	return purple_plugin_info_new(
	  "id", PLUGIN_ID,
	  "name", "Purple Plugins Loader Prpl",
	  "version", PLUGIN_VERSION,
	  "category", _("Protocol"),
	  "summary", "",
	  "description", "",
	  "website", PLUGIN_WEBSITE,
	  "abi-version", PURPLE_ABI_VERSION,
	  "flags", PURPLE_PLUGIN_INFO_FLAGS_INTERNAL |
				 PURPLE_PLUGIN_INFO_FLAGS_AUTO_LOAD,
	  NULL);
}

PURPLE_PLUGIN_INIT(plugins, plugin_query, libpurple3_plugin_load, libpurple3_plugin_unload);

#endif
