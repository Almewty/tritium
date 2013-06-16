#ifndef ENGINE_H
#define ENGINE_H

#include "stdafx.h"

// Common console msg
void dbg_msg(int color, const char* sys, const char* fmt, ...);

bool str_cmp(char* str1, char* str2, int count);

bool valid_name(char* str, unsigned int len);

HANDLE proc_exists(char* name);

//void time_str(char* buf);

// CRC
/*class Crc
{
public:
    typedef unsigned long Type;

    Crc (Type key)
        : _key (key), _register (0)
    {}
    Type Done ()
    {
        Type tmp = _register;
        _register = 0;
        return tmp;
    }
protected:
    Type _key; // really 33-bit key, counting implicit 1 top-bit
    Type _register;
};

class FastCrc: public Crc
{
public:
    FastCrc (Crc::Type key) 
        : Crc (key) 
    {
        _table.Init (key);
    }
    void PutByte (unsigned byte);
private:
    class Table
    {
    public:
        Table () : _key (0) {}
        void Init (Crc::Type key);
        Crc::Type operator [] (unsigned i)
        {
            return _table [i];
        }
    private:
        Crc::Type _table [256];
        Crc::Type _key;
    };
private:
    static Table _table;
};

//FastCrc::Table FastCrc::_table;*/


#endif