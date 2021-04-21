// define number of ADCs to read, and its indices
#define N_ADC 3 // ADC channels used
#define N_ADC_MEASURES 2 // number of ADCs used for own measures (different
                        //channels could be used to calculate the same measure)

#define IRRADIATION_ADC_INDEX 0
#define BATTERY_ADC_INDEX 1
#define BIAS_ADC_INDEX 2 // used for irradiation, always keep it with the highest index

// MQTT topics
#define TOPIC_IRRADIATION "/ciu/lopy4/irradiation/1"
#define TOPIC_BATTERY_LEVEL "/ciu/lopy4/battery_level/1"
                                                    // /location/board name/sensor metric/sensor number/config parameter  
static const char * TOPIC_SAMPLE_FREQ_IRRADIATION = "/ciu/lopy4/irradiation/1/sample_frequency";
static const char * TOPIC_SEND_FREQ_IRRADIATION = "/ciu/lopy4/irradiation/1/send_frequency";
static const char * TOPIC_N_SAMPLES_IRRADIATION = "/ciu/lopy4/irradiation/1/sample_number";

static const char * TOPIC_SAMPLE_FREQ_BATTERY_LEVEL = "/ciu/lopy4/battery_level/1/sample_frequency";
static const char * TOPIC_SEND_FREQ_BATTERY_LEVEL = "/ciu/lopy4/battery_level/1/send_frequency";
static const char * TOPIC_N_SAMPLES_BATTERY_LEVEL = "/ciu/lopy4/battery_level/1/sample_number";