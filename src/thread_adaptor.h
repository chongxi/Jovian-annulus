#ifndef THREAD_ADAPTOR_H
#define THREAD_ADAPTOR_H

/**
 * @brief Adaptor class to invoke a class method from a thread
 * @details Calls a static method '_func' of a class 'param' that will in turn invoke a method 
 * of that class
 */
class thread_adapter
{
  public:
    thread_adapter( void ( *func )( void* ), void* param )
        : _func( func ), _param( param )
    {
    }
    void operator()() const { _func( _param ); }
  private:
    void ( *_func )( void* );
    void* _param;
};


#endif // THREAD_ADAPTOR_H

