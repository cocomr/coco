import sys,glob,subprocess


comps = []
for x in sys.argv[1:]:
	for y in glob.glob(x):
		if y.endswith(".a") or y.endswith(".lib"):
			try:
				r = subprocess.check_output(["nm",y],shell=False).split("\n")
				for z  in r:
					z = z.strip()
					if z.endswith("_spec"):
						a = z.split(" ",2)[2]
						if a[0] == "_":
							comps.append(a[1:-5])
							z[0] == "_" and z.split("\n")	
			except Exception, e:
				raise
			else:
				pass
			finally:
				pass
print comps
print "# Linker Flag"
print " ".join(["-u _%s_spec" % x for x in comps])

print 
print "//Code"
for c in comps:
	print "extern coco::ComponentSpec %s;" % c
print "coco::ComponentSpec * specs[] = {"
for c in comps:
	print "&%s_spec," % c
print "};"