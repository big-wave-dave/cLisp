/* This is my implementation of Daniel Holden's tutorial "Build Your Own Lisp" at buildyourownlisp.com. My formatting is a little different, I implemented things a little differently in some places (just a personal preference kind of deal), and added a lot of comments, but all in all it was a great tutorial and a great way to become more familiar with C and put myself in the shoes (at least a tiny bit!) of language developers. Major kudos to him. */

//Mpc is a parser made by Daniel Holden
#include "mpc.h"

/* This preprocessor conditional statement is just for those who compile this on a windows system. */
#ifdef _WIN32
static char buffer[2048];
char *readline(char *prompt)
{
    fputs(prompt, stdout);
    fgets(buffer, 2048, stdin);
    char *cpy = malloc(strlen(buffer)+1);
    strcpy(cpy, buffer);
    cpy[strlen(cpy)-1] = '\0';
    return cpy;
}

void add_history(char *unused) {}

/* If you're not using windows, include these headers to let the user of Lispy edit their command line entry more easily. */
#else
#include <editline/readline.h>
#include <editline/history.h>
#endif

/* These are the declartions for the various inputs the parser can expect. */
mpc_parser_t *Number;
mpc_parser_t *Symbol;
mpc_parser_t *String;
mpc_parser_t *Comment;
mpc_parser_t *Sexpr;
mpc_parser_t *Qexpr;
mpc_parser_t *Expr;
mpc_parser_t *Lispy;

/* Forward declarations. */
struct lval;
struct lenv;
typedef struct lval lval;
typedef struct lenv lenv;

/* Lisp Value */
enum { LVAL_ERR, LVAL_NUM, LVAL_SYM, LVAL_STR, LVAL_FUN, LVAL_SEXPR, LVAL_QEXPR };

typedef lval*(*lbuiltin)(lenv*, lval*);

struct lval 
{
    int type;

    //Basics
    long num;
    char *err;
    char *sym;
    char *str;

    //Functions
    lbuiltin builtin;
    lenv *env;
    lval *formals;
    lval *body;

    //Expressions
    int count;
    lval **cell;
};

lval *lval_err(char *fmt, ...);
lval *lval_num(long x);
lval *lval_sym(char *s);
lval *lval_str(char *s);
lval *lval_builtin(lbuiltin func);

lenv *lenv_new(void);

lval *lval_lambda(lval *formals, lval *body);
lval *lval_sexpr(void);
lval *lval_qexpr(void);

void lenv_del(lenv *e);
void lval_del(lval *v);

lenv *lenv_copy(lenv *e);

lval *lval_copy(lval *v);
lval *lval_add(lval *v, lval *x);
lval *lval_join(lval *x, lval *y);
lval *lval_pop(lval *v, int i);
lval *lval_take(lval *v, int i);

//These are the print functions. Gutenberg would be proud. 
void lval_print(lval *v);
void lval_print_expr(lval *v, char open, char close);
void lval_print_str(lval *v);
void lval_println(lval *v);

//Equality is a good thing. This is our version of affirmative action.
int lval_eq(lval *x, lval *y);

char *ltype_name(int t);

//Create the Lisp environment
struct lenv
{
    lenv *par;
    int count;
    char **syms;
    lval **vals;
};
lenv *lenv_new(void);
void lenv_del(lenv *e);
lenv *lenv_copy(lenv *e);
lval *lenv_get(lenv *e, lval *k);
void lenv_put(lenv *e, lval *k, lval *v);
void lenv_def(lenv *e, lval *k, lval *v);

