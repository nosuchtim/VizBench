#ifndef _SPACE_SERVER_H
#define _SPACE_SERVER_H

// The following ifdef block is the standard way of creating macros which make exporting 
// from a DLL simpler. All files within this DLL are compiled with the VIZSERVER_EXPORTS
// symbol defined on the command line. This symbol should not be defined on any project
// that uses this DLL. This way any other project whose source files include this file see 
// VIZSERVER_API functions as being imported from a DLL, whereas this DLL sees symbols
// defined with this macro as being exported.
#ifdef VIZSERVER_EXPORTS
#define VIZSERVER_API __declspec(dllexport)
#else
#define VIZSERVER_API __declspec(dllimport)
#endif

// #include "NosuchDaemon.h"
#include "NosuchJSON.h"
#include "cJSON.h"
#include "NosuchOsc.h"
#include "NosuchMidi.h"
#include "NosuchGraphics.h"
#include "mmtt_sharedmem.h"
// #include "limits.h"
typedef int click_t;

#include <list>
#include <pthread.h>

class MidiMsg;
class VizServerJsonProcessor;
class VizServerOscProcessor;
class VizServerMidiProcessor;
class VizServerCursorProcessor;
class VizServerKeystrokeProcessor;
class NosuchDaemon;
class NosuchScheduler;
class MidiPhrase;
class UT_SharedMem;
class MMTT_SharedMemHeader;
class MidiMsg;
class VizServer;
class Region;
class VizCursor;
struct OutlineMem;

typedef const char* (*jsoncallback_t)(void* data,const char *method, cJSON* json, const char* id);
typedef void (*osccallback_t)(void* data,const char* source, const osc::ReceivedMessage&);
typedef void (*midicallback_t)(void* data,MidiMsg* m);
typedef void (*cursorcallback_t)(void* data,VizCursor* c,int downdragup);
typedef void (*keystrokecallback_t)(void* data,int key,int downup);

#define CURSOR_DOWN 0
#define CURSOR_DRAG 1
#define CURSOR_UP 2

#define KEYSTROKE_DOWN 0
#define KEYSTROKE_UP 1

#define CURSOR_Z_UNSET (-1.0f)
#define CURSOR_AREA_UNSET (-1.0f)

class KeystrokeListener {
public:
	virtual void processKeystroke(int key, int downup) {
		DEBUGPRINT(("HEY!  processKeystroke in KeystrokeListener called!?"));
	}
};

class CursorListener {
public:
	virtual void processCursor(VizCursor* c, int downdragup) {
		DEBUGPRINT(("HEY!  processCursor in NosuchCursorProcess called!?"));
	}
};

struct CursorFilter {
public:
	CursorFilter(int mn, int mx) {
		sidmin = mn;
		sidmax = mx;
	}
	CursorFilter() {
		sidmin = INT_MIN;
		sidmax = INT_MAX;
	}
	int sidmin;
	int sidmax;
};

class VizCursor {
public:
	// methods
	VizCursor(VizServer* ss, int sid_, std::string source_,
		NosuchPos pos_, double area_, OutlineMem* om_, MMTT_SharedMemHeader* hdr_);

	void touch(double tm) {
		last_touched = tm;
	}
	bool matches(CursorFilter cf) {
		return (sid >= cf.sidmin && sid <= cf.sidmax);
	}
	double depth() {
		return pos.z;
	}
	void setdepth(double d) {
		pos.z = d;
	}
	double target_depth() { return m_target_depth; }
	void set_target_depth(double d) { m_target_depth = d; }

	NosuchPos previous_musicpos() { return _previous_musicpos; }
	double last_depth() { return m_last_depth; }
	void set_previous_musicpos(NosuchPos p) { _previous_musicpos = p; }
	void set_last_depth(double f) { m_last_depth = f; }
	void advanceTo(double tm);

	double radian2degree(double r) {
		return r * 360.0 / (2.0 * (double)M_PI);
	}

