/* Evaluator for GNU Emacs Lisp interpreter. 
   Copyright (C) 1985 Richard M. Stallman. 
 
This file is part of GNU Emacs. 
 
GNU Emacs is distributed in the hope that it will be useful, 
but without any warranty.  No author or distributor 
accepts responsibility to anyone for the consequences of using it 
or for whether it serves any particular purpose or works at all, 
unless he says so in writing. 
 
Everyone is granted permission to copy, modify and redistribute 
GNU Emacs, but only under the conditions described in the 
document "GNU Emacs copying permission notice".   An exact copy 
of the document is supposed to have been given to you along with 
GNU Emacs so that you can know how you may redistribute it all. 
It should be in a file named COPYING.  Among other things, the 
copyright notice and this notice must be preserved on all copies.  */ 
 
 
#include "lisp.h" 
 
#ifndef standalone 
#include "commands.h" 
#else 
#define INTERACTIVE 1 
#endif 
 
#include <setjmp.h> 
 
/* This definition is duplicated in alloc.c and keyboard.c */ 
/* Putting it in lisp.h makes cc bomb out! */ 
 
struct backtrace 
  { 
    struct backtrace *next; 
    Lisp_Object *function; 
    Lisp_Object *args;	/* Points to vector of args. */ 
    int nargs;		/* length of vector */ 
	       /* if nargs is UNEVALLED, args points to slot holding list of unevalled args */ 
    char evalargs; 
    /* Nonzero means call value of debugger when done with this operation. */ 
    char debug_on_exit; 
  }; 
 
struct backtrace *backtrace_list; 
 
struct catchtag 
  { 
    Lisp_Object tag; 
    Lisp_Object val; 
    struct catchtag *next; 
    jmp_buf jmp; 
    struct backtrace *backlist; 
  }; 
 
struct catchtag *catchlist; 
 
Lisp_Object Qautoload, Qmacro, Qexit, Qinteractive, Qcommandp, Qdefun; 
Lisp_Object Vquit_flag, Vinhibit_quit; 
Lisp_Object Qmocklisp_arguments, Vmocklisp_arguments, Qmocklisp; 
Lisp_Object Qand_rest, Qand_optional, Qand_quote; 
 
int specpdl_size; 
struct specbinding *specpdl; 
struct specbinding *specpdl_ptr; 
 
/* Nonzero means enter debugger before next function call */ 
int debug_on_next_call; 
 
/* Nonzero means display a backtrace if an error 
 is handled by the command loop's error handler. */ 
int stack_trace_on_error; 
 
/* Nonzero means enter debugger if an error 
 is handled by the command loop's error handler. */ 
int debug_on_error; 
 
/* Nonzero means enter debugger if a quit signal 
 is handled by the command loop's error handler. */ 
int debug_on_quit; 
 
Lisp_Object Vdebugger; 
 
void specbind (), unbind_to (), record_unwind_protect (); 
 
Lisp_Object funcall_lambda (); 
extern Lisp_Object ml_apply (); /* Apply a mocklisp function to unevaluated argument list */ 
 
init_eval_once () 
{ 
  specpdl_size = 100; 
  specpdl = (struct specbinding *) malloc (specpdl_size * sizeof (struct specbinding)); 
} 
 
init_eval () 
{ 
  specpdl_ptr = specpdl; 
  catchlist = 0; 
  handlerlist = 0; 
  backtrace_list = 0; 
  Vquit_flag = Qnil; 
  debug_on_next_call = 0; 
} 
 
do_debug_on_call (code) 
     Lisp_Object code; 
{ 
  debug_on_next_call = 0; 
  backtrace_list->debug_on_exit = 1; 
  Fapply (Vdebugger, Fcons (code, Qnil)); 
} 
 
/* NOTE!!! Every function that can call EVAL must protect its args 
 and temporaries from garbage collection while it needs them. 
 The definition of `For' shows what you have to do.  */ 
 
