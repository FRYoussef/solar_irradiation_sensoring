// define number of ADCs to read, and its indices
#define N_ADC 3 // ADC channels used
#define N_ADC_MEASURES 2 // number of ADCs used for own measures (different
                        //channels could be used to calculate the same measure)

#define IRRADIATION_ADC_INDEX 0
#define BATTERY_ADC_INDEX 1
#define BIAS_ADC_INDEX 2 // used for irradiation, always keep it with the highest index

struct adc_reading
{
    int value;
};