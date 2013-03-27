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


#ifndef QGAMEWINDOW_H
#define QGAMEWINDOW_H

#include <QWindow>
#include <QOpenGLContext>
#include <QOpenGLShader>
#include <QOpenGLShaderProgram>
#include <QTimer>
// #include <QOpenGLFunctions>
#include <memory>

class QGameWindow : public QWindow/*, protected QOpenGLFunctions*/{
Q_OBJECT
public:
	QGameWindow();
	virtual ~QGameWindow();
	void init();
private:
	std::shared_ptr<QOpenGLContext> m_context;
	std::shared_ptr<QTimer> m_timer;

	void initGL();

protected slots:
	void updateGL();
};
#endif // QGAMEWINDOW_H
