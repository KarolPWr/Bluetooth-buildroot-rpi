#define _GNU_SOURCE
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/uio.h>
#include <fcntl.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>

#include <glib.h>

// #include "src/shared/util.h"
// #include "src/shared/queue.h"
// #include "src/shared/io.h"
// #include "src/shared/shell.h"
#include "gdbus/gdbus.h"
// #include "gatt.h"

#define APP_PATH "/org/bluez/app"
#define DEVICE_INTERFACE "org.bluez.Device1"
#define PROFILE_INTERFACE "org.bluez.GattProfile1"
#define SERVICE_INTERFACE "org.bluez.GattService1"
#define CHRC_INTERFACE "org.bluez.GattCharacteristic1"
#define DESC_INTERFACE "org.bluez.GattDescriptor1"

/* String display constants */
#define COLORED_NEW	COLOR_GREEN "NEW" COLOR_OFF
#define COLORED_CHG	COLOR_YELLOW "CHG" COLOR_OFF
#define COLORED_DEL	COLOR_RED "DEL" COLOR_OFF

#define MAX_ATTR_VAL_LEN	512

struct desc {
	struct chrc *chrc;
	char *path;
	uint16_t handle;
	char *uuid;
	char **flags;
	size_t value_len;
	unsigned int max_val_len;
	uint8_t *value;
};

struct chrc {
	struct service *service;
	GDBusProxy *proxy;
	char *path;
	uint16_t handle;
	char *uuid;
	char **flags;
	bool notifying;
	GList *descs;
	size_t value_len;
	unsigned int max_val_len;
	uint8_t *value;
	uint16_t mtu;
	struct io *write_io;
	struct io *notify_io;
	bool authorization_req;
};

struct service {
	DBusConnection *conn;
	GDBusProxy *proxy;
	char *path;
	uint16_t handle;
	char *uuid;
	bool primary;
	GList *chrcs;
	GList *inc;
};

static GList *local_services;
static GList *services;
static GList *characteristics;
static GList *descriptors;
static GList *managers;
static GList *uuids;
static DBusMessage *pending_message = NULL;

struct sock_io {
	GDBusProxy *proxy;
	struct io *io;
	uint16_t mtu;
};

static struct sock_io write_io;
static struct sock_io notify_io;

static void print_service(struct service *service, const char *description)
{
	const char *text;

	text = bt_uuidstr_to_str(service->uuid);
	if (!text)
		printf("%s%s%s%s Service (Handle 0x%04x)\n\t%s\n\t"
					"%s\n",
					description ? "[" : "",
					description ? : "",
					description ? "] " : "",
					service->primary ? "Primary" :
					"Secondary",
					service->handle, service->path,
					service->uuid);
	else
		printf("%s%s%s%s Service (Handle 0x%04x)\n\t%s\n\t%s"
					"\n\t%s\n",
					description ? "[" : "",
					description ? : "",
					description ? "] " : "",
					service->primary ? "Primary" :
					"Secondary",
					service->handle, service->path,
					service->uuid, text);
}

static void print_service_proxy(GDBusProxy *proxy, const char *description)
{
	struct service service;
	DBusMessageIter iter;
	const char *uuid;
	dbus_bool_t primary;

	if (g_dbus_proxy_get_property(proxy, "UUID", &iter) == FALSE)
		return;

	dbus_message_iter_get_basic(&iter, &uuid);

	if (g_dbus_proxy_get_property(proxy, "Primary", &iter) == FALSE)
		return;

	dbus_message_iter_get_basic(&iter, &primary);

	service.path = (char *) g_dbus_proxy_get_path(proxy);
	service.uuid = (char *) uuid;
	service.primary = primary;

	print_service(&service, description);
}



static void list_attributes(const char *path, GList *source)
{
	GList *l;

	for (l = source; l; l = g_list_next(l)) {
		GDBusProxy *proxy = l->data;
		const char *proxy_path;

		proxy_path = g_dbus_proxy_get_path(proxy);

		if (!g_str_has_prefix(proxy_path, path))
			continue;

		if (source == services) {
			print_service_proxy(proxy, NULL);
			list_attributes(proxy_path, characteristics);
		} else if (source == characteristics) {
			print_characteristic(proxy, NULL);
			list_attributes(proxy_path, descriptors);
		} else if (source == descriptors)
			print_descriptor(proxy, NULL);
	}
}




