(1)$ (1)$ Richard M. Stallman, Linus Thorvalds, and
Donald E. Knuth engage in a discussion on
whose impact on the computerized world was
the greatest.

Stallman: "God told me I have programmed the
best editor in the world!"

Thorvalds: "Well, God told *me* that I have
programmed the best operating system in
the world!"

Knuth: "Wait, wait - I never said that."
(2)$ (2)$ 		Cottam's Good C Style Guide$$$$$
		===========================$$$$$
$$$$$
Note:  Advice that I, and a few others, have given but has been universally$$$$$
       ignored is indicated by a + rather than a * in the list below!$$$$$
$$$$$
$$$$$
* split programs over several files for ``information hiding''$$$$$
$$$$$
* choose a personal/project standard for layout and stick to it$$$$$
$$$$$
* do not rely on defaults, e.g. extern/static variables being set to 0$$$$$
  [this is for readability -- I am not concerned with non-C compilers]$$$$$
$$$$$
* use void when you are writing a non-value returning function$$$$$
  (not the default int)$$$$$
$$$$$
* cast the return values of functions to void if you are certain that$$$$$
  their result is not needed$$$$$
$$$$$
* use lint (if you have it) as a matter of course$$$$$
  (not just after 10 hours debugging)$$$$$
$$$$$
* write portable C (Kernel level code, e.g. drivers, excepting.  Put$$$$$
  all such code in a separate file.)$$$$$
$$$$$
* use standard I/O (i.e. don't use UNIX specific I/O without good reason)$$$$$
$$$$$
* don't use a macro if a function call is acceptably efficient$$$$$
$$$$$
* avoid complex macros$$$$$
$$$$$
* don't use complex conditional expressions especially ones returning$$$$$
  non-arithmetic values$$$$$
$$$$$
* use typedef to build up (very complex) declarations piecemeal$$$$$
$$$$$
* use enumerations and #define for constant names$$$$$
  (enum is preferable to #define, especially with ANSI C)$$$$$
$$$$$
* always check the error status of a system call and C library functions$$$$$
  if impossible to prove exception can not occur$$$$$
$$$$$
* use header files, e.g. <stdio.h>, rather than assume, e.g., -1 == EOF$$$$$
$$$$$
* use casts rather than rely on representational coincidences (see also$$$$$
  comments on use of lint and portability)$$$$$
$$$$$
* only have one copy of global declarations in a .h file$$$$$
  (of course there may be many header files)$$$$$
$$$$$
* do not embed absolute pathnames in program code (at least #define them)$$$$$
  put the strings in a separate file for ease of change and to minimise$$$$$
  recompilation$$$$$
$$$$$
* use static for variables and functions that are local to a file$$$$$
$$$$$
* use the ``UNIX standard'' for command line argument syntax and getopt()$$$$$
  for parsing$$$$$
$$$$$
* don't use the preprocessor to make C look like anything but C$$$$$
$$$$$
+ show the difference between = and == by using asymmetric layout$$$$$
  for assignment (e.g. ptr= NULL) and symmetric layout for equality$$$$$
  (e.g. ptr == NULL).$$$$$
$$$$$
+ use  expr == var  to emphasise and catch some = for == mistakes$$$$$
  (e.g. x*2 == y)$$$$$
$$$$$
+ avoid EXCESSIVE use of multiple exits from a context$$$$$
  (but be realistic about this when using  switch  statements)$$$$$
$$$$$
+ don't use assignment inside a complex expression$$$$$
  (e.g. use (chptr= malloc(N), chptr != NULL)$$$$$
	rather than$$$$$
	    ((chptr = malloc(N)) != NULL)$$$$$
  [but note that multiple assignments in statement context is no problem$$$$$
   e.g. x= y= z= 0;]$$$$$
$$$$$
+ avoid #ifdefs for version/revision control purposes (use a proper version$$$$$
  control system)$$$$$
$$$$$
+ use side-effect operations in statement context only$$$$$
  (exception: the comma operator)$$$$$
$$$$$
+ use local blocks to indicate the extent of variables and comments, e.g.$$$$$
	{ /* this does foo */$$$$$
		int foovar;$$$$$
		/* stuff for foo */$$$$$
	}$$$$$
	{ /* this does bar */$$$$$
		...etc...$$$$$
	}$$$$$
$$$$$
___________________________________________________________________$$$$$
$$$$$
$$$$$
(This is a small revision to the list I posted recently.  I would like to$$$$$
 thank the following for their constructive comments: P.Crowther,$$$$$
 R.Tobin, D.Jones, R.Calbridge, T.Stockfisch, C.Forsyth, A.Jonasson,$$$$$
 B.Todd, H.Spencer, M.Sullivan, J.Dzikiewicz, and B.Needham)$$$$$
$$$$$
A few meta-comments are in order judging from the criticisms I have$$$$$
received.$$$$$
 1) This is a tiny part of my "C course notes".  Other sections discuss$$$$$
    generally applicable programming habits and "C pitfalls" etc.  This$$$$$
    list will always be terse and reflect my bias towards demonstratively$$$$$
    correct (w.r.t formal spec) programs irrespective of the implementation$$$$$
    language.  (Not something that is as popular in the USA as Europe :-) )$$$$$
    It is (largely) C specific -- I have even removed the "don't assume$$$$$
    ASCII" because that it is applicable to all languages.$$$$$
$$$$$
 2) Several people said the points about asymmetric layout of the assignment$$$$$
    operator versus symmetric layout of equality were interesting, but they$$$$$
    had never seen it used and therefore they would not start now.  The$$$$$
    same was said about my support of the comma operator to avoid side$$$$$
    effects in expression context (see also point (1) above).  Because I$$$$$
    program *into* several different languages every day (Aside: I have$$$$$
    been using C on and off for 10 years for those people that wondered$$$$$
    if I was new to it! :-) ) I know that, without a discipline, I am$$$$$
    quite likely, for example, to use = to mean equality.$$$$$
    I dislike the use of the conventional equality sign for assignment,$$$$$
    but then it is only sugar upon the abstract syntax.$$$$$
    Since adopting the conventions below I have never made the =/== slip.$$$$$
    Arguments such as "I have never seen this done before" or "Dennis$$$$$
    doesn't use it" have never carried much weight with me [:-)].$$$$$
