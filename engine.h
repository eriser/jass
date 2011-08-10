#ifndef JASS_ENGINE_HH
#define JASS_ENGINE_HH

#include <vector>
#include <list>
#include <algorithm>
#include <iostream>

#include <jack/jack.h>
#include <jack/midiport.h>
#include <jack/session.h>

#include <boost/function.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/bind.hpp>
#include <boost/function.hpp>

#include "disposable.h"
#include "generator.h"
#include "ringbuffer.h"
#include "jass.hxx"
#include "xsd_error_handler.h"
#include "assign.h"

#include <QObject>

typedef std::vector<disposable_generator_ptr> generator_vector;
typedef disposable<generator_vector> disposable_generator_vector;
typedef boost::shared_ptr<disposable_generator_vector> disposable_generator_vector_ptr;

typedef std::list<disposable_generator_ptr> generator_list;
typedef disposable<generator_list> disposable_generator_list;
typedef boost::shared_ptr<disposable_generator_list> disposable_generator_list_ptr;


typedef ringbuffer<boost::function<void(void)> > command_ringbuffer;

struct engine;

extern "C" {
	int process_callback(jack_nframes_t, void *p);
	void session_callback(jack_session_event_t *event, void *arg);
}

class engine : public QObject {
	Q_OBJECT

	public:
		//! The ringbuffer for the commands that have to be passed to the process callback
		command_ringbuffer commands;

		//! When the engine is done processing a command that possibly alters references, it will signal completion by writing a 0 in this ringbuffer
		ringbuffer<char> acknowledgements;

		//! disposable vector holding generators. This is a disposable_vector_ptr so that the whole collection of generators can be replaced in one step, which is useful for loading/reloading setups
		disposable_generator_list_ptr gens;

		//! a single generator to audit a sample
		disposable_generator_ptr auditor_gen;

		jack_client_t *jack_client;
		jack_port_t *out_0;
		jack_port_t *out_1;
		jack_port_t *midi_in;

		//! Set this member only using the set_samplerate method..
		double sample_rate;

	public:
		engine(const char *uuid = 0) 
		: 
			commands(1024),
			acknowledgements(1024),
			gens(disposable_generator_list::create(generator_list()))
		{
			heap *h = heap::get();	

			jack_client = jack_client_open("jass", JackSessionID, NULL, uuid);
			out_0 = jack_port_register(jack_client, "out_0", JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);
			out_1 = jack_port_register(jack_client, "out_1", JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);
			midi_in = jack_port_register(jack_client, "in", JACK_DEFAULT_MIDI_TYPE, JackPortIsInput, 0);

			sample_rate = jack_get_sample_rate(jack_client);

			jack_set_session_callback(jack_client, ::session_callback, this);

			jack_set_process_callback(jack_client, process_callback, (void*)this);
			jack_activate(jack_client);
		}

		~engine() {
			jack_deactivate(jack_client);
			jack_client_close(jack_client);
		}

		void set_sample_rate(double rate) {
			if (rate != sample_rate) {
				sample_rate = rate;
				//! TODO: Reload all samples..
			}
		}

		void session_callback(jack_session_event_t *event) {
			emit session_event(event);
		}

		void play_auditor() {
			assert(auditor_gen.get());

			auditor_gen->t.channel = 16;
			auditor_gen->t.voices->t[0].gain_envelope_state = voice::ATTACK;
			auditor_gen->t.voices->t[0].filter_envelope_state = voice::ATTACK;
			auditor_gen->t.voices->t[0].note_on_frame = jack_last_frame_time(jack_client);
			auditor_gen->t.voices->t[0].note_on_velocity = 64;
			auditor_gen->t.voices->t[0].note = 64;
		}
	
		void process(jack_nframes_t nframes) {
			float *out_0_buf = (float*)jack_port_get_buffer(out_0, nframes);
			float *out_1_buf = (float*)jack_port_get_buffer(out_1, nframes);
			void *midi_in_buf = jack_port_get_buffer(midi_in, nframes);	

			//! zero the buffers first
			std::fill(out_0_buf, out_0_buf + nframes, 0);
			std::fill(out_1_buf, out_1_buf + nframes, 0);
	
			//! Execute commands passed in through ringbuffer
			while(commands.can_read()) { /* std::cout << "read()()" << std::endl; */ 
				commands.read()(); 
				if (!acknowledgements.can_write()) std::cout << "ack buffer full" << std::endl;
				else acknowledgements.write(0);
			}
	
			if (auditor_gen.get()) {
				auditor_gen->t.process(out_0_buf, out_1_buf, midi_in_buf, nframes, jack_client);
			}
	
			for (generator_list::iterator it = gens->t.begin(); it != gens->t.end(); ++it) {
				(*it)->t.process(out_0_buf, out_1_buf, midi_in_buf, nframes, jack_client);
			}
			//! Synthesize
		}
	
	signals:
		void session_event(jack_session_event_t *);
};


#endif
