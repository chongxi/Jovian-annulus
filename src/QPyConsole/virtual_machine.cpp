
#include <algorithm>
#include <iostream>
#include <sstream>

#include <boost/algorithm/string.hpp>

#include "virtual_machine.h"
#include "Console.h"
#include "scene_model.h"
#include "thread_adaptor.h"

/** @brief Virtual_Machine.
 * @details
 */

std::ostream& operator<<( std::ostream& os, Virtual_Machine const& f ) { return os; }

PythonOutputRedirector::PythonOutputRedirector( Virtual_Machine* interpreter ) : _interpreter( interpreter ) {}

void
PythonOutputRedirector::write( std::string const& str )
{
    _interpreter->add( str );
}

// Parses the value of the active python exception
// NOTE SHOULD NOT BE CALLED IF NO EXCEPTION
std::string
parse_python_exception()
{
    PyObject* type_ptr = NULL, *value_ptr = NULL, *traceback_ptr = NULL;
    // Fetch the exception info from the Python C API
    PyErr_Fetch( &type_ptr, &value_ptr, &traceback_ptr );

    // Fallback error
    std::string ret( "Unfetchable Python error" );
    // If the fetch got a type pointer, parse the type into the exception string
    if ( type_ptr != NULL )
    {
        bpy::handle<> h_type( type_ptr );
        bpy::str type_pstr( h_type );
        // Extract the string from the boost::python object
        bpy::extract<std::string> e_type_pstr( type_pstr );
        // If a valid string extraction is available, use it
        //  otherwise use fallback
        if ( e_type_pstr.check() )
            ret = e_type_pstr();
        else
            ret = "Unknown exception type";
    }
    // Do the same for the exception value(the stringification of the exception)
    if ( value_ptr != NULL )
    {
        bpy::handle<> h_val( value_ptr );
        bpy::str a( h_val );
        bpy::extract<std::string> returned( a );
        if ( returned.check() )
            ret +=  ": " + returned();
        else
            ret += std::string( ": Unparseable Python error: " );
    }
    // Parse lines from the traceback using the Python traceback module
    if ( traceback_ptr != NULL )
    {
        bpy::handle<> h_tb( traceback_ptr );
        // Load the traceback module and the format_tb function
        bpy::object tb( bpy::import( "traceback" ) );
        bpy::object fmt_tb( tb.attr( "format_tb" ) );
        // Call format_tb to get a list of traceback strings
        bpy::object tb_list( fmt_tb( h_tb ) );
        // Join the traceback strings into a single string
        bpy::object tb_str( bpy::str( "\n" ).join( tb_list ) );
        // Extract the string, check the extraction, and fallback in necessary
        bpy::extract<std::string> returned( tb_str );
        if ( returned.check() )
            ret += ": " + returned();
        else
            ret += std::string( ": Unparseable Python traceback" );
    }
    return ret;
}


//QTcl console constructor (init the QTextEdit & the attributes)
Virtual_Machine::Virtual_Machine(): _lines( 0 ), stdoutRedirector( this ),
_terminated(false), _compiled(false), _executed( false )
{
    try
    {
        Py_Initialize();
        _main = bpy::import( "__main__" );
        _global = _main.attr( "__dict__" );

        _global["PythonOutputRedirector"] =
            bpy::class_<PythonOutputRedirector>( "PythonOutputRedirector", bpy::init<>() )
            .def( "write", &PythonOutputRedirector::write )
            ;

        bpy::import( "sys" ).attr( "stdout" ) = stdoutRedirector;

        bpy::import( "rlcompleter" );
        bpy::object ignored = bpy::exec( "import sys\n"
                                         "import rlcompleter\n"
                                         "sys.path.insert(0, \".\")\n" // add current
                                         // path
                                         "sys.stderr = sys.stdout\n"
                                         "import builtins\n"
                                         "builtins.completer=rlcompleter.Completer()\n",
                                         _global, _global
                                       );
        ignored = bpy::exec( "from Console import *\n", _global, _global );
        ignored = bpy::exec( "from Scene_Model import *\n", _global, _global );
    }
    catch ( boost::python::error_already_set const& )
    {
        // Parse and output the exception
        std::string perror_str = parse_python_exception();
        std::cout << "Error in Python: " << perror_str << std::endl;
    }
}

// Element change
void
Virtual_Machine::set_console( Console_Ptr console )
{
    try
    {
        _global[ "console" ] = bpy::object( bpy::ptr( console.get() ) );
        Scene_Model_Ptr model = console->model();
        _global[ "model" ] = bpy::ptr( model.get() );
    }
    catch ( boost::python::error_already_set const& )
    {
        // Parse and output the exception
        std::string perror_str = parse_python_exception();
        std::cout << "Error in Python: " << perror_str << std::endl;
        throw;
    }
}

// Basic operations
static void do_queue_thread( void* param )
{
    static_cast<Virtual_Machine*>( param )->queue_run();
}

void
Virtual_Machine::run()
{
    boost::thread* thread = new boost::thread( thread_adapter( &do_queue_thread, this ) );

    while ( !_terminated )
    {
        boost::unique_lock< boost::mutex > lock( _mutex );       // acquire the mutex before waiting
        _python_cond.wait( lock );
        execute();
        lock.unlock();
        _condition.notify_one();
    }

    thread->join();
    delete thread;
}

