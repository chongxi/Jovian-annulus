#include "python_socket_thread.h"

void
Python_Socket_Thread::run()
{
	_comm->start();
	
  while ( !_terminated )
  {
        boost::unique_lock< boost::try_mutex > lock( _mutex );
        // We block only if the renderer hasn't gotten to this
        // bucket yet
        boost::system_time const timeout =
            boost::get_system_time() + boost::posix_time::milliseconds( 16 );
        _condition.timed_wait( lock, timeout );
  }
}
