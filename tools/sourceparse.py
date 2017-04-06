import re
import argparse
import os
from os.path import join, getsize
import sys

class Item:
	def __init__(self,nature,name,type):
		self.nature = nature
		self.name = name
		self.type = type
	def __repr__(self):
		return "%s %s %s" % (self.nature,self.type,self.name)
class Component:
	def __init__(self,classname,base):
		self.items = []
		self.classname = classname
		self.base = base
		self.registrations = []
	def __repr__(self):
		s = "Class "+self.classname+" from "+ self.base+(" with %d " % len(self.items)) + " with regs:"+" ".join(self.registrations)
		for x in self.items:
			s += x.__repr__()
		return s

def main():
	natures = ""
	components = {}
	lastcomponent = None
	for root, dirs, files in os.walk(sys.argv[1]):
		# first includes
		hfiles = [h for h in files if h.endswith(".h")]
		cfiles = [h for h in files if h.endswith(".cpp")]
		for f in hfiles+cfiles:
			if f.endswith(".cpp") or f.endswith(".h"):
				fp = join(root,f)
				x = open(fp,"r")
				foundtype = None
				needed = ""
				waitingrest = None
				for line in x:
					line = line.strip()
					donow = False
					if line.startswith("//"):
						continue
					if line.startswith("COCO_REGISTER("):
						cz = line.split("(")[1].rstrip().rstrip(")")
						print "register",cz
						components[cz].registrations.append(cz)
					elif line.startswith("COCO_REGISTER_ALIAS("):
						cz,alias = line.split("(")[1].rstrip().rstrip(")").split(",")
						print "register",cz,alias
						components[cz].registrations.append(alias)
					elif line.startswith("class"):
						m = re.match("class\s+([^:]+):\s*public\s+([:A-Za-z_0-9]+)",line)
						if m:
							name = m.group(1).strip()
							base = m.group(2)
							print "class",name,"base",base
							if "TaskContext" in base or "PeerTask"in base or base in components:
								components[name] = Component(name,base)
								lastcomponent = components[name]
							else:
								print "class with base",base
						else:
							print "classnomatch",line
					else:
						if waitingrest is not None:
							waitingrest += line
							if needed in line:
								donow = True
							else:
								waitingrest = None
						else:
							if "coco::" in line:
								foundtype = None
								for x in ["coco::Attribute","coco::Operation","coco::InputPort","coco::OutputPort"]:
									if x in line:
										foundtype = x
										print "foundtype",x
										break
								if foundtype:
									if "{this" in line and "}" in line:
										donow = True
										waitingrest = line
									elif "{this" in line:
										needed = "}"
									else:
										waitingrest = line
										needed = "{this"
								else:
									print "skip",line
						if donow:
							#print "\ttry",waitingrest
							type = waitingrest.split("<",1)[1].split("=")[0].rstrip()
							ls = type.rfind(" ")
							type = type[0:ls][0:-1].rstrip()
							name = waitingrest.split("{this",1)[1].split("\"")[1]
							print "\tfound",foundtype[6:],"<",type,">",name
							lastcomponent.items.append(Item(foundtype[6:],name,type))
							waitingrest = None
	for c in components.values():
		print c
if __name__ == '__main__':
	main()