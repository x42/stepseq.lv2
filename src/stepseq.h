#define xstr(s) str(s)
#define str(s) #s
#define SEQ_URI "http://gareus.org/oss/lv2/stepseq#s" xstr(N_STEPS) "n" xstr(N_NOTES)

enum {
	PORT_CTRL_IN = 0,
	PORT_MIDI_OUT,
	PORT_SYNC,
	PORT_BPM,
	PORT_DIVIDER,
	PORT_SWING,
	PORT_DRUM,
	PORT_CHN,
	PORT_PANIC,
	PORT_STEP,
	PORT_NOTES
};
