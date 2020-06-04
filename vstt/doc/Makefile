doc.pdf : doc.tex
	pdflatex doc
	pdflatex doc
	pdflatex doc

v : doc.pdf
	evince doc.pdf

e :
	pluma doc.tex || gedit doc.tex

clean :
	rm -f doc.aux doc.log *~ doc.toc

