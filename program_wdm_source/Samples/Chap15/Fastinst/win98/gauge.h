// gauge.h : interface to progress gauge control

#ifndef GAUGE_H
#define GAUGE_H

void ResetGauge(HWND hwnd, WORD full);
void SetGauge(HWND hwnd, WORD pos);
BOOL RegisterGauge();

#endif
