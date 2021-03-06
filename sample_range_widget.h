#ifndef JASS_SAMPLE_RANGE_WIDGET_HH
#define JASS_SAMPLE_RANGE_WIDGET_HH

#include <QWidget>
#include <QHBoxLayout>
#include <QCheckBox>

#include "generator.h"
#include "waveform_widget.h"
#include "engine.h"
#include "assign.h"

struct sample_range_widget : public QWidget {
	Q_OBJECT

	disposable_generator_ptr gen;

	QCheckBox *looping;

	public slots:
		void loop_changed(bool state) {
			engine::get()->write_command(assign(gen->t.looping, state));
			engine::get()->deferred_commands.write(boost::bind(&sample_range_widget::update, this));
		}

	public:
		sample_range_widget(disposable_generator_ptr g, QWidget *parent = 0) :
			QWidget(parent),
			gen(g)
		{
			QHBoxLayout *layout = new QHBoxLayout();
			looping = new QCheckBox();
			looping->setChecked(gen->t.looping);
			looping->setToolTip("Toggle to enable looping");
			connect(looping, SIGNAL(toggled(bool)), this, SLOT(loop_changed(bool)));
			layout->addWidget(looping, 0);
			layout->addWidget(new waveform_widget(gen), 1);
			setLayout(layout);
		}
};

#endif

