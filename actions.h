/* actions.h -- Actions executed on each event.
 *
 * This file is part of fsnoop.
 *
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <vladz@devzero.fr> wrote this file.  As long as you retain this notice
 * you can do whatever you want with this stuff. If we meet some day, and
 * you think this stuff is worth it, you can buy me a beer in return.
 */

#ifndef _FSNOOP_ACTIONS_H
#define _FSNOOP_ACTIONS_H

extern void action_recursive(void);

extern void action_openfd(void);

extern void action_ropenfd(void);

extern void action_normal_output(void);

extern void action_kill(void);

extern void action_payload(void);

extern void action_payload_proc(void);

#endif
