#ifndef JASS_ADSR_HH
#define JASS_ADSR_HH

#include <QWidget>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QGraphicsItem>
#include <QGraphicsLineItem>
#include <QGraphicsPolygonItem>
#include <QVector>
#include <QPointF>
#include <QPalette>
#include <QPainter>

#include <cmath>

#include "generator.h"

struct adsr_widget : public QWidget {
	Q_OBJECT

	disposable_generator_ptr gen;

	public:
		adsr_widget(disposable_generator_ptr gen, QWidget *parent = 0) :
			QWidget(parent),
			gen(gen)
		{
			QPalette palette = QWidget::palette();
			palette.setColor(backgroundRole(), Qt::white);
			setPalette(palette);
		}

		void paintEvent(QPaintEvent *)
		{
			QPainter painter(this);
			painter.setRenderHint(QPainter::Antialiasing);
			painter.setPen(Qt::darkGreen);
			painter.setBrush(Qt::darkGreen);

			unsigned int n = 500;
			QVector<QPointF> points;
			points.push_back(QPointF(0.0, height()-1));
			points.push_back(QPointF(gen->t.attack_g * 50.0, 0.0));
			points.push_back(QPointF(gen->t.attack_g + gen->t.decay_g * 50.0, height() * (1.0 - gen->t.sustain_g)));
			points.push_back(QPointF(gen->t.attack_g + gen->t.decay_g + gen->t.sustain_g * 50.0, height() * (1.0 - gen->t.sustain_g)));

			painter.drawPolygon(&points[0], points.size(), Qt::OddEvenFill);
		}

		QSize minimumSizeHint() const
		{
			return QSize(50, 100);
		}

		QSize sizeHint() const
		{
			return QSize(200, 100);
		}
};

#endif
