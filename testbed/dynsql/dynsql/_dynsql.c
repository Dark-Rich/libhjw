// vim:fdm=marker:nu:nowrap

#include <Python.h>
#include <string.h>

// ref: http://www.sics.se/~adam/pt/ 

#define _(x) (*(x))
#define CNTX_HEAD int __resume_lineno
#define CNTX_VAR(T, name, cntx) T * name = &((cntx).name)
#define CNTX_INIT(cntx) (cntx).__resume_lineno = 0
#define BEGIN(cntx) switch((cntx).__resume_lineno) { default: case 0:
// !!note: do not call YIELD() in an switch statement
#define YIELD(cntx, ret) { (cntx).__resume_lineno = __LINE__; return (ret); }   \
    case __LINE__: 
#define END() }

typedef enum {
    INIT = 0,
    PLAIN,                          // got a plain text
    VAR,                            // got a variable
    EXPR,                           // got an expression
    FLAG,                           // got a flag
    SUB_START,                      // a sub tree start
    SUB_END,                        // a sub tree end
    SYNTAX_ERR,                     // syntax err
} parserEventType;

typedef struct {
    parserEventType type;
    char kind;                      // '?'/'$'/'#'/'['/'{'
    int start;                      // [start, end)
    int end;
} parserEvent;

typedef struct {
    CNTX_HEAD;
    char c;
    const char * p;                 // current position
    const char * base;              // base position
    int all_whitespace;             // does from base to p are all white spaces
    int nest_cnt;                   // '[' nest level

    parserEvent curr;               // current parser event
} parserCntx;

typedef struct {
    PyObject_VAR_HEAD
    PyObject * tmpl;                    // the template
    parserCntx cntx;                    // parser contex
} dynsqlParser;

static inline void reset_cntx(parserCntx * cntx, const char * tmpl) {
    CNTX_INIT(*cntx);                   // this will make _parse run from the begining
    cntx->p = cntx->base = tmpl;
    cntx->all_whitespace = 1;
    cntx->nest_cnt = 0;                 
    cntx->curr.type = INIT;             // reset event
}

static inline void init_cntx(parserCntx * cntx, const char * tmpl) {
    memset(cntx, 0, sizeof(parserCntx));
    reset_cntx(cntx, tmpl);
}

#define PARSER_VAR(T, name) CNTX_VAR(T, name, parser->cntx)
#define PARSER_BEGIN() BEGIN(parser->cntx)
#define PARSER_END() END()
#define PARSER_YIELD(T, K, S, E) {               \
            parser->cntx.curr.type = (T);        \
            parser->cntx.curr.kind = (K);        \
            parser->cntx.curr.start = (S);       \
            parser->cntx.curr.end = (E);         \
        }                                   \
        YIELD(parser->cntx, 1)

#define BEGIN_PLAIN() {                     \
            _(base) = _(p);                 \
            _(all_whitespace) = 1;          \
        }

#define STORE_PLAIN() if ( _(base) != _(p) && !_(all_whitespace) ) {    \
            PARSER_YIELD(PLAIN, '\0', _(base) - tmpl, _(p) - tmpl);  \
        }
    
// input param: tmpl, p
// output param: p
//      on success, *p point to the next address of last " or '
// return true/false
int _consume_str(const char * tmpl, const char ** p) {
    char q = *_(p)++;
    char c;
    while (( c = *_(p)++ )) {
        if (c == q)
            return 1;
        else if (c == '\\')
            ++_(p);
    }
    return 0;
}

// input param: tmpl, p
//      *p point to '$' or '#' or '?'
// output param: base, p
//      on success, *p point to the next address of last char in the var
// return true/false
int _consume_var(const char * tmpl, const char ** base, 
        const char **p, int * is_expr) {
    char c;
    int is_enclosed, nest_cnt, paren_cnt;

    _(base) = ++_(p);                       // base point to the first char after '$#?'
    *is_expr = 0;
    is_enclosed = *_(base) == '(';
    nest_cnt = 0;
    paren_cnt = is_enclosed ? -1 : 0;

    while (( c = *_(p) )) {
        switch (c) {
        case '(':
            ++nest_cnt;
            ++paren_cnt;
            break;
        case ')':
            if (--nest_cnt == 0) {
                ++_(p);
                goto end;
            }
            if (nest_cnt < 0)
                return 0;
            break;
        case '\'':
        case '"':
            if (!_consume_str(tmpl, p))
                return 0;
            --_(p);
            break;
        default:
            if (!isprint(c))
                return 0;
            if (!isalnum(c) && c != '_') {
                if (nest_cnt == 0)
                    goto end;
                *is_expr = 1;
            }
        }
        ++_(p);
    }
end:
    // _(p) finally point to the next address of the var or expression
    if (nest_cnt != 0)
        return 0;

    if ((_(p) - _(base) - (is_enclosed ? 2 : 0)) <= 0)
        return 0;

    if (paren_cnt > 0)
        *is_expr = 1;

    return 1;

}

