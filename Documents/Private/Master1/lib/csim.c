/*
 * Component simulator main header
 * Copyright (c) 2019, IRIT - UPS <casse@irit.fr>
 *
 * This file is part of GLISS2.
 *
 * GLISS2 is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * GLISS2 is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with GLISS2; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "csim.h"
#include "mem.h"

#define CSIM_IO_HASH(a)	(((csim_word_t)(a) >> CSIM_IO_SHIFT) & (CSIM_IO_SIZE - 1))

/**
 * Default log function: log to stderr.
 * @param board	Current board.
 * @param level	Logging level.
 * @param msg	Message to display.
 */
void csim_log(csim_board_t *board, csim_level_t level, const char *msg, ...) {
	static const char *pref[] = {
		"",
		"DEBUG  ",
		"INFO   ",
		"WARNING",
		"ERROR  ",
		"FATAL  "
	};
	va_list args;
	va_start(args, msg);
	fprintf(stderr, "%s: % 9ld: ", pref[level], board->date);
	vfprintf(stderr, msg, args);
	fputc('\n', stderr);
	va_end(args);
}


/* unit management */
struct {
	const char *name;
	csim_port_type_t type;
} csim_units[256] = {
	{ "volt", CSIM_ELECTRIC }
};
int csim_unit_top = 0;


/**
 * Get identifier for a new or existing unit based on its name.
 * @param name	Unit name.
 * @return		Corresponding unit type.
 */
csim_port_type_t csim_get_unit(const char *name) {
	for(int i = 0; i <= csim_unit_top; i++)
		if(strcmp(name, csim_units[i].name) == 0)
			return csim_units[i].type;
	csim_unit_top++;
	csim_units[csim_unit_top].name = name;
	csim_units[csim_unit_top].type = csim_unit_top;
	return csim_unit_top;
}


/**
 * Get the name of a unit type.
 * @param t		Type to get name for.
 * @return		Type name.
 */
const char *csim_unit_name(csim_port_type_t t) {
	assert(t <= csim_unit_top);
	return csim_units[t].name;
}


/**
 * Called to manage an IO.
 * @param addr		Accessed address.
 * @param size		Accessed size.
 * @param data		Pointer to data.
 * @param access	Type of access.
 * @param cdata		Should be board.
 */
static void csim_on_io(csim_addr_t addr, int size, void *data, int access, void *cdata) {
	csim_board_t *board = (csim_board_t *)cdata;
	int h = CSIM_IO_HASH(addr);
	for(csim_io_t *p = board->ios[h]; p != NULL; p = p->next)
		if(p->addr == addr) {
			int i = (addr - p->inst->base - p->reg->offset) / p->reg->stride;
			if(size != p->reg->size)
				board->log(board, CSIM_ERROR, "bad IO access at %08x: size=%d and should be %d", addr, size, p->reg->size);
			else
				switch(p->reg->size) {
					
				case 1:
					if(access == CSIM_MEM_READ)
						*(int8_t *)data = p->reg->read(p->inst, i);
					else
						p->reg->write(p->inst, i, *(int8_t *)data);
					break;
				
				case 2:
					if(access == CSIM_MEM_READ)
						*(int16_t *)data = p->reg->read(p->inst, i);
					else
						p->reg->write(p->inst, i, *(int16_t *)data);
					break;
				
				case 4:
					if(access == CSIM_MEM_READ)
						*(int32_t *)data = p->reg->read(p->inst, i);
					else
						p->reg->write(p->inst, i, *(int32_t *)data);
					break;
				
				default:
					assert(0);
				}
		}
}


/**
 * Add IO entry for the given registers in the given instance.
 * @param reg	Register to record.
 * @param inst	Component instance.
 */
