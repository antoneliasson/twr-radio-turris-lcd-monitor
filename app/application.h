#ifndef _APPLICATION_H
#define _APPLICATION_H

#ifndef VERSION
#define VERSION "vdev"
#endif

#include <bcl.h>

void qrcode_project(char *project_name);
void get_qr_data();
void bc_change_qr_value(uint64_t *id, const char *topic, void *value, void *param);
void bc_get_system_info(uint64_t *id, const char *topic, void *value, void *param);
void bc_get_network_info(uint64_t *id, const char *topic, void *value, void *param);


#endif // _APPLICATION_H
