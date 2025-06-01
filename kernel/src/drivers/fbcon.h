#ifndef FBCON_H
#define FBCON_H

struct vt_device;

void fbcon_repaint_desktop(struct vt_device *vt_device);
void fbcon_repaint_terminal(struct vt_device *vt_device);

#endif
