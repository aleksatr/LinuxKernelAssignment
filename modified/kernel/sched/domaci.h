#ifndef _DOMACI_H
#define _DOMACI_H_H

#include <uapi/asm-generic/param.h>

#define PROCESS_BUFF_LEN 50
#define DOMACI_SLICE HZ/200

void domaci_update_statistics(struct task_struct *tsk);

#endif
