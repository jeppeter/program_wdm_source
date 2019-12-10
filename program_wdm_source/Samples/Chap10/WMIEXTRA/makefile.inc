mofcomp: wmiextra.bmf

wmiextra.bmf: $*.mof
	mofcomp -b:$@ $*.mof
	wmimofck -h$*.h -m $@