DEFUN ("or", For, Sor, 0, UNEVALLED, 0, 
  "Eval args until one of them yields non-NIL, then return that value.\n\ 
The remaining args are not evalled at all.\n\ 
If all args return NIL, return NIL.") 
  (args) 
     Lisp_Object args; 
{ 
  Lisp_Object val; 
  Lisp_Object args_left; 
  struct gcpro gcpro1; 
 
  if (NULL(args)) 
    return Qnil; 
 
  args_left = args; 
  GCPRO1 (args_left); 
 
  do 
    { 
      val = Feval (Fcar (args_left)); 
      if (!NULL (val)) 
	break; 
      args_left = Fcdr (args_left); 
    } 
  while (!NULL(args_left)); 
 
  UNGCPRO; 
  return val; 
} 
 
DEFUN ("and", Fand, Sand, 0, UNEVALLED, 0, 
  "Eval args until one of them yields NIL, then return NIL.\n\ 
The remaining args are not evalled at all.\n\ 
If no arg yields NIL, return the last arg's value.") 
  (args) 
     Lisp_Object args; 
{ 
  Lisp_Object val; 
  Lisp_Object args_left; 
  struct gcpro gcpro1; 
 
  if (NULL(args)) 
    return Qt; 
 
  args_left = args; 
  GCPRO1 (args_left); 
 
  do 
    { 
      val = Feval (Fcar (args_left)); 
      if (NULL (val)) 
	break; 
      args_left = Fcdr (args_left); 
    } 
  while (!NULL(args_left)); 
 
  UNGCPRO; 
  return val; 
} 
 
DEFUN ("if", Fif, Sif, 2, UNEVALLED, 0, 
  "(if C T E...) if C yields non-NIL do T, else do E...\n\ 
Returns the value of T or the value of the last of the E's.\n\ 
There may be no E's; then if C yields NIL, the value is NIL.") 
  (args) 
     Lisp_Object args; 
{ 
  Lisp_Object cond; 
  struct gcpro gcpro1; 
 
  GCPRO1 (args); 
  cond = Feval (Fcar (args)); 
  UNGCPRO; 
 
  if (!NULL (cond)) 
    return Feval (Fcar (Fcdr (args))); 
  return Fprogn (Fcdr (Fcdr (args))); 
} 
 
DEFUN ("cond", Fcond, Scond, 0, UNEVALLED, 0, 
  "(cond CLAUSES...) tries each clause until one succeeds.\n\ 
Each clause looks like (C BODY...).  C is evaluated\n\ 
and, if the value is non-nil, this clause succeeds:\n\ 
then the expressions in BODY are evaluated and the last one's\n\ 
value is the value of the cond expression.\n\ 
If a clause looks like (C), C's value if non-nil is returned from cond.\n\ 
If no clause succeeds, cond returns nil.") 
  (args) 
     Lisp_Object args; 
{ 
  Lisp_Object clause, val; 
  struct gcpro gcpro1; 
 
  GCPRO1 (args); 
  while (!NULL (args)) 
    { 
      clause = Fcar (args); 
      val = Feval (Fcar (clause)); 
      if (!NULL (val)) 
	{ 
	  if (!EQ (XCONS (clause)->cdr, Qnil)) 
	    val = Fprogn (XCONS (clause)->cdr); 
	  break; 
	} 
      args = XCONS (args)->cdr; 
    } 
  UNGCPRO; 
 
  return val; 
} 
 
DEFUN ("progn", Fprogn, Sprogn, 0, UNEVALLED, 0, 
  "Eval arguments in sequence, and return the value of the last one.") 
  (args) 
     Lisp_Object args; 
{ 
  Lisp_Object val, tem; 
  Lisp_Object args_left; 
  struct gcpro gcpro1; 
 
  int count = specpdl_ptr - specpdl; 
 
  /* In Mocklisp code, symbols at the front of the progn arglist 
   are to be bound to zero. */ 
  if (!EQ (Vmocklisp_arguments, Qt)) 
    { 
      val = make_number (0); 
      while (!NULL (args) && (tem = Fcar (args), XTYPE (tem) == Lisp_Symbol)) 
	specbind (tem, val), args = Fcdr (args); 
    } 
 
  if (NULL(args)) 
    return Qnil; 
 
  args_left = args; 
  GCPRO1 (args_left); 
 
  do 
    { 
      val = Feval (Fcar (args_left)); 
      args_left = Fcdr (args_left); 
    } 
  while (!NULL(args_left)); 
 
  UNGCPRO; 
  return val; 
} 
 
DEFUN ("prog1", Fprog1, Sprog1, 1, UNEVALLED, 0, 
  "Eval arguments in sequence, then return the FIRST arg's value.\n\ 
This value is saved during the evaluation of the remaining args,\n\ 
whose values are discarded.") 
  (args) 
     Lisp_Object args; 
{ 
  Lisp_Object val; 
  Lisp_Object args_left; 
  struct gcpro gcpro1; 
  int argnum = 0; 
 
  if (NULL(args)) 
    return Qnil; 
 
  args_left = args; 
  GCPRO1 (args); 
 
  do 
    { 
      if (!(argnum++)) 
        val = Feval (Fcar (args_left)); 
      else 
	Feval (Fcar (args_left)); 
      args_left = Fcdr (args_left); 
    } 
  while (!NULL(args_left)); 
 
  UNGCPRO; 
  return val; 
} 
 
DEFUN ("prog2", Fprog2, Sprog2, 2, UNEVALLED, 0, 
  "Eval arguments in sequence, then return the SECOND arg's value.\n\ 
This value is saved during the evaluation of the remaining args,\n\ 
whose values are discarded.") 
  (args) 
     Lisp_Object args; 
{ 
  Lisp_Object val; 
  Lisp_Object args_left; 
  struct gcpro gcpro1; 
  int argnum = -1; 
 
  val = Qnil; 
 
  if (NULL(args)) 
    return Qnil; 
 
  args_left = args; 
  GCPRO1 (args); 
 
  do 
    { 
      if (!(argnum++)) 
        val = Feval (Fcar (args_left)); 
      else 
	Feval (Fcar (args_left)); 
      args_left = Fcdr (args_left); 
    } 
  while (!NULL(args_left)); 
 
  UNGCPRO; 
  return val; 
} 
 
DEFUN ("setq", Fsetq, Ssetq, 0, UNEVALLED, 0, 
  "(setq SYM VAL SYM VAL ...) sets each SYM to the value of its VAL.\n\ 
The SYMs are not evaluated.  Thus (setq x y) sets x to the value of y.\n\ 
Each SYM is set before the next VAL is computed.") 
  (args) 
     Lisp_Object args; 
{ 
  Lisp_Object val, sym; 
  Lisp_Object args_left; 
  struct gcpro gcpro1; 
 
  if (NULL(args)) 
    return Qnil; 
 
  args_left = args; 
  GCPRO1(args_left); 
 
  do 
    { 
      val = Feval (Fcar (Fcdr (args_left))); 
      sym = Fcar (args_left); 
      Fset (sym, val); 
      args_left = Fcdr (Fcdr (args_left)); 
    } 
  while (!NULL(args_left)); 
 
  UNGCPRO; 
  return val; 
} 
      
DEFUN ("quote", Fquote, Squote, 1, UNEVALLED, 0, 
  "Return the argument, without evaluating it.  (quote x)  yields  x.") 
  (args) 
     Lisp_Object args; 
{ 
  return Fcar (args); 
} 
      
DEFUN ("function", Ffunction, Sfunction, 1, UNEVALLED, 0, 
  "Quote a function object.\n\ 
Equivalent to the quote function in the interpreter,\n\ 
but causes the compiler to compile the argument as a function\n\ 
if it is not a symbol.") 
  (args) 
     Lisp_Object args; 
{ 
  return Fcar (args); 
} 
 
DEFUN ("interactive-p", Finteractive_p, Sinteractive_p, 0, UNEVALLED, 0, 
  "Return t if function in which this appears was called interactively.\n\ 
Also, input must be coming from the terminal.") 
  () 
{ 
  struct backtrace *btp; 
 
  if (!INTERACTIVE) 
    return Qnil; 
  /* Note that interactive-p takes UNEVALLED args 
     so that its own frame does not terminale this loop.  */ 
  for (btp = backtrace_list; btp && btp->nargs == UNEVALLED; btp = btp->next) 
    {} 
  if (btp && btp->next && EQ (*btp->next->function, Qcall_interactively)) 
    return Qt; 
  return Qnil; 
} 
 
DEFUN ("defun", Fdefun, Sdefun, 2, UNEVALLED, 0, 
  "(defun NAME ARGLIST [DOCSTRING] BODY...) defines NAME as a function.\n\ 
The definition is (lambda ARGLIST [DOCSTRING] BODY...).\n\ 
See also the function  interactive .") 
  (args) 
     Lisp_Object args; 
{ 
  Lisp_Object fn_name; 
  Lisp_Object defn; 
 
  fn_name = Fcar (args); 
  defn = Fcons (Qlambda, Fcdr (args)); 
  if (!NULL (Vpurify_flag)) 
    defn = Fpurecopy (defn); 
  Ffset (fn_name, defn); 
  return fn_name; 
} 
 
DEFUN ("defmacro", Fdefmacro, Sdefmacro, 2, UNEVALLED, 0, 
  "(defmacro NAME ARGLIST [DOCSTRING] BODY...) defines NAME as a macro.\n\ 
The definition is (macro lambda ARGLIST [DOCSTRING] BODY...).\n\ 
When the macro is called, as in (NAME ARGS...),\n\ 
the function (lambda ARGLIST BODY...) is applied to\n\ 
the list ARGS... as it appears in the expression,\n\ 
and the result should be a form to be evaluated instead of the original.") 
  (args) 
     Lisp_Object args; 
{ 
  Lisp_Object fn_name; 
  Lisp_Object defn; 
 
  fn_name = Fcar (args); 
  defn = Fcons (Qmacro, Fcons (Qlambda, Fcdr (args))); 
  if (!NULL (Vpurify_flag)) 
    defn = Fpurecopy (defn); 
  Ffset (fn_name, defn); 
  return fn_name; 
} 
 
DEFUN ("defvar", Fdefvar, Sdefvar, 1, UNEVALLED, 0, 
  "(defvar SYMBOL INITVALUE DOCSTRING) defines SYMBOL as an advertised variable.\n\ 
INITVALUE is evaluated, and used to set SYMBOL, only if SYMBOL's value is void.\n\ 
INITVALUE and DOCSTRING are optional.\n\ 
If DOCSTRING starts with *, this variable is identified as a user option.\n\ 
If INITVALUE is missing, SYMBOL's value is not set.") 
  (args) 
     Lisp_Object args; 
{ 
  Lisp_Object sym, tem; 
 
  sym = Fcar (args); 
  tem = Fcdr (args); 
  if (!NULL (tem)) 
    { 
      tem = Fboundp (sym); 
      if (NULL (tem)) 
	Fset (sym, Feval (Fcar (Fcdr (args)))); 
    } 
  tem = Fcar (Fcdr (Fcdr (args))); 
  if (!NULL (tem)) 
    { 
      if (!NULL (Vpurify_flag)) 
	tem = Fpurecopy (tem); 
      Fput (sym, Qvariable_documentation, tem); 
    } 
  return sym; 
} 
 
DEFUN ("defconst", Fdefconst, Sdefconst, 2, UNEVALLED, 0, 
  "(defconst SYMBOL INITVALUE DOCSTRING) defines SYMBOL as an advertised constant.\n\ 
The intent is that programs do not change this value (but users may).\n\ 
Always sets the value of SYMBOL to the result of evalling INITVALUE.\n\ 
DOCSTRING is optional.\n\ 
If DOCSTRING starts with *, this variable is identified as a user option.") 
  (args) 
     Lisp_Object args; 
{ 
  Lisp_Object sym, tem; 
 
  sym = Fcar (args); 
  Fset (sym, Feval (Fcar (Fcdr (args)))); 
  tem = Fcar (Fcdr (Fcdr (args))); 
  if (!NULL (tem)) 
    { 
      if (!NULL (Vpurify_flag)) 
	tem = Fpurecopy (tem); 
      Fput (sym, Qvariable_documentation, tem); 
    } 
  return sym; 
} 
 
DEFUN ("let*", FletX, SletX, 1, UNEVALLED, 0, 
  "(let* VARLIST BODY...) binds variables according to VARLIST then executes BODY.\n\ 
The value of the last form in BODY is returned.\n\ 
Each element of VARLIST is a symbol (which is bound to NIL)\n\ 
or a list (SYMBOL VALUEFORM) (which binds SYMBOL to the value of VALUEFORM).\n\ 
Each VALUEFORM can refer to the symbols already bound by this VARLIST.") 
  (args) 
     Lisp_Object args; 
{ 
  Lisp_Object varlist, val, elt; 
  int count = specpdl_ptr - specpdl; 
  struct gcpro gcpro1, gcpro2, gcpro3; 
 
  GCPRO3(args, elt, varlist); 
 
  varlist = Fcar (args); 
  while (!NULL (varlist)) 
    { 
      elt = Fcar (varlist); 
      if (XTYPE (elt) == Lisp_Symbol) 
	specbind (elt, Qnil); 
      else 
	{ 
	  val = Feval (Fcar (Fcdr (elt))); 
	  specbind (Fcar (elt), val); 
	} 
      varlist = Fcdr (varlist); 
    } 
  val = Fprogn (Fcdr (args)); 
  unbind_to (count); 
  UNGCPRO; 
  return val; 
} 
 
DEFUN ("let", Flet, Slet, 1, UNEVALLED, 0, 
  "(let VARLIST BODY...) binds variables according to VARLIST then executes BODY.\n\ 
The value of the last form in BODY is returned.\n\ 
Each element of VARLIST is a symbol (which is bound to NIL)\n\ 
or a list (SYMBOL VALUEFORM) (which binds SYMBOL to the value of VALUEFORM).\n\ 
All the VALUEFORMs are evalled before any symbols are bound.") 
  (args) 
     Lisp_Object args; 
{ 
  Lisp_Object varlist, var_tail, val, elt, nvars; 
  Lisp_Object *temps, tem; 
  int count = specpdl_ptr - specpdl; 
  int argnum; 
  struct gcpro gcpro1, gcpro2, gcpro3, gcpro4; 
 
  varlist = Fcar (args); 
 
  /* Make space to hold the values to give the bound variables */ 
  nvars = Flength (varlist); 
  temps = (Lisp_Object *) alloca (XINT (nvars) * sizeof (Lisp_Object)); 
 
  /* Compute the values and store them in `temps' */ 
 
  GCPRO3 (args, elt, varlist); 
  gcpro4.next = &gcpro3, gcpro4.var = temps, gcpro4.nvars = XINT (nvars); 
  gcprolist = &gcpro4; 
 
  for (argnum = 0; !NULL (varlist); varlist = Fcdr (varlist)) 
    { 
      elt = Fcar (varlist); 
      if (XTYPE (elt) == Lisp_Symbol) 
	temps [argnum++] = Qnil; 
      else 
	temps [argnum++] = Feval (Fcar (Fcdr (elt))); 
    } 
  UNGCPRO; 
 
  varlist = Fcar (args); 
  for (argnum = 0; !NULL (varlist); varlist = Fcdr (varlist)) 
    { 
      elt = Fcar (varlist); 
      if (XTYPE (elt) == Lisp_Symbol) 
	{ tem = temps[argnum++], specbind (elt, tem); } 
      else 
	{ tem = temps[argnum++], specbind (Fcar (elt), tem); } 
    } 
 
  val = Fprogn (Fcdr (args)); 
  unbind_to (count); 
  return val; 
} 
 
DEFUN ("while", Fwhile, Swhile, 1, UNEVALLED, 0, 
  "(while TEST BODY...) if TEST yields non-NIL, execute the BODY forms and repeat.") 
  (args) 
     Lisp_Object args; 
{ 
  Lisp_Object test, body, tem; 
  struct gcpro gcpro1, gcpro2; 
 
  GCPRO2 (test, body); 
 
  test = Fcar (args); 
  body = Fcdr (args); 
  while (tem = Feval (test), !NULL (tem)) 
    { 
      QUIT; 
      Fprogn (body); 
    } 
 
  UNGCPRO; 
  return Qnil; 
} 
 
DEFUN ("macroexpand", Fmacroexpand, Smacroexpand, 1, 1, 0, 
  "If FORM is a macro call, expand it.\n\ 
If the result of expansion is another macro call, expand it, etc.\n\ 
Return the ultimate expansion.") 
  (form) 
     Lisp_Object form; 
{ 
  Lisp_Object tem; 
 
  while (XTYPE (form) == Lisp_Cons 
	 && (tem = XCONS (form)->car, 
	     XTYPE (tem) == Lisp_Symbol) 
	 && (tem = XSYMBOL (tem)->function, 
	     !EQ (tem, Qunbound) 
	     && XTYPE (tem) == Lisp_Cons) 
	 && EQ (XCONS (tem)->car, Qmacro)) 
    form = Fapply (XCONS (tem)->cdr, XCONS (form)->cdr); 
  return form; 
} 
 
DEFUN ("catch", Fcatch, Scatch, 1, UNEVALLED, 0, 
  "(catch TAG BODY...) perform BODY allowing nonlocal exits using (throw TAG).\n\ 
TAG is evalled to get the tag to use.  throw  to that tag exits this catch.\n\ 
Then the BODY is executed.  If no  throw  happens, the value of the last BODY\n\ 
form is returned from  catch.  If a  throw  happens, it specifies the value to\n\ 
return from  catch.") 
  (args) 
     Lisp_Object args; 
{ 
  Lisp_Object tag, val; 
  int count = specpdl_ptr - specpdl; 
  struct gcpro *gcpro = gcprolist; 
  struct gcpro gcpro1; 
  struct catchtag c; 
  struct handler *hlist = handlerlist; 
 
  c.tag = Feval (Fcar (args)); 
  c.backlist = backtrace_list; 
  if (_setjmp (c.jmp)) 
    { 
      catchlist = c.next; 
      handlerlist = hlist; 
      backtrace_list = c.backlist; 
      gcprolist = gcpro; 
      GCPRO1 (c.val); 
      unbind_to (count); 
      UNGCPRO; 
      return c.val; 
    } 
  c.next = catchlist; 
  catchlist = &c; 
  val = Fprogn (Fcdr (args)); 
  catchlist = c.next; 
  return val; 
} 
 
/* Set up a catch, then call C function `func'. 
 This is how catches are done from within C code. */ 
 
Lisp_Object 
internal_catch (tag, func, arg) 
     Lisp_Object tag; 
     Lisp_Object (*func) (); 
     Lisp_Object arg; 
{ 
  int count = specpdl_ptr - specpdl; 
  struct gcpro *gcpro = gcprolist; 
  struct gcpro gcpro1; 
  struct catchtag c; 
  struct handler *hlist = handlerlist; 
  Lisp_Object val; 
 
  c.tag = tag; 
  c.backlist = backtrace_list; 
  if (_setjmp (c.jmp)) 
    { 
      catchlist = c.next; 
      handlerlist = hlist; 
      backtrace_list = c.backlist; 
      gcprolist = gcpro; 
      GCPRO1 (c.val); 
      unbind_to (count); 
      UNGCPRO; 
      return c.val; 
    } 
  c.next = catchlist; 
  catchlist = &c; 
  val = func (arg); 
  catchlist = c.next; 
  return val; 
} 
 
DEFUN ("throw", Fthrow, Sthrow, 2, 2, 0, 
  "(throw TAG VALUE): throw to the catch for TAG and return VALUE from it.\n\ 
Both TAG and VALUE are evalled.") 
  (tag, val) 
     Lisp_Object tag, val; 
{ 
  struct catchtag *c; 
 
  for (c = catchlist; c; c = c->next) 
    { 
      if (EQ (c->tag, tag)) 
	{ 
	  c->val = val; 
	  _longjmp (c->jmp, 1); 
	} 
    } 
  Fsignal (Qno_catch, Fcons (tag, Fcons (val, Qnil))); 
} 
 
DEFUN ("unwind-protect", Funwind_protect, Sunwind_protect, 1, UNEVALLED, 0, 
  "(unwind-protect BODYFORM UNWINDFORMS...) do BODYFORM, protecting with UNWINDFORMS.\n\ 
If BODYFORM completes normally, its value is returned\n\ 
after executing the UNWINDFORMS.\n\ 
If BODYFORM exits nonlocally, the UNWINDFORMS are executed anyway.") 
  (args) 
     Lisp_Object args; 
{ 
  Lisp_Object val; 
  int count = specpdl_ptr - specpdl; 
  struct gcpro gcpro1; 
 
  record_unwind_protect (0, Fcdr (args)); 
  (specpdl_ptr - 1)->symbol = Qnil; 
  val = Feval (Fcar (args)); 
  GCPRO1 (val); 
  unbind_to (count);   
  UNGCPRO; 
  return val; 
} 
 
struct handler *handlerlist; 
 
DEFUN ("condition-case", Fcondition_case, Scondition_case, 2, UNEVALLED, 0, 
  "Regain control when an error is signaled.\n\ 
 (condition-case VAR BODYFORM HANDLERS...)\n\ 
executes BODYFORM and returns its value if no error happens.\n\ 
Each element of HANDLERS looks like (CONDITIONLIST BODY...)\n\ 
CONDITIONLIST is a list of condition names.  The handler applies\n\ 
if the error has any of those condition names.  When this is so,\n\ 
control returns to the condition-case and the handler BODY... is executed\n\ 
with VAR bound to (SIGNALED-CONDITIONS . SIGNAL-DATA).\n\ 
The value of the last BODY form is returned from the condition-case.\n\ 
See SIGNAL for more info.") 
  (args) 
     Lisp_Object args; 
{ 
  Lisp_Object tag, val; 
  int count = specpdl_ptr - specpdl; 
  struct gcpro *gcpro = gcprolist; 
  struct gcpro gcpro1, gcpro2; 
  struct catchtag c; 
  struct handler h; 
 
  c.tag = Qnil; 
  c.backlist = backtrace_list; 
  if (_setjmp (c.jmp)) 
    { 
      catchlist = c.next; 
      handlerlist = h.next; 
      backtrace_list = c.backlist; 
      gcprolist = gcpro; 
      GCPRO2 (c.val, h.var); 
      unbind_to (count); 
      UNGCPRO; 
      if (!NULL (h.var)) 
        specbind (h.var, Fcdr (c.val)); 
      val = Fprogn (Fcdr (Fcar (c.val))); 
      unbind_to (count); 
      return val; 
    } 
  c.next = catchlist; 
  catchlist = &c; 
  h.var = Fcar (args); 
  h.handler = Fcdr (Fcdr (args)); 
  h.next = handlerlist; 
  h.tag = &c; 
  handlerlist = &h; 
 
  val = Feval (Fcar (Fcdr (args))); 
  catchlist = c.next; 
  handlerlist = h.next; 
  return val; 
} 
 
Lisp_Object 
internal_condition_case (bfun, handlers, hfun) 
     Lisp_Object (*bfun) (); 
     Lisp_Object handlers; 
     Lisp_Object (*hfun) (); 
{ 
  Lisp_Object tag, val; 
  int count = specpdl_ptr - specpdl; 
  struct gcpro *gcpro = gcprolist; 
  struct gcpro gcpro1; 
  struct catchtag c; 
  struct handler h; 
 
  c.tag = Qnil; 
  c.backlist = backtrace_list; 
  if (_setjmp (c.jmp)) 
    { 
      catchlist = c.next; 
      handlerlist = h.next; 
      backtrace_list = c.backlist; 
      gcprolist = gcpro; 
      GCPRO1 (c.val); 
      unbind_to (count); 
      UNGCPRO; 
      return hfun (Fcdr (c.val)); 
    } 
  c.next = catchlist; 
  catchlist = &c; 
  h.handler = handlers; 
  h.next = handlerlist; 
  h.tag = &c; 
  handlerlist = &h; 
 
  val = bfun (); 
  catchlist = c.next; 
  handlerlist = h.next; 
  return val; 
} 
 
static Lisp_Object find_handler_clause (); 
 
DEFUN ("signal", Fsignal, Ssignal, 2, 2, 0, 
  "Signal an error.  Args are SIGNAL-NAME, and associated DATA.\n\ 
A signal name is a symbol with an  error-conditions  property\n\ 
that is a list of condition names.  DATA can be anything.\n\ 
A handlers for any of those names will get to handle this signal.\n\ 
The symbol  error  should always be one of them.") 
  (sig, data) 
     Lisp_Object sig, data; 
{ 
  struct handler *allhandlers = handlerlist; 
  Lisp_Object conditions; 
  extern int gc_in_progress; 
 
  if (gc_in_progress) 
    abort (); 
 
  conditions = Fget (sig, Qerror_conditions); 
 
  for (; handlerlist; handlerlist = handlerlist->next) 
    { 
      Lisp_Object clause; 
      clause = find_handler_clause (handlerlist->handler, conditions, 
				    sig, data); 
      if (!NULL (clause)) 
	{ 
	  struct handler *h = handlerlist; 
	  handlerlist = allhandlers; 
	  h->tag->val = Fcons (clause, Fcons (sig, data)); 
	  _longjmp (h->tag->jmp, 1); 
	} 
    } 
 
  handlerlist = allhandlers; 
  debugger (sig, data); 
} 
 
static Lisp_Object 
find_handler_clause (handlers, conditions, sig, data) 
     Lisp_Object handlers, conditions, sig, data; 
{ 
  Lisp_Object h; 
  if (EQ (handlers, Qt))  /* t is used by handlers for all conditions, set up by C code.  */ 
    return Qt; 
  if (EQ (handlers, Qerror))  /* error is used similarly, but means display a backtrace too */ 
    { 
      if (stack_trace_on_error) 
	internal_with_output_to_temp_buffer ("*Backtrace*", Fbacktrace, Qnil); 
      if (EQ (sig, Qquit) ? debug_on_quit : debug_on_error) 
	Fapply (Vdebugger, Fcons (Qerror, 
				  Fcons (Fcons (sig, data), 
					 Qnil))); 
      return Qt; 
    } 
  for (h = handlers; !NULL (h); h = Fcdr (h)) 
    { 
      Lisp_Object tem; 
      tem = Fmemq (Fcar (Fcar (h)), conditions); 
      if (!NULL (tem)) 
        return Fcar (h); 
    } 
  return Qnil; 
} 
 
/* dump an error message; called like printf */ 
 
void 
error (m, a1, a2, a3) 
{ 
  char buf[200]; 
  sprintf (buf, m, a1, a2, a3); 
  Fsignal (Qerror, Fcons (build_string (buf), Qnil)); 
} 
 
DEFUN ("function-type", Ffunction_type, Sfunction_type, 1, 1,0, 
  "Return a symbol indicating what type of function the argument is.\n\ 
It may be  lambda,  subr  or  autoload.") 
  (function) 
     Lisp_Object function; 
{ 
  Lisp_Object fun; 
  Lisp_Object funcar; 
  Lisp_Object tem; 
 
  fun = function; 
  while (XTYPE (fun) == Lisp_Symbol) 
    fun = Fsymbol_function (fun); 
  if (XTYPE (fun) == Lisp_Subr) 
    return Qsubr; 
  if (!LISTP(fun)) 
    Fsignal (Qinvalid_function, Fcons (fun, Qnil)); 
  funcar = Fcar (fun); 
  if (XTYPE (funcar) != Lisp_Symbol) 
    Fsignal (Qinvalid_function, Fcons (fun, Qnil)); 
  if (XSYMBOL (funcar) == XSYMBOL (Qlambda)) 
    return Qlambda; 
  if (XSYMBOL (funcar) == XSYMBOL (Qmacro)) 
    return Qmacro; 
  if (XSYMBOL (funcar) == XSYMBOL (Qautoload)) 
    return Qautoload; 
  if (XSYMBOL (funcar) == XSYMBOL (Qmocklisp)) 
    return Qmocklisp; 
  else 
    Fsignal (Qinvalid_function, Fcons (fun, Qnil)); 
} 
 
DEFUN ("commandp", Fcommandp, Scommandp, 1, 1, 0, 
  "T if FUNCTION makes provisions for interactive calling.\n\ 
This means it contains a description for how to read arguments to give it.\n\ 
The value is nil for an invalid function or a symbol with no function definition.") 
  (function) 
     Lisp_Object function; 
{ 
  Lisp_Object fun; 
  Lisp_Object funcar; 
  Lisp_Object tem; 
  int i = 0; 
 
  fun = function; 
  while (XTYPE (fun) == Lisp_Symbol) 
    { 
      if (++i > 10) return Qnil; 
      tem = Ffboundp (fun); 
      if (NULL (tem)) return Qnil; 
      fun = Fsymbol_function (fun); 
    } 
  if (XTYPE (fun) == Lisp_Subr) 
    if (XSUBR (fun)->prompt) 
      return Qt; 
    else 
      return Qnil; 
  if (XTYPE (fun) == Lisp_Vector || XTYPE (fun) == Lisp_String) 
    return Qt; 
  if (!LISTP(fun)) 
    return Qnil; 
  funcar = Fcar (fun); 
  if (XTYPE (funcar) != Lisp_Symbol) 
    Fsignal (Qinvalid_function, Fcons (fun, Qnil)); 
  if (XSYMBOL (funcar) == XSYMBOL (Qlambda)) 
    return Fassq (Qinteractive, Fcdr (Fcdr (fun))); 
  if (XSYMBOL (funcar) == XSYMBOL (Qmocklisp)) 
    return Qt;  /* All mocklisp functions can be called interactively */ 
  if (XSYMBOL (funcar) == XSYMBOL (Qautoload)) 
    return Fcar (Fcdr (Fcdr (Fcdr (fun)))); 
  else 
    return Qnil; 
} 
 
DEFUN ("autoload", Fautoload, Sautoload, 2, 4, 0, 
  "Define FUNCTION to autoload from FILE.\n\ 
FUNCTION is a symbol; FILE is a file name string to pass to  load.\n\ 
Third arg DOCSTRING is documentation for the function.\n\ 
Fourth arg INTERACTIVE if non-nil says function can be called interactively.\n\ 
Third and fourth args give info about the real definition.\n\ 
They default to nil.") 
  (function, file, docstring, interactive) 
     Lisp_Object function, file, docstring, interactive; 
{ 
  CHECK_SYMBOL (function, 0); 
  CHECK_STRING (file, 1); 
  return Ffset (function, Fcons (Qautoload, Flist (3, &file))); 
} 
 
DEFUN ("eval", Feval, Seval, 1, 1, 0, 
  "Evaluate FORM and return its value.") 
  (form) 
     Lisp_Object form; 
{ 
  Lisp_Object fun, val, original_fun, original_args; 
  Lisp_Object funcar; 
  struct backtrace backtrace; 
  struct gcpro gcpro1, gcpro2, gcpro3; 
 
  if (XTYPE (form) == Lisp_Symbol) 
    { 
      if (EQ (Vmocklisp_arguments, Qt)) 
        return Fsymbol_value (form); 
      val = Fsymbol_value (form); 
      if (NULL (val)) 
	XFASTINT (val) = 0; 
      else if (EQ (val, Qt)) 
	XFASTINT (val) = 1; 
      return val; 
    } 
  if (!LISTP (form)) 
    return form; 
 
  QUIT; 
  if (consing_since_gc > gc_cons_threshold) 
    { 
      GCPRO1 (form); 
      Fgarbage_collect (); 
      UNGCPRO; 
    } 
 
  original_fun = fun = Fcar (form); 
  original_args = Fcdr (form); 
 
  backtrace.next = backtrace_list; 
  backtrace_list = &backtrace; 
  backtrace.function = &original_fun; 
  backtrace.args = &original_args; 
  backtrace.nargs = UNEVALLED; 
  backtrace.evalargs = 1; 
  backtrace.debug_on_exit = 0; 
 
  if (debug_on_next_call) 
    do_debug_on_call (Qt); 
 
 retry: 
  while (XTYPE (fun) == Lisp_Symbol) 
    fun = Fsymbol_function (fun); 
 
  if (XTYPE (fun) == Lisp_Subr) 
    { 
      Lisp_Object numargs; 
      Lisp_Object argvals[5]; 
      Lisp_Object args_left; 
      int i, maxargs; 
 
      args_left = original_args; 
      numargs = Flength (args_left); 
 
      if (XINT (numargs) < XSUBR (fun)->min_args || 
	  (XSUBR (fun)->max_args >= 0 && XSUBR (fun)->max_args < XINT (numargs))) 
	Fsignal (Qwrong_number_of_arguments, Fcons (fun, Fcons (numargs, Qnil))); 
 
      if (XSUBR (fun)->max_args == UNEVALLED) 
	{ 
	  backtrace.evalargs = 0; 
	  val = XSUBR (fun)->function (args_left); 
	  goto done; 
	} 
 
      if (XSUBR (fun)->max_args == MANY) 
	{ 
	  /* Pass a vector of evaluated arguments */ 
	  Lisp_Object *vals; 
	  int argnum = 0; 
 
	  vals = (Lisp_Object *) alloca (XINT (numargs) * sizeof (Lisp_Object)); 
 
	  GCPRO3 (args_left, fun, fun); 
	  gcpro3.var = vals; 
	  gcpro3.nvars = XINT (numargs); 
 
	  while (!NULL (args_left)) 
	    { 
	      vals[argnum++] = Feval (Fcar (args_left)); 
	      args_left = Fcdr (args_left); 
	    } 
	  UNGCPRO; 
 
	  backtrace.args = vals; 
	  backtrace.nargs = XINT (numargs); 
 
	  val = XSUBR (fun)->function (XINT (numargs), vals); 
	  goto done; 
	} 
 
      GCPRO3 (args_left, fun, fun); 
      gcpro3.var = argvals; 
      gcpro3.nvars = 5; 
 
      maxargs = XSUBR (fun)->max_args; 
      for (i = 0; i < maxargs; i++, args_left = Fcdr (args_left)) 
	argvals[i] = Feval (Fcar (args_left)); 
 
      UNGCPRO; 
 
      backtrace.args = argvals; 
      backtrace.nargs = XINT (numargs); 
 
      switch (i) 
	{ 
	case 0: 
	  val = XSUBR (fun)->function (); 
	  goto done; 
	case 1: 
	  val = XSUBR (fun)->function (argvals[0]); 
	  goto done; 
	case 2: 
	  val = XSUBR (fun)->function (argvals[0], argvals[1]); 
	  goto done; 
	case 3: 
	  val = XSUBR (fun)->function (argvals[0], argvals[1], argvals[2]); 
	  goto done; 
	case 4: 
	  val = XSUBR (fun)->function (argvals[0], argvals[1], argvals[2], argvals[3]); 
	  goto done; 
	case 5: 
	  val = XSUBR (fun)->function (argvals[0], argvals[1], argvals[2], 
					 argvals[3], argvals[4]); 
	  goto done; 
	} 
    } 
  if (!LISTP(fun)) 
    Fsignal (Qinvalid_function, Fcons (fun, Qnil)); 
  funcar = Fcar (fun); 
  if (XTYPE (funcar) != Lisp_Symbol) 
    Fsignal (Qinvalid_function, Fcons (fun, Qnil)); 
  if (XSYMBOL (funcar) == XSYMBOL (Qautoload)) 
    { 
      Fload (Fcar (Fcdr (fun)), Qnil, Qnil); 
      fun = original_fun; 
      goto retry; 
    } 
  if (XSYMBOL (funcar) == XSYMBOL (Qmacro)) 
    val = Feval (Fapply (Fcdr (fun), Fcdr (form))); 
  else if (XSYMBOL (funcar) == XSYMBOL (Qlambda)) 
    val = apply_lambda (fun, original_args, 1); 
  else if (XSYMBOL (funcar) == XSYMBOL (Qmocklisp)) 
    val = ml_apply (fun, original_args); 
  else 
    Fsignal (Qinvalid_function, Fcons (fun, Qnil)); 
 
 done: 
  if (!EQ (Vmocklisp_arguments, Qt)) 
    { 
      if (NULL (val)) 
	XFASTINT (val) = 0; 
      else if (EQ (val, Qt)) 
	XFASTINT (val) = 1; 
    } 
  if (backtrace.debug_on_exit) 
    val = Fapply (Vdebugger, Fcons (Qexit, Fcons (val, Qnil))); 
  backtrace_list = backtrace.next; 
  return val; 
} 
 
DEFUN ("apply", Fapply, Sapply, 2, 2, 0, 
  "Call FUNCTION with arguments being the elements of ARGS.") 
  (original_fun, original_args) 
     Lisp_Object original_fun, original_args; 
{ 
  Lisp_Object fun; 
  Lisp_Object funcar; 
  Lisp_Object val; 
  struct backtrace backtrace; 
  struct gcpro gcpro1, gcpro2; 
 
  QUIT; 
  if (consing_since_gc > gc_cons_threshold) 
    { 
      GCPRO2 (original_fun, original_args); 
      Fgarbage_collect (); 
      UNGCPRO; 
    } 
 
  backtrace.next = backtrace_list; 
  backtrace_list = &backtrace; 
  backtrace.function = &original_fun; 
  backtrace.args = &original_args; 
  backtrace.nargs = MANY; 
  backtrace.evalargs = 0; 
  backtrace.debug_on_exit = 0; 
 
  if (debug_on_next_call) 
    do_debug_on_call (Qlambda); 
 
 retry: 
 
  fun = original_fun; 
  while (XTYPE (fun) == Lisp_Symbol) 
    fun = Fsymbol_function (fun); 
 
  if (XTYPE (fun) == Lisp_Subr) 
    { 
      Lisp_Object numargs; 
      Lisp_Object argvals[5]; 
      Lisp_Object args_left; 
      int i, maxargs; 
 
      args_left = original_args; 
      numargs = Flength (args_left); 
 
      if (XINT (numargs) < XSUBR (fun)->min_args || 
	  (XSUBR (fun)->max_args >= 0 && XSUBR (fun)->max_args < XINT (numargs))) 
	Fsignal (Qwrong_number_of_arguments, Fcons (fun, Fcons (numargs, Qnil))); 
 
      if (XSUBR (fun)->max_args == UNEVALLED) 
	{ 
	  val = XSUBR (fun)->function (original_args); 
	  goto done; 
	} 
 
      if (XSUBR (fun)->max_args == MANY) 
	{ 
	  /* Pass a vector of evaluated arguments */ 
	  Lisp_Object *vals; 
	  int argnum = 0; 
 
	  vals = (Lisp_Object *) alloca (XINT (numargs) * sizeof (Lisp_Object)); 
 
	  while (!NULL (args_left)) 
	    { 
	      vals[argnum++] = Fcar (args_left); 
	      args_left = Fcdr (args_left); 
	    } 
 
	  backtrace.args = vals; 
	  backtrace.nargs = argnum; 
	  val = XSUBR (fun)->function (XINT (numargs), vals); 
	  goto done; 
	} 
 
      maxargs = XSUBR (fun)->max_args; 
      for (i = 0; i < maxargs; i++, args_left = Fcdr (args_left)) 
	argvals[i] = Fcar (args_left); 
 
      backtrace.args = argvals; 
      backtrace.nargs = XINT (numargs); 
 
      switch (i) 
	{ 
	case 0: 
	  val = XSUBR (fun)->function (); 
	  goto done; 
	case 1: 
	  val = XSUBR (fun)->function (argvals[0]); 
	  goto done; 
	case 2: 
	  val = XSUBR (fun)->function (argvals[0], argvals[1]); 
	  goto done; 
	case 3: 
	  val = XSUBR (fun)->function (argvals[0], argvals[1], argvals[2]); 
	  goto done; 
	case 4: 
	  val = XSUBR (fun)->function (argvals[0], argvals[1], argvals[2], argvals[3]); 
	  goto done; 
	case 5: 
	  val = XSUBR (fun)->function (argvals[0], argvals[1], argvals[2], 
					argvals[3], argvals[4]); 
	  goto done; 
	} 
    } 
  if (!LISTP(fun)) 
    Fsignal (Qinvalid_function, Fcons (fun, Qnil)); 
  funcar = Fcar (fun); 
  if (XTYPE (funcar) != Lisp_Symbol) 
    Fsignal (Qinvalid_function, Fcons (fun, Qnil)); 
  if (XSYMBOL (funcar) == XSYMBOL (Qautoload)) 
    { 
      Fload (Fcar (Fcdr (fun)), Qnil, Qnil); 
      goto retry; 
    } 
  if (XSYMBOL (funcar) == XSYMBOL (Qlambda)) 
    val = apply_lambda (fun, original_args, 0); 
  else if (XSYMBOL (funcar) == XSYMBOL (Qmocklisp)) 
    val = ml_apply (fun, original_args); 
  else 
    Fsignal (Qinvalid_function, Fcons (fun, Qnil)); 
 
 done: 
  if (backtrace.debug_on_exit) 
    val = Fapply (Vdebugger, Fcons (Qexit, Fcons (val, Qnil))); 
  backtrace_list = backtrace.next; 
  return val; 
} 
 
/* Call function fn with argument arg */ 
Lisp_Object 
call1 (fn, arg) 
     Lisp_Object fn, arg; 
{ 
  return Ffuncall (2, &fn); 
} 
 
/* Call function fn with arguments arg, arg1 */ 
Lisp_Object 
call2 (fn, arg, arg1) 
     Lisp_Object fn, arg, arg1; 
{ 
  return Ffuncall (3, &fn); 
} 
 
/* Call function fn with arguments arg, arg1, arg2 */ 
Lisp_Object 
call3 (fn, arg, arg1, arg2) 
     Lisp_Object fn, arg, arg1, arg2; 
{ 
  return Ffuncall (4, &fn); 
} 
 
DEFUN ("funcall", Ffuncall, Sfuncall, 1, MANY, 0, 
  "Call first argument as a function, passing remaining arguments to it.\n\ 
Thus,  (funcall 'cons 'x 'y)  returns  (x . y).") 
  (nargs, args) 
     int nargs; 
     Lisp_Object *args; 
{ 
  Lisp_Object fun; 
  Lisp_Object funcar; 
  int numargs = nargs - 1; 
  Lisp_Object lisp_numargs; 
  Lisp_Object val; 
  struct backtrace backtrace; 
  struct gcpro gcpro1; 
  Lisp_Object *internal_args; 
  int i; 
 
  QUIT; 
  if (consing_since_gc > gc_cons_threshold) 
    { 
      GCPRO1 (*args); 
      gcpro1.nvars = nargs; 
      Fgarbage_collect (); 
      UNGCPRO; 
    } 
 
  backtrace.next = backtrace_list; 
  backtrace_list = &backtrace; 
  backtrace.function = &args[0]; 
  backtrace.args = &args[1]; 
  backtrace.nargs = nargs - 1; 
  backtrace.evalargs = 0; 
  backtrace.debug_on_exit = 0; 
 
  if (debug_on_next_call) 
    do_debug_on_call (Qlambda); 
 
 retry: 
 
  fun = args[0]; 
  while (XTYPE (fun) == Lisp_Symbol) 
    fun = Fsymbol_function (fun); 
 
  if (XTYPE (fun) == Lisp_Subr) 
    { 
      if (numargs < XSUBR (fun)->min_args || 
	  (XSUBR (fun)->max_args >= 0 && XSUBR (fun)->max_args < numargs)) 
	{ 
	  XFASTINT (lisp_numargs) = numargs; 
	  Fsignal (Qwrong_number_of_arguments, Fcons (fun, Fcons (lisp_numargs, Qnil))); 
	} 
 
      if (XSUBR (fun)->max_args == UNEVALLED) 
	Fsignal (Qinvalid_function, Fcons (fun, Qnil)); 
 
      if (XSUBR (fun)->max_args == MANY) 
	{ 
	  val = XSUBR (fun)->function (numargs, args + 1); 
	  goto done; 
	} 
 
      if (XSUBR (fun)->max_args > numargs) 
	{ 
	  internal_args = (Lisp_Object *) alloca (XSUBR (fun)->max_args * sizeof (Lisp_Object)); 
	  bcopy (args + 1, internal_args, numargs * sizeof (Lisp_Object)); 
	  for (i = numargs; i < XSUBR (fun)->max_args; i++) 
	    internal_args[i] = Qnil; 
	} 
      else 
	internal_args = args + 1; 
      switch (XSUBR (fun)->max_args) 
	{ 
	case 0: 
	  val = XSUBR (fun)->function (); 
	  goto done; 
	case 1: 
	  val = XSUBR (fun)->function (internal_args[0]); 
	  goto done; 
	case 2: 
	  val = XSUBR (fun)->function (internal_args[0], internal_args[1]); 
	  goto done; 
	case 3: 
	  val = XSUBR (fun)->function (internal_args[0], internal_args[1], internal_args[2]); 
	  goto done; 
	case 4: 
	  val = XSUBR (fun)->function (internal_args[0], internal_args[1], internal_args[2], internal_args[3]); 
	  goto done; 
	case 5: 
	  val = XSUBR (fun)->function (internal_args[0], internal_args[1], 
					internal_args[2], internal_args[3], 
					internal_args[4]); 
	  goto done; 
	} 
    } 
  if (!LISTP(fun)) 
    Fsignal (Qinvalid_function, Fcons (fun, Qnil)); 
  funcar = Fcar (fun); 
  if (XTYPE (funcar) != Lisp_Symbol) 
    Fsignal (Qinvalid_function, Fcons (fun, Qnil)); 
  if (XSYMBOL (funcar) == XSYMBOL (Qlambda)) 
    val = funcall_lambda (fun, numargs, args + 1); 
  else if (XSYMBOL (funcar) == XSYMBOL (Qmocklisp)) 
    val = ml_apply (fun, Flist (numargs, args + 1)); 
  else if (XSYMBOL (funcar) == XSYMBOL (Qautoload)) 
    { 
      Fload (Fcar (Fcdr (fun)), Qnil, Qnil); 
      goto retry; 
    } 
  else 
    Fsignal (Qinvalid_function, Fcons (fun, Qnil)); 
 
 done: 
  if (backtrace.debug_on_exit) 
    val = Fapply (Vdebugger, Fcons (Qexit, Fcons (val, Qnil))); 
  backtrace_list = backtrace.next; 
  return val; 
} 
 
Lisp_Object 
apply_lambda (fun, args, eval_flag) 
     Lisp_Object fun, args; 
     int eval_flag; 
{ 
  Lisp_Object val, tem; 
  Lisp_Object args_left; 
  Lisp_Object numargs; 
  Lisp_Object *arg_vector; 
  int count = specpdl_ptr - specpdl; 
  struct gcpro gcpro1, gcpro2, gcpro3; 
  int i; 
 
  numargs = Flength (args); 
  arg_vector = (Lisp_Object *) alloca (XINT (numargs) * sizeof (Lisp_Object)); 
  args_left = args; 
 
  GCPRO3 (*arg_vector, args_left, fun); 
  gcpro1.nvars = XINT (numargs); 
 
  tem = Fcar (Fcar (Fcdr (fun))); 
  if (EQ (tem, Qand_quote)) eval_flag = 0; 
 
  for (i = 0; i < XINT (numargs); i++) 
    { 
      tem = Fcar (args_left), args_left = Fcdr (args_left); 
      if (eval_flag) tem = Feval (tem); 
      arg_vector[i] = tem; 
    } 
 
  UNGCPRO; 
 
  if (eval_flag) 
    { 
      backtrace_list->args = arg_vector; 
      backtrace_list->nargs = i; 
    } 
  backtrace_list->evalargs = 0; 
  return funcall_lambda (fun, XINT (numargs), arg_vector); 
} 
 
Lisp_Object 
funcall_lambda (fun, nargs, arg_vector) 
     Lisp_Object fun; 
     int nargs; 
     Lisp_Object *arg_vector; 
{ 
  Lisp_Object val, tem; 
  Lisp_Object syms_left; 
  Lisp_Object numargs; 
  Lisp_Object next; 
  int count = specpdl_ptr - specpdl; 
  int i; 
  int optional = 0, rest = 0, quote = 0; 
 
  specbind (Qmocklisp_arguments, Qt);   /* t means NOT mocklisp! */ 
 
  XFASTINT (numargs) = nargs; 
 
  i = 0; 
  for (syms_left = Fcar (Fcdr (fun)); !NULL (syms_left); syms_left = Fcdr (syms_left)) 
    { 
      next = Fcar (syms_left); 
      if (EQ (next, Qand_rest)) 
	rest = 1; 
      else if (EQ (next, Qand_optional)) 
	optional = 1; 
      else if (EQ (next, Qand_quote)) 
	quote = 1; 
      else if (rest) 
	{ 
	  specbind (Fcar (syms_left), Flist (nargs - i, &arg_vector[i])); 
	  i = nargs; 
	} 
      else if (i < nargs) 
        tem = arg_vector[i++], specbind (next, tem); 
      else if (!optional) 
	Fsignal (Qwrong_number_of_arguments, Fcons (fun, Fcons (numargs, Qnil))); 
      else 
	specbind (next, Qnil); 
    } 
 
  if (i < nargs) 
    Fsignal (Qwrong_number_of_arguments, Fcons (fun, Fcons (numargs, Qnil))); 
 
  val = Fprogn (Fcdr (Fcdr (fun))); 
  unbind_to (count); 
  return val; 
} 
 
void 
specbind (symbol, value) 
     Lisp_Object symbol, value; 
{ 
  if (specpdl_ptr == specpdl + specpdl_size) 
    { 
      struct specbinding *old = specpdl; 
      specpdl = (struct specbinding *) realloc (specpdl, (specpdl_size *= 2) * sizeof (struct specbinding)); 
      specpdl_ptr += specpdl - old; 
    } 
  specpdl_ptr->symbol = symbol; 
  specpdl_ptr->old_value = EQ (XSYMBOL (symbol)->value, Qunbound) ? Qunbound : Fsymbol_value (symbol); 
  specpdl_ptr++; 
  Fset (symbol, value); 
} 
 
void 
record_unwind_protect (function, arg) 
     Lisp_Object (*function)(); 
     Lisp_Object arg; 
{ 
  if (specpdl_ptr == specpdl + specpdl_size) 
    { 
      struct specbinding *old = specpdl; 
      specpdl = (struct specbinding *) realloc (specpdl, (specpdl_size *= 2) * sizeof (struct specbinding)); 
      specpdl_ptr += specpdl - old; 
    } 
  XSETTYPE (specpdl_ptr->symbol, Lisp_Internal_Function); 
  XSETFUNCTION (specpdl_ptr->symbol, function); 
  specpdl_ptr->old_value = arg; 
  specpdl_ptr++; 
} 
 
void 
unbind_to (count) 
     int count; 
{ 
  struct specbinding *downto = specpdl + count; 
  int quitf = !NULL (Vquit_flag); 
 
  Vquit_flag = Qnil; 
 
  while (specpdl_ptr != downto) 
    { 
      --specpdl_ptr; 
      /* Note that a "binding" of nil is really an unwind protect, 
	so in that case the "old value" is a list of forms to evaluate.  */ 
      if (NULL (specpdl_ptr->symbol)) 
	Fprogn (specpdl_ptr->old_value); 
      /* a "binding" of a Lisp_Internal_Function (rather than a symbol) 
	means to call that function. 
	This is used when C code makes an unwind-protect.  */ 
      else if (XTYPE (specpdl_ptr->symbol) == Lisp_Internal_Function) 
	XFUNCTION (specpdl_ptr->symbol) (specpdl_ptr->old_value); 
      else 
        Fset (specpdl_ptr->symbol, specpdl_ptr->old_value); 
    } 
  if (NULL (Vquit_flag) && quitf) Vquit_flag = Qt; 
} 
 
/* Get the value of symbol's global binding, even if that binding 
 is not now dynamically visible.  This is used in turning per-buffer bindings on and off */ 
 
DEFUN ("global-value", Fglobal_value, Sglobal_value, 1, 1, "vGlobal value of variable: ", 
  "Return the global value of VARIABLE, even if other bindings of it exist currently.\n\ 
Normal evaluation of VARIABLE would get the innermost binding.") 
  (symbol) 
     Lisp_Object symbol; 
{ 
  struct specbinding *ptr = specpdl; 
 
  CHECK_SYMBOL (symbol, 0); 
  for (; ptr != specpdl_ptr; ptr++) 
    { 
      if (EQ (ptr->symbol, symbol)) 
	return ptr->old_value; 
    } 
  return Fsymbol_value (symbol); 
} 
 
DEFUN ("global-set", Fglobal_set, Sglobal_set, 2, 2, 0, 
  "Set the global binding of VARIABLE to VALUE, ignoring other bindings.\n\ 
Normal setting of VARIABLE with  set  would set the innermost binding.") 
  (symbol, newval) 
     Lisp_Object symbol, newval; 
{ 
  struct specbinding *ptr = specpdl; 
 
  CHECK_SYMBOL (symbol, 0); 
  for (; ptr != specpdl_ptr; ptr++) 
    { 
      if (EQ (ptr->symbol, symbol)) 
	return ptr->old_value = newval; 
    } 
  return Fset (symbol, newval); 
}   
 
DEFUN ("backtrace-debug", Fbacktrace_debug, Sbacktrace_debug, 2, 2, 0, 
  "Set the debug-on-exit flag of eval frame LEVEL levels down to FLAG.\n\ 
The debugger is entered when that frame exits, if the flag is non-nil.") 
  (level, flag) 
     Lisp_Object level, flag; 
{ 
  struct backtrace *backlist = backtrace_list; 
  int i; 
 
  CHECK_NUMBER (level, 0); 
 
  for (i = 0; backlist && i < XINT (level); i++) 
    { 
      backlist = backlist->next; 
    } 
 
  if (backlist) 
    backlist->debug_on_exit = !NULL (flag); 
 
  return flag; 
} 
 
DEFUN ("backtrace", Fbacktrace, Sbacktrace, 0, 0, "", 
  "Print a trace of Lisp function calls currently active.\n\ 
Output stream used is value of standard-output.") 
  () 
{ 
  struct backtrace *backlist = backtrace_list; 
  int i; 
  Lisp_Object tail; 
 
  while (backlist) 
    { 
      write_string (backlist->debug_on_exit ? "* " : "  ", 2); 
      if (backlist->nargs == UNEVALLED) 
        write_string ("(", -1); 
      Fprin1 (*backlist->function, Qnil); 
      if (backlist->nargs == UNEVALLED) 
	{ 
	  if (backlist->evalargs) 
	    write_string (" ...computing arguments...", -1); 
	  else 
	    write_string (" ...", -1); 
	} 
      else if (backlist->nargs == MANY) 
	{ 
	  write_string ("(", -1); 
	  for (tail = *backlist->args, i = 0; !NULL (tail); tail = Fcdr (tail), i++) 
	    { 
	      if (i) write_string (" ", -1); 
	      Fprin1 (Fcar (tail), Qnil); 
	    } 
	} 
      else 
	{ 
	  write_string ("(", -1); 
	  for (i = 0; i < backlist->nargs; i++) 
	    { 
	      if (i) write_string (" ", -1); 
	      Fprin1 (backlist->args[i], Qnil); 
	    } 
	} 
      write_string (")\n", -1); 
      backlist = backlist->next; 
    } 
  return Qnil; 
} 
 
syms_of_eval () 
{ 
  DefLispVar ("quit-flag", &Vquit_flag, 
    "Non-nil causes  eval  to abort, unless  inhibit-quit  is non-nil.\n\ 
Typing C-G sets  quit-flag  non-nil."); 
  Vquit_flag = Qnil; 
 
  DefLispVar ("inhibit-quit", &Vinhibit_quit, 
    "Non-nil inhibits C-G quitting."); 
  Vinhibit_quit = Qnil; 
 
  Qautoload = intern ("autoload"); 
  staticpro (&Qautoload); 
 
  Qmacro = intern ("macro"); 
  staticpro (&Qmacro); 
 
  Qexit = intern ("exit"); 
  staticpro (&Qexit); 
 
  Qinteractive = intern ("interactive"); 
  staticpro (&Qinteractive); 
 
  Qcommandp = intern ("commandp"); 
  staticpro (&Qcommandp); 
 
  Qdefun = intern ("defun"); 
  staticpro (&Qdefun); 
 
  Qand_rest = intern ("&rest"); 
  staticpro (&Qand_rest); 
 
  Qand_optional = intern ("&optional"); 
  staticpro (&Qand_optional); 
 
  Qand_quote = intern ("&quote"); 
  staticpro (&Qand_quote); 
 
  DefBoolVar ("stack-trace-on-error", &stack_trace_on_error, 
    "*Non-nil means automatically display a backtrace buffer\n\ 
after any error that is handled by the editor command loop."); 
  stack_trace_on_error = 0; 
 
  DefBoolVar ("debug-on-error", &debug_on_error, 
    "*Non-nil means enter debugger if an error is signaled.\n\ 
Does not apply to errors handled by condition-case.\n\ 
See also variable debug-on-quit."); 
  debug_on_error = 0; 
 
  DefBoolVar ("debug-on-quit", &debug_on_quit, 
    "*Non-nil means enter debugger if quit is signaled (C-G, for example).\n\ 
Does not apply if quit is handled by a condition-case."); 
  debug_on_quit = 0; 
 
  DefBoolVar ("debug-on-next-call", &debug_on_next_call, 
    "Non-nil means enter debugger before next eval, apply or funcall."); 
 
  DefLispVar ("debugger", &Vdebugger, 
    "Function to call to invoke debugger.\n\ 
If due to frame exit, args are 'exit and value being returned;\n\ 
 this function's value will be returned instead of that.\n\ 
If due to error, args are 'error and list of signal's args.\n\ 
If due to apply or funcall entry, one arg, 'lambda.\n\ 
If due to eval entry, one arg, 't."); 
  Vdebugger = Qnil; 
 
  Qmocklisp_arguments = intern ("mocklisp-arguments"); 
  staticpro (&Qmocklisp_arguments); 
  DefLispVar ("mocklisp-arguments", &Vmocklisp_arguments, 
    "While in a mocklsp function, the list of its unevaluated args."); 
  Vmocklisp_arguments = Qt; 
 
  defsubr (&Sor); 
  defsubr (&Sand); 
  defsubr (&Sif); 
  defsubr (&Scond); 
  defsubr (&Sprogn); 
  defsubr (&Sprog1); 
  defsubr (&Sprog2); 
  defsubr (&Ssetq); 
  defsubr (&Sglobal_set); 
  defsubr (&Sglobal_value); 
  defsubr (&Squote); 
  defsubr (&Sfunction); 
  defsubr (&Sdefun); 
  defsubr (&Sdefmacro); 
  defsubr (&Sdefvar); 
  defsubr (&Sdefconst); 
  defsubr (&Slet); 
  defsubr (&SletX); 
  defsubr (&Swhile); 
  defsubr (&Smacroexpand); 
  defsubr (&Scatch); 
  defsubr (&Sthrow); 
  defsubr (&Sunwind_protect); 
  defsubr (&Scondition_case); 
  defsubr (&Ssignal); 
  defsubr (&Sinteractive_p); 
  defsubr (&Scommandp); 
  defsubr (&Sautoload); 
  defsubr (&Seval); 
  defsubr (&Sapply); 
  defsubr (&Sfuncall); 
  defsubr (&Sbacktrace_debug); 
  defsubr (&Sbacktrace); 
} 