void csim_io_add(csim_reg_t *reg, csim_inst_t *inst) {
	csim_board_t *board = inst->board;
	for(int i = 0; i < reg->count; i++) {
		
		/* build the IO entry */
		csim_addr_t a = inst->base + i * reg->stride + reg->offset;
		int h = CSIM_IO_HASH(a);
		csim_io_t *io = (csim_io_t *)malloc(sizeof(csim_io_t));
		io->addr = a;
		io->reg = reg;
		io->inst = inst;
		io->next = board->ios[h];
		board->ios[h] = io;
		
		/* install the IO in memory */
		csim_addr_t pa = a & ~(CSIM_PAGE_SIZE - 1);
		if(csim_get_callback_data(board->mem, pa) == NULL)
			csim_set_range_callback(board->mem, pa, pa + reg->size - 1, csim_on_io, board);
	}
}


/**
 * Remove IO entries for the given register.
 * @param reg	Register to record.
 * @param inst	Component instance.
 */
void csim_io_remove(csim_reg_t * reg, csim_inst_t *inst) {
	csim_board_t *board = inst->board;
	for(int i = 0; i < reg->count; i++) {
		csim_addr_t a = inst->base + i * reg->stride + reg->offset;
		int h = CSIM_IO_HASH(a);
		for(csim_io_t *p = board->ios[h], *q = NULL; p != NULL; q = p, p = p->next)
			if(p->addr == a) {
				if(q == NULL)
					board->ios[h] = p->next;
				else
					q->next = p->next;
				free(p);
			}
		assert(0);
	}
}


/**
 * Initialize the board.
 * @param name	Board name.
 * @param mem	Memory to use.
 * @return		Built board (or null if allocation fails).
 */
csim_board_t *csim_new_board(const char *name, csim_memory_t *mem) {
	csim_board_t *board = (csim_board_t *)malloc(sizeof(csim_board_t));
	if(board == NULL)
		return NULL;
	board->name = name;
	board->insts = NULL;
	board->cores = NULL;
	board->clock = 0;
	board->date = 0;
	board->evts = 0;
	board->level = CSIM_INFO;
	board->mem = mem;
	board->log = csim_log;
	memset(board->ios, 0, sizeof(csim_io_t *) * CSIM_IO_SIZE);
	return board;
}

/**
 * Delete the given board.
 * @param board		Board to delete.
 */
void csim_delete_board(csim_board_t *board) {
	
	csim_inst_t *i = board->insts;
	while(i != NULL) {
		csim_inst_t *next = i->next;
		csim_delete_component(i);
		i = next;
	}
	
	for(int i = 0; i < CSIM_IO_SIZE; i++) {
		csim_io_t *p = board->ios[i];
		while(p != NULL) {
			csim_io_t *q = p->next;
			free(p);
			p = q;
		}
	}

	board->log(board, CSIM_INFO, "deleting board %s", board->name);
	free(board);
}


/**
 * Build a new instance of the given component and add it to the board.
 * @param board	Board to add to.
 * @param comp	Component to build an instance for.
 * @param name	Name of the instance.
 * @param base	Base adress of the instance.
 * @return		Built instance.	
 */
csim_inst_t *csim_new_component(csim_board_t *board, csim_component_t *comp, const char *name, csim_addr_t base) {
	
	/* build the instance */
	uint8_t *p = (uint8_t *)malloc(comp->size + comp->port_cnt * sizeof(csim_port_inst_t));
	csim_inst_t *i = (csim_inst_t *)p;
	i->comp = comp;
	i->name = strdup(name);
	i->base = base;
	i->board = board;
	i->ports = (csim_port_inst_t *)(p + comp->size);
	for(int j = 0; j < comp->port_cnt; j++) {
		i->ports[j].port = &comp->ports[j];
		i->ports[j].inst = i;
		i->ports[j].link = NULL;
	}
	
	/* link to the board */
	i->next = board->insts;
	board->insts = i;

	/* if core, record it in core list */
	if(comp->core != NULL) {
		i->next_core = board->cores;
		board->cores = i;
		if(board->clock == 0)
			board->clock = comp->core->clock;
		else {
			if(board->clock != comp->core->clock)
				board->log(board, CSIM_FATAL, "ERROR: current version only supports multiple core with same clock.");
		}
	}
	
	/* record the IO registers */
	for(int j = 0; j < comp->reg_cnt; j++)
		csim_io_add(&comp->regs[j], i);
	
	/* call preparation of the instance */
	if(CSIM_DEBUG >=board->level)
		board->log(board, CSIM_INFO, "new instance %s of %s at %08x", name, comp->name, name);
	comp->construct(i);
	return i;
}


