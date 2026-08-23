// RUN: %gta3sc-verify %s --config=gta3 -fsyntax-only

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