mofcomp: wmi42.bmf

wmi42.bmf: $*.mof
	mofcomp -b:$@ $*.mof
	wmimofck -h$*.h $@