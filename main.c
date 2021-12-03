#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <wctype.h>
#include <wchar.h>
#include <errno.h>
#include <time.h>
#include "include/value.h"

// TODO: make shared library so that it can be used by other programs I intend to develop
// TODO: need better EOF / error checking when reading from file
// TODO: need to handle syntax errors

value_t* parseValue(FILE *fh);
wchar_t* parseString(FILE *fh);
object_t* parseObject(FILE *fh);
array_t* parseArray(FILE *fh);
double parseNumber(FILE *fh);
char parseLiteral(FILE *fh, wchar_t *staticvalue);

int col = 1;
int row = 1;

wchar_t skipWhitespace(FILE *fh)
{
    wchar_t c = getwc(fh); col++;
   
    while ((c == 0x0009 || c == 0x000a || c == 0x000d || c == 0x0020))
    {
        if (c == 0x000a) 
        {
            col = 0;
            row++;
        }
        c = getwc(fh); col++;
    }

    return c;
}

wchar_t* parseString(FILE *fh)
{
    fwprintf(stderr, L"Parsing string %d,%d\n", row, col);
    int len = 128;
    wchar_t *string = (wchar_t *)malloc(sizeof(wchar_t) * len);
    memset(string, 0, sizeof(wchar_t) * len);
    int pos = 0;
    wchar_t c = 0;
    char done = 0;
    char esc = 0;

    while (!done)
    {
        c = getwc(fh); col++;
        // putwc(c, stderr);
    
        switch (c)
        {
            case WEOF:
                break;
            case 0x0022: // closing quotation marks
                if (esc) { esc = 0; string[pos++] = 0x0022; }
                else { done = 1; }
                break;
            case 0x005c: // reserve solidus, escaping the next character(s)
                if (esc) { string[pos++] = 0x005c; esc = 0; }
                else { esc = 1; }
                break;
            case 0x002f: // forward solidus
                if (esc) { string[pos++] = 0x002f; esc = 0; }
                else { string[pos++] = 0x002f; }
                break;
            case 'b': // 0x0062
                if (esc) { string[pos++] = 0x0008; esc = 0; }
                else { string[pos++] = 'b'; }
                break;
            case 'f':
                if (esc) { string[pos++] = 0x000c; esc = 0; }
                else { string[pos++] = 'f'; }
                break;
            case 'n':
                if (esc) { string[pos++] = 0x000a; esc = 0; }
                else { string[pos++] = 'n'; }
                break;
            case 'r':
                if (esc) { string[pos++] = 0x000d; esc = 0; }
                else { string[pos++] = 'r'; }
                break;
            case 't':
                if (esc) { string[pos++] = 0x0009; esc = 0; }
                else { string[pos++] = 't'; }
                break;
            case 'u':
                if (esc) 
                {
                    // expect 4 hex digits
                    wchar_t hex[4];
                    hex[0] = getwc(fh);
                    hex[1] = getwc(fh);
                    hex[2] = getwc(fh);
                    hex[3] = getwc(fh);
                    string[pos++] = (wchar_t)wcstol(hex, NULL, 16);
                }
                else
                {
                    string[pos++] = 'u';
                }
                break;
            default:
                string[pos++] = c;
                break;
        }

        if (pos == len - 1)
        {
            len <<= 1;
            // extend the string
            wchar_t *newstring = (wchar_t *)malloc(sizeof(wchar_t) * len);
            memset(newstring, 0, sizeof(wchar_t) * len);
            wcscpy(newstring, string);
            free(string);
            string = newstring;
        }
    }

    fwprintf(stderr, L"%ls\n", string);

    return string;
}

object_t* parseObject(FILE *fh)
{
    fwprintf(stderr, L"Parsing object\n");
    object_t *object = (object_t *)malloc(sizeof(object_t));
    namevalue_t *currentpair = 0;

    char done = 0;

    while (!done)
    {
        wchar_t c = skipWhitespace(fh);
    
        if (c == 0x0022) // double quote, start of a name
        {
            namevalue_t *namevalue = (namevalue_t *)malloc(sizeof(namevalue_t));
            namevalue->nextpair = 0;

            if (currentpair == 0)
            {
                currentpair = namevalue;
                object->firstpair = currentpair;
            }
            else
            {
                currentpair->nextpair = namevalue;
                currentpair = namevalue;
            }

            currentpair->name = parseString(fh);
        }
        else if (c == 0x007d) // right curly bracket, object is done
        {
            fwprintf(stderr, L"Found end of object: %d, %d\n", row, col);
            done = 1;
        }
        else if (c == 0x003a) // colon, now parse the value
        {
            currentpair->value = parseValue(fh);
        }
        else if (c == 0x002c) // comma, next name/value pair
        {
        }
    }

    return object;
}

array_t* parseArray(FILE *fh)
{
    fwprintf(stderr, L"Parsing array\n");
    array_t *array = (array_t *)malloc(sizeof(array_t));
    array->length = 0;
    array->values = 0;
    wchar_t c = 0;
    char done = 0;

    while (!done)
    {
        c = skipWhitespace(fh);
    
        if (c == 0x005d) // right square bracket, array is done
        {
            fwprintf(stderr, L"Found end of array: %d, %d\n", row, col);
            done = 1;
        }
        else if (c == 0x002c) // comma, next value
        {
        }
        else
        {
            ungetwc(c, fh); col--;
            value_t* value = parseValue(fh);

            value_t **values = (value_t **)malloc(sizeof(value_t *) * array->length + 1);
            for (int i = 0; i < array->length; i++)
            {
                values[i] = array->values[i];
            }
            values[array->length] = value;

            free(array->values);
            array->values = values;
            array->length++;
        }
    }

    return array;
}