// input param: tmpl, p
//      *p point to '{'
// output param: base, p
//      on success, *p point to the next address of last char in the var
// return true/false
int _consume_flag(const char * tmpl, const char ** base,
        const char **p) {
    char c;
    _(base) = ++_(p);
    while (( c = *_(p) )) {
        switch (c) {
        case '{':
            return 0;                   // error, not support nest flag
        case '}':
            if ( _(base) == _(p) )      // empty flag
                return 0;
            return 1;
        default:
            break;
        }
        ++_(p);
    }
    return 0;
}

// main parser function
// 1 for yielding events, 0 for finish
static inline int _parse(dynsqlParser * parser) {
    PARSER_VAR(char, c);
    PARSER_VAR(const char *, p);
    PARSER_VAR(const char *, base);
    PARSER_VAR(int, all_whitespace);

    // local variable can be used. but it must be read-only, or not used 
    // cross yields
    const char * tmpl = PyString_AS_STRING(parser->tmpl);
    int is_expr;

PARSER_BEGIN();

    BEGIN_PLAIN();

    while (( _(c) = *_(p) )) {
        switch ( _(c) ) {
        case '?':
        case '#':
        case '$':  
            goto STORE_VAR_OR_EXPR;
        case '{':
            goto FLAG;
        case '[':
            goto TREE_DOWN;
        case ']':
            goto TREE_UP;
        case '}':
            goto ERR;
        case '\'':
        case '"':
            if (!_consume_str(tmpl, p))
                goto ERR;
            _(all_whitespace) = 0;
            continue;
        default:
            if (isprint( _(c) ) && !isspace( _(c) ))
                _(all_whitespace) = 0;
            ++_(p);
            continue;
        }

    FLAG:
        STORE_PLAIN();
        
        if (!_consume_flag(tmpl, base, p))
            goto ERR;
        PARSER_YIELD(FLAG, _(c), _(base) -  tmpl, _(p) - tmpl);
        ++_(p);

        BEGIN_PLAIN();
        continue;

    STORE_VAR_OR_EXPR:
        STORE_PLAIN();

        if (!_consume_var(tmpl, base, p, &is_expr))
            goto ERR;

        if (is_expr) {
            PARSER_YIELD(EXPR, _(c), _(base) - tmpl, _(p) - tmpl);
        }
        else if (*_(base) == '(') {
            PARSER_YIELD(VAR, _(c), _(base) - tmpl + 1, _(p) - tmpl - 1);
        }
        else {
            PARSER_YIELD(VAR, _(c), _(base) - tmpl, _(p) - tmpl);
        }

        BEGIN_PLAIN();
        continue;

    TREE_DOWN:
        STORE_PLAIN();

        ++parser->cntx.nest_cnt;

        // yield
        ++_(p);
        PARSER_YIELD(SUB_START, _(c), _(p) - tmpl, -1);  // we don't know end so -1

        BEGIN_PLAIN();
        continue;
    TREE_UP:
        STORE_PLAIN();
        
        // some check 
        if (parser->cntx.curr.type == SUB_START)            // empty child
            goto ERR;
        if (--parser->cntx.nest_cnt < 0)
            goto ERR;
        
        // yield
        PARSER_YIELD(SUB_END, _(c), -1, _(p) - tmpl);       // we don't know start so -1

        ++_(p);
        BEGIN_PLAIN();
        continue;

    }
    STORE_PLAIN();

    if (parser->cntx.nest_cnt != 0)
        goto ERR;
    return 0;

    // once error then finished
    ERR:
        PARSER_YIELD(SYNTAX_ERR, _(c), _(base) - tmpl, _(p) - tmpl);              
        return 0;

PARSER_END();

}


/* Python Interface functions */

// standard functions 

static int dynsqlParser_init(dynsqlParser * parser, PyObject * args, PyObject * kw) {
    // init template string
    parser->tmpl = NULL;
    if (!PyArg_UnpackTuple(args, "dynsqlParser", 0, 1, &parser->tmpl))
        return -1;

    if (parser->tmpl) {
        if (!PyString_Check(parser->tmpl)) {
            PyErr_SetString(PyExc_ValueError, "a string is expected");
            goto failed;
        }
        Py_INCREF(parser->tmpl);
    }
    else {
        parser->tmpl = PyString_FromString("");
    }

    // init template contex
    init_cntx(&parser->cntx, PyString_AS_STRING(parser->tmpl));
    return 0;

failed:
    Py_DECREF(parser->tmpl);
    parser->tmpl = NULL;
    return -1;
}

