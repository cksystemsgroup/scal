#!/bin/bash

# Call from base directory:
#   _contrib/makepubs.sh


TMPDIR=. bibtex2html -nobibsource -noheader -nofooter -nodoc -unicode -dl -nokeys -d -r scal.bib

cp scal.bib scal.bib.txt

