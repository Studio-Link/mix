enum { PTIME = 20, SRATE = 48000, CH = 2, MAX_LEVEL = 500 };
int aumix_record(const uint8_t *buf, size_t size);
void aumix_record_close(void);
int aumix_record_start(char *token);
