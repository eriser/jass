#include <iostream>
#include <vector>
#include <functional>

#include <jack/jack.h>

#include <QApplication>
#include <QTimer>

#include <boost/program_options.hpp>

#include "signal.h"

#include "sample.h"
#include "ringbuffer.h"
#include "disposable.h"
#include "generator.h"
#include "assign.h"

#include "main_window.h"
#include "qfunctor.h"
#include "timed_functor.h"

#include "engine.h"

//! A global variable to communicate the receiption of SIGUSR1 to the check_signalled function
bool signalled = false;
void signal_handler(int signum) {
	if (signum == SIGUSR1) {
		signalled = true;
	}
}

//! Called periodically in GUI thread to see whether we received SIGUSR1 (ladish)
void check_signalled(main_window &w) {
	if (signalled) {
		signalled = false;
		w.save_setup();
	}
}

namespace po = boost::program_options;

int main(int argc, char **argv) {
	QApplication q_application(argc, argv);

	po::options_description desc("Allowed options:");
	desc.add_options()
		("help", "produce this help message")
		("UUID,U", po::value<std::string>(), "jack session UUID")
		("create_setup,c", po::value<std::string>(), "create a named setup (LADISH L1)")
		("setup", po::value<std::vector<std::string> >(), "load setup")
	;

	po::positional_options_description p;
	p.add("setup", -1);

	po::variables_map vm;
	po::store(po::command_line_parser(argc, argv).options(desc).positional(p).run(), vm);
	po::notify(vm);

	//! Make sure the heap instance is created
	heap *h = heap::get();

	{
		const char *uuid = 0;
		if (vm.count("UUID")) uuid = vm["UUID"].as<std::string>().c_str();
		engine e(uuid);

		main_window w(e);

		if (vm.count("create_setup")) w.setup_file_name = vm["create_setup"].as<std::string>();
		if (vm.count("setup")) w.load_setup(vm["setup"].as<std::vector<std::string> >()[0]);

		//! The session_signal is received possibly in the process thread thus we need to use a QueuedConnection
		QObject::connect(
			&e, SIGNAL(session_event(jack_session_event_t*)), 
			&w, SLOT(handle_jack_session_event(jack_session_event_t*)), 
			Qt::QueuedConnection
		);

		//! register SIGUSR1 for ladish session support
		signal(SIGUSR1, signal_handler);

		w.show();

		//! Register a timed function to clean the heap
		timed_functor tf1(boost::bind(&heap::cleanup, heap::get()), 1000);

		//! This function checks for acknowledgements of the engine and reenables the GUI
		timed_functor tf2(boost::bind(&main_window::check_acknowledgements, &w), 100);

		//! This one checks for ladish save signals..
		timed_functor tf3(boost::bind(check_signalled, boost::ref(w)), 100);

		q_application.exec();
	}
	std::cout << "exiting" << std::endl;

	delete heap::get();

	return 0;
}