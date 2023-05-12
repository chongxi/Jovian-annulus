/*
 QPyConsole.h

 QConsole specialization for Python

 (C) 2006, Mondrian Nuessle, Computer Architecture Group, University of Mannheim, Germany

*
* This library is free software; you can redistribute it and/or
* modify it under the terms of the GNU Lesser General Public
* License as published by the Free Software Foundation; either
* version 2.1 of the License, or (at your option) any later version.
*
* This library is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
* Lesser General Public License for more details.
*
* You should have received a copy of the GNU Lesser General Public
* License along with this library; if not, write to the Free Software
* Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
* MA 02110-1301  USA

  nuessle@uni-mannheim.de

*/

#ifndef QPYCONSOLE_H
#define QPYCONSOLE_H

#include "virtual_machine.h"

#include <QAbstractEventDispatcher>
#include "qconsole.h"

void wrap_pyconsole();

class QPyConsole : public QConsole
{
    Q_OBJECT

  public:
    //destructor
    ~QPyConsole();

    //get the QPyConsole instance
    static QPyConsole* getInstance( QWidget* parent = NULL,
                                    const QString& welcomeText = "" );

    void printHistory();

    void setNormalPrompt( bool display ) { setPrompt( ">>", display ); }
    void setMultilinePrompt( bool display ) { setPrompt( "...", display ); }

    void add( std::string const& str ) { _result += QString::fromStdString( str ); }
    void set( std::string const& str ) { _result = QString::fromStdString( str ); }

    void set_vm( Virtual_Machine* vm );

  Q_SIGNALS:
    //Signal emitted after that a command is executed
    void resultsPosted( const std::string& results );

  protected:
    //give suggestions to complete a command (not working...)
    QStringList suggestCommand( const QString& cmd, QString& prefix );

    //private constructor
    QPyConsole( QWidget* parent = NULL,
                const QString& welcomeText = "" );

    //execute a validated command
    QString interpretCommand( const QString& command, int* res );

  private:

    //The instance
    static QPyConsole* theInstance;

  private Q_SLOTS:
    void awake();

  private:
    QAbstractEventDispatcher* dispatcher;

    bool _use_local_result;
    // string holding the current command
    QString _result;
    Virtual_Machine* _vm;
};

#endif
