// RuntimeException.h
//

#ifndef LJINC_RUNTIMEEXCEPTION_H
#define LJINC_RUNTIMEEXCEPTION_H

#include "gtkenv.h"

class RuntimeException : public Glib::Exception
{
public:
    explicit RuntimeException(const Glib::ustring& msg) : msg_(msg) {}
    virtual ~RuntimeException() throw() {}

    RuntimeException(const RuntimeException& o) : msg_(o.msg_) {}
    RuntimeException& operator=(const RuntimeException& o) { msg_ = o.msg_; }

    virtual Glib::ustring what() const { return msg_; }

private:
    Glib::ustring msg_;
};

#endif//LJINC_RUNTIMEEXCEPTION_H

