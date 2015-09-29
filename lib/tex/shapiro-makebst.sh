#! /bin/sh
PATH=/lib:$PATH
export PATH
set -e
set -x

cpp -P -DPLAIN shapiro-btxbst-0.98.doc | sed -e '/^$/d' > plain.bst
cpp -P -DUNSRT shapiro-btxbst-0.98.doc | sed -e '/^$/d' > unsrt.bst
cpp -P -DALPHA shapiro-btxbst-0.98.doc | sed -e '/^$/d' > alpha.bst
cpp -P -DABBRV shapiro-btxbst-0.98.doc | sed -e '/^$/d' > abbrv.bst
cpp -P -DFRENCH -DPLAIN shapiro-btxbst-0.98.doc | sed -e '/^$/d' > fplain.bst
cpp -P -DFRENCH -DUNSRT shapiro-btxbst-0.98.doc | sed -e '/^$/d' > funsrt.bst
cpp -P -DFRENCH -DALPHA shapiro-btxbst-0.98.doc | sed -e '/^$/d' > falpha.bst
cpp -P -DFRENCH -DABBRV shapiro-btxbst-0.98.doc | sed -e '/^$/d' > fabbrv.bst
cpp -P -DALPHA3 shapiro-btxbst-0.98.doc | sed -e '/^$/d' > alpha3.bst
cpp -P -DFRENCH -DALPHA3 shapiro-btxbst-0.98.doc | sed -e '/^$/d' > falpha3.bst
cpp -P -DLONG shapiro-btxbst-0.98.doc | sed -e '/^$/d' > long.bst
cpp -P -DFRENCH -DLONG shapiro-btxbst-0.98.doc | sed -e '/^$/d' > flong.bst
cpp -P -DKEY shapiro-btxbst-0.98.doc | sed -e '/^$/d' > key.bst
cpp -P -DFRENCH -DKEY shapiro-btxbst-0.98.doc | sed -e '/^$/d' > fkey.bst
cpp -P -DSKEY shapiro-btxbst-0.98.doc | sed -e '/^$/d' > skey.bst
cpp -P -DFRENCH -DSKEY shapiro-btxbst-0.98.doc | sed -e '/^$/d' > fskey.bst