static void dynsqlParser_dealloc(dynsqlParser * parser) {
    // !!! note that this function will be called when init is failed
    // so some fields may be not fill in this case
    Py_XDECREF(parser->tmpl);
    parser->ob_type->tp_free((PyObject *)parser);
}

static PyObject * dynsqlParser_iternext(dynsqlParser * parser) {
    if (_parse(parser) == 0)                        // finished
        return NULL;

    parserEvent * ev = &parser->cntx.curr;
    return Py_BuildValue("(icii)", ev->type, ev->kind, ev->start, ev->end);
}

// methods

static PyObject * dynsqlParser_set(dynsqlParser * parser, PyObject * tmpl) {
    if (!PyString_Check(tmpl)) {
        PyErr_SetString(PyExc_ValueError, "a string is expected");
        return NULL;
    }
    Py_DECREF(parser->tmpl);
    Py_INCREF(tmpl);
    parser->tmpl = tmpl;
    reset_cntx(&parser->cntx, PyString_AS_STRING(parser->tmpl));

    Py_RETURN_NONE;
}

static PyMethodDef dynsqlParser_methods[] = {
    {"set", (PyCFunction)dynsqlParser_set, METH_O, "set a new template string for the parser"},
    { NULL }
};

PyDoc_STRVAR(dynsqlParser_doc, "syntax err includes:\n"\
    "bracket mismatch\n"\
    "empty sub tree\n"\
    "syntax err in variable/expression\n"\
    "open string literal\n");


PyTypeObject dynsqlParser_Type = {
    PyObject_HEAD_INIT(&PyType_Type)
    0,
    "dynsqlParser",
    sizeof(dynsqlParser),
    0,
    (destructor)dynsqlParser_dealloc,       /* tp_dealloc */
    0,                                      /* tp_print */
    0,                                      /* tp_getattr */
    0,                                      /* tp_setattr */
    0,                                      /* tp_compare */
    0,                                      /* tp_repr */
    0,                                      /* tp_as_number */
    0,                                      /* tp_as_sequence */
    0,                                      /* tp_as_mapping */
    0,                                      /* tp_hash */
    0,                                      /* tp_call */
    0,                                      /* tp_str */
    0,                                      /* tp_getattro */
    0,                                      /* tp_setattro */
    0,                                      /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT,                     /* tp_flags */
    dynsqlParser_doc,                       /* tp_doc */
    0,                                      /* tp_traverse */
    0,                                      /* tp_clear */
    0,                                      /* tp_richcompare */
    0,                                      /* tp_weaklistoffset */
    PyObject_SelfIter,                      /* tp_iter */
    (iternextfunc)dynsqlParser_iternext,    /* tp_iternext */
    dynsqlParser_methods,                   /* tp_methods */
    0,                                      /* tp_members */
    0,                                      /* tp_getset */
    0,                                      /* tp_base */
    0,                                      /* tp_dict */
    0,                                      /* tp_descr_get */
    0,                                      /* tp_descr_set */
    0,                                      /* tp_dictoffset */
    (initproc)dynsqlParser_init,            /* tp_init */
    PyType_GenericAlloc,                    /* tp_alloc */
    PyType_GenericNew,                      /* tp_new */
    0,                                      /* tp_free */
};

void init_dynsql(void) {
    PyObject * mod;

    // Create the module
    mod = Py_InitModule3("_dynsql", NULL, "dynsql c module");
    if (mod == NULL) {
        return;
    }

    if (PyType_Ready(&dynsqlParser_Type) < 0) {
        return;
    }

    Py_INCREF(&dynsqlParser_Type);
    PyModule_AddObject(mod, "dynsqlParser", (PyObject *)&dynsqlParser_Type);

    PyModule_AddIntConstant(mod, "PLAIN", PLAIN);
    PyModule_AddIntConstant(mod, "VAR", VAR);
    PyModule_AddIntConstant(mod, "EXPR", EXPR);
    PyModule_AddIntConstant(mod, "FLAG", EXPR);
    PyModule_AddIntConstant(mod, "SUB_START", SUB_START);
    PyModule_AddIntConstant(mod, "SUB_END", SUB_END);
    PyModule_AddIntConstant(mod, "SYNTAX_ERR", SYNTAX_ERR);
}