/* Let's define some macros because, let's be honest, with some of these variable names, this code is hard enough to read as it is. */
#define LASSERT(args, cond, fmt, ...) \
    if(!(cond)) { lval *err = lval_err(fmt, ##__VA_ARGS__); lval_del(args); return err; }

#define LASSERT_TYPE(func, args, index, expect) \
    LASSERT(args, args->cell[index]->type == expect, \
            "Function '%s' passed incorrect type for argument %i. Got %s, expected %s. ", \
            func, index, ltype_name(args->cell[index]->type), ltype_name(expect))

#define LASSERT_NUM(func, args, num) \
    LASSERT(args, args->count == num, \
            "Function '%s' passed incorrect number of arguments. Got %i, expected %i. ", \
            func, args->count, num)

#define LASSERT_NOT_EMPTY(func, args, index) \
    LASSERT(args, args->cell[index]->count != 0, \
            "Function '%s' passed {} for argument %i. ", func, index);

//Prepare for built-ins. Lots and lots of built-ins.
lval *lval_eval(lenv *e, lval *v);
lval *builtin_lambda(lenv *e, lval *a);

//Built-in functions
lval *builtin_list(lenv *e, lval *a);
lval *builtin_head(lenv *e, lval *a);
lval *builtin_tail(lenv *e, lval *a);
lval *builtin_eval(lenv *e, lval *a);
lval *builtin_join(lenv *e, lval *a);
lval *builtin_op(lenv *e, lval *a, char *op);

//Add, subtract, multiply, divide
lval *builtin_add(lenv *e, lval *a);
lval *builtin_sub(lenv *e, lval *a);
lval *builtin_mul(lenv *e, lval *a);
lval *builtin_div(lenv *e, lval *a);

//Built-in variable controls
lval *builtin_var(lenv *e, lval *a, char *func);
lval *builtin_def(lenv *e, lval *a);
lval *builtin_put(lenv *e, lval *a);
lval *builtin_ord(lenv *e, lval *a, char *op);

//Operators!
lval *builtin_gt(lenv *e, lval *a);
lval *builtin_lt(lenv *e, lval *a);
lval *builtin_ge(lenv *e, lval *a);
lval *builtin_le(lenv *e, lval *a);

//Comparison
lval *builtin_cmp(lenv *e, lval *a, char *op);

//Equal or not?
lval *builtin_eq(lenv *e, lval *a);
lval *builtin_ne(lenv *e, lval *a);

//Various built-ins
lval *builtin_if(lenv *e, lval *a);
lval *builtin_load(lenv *e, lval *a);
lval *builtin_print(lenv *e, lval *a);
lval *builtin_error(lenv *e, lval *a);
void lenv_add_builtin(lenv *e, char *name, lbuiltin func);
void lenv_add_builtins(lenv *e);

//These functions are for evaluations.
lval *lval_call(lenv *e, lval *f, lval *a);
lval *lval_eval_sexpr(lenv *e, lval *v);
lval *lval_eval(lenv *e, lval *v);

//These functions are for reading. Reading is good for you, don't you know?
lval *lval_read_num(mpc_ast_t *t);
lval *lval_read_str(mpc_ast_t *t);
lval *lval_read(mpc_ast_t *t);



/************************************************/
/* A function that needs no introduction: main! */
/************************************************/
int main(int argc, char** argv)
{
    /* Basically, these are telling the mpc parser what kind of grammars it can expect us to define */
    Number = mpc_new("number");
    Symbol = mpc_new("symbol");
    String = mpc_new("string");
    Comment = mpc_new("comment");
    Sexpr = mpc_new("sexpr");
    Qexpr = mpc_new("qexpr");
    Expr = mpc_new("expr");
    Lispy = mpc_new("lispy");

    //Now we're actually defining the grammars
    mpca_lang(MPCA_LANG_DEFAULT,
            "                                             \
             number  : /-?[0-9]+/ ;                       \
             symbol  : /[a-zA-Z0-9_+\\-*\\/\\\\=<>!*]+/ ; \
             string  : /\"(\\\\.|[^\"])*\"/ ;             \
             comment : /;[^\\r\\n]*/ ;                    \
             sexpr   : '(' <expr>* ')' ;                  \
             qexpr   : '{' <expr>* '}' ;                  \
             expr    : <number>  | <symbol> | <string>    \
                     | <comment> | <sexpr>  | <qexpr>;    \
             lispy   : /^/ <expr>* /$/ ;                  \
            ",
            Number, Symbol, String, Comment, Sexpr, Qexpr, Expr, Lispy);
    
    //Now create an environment for our functions
    lenv *e = lenv_new();
    lenv_add_builtins(e);

    //Initialize the REPL
    if(argc == 1)
    {
        puts("Lispy Version 0.0.Good.Enough.1");
        puts("Press Ctrl+c to Exit \n");

        while(1)
        {
            //Print the prompt and get user input
            char *input = readline("lispy> ");
            add_history(input);

            mpc_result_t r;
            //Parse the input
            if(mpc_parse("<stdin>", input, Lispy, &r))
            {
                lval *x = lval_eval(e, lval_read(r.output));
                lval_println(x);
                lval_del;

                mpc_ast_delete(r.output);
            }
            //Error control
            else
            {
                mpc_err_print(r.error);
                mpc_err_delete(r.error);
            }
            //Clean up
            free(input);
        }
    }
    
    //File IO!
    if(argc >= 2)
    {
        //Loop over each filename
        for(int i = 1; i < argc; i++)
        {
            //Arg list with the filename as the only arg
            lval *args = lval_add(lval_sexpr(), lval_str(argv[i]));
            //Try to load the file
            lval *x = builtin_load(e, args);
            
            //Print any errors
            if(x->type == LVAL_ERR)
            {
                lval_println(x);
            }
            lval_del(x);
        }
    }
    //Clean up everything
    lenv_del(e);
    mpc_cleanup(8, Number, Symbol, String, Comment, Sexpr, Qexpr, Expr, Lispy);

    /************************************************\
    |* Here then, as I lay down the pen and proceed *|
    |** to seal up my confession, I bring the life **|
    |**** of that unhappy Henry Jekyll to an end ****|
    \************************************************/
    return 0;
    //Get it? Because the main function is done the program has terminated?
}



/* Function implementations. Get ready for some MAJOR meat and potatoes. */
lval *lval_err(char *fmt, ...)
{
    lval *v = malloc(sizeof(lval));
    v->type = LVAL_ERR;
    va_list va;
    va_start(va, fmt);
    v->err = malloc(512);
    vsnprintf(v->err, 511, fmt, va);
    v->err = realloc(v->err, strlen(v->err)+1);
    va_end(va);
    return v;
}
lval *lval_num(long x)
{
    lval *v = malloc(sizeof(lval));
    v->type = LVAL_NUM;
    v->num = x;
    return v;
}
lval *lval_sym(char *s)
{
    lval *v = malloc(sizeof(lval));
    v->type = LVAL_SYM;
    v->sym = malloc(strlen(s) + 1);
    strcpy(v->sym, s);
    return v;
}
lval *lval_str(char *s)
{
    lval *v = malloc(sizeof(lval));
    v->type = LVAL_STR;
    v->str = malloc(strlen(s) + 1);
    strcpy(v->str, s);
    return v;
}



lval *lval_builtin(lbuiltin func)
{
    lval *v = malloc(sizeof(lval));
    v->type = LVAL_FUN;
    v->builtin = func;
    return v;
}



lenv *lenv_new(void)
{
    lenv *e = malloc(sizeof(lenv));
    e->par = NULL;
    e->count = 0;
    e->syms = NULL;
    e->vals = NULL;
    return e;
}



lval *lval_lambda(lval *formals, lval *body)
{
    lval *v = malloc(sizeof(lval));
    v->type = LVAL_FUN;
    v->builtin = NULL;
    v->env = lenv_new();
    v->formals = formals;
    v->body = body;
    return v;
}
lval *lval_sexpr(void)
{
    lval *v = malloc(sizeof(lval));
    v->type = LVAL_SEXPR;
    v->count = 0;
    v->cell = NULL;
    return v;
}
lval *lval_qexpr(void)
{
    lval *v = malloc(sizeof(lval));
    v->type = LVAL_QEXPR;
    v->count = 0;
    v->cell = NULL;
    return v;
}



void lenv_del(lenv *e)
{
    for(int i =0; i < e->count; i++)
    {
        free(e->syms[i]);
        lval_del(e->vals[i]);
    }
    free(e->syms);
    free(e->vals);
    free(e);
}
void lval_del(lval *v)
{
    switch(v->type)
    {
        case LVAL_NUM:
            break;
        case LVAL_FUN:
            if(!v->builtin)
            {
                lenv_del(v->env);
                lval_del(v->formals);
                lval_del(v->type);
            }
            break;
        case LVAL_ERR: 
            free(v->err);
            break;
        case LVAL_SYM:
            free(v->sym);
            break;
        case LVAL_STR:
            free(v->str);
            break;
        case LVAL_QEXPR:
        case LVAL_SEXPR:
            for(int i = 0; i < v->count; i++)
            {
                lval_del(v->cell[i]);
            }
            free(v->cell);
            break;
    }
    free(v);
}



lenv *lenv_copy(lenv *e)
{
    lenv *n = malloc(sizeof(lenv));
    n->par = e->par;
    n->count = e->count;
    n->syms = malloc(sizeof(char*) * n->count);
    n->vals = malloc(sizeof(lval*) * n->count);
    for(int i = 0; i < e->count; i++)
    {
        n->syms[i] = malloc(strlen(e->syms[i]) + 1);
        strcpy(n->syms[i], e->syms[i]);
        n->vals[i] = lval_copy(e->vals[i]);
    }
    return n;
}



lval *lval_copy(lval *v)
{
    lval *x = malloc(sizeof(lval));
    x->type = v->type;
    switch(v->type)
    {
        case LVAL_FUN:
            if(v->builtin)
            {
                x->builtin = v->builtin;
            }
            else
            {
                x->builtin = NULL;
                x->env = lenv_copy(v->env);
                x->formals = lval_copy(v->formals);
                x->body = lval_copy(v->body);
            }
            break;
        case LVAL_NUM:
            x->num = v->num;
            break;
        case LVAL_ERR:
            x->err = malloc(strlen(v->err) + 1);
            strcpy(x->err, v->err);
            break;
        case LVAL_SYM:
            x->sym = malloc(strlen(v->sym) + 1);
            strcpy(x->str, v->str);
            break;
        case LVAL_STR:
            x->str = malloc(strlen(v->sym) + 1);
            strcpy(x->str, v->str);
            break;
        case LVAL_SEXPR:
        /* This is nifty. I've never thought about letting same-outcome switch cases fall through like this instead defining each case individually. */
        case LVAL_QEXPR:
            x->count = v->count;
            x->cell = malloc(sizeof(lval*) * x->count);
            for(int i = 0; i < x->count; i++)
            {
                x->cell[i] = lval_copy(v->cell[i]);
            }
            break;
    }
    return x;
}
lval *lval_add(lval *v, lval *x)
{
    v->count++;
    v->cell = realloc(v->cell, sizeof(lval*) * v->count);
    v->cell[v->count - 1] = x;
    return v;
}
lval *lval_join(lval *x, lval *y)
{
    for(int i = 0; i < y->count; i++)
    {
        x = lval_add(x, y->cell[i]);
    }
    free(y->cell);
    free(y);
    return x;
}
lval *lval_pop(lval *v, int i)
{
    lval *x = v->cell[i];
    memmove(&v->cell[i], &v->cell[i+1], sizeof(lval*) * (v->count-i-1));
    v->count--;
    v->cell = realloc(v->cell, sizeof(lval*) * v->count);
    return x;
}
lval *lval_take(lval *v, int i)
{
    lval *x = lval_pop(v, i);
    lval_del(v);
    return x;
}



//These are the print functions. Gutenberg would be proud. 
void lval_print(lval *v)
{
    switch(v->type)
    {
        case LVAL_FUN:
            if(v->builtin)
            {
                printf("<builtin>");
            }
            else
            {
                printf("\\ ");
                lval_print(v->formals);
                putchar(' ');
                lval_print(v->body);
                putchar(')');
            }
            break;
        case LVAL_NUM:   printf("%li", v->num); break;
        case LVAL_ERR:   printf("Error: %s", v->err); break;
        case LVAL_SYM:   printf("%s", v->sym); break;
        case LVAL_STR:   lval_print_str(v); break;
        case LVAL_SEXPR: lval_print_expr(v, '(', ')'); break;
        case LVAL_QEXPR: lval_print_expr('v', '{', '}'); break;
    }
}
void lval_print_expr(lval *v, char open, char close)
{
    putchar(open);
    for(int i = 0; i < v->count; i++)
    {
        lval_print(v->cell[i]);
        if(i != (v->count-1))
        {
            putchar(' ');
        }
    }
    putchar(close);
}
void lval_print_str(lval *v)
{
    //Make a copy of the string
    char *escaped = malloc(strlen(v->str)+1);
    strcpy(escaped, v->str);
    //Pass it through the escape function
    escaped = mpcf_escape(escaped);
    //Print it between double quotation characters
    printf("\"%s\"", escaped);
    //Clean up the copied string
    free(escaped);
}
void lval_println(lval *v)
{
    lval_print(v);
    putchar('\n');
}



//Equality is a good thing. This is our version of affirmative action.
int lval_eq(lval *x, lval *y)
{
    if(x->type != y->type)
    {
        return 0;
    }
    
    //Basically just some type checking
    switch(x->type)
    {
        case LVAL_NUM: return(x->num == y->num);
        case LVAL_ERR: return(strcmp(x->err, y->err) == 0);
        case LVAL_SYM: return(strcmp(x->sym, y->sym) == 0);
        case LVAL_STR: return(strcmp(x->str, y->str) == 0);
        case LVAL_QEXPR:
        case LVAL_SEXPR:
                       if(x->count != y->count)
                       {
                           return 0;
                       }
                       for(int i = 0; i < x->count; i++)
                       {
                           if(!lval_eq(x->cell[i], y->cell[i]))
                           {
                               return 0;
                           }
                       }
                       return 1;
                       break;
    }
    return 0;
}



char *ltype_name(int t)
{
    switch(t)
    {
        case LVAL_FUN:   return "Function";
        case LVAL_NUM:   return "Number";
        case LVAL_ERR:   return "Error";
        case LVAL_SYM:   return "Symbol";
        case LVAL_STR:   return "String";
        case LVAL_SEXPR: return "S-Expression";
        case LVAL_QEXPR: return "Q-Expression";
        default:         return "Unknown";
    }
}



//Create the Lisp environment
lval *lenv_get(lenv *e, lval *k)
{
    for(int i = 0; i < e->count; i++)
    {
        if(strcmp(e->syms[i], k->sym) == 0)
        {
            return lval_copy(e->vals[i]);
        }
    }
    if(e->par)
    {
        return lenv_get(e->par, k);
    }
    else
    {
        return lval_err("Unbound Symbol '%s'", k->sym);
    }
}
void lenv_put(lenv *e, lval *k, lval *v)
{
    for(int i = 0; i < e->count; i++)
    {
        if(strcmp(e->syms[i], k->sym) == 0)
        {
            lval_del(e->vals[i]);
            e->vals[i] = lval_copy(v);
            return;
        }
    }
    e->count++;
    e->vals = realloc(e->vals, sizeof(lval*) * e->count);
    e->syms = realloc(e->syms, sizeof(char*) * e->count);
    e->vals[e->count-1] = lval_copy(v);
    e->syms[e->count-1] = malloc(strlen(k->sym)+1);
    strcpy(e->syms[e->count-1], k->sym);
}
void lenv_def(lenv *e, lval *k, lval *v)
{
    while(e->par)
    {
        e = e->par;
    }
    lenv_put(e, k ,v);
}



//Prepare for built-ins. Lots and lots of built-ins.
lval *lval_eval(lenv *e, lval *v)
{
    if(v->type == LVAL_SYM)
    {
        lval *x = lenv_get(e, v);
        lval_del(v);
        return x;
    }
    if(v->type == LVAL_SEXPR)
    {
        return lval_eval_sexpr(e, v);
    }
    return v;
}



//Gee, look at all these built-ins!
//This function allows our users to utilize lambda expressions
lval *builtin_lambda(lenv *e, lval *a)
{
    LASSERT_NUM("\\", a, 2);
    LASSERT_TYPE("\\", a, 0, LVAL_QEXPR);
    LASSERT_TYPE("\\", a, 1, LVAL_QEXPR);

    for(int i = 0; i < a->cell[0]->count; i++)
    {
        LASSERT(a, (a->cell[0]->cell[i]->type == LVAL_SYM),
                "Cannot define non-symbol. Got %s, Expected %s. ",
                ltype_name(a->cell[0]->cell[i]->type), ltype_name(lval_sym));
    }

    lval *formals = lval_pop(a, 0);
    lval *body = lval_pop(a , 0);
    lval_del(a);
    return lval_lambda(formals, body);
}
//This one lets our users implement lists
lval *builtin_list(lenv *e, lval *a)
{
    a->type = LVAL_QEXPR;
    return a;
}
//This builds-in the head function
lval *builtin_head(lenv *e, lval *a)
{
    LASSERT_NUM("head", a, 1);
    LASSERT_TYPE("head", a, 0, LVAL_QEXPR);
    LASSERT_NOT_EMPTY("head", a, 0);

    lval* v = lval_take(a, 0);
    while(v->count > 1)
    {
        lval_del(lval_pop(v, 1));
    }
    return v;
}
//This implements the tail function
lval *builtin_tail(lenv *e, lval *a)
{
    LASSERT_NUM("tail", a, 1);
    LASSERT_TYPE("tail", a, 0, LVAL_QEXPR);
    LASSERT_NOT_EMPTY("tail", a, 0);

    lval *v = lval_take(a, 0);
    lval_del(lval_pop(v, 0));
    return v;
}
//This implements the eval ability
lval *builtin_eval(lenv *e, lval *a)
{
    LASSERT_NUM("eval", a, 1);
    LASSERT_TYPE("eval", a, 0, LVAL_QEXPR);

    lval *x = lval_take(a, 0);
    x->type = LVAL_SEXPR;
    return lval_eval(e, x);
}
//This implements the join keyword
lval *builtin_join(lenv *e, lval *a)
{
    for(int i = 0; i < a->count; i++)
    {
        LASSERT_TYPE("join", a, i, LVAL_QEXPR);
    }
    lval *x = lval_pop(a, 0);
    
    while(a->count)
    {
        lval* y = lval_pop(a, 0);
        x = lval_join(x, y);
    }
    lval_del(a);
    return x;
}
//Not-so-black ops! Just implementing built in mathematical operators
lval *builtin_op(lenv *e, lval *a, char *op)
{
    for(int i = 0; i < a->count; i++)
    {
        LASSERT_TYPE(op, a, i, LVAL_NUM);
    }
    lval *x = lval_pop(a, 0);

    if((strcmp(op, "-") == 0) && a->count == 0)
    {
        //If subtraction is required, just do some unary negation
        x->num = -x->num;
    }
    
    while(a->count > 0)
    {
        lval *y = lval_pop(a, 0);

        if(strcmp(op, "+") == 0) { x->num += y->num; }
        if(strcmp(op, "-") == 0) { x->num -= y->num; }
        if(strcmp(op, "*") == 0) { x->num *= y->num; }
        if(strcmp(op, "/") == 0) { 
            if(y->num == 0)
            {
                lval_del(x);
                lval_del(y);
                x = lval_err("Division by zero. Not cool. ");
                break;
            }
            x->num /= y->num;
        }
        lval_del(y);
    }
    return x;
}



//Add, subtract, multiply, divide. Handle these operators by calling their implementations.
lval *builtin_add(lenv *e, lval *a) { return builtin_op(e, a, "+"); }
lval *builtin_sub(lenv *e, lval *a) { return builtin_op(e, a, "-"); }
lval *builtin_mul(lenv *e, lval *a) { return builtin_op(e, a, "*"); }
lval *builtin_div(lenv *e, lval *a) { return builtin_op(e, a, "/"); }

//Built-in variable controls PLUS error handling/reporting at NO EXTRA CHARGE!
lval *builtin_var(lenv *e, lval *a, char *func)
{
    LASSERT_TYPE(func, a, 0, LVAL_QEXPR);

    lval *syms = a->cell[0];
    for(int i = 0; i < syms->count; i++)
    {
        LASSERT(a, (syms->cell[i]->type == LVAL_SYM),
                "Function '%s' cannot define non-symbol. "
                "Got %s, expected %s. ",
                func, ltype_name(syms->cell[i]->type), ltype_name(LVAL_SYM));
    }

    LASSERT(a, (syms->count == a->count-1),
            "Function '%s' passed too many arguments for symbols. "
            "Got %i, expected %i. ",
            func, syms->count, a->count-1);

    for(int i = 0; i < syms->count; i++)
    {
        if(strcmp(func, "def") == 0) { lenv_def(e, syms->cell[i], a->cell[i+1]); }
        if(strcmp(func, "=")   == 0) { lenv_put(e, syms->cell[i], a->cell[i+1]); }
    }
    lval_del(a);
    return lval_sexpr();
}
lval *builtin_def(lenv *e, lval *a) { return builtin_var(e, a, "def"); }
lval *builtin_put(lenv *e, lval *a) { return builtin_var(e, a, "="); }



//Rules for comparison operators
lval *builtin_ord(lenv *e, lval *a, char *op)
{
    LASSERT_NUM(op, a, 2);
    LASSERT_TYPE(op, a, 0, LVAL_NUM);
    LASSERT_TYPE(op, a, 1, LVAL_NUM);

    int r;
    if(strcmp(op, ">")  == 0) { r = (a->cell[0]->num >  a->cell[1]->num); }
    if(strcmp(op, "<")  == 0) { r = (a->cell[0]->num <  a->cell[1]->num); }   
    if(strcmp(op, ">=") == 0) { r = (a->cell[0]->num >= a->cell[1]->num); }
    if(strcmp(op, "<=") == 0) { r = (a->cell[0]->num <= a->cell[1]->num); }
    
    lval_del(a);
    return lval_num(r);
}
//Deal with comparison operators by calling their implementations
lval *builtin_gt(lenv *e, lval *a) { return builtin_ord(e, a, ">");   }
lval *builtin_lt(lenv *e, lval *a) { return builtin_ord(e, a, "<");   }
lval *builtin_ge(lenv *e, lval *a) { return builtin_ord(e, a, ">=");  }
lval *builtin_le(lenv *e, lval *a) { return builtin_ord(e, a, "<=");  }



//Rulesets for == and !=
lval *builtin_cmp(lenv *e, lval *a, char *op)
{
    LASSERT_NUM(op, a, 2);
    int r;
    if(strcmp(op, "==") == 0) { r =  lval_eq(a->cell[0], a->cell[1]); }
    if(strcmp(op, "!=") == 0) { r = !lval_eq(a->cell[0], a->cell[1]); }
    lval_del(a);
    return lval_num(r);
}
//Handle == and != by calling their implementations
lval *builtin_eq(lenv *e, lval *a) { return builtin_cmp(e, a, "=="); }
lval *builtin_ne(lenv *e, lval *a) { return builtin_cmp(e, a, "!="); }



//Various built-ins
//Built-in conditionals
lval *builtin_if(lenv *e, lval *a)
{
    LASSERT_NUM("if", a, 3);
    LASSERT_TYPE("if", a, 0, LVAL_NUM);
    LASSERT_TYPE("if", a, 1, LVAL_QEXPR);
    LASSERT_TYPE("if", a, 2, LVAL_QEXPR);

    lval *x;
    a->cell[1]->type = LVAL_SEXPR;
    a->cell[2]->type = LVAL_SEXPR;

    if(a->cell[0]->num)
    {
        x = lval_eval(e, lval_pop(a, 1));
    }
    else
    {
        x = lval_eval(e, lval_pop(a, 2));
    }

    lval_del(a);
    return x;
}
//Load and parse a file instead of using a REPL the whole time
lval *builtin_load(lenv *e, lval *a)
{
    LASSERT_NUM("load", a, 1);
    LASSERT_TYPE("load", a, 0, LVAL_STR);
    
    //Parse a file by a given string name
    mpc_result_t r;
    if(mpc_parse_contents(a->cell[0]->str, Lispy, &r))
    {
        //Read the contents in
        lval *expr = lval_read(r.output);
        mpc_ast_delete(r.output);

        //Evaluate each expression
        while(expr->count)
        {
            lval *x = lval_eval(e, lval_pop(expr, 0));
            //If evaluation leads to an error, print it. 
            if(x->type == LVAL_ERR)
            {
                lval_println(x);
            }
            lval_del(x);
        }
        //Do some clean up
        lval_del(expr);
        lval_del(a);

        //Return an empty list
        return lval_sexpr();
    }
    else
    {
        //Get parse error as string
        char * err_msg = mpc_err_string(r.error);
        mpc_err_delete(r.error);

        //Create new error message from that string
        lval *err = lval_err("Could not load library %s", err_msg);
        free(err_msg);
        lval_del(a);

        return err;
    }
}
lval *builtin_print(lenv *e, lval *a)
{
    //Print each arg + ' '
    for(int i = 0; i < a->count; i++)
    {
        lval_print(a->cell[i]); 
        putchar(' ');
    }
    putchar('\n');
    lval_del(a);
    return lval_sexpr();
}
//Error handling!
lval *builtin_error(lenv *e, lval *a)
{
    LASSERT_NUM("error", a, 1);
    LASSERT_TYPE("error", a, 0, LVAL_STR);
    //Construct the error from the first arg
    lval *err = lval_err(a->cell[0]->str);
    //Clean up
    lval_del(a);
    return err;
}
//Process for adding our built-in funcs
void lenv_add_builtin(lenv *e, char *name, lbuiltin func)
{
    lval *k = lval_sym(name);
    lval *v = lval_builtin(func);
    lenv_put(e, k, v);
    lval_del(k);
    lval_del(v);
}
//Add all desired built-in functions to our language
void lenv_add_builtins(lenv *e)
{
    //Variable funcs
    lenv_add_builtin(e, "\\",  builtin_lambda);
    lenv_add_builtin(e, "def", builtin_def);
    lenv_add_builtin(e, "=",   builtin_put);

    //List funcs
    lenv_add_builtin(e, "list", builtin_list);
    lenv_add_builtin(e, "head", builtin_head);
    lenv_add_builtin(e, "tail", builtin_tail);
    lenv_add_builtin(e, "eval", builtin_eval);
    lenv_add_builtin(e, "join", builtin_join);

    //Math operators
    lenv_add_builtin(e, "+", builtin_add);
    lenv_add_builtin(e, "-", builtin_sub);
    lenv_add_builtin(e, "*", builtin_mul);
    lenv_add_builtin(e, "/", builtin_div);
    
    //Comparison ops
    lenv_add_builtin(e, "if", builtin_if);
    lenv_add_builtin(e, "==", builtin_eq);
    lenv_add_builtin(e, "!=", builtin_ne);
    lenv_add_builtin(e, ">",  builtin_gt);
    lenv_add_builtin(e, "<",  builtin_lt);
    lenv_add_builtin(e, ">=", builtin_ge);
    lenv_add_builtin(e, "<=", builtin_le);

    //String funcs
    lenv_add_builtin(e, "load",  builtin_load);
    lenv_add_builtin(e, "error", builtin_error);
    lenv_add_builtin(e, "print", builtin_print);
}



//These functions are for evaluations.
lval *lval_call(lenv *e, lval *f, lval *a)
{
    if(f->builtin)
    {
       return f->builtin(e, a);
    }
    
    int given = a->count;
    int total = f->formals->count;

    while(a->count)
    {
        if(f->formals->count == 0)
        {
            lval_del(a);
            return lval_err("Function passed too many arguments. " "Got %i, expected %i. ", given, total);
        }
        lval *sym = lval_pop(f->formals, 0);

        if(strcmp(sym->sym, "&") == 0)
        {
            if(f->formals->count != 1)
            {
                lval_del(a);
                return lval_err("Function format invalid. " "Symbol '&' not followed by single symbol. ");
            }
            lval *nsym = lval_pop(f->formals, 0);
            lenv_put(f->env, nsym, builtin_list(e, a));
            lval_del(sym);
            lval_del(nsym);
            break;
        }
        lval *val = lval_pop(a, 0);
        lenv_put(f->env, sym, val);
        lval_del(sym);
        lval_del(val);
    }
    lval_del(a);

    if(f->formals->count > 0 && strcmp(f->formals->cell[0]->sym, "&") == 0)
    {
        if(f->formals->count != 2)
        {
            return lval_err("Function format invalid. " "Symbol '&' not followed by single symbol. ");
        }
        lval_del(lval_pop(f->formals, 0));

        lval *sym = lval_pop(f->formals, 0);
        lval *val = lval_qexpr();
        lenv_put(f->env, sym, val);
        lval_del(sym);
        lval_del(val);
    }

    if(f->formals->count == 0)
    {
        f->env->par = e;
        return builtin_eval(f->env, lval_add(lval_sexpr(), lval_copy(f->body)));
    }
    else
    {
        return lval_copy(f);
    }
}
//S-expressions
lval *lval_eval_sexpr(lenv *e, lval *v)
{
    for(int i = 0; i < v->count; i++)
    {
        v->cell[i] = lval_eval(e, v->cell[i]);
    }
    for(int i = 0; i < v->count; i++)
    {
        if(v->cell[i]->type == LVAL_ERR)
        {
            return lval_take(v, i);
        }
    }
    
    if(v->count == 0)
    {
        return v;
    }
    if(v->count == 1)
    {
        return lval_eval(e, lval_take(v, 0));
    }

    lval *f = lval_pop(v, 0);
    if(f->type != LVAL_FUN)
    {
        lval *err = lval_err("S-Expression starts with incorrect type. " "Got %s, Expected %s. ", ltype_name(f->type), ltype_name(LVAL_FUN));
        lval_del(f);
        lval_del(v);
        return err;
    }

    lval *result = lval_call(e, f, v);
    lval_del(f);
    return result;
}



//These functions are for reading. Reading is good for you, don't you know?
lval *lval_read_num(mpc_ast_t *t)
{
    errno = 0;
    long x = strtol(t->contents, NULL, 10);
    return errno != ERANGE ? lval_num(x) : lval_err("Invalid Number.");
}
lval *lval_read_str(mpc_ast_t *t)
{
    //Trunc the trailing quote char
    t->contents[strlen(t->contents)-1] = '\0';
    //Copy the string minus the leading quote char
    char *unescaped = malloc(strlen(t->contents+1)+1);
    strcpy(unescaped, t->contents+1);
    //Pass it through the unescape func
    unescaped = mpcf_unescape(unescaped);
    //Constrcut new lval using the string
    lval *str = lval_str(unescaped);
    //Clean up
    free(unescaped);
    return str;
}
//Read input, call the proper functions. Pretty self-explanatory.
lval *lval_read(mpc_ast_t *t)
{
    if(strstr(t->tag, "number")) { return lval_read_num(t); }
    if(strstr(t->tag, "string")) { return lval_read_str(t); }
    if(strstr(t->tag, "symbol")) { return lval_sym(t->contents); }

    lval *x = NULL;
    if(strcmp(t->tag, ">") == 0) { x = lval_sexpr(); }
    if(strstr(t->tag, "sexpr"))  { x = lval_sexpr(); }
    if(strstr(t->tag, "qexpr"))  { x = lval_qexpr(); }

    for(int i = 0; i < t->children_num; i++)
    {
        if(strcmp(t->children[i]->contents, "(") == 0) { continue; }
        if(strcmp(t->children[i]->contents, ")") == 0) { continue; }
        if(strcmp(t->children[i]->contents, "}") == 0) { continue; }
        if(strcmp(t->children[i]->contents, "{") == 0) { continue; }
        if(strcmp(t->children[i]->tag,  "regex") == 0) { continue; }
        if(strstr(t->children[i]->tag, "comment")) { continue; }
        x = lval_add(x, lval_read(t->children[i]));
    }
    return x;
}
