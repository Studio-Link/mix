enum { PTIME = 20, SRATE = 48000, CH = 1, MAX_LEVEL = 500 };
void aumix_record(struct auframe *af);
void aumix_record_close(void);
int aumix_record_start(char *token);

struct flac;
int flac_init(struct flac **flacp, struct auframe *af, char *file);
int flac_record(struct flac *flac, struct auframe *af, uint64_t offset);