	std::vector<int>& lastpitches() { return m_last_pitches; }
	int lastchannel() { return m_last_channel; }
	int lastclick() { return m_last_click; }

	void add_last_note(int clk, MidiMsg* m) {
		// NosuchDebug(2,"ADD_LAST_NOTE clk=%d m=%s",clk,m->DebugString().c_str());
		m_last_click = clk;
		m_last_channel = m->Channel();
		m_last_pitches.push_back(m->Pitch());
	}
	int last_pitch() {
		if ( m_last_pitches.size() == 0 ) {
			return -1;
		}
		return m_last_pitches.front();
	}
	void clear_last_note() {
		NosuchDebug(2,"CLEAR_LAST_NOTE!");
		m_last_click = -1;
		m_last_channel = -1;
		m_last_pitches.clear();
	}

	// members
	NosuchPos pos;
	NosuchPos target_pos;
	VizServer* m_vizserver;
	double last_touched;
	int sid;
	std::string source;
	double area;
	OutlineMem* outline;
	MMTT_SharedMemHeader* hdr;
	Region* region;

	double curr_speed;
	double curr_degrees;

private:
	double m_target_depth;
	double m_last_depth;
	std::vector<int> m_last_pitches;
	int m_last_channel;
	int m_last_click;
	NosuchPos _previous_musicpos;
	double m_last_tm;
	NosuchPos m_last_pos;
	double m_target_degrees;
	bool m_g_firstdir;
	double m_smooth_degrees_factor;

};

class VIZSERVER_API VizServer {
public:
	static VizServer* GetServer();
	static void DeleteServer();

	// Some utilities
	int SchedulerTimestamp();
	click_t SchedulerCurrentClick();
	void SetTime(double tm);
	double GetTime();

	bool Start();
	void Stop();

	bool IsVizlet(const char* iname);
	const char *VizTags();
	void ChangeVizTag(void* handle, const char* newtag);
	void AdvanceCursorTo(VizCursor* c, double tm);

	void AddJsonCallback(void* handle, const char* apiprefix, jsoncallback_t cb, void* data);
	void AddMidiInputCallback(void* handle, MidiFilter mf, midicallback_t cb, void* data);
	void AddMidiOutputCallback(void* handle, MidiFilter mf, midicallback_t cb, void* data);
	void AddCursorCallback(void* handle, CursorFilter cf, cursorcallback_t cb, void* data);
	void AddKeystrokeCallback(void* handle, keystrokecallback_t cb, void* data);

	void RemoveJsonCallback(void* handle);
	void RemoveMidiInputCallback(void* handle);
	void RemoveMidiOutputCallback(void* handle);
	void RemoveCursorCallback(void* handle);
	void RemoveKeystrokeCallback(void* handle);

	void SendMidiMsg(MidiMsg* msg);

	void QueueMidiMsg(MidiMsg* m, click_t clk);
	void QueueMidiPhrase(MidiPhrase* ph, click_t clk);
	void QueueClear();

	const char* MidiInputName(size_t n) {
		return m_scheduler->MidiInputName(n);
	}
	const char* MidiOutputName(size_t n) {
		return m_scheduler->MidiOutputName(n);
	}

	void SetClicksPerSecond(int clicks);
	int GetClicksPerSecond();
	void ANO(int ch = -1);
	void SetTempoFactor(float f);
	void SetGlobalPitchOffset(int offset) {
		GlobalPitchOffset = offset;
	}

	void IncomingNoteOff(click_t clk, int ch, int pitch, int vel, void* handle);
	void IncomingMidiMsg(MidiMsg* m, click_t clk, void* handle);
	void SendControllerMsg(MidiMsg* m, void* handle, bool smooth);
	void SendPitchBendMsg(MidiMsg* m, void* handle, bool smooth);

	void InsertKeystroke(int key, int downup);

	bool Started() { return m_started; }
	int NumCallbacks();
	int MaxCallbacks() { return m_maxcallbacks; }