$$$$$
    The view that tools such as modified-lints or the recently posted check$$$$$
    program should be used instead is a valid alternative.$$$$$
    However, I occasionally use non-UNIX systems without lint;$$$$$
    I also prefer not to make mistakes in the first place.$$$$$
$$$$$
 3) My deprecation of side effect operations together with my use of the$$$$$
    comma operator is to:$$$$$
	a) permit Hoare logic style reasoning (when required);$$$$$
	b) to follow the advice, often posted but rarely followed, about$$$$$
	   the dangers of premature optimisation;$$$$$
	   [Aside: I fully support the use of side effects for extreme$$$$$
	    efficiency in critical loops as recently demonstrated in$$$$$
	    the staggeringly impressive version of egrep that was posted.$$$$$
	    Such examples are not so common!$$$$$
	    Test:  remove all occurences of the$$$$$
	    word register from your favourite program and measure the$$$$$
	    speed degradation.]$$$$$
	c) make the code more readable to "casual C users";$$$$$
	d) simply prevent blunders I know myself to be all to capable$$$$$
	   of making [e.g. missing the extra brackets in the awful$$$$$
	   idiom ((x= call()) != 0)].$$$$$
$$$$$
I will not post the list again and consider the discussion closed.$$$$$
Advice is there to be ignored!  Or, to quote Monty to his batman:$$$$$
``When I require the services of a sex consultant -- I will ask you.''$$$$$
(admission: this is a deliberate misquote in case children are watching :-) )$$$$$
(3)$ End of test
(4)$ 
