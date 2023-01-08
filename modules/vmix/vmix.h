/**
 * @file vidmix.h vidmix -- internal interface
 *
 * Copyright (C) 2021 Sebastian Reimers
 */


struct vidsrc_st {
	struct le le;
	struct le le2;
	struct vidisp_st *vidisp;
	double fps;
	char *device;
	vidsrc_packet_h *packeth;
	vidsrc_frame_h *frameh;
	struct vidmix_source *vidmix_src;
	void *arg;
	bool run;
	mtx_t *lock;
};


struct vidisp_st {
	struct le le;
	struct vidsrc_st *vidsrc;
	char *device;
};


extern struct hash *vmix_src;
extern struct list vmix_srcl;
extern struct hash *vmix_disp;
extern struct vidmix *vmix_mix;


int vmix_disp_alloc(struct vidisp_st **stp, const struct vidisp *vd,
			 struct vidisp_prm *prm, const char *dev,
			 vidisp_resize_h *resizeh, void *arg);
int vmix_disp_display(struct vidisp_st *st, const char *title,
			   const struct vidframe *frame, uint64_t timestamp);
struct vidisp_st *vmix_disp_find(const char *device);


int vmix_src_init(void);
void vmix_src_close(void);
int vmix_src_alloc(struct vidsrc_st **stp, const struct vidsrc *vs,
			struct vidsrc_prm *prm,
			const struct vidsz *size, const char *fmt,
			const char *dev, vidsrc_frame_h *frameh,
			vidsrc_packet_h  *packeth,
			vidsrc_error_h *errorh, void *arg);
struct vidsrc_st *vmix_src_find(const char *device);
void vmix_src_input(struct vidsrc_st *st,
			 const struct vidframe *frame, uint64_t timestamp);


int vmix_record_start(char *record_folder);
int vmix_record(const uint8_t *buf, size_t size, bool *update);
void vmix_record_close(void);


