#pragma once


int fsm_init();
int fsm_monitor_main(const char *path);
int fsm_monitor_fork(const char *path);
void fsm_monitor(const char *path);