/**
 * Delete the given component.
 * @param inst	Component instance to delete.
 */
void csim_delete_component(csim_inst_t *inst) {
	csim_board_t *b = inst->board;
	b->log(b, CSIM_INFO, "deleting %s (%s)", inst->name, inst->comp->name);
	inst->comp->destruct(inst);
	free(inst);
}


/**
 * Link two ports.
 * @param inst1	Component instance 1.
 * @param port1	Pin 1.
 * @param inst2	Component instance 2.
 * @param port2	Pin 2.
 */
void csim_connect(csim_inst_t *inst1, csim_port_t *port1, csim_inst_t *inst2, csim_port_t *port2) {
	assert(inst1->board == inst2->board);
	csim_board_t *b = inst1->board;
	
	/* compute pin index */
	int i1 = port1 - inst1->comp->ports;
	assert(0 <= i1 && i1 < inst1->comp->port_cnt);
	csim_port_inst_t *p1 = &inst1->ports[i1];
	int i2 = port2 - inst2->comp->ports;
	assert(0 <= i2 && i2 < inst2->comp->port_cnt);
	csim_port_inst_t *p2 = &inst2->ports[i2];

	/* already connected? */
	if(p1->link != NULL) {
		if(CSIM_ERROR <= b->level)
			b->log(b, CSIM_ERROR, "%s of %s is already connected!", port1->name, inst1->name);
		return;
	}
	if(p2->link != NULL) {
		if(CSIM_ERROR <= b->level)
			b->log(b, CSIM_ERROR, "%s of %s is already connected!", port2->name, inst2->name);
		return;
	}
	
	/* connect the ports */
	if(CSIM_DEBUG <= b->level)
		b->log(b, CSIM_DEBUG, "connecting %s of %s with %s of %s", port1->name, inst1->name, port2->name, inst2->name);
	p1->link = p2;
	p2->link = p1;
}


/**
 * Disconnect a link.
 * @param inst1	Component instance 1.
 * @param port1	Pin 1.
 * @param inst2	Component instance 2.
 * @param port2	Pin 2.
 */
void csim_disconnect(csim_inst_t *inst1, csim_port_t *port1, csim_inst_t *inst2, csim_port_t *port2) {
	assert(inst1->board == inst2->board);
	csim_board_t *b = inst1->board;

	/* compute pin index */
	int i1 = port1 - inst1->comp->ports;
	assert(0 <= i1 && i1 < inst1->comp->port_cnt);
	csim_port_inst_t *p1 = &inst1->ports[i1];
	int i2 = port2 - inst2->comp->ports;
	assert(0 <= i2 && i2 < inst2->comp->port_cnt);
	csim_port_inst_t *p2 = &inst2->ports[i2];

	/* disconnect ports */
	if(CSIM_DEBUG <= b->level)
		b->log(b, CSIM_DEBUG, "disconnecting %s of %s with %s of %s", port1->name, inst1->name, port2->name, inst2->name);
	p1->link = NULL;
	p2->link = NULL;
}


/**
 * Stop emitting on the port.
 * @param inst	Instance containing the port.
 * @param port	Port to listen to.
 */
