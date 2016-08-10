/* stepseq -- LV2 midi event generator
 *
 * Copyright (C) 2016 Robin Gareus <robin@gareus.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#define _GNU_SOURCE

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>

#include <lv2/lv2plug.in/ns/lv2core/lv2.h>
#include <lv2/lv2plug.in/ns/ext/atom/atom.h>
#include <lv2/lv2plug.in/ns/ext/atom/forge.h>
#include <lv2/lv2plug.in/ns/ext/log/logger.h>
#include <lv2/lv2plug.in/ns/ext/midi/midi.h>
#include "lv2/lv2plug.in/ns/ext/time/time.h"
#include <lv2/lv2plug.in/ns/ext/urid/urid.h>

#define SEQ_URI "http://gareus.org/oss/lv2/stepseq"

#ifndef N_NOTES
# define N_NOTES 8
#endif
#ifndef N_STEPS
# define N_STEPS 8
#endif

#define STEPS_PER_BEAT 2.f

enum {
	PORT_CTRL_IN = 0,
	PORT_MIDI_OUT,
	PORT_BPM,
	PORT_CHN,
	PORT_PANIC,
	PORT_STEP,
	PORT_NOTES
};

typedef struct {
	LV2_URID atom_Blank;
	LV2_URID atom_Object;
	LV2_URID atom_Sequence;
	LV2_URID midi_MidiEvent;
	LV2_URID atom_Float;
	LV2_URID atom_Long;
	LV2_URID time_Position;
	LV2_URID time_barBeat;
	LV2_URID time_beatsPerMinute;
	LV2_URID time_speed;
	LV2_URID time_frame;
} StepSeqURIs;

typedef struct {
	/* ports */
	const LV2_Atom_Sequence* ctrl_in;
	LV2_Atom_Sequence* midiout;
	float* p_bpm;
	float* p_chn;
	float* p_panic;
	float* p_step;

	float* p_note[N_NOTES];
	float* p_grid[N_NOTES * N_STEPS];

	/* atom-forge and URI mapping */
	LV2_URID_Map* map;
	StepSeqURIs uris;
	LV2_Atom_Forge forge;
	LV2_Atom_Forge_Frame frame;

	/* LV2 Output */
	LV2_Log_Log* log;
	LV2_Log_Logger logger;

	/* Cached Port */
	float bpm;

	/* Settings */
	float sample_rate;
	float spb; // samples / beat

	/* Host Time */
	bool  host_info;
	float host_bpm;
	float bar_beats;
	float host_speed;
	long int host_frame;

	/* State */
	float    stme;  // sample-time
	int32_t  step; // current step
	uint8_t  chn;  // midi channel

	uint8_t  notes[N_NOTES];
	uint8_t  active[127];

} StepSeq;

#define NSET(note, step) (*self->p_grid[ (note) * N_STEPS + (step) ] > 0)
#define ACTV(note) (self->active[note] > 0)
#define NOTE(note) (self->notes[note])


/* *****************************************************************************
 * helper functions
 */

/** map uris */
static void
map_mem_uris (LV2_URID_Map* map, StepSeqURIs* uris)
{
	uris->atom_Blank          = map->map (map->handle, LV2_ATOM__Blank);
	uris->atom_Object         = map->map (map->handle, LV2_ATOM__Object);
	uris->midi_MidiEvent      = map->map (map->handle, LV2_MIDI__MidiEvent);
	uris->atom_Sequence       = map->map (map->handle, LV2_ATOM__Sequence);

	uris->atom_Long           = map->map(map->handle, LV2_ATOM__Long);
	uris->atom_Float          = map->map(map->handle, LV2_ATOM__Float);
	uris->time_barBeat        = map->map(map->handle, LV2_TIME__barBeat);
	uris->time_beatsPerMinute = map->map(map->handle, LV2_TIME__beatsPerMinute);
	uris->time_speed          = map->map(map->handle, LV2_TIME__speed);
	uris->time_frame          = map->map(map->handle, LV2_TIME__frame);
}

/**
 * Update the current position based on a host message. This is called by
 * run() when a time:Position is received.
 */
static void
update_position (StepSeq* self, const LV2_Atom_Object* obj)
{
	const StepSeqURIs* uris = &self->uris;

	LV2_Atom* speed = NULL;
	LV2_Atom* frame = NULL;
	LV2_Atom* beat  = NULL;
	LV2_Atom* bpm   = NULL;

	lv2_atom_object_get (
			obj,
			uris->time_barBeat, &beat,
			uris->time_beatsPerMinute, &bpm,
			uris->time_speed, &speed,
			uris->time_frame, &frame,
			NULL);

	if (bpm && bpm->type == uris->atom_Float
			&& beat && beat->type == uris->atom_Float
			&& frame && frame->type == uris->atom_Long
			&& speed && speed->type == uris->atom_Float ) {
		self->host_speed = ((LV2_Atom_Float*)speed)->body;
		self->host_frame = ((LV2_Atom_Long*)frame)->body;
		self->host_bpm   = ((LV2_Atom_Float*)bpm)->body;
		self->bar_beats  = ((LV2_Atom_Float*)beat)->body;
		self->host_info  = true;
	}
}
/**
 * add a midi message to the output port
 */
