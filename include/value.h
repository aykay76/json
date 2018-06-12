#ifndef _value_h
#define _value_h

enum State { Object, Array, Number, String, True, False, Null };

struct _value;

typedef struct _array {
    struct _value **values;
    int length;
} array_t;

typedef struct _namevalue {
    wchar_t *name;
    struct _value *value;
    struct _namevalue *nextpair;
} namevalue_t;

typedef struct _object {
    namevalue_t *firstpair;
} object_t;

typedef struct _value {
    enum State type;

    object_t *object;
    array_t *array;
    double number;
    wchar_t *string;
} value_t;

#endif 