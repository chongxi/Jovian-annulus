/*
 QPyConsole.cpp

 Controls the GEMBIRD Silver Shield PM USB outlet device

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

// modified by YoungTaek Oh.

#ifdef WIN32
#   undef _DEBUG
#endif

#include <iostream>

#include "qpyconsole.h"

#include <QDebug>

struct PyConsole
{
    void clear() { QPyConsole::getInstance()->clear(); }
    void reset() { QPyConsole::getInstance()->reset(); }
    void save( std::string filename ) { QPyConsole::getInstance()->saveScript( QString::fromStdString( filename ) ); }
    void load( std::string filename ) { QPyConsole::getInstance()->loadScript( QString::fromStdString( filename ) ); }
    void history() { QPyConsole::getInstance()->printHistory(); }
    void quit() { QPyConsole::getInstance()->set( "Use reset() to restart the interpreter; otherwise exit your application\n" ); }
};

BOOST_PYTHON_MODULE( PyConsole )
{
    bpy::class_< PyConsole >( "PyConsole" )
    .def( "clear", &PyConsole::clear )
    .def( "reset", &PyConsole::reset )
    .def( "save", &PyConsole::save )
    .def( "load", &PyConsole::load )
    .def( "history", &PyConsole::history )
    .def( "quit", &PyConsole::quit )
    ;
}

void
wrap_pyconsole()
{
    // Register the module with the interpreter
    if ( PyImport_AppendInittab( "PyConsole", PyInit_PyConsole ) == -1 )
        throw std::runtime_error( "Failed to add PyConsole to the interpreter's "
                                  "builtin modules" );
}

void
QPyConsole::printHistory()
{
    uint index = 1;
    for ( QStringList::Iterator it = history.begin(); it != history.end(); ++it )
    {
        _result.append( QString( "%1\t%2\n" ).arg( index ).arg( *it ) );
        index ++;
    }

    _use_local_result = true;
}

QPyConsole* QPyConsole::theInstance = NULL;

QPyConsole* QPyConsole::getInstance( QWidget* parent, const QString& welcomeText )
{
    if ( !theInstance )
    {
        theInstance = new QPyConsole( parent, welcomeText );
    }
    return theInstance;
}

//QTcl console constructor (init the QTextEdit & the attributes)
QPyConsole::QPyConsole( QWidget* parent, const QString& welcomeText ) :
    QConsole( parent, welcomeText ), _use_local_result( false )
{
    dispatcher = QAbstractEventDispatcher::instance();
    connect( dispatcher, SIGNAL( awake() ), SLOT( awake() ) );

    //set the Python Prompt
    setNormalPrompt( true );
}

void
QPyConsole::set_vm( Virtual_Machine* vm )
{
    _vm = vm;

    _vm->execute( "import PyConsole\n" );
    _vm->execute( "pyconsole = PyConsole.PyConsole()\n"
                  "builtins.clear=pyconsole.clear\n"
                  "builtins.reset=pyconsole.reset\n"
                  "builtins.save=pyconsole.save\n"
                  "builtins.load=pyconsole.load\n"
                  "builtins.history=pyconsole.history\n"
                  "builtins.quit=pyconsole.quit\n" );
}

//Desctructor
QPyConsole::~QPyConsole()
{
}

//Call the Python interpreter to execute the command
//retrieve back results using the python internal stdout/err redirectory (see above)
QString
QPyConsole::interpretCommand( const QString& command, int* res )
{
    int prompt;
    QString result = QString::fromStdString( _vm->interpret_command( command.toStdString(), res, prompt ) );
    switch ( prompt )
    {
        case 0:
            break;
        case 1:
            setNormalPrompt( false );
            break;
        case 2:
            setMultilinePrompt( false );
            break;
    }
    QConsole::interpretCommand( command, res );
    if ( _use_local_result )
    {
        _use_local_result = false;
        result = _result;
    }

    return result;
}

QStringList
QPyConsole::suggestCommand( const QString& cmd, QString& prefix )
{
    std::vector< std::string > result = _vm->suggest_command( cmd.toStdString() );
    QStringList list;
    for ( std::vector< std::string >::iterator it = result.begin(); it != result.end(); ++it )
    {
        list.append( QString::fromStdString( *it ) );
    }

    return list;
}

void
QPyConsole::awake()
{
    if ( _vm->executed() )
    {
        boost::unique_lock< boost::mutex > lock( _vm->mutex() );
        std::string result( _vm->result() );
        append( QString::fromStdString( result ) );
        moveCursor( QTextCursor::End );
        displayPrompt();
        _vm->clear();
        lock.unlock();
        _vm->condition().notify_one();

        Q_EMIT resultsPosted( result );
    }
}

