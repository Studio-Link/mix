enum { PTIME = 20, SRATE = 48000, CH = 1, MAX_LEVEL = 500 };

uint64_t amix_record_msecs(void);
int amix_record_start(const char *folder);
int amix_record_close(void);
void amix_record(struct auframe *af);

struct flac;
int flac_init(struct flac **flacp, struct auframe *af, char *file);
int flac_record(struct flac *flac, struct auframe *af, uint64_t offset);
