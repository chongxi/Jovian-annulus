#ifndef SESSION_H
#define SESSION_H

#include <iostream>

#include <boost/array.hpp>
#include <boost/asio.hpp>

#include <boost/thread/condition.hpp>
#include <boost/thread/mutex.hpp>

using boost::asio::ip::tcp;

typedef boost::array< char, 1024 > Socket_Data;

class Session
{
 public:
   Session() : _socket( 0 ), state_( reading ) {}

   virtual ~Session() { _socket = 0; }

   void wake()
   {
      boost::unique_lock< boost::try_mutex > lock( _mutex );
	  _condition.notify_all();
   }

   void
   set_socket( tcp::socket *socket )
   {
      _socket = socket;
   }

   // Returns true if we want to be notified when the
   // socket is ready for reading.
   bool
   want_read() const
   {
      return state_ == reading;
   }

   // Notify that it should perform its read operation.
   virtual void do_read( boost::system::error_code &ec ) = 0;

   // Returns true we want to be notified when the
   // socket is ready for writing.
   bool
   want_write() const
   {
      return state_ == writing;
   }

   virtual void
   write( std::string data )
   {
   }

   // Notify that it should perform its write operation.
   virtual void do_write( boost::system::error_code &ec ) = 0;

 protected:
   tcp::socket *_socket;
   boost::try_mutex _mutex;
   boost::condition _condition;

   enum
   {
      reading,
      writing
   } state_;
};

class Echo_Session : public Session
{
 public:
   Echo_Session() : Session() {}

   ~Echo_Session() {}

   // Notify that it should perform its read operation.
   void
   do_read( boost::system::error_code &ec )
   {
      if ( std::size_t len = _socket->read_some( boost::asio::buffer( data_ ), ec ) )
      {
         write_buffer_ = boost::asio::buffer( data_, len );
         state_ = writing;
      }
      else
         std::cout << ec << std::endl;
   }

   // Notify that it should perform its write operation.
   void
   do_write( boost::system::error_code &ec )
   {
      if ( std::size_t len =
               _socket->write_some( boost::asio::buffer( write_buffer_ ), ec ) )
      {
         write_buffer_ = write_buffer_ + len;
         state_ = boost::asio::buffer_size( write_buffer_ ) > 0 ? writing : reading;
      }
   }

 private:
   Socket_Data data_;
   boost::asio::const_buffer write_buffer_;
};

class Write_Session : public Session
{
 public:
   Write_Session() : Session() { state_ = writing; }

   ~Write_Session() {}

   // Notify that it should perform its read operation.
   void
   do_read( boost::system::error_code &ec )
   {
      if ( std::size_t len = _socket->read_some( boost::asio::buffer( data_ ), ec ) )
      {
         write_buffer_ = boost::asio::buffer( data_, len );
         state_ = writing;
      }
      else
         std::cout << ec << std::endl;
   }

   // Notify that it should perform its write operation.
   void
   do_write( boost::system::error_code &ec )
   {
      if ( std::size_t len =
               _socket->write_some( boost::asio::buffer( write_buffer_ ), ec ) )
      {
         write_buffer_ = write_buffer_ + len;
      }

      if ( boost::asio::buffer_size( write_buffer_ ) == 0 )
      {
         boost::unique_lock< boost::try_mutex > lock( _mutex );
         _condition.wait( lock );
      }
   }

   void
   write( std::string data )
   {
      std::copy( data.begin(), data.end(), data_.data() );
      write_buffer_ = boost::asio::buffer( data_, data.size() );
      _condition.notify_all();
   }

 private:
   Socket_Data data_;
   boost::asio::const_buffer write_buffer_;
};

class Read_Session : public Session
{
 public:
   Read_Session() : Session() { state_ = reading; }

   virtual ~Read_Session() {}

   virtual void invoke( Socket_Data data, size_t len ) const = 0;

   // Notify that it should perform its read operation.
   void
   do_read( boost::system::error_code &ec )
   {
      if ( std::size_t len = _socket->read_some( boost::asio::buffer( data_ ), ec ) )
      {
      	invoke( data_, len );
      }
      else
      {
         boost::unique_lock< boost::try_mutex > lock( _mutex );
         boost::system_time const timeout = boost::get_system_time() + boost::posix_time::milliseconds( 64 );
         _condition.timed_wait( lock, timeout );
         //std::cout << ec << std::endl;
      }
   }

   // Notify that it should perform its write operation.
   void
   do_write( boost::system::error_code &ec )
   {
      if ( std::size_t len =
               _socket->write_some( boost::asio::buffer( write_buffer_ ), ec ) )
      {
         write_buffer_ = write_buffer_ + len;
      }

      if ( boost::asio::buffer_size( write_buffer_ ) == 0 )
      {
         boost::unique_lock< boost::try_mutex > lock( _mutex );
         _condition.wait( lock );
      }
   }

   void
   write( std::string data )
   {
      std::copy( data.begin(), data.end(), data_.data() );
      write_buffer_ = boost::asio::buffer( data_, data.size() );
      _condition.notify_all();
   }

 private:
   Socket_Data data_;
   boost::asio::const_buffer write_buffer_;
};


#endif // SESSION_H
