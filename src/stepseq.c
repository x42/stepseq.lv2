/* stepseq -- LV2 midi step sequencer
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

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

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

#include "stepseq.h"

typedef struct {
	LV2_URID atom_Blank;
	LV2_URID atom_Object;
	LV2_URID atom_Sequence;
	LV2_URID midi_MidiEvent;
	LV2_URID atom_Float;
	LV2_URID atom_Int;
	LV2_URID atom_Long;
	LV2_URID time_Position;
	LV2_URID time_bar;
	LV2_URID time_barBeat;
	LV2_URID time_beatUnit;
	LV2_URID time_beatsPerBar;
	LV2_URID time_beatsPerMinute;
	LV2_URID time_speed;
} StepSeqURIs;

typedef struct {
	/* ports */
	const LV2_Atom_Sequence* ctrl_in;
	LV2_Atom_Sequence* midiout;
	float* p_sync;
	float* p_bpm;
	float* p_div;
	float* p_swing;
	float* p_drum;
	float* p_chn;
	float* p_panic;
	float* p_step;
	float* p_hostbpm;

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
	float bpm; // beats per minute
	float div; // beats per step

	/* Settings */
	double sample_rate; // samples per second
	double sps; // samples per step

	double swing;
	bool   drum_mode;

	/* Host Time */
	bool     host_info;
	float    host_bpm;
	double   bar_beats;
	float    host_speed;
	int      host_div;

	/* State */
	double   stme; // sample-time
	int32_t  step; // current step
	uint8_t  chn;  // midi channel

	uint8_t  notes[N_NOTES];
	uint8_t  active[128];
	bool     rolling;

} StepSeq;

#define NSET(note, step) (*self->p_grid[ (note) * N_STEPS + (step) ] > 0)
#define NVEL(note, step) ((int)floor(*self->p_grid[ (note) * N_STEPS + (step) ]))
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
	uris->time_Position       = map->map (map->handle, LV2_TIME__Position);
	uris->atom_Long           = map->map (map->handle, LV2_ATOM__Long);
	uris->atom_Int            = map->map (map->handle, LV2_ATOM__Int);
	uris->atom_Float          = map->map (map->handle, LV2_ATOM__Float);
	uris->time_bar            = map->map (map->handle, LV2_TIME__bar);
	uris->time_barBeat        = map->map (map->handle, LV2_TIME__barBeat);
	uris->time_beatUnit       = map->map (map->handle, LV2_TIME__beatUnit);
	uris->time_beatsPerBar    = map->map (map->handle, LV2_TIME__beatsPerBar);
	uris->time_beatsPerMinute = map->map (map->handle, LV2_TIME__beatsPerMinute);
	uris->time_speed          = map->map (map->handle, LV2_TIME__speed);
}

/**
 * Update the current position based on a host message. This is called by
 * run() when a time:Position is received.
 */
