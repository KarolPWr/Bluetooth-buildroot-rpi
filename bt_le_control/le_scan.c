#include <stdio.h>
#include <errno.h>
#include <ctype.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
// #include <getopt.h>
// #include <sys/param.h>
// #include <sys/ioctl.h>
#include <sys/socket.h>
#include <signal.h>

// #include "bluetooth.h"
// #include "lib/hci.h"
// #include "lib/hci_lib.h"

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>

static volatile int signal_received = 0;

static struct option lescan_options[] = {
	{ "help",	0, 0, 'h' },
	{ "static",	0, 0, 's' },
	{ "privacy",	0, 0, 'p' },
	{ "passive",	0, 0, 'P' },
	{ "whitelist",	0, 0, 'w' },
	{ "discovery",	1, 0, 'd' },
	{ "duplicates",	0, 0, 'D' },
	{ 0, 0, 0, 0 }
};

static const char *lescan_help =
	"Usage:\n"
	"\tlescan [--privacy] enable privacy\n"
	"\tlescan [--passive] set scan type passive (default active)\n"
	"\tlescan [--whitelist] scan for address in the whitelist only\n"
	"\tlescan [--discovery=g|l] enable general or limited discovery"
		"procedure\n"
	"\tlescan [--duplicates] don't filter duplicates\n";

static void sigint_handler(int sig)
{
	signal_received = sig;
}

static int print_advertising_devices(int dd, uint8_t filter_type)
{
	unsigned char buf[HCI_MAX_EVENT_SIZE], *ptr;
	struct hci_filter nf, of;
	struct sigaction sa;
	socklen_t olen;
	int len;

	olen = sizeof(of);
	if (getsockopt(dd, SOL_HCI, HCI_FILTER, &of, &olen) < 0) {
		printf("Could not get socket options\n");
		return -1;
	}

	hci_filter_clear(&nf);
	hci_filter_set_ptype(HCI_EVENT_PKT, &nf);
	hci_filter_set_event(EVT_LE_META_EVENT, &nf);

	if (setsockopt(dd, SOL_HCI, HCI_FILTER, &nf, sizeof(nf)) < 0) {
		printf("Could not set socket options\n");
		return -1;
	}

	memset(&sa, 0, sizeof(sa));
	sa.sa_flags = SA_NOCLDSTOP;
	sa.sa_handler = sigint_handler;
	sigaction(SIGINT, &sa, NULL);

	while (1) {
		evt_le_meta_event *meta;
		le_advertising_info *info;
		char addr[18];

		while ((len = read(dd, buf, sizeof(buf))) < 0) {
			if (errno == EINTR && signal_received == SIGINT) {
				len = 0;
				goto done;
			}

			if (errno == EAGAIN || errno == EINTR)
				continue;
			goto done;
		}

		ptr = buf + (1 + HCI_EVENT_HDR_SIZE);
		len -= (1 + HCI_EVENT_HDR_SIZE);

		meta = (void *) ptr;

		if (meta->subevent != 0x02)
			goto done;

		/* Ignoring multiple reports */
		info = (le_advertising_info *) (meta->data + 1);
		if (check_report_filter(filter_type, info)) {
			char name[30];

			memset(name, 0, sizeof(name));

			ba2str(&info->bdaddr, addr);
			eir_parse_name(info->data, info->length,
							name, sizeof(name) - 1);

			printf("%s %s\n", addr, name);
		}
	}

done:
	setsockopt(dd, SOL_HCI, HCI_FILTER, &of, sizeof(of));

	if (len < 0)
		return -1;

	return 0;
}


static void cmd_lescan(int dev_id, int argc, char **argv)
{
	int err, opt, dd;
	uint8_t own_type = LE_PUBLIC_ADDRESS;
	uint8_t scan_type = 0x01;
	uint8_t filter_type = 0;
	uint8_t filter_policy = 0x00;
	uint16_t interval = htobs(0x0010);
	uint16_t window = htobs(0x0010);
	uint8_t filter_dup = 0x01;

	for_each_opt(opt, lescan_options, NULL) {
		switch (opt) {
		case 's':
			own_type = LE_RANDOM_ADDRESS;
			break;
		case 'p':
			own_type = LE_RANDOM_ADDRESS;
			break;
		case 'P':
			scan_type = 0x00; /* Passive */
			break;
		case 'w':
			filter_policy = 0x01; /* Whitelist */
			break;
		case 'd':
			filter_type = optarg[0];
			if (filter_type != 'g' && filter_type != 'l') {
				fprintf(stderr, "Unknown discovery procedure\n");
				exit(1);
			}

			interval = htobs(0x0012);
			window = htobs(0x0012);
			break;
		case 'D':
			filter_dup = 0x00;
			break;
		default:
			printf("%s", lescan_help);
			return;
		}
	}
	helper_arg(0, 1, &argc, &argv, lescan_help);

	if (dev_id < 0)
		dev_id = hci_get_route(NULL);

	dd = hci_open_dev(dev_id);
	if (dd < 0) {
		perror("Could not open device");
		exit(1);
	}

	err = hci_le_set_scan_parameters(dd, scan_type, interval, window,
						own_type, filter_policy, 10000);
	if (err < 0) {
		perror("Set scan parameters failed");
		exit(1);
	}

	err = hci_le_set_scan_enable(dd, 0x01, filter_dup, 10000);
	if (err < 0) {
		perror("Enable scan failed");
		exit(1);
	}

	printf("LE Scan ...\n");

	err = print_advertising_devices(dd, filter_type);
	if (err < 0) {
		perror("Could not receive advertising events");
		exit(1);
	}

	err = hci_le_set_scan_enable(dd, 0x00, filter_dup, 10000);
	if (err < 0) {
		perror("Disable scan failed");
		exit(1);
	}

	hci_close_dev(dd);
}


int main(int argc, char *argv[])
{

    printf("LE Scan start\n");
	int opt, i, dev_id = -1;
	bdaddr_t ba;

	if (argc < 1) {
		usage();
		exit(0);
	}

	if (dev_id != -1 && hci_devba(dev_id, &ba) < 0) {
		perror("Device is not available");
		exit(1);
	}

	return 0;
}

