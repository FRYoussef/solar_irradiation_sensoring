#ifndef _FSM_H_
#define _FSM_H_

void fsm_init(void);
void fsm_provisioned(void);
void fsm_wifi_connected(void);
void fsm_wifi_disconnected(void);
void fsm_ntp_sync(void);
void fsm_mqtt_connected(void);
void fsm_mqtt_disconnected(void);

#endif