void csim_mute(csim_inst_t *inst, csim_port_t *port) {
	csim_board_t *b = inst->board;
	assert(port->type == CSIM_ELECTRIC);

	/* compute index */
	int i = port - inst->comp->ports;
	assert(0 <= i && i < inst->comp->port_cnt);
	csim_port_inst_t *pi = &inst->ports[i];

	/* set mute if any */
	if(CSIM_DEBUG <= b->level)
		b->log(b, CSIM_DEBUG, "muting from %s of %s (%d)", port->name, inst->name, b->date);
	
	/* update if needed */
	if(pi->link != NULL) {
		csim_value_t v;
		pi->link->port->update(pi->link, CSIM_NONE, v);
	}
}


/**
 * Send a digital message to a port.
 * @param inst	Instance sending the event.
 * @param port	Pin to send to.
 * @param digit	Digital value to send.
 */
void csim_send_digital(csim_inst_t *inst, csim_port_t *port, int digit) {
	csim_board_t *b = inst->board;
	assert(port->type == CSIM_ELECTRIC);

	/* compute index */
	int i = port - inst->comp->ports;
	assert(0 <= i && i < inst->comp->port_cnt);
	csim_port_inst_t *pi = &inst->ports[i];
	
	/* update the port */
	if(CSIM_DEBUG <= b->level)
		b->log(b, CSIM_DEBUG, "sending digital %d (%d) to %s of %s", digit, b->date, port->name, inst->name);
	
	/* update distant port if any */
	if(pi->link != NULL) {
		csim_value_t v;
		v.digital = digit;
		pi->link->port->update(pi->link, CSIM_DIGITAL, v);
	}
}


/**
 * Record a new event in the event queue.
 * @param board		Board to record event in.
 * @param evt		Event to record.
 */
void csim_record_event(csim_board_t *board, csim_evt_t *evt) {
	if(CSIM_DEBUG >= board->level)
		board->log(board, CSIM_DEBUG, "record event at %d from %s", evt->date, evt->inst->name);
	
	if(evt->date <= board->date) {
		if(CSIM_DEBUG >= board->level)
			board->log(board, CSIM_DEBUG, "trigger event from %s", evt->inst->name);
		evt->trigger(evt);
		if(evt->period == 0)
			return;
		else
			evt->date += evt->period;
	}
	
	if(board->evts == NULL || evt->date < board->evts->date) {
		evt->next = board->evts;
		board->evts = evt;
	}
	else {
		csim_evt_t *cur = board->evts;
		while(cur->next != NULL && cur->date < evt->date)
			cur = cur->next;
		if(cur->next != NULL)
			cur->next->prev = evt;
		evt->next = cur->next;
		cur->next = evt;
		evt->prev = cur;
	}
}


/**
 * Remove an event from the schedule.
 * @param board		Board to work with.
 * @param evt		Event to cancle.
 */
void csim_cancel_event(csim_board_t *board, csim_evt_t *evt) {
	if(board->evts == evt) {
		evt->next->prev = NULL;
		board->evts = evt->next;
		evt->next = NULL;
	}
	else {
		evt->next->prev = evt->prev;
		evt->prev->next = evt->next;
		evt->next = NULL;
		evt->prev = NULL;
	}
}


/**
 * Simulate for the given amount of time.
 * @param board		Board to simulate in.
 * @param time		Time in cycle (cycle duration depends on the board clock).
 */
void csim_run(csim_board_t *board, csim_time_t time) {
	csim_date_t end = board->date + time;
	while(board->date < end) {
		
		while(board->evts != NULL && board->evts->date <= board->date) {
			csim_evt_t *evt = board->evts;
			if(CSIM_DEBUG >= board->level)
				board->log(board, CSIM_DEBUG, "trigger event from %s", evt->inst->name);
			evt->trigger(evt);
			board->evts = evt->next;
			if(board->evts != NULL)
				board->evts->prev = NULL;
			evt->next = NULL;
			if(evt->period != 0) {
				evt->date += evt->period;
				csim_record_event(board, evt);
			}
		}
		
		csim_inst_t *core = board->cores;
		while(core != NULL) {
			core->comp->core->step(core->comp->core);
			core = core->next_core;
		}
		
		board->date++;
	}
}