	void LockNotesDown() {
		m_scheduler->LockNotesDown();
	}
	std::list<MidiMsg*>& NotesDown() {
		return m_scheduler->NotesDown();
	}
	void ClearNotesDown() {
		return m_scheduler->ClearNotesDown();
	}
	void UnlockNotesDown() {
		m_scheduler->UnlockNotesDown();
	}

	int TryLockCursors() {
		return NosuchTryLock(&_cursors_mutex,"cursors");
	}
	void LockCursors() {
		NosuchLock(&_cursors_mutex,"cursors");
	}
	void UnlockCursors() {
		NosuchUnlock(&_cursors_mutex,"cursors");
	}

	int MmttSeqNum() { return m_mmtt_seqnum; }

private:
	
	friend class VizServerOscProcessor;	// so it can access _touchCursorSid, _checkCursorUp, _setCursor
	friend class VizServerClickProcessor;	// so it can access _advanceClickTo
	friend class VizServerJsonProcessor;

	VizServer();
	virtual ~VizServer();

	void _setMidiFile(const char* file) { m_midifile = file; }
	const char* _getMidiFile() { return m_midifile; }

	void _setMidiOutput(int i) { m_midioutput = i; }
	int _getMidiOutput() { return m_midioutput; }

	void _processServerConfig(cJSON* json);
	cJSON* _readconfig(const char* fname);
	void _setMaxCallbacks();

	std::string _processJson(std::string fullmethod,cJSON* params,const char* id);
	void _advanceClickTo(int current_click, NosuchScheduler* sched);
	void _checkSharedMem();

	void _openSharedMemOutlines();
	void _closeSharedMemOutlines();
	UT_SharedMem* _getSharedMemOutlines() { return m_sharedmem_outlines; }

	void _checkCursorUp();
	VizCursor* _getCursor(int sidnum, std::string sidsource, bool lockit);
	void _touchCursorSid(int sidnum, std::string sidsource);
	void _setCursor(int sidnum, std::string sidsource, NosuchPos pos, double area, OutlineMem* om, MMTT_SharedMemHeader* hdr);
	void _setCursorSid(int sidnum, const char* source, double x, double y, double z, double tuio_f, OutlineMem* om, MMTT_SharedMemHeader* hdr );
	void _processCursorsFromBuff(MMTT_SharedMemHeader* hdr);

	void _scheduleMidiMsg(MidiMsg* m, click_t clk, void* handle);
	void _scheduleMidiPhrase(MidiPhrase* ph, click_t clk, void* handle);
	void _scheduleClear();

	static void _errorPopup(const char* msg);

	bool m_started;

	bool m_debugApi;

	UT_SharedMem* m_sharedmem_outlines;
	long m_sharedmem_last_attempt;

	// These things are pulled from the config file
	const char* m_midi_input_list;
	const char* m_midi_output_list;
	const char* m_midi_merge_list;
	bool m_do_sharedmem;
	const char *m_sharedmemname;
	bool m_do_errorpopup;
	bool m_do_ano;
	const char* m_midifile;
	int m_midioutput;   // default MIDI output index

	int m_osc_input_port;
	const char * m_osc_input_host;

	int m_maxcallbacks;
	int m_mmtt_seqnum;
	double m_time;

	pthread_mutex_t _cursors_mutex;
	std::list<VizCursor*>* m_cursors;

	int m_httpport;
	const char* m_htmlpath;
	NosuchDaemon* m_daemon;

	// Processors are Listeners that broadcast things to Callbacks
	VizServerJsonProcessor* m_jsonprocessor;
	VizServerOscProcessor* m_oscprocessor;
	VizServerMidiProcessor* m_midiinputprocessor;
	VizServerMidiProcessor* m_midioutputprocessor;
	VizServerCursorProcessor* m_cursorprocessor;
	VizServerKeystrokeProcessor* m_keystrokeprocessor;
	NosuchScheduler* m_scheduler;
	NosuchClickListener* m_clickprocessor;

};

#endif
