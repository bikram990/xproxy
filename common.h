#ifndef COMMON_H
#define COMMON_H

#define DECLARE_GENERAL_CONSTRUCTOR(Class) \
    Class() { TRACE_THIS_PTR; }

#define DECLARE_GENERAL_DESTRUCTOR(Class) \
    virtual ~Class() { TRACE_THIS_PTR; }

#define CLEAN_MEMBER(member) \
    if(member) { \
        delete member; \
        member = NULL; \
    }

#define CLEAN_ARRAY_MEMBER(member) \
    if(member) { \
        delete [] member; \
        member = NULL; \
    }

#endif // COMMON_H