static void
update_position (StepSeq* self, const LV2_Atom_Object* obj)
{
	const StepSeqURIs* uris = &self->uris;

	LV2_Atom* bar   = NULL;
	LV2_Atom* beat  = NULL;
	LV2_Atom* bunit = NULL;
	LV2_Atom* bpb   = NULL;
	LV2_Atom* bpm   = NULL;
	LV2_Atom* speed = NULL;

	lv2_atom_object_get (
			obj,
			uris->time_bar, &bar,
			uris->time_barBeat, &beat,
			uris->time_beatUnit, &bunit,
			uris->time_beatsPerBar, &bpb,
			uris->time_beatsPerMinute, &bpm,
			uris->time_speed, &speed,
			NULL);

	if (   bpm   && bpm->type == uris->atom_Float
			&& bpb   && bpb->type == uris->atom_Float
			&& bar   && bar->type == uris->atom_Long
			&& beat  && beat->type == uris->atom_Float
			&& bunit && bunit->type == uris->atom_Int
			&& speed && speed->type == uris->atom_Float)
	{
		float    _bpb   = ((LV2_Atom_Float*)bpb)->body;
		int64_t  _bar   = ((LV2_Atom_Long*)bar)->body;
		float    _beat  = ((LV2_Atom_Float*)beat)->body;

		self->host_div   = ((LV2_Atom_Int*)bunit)->body;
		self->host_bpm   = ((LV2_Atom_Float*)bpm)->body;
		self->host_speed = ((LV2_Atom_Float*)speed)->body;

		self->bar_beats  = _bar * _bpb + _beat; // * host_div / 4.0 // TODO map host metrum
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

static float
parse_division (float div) {
	int d = rintf (div);
	switch (d) {
		case 0: return 0.125f;
		case 1: return 0.25f;
		case 2: return 0.5f;
		case 3: return 1.f;
		case 4: return 2.f;
		case 5: return 3.f;
		case 6: return 4.f;
		case 7: return 8.f;
		case 8: return 12.f;
		case 9: return 16.f;
	}
	return 1.f;
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

		if (NSET (n, step) && ACTV (note) && self->drum_mode) {
			/* retrigger */
			if (ts > 0) {
				forge_note_event (self, ts - 1, note, 0);
				forge_note_event (self, ts, note, NVEL(n, step));
			} else {
				forge_note_event (self, ts, note, 0);
				forge_note_event (self, ts + 1, note, NVEL(n, step));
			}
		}
		else if (NSET (n, step) && !ACTV (note)) {
			/* send note on */
			forge_note_event (self, ts, note, NVEL(n, step));
		}
		else if (!NSET (n, step) && ACTV (note)) {
			/* send note off */
			forge_note_event (self, ts, note, 0);
		}
		else if (step == 0 && NSET (n, 0)) {
			/* re-trigger note if it's always on on the first beat. */
			bool retriger = true;
			for (uint32_t s = 0; s < N_STEPS; ++s) {
				if (!NSET (n, s)) {
					retriger = false;
					break;
				}
			}
			if (retriger) {
				if (ts > 0) {
					forge_note_event (self, ts - 1, note, 0);
					forge_note_event (self, ts, note, NVEL(n, step));
				} else {
					forge_note_event (self, ts, note, 0);
					forge_note_event (self, ts + 1, note, NVEL(n, step));
				}
			}
		}
	}
}

static double
calc_next_step (StepSeq* self) {
	const bool eighth = true; // self->div == 0.5;
	const uint32_t step = self->step;
	if (eighth && (step & 1) == 0) {
		/* add 0.2 -> "3:2 light swing  -- long eighth + short eighth"
		 * add 1/3 -> "2:1 medium swing -- triplet quarter note + triplet eighth"
		 * add 1/2 -> "3:1 hard swing   -- dotted eighth note + sixteenth note"
		 */
		return (step + 1.0) * self->sps + self->swing * self->sps;
	} else {
		return (step + 1.0) * self->sps;
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
	self->bpm = 120.f;
	self->div = .5f;
	self->sps = self->sample_rate * 60.0 * self->div / self->bpm;

	self->step = N_STEPS - 1;
	self->stme = N_STEPS * self->sps;

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
		case PORT_SYNC:
			self->p_sync = (float*)data;
			break;
		case PORT_BPM:
			self->p_bpm = (float*)data;
			break;
		case PORT_DIVIDER:
			self->p_div = (float*)data;
			break;
		case PORT_SWING:
			self->p_swing = (float*)data;
			break;
		case PORT_DRUM:
			self->p_drum = (float*)data;
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
		case PORT_HOSTBPM:
			self->p_hostbpm = (float*)data;
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

	for (uint32_t n = 0; n < N_NOTES; ++n) {
		uint8_t note = ((int)floorf (*self->p_note[n])) & 0x7f;
		if (self->notes[n] == note) {
			continue;
		}
		if (NOTE (n) < 128 && ACTV (NOTE (n))) {
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

	const uint8_t chn = ((int)floorf (*self->p_chn)) & 0xf;
	if (chn != self->chn || *self->p_panic > 0) {
		self->chn = chn;
		midi_panic (self);
		reset_note_tracker (self);
	}

	if (*self->p_panic > 0) {
		self->step = N_STEPS - 1;
		self->stme = N_STEPS * self->sps;
	}

	float bpm;

	if (self->host_info && *self->p_sync > 0) {
		*self->p_hostbpm = self->host_bpm;
		if (self->host_speed <= 0) {
			/* keep track of host position.. */
			self->bar_beats += (double)n_samples * self->host_bpm * self->host_speed / (60.0 * self->sample_rate);
			/* report only, don't modify state  (stme & step need to remain in sync) */
			*self->p_step = 1 + ((int)floor (self->bar_beats / self->div) % N_STEPS);

			if (self->rolling) {
				self->rolling = false;
				midi_panic (self);
				reset_note_tracker (self);
			}
			return;
		}
		bpm = self->host_bpm * self->host_speed;
	} else {
		*self->p_hostbpm = self->host_info ? -1 : 0;
		bpm = *self->p_bpm;
	}

	const float division = parse_division (*self->p_div);
	if (bpm != self->bpm || division != self->div) {
		const double old = self->sps;
		self->bpm = bpm;
		self->div = division;
		self->sps = self->sample_rate * 60.0 * self->div / self->bpm;
		if (self->sps < 64) { self->sps = 64; }
		if (self->sps > 60 * self->sample_rate) { self->sps = 60 * self->sample_rate; }
		self->stme = fmod (self->stme * self->sps / old, N_STEPS * self->sps);
	}

	const double sps = self->sps;
	const double loop_duration = N_STEPS * sps;
	double stme = self->stme;

	self->drum_mode = *self->p_drum > 0;
	self->swing = *self->p_swing;
	if (self->swing < 0) {
		self->swing = 0;
	}
	if (self->swing > 0.5) {
		self->swing = 0.5;
	}

	if (self->host_info && *self->p_sync > 0) {
		double hp = self->bar_beats / self->div;

		stme = fmod (hp, N_STEPS) * sps;

		/* handle seek - jumps to step if needed */
		const double ns = calc_next_step (self);
		if (ns < stme || ns - stme > 1.5 /* max swing*/ * sps || !self->rolling) {

			if (floor (hp) == hp) {
				assert (stme == 0);
				/* immediate transition to step 0 */
				self->step = N_STEPS - 1;
				self->stme = stme = N_STEPS * self->sps;

			} else {
				self->step = ((int)floor (hp) % N_STEPS);
			}

			midi_panic (self);
			reset_note_tracker (self);
		}
	}

	double next_step = calc_next_step (self);
	uint32_t remain = n_samples;

	if (*self->p_panic > 0) {
		/* skip processing */
		remain = 0;
	}

	while (stme + remain > next_step) {
		uint32_t pos;
		if (stme > next_step) {
			/* When decreasing swing, it may be too late for an event.
			 *
			 * In the previous cycle with a larger swing-offset, the event was
			 * still in the future. Now with smaller swing-offset it's in the past.
			 */
			lv2_log_error (&self->logger, "StepSeq.lv2: Past event sneaked through.\n");
			pos = 0;
		} else {
			pos = next_step - stme;
		}

		remain -= pos;
		stme += pos;

		self->step = (self->step + 1) % N_STEPS;

			if (self->step == 0) {
				stme -= loop_duration;
			}
			beat_machine (self, pos, self->step);

			next_step = calc_next_step (self);
	}

	self->stme = stme + remain;
	self->rolling = true;

	/* crude sort Atom sequence by event-time
	 * (in drum-mode or with re-trigger events may
	 * not be sequential in time)
	 */
	LV2_ATOM_SEQUENCE_FOREACH (self->midiout, ev1) {
		LV2_ATOM_SEQUENCE_FOREACH (self->midiout, ev2) {
			if (ev2 <= ev1) {
				continue;
			}
			if (ev1->time.frames > ev2->time.frames) {
				// swap events
				assert (ev1->body.size == ev2->body.size);
				assert (ev1->body.size == 3);
				int64_t tme = ev1->time.frames;
				uint8_t body[3];
				memcpy (body, (const uint8_t*)(ev1 + 1), 3);
				memcpy ((uint8_t*)(ev1 + 1), (const uint8_t*)(ev2 + 1), 3);
				ev1->time.frames = ev2->time.frames;
				memcpy ((uint8_t*)(ev2 + 1), body, 3);
				ev2->time.frames = tme;
			}
		}
	}

	*self->p_step = 1 + (self->step % N_STEPS);
	if (self->host_info) {
		/* keep track of host position.. */
		self->bar_beats += n_samples * self->host_bpm * self->host_speed / (60.0 * self->sample_rate);
	}
}

static void
activate (LV2_Handle instance)
{
	StepSeq* self = (StepSeq*)instance;
	self->chn = 255; // queue reset/panic
	self->step = N_STEPS - 1;
	self->stme = N_STEPS * self->sps;
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
	activate,
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
