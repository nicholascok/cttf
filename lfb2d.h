#ifndef __CRD_LINUX_FRAMEBUFFER_GRAPHICS_2D_H__
#define __CRD_LINUX_FRAMEBUFFER_GRAPHICS_2D_H__

void fb_draw_point(rgbx32 _col, int x, int y) {
	if (y >= 2 && x >= 2 && y < fb_vinfo.yres - 2 && x < fb_vinfo.xres - 2) {
		sbuf[y - 2][x - 1] = _col;
		sbuf[y - 2][x - 0] = _col;
		sbuf[y - 2][x + 1] = _col;
		
		sbuf[y - 1][x - 2] = _col;
		sbuf[y - 1][x - 1] = _col;
		sbuf[y - 1][x - 0] = _col;
		sbuf[y - 1][x + 1] = _col;
		sbuf[y - 1][x + 2] = _col;
		
		sbuf[y - 0][x - 2] = _col;
		sbuf[y - 0][x - 1] = _col;
		sbuf[y - 0][x - 0] = _col;
		sbuf[y - 0][x + 1] = _col;
		sbuf[y - 0][x + 2] = _col;
		
		sbuf[y + 2][x - 1] = _col;
		sbuf[y + 2][x - 0] = _col;
		sbuf[y + 2][x + 1] = _col;
		
		sbuf[y + 1][x - 2] = _col;
		sbuf[y + 1][x - 1] = _col;
		sbuf[y + 1][x - 0] = _col;
		sbuf[y + 1][x + 1] = _col;
		sbuf[y + 1][x + 2] = _col;
	}
}

void fb_draw_centroid(rgbx32 _col, int x, int y) {
	if (y >= 2 && x >= 2 && y < fb_vinfo.yres - 2 && x < fb_vinfo.xres - 2) {
		sbuf[y - 2][x - 1] = (rgbx32) {255, 255, 255, 255};
		sbuf[y - 2][x - 0] = (rgbx32) {255, 255, 255, 255};
		sbuf[y - 2][x + 1] = (rgbx32) {255, 255, 255, 255};
		
		sbuf[y - 1][x - 2] = (rgbx32) {255, 255, 255, 255};
		sbuf[y - 1][x - 1] = _col;
		sbuf[y - 1][x - 0] = _col;
		sbuf[y - 1][x + 1] = _col;
		sbuf[y - 1][x + 2] = (rgbx32) {255, 255, 255, 255};
		
		sbuf[y - 0][x - 2] = (rgbx32) {255, 255, 255, 255};
		sbuf[y - 0][x - 1] = _col;
		sbuf[y - 0][x - 0] = _col;
		sbuf[y - 0][x + 1] = _col;
		sbuf[y - 0][x + 2] = (rgbx32) {255, 255, 255, 255};
		
		sbuf[y + 2][x - 1] = (rgbx32) {255, 255, 255, 255};
		sbuf[y + 2][x - 0] = (rgbx32) {255, 255, 255, 255};
		sbuf[y + 2][x + 1] = (rgbx32) {255, 255, 255, 255};
		
		sbuf[y + 1][x - 2] = (rgbx32) {255, 255, 255, 255};
		sbuf[y + 1][x - 1] = _col;
		sbuf[y + 1][x - 0] = _col;
		sbuf[y + 1][x + 1] = _col;
		sbuf[y + 1][x + 2] = (rgbx32) {255, 255, 255, 255};
	}
}

void fb_draw_line(rgbx32 _col, int x1, int y1, int x2, int y2) {
	int dx = abs(x2 - x1);
	int dy = -abs(y2 - y1);
	int sx = x1 < x2 ? 1 : -1;
	int sy = y1 < y2 ? 1 : -1;
	int er = dx + dy;
	int e2;
	
	while (x1 != x2 || y1 != y2) {
		if (y1 >= 0 && x1 >= 0 && y1 < fb_vinfo.yres && x1 < fb_vinfo.xres)
			sbuf[y1][x1] = _col;
		e2 = 2 * er;
		if (e2 >= dy) er += dy, x1 += sx;
		if (e2 <= dx) er += dx, y1 += sy;
	}
}

#endif