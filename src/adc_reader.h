#ifndef ADC_READER_H
#define ADC_READER_H

int adc_reader_setup(void);
void adc_reader_update(char *topic, char *data);
int adc_reader_stop_send_timers(void);
int adc_reader_start_send_timers(void);

#endif
