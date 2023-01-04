" Vim syntax file
" Language: ALAN-2019
" Maintainer: W. H. K. Bester (whkbester@cs.sun.ac.za)
" Latest Revision: 3 August 2022

if exists("b:current_syntax")
	finish
endif

" keywords
syn keyword	alanBoolean				false true
syn keyword	alanConditional			else elsif if
syn keyword	alanFunction			get put
syn keyword	alanOperator			and not or
syn keyword	alanOperator			= <> < > <= >= + - * / rem := .
syn keyword	alanRepeat				while
syn keyword	alanBlockStatement		begin do end then
syn keyword	alanDefineStatement		function source to
syn keyword	alanStatement			call leave relax
syn keyword	alanType				array boolean integer

" literals
syn match	alanNumber				"-\?\d\+"
syn match	alanIdentifier			"\<[a-zA-Z_][a-zA-Z0-9_]*\>"
syn match	alanUserFunction		/\<[a-zA-Z_][a-zA-Z0-9_]*\s*(/me=e-1,he=e-1
syn region	alanString matchgroup=alanString start=+"+ skip=+\\.+ end=+"+

" comments
syn region	alanComment start="{" end="}" contains=alanTodo,alanComment
syn keyword	alanTodo contained		TODO FIXME XXX DEBUG NOTE HBD
syn match	alanTodo contained		"HERE BE DRAGONS"
syn match	alanTodo contained		"HIC SVNT DRACONES"
" Anyone who alleges a Harry Potter or Game of Thrones reference, will be
" eaten by said dragons.  Fiere potest vt cerebrum tvvm liquefiat.  On the
" other hand: tugh qoH nachDaj je chevlu'ta'.

" associations
let b:current_syntax = "alan"

" The following is a bit colourful, rather like what SublimeText fanboys are
" used to.  Do feel free to tune to tase.  (And if you are a SublimeText fan:
" I bite my thumb at you, and a plague on your house!)
"
hi def link	alanBlockStatement		Statement
hi def link	alanBoolean				Boolean
hi def link	alanComment				Comment
hi def link	alanConditional			Conditional
hi def link	alanDefineStatement		Statement
hi def link	alanFunction			Function
" Uncommenting the following makes it *really* colourful.
" hi def link	alanIdentifier			Identifier
hi def link	alanNumber				Number
hi def link	alanRepeat				Repeat
hi def link	alanTodo				Todo
hi def link	alanType				Type
hi def link alanOperator			Operator
hi def link alanStatement			Keyword
hi def link alanString				String
hi def link alanUserFunction		Identifier

" vim: ts=4 sw=2:
