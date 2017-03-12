#!/usr/bin/env python
#
# Python CoCo launcher with replacement syntax of ROS Launcher (2017)
#
# Future: support includes 
# Future: implement find
import re
import argparse
import xml.etree.ElementTree as ET
import os
import time
import subprocess

def xenv(a):
	return os.environ[a]
def xoptenv(a):
	a,b = a.split(" ")
	return os.environ.get(a,b)
def xarg(a,eargs):
	return eargs.get(a)
def xfind(a):
	return ""
def xeval(a,gg):
	return eval(a,globals=gg)
def xanon(n):
	return n+str(time.time())
def fixme(m,eargs,gg):
	what = m.group(1)
	rest = m.group(2).strip()
	fx = gg.get(what)
	if fx is not None:
		return fx(rest)
	else:
		return ""
def evaltrue(s):
	if s == "true" or s == "1":
		return True
	elif s == "false" or s == "0":
		return False
	return eval(s)

resub = re.compile(r'\$\(([a-z]+)([^\)]*)\)')
def process(et,eargs,gg):

	changed = False

	afx = lambda m: fixme(m,eargs,gg)
	# first replace
	newvals = []
	for k,v in et.attrib.iteritems():
		if v.find("$(") >= 0:
			print "!",et.tag,k
			newvals.append((k,resub.sub(afx,v)))
			changed = True
	for k,v in newvals:
		et.attrib[k] = v

	if "if" in et.attrib:
		s = evaltrue(et.attrib["if"])
		changed = True
		if not s:
			# exclude		
			et.tag = "_" + et.tag	
			return True
	elif "unless" in et.attrib:
		s = evaltrue(et.attrib["unless"])
		if s:
			et.tag = "_" + et.tag	
			return True
	if et.tag == "arg":
		if "value" in et.attrib:
			eargs[et.attrib["name"]] = et.attrib["value"]
	# for all children
	for c in et.iter():
		if c != et:
			cc = process(c,eargs,gg)
			changed = changed or cc
	t = et.text
	if t is not None and t.find("$(") >= 0:
		et.text = resub.sub(afx,t)
		changed = True

	if et.tag == "include":
		raise "include not ready"
		infile = et.attrib["file"]
		print "loading include infile",infile
		tree = ET.parse(x)
		seargs = {}
		for p in et.findall("param"):
			seargs[p.attrib["name"]] = p.attrib["value"]
		g = g.copy()
		# update
		g["arg"] = lambda x: xarg(x,seargs)
		g["eval"] = lambda x: xeval(x,g)
		c = process(tree.getroot(),seargs,g)

		# proceed by replacing the content


	return True	
def main():
	parser = argparse.ArgumentParser(description='CoCo Launcher X')
	parser.add_argument('-x','--config-file',action="append",help="Xml file with the configurations of the application")
	parser.add_argument('-d','--disabled',action="append",help="List of components disabled in the execution")	
	parser.add_argument('-p','--profiling',type=int,help="Enable the collection of statistics of the executions. Use only during debug as it slow down the performances.")
	parser.add_argument('-g','--graph',help="Create the graph of the various components and of their connections.")
	parser.add_argument('-t','--xml_template',help="Print the xml template for all the components contained in the library")
	parser.add_argument('-w','--web_server', help="Instantiate a web server that allows to view statics about the executions.",type=int,default=7707)
	parser.add_argument('-r','--web_root',help="set document root for web server")
	parser.add_argument('-l','--latency',help="Set the two task between which calculate the latency. Peer are not valid.")
	parser.add_argument('--ros',action="store_true",help="invokes ros_coco_launcher")
	parser.add_argument('--none',action="store_true",help="just prints arguments")
	parser.add_argument('arg',help="Arguments as x:=y",nargs="*")
	args = parser.parse_args()

	eargs = {}
	for aa in args.arg:		
		a,b = aa.split(":=")
		eargs[a] = b

	g = dict(arg=lambda x: xarg(x,eargs),env=xenv,optenv=xoptenv,find=xfind,anon=xanon)
	g["eval"] = lambda x: xeval(x,g)

	launcher = "ros_coco_launcher" if args.ros else "coco_launcher"

	oa = []
	for x in args.config_file:
		tree = ET.parse(x)
		# attribute replace
		c = process(tree.getroot(),eargs,g)
		if c:
			# if/unless
			xx = x + ".gen"
			open(xx,"wb").write(ET.tostring(tree.getroot()))
		else:
			xx = x 
		oa.append(xx)

	args.config_file = oa
	goodargs = [launcher]
	# THIS IS MANUAL sorry
	for l in args.config_file:
		goodargs.append("-x")
		goodargs.append(l)
	if args.disabled != None:
		for l in args.disabled:
			goodargs.append("-d")
			goodargs.append(l)
	aa = ["profiling","graph","web_root","web_server","latency","xml_template"]
	for a in aa:
		w = getattr(args,a)
		if w is not None:
			goodargs.append("--" + a)
			goodargs.append(str(w))
	if args.profiling:
		pass


	# Build Arguments: ignore free standing args, keep the rest
	#goodargs = [getattr(x)]
	if not args.none:
		subprocess.call(goodargs)
	else:
		print goodargs


if __name__ == '__main__':
	main()