static void
forge_midimessage (StepSeq* self,
                   uint32_t ts,
                   const uint8_t* const buffer,
                   uint32_t size)
{
	LV2_Atom midiatom;
	midiatom.type = self->uris.midi_MidiEvent;
	midiatom.size = size;

	if (0 == lv2_atom_forge_frame_time (&self->forge, ts)) return;
	if (0 == lv2_atom_forge_raw (&self->forge, &midiatom, sizeof (LV2_Atom))) return;
	if (0 == lv2_atom_forge_raw (&self->forge, buffer, size)) return;
	lv2_atom_forge_pad (&self->forge, sizeof (LV2_Atom) + size);
}

static void
midi_panic (StepSeq* self)
{
	uint8_t event[3];
	event[2] = 0;

	for (uint32_t c = 0; c < 0xf; ++c) {
		event[0] = 0xb0 | c;
		event[1] = 0x40; // sustain pedal
		forge_midimessage (self, 0, event, 3);
		event[1] = 0x7b; // all notes off
		forge_midimessage (self, 0, event, 3);
#if 0
		event[1] = 0x78; // all sound off
		forge_midimessage (self, 0, event, 3);
#endif
	}
}

static void
forge_note_event (StepSeq* self, uint32_t ts, uint8_t note, uint8_t vel)
{
	uint8_t msg[3];
	if (vel > 0) {
		if (ACTV (note)) {
			++self->active[note];
			return;
		}
		++self->active[note];
		msg[0] = 0x90;
	} else {
		if (!ACTV (note)) {
			lv2_log_error (&self->logger, "StepSeq.lv2: Note-off for a note that's already off\n");
			return;
		}
		--self->active[note];
		msg[0] = 0x80;
	}

	msg[0] |= self->chn;
	msg[1]  = note & 0x7f;
	msg[2]  = vel & 0x7f;
	forge_midimessage (self, ts, msg, 3);
}

/* *****************************************************************************
 * Sequencer
 */

static void
reset_note_tracker (StepSeq* self)
{
	for (uint32_t i = 0; i < 127; ++i) {
		self->active[i] = 0;
	}
}

static void
beat_machine (StepSeq* self, uint32_t ts, uint32_t step)
{
	for (uint32_t n = 0; n < N_NOTES; ++n) {
		const uint8_t note = NOTE (n);
		if (note > 127) {
			continue;
		}
		if (NSET (n, step) && !ACTV (note)) {
			/* send note on */
			forge_note_event (self, ts, note, 127); // TODO maybe use self->grid[] as velocity.
		}
		else if (!NSET (n, step) && ACTV (note)) {
			/* send note off */
			forge_note_event (self, ts, note, 0);
		}
		else if (step == 0) {
			/* re-trigger note if it's always on on the first beat. */
			bool retriger = true;
			for (uint32_t s = 0; s < N_STEPS; ++s) {
				if (!NSET (n, s)) {
					retriger = false;
					break;
				}
			}
			if (retriger) {
				const uint8_t note = NOTE (n);
				forge_note_event (self, ts, note, 0);
				forge_note_event (self, ts + 1, note, 127); // TODO velocity
			}
		}
	}
}

/* *****************************************************************************
 * LV2 Plugin
 */

static LV2_Handle
instantiate (const LV2_Descriptor*     descriptor,
             double                    rate,
             const char*               bundle_path,
             const LV2_Feature* const* features)
{
	StepSeq* self = (StepSeq*)calloc (1, sizeof (StepSeq));

	int i;
	for (i=0; features[i]; ++i) {
		if (!strcmp (features[i]->URI, LV2_URID__map)) {
			self->map = (LV2_URID_Map*)features[i]->data;
		} else if (!strcmp (features[i]->URI, LV2_LOG__log)) {
			self->log = (LV2_Log_Log*)features[i]->data;
		}
	}

	lv2_log_logger_init (&self->logger, self->map, self->log);

	if (!self->map) {
		lv2_log_error (&self->logger, "StepSeq.lv2 error: Host does not support urid:map\n");
		free (self);
		return NULL;
	}

	lv2_atom_forge_init (&self->forge, self->map);
	map_mem_uris (self->map, &self->uris);

	self->sample_rate = rate;
	self->bpm = 120;
	self->spb = self->sample_rate * 60.f / self->bpm;

	reset_note_tracker (self);

	return (LV2_Handle)self;
}

