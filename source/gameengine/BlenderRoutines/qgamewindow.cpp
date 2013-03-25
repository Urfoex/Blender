/*
    <one line to give the program's name and a brief idea of what it does.>
    Copyright (C) 2013  Manuel Bellersen <email>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/


#include "qgamewindow.h"

QGameWindow::QGameWindow():
QWindow((QScreen*)nullptr),
m_context( new QOpenGLContext())
{
	this->resize(800,600);
	this->setFormat(this->requestedFormat());
	this->create();
	
	m_context->create();
	
}

QGameWindow::~QGameWindow() {

}


void QGameWindow::initGL(){
	m_context->makeCurrent(this);
}

void QGameWindow::updateGL(){
	m_context->makeCurrent(this);
	glClear( GL_COLOR_BUFFER_BIT);
	m_context->swapBuffers(this);
	m_context->doneCurrent();
}

void QGameWindow::init() {
	initGL();
	
	m_timer = std::make_shared<QTimer>(this);
	this->connect(m_timer.get(), SIGNAL(timeout()), this, SLOT(updateGL()));
	m_timer->start(16);
}
