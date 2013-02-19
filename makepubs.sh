#!/bin/bash

TMPDIR=. bibtex2html -nobibsource -noheader -nofooter -nodoc -unicode -dl -nokeys -d -r scal.bib

# publications -> publications-bib links
#perl -pi -e 's/mlippautz_bib.html/\{\{ urls.base_path \}\}publications-bib/g' mlippautz.html

# publications-bib -> publications links
#perl -pi -e 's/mlippautz.html/\{\{ urls.base_path \}\}publications/g' mlippautz_bib.html

# disable code highlighting in bib code listings
#perl -pi -e 's/<pre>/<pre class="no-highlight">/g' mlippautz_bib.html
#perl -pi -e 's/<pre>/<pre class="no-highlight">/g' mlippautz_other_bib.html

# remove headings
#perl -pi -e 's/<h1>mlippautz.bib<\/h1>//g' mlippautz_bib.html
#perl -pi -e 's/<h1>mlippautz_other.bib<\/h1>//g' mlippautz_other_bib.html