double parseNumber(FILE *fh)
{
    fwprintf(stderr, L"Parsing number\n");
    double number = 0.0;
    char done = 0;
    wchar_t c = 0;
    wchar_t *set = L"+-01234566789.eE";
    wchar_t numberString[128];
    int i = 0;
    wchar_t *end;

    while (!done)
    {
        c = getwc(fh); col++;
        if (wcschr(set, c) == 0)
        {
            numberString[i] = 0;
            ungetwc(c, fh); col--;
            done = 1;
            number = wcstod(numberString, &end);
        }
	    else
        {
	        numberString[i++] = c;
        }
    }

    fprintf(stderr, "%f\n", number);

    return number;
}

char parseLiteral(FILE *fh, wchar_t *staticvalue)
{
    fwprintf(stderr, L"Parsing literal: %ls\n", staticvalue);
    wchar_t c = getwc(fh); col++;
    int index = 1;

    while (index < wcslen(staticvalue))
    {
        if (c == staticvalue[index])
        {
            index++;
            c = getwc(fh); col++;
            if (c == 0x000a)
            {
                row++;
                col = 0;
            }
        }
        else
        {
            return 0;    
        }
    }

    return 1;
}

value_t* parseValue(FILE *fh)
{
    // fwprintf(stderr, L"Parsing value\n");
    wchar_t c = 0;
    char done = 0;
    value_t* value = (value_t *)malloc(sizeof(value_t));

    c = skipWhitespace(fh);

    switch (c)
    {
        case WEOF:
            done = 1;
            break;
        case 0x007b: // left curly bracket
            value->type = Object;
            value->object = parseObject(fh);
            break;
        case 0x005b: // left square bracket
            value->type = Array;
            value->array = parseArray(fh);
            break;
        case 0x002d: // minus sign
        case 0x0030: // zero
        case 0x0031: // one
        case 0x0032: // two
        case 0x0033: // three
        case 0x0034: // four
        case 0x0035: // five
        case 0x0036: // six
        case 0x0037: // seven
        case 0x0038: // eight
        case 0x0039: // nine
            value->type = Number;
            ungetwc(c, fh);
            value->number = parseNumber(fh);
            break;
        case 0x0022: // quotation marks
            value->type = String;
            value->string = parseString(fh);
            break;
        case 0x0074: // 't' - true
            value->type = True;
            if (parseLiteral(fh, L"true") == 0)
            {
                fwprintf(stderr, L"Error parsing true");
            }
            break;
        case 0x0066: // 'f' - false
            value->type = False;
            parseLiteral(fh, L"false");
            break;
        case 0x006e: // 'n' - null
            value->type = Null;
            parseLiteral(fh, L"null");
            break;
    }

    return value;
}

// Exported functions, main will disappear when i'm done testing

value_t* jsonFromFile(char *filename)
{
    value_t *value = 0;
    FILE *fh = fopen(filename, "r");
    if (fh)
    {
        value = parseValue(fh);
        fclose(fh);
    }
    return value;
}

value_t* jsonFromString(wchar_t *input)
{
    value_t *value = 0;
    FILE *fh = fmemopen(input, wcslen(input), "r");
    if (fh)
    {
        value = parseValue(fh);
        fclose(fh);
    }
    return value;
}

value_t* getValue(wchar_t *name)
{
    // TODO: search through object to find requested value
    // input will be in the form of name.name.name[2].name so need to parse array index and dotted references
    return 0;
}

value_t* createValue()
{
    return 0;
}

unsigned char replaceValue(wchar_t *path, value_t *value)
{
    return 1;
}

void freeJson(value_t *json)
{
    // TODO: iterate through whole object freeing all string, arrays and subobjects
    switch (json->type)
    {
        case Object: {
            namevalue_t *pair = json->object->firstpair;
            while (pair != 0)
            {
                namevalue_t *next = pair->nextpair;
                free(pair->name);
                freeJson(pair->value);
                free(pair);
                pair = next;
            }
            free(json->object);
        }
        break;
        case Array: {
            for (int i = 0; i < json->array->length; i++)
            {
                freeJson(json->array->values[i]);
            }
            free(json->array->values);
            free(json->array);
        }
        break;
        case String: {
            free(json->string);
        }
        break;
        case Null:
        case True:
        case False:
        case Number:
        default:
            // nothing to do
        break;
    }

    free(json);
}

int main(int argc, char** argv)
{
    wchar_t c;
    char iws = 1;
    char esc = 0;
    FILE* fh;

    // TODO: obviously need better command line argument handling than this!
    if (argc > 0)
    {
        fh = fopen(argv[1], "r");
    }

    if (fh)
    {
        struct timespec t1, t2;
        clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &t1);
        value_t *value = parseValue(fh);
        clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &t2);
        freeJson(value);
        fclose(fh);

        fprintf(stdout, "Parsed %d lines in %lf sec\n", row, (double)((t2.tv_nsec - t1.tv_nsec) * 1e-9));
    }
    else
    {
        fprintf(stderr, "Error opening file: %d\n", errno);
    }

    return 0;
}
