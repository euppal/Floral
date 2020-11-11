#include <inttypes.h>
#include <stddef.h>

#define FloralTypeIDChar ch
#define FloralTypeIDUChar uch
#define FloralTypeIDShort i16
#define FloralTypeIDUShort u16
#define FloralTypeIDInt32 i32
#define FloralTypeIDUInt32 u32
#define FloralTypeIDWideChar wch
#define FloralTypeIDWideUChar uwch
#define FloralTypeIDInt i
#define FloralTypeIDUInt u
#define FloralTypeIDPointer u
#define FloralTypeIDInt i
#define _floralmacro__cat(a, b) a ## b
#define _floralmacro_cat(a, b) _floralmacro__cat(a, b)
#define FloralTypeIDSuffixBuilder(a, b) _floralmacro_cat(_floralmacro_cat(a, _), b)
#define FloralID(name, types) _floralmacro_cat(_floralmacro_cat(_floralmacro_cat(floralid_, name), _), types)

#define _floralmacro__paste(x) x
#define _floralmacro_paste(x) _floralmacro__paste(x)
#define FloralPointer(type) _floralmacro_cat(_floralmacro_paste(type), *)

typedef void FloralVoid;

typedef int8_t FloralInt8;
typedef uint8_t FloralUInt8;
typedef int16_t FloralInt16;
typedef uint16_t FloralUInt16;
typedef int32_t FloralInt32;
typedef uint32_t FloralUInt32;
typedef int64_t FloralInt64;
typedef uint64_t FloralUInt64;

typedef FloralUInt8 FloralByte;
typedef FloralUInt16 FloralWord;
typedef FloralUInt32 FloralDWord;
typedef FloralUInt64 FloralQWord;

typedef char FloralChar;
typedef FloralUInt8 FloralUChar;
typedef FloralInt16 FloralShort;
typedef FloralUInt16 FloralUShort;
typedef FloralInt32 FloralWideChar;
typedef FloralUInt32 FloralWideUChar;
typedef FloralInt64 FloralInt;
typedef FloralUInt64 FloralUInt;

typedef FloralChar* FloralCString;

typedef int64_t FloralBool;
#define FloralTrue 1
#define FloralFalse 0

#define FloralNull NULL

#define POSIX_BLACK_VAL 30
#define POSIX_RED_VAL 31
#define POSIX_GREEN_VAL 32
#define POSIX_YELLOW_VAL 33
#define POSIX_BLUE_VAL 34
#define POSIX_MAGENGA_VAL 35
#define POSIX_CYAN_VAL 36
#define POSIX_WHITE_VAL 37

#define POSIX_BLACK "\e[30m"
#define POSIX_RED "\e[31m"
#define POSIX_GREEN "\e[32m"
#define POSIX_YELLOW "\e[33m"
#define POSIX_BLUE "\e[34m"
#define POSIX_MAGENGA "\e[35m"
#define POSIX_CYAN "\e[36m"
#define POSIX_WHITE "\e[37m"

#define POSIX_RESET "\e[0;0m"