static void
connect_port (LV2_Handle instance,
              uint32_t   port,
              void*      data)
{
	StepSeq* self = (StepSeq*)instance;

	switch (port) {
		case PORT_CTRL_IN:
			self->ctrl_in = (const LV2_Atom_Sequence*)data;
			break;
		case PORT_MIDI_OUT:
			self->midiout = (LV2_Atom_Sequence*)data;
			break;
		case PORT_BPM:
			self->p_bpm = (float*)data;
			break;
		case PORT_CHN:
			self->p_chn = (float*)data;
			break;
		case PORT_PANIC:
			self->p_panic = (float*)data;
			break;
		case PORT_STEP:
			self->p_step = (float*)data;
			break;
		default:
			if (port < PORT_NOTES + N_NOTES) {
				self->p_note[port - PORT_NOTES] = (float*)data;
			}
			else if (port < PORT_NOTES + N_NOTES + N_NOTES * N_STEPS) {
				self->p_grid[port - PORT_NOTES - N_NOTES] = (float*)data;
			}
			break;
	}
}

static void
run (LV2_Handle instance, uint32_t n_samples)
{
	StepSeq* self = (StepSeq*)instance;
	if (!self->midiout || !self->ctrl_in) {
		return;
	}


	const uint32_t capacity = self->midiout->atom.size;
	lv2_atom_forge_set_buffer (&self->forge, (uint8_t*)self->midiout, capacity);
	lv2_atom_forge_sequence_head (&self->forge, &self->frame, 0);


	/* process control events */
	LV2_Atom_Event* ev = lv2_atom_sequence_begin (&(self->ctrl_in)->body);
	while (!lv2_atom_sequence_is_end (&(self->ctrl_in)->body, (self->ctrl_in)->atom.size, ev)) {
		if (ev->body.type == self->uris.atom_Blank || ev->body.type == self->uris.atom_Object) {
			const LV2_Atom_Object* obj = (LV2_Atom_Object*)&ev->body;
			if (obj->body.otype == self->uris.time_Position) {
				update_position (self, obj);
			}
		}
		ev = lv2_atom_sequence_next (ev);
	}

	if (*self->p_bpm != self->bpm) {
		const float old = self->spb;
		self->bpm = *self->p_bpm;
		self->spb = self->sample_rate * 60.f / self->bpm;
		if (self->spb < 20) { self->spb = 20; }
		if (self->spb > 12 * self->sample_rate) { self->spb = 12 * self->sample_rate; }
		self->stme = self->stme * self->spb / old;
	}

	for (uint32_t n = 0; n < N_NOTES; ++n) {
		uint8_t note = ((int)floorf (*self->p_note[n])) & 0x7f;
		if (self->notes[n] == note) {
			continue;
		}
		if (ACTV (NOTE (n))) {
			forge_note_event (self, 0, NOTE (n), 0);
		}
		bool in_use = false;
		for (uint32_t n2 = 0; n2 < N_NOTES; ++n2) {
			if (n2 == n) {
				continue;
			}
			if (self->notes[n2] == note) {
				in_use = true;
			}
		}
		if (in_use) {
			self->notes[n] = 255;
		} else {
			self->notes[n] = note;
		}
	}

	uint8_t chn = ((int)floorf (*self->p_chn)) & 0xf;
	if (chn != self->chn || *self->p_panic > 0) {
		self->chn = chn;
		midi_panic (self);
		reset_note_tracker (self);
	}

	// TODO add host-time support
	// ( self->bar_beats  * STEPS_PER_BEAT ) % N_STEPS

	const float spb = self->spb;
	const float loop_duration = N_STEPS * spb / STEPS_PER_BEAT;

	float stme = self->stme;
	uint32_t step = self->step;
	float next_step = step * spb / STEPS_PER_BEAT;

	uint32_t remain = n_samples;

	while (stme + remain >= next_step) {
		assert (stme <= next_step);
		uint32_t pos = next_step - stme;
		if (stme > next_step) {
			lv2_log_error (&self->logger, "StepSeq.lv2: Past event sneaked through.\n");
			pos = 0;
		}

		remain -= pos;
		stme += pos;

			if (step >= N_STEPS) {
				beat_machine (self, pos, 0);
				stme -= loop_duration;
				self->step = step = 1;
			} else {
				beat_machine (self, pos, step);
				self->step = ++step ;
			}
			next_step = step * spb / STEPS_PER_BEAT;

	}
	self->stme = stme + remain;
	*self->p_step = 1 + (self->step % N_STEPS);
}

static void
cleanup (LV2_Handle instance)
{
	free (instance);
}

static const void*
extension_data (const char* uri)
{
	return NULL;
}

static const LV2_Descriptor descriptor = {
	SEQ_URI,
	instantiate,
	connect_port,
	NULL,
	run,
	NULL,
	cleanup,
	extension_data
};

#undef LV2_SYMBOL_EXPORT
#ifdef _WIN32
#    define LV2_SYMBOL_EXPORT __declspec(dllexport)
#else
#    define LV2_SYMBOL_EXPORT  __attribute__ ((visibility ("default")))
#endif
LV2_SYMBOL_EXPORT
const LV2_Descriptor*
lv2_descriptor (uint32_t index)
{
	switch (index) {
	case 0:
		return &descriptor;
	default:
		return NULL;
	}
}
