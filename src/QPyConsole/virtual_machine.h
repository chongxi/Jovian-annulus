#ifndef VIRTUAL_MACHINE
#define VIRTUAL_MACHINE

#include <iostream>
#include <vector>

#include <Python.h>

#ifndef Q_MOC_RUN
#include <boost/python.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/condition.hpp>
#endif

#include "Globals.h"

namespace bpy = boost::python;

class Virtual_Machine;
class PythonOutputRedirector
{
  public:
    PythonOutputRedirector( Virtual_Machine* interpreter = NULL );
    void write( std::string const& str );

  private:
    Virtual_Machine* _interpreter;
};


/** @brief Virtual_Machine.
 * @details
 */

class Virtual_Machine
{
    friend std::ostream& operator<<( std::ostream& os, Virtual_Machine const& f );

  public:

    /// @name Initialization
    ///@{
    Virtual_Machine();
    ///@}

    /// @name Duplication
    ///@{
    ///@}

    /// @name Move
    ///@{
    ///@}

    /// @name Destruction
    ///@{
    ~Virtual_Machine() {}
    ///@}

    /// @name Access
    ///@{
    boost::condition& condition() { return _condition; }
    boost::mutex& mutex() { return _mutex; }
    std::string result() { return _exec_result; }
    ///@}
    /// @name Measurement
    ///@{
    ///@}
    /// @name Comparison
    ///@{
    ///@}
    /// @name Status report
    ///@{
    bool executed() { return _executed; }
    ///@}
    /// @name Status setting
    ///@{
    void clear() { _executed = false; }
    void terminate() { _terminated = true; }
    ///@}
    /// @name Cursor movement
    ///@{
    ///@}
    /// @name Element change
    ///@{
    void add( std::string const& str ) { _result += str; }
    void queue_command( std::string command );
    void set_console( Console_Ptr console );
    ///@}
    /// @name Removal
    ///@{
    ///@}
    /// @name Resizing
    ///@{
    ///@}
    /// @name Transformation
    ///@{
    ///@}
    /// @name Conversion
    ///@{
    ///@}
    /// @name Basic operations
    ///@{
    // execute a/several python statements
    void execute( std::string commands );
    std::string interpret_command( const std::string& command, int* res, int& prompt );
    void queue_run();
    void run();
    std::vector< std::string > suggest_command( const std::string& cmd );
    ///@}
    /// @name Miscellaneous
    ///@{
    ///@}
    /// @name Obsolete
    ///@{
    ///@}
    /// @name Inapplicable
    ///@{
    ///@}

    static void do_thread( void* param )
    {
        static_cast<Virtual_Machine*>( param )->run();
    }

  protected:
    ///@{
    void execute();
    bool check_for_unexpected_eof();
    ///@}

  private:
    bool _terminated, _compiled, _executed;
    std::string _command, _result, _exec_result;
    int _lines;
    bpy::object _main, _global;
    bpy::object _compiled_code;
    bpy::object _default_object;
    std::vector< std::string > _commands;
    boost::mutex _mutex;
    boost::condition _condition, _python_cond;
    PythonOutputRedirector stdoutRedirector;

};  // end of class Virtual_Machine
#endif