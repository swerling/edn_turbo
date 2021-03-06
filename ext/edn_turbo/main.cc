#include <signal.h>
#include <iostream>
#include <clocale>

#include <cstring>

#include "edn_parser.h"

#include <ruby/ruby.h>


namespace edn {

    VALUE rb_mEDNT;

    // Symbols used to call into the ruby world.
    VALUE EDN_MODULE_SYMBOL             = Qnil;

    VALUE EDNT_MAKE_SYMBOL_METHOD       = Qnil;
    VALUE EDNT_MAKE_LIST_METHOD         = Qnil;
    VALUE EDNT_MAKE_SET_METHOD          = Qnil;
    VALUE EDNT_MAKE_BIG_DECIMAL_METHOD  = Qnil;
    VALUE EDNT_TAGGED_ELEM_METHOD       = Qnil;
    VALUE EDNT_EXTENDED_VALUE_METHOD    = Qnil;

    VALUE EDNT_STRING_TO_I_METHOD       = Qnil;
    VALUE EDNT_STRING_TO_F_METHOD       = Qnil;

    // returned when EOF - defined as a constant in EDN module
    VALUE EDNT_EOF_CONST                = Qnil;

    //
    // Wrappers to hook the class w/ the C-api.
    static void delete_parser(edn::Parser *ptr) {
        delete ptr;
    }

    static VALUE wrap_parser_ptr(VALUE klass, edn::Parser* ptr) {
        return Data_Wrap_Struct(klass, 0, delete_parser, ptr);
    }

    static VALUE alloc_obj(VALUE self){
        return wrap_parser_ptr(self, new Parser());
    }

    static inline Parser* get_parser(VALUE self)
    {
        Parser *p;
        Data_Get_Struct( self, edn::Parser, p );
        return p;
    }


    //
    // Called by the constructor - sets the source if passed.
    static VALUE initialize(int argc, VALUE* argv, VALUE self)
    {
        Parser* p = get_parser(self);

        if (argc > 0)
        {
            const char* stream = StringValueCStr(argv[0]);
            if (stream)
                p->set_source( stream, std::strlen(stream) );
        }
        return self;
    }

    //
    // change the input source
    static VALUE set_source(VALUE self, VALUE data)
    {
        Parser* p = get_parser(self);

        const char* stream = StringValueCStr(data);
        if (stream)
            p->set_source( stream, std::strlen(stream) );

        return self;
    }

    //
    // eof?
    static VALUE eof(VALUE self, VALUE data)
    {
        return get_parser(self)->is_eof();
    }

    //
    // parses an entire stream
    static VALUE read(VALUE self, VALUE data)
    {
        const char* stream = StringValueCStr(data);
        if (stream)
            return get_parser(self)->parse(stream, std::strlen(stream) );
        return Qnil;
    }

    //
    // gets the next token in the current stream
    static VALUE next(VALUE self, VALUE data)
    {
        return get_parser(self)->next();
    }

    //
    // signal handler
    static void die(int sig)
    {
        exit(-1);
    }
}


//
// ruby calls this to load the extension
extern "C"
void Init_edn_turbo(void)
{
    struct sigaction a;
    a.sa_handler = edn::die;
    sigemptyset(&a.sa_mask);
    a.sa_flags = 0;
    sigaction(SIGINT, &a, 0);

    // pass things back as utf-8
    if (!setlocale( LC_ALL, "" )) {
        std::cerr << "Error setting locale" << std::endl;
        return;
    }

    edn::rb_mEDNT = rb_define_module("EDNT");

    // bind the ruby Parser class to the C++ one
    VALUE rb_cParser = rb_define_class_under(edn::rb_mEDNT, "Parser", rb_cObject);
    rb_define_alloc_func(rb_cParser, edn::alloc_obj);
    rb_define_method(rb_cParser, "initialize", (VALUE(*)(ANYARGS)) &edn::initialize, -1 );
    rb_define_method(rb_cParser, "ext_eof", (VALUE(*)(ANYARGS)) &edn::eof, 0 );

    rb_define_method(rb_cParser, "set_input", (VALUE(*)(ANYARGS)) &edn::set_source, 1 );
    rb_define_method(rb_cParser, "parse", (VALUE(*)(ANYARGS)) &edn::read, 1 );
    rb_define_method(rb_cParser, "read", (VALUE(*)(ANYARGS)) &edn::next, 0 );

    // bind ruby methods we'll call - these should be defined in edn_turbo.rb
    edn::EDN_MODULE_SYMBOL             = rb_intern("EDN");
    edn::EDNT_MAKE_SYMBOL_METHOD       = rb_intern("symbol");
    edn::EDNT_MAKE_LIST_METHOD         = rb_intern("list");
    edn::EDNT_MAKE_SET_METHOD          = rb_intern("set");
    edn::EDNT_MAKE_BIG_DECIMAL_METHOD  = rb_intern("big_decimal");
    edn::EDNT_TAGGED_ELEM_METHOD       = rb_intern("tagged_element");
    edn::EDNT_EXTENDED_VALUE_METHOD    = rb_intern("extend_for_meta");

    edn::EDNT_STRING_TO_I_METHOD       = rb_intern("to_i");
    edn::EDNT_STRING_TO_F_METHOD       = rb_intern("to_f");

    // so we can return EOF directly
    VALUE edn_module                   = rb_const_get(rb_cObject, edn::EDN_MODULE_SYMBOL);
    edn::EDNT_EOF_CONST                = rb_const_get(edn_module, rb_intern("EOF"));
}
