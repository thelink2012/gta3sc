// RUN: %dis %gta3sc %s --config=gta3 -fsyntax-only 2>&1 | %verify %s

// expected-no-diagnostics

 (,,
),
,(  ()),)         )(            ((),)   ()
,,))) )(         ,,,,           ,), (,

(
)
(        ,, ),(()
(
        ,(() (  )
(,      ,,,
,

TERMINATE_THIS_SCRIPT

// To generate the random tokens:
// tr -dc "(), \n\t" < /dev/urandom | head -c 100