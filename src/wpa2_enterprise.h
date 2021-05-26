#ifndef _WPA2_ENTERPRISE_H
#define _WPA2_ENTERPRISE_H
/* obtained using the xxd unix command from the original files
 *  xxd -i wpa2_ca.pem wpa2_ca_pem.c
 */
extern unsigned char wpa2_ca_pem[];
extern int wpa2_ca_pem_len;

/* obtained using the xxd unix command from the original files
 *  xxd -i wpa2_client.crt wpa2_client_crt.c
 */
extern unsigned char wpa2_client_crt[];
extern int wpa2_client_crt_len;

/* obtained using the xxd unix command from the original files
 *  xxd -i wpa2_client.key wpa2_client_key.c
 */
extern unsigned char wpa2_client_key[];
extern int wpa2_client_key_len;

#endif
