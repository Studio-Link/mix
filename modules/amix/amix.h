enum { PTIME = 20, SRATE = 48000, CH = 1, MAX_LEVEL = 500 };
void amix_record(struct auframe *af);
void amix_record_close(void);
int amix_record_start(char *token, bool audio_only);

struct flac;
int flac_init(struct flac **flacp, struct auframe *af, char *file);
int flac_record(struct flac *flac, struct auframe *af, uint64_t offset);