void
Virtual_Machine::execute( std::string commands )
{
    try
    {
        bpy::object ignored = bpy::exec( commands.c_str(), _global, _global );
    }
    catch ( boost::python::error_already_set const& )
    {
        std::cout << parse_python_exception() << std::endl;
    }
}

void
Virtual_Machine::queue_run()
{
    while ( !_terminated )
    {
        boost::unique_lock< boost::mutex > lock( _mutex );
        if ( _compiled )
        {
            lock.unlock();
            _python_cond.notify_one();
        }
        else
            _condition.wait( lock );
    }

    _python_cond.notify_one();
}

std::vector< std::string >
Virtual_Machine::suggest_command( const std::string& cmd )
{
    char run[255];
    int n = 0;
    std::vector< std::string > list;
    _result = "";
    if ( !cmd.empty() )
    {
        do
        {
            snprintf( run, 255, "print(completer.complete(\"%s\",%d))\n",
                      cmd.c_str(), n );
            PyRun_SimpleString( run );
            _result.resize( _result.size() - 1 ); //strip trailing newline
            if ( _result != "None" )
            {
                list.push_back( _result );
                _result = "";
            }
            else
            {
                _result = "";
                break;
            }
            n++;
        }
        while ( true );
    }
    std::vector< std::string >::iterator it;
    it = std::unique( list.begin(), list.end() );
    list.resize( std::distance( list.begin(), it ) );

    return list;
}

//Call the Python interpreter to execute the command
//retrieve back results using the python internal stdout/err redirectory (see above)
std::string
Virtual_Machine::interpret_command( const std::string& command, int* res, int& prompt )
{
    bpy::object py_result;

    bool multiline = false;
    *res = 0;
    prompt = 0;
    if ( command.front() != '#' && ( !command.empty() || ( command.empty() && _lines != 0 ) ) )
    {
        this->_command += command;
        try
        {
            py_result = bpy::object( bpy::handle<>( Py_CompileString( this->_command.c_str(), "<stdin>", Py_single_input ) ) );
        }
        catch ( boost::python::error_already_set const& ) {} // Ignore the exception
        if ( py_result.ptr() == _default_object.ptr() )
        {
            multiline = check_for_unexpected_eof();
            if ( !multiline )
            {
                if ( command.back() == ':' )
                    multiline = true;
            }

            if ( multiline )
            {
                prompt = 2;
                this->_command += "\n";
                _lines++;
                _result = "";
                return "";
            }
            else
            {
                prompt = 1;
                *res = -1;
                std::string result = _result;
                _result = "";
                this->_command = "";
                this->_lines = 0;
                return result;
            }
        }
        if ( ( _lines != 0 && command == "" ) || ( this->_command != "" && _lines == 0 ) )
        {
            prompt = 1;
            this->_command = "";
            this->_lines = 0;

            _compiled_code = py_result;
            _compiled = true;

            _condition.notify_one();

            _result = "";
            return _result;
        }
        else if ( _lines != 0 && command != "" ) //following multiliner line
        {
            this->_command += "\n";
            *res = 0;
            return "";
        }
        else
        {
            return "";
        }

    }
    else
        return "";
}

void
Virtual_Machine::execute()
{
    if ( _compiled_code.ptr() != _default_object.ptr() )
    {
        try
        {
            bpy::object temp( bpy::handle<>( PyEval_EvalCode ( ( PyObject* )_compiled_code.ptr(), _global.ptr(), _global.ptr() ) ) );
        }
        catch ( boost::python::error_already_set const& )
        {
            PyErr_Print();
        }
        _exec_result = _result;
        _compiled = false;
        _executed = true;
        _compiled_code = _default_object;
    }
}

bool
Virtual_Machine::check_for_unexpected_eof()
{
    bool result = false;

    PyObject* type_ptr = NULL, *value_ptr = NULL, *traceback_ptr = NULL;
    // Fetch the exception info from the Python C API
    PyErr_Fetch( &type_ptr, &value_ptr, &traceback_ptr );

    // Fallback error
    _result = "Unfetchable Python error" ;
    // If the fetch got a type pointer, parse the type into the exception string
    if ( type_ptr != NULL )
    {
        bpy::handle<> h_type( type_ptr );
        bpy::str type_pstr( h_type );
        // Extract the string from the boost::python object
        bpy::extract<std::string> e_type_pstr( type_pstr );
        // If a valid string extraction is available, use it
        //  otherwise use fallback
        if ( e_type_pstr.check() )
            _result = e_type_pstr();
        else
            _result = "Unknown exception type";
    }
    // Do the same for the exception value(the stringification of the exception)
    if ( value_ptr != NULL )
    {
        bpy::handle<> h_val( value_ptr );
        bpy::str a( h_val );
        bpy::extract<std::string> returned( a );
        if ( returned.check() )
            _result +=  ": " + returned();
        else
            _result += std::string( ": Unparseable Python error: " );
    }

    if ( _result.find( "exceptions.SyntaxError" ) != std::string::npos &&
            _result.find( "('unexpected EOF while parsing'," ) != std::string::npos )
        result = true;

    return result;
